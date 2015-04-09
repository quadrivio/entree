//
//  train.cpp
//  entree
//
//  Created by MPB on 5/7/13.
//  Copyright (c) 2013 Quadrivio Corporation. All rights reserved.
//  License http://opensource.org/licenses/BSD-2-Clause
//          <YEAR> = 2013
//          <OWNER> = Quadrivio Corporation
//

//
// Create ensemble of decision trees from training data
//

#include "train.h"

#include "csv.h"
#include "prune.h"
#include "subsets.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>

using namespace std;

#undef DUMP_VALUES  // for debugging

// ========== Local Types ==========================================================================

// contains a candidate split value, and measure of results quality when the split value is used
struct ValueAndMeasure {
    Value value;
    double measure;
};
typedef struct ValueAndMeasure ValueAndMeasure;

// ========== Local Headers ========================================================================

// create one decision tree using the specified subset of columns of the Values array
void evaluateTree(CompactTree& compactTree,
                  int maxDepth,
                  int maxNodes,
                  int& maxDepthUsed,
                  bool doPrune,
                  double minImprovement,
                  index_t minLeafCount,
                  index_t maxSplitsPerNumericAttribute,
                  const vector< vector<Value> >& values,
                  const vector<ValueType>& valueTypes, 
                  const vector<CategoryMaps>& categoryMaps,
                  const vector<size_t>& subsetIndexes,
                  const SelectIndexes& selectRows,
                  const SelectIndexes& selectColumns,
                  size_t targetColumn,
                  const vector< vector<size_t> >& sortedIndexes,
                  const vector<string>& colNames,
                  const vector<Value>& imputedValues);

// try improving the decision tree by splitting the specified leaf node; if success, return true;
// if not, return false and leave leaf unsplit
bool improveLeaf(TreeNode *nodeP,
                 const vector<size_t>& subsetIndexes,
                 const vector< vector<Value> >& values,
                 const vector<ValueType>& valueTypes,
                 const vector<CategoryMaps>& categoryMaps,
                 const SelectIndexes& selectColumns,
                 size_t targetColumn,
                 const vector< vector<size_t> >& sortedIndexes,
                 const vector<string>& colNames,
                 double minImprovement,
                 index_t minLeafCount,
                 index_t maxSplitsPerNumericAttribute,
                 const vector<Value>& imputedValues);

// recursively improve subtree from specified leaf node (called initially on the root node)
void improveSubtree(TreeNode *nodeP,
                    int depth,
                    int maxDepth,
                    int maxNodes,
                    int& maxDepthUsed,
                    const vector<size_t>& subsetIndexes,
                    const vector< vector<Value> >& values,
                    const vector<ValueType>& valueTypes,
                    const vector<CategoryMaps>& categoryMaps,
                    const SelectIndexes& selectColumns,
                    size_t targetColumn,
                    const vector< vector<size_t> >& sortedIndexes,
                    const vector<string>& colNames,
                    double minImprovement,
                    index_t minLeafCount,
                    index_t maxSplitsPerNumericAttribute,
                    index_t& finalLeafCount,
                    const vector<Value>& imputedValues);

// try to improve leaf that has potential for improvement (i.e., not already perfect)
bool improveImperfectLeaf(TreeNode *nodeP,
                          const vector<size_t>& subsetIndexes,
                          const vector< vector<Value> >& values,
                          const vector<ValueType>& valueTypes,
                          const vector<CategoryMaps>& categoryMaps,
                          const SelectIndexes& selectColumns,
                          size_t targetColumn,
                          const vector< vector<size_t> >& sortedIndexes,
                          const vector<string>& colNames,
                          double minImprovement,
                          index_t minLeafCount,
                          index_t maxSplitsPerNumericAttribute,
                          const vector<Value>& imputedValues);

// recursively count all nodes in the subtree beginning at specified node
// (if leaf node then count = 1)
size_t countNodes(const TreeNode *nodeP);

// recursively assign a serial number to all nodes in the subtree beginning at specified node 
void indexNodes(TreeNode *nodeP);

// create a CompactTree from the decision tree beginning at the specified root node
void makeCompactTree(CompactTree& compactTree, TreeNode& root);

// recursively copy the subtree beginning at the specified node to the CompactTree struct
void copyToCompact(CompactTree& compactTree, TreeNode *nodeP);

// for debugging; print tree
void printTree(const TreeNode *nodeP,
               const vector< vector<Value> >& values,
               const vector<ValueType>& valueTypes,
               size_t targetColumn,
               const SelectIndexes& selectColumns,
               const vector<CategoryMaps>& categoryMaps,
               const vector<string>& colNames,
               int indent, index_t count);

// calculate entropy for selected set of response values; return vector of category counts
double selectRowsEntropy(const SelectIndexes& selectRows,
                         const vector< vector<Value> >& values,
                         size_t targetColumn,
                         const vector<CategoryMaps>& categoryMaps,
                         vector<int>& targetCategoryCounts);

// calculate entropy for set of category counts
double entropyForCounts(const vector<int>& targetCategoryCounts);

// calculate entropy for a binary split of counts
double entropyForSplit(const vector<int>& lessThanOrEqualCounts, const vector<int>& totalCounts);

// calculate standard deviation for a selected set of response values
double selectRowsSd(const SelectIndexes& selectRows,
                    const vector< vector<Value> >& values,
                    size_t targetColumn);

// calculate standard deviation from statistics
double stDev(int count, double sum, double sum2);

// calculate weighted standard deviation for a binary split of values
double sdForSplit(double lessThanOrEqualSum,
                  double lessThanOrEqualSum2,
                  int lessThanOrEqualCount,
                  double totalSum,
                  double totalSum2,
                  int totalCount);

// get the best split for the specified numeric column
ValueAndMeasure getBestNumericalSplit(size_t col,
                                      size_t targetColumn,
                                      const SelectIndexes& selectRows,
                                      const vector< vector<Value> >& values,
                                      const vector<ValueType>& valueTypes,
                                      const vector<CategoryMaps>& categoryMaps,
                                      const vector< vector<size_t> >& sortedIndexes,
                                      const vector<string>& colNames);

// get the best split for the specified categorical column
ValueAndMeasure getBestCategoricalSplit(size_t col,
                                        size_t targetColumn,
                                        const SelectIndexes& selectRows,
                                        const vector< vector<Value> >& values,
                                        const vector<ValueType>& valueTypes,
                                        const vector<CategoryMaps>& categoryMaps,
                                        const vector< vector<size_t> >& sortedIndexes,
                                        const vector<string>& colNames);

// ========== Globals ========================================================================

namespace ns_train {
    // for debugging
#if 0
    bool gVerbose = true;    
    bool gVerbose1 = true;    
    bool gVerbose2 = true;    
    bool gVerbose3 = true; 
    bool gVerbose4 = true; 
#else
    bool gVerbose = false;    
    bool gVerbose1 = false;    
    bool gVerbose2 = false;    
    bool gVerbose3 = false; 
    bool gVerbose4 = false; 
#endif
};

using namespace ns_train;

size_t gNextIndex = 0;  // TODO fix for thread-safety

// ========== Functions ============================================================================

// train ensemble of decision trees
void train(std::vector<CompactTree>& trees, 
           index_t columnsPerTree,
           int maxDepth,
           int minDepth,
           bool doPrune,
           double minImprovement,
           index_t minLeafCount,
           index_t maxSplitsPerNumericAttribute,
           index_t maxTrees,
           index_t maxNodes,
           const SelectIndexes& selectRows,
           const SelectIndexes& availableColumns,
           SelectIndexes& selectColumns,
           std::vector< std::vector<Value> >& values,
           const std::vector<ValueType>& valueTypes,
           std::vector<CategoryMaps>& categoryMaps,
           size_t targetColumn,
           const std::vector<std::string>& colNames,
           std::vector<ImputeOption>& imputeOptions)
{
    if (gVerbose) {
        CERR << "train(maxDepth = " << maxDepth << ", minDepth = " << minDepth <<
        ", maxTrees = " << maxTrees  << ", maxNodes = " << maxNodes <<
        ", columnsPerTree = " << columnsPerTree << ", prune = " << (doPrune ? 1 : 0) <<
        ", minImprovement = " <<
        fixed << setprecision(2) << minImprovement << ", minLeafCount = " << minLeafCount << 
        ", selectRows = " << selectRows.countSelected() <<
        ", availableColumns = " << availableColumns.countSelected() << ")" << endl;
        
        CERR << "values.size() = " << values.size() << endl;
        CERR << "values[0].size() = " << values[0].size() << endl;
        CERR << "valueTypes.size() = " << valueTypes.size() << endl;
        CERR << "categoryMaps.size() = " << categoryMaps.size() << endl;
        CERR << "colNames.size() = " << colNames.size() << endl;
        CERR << "imputeOptions.size() = " << imputeOptions.size() << endl;
    }
    
    // convert any kToDefault impute options
    for (size_t col = 0; col < imputeOptions.size(); col++) {
        if (imputeOptions[col] == kToDefault) {
            imputeOptions[col] = getDefaultImputeOption(valueTypes[col]);    
        }
    }
 
#ifdef DUMP_VALUES
    // for debugging - save values and categorymaps to .csv file
    string testFileDir = "/Users/michael/Documents/Projects/RPackages/bin/";
    std::vector< std::vector<std::string> > cells;
    
    valuesToCells(values, valueTypes, categoryMaps, false, "", cells);
    writeCsvPath(testFileDir + "dumpValues.csv", cells, "", colNames);
    
    cells.clear();
    for (index_t col = 0; col < categoryMaps.size(); col++) {
        for (index_t category = categoryMaps.at(col).beginIndex();
             category < categoryMaps.at(col).endIndex();
             category++) {
            
            vector<string> rowCells;
            ostringstream oss;
            oss << colNames[col] << "/" << category;
            rowCells.push_back(oss.str());
            
            rowCells.push_back(categoryMaps.at(col).getCategoryForIndex(category));
            
            cells.push_back(rowCells);
        }
    }
    
    vector<string> ununsedColNames;
    writeCsvPath(testFileDir + "dumpCatMaps.csv", cells, "", ununsedColNames);
#endif
    
    time_t t;
    time(&t);
    if (gVerbose) CERR << localTimeString(t) << " start train()" << endl;
    
    const vector<size_t>& availableColumnIndexes = availableColumns.indexVector();
    const vector<size_t>& selectRowIndexes = selectRows.indexVector();
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
    // make list of candidate columns; skip columns that have no variation
    
    selectColumns.clear(availableColumns.boolVector().size());
    
    for (size_t colIndex = 0; colIndex < availableColumns.countSelected(); colIndex++) {
        size_t col = availableColumnIndexes[colIndex];

        if (gVerbose4) {
            CERR << colNames[col] << endl;
        }
        
        bool variesWithinColumn = false;
        Value firstValue;
        bool first = true;

        for (size_t rowIndex = 0;
             rowIndex < selectRows.countSelected() && !variesWithinColumn;
             rowIndex++) {
            
            size_t row = selectRowIndexes[rowIndex];
            Value nextValue = values[col][row];
            
            if (nextValue.na) {
                SKIP
                
            } else if (first) {
                firstValue = nextValue;
                first = false;
                
            } else {
                switch (valueTypes.at(col)) {
                    case kCategorical:
                        variesWithinColumn = firstValue.number.i != nextValue.number.i;
                        break;
                        
                    case kNumeric:
                        variesWithinColumn = firstValue.number.d != nextValue.number.d;
                        break;
                }
            }
        }
        
        if (variesWithinColumn) {
            selectColumns.select(col);
        }
    }

    size_t numSelectedCols = selectColumns.countSelected();
    
    if (gVerbose4) {
        CERR << numSelectedCols << " Selected:" << endl;
        for (size_t k = 0; k < colNames.size(); k++) {
            if (selectColumns.boolVector().at(k)) {
                CERR << colNames[k] << " ";
            }
        }
        CERR << endl;
    }
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
    // calculate and/or adjust columnsPerTree
    
    if (columnsPerTree <= 0) {
        // calculate default        
        switch (valueTypes.at(targetColumn)) {
            case kCategorical:
                columnsPerTree = (index_t)ceil(sqrt(numSelectedCols));
                break;
                
            case kNumeric:
                columnsPerTree = (index_t)ceil(numSelectedCols / 3.0);
                break;
        }
    }
    
    RUNTIME_ERROR_IF(columnsPerTree < 1, "no useful columns");
    
    // make sure number is reasonable
    if (columnsPerTree > (index_t)numSelectedCols) {
        columnsPerTree = (index_t)numSelectedCols;
    }

    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
    // make sortedIndexes and impute values
    
    vector< vector<size_t> > sortedIndexes;
    makeSortedIndexes(values, valueTypes, selectColumns, sortedIndexes);

    vector<Value> imputedValues;
    
    imputeValues(imputeOptions, valueTypes, values, selectRows, selectColumns, categoryMaps,
                 sortedIndexes, imputedValues);

    if (gVerbose4) {
        for (size_t k = 0; k < categoryMaps.size(); k++) {
            CERR << endl << k << "\t" << colNames[k] << endl;
            categoryMaps.at(k).dump();
        }
    }

    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
    // get column subsets
    
    vector< vector<size_t> > subsets;
    
    makeSelectColSubsets((size_t)numSelectedCols, (size_t)columnsPerTree, maxTrees, subsets);
    
    if (gVerbose) CERR << localTimeString(t) << " done makeSelectColSubsets" << endl;
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
    // make decision tree for each subset

    trees.clear();
    for (size_t subsetIndex = 0; subsetIndex < subsets.size(); subsetIndex++) {
        if (gVerbose3) CERR << "(" << subsetIndex << ") ";

        CompactTree compactTree;
        size_t treeIndex = trees.size();
        trees.push_back(compactTree);
        
        int maxDepthUsed;
        evaluateTree(trees[treeIndex], maxDepth, (int)maxNodes, maxDepthUsed, doPrune,
                     minImprovement, minLeafCount, maxSplitsPerNumericAttribute, values, valueTypes,
                     categoryMaps, subsets[subsetIndex], selectRows, selectColumns, targetColumn,
                     sortedIndexes, colNames, imputedValues);
        
        if (maxDepthUsed < minDepth) {
            trees.resize(trees.size() - 1);
        }

#if RPACKAGE
        R_CheckUserInterrupt();     // Allowing interrupt
#endif
    }

    if (gVerbose) CERR << localTimeString(t) << " done train()" << endl;
    
    if (gVerbose) printCompactTrees(trees, valueTypes, targetColumn, selectColumns, colNames,
                                    categoryMaps);
    
}

// recursively delete all nodes for which specified node is ancestor
void deleteSubtrees(TreeNode *nodeP)
{
    if (nodeP->lessOrEqualNode != NULL) {
        deleteSubtrees(nodeP->lessOrEqualNode);
        delete nodeP->lessOrEqualNode;
        
        nodeP->lessOrEqualNode = NULL;
    }
    
    if (nodeP->greaterOrNotNode != NULL) {
        deleteSubtrees(nodeP->greaterOrNotNode);
        delete nodeP->greaterOrNotNode;
        
        nodeP->greaterOrNotNode = NULL;
    }
}

// for debugging; print list of decision trees
void printCompactTrees(const std::vector<CompactTree>& trees,
                       const std::vector<ValueType>& valueTypes,
                       size_t targetColumn,
                       const SelectIndexes& selectColumns,
                       const std::vector<std::string>& colNames,
                       const std::vector<CategoryMaps>& categoryMaps)
{
    const vector<size_t>& selectColumnIndexes = selectColumns.indexVector();
    
    for (size_t tree = 0; tree < trees.size(); tree++) {
        CERR << "Tree " << tree << endl;
        
        size_t numNodes = trees[tree].splitColIndex.size();
        
        LOGIC_ERROR_IF(numNodes != trees[tree].lessOrEqualIndex.size(), "broken CompactTree");
        LOGIC_ERROR_IF(numNodes != trees[tree].greaterOrNotIndex.size(), "broken CompactTree");
        LOGIC_ERROR_IF(numNodes != trees[tree].toLessOrEqualIfNA.size(), "broken CompactTree");
        LOGIC_ERROR_IF(numNodes != trees[tree].value.size(), "broken CompactTree");
        
        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++) {
            index_t colIndex = trees[tree].splitColIndex[nodeIndex];
            size_t col;
            if (colIndex == NO_INDEX) {
                col = targetColumn;
                
            } else {
                col = selectColumnIndexes.at((size_t)colIndex);    
            }
            
            index_t lessOrEqualNodeIndex = trees[tree].lessOrEqualIndex[nodeIndex];
            index_t greaterOrNotNodeIndex = trees[tree].greaterOrNotIndex[nodeIndex];
            
            string lessOrEqualNodeString(" ");
            if (lessOrEqualNodeIndex >= 0) {
                ostringstream oss;
                oss << lessOrEqualNodeIndex;
                lessOrEqualNodeString = oss.str();
            }
            
            string greaterOrNotNodeString(" ");
            if (greaterOrNotNodeIndex >= 0) {
                ostringstream oss;
                oss << greaterOrNotNodeIndex;
                greaterOrNotNodeString = oss.str();
            }

            switch(valueTypes.at(col)) {
                case kCategorical:
                {
                    index_t index = trees[tree].value[nodeIndex].i;
                    string category = categoryMaps.at(col).getCategoryForIndex(index);
                    
                    CERR << nodeIndex << "]\t" << colNames.at(col) <<
                    "\t" << lessOrEqualNodeString <<
                    "\t" << greaterOrNotNodeString <<
                    "\t" << (trees[tree].toLessOrEqualIfNA[nodeIndex] ?
                             "toLEIfNA = T" : "toLEIfNA = F") <<
                    "\t" << category << endl;
                }
                    break;
                    
                case kNumeric:
                {
                    double value = trees[tree].value[nodeIndex].d;
                    
                    CERR << nodeIndex << "]\t" << colNames.at(col) <<
                    "\t" << lessOrEqualNodeString <<
                    "\t" << greaterOrNotNodeString <<
                    "\t" << (trees[tree].toLessOrEqualIfNA[nodeIndex] ?
                             "toLEIfNA = T" : "toLEIfNA = F") <<
                    "\t" << fixed << setprecision(8) << value << endl;
                }
                    break;
            }
        }
    }
}

// for debugging and testing; return the fraction of matches between selected rows of two vectors of
// categorical Values
double compareMatch(const vector<Value>& values1,
                    const vector<Value>& values2,
                    const SelectIndexes& selectRows)
{
    double result;
    
    if (values1.size() != values2.size()) {
        // indicate size mismatch
        result = -2.0;
        
    } else if (values1.size() == 0) {
        // indicate zero length
        result = -1.0;
        
    } else {
        result = 0.0;
        bool na = false;
        
        const vector<size_t>& rowIndexes = selectRows.indexVector();
        for (size_t rowIndex = 0; rowIndex < rowIndexes.size(); rowIndex++) {
            size_t row = rowIndexes[rowIndex];
            
            if (values1[row].na || values2[row].na) {
                na = true;
            }
            
            if (values1[row].number.i == values2[row].number.i) {
                result++;
            }
        }
        
        if (na) {
            // indicate comparing NA values
            result = -3.0;
            
        } else {
            result /= values1.size();
        }
    }
    
    return result;
}

// for debugging and testing; return the rms difference between selected rows of two vectors of
// numerical Values
double compareRms(const vector<Value>& values1,
                  const vector<Value>& values2,
                  const SelectIndexes& selectRows)
{
    double result;
    
    if (values1.size() != values2.size()) {
        // indicate size mismatch
        result = -2.0;
        
    } else if (values1.size() == 0) {
        // indicate zero length
        result = -1.0;
        
    } else {
        result = 0.0;
        bool na = false;
        
        const vector<size_t>& rowIndexes = selectRows.indexVector();
        for (size_t rowIndex = 0; rowIndex < rowIndexes.size(); rowIndex++) {
            size_t row = rowIndexes[rowIndex];
            
            if (values1[row].na || values2[row].na) {
                na = true;
            }
            
            double delta = values1[row].number.d - values2[row].number.d;
            result += delta * delta;
        }
        
        if (na) {
            // indicate comparing NA values
            result = -3.0;
            
        } else {
            result = sqrt( result / selectRows.countSelected() );
        }
    }
    
    return result;
}

// ========== Local Functions ======================================================================

// recursively count all nodes in the subtree beginning at specified node
// (if leaf node then count = 1)
size_t countNodes(const TreeNode *nodeP)
{
    size_t count = 1;
    
    if (nodeP->lessOrEqualNode != NULL) {
        count += countNodes(nodeP->lessOrEqualNode);
    }
    
    if (nodeP->greaterOrNotNode != NULL) {
        count += countNodes(nodeP->greaterOrNotNode);
    }
    
    return count;
}

// recursively assign a serial number to all nodes in the subtree beginning at specified node 
void indexNodes(TreeNode *nodeP)
{
    nodeP->index = gNextIndex++;;
    
    if (nodeP->lessOrEqualNode != NULL) {
        indexNodes(nodeP->lessOrEqualNode);
    }
    
    if (nodeP->greaterOrNotNode != NULL) {
        indexNodes(nodeP->greaterOrNotNode);
    }
}

// recursively copy the subtree beginning at the specified node to the CompactTree struct
void copyToCompact(CompactTree& compactTree, TreeNode *nodeP)
{
    size_t nodeIndex = nodeP->index;
    LOGIC_ERROR_IF(nodeIndex < 0 || nodeIndex >= compactTree.value.size(), "out of range");
    
    if (nodeP->lessOrEqualNode != NULL) {
        compactTree.splitColIndex[nodeIndex] = nodeP->splitColIndex;
        compactTree.lessOrEqualIndex[nodeIndex] = (index_t)nodeP->lessOrEqualNode->index;
        compactTree.greaterOrNotIndex[nodeIndex] = (index_t)nodeP->greaterOrNotNode->index;
        compactTree.toLessOrEqualIfNA[nodeIndex] = nodeP->toLessOrEqualIfNA;
        compactTree.value[nodeIndex] = nodeP->splitValue.number;
        
        copyToCompact(compactTree, nodeP->lessOrEqualNode);
        copyToCompact(compactTree, nodeP->greaterOrNotNode);
        
    } else {
        compactTree.splitColIndex[nodeIndex] = NO_INDEX;
        compactTree.lessOrEqualIndex[nodeIndex] = NO_INDEX;
        compactTree.greaterOrNotIndex[nodeIndex] = NO_INDEX;
        compactTree.toLessOrEqualIfNA[nodeIndex] = false;
        compactTree.value[nodeIndex] = nodeP->leafValue.number;
    }
}

// create a CompactTree from the decision tree beginning at the specified root node
void makeCompactTree(CompactTree& compactTree, TreeNode& root)
{
    // reindex nodes (may have been pruning)
    gNextIndex = 0;
    indexNodes(&root);
    
    // allocate space
    size_t count = countNodes(&root);
    
    compactTree.splitColIndex.resize(count);
    compactTree.lessOrEqualIndex.resize(count);
    compactTree.greaterOrNotIndex.resize(count);
    compactTree.toLessOrEqualIfNA.resize(count);
    compactTree.value.resize(count);
    
    copyToCompact(compactTree, &root);
}

// recursively improve subtree from specified leaf node (called initially on the root node)
void improveSubtree(TreeNode *nodeP,
                    int depth,
                    int maxDepth,
                    int maxNodes,
                    int& maxDepthUsed,          // updated during recursion
                    const vector<size_t>& subsetIndexes,
                    const vector< vector<Value> >& values,
                    const vector<ValueType>& valueTypes,
                    const vector<CategoryMaps>& categoryMaps,
                    const SelectIndexes& selectColumns,
                    size_t targetColumn,
                    const vector< vector<size_t> >& sortedIndexes,
                    const vector<string>& colNames,
                    double minImprovement,
                    index_t minLeafCount,
                    index_t maxSplitsPerNumericAttribute,
                    index_t& finalLeafCount,    // updated for each leaf found
                    const vector<Value>& imputedValues)
{
    if (depth < maxDepth && (maxNodes <= 0 || gNextIndex < (size_t)maxNodes)) {
        bool improved = improveLeaf(nodeP, subsetIndexes, values, valueTypes, categoryMaps,
                                    selectColumns, targetColumn, sortedIndexes, colNames,
                                    minImprovement, minLeafCount, maxSplitsPerNumericAttribute,
                                    imputedValues);
        
        if (improved) {
            if (maxDepthUsed < depth + 1) {
                maxDepthUsed = depth + 1;   
            }
            
            // recursion happens here
            
            TreeNode *lessOrEqualNode = nodeP->lessOrEqualNode;
            
            improveSubtree(lessOrEqualNode, depth + 1, maxDepth, maxNodes, maxDepthUsed,
                           subsetIndexes, values, valueTypes, categoryMaps, selectColumns,
                           targetColumn, sortedIndexes, colNames, minImprovement, minLeafCount,
                           maxSplitsPerNumericAttribute,finalLeafCount, imputedValues);
            
            TreeNode *greaterOrNotNode = nodeP->greaterOrNotNode;

            improveSubtree(greaterOrNotNode, depth + 1, maxDepth, maxNodes, maxDepthUsed,
                           subsetIndexes, values, valueTypes, categoryMaps, selectColumns,
                           targetColumn, sortedIndexes, colNames, minImprovement, minLeafCount,
                           maxSplitsPerNumericAttribute, finalLeafCount, imputedValues);
        
        } else {
            // cannot improve this leaf; update tally
            finalLeafCount += nodeP->leafLessOrEqualCount + nodeP->leafGreaterOrNotCount;
        }
    }
}

// for debugging; print tree
void printTree(const TreeNode *nodeP,
               const vector< vector<Value> >& values,
               const vector<ValueType>& valueTypes,
               size_t targetColumn,
               const SelectIndexes& selectColumns,
               const vector<CategoryMaps>& categoryMaps,
               const vector<string>& colNames,
               int indent, index_t count)
{
    string indentStr;
    for (int k = 0; k < indent; k++) {
        indentStr += "  ";
    }

    const vector<size_t>& selectColumnIndexes = selectColumns.indexVector();

    string suffix;
    switch (valueTypes.at(targetColumn)) {
        case kCategorical:
        {
            vector<int> targetCategoryCounts;
            double entropy = selectRowsEntropy(nodeP->selectRows, values, targetColumn,
                                               categoryMaps, targetCategoryCounts);
            ostringstream oss;
            for (size_t k = 0; k < targetCategoryCounts.size(); k++) {
                if (k > 0) oss << "/";
                oss << targetCategoryCounts[k];
            }
            
            oss << " [" << fixed << setprecision(8) << entropy << "]" << endl;
            suffix = oss.str();
        }
            break;
            
        case kNumeric:
        {
            double rms = selectRowsSd(nodeP->selectRows, values, targetColumn);  
            ostringstream oss;
            oss << "[" << fixed << setprecision(8) << rms << "]" << endl;
            suffix = oss.str();
        }
            break;
    }
    
    if (nodeP->lessOrEqualNode == NULL) {
        // leaf
        switch (valueTypes.at(targetColumn)) {
            case kCategorical:
                CERR << indentStr << "[" << nodeP->index << "] " << "leaf " <<
                categoryMaps.at(targetColumn).getCategoryForIndex(nodeP->leafValue.number.i) <<
                " (" << count << ") " << suffix;
                break;
                
            case kNumeric:
                CERR << indentStr << "[" << nodeP->index << "] " << "leaf " <<
                fixed << setprecision(8) << nodeP->leafValue.number.d <<
                " (" << count << ") " << suffix;
                break;
        }
        
    } else {
        // node
        index_t splitColIndex = nodeP->splitColIndex;
        LOGIC_ERROR_IF(splitColIndex < 0, "out of range");
        
        size_t col = selectColumnIndexes[(size_t)splitColIndex];
        
        switch (valueTypes.at(col)) {
            case kCategorical:
                CERR << indentStr << "[" << nodeP->index << "] " << "node " <<
                colNames.at(col) << " == " <<
                categoryMaps.at(col).getCategoryForIndex(nodeP->splitValue.number.i) <<
                " (" << count << ") " << suffix;
                break;
                
            case kNumeric:
                CERR << indentStr << "[" << nodeP->index << "] " << "node " <<
                colNames.at(col) << " <= " <<
                fixed << setprecision(8) << nodeP->splitValue.number.d <<
                " (" << count << ") " << suffix;
                break;
        }
        
        TreeNode *lessOrEqualNode = nodeP->lessOrEqualNode;
        TreeNode *greaterOrNotNode = nodeP->greaterOrNotNode;
        
        index_t splitLessOrEqualCount = lessOrEqualNode->leafLessOrEqualCount +
            lessOrEqualNode->leafGreaterOrNotCount;
        
        index_t splitGreaterOrNotCount = greaterOrNotNode->leafLessOrEqualCount +
            greaterOrNotNode->leafGreaterOrNotCount;
        
        printTree(lessOrEqualNode, values, valueTypes, targetColumn, selectColumns, categoryMaps,
                  colNames, indent + 1, splitLessOrEqualCount);
        
        printTree(greaterOrNotNode, values, valueTypes, targetColumn, selectColumns, categoryMaps,
                  colNames, indent + 1, splitGreaterOrNotCount);
    }
}

// calculate entropy for selected set of response values; return vector of category counts
double selectRowsEntropy(const SelectIndexes& selectRows,
                         const vector< vector<Value> >& values,
                         size_t targetColumn,
                         const vector<CategoryMaps>& categoryMaps,
                         vector<int>& targetCategoryCounts)
{
    size_t numTargetCategories = categoryMaps.at(targetColumn).countAllCategories();
    
    index_t beginCategoryIndex = categoryMaps.at(targetColumn).beginIndex();
    
    targetCategoryCounts.assign(numTargetCategories, 0);
    
    const vector<size_t>& rowIndexes = selectRows.indexVector();
    for (size_t rowIndex = 0; rowIndex < rowIndexes.size(); rowIndex++) {
        size_t row = rowIndexes[rowIndex];
        index_t targetCategory = values[targetColumn][row].number.i;
        
        size_t countsIndex = (size_t)(targetCategory - beginCategoryIndex);
        targetCategoryCounts[countsIndex]++;
    }
    
    double entropy = entropyForCounts(targetCategoryCounts);
    
    return entropy;
}

// calculate entropy for set of category counts
double entropyForCounts(const vector<int>& targetCategoryCounts)
{
    double entropy = 0.0;
    
    int total = 0;
    for (size_t k = 0; k < targetCategoryCounts.size(); k++) {
        total += targetCategoryCounts[k];
    }
    
    if (total > 0) {
        for (size_t k = 0; k < targetCategoryCounts.size(); k++) {
            if (targetCategoryCounts[k] > 0) {
                double p = (double)targetCategoryCounts[k] / total;
                entropy += -p * log(p);
            }
        }
    }
    
    return entropy;
}

// calculate entropy for a binary split of counts
double entropyForSplit(const vector<int>& lessThanOrEqualCounts, const vector<int>& totalCounts)
{
    double entropy = 0.0;
    
    int lessThanOrEqualTotal = 0;
    for (size_t k = 0; k < lessThanOrEqualCounts.size(); k++) {
        lessThanOrEqualTotal += lessThanOrEqualCounts[k];
    }
    
    int total = 0;
    for (size_t k = 0; k < totalCounts.size(); k++) {
        total += totalCounts[k];
    }
    
    int greaterThanOrNotEqualTotal = total - lessThanOrEqualTotal;
    
    double lessThanOrEqualEntropy = 0.0;
    double greaterThanOrNotEqualEntropy = 0.0;
    
    if (lessThanOrEqualTotal > 0) {
        for (size_t k = 0; k < lessThanOrEqualCounts.size(); k++) {
            if (lessThanOrEqualCounts[k] > 0) {
                double p = (double)lessThanOrEqualCounts[k] / lessThanOrEqualTotal;
                lessThanOrEqualEntropy += -p * log(p);
            }
        }
    }
    
    if (greaterThanOrNotEqualTotal > 0) {
        for (size_t k = 0; k < lessThanOrEqualCounts.size(); k++) {
            int greaterThanOrNotEqualCount = totalCounts[k] - lessThanOrEqualCounts[k];
            
            if (greaterThanOrNotEqualCount > 0) {
                double p = (double)greaterThanOrNotEqualCount / greaterThanOrNotEqualTotal;
                greaterThanOrNotEqualEntropy += -p * log(p);
            }
        }
    }
    
    if (total > 0) {
        entropy = lessThanOrEqualEntropy * lessThanOrEqualTotal / total +
        greaterThanOrNotEqualEntropy * greaterThanOrNotEqualTotal / total;   
    }
    
    LOGIC_ERROR_IF(isnan(entropy), "entropy = nan");
    
    return entropy;
}

// calculate standard deviation for a selected set of response values
double selectRowsSd(const SelectIndexes& selectRows,
                    const vector< vector<Value> >& values,
                    size_t targetColumn)
{
    double sum = 0.0;
    double sum2 = 0.0;
    int count = 0;
    
    const vector<size_t>& rowIndexes = selectRows.indexVector();
    for (size_t rowIndex = 0; rowIndex < rowIndexes.size(); rowIndex++) {
        size_t row = rowIndexes[rowIndex];
        double value = values[targetColumn][row].number.d;
        sum += value;
        sum2 += value * value;
        count++;
    }
    
    double sd = 0.0;
    
    if (count > 1) {
        double sd2 = (sum2 - sum * sum / count) / (count - 1);
        // protect against negative from loss of significance
        if (sd2 > 0) {
            sd = sqrt(sd2);
        }
    }
    
    return sd;
}

// calculate standard deviation from statistics
double stDev(int count, double sum, double sum2)
{
    double sd = 0.0;
    
    if (count > 1) {
        double sd2 = (sum2 - sum * sum / count) / (count - 1);
        if (sd2 > 0) {
            // protect against negative from loss of significance
            sd = sqrt(sd2);
        }
    }
    
    return sd;
}

double sdForSplit(double lessThanOrEqualSum,
                  double lessThanOrEqualSum2,
                  int lessThanOrEqualCount,
                  double totalSum,
                  double totalSum2,
                  int totalCount)
{
    double sd = 0.0;
    
    int greaterThanOrNotEqualCount = totalCount - lessThanOrEqualCount;
    double greaterThanOrNotEqualSum = totalSum - lessThanOrEqualSum;
    double greaterThanOrNotEqualSum2 = totalSum2 - lessThanOrEqualSum2;
    
    if (lessThanOrEqualCount > 1 && greaterThanOrNotEqualCount > 1) {
        double lessThanOrEqualSd = stDev(lessThanOrEqualCount, lessThanOrEqualSum,
                                         lessThanOrEqualSum2);
        
        double greaterThanOrNotEqualSd = stDev(greaterThanOrNotEqualCount, greaterThanOrNotEqualSum,
                                               greaterThanOrNotEqualSum2);
        
        sd = lessThanOrEqualSd * lessThanOrEqualCount / totalCount +
        greaterThanOrNotEqualSd * greaterThanOrNotEqualCount / totalCount;
        
    } else {
        sd = stDev(totalCount, totalSum, totalSum2);
    }
    
    LOGIC_ERROR_IF(isnan(sd), "sd = nan");
    
    return sd;
}

// get the best split for the specified numeric column
ValueAndMeasure getBestNumericalSplit(size_t col,
                                      size_t targetColumn,
                                      const SelectIndexes& selectRows,
                                      const vector< vector<Value> >& values,
                                      const vector<ValueType>& valueTypes,
                                      const vector<CategoryMaps>& categoryMaps,
                                      const vector< vector<size_t> >& sortedIndexes,
                                      const vector<string>& colNames)
{
    size_t numSortedIndexes = sortedIndexes.at(col).size();
    
    ValueAndMeasure bestSplit;
    bestSplit.value = gNaValue;
    
    switch(valueTypes.at(targetColumn)) {
        case kNumeric:
        {
            // target column is numeric - quality measure will be based on standard deviation
            
            // calculate count, sum, sum-squared of values in selectRows (i.e., for current node)
            double totalSum = 0.0;
            double totalSum2 = 0.0;
            int totalCount = 0;
            
            const vector<size_t>& rowIndexes = selectRows.indexVector();
            for (size_t rowIndex = 0; rowIndex < rowIndexes.size(); rowIndex++) {
                size_t row = rowIndexes[rowIndex];
                double value = values[targetColumn][row].number.d;
                totalSum += value;
                totalSum2 += value * value;
                totalCount++;
            }
            
            if (totalCount >= 2) {
                // start with biggest value in split column (since split is based on less than or
                // equal) then proceed down to find best
                
                bool first = true;
                double previousValue = 0.0;
                
                // initially, all rows are less than or equal to top row
                double lessThanOrEqualSum = totalSum;
                double lessThanOrEqualSum2 = totalSum2;
                int lessThanOrEqualCount = totalCount;
                
                const vector<bool>& rowSelected = selectRows.boolVector();
                
                // iterate over rows in descending order of value in split column
                for (index_t index = (index_t)numSortedIndexes - 1; index >= 0; index--) {
                    size_t row = sortedIndexes[col][(size_t)index];
                    
                    if (rowSelected[row]) {
                        // all values should have been imputed by this point
                        RUNTIME_ERROR_IF(values[col][row].na, "encountered unimputed value");
                        
                        double currentValue = values[col][row].number.d;
                        double currentMeasure = sdForSplit(lessThanOrEqualSum, lessThanOrEqualSum2,
                                                           lessThanOrEqualCount, totalSum,
                                                           totalSum2, totalCount);
                        
                        if (first) {
                            first = false;
                            
                        } else if (currentValue < previousValue) {
                            // don't bother checking unless value has changed from
                            // previously-checked value
                            
                            if (bestSplit.value.na || currentMeasure < bestSplit.measure) {
                                // first candidate for split value, or improvement over previous
                                // best
                                
                                bestSplit.measure = currentMeasure;
                                bestSplit.value.number.d = 0.5 * (currentValue + previousValue);
                                bestSplit.value.na = false;
                            }
                            
                        } else if (currentValue == previousValue) {
                            SKIP
                        }
                        
                        // remove from statistics the value that was just examined 
                        double value = values[targetColumn][row].number.d;
                        lessThanOrEqualSum -= value;
                        lessThanOrEqualSum2 -= value * value;
                        lessThanOrEqualCount--;
                        
                        previousValue = currentValue;
                    }
                }
            }
        }
            break;
            
        case kCategorical:
        {
            // target column is categorical - quality measure will be based on entropy

            // count entries in each target category in selectRows (i.e., for current node)

            size_t numTargetCategories = categoryMaps.at(targetColumn).countAllCategories();
            
            index_t beginCategoryIndex = categoryMaps.at(targetColumn).beginIndex();
            
            vector<int> totalTargetCategoryCounts(numTargetCategories, 0);
            int totalRows = 0;
            
            const vector<size_t>& rowIndexes = selectRows.indexVector();
            for (size_t rowIndex = 0; rowIndex < rowIndexes.size(); rowIndex++) {
                size_t row = rowIndexes[rowIndex];
                index_t targetCategory = values[targetColumn][row].number.i;
                size_t countsIndex = (size_t)(targetCategory - beginCategoryIndex);
                totalTargetCategoryCounts[countsIndex]++;
                totalRows++;
            }
            
            if (totalRows >= 2) {
                // start with biggest value (since split is based on less than or equal)
                // then proceed down to find best
                
                bool first = true;
                double previousValue = 0.0;

                // initially, all rows are less than or equal to top row
                vector<int> currentTargetCategoryCounts(totalTargetCategoryCounts);
                
                const vector<bool>& rowSelected = selectRows.boolVector();
                
                for (index_t index = (index_t)numSortedIndexes - 1; index >= 0; index--) {
                    size_t row = sortedIndexes[col][(size_t)index];
                    
                    if (rowSelected[row]) {
                        // all values should have been imputed by this point
                        RUNTIME_ERROR_IF(values[col][row].na, "encountered unimputed value");
                        
                        double currentValue = values[col][row].number.d;

                        if (first) {
                            first = false;
                            
                        } else if (currentValue < previousValue) {
                            // don't bother checking unless value has changed from
                            // previously-checked value

                            double currentMeasure = entropyForSplit(currentTargetCategoryCounts,
                                                                    totalTargetCategoryCounts);
                            
                            if (bestSplit.value.na || currentMeasure < bestSplit.measure) {
                                // first candidate for split value, or improvement over previous
                                // best
                                
                                bestSplit.measure = currentMeasure;
                                bestSplit.value.number.d = 0.5 * (currentValue + previousValue);
                                bestSplit.value.na = false;
                            }
                            
                        } else if (currentValue == previousValue) {
                            SKIP
                        }

                        // remove from currentTargetCategoryCounts the row that was just examined
                        index_t targetCategory = values[targetColumn][row].number.i;
                        size_t countsIndex = (size_t)(targetCategory - beginCategoryIndex);
                        currentTargetCategoryCounts[countsIndex]--;
                        
                        previousValue = currentValue;
                    }
                }
            }
        }
            break;
    }
    
    return bestSplit;
}

// get the best split for the specified categorical column
ValueAndMeasure getBestCategoricalSplit(size_t col,
                                        size_t targetColumn,
                                        const SelectIndexes& selectRows,
                                        const vector< vector<Value> >& values,
                                        const vector<ValueType>& valueTypes,
                                        const vector<CategoryMaps>& categoryMaps,
                                        const vector< vector<size_t> >& sortedIndexes,
                                        const vector<string>& colNames)
{
    ValueAndMeasure bestSplit;
    bestSplit.value = gNaValue;
    string bestSplitName = "";
    
    switch(valueTypes.at(targetColumn)) {
        case kNumeric:
        {
            // target column is numeric - quality measure will be based on standard deviation
            
            size_t numCurrentCategories = categoryMaps.at(col).countAllCategories();
            index_t beginCategoryIndex = categoryMaps.at(col).beginIndex();
            index_t endCategoryIndex = categoryMaps.at(col).endIndex();
            
            if (numCurrentCategories > 1) {
                // calculate count, sum, sum-squared of all values in selectRows (i.e., for current
                // node) and for rows belonging to each category in split column
                
                double totalSum = 0.0;
                double totalSum2 = 0.0;
                int totalCount = 0;
                
                vector<double> categorySum(numCurrentCategories, 0.0);
                vector<double> categorySum2(numCurrentCategories, 0.0);
                vector<int> categoryCount(numCurrentCategories, 0);
                
                const vector<size_t>& rowIndexes = selectRows.indexVector();
                for (size_t rowIndex = 0; rowIndex < rowIndexes.size(); rowIndex++) {
                    size_t row = rowIndexes[rowIndex];
                    double value = values[targetColumn][row].number.d;
                    
                    totalSum += value;
                    totalSum2 += value * value;
                    totalCount++;
                    
                    // all values should have been imputed by this point
                    RUNTIME_ERROR_IF(values[col][row].na, "encountered unimputed value");
                    
                    index_t category = values[col][row].number.i;
                    
                    size_t countsIndex = (size_t)(category - beginCategoryIndex);
                    
                    categorySum[countsIndex] += value;
                    categorySum2[countsIndex] += value * value;
                    categoryCount[countsIndex]++;
                }
                
                if (totalCount >= 2) {
                    bool first = true;
                    
                    // try each category in split column as candidate for split
                    for (index_t categoryIndex = beginCategoryIndex;
                         categoryIndex < endCategoryIndex;
                         categoryIndex++) {
                        
                        size_t countsIndex = (size_t)(categoryIndex - beginCategoryIndex);
                        
                        if (categoryCount.at(countsIndex) >= 1) {
                            double categoryMeasure =
                            sdForSplit(categorySum[countsIndex], categorySum2[countsIndex],
                                       categoryCount[countsIndex],
                                       totalSum, totalSum2, totalCount);
                            
                            if (first || categoryMeasure < bestSplit.measure) {
                                // first candidate, or better than previous categories
                                bestSplit.value.number.i = categoryIndex;
                                bestSplit.value.na = false;
                                bestSplit.measure = categoryMeasure;
                                bestSplitName =
                                    categoryMaps.at(col).getCategoryForIndex(categoryIndex);
                                
                                first = false;
                                
                            } else if (categoryMeasure == bestSplit.measure) {
                                // use name as tiebreaker, to eliminate dependence on order
                                // of categories
                                
                                string nextName =
                                    categoryMaps.at(col).getCategoryForIndex(categoryIndex);
                                
                                if (nextName < bestSplitName) {
                                    bestSplit.value.number.i = categoryIndex;
                                    bestSplit.value.na = false;
                                    bestSplit.measure = categoryMeasure;
                                    bestSplitName = nextName;
                                }
                            }
                        }
                    }
                }
            }
        }
            break;
            
        case kCategorical:
        {
            // target column is categorical - quality measure will be based on entropy
            
            size_t numTargetCategories = categoryMaps.at(targetColumn).countAllCategories();
            index_t beginCategoryIndex = categoryMaps.at(targetColumn).beginIndex();

            vector<int> totalTargetCategoryCounts(numTargetCategories, 0);
            int totalRows = 0;
            
            const vector<size_t>& rowIndexes = selectRows.indexVector();
            for (size_t rowIndex = 0; rowIndex < rowIndexes.size(); rowIndex++) {
                size_t row = rowIndexes[rowIndex];
                index_t targetCategoryIndex = values[targetColumn][row].number.i;
                
                size_t countsIndex = (size_t)(targetCategoryIndex - beginCategoryIndex);
                
                totalTargetCategoryCounts[countsIndex]++;
                totalRows++;
            }
            
            if (totalRows > 0) {
                bool first = true;
                index_t previousCategory = 0;
                index_t currentCategory = 0;
                int categoryCount = 0;
                vector<int> currentTargetCategoryCounts(numTargetCategories, 0);
                
                const vector<bool>& rowSelected = selectRows.boolVector();
                
                size_t numSortedIndexes = sortedIndexes.at(col).size();

                // rows are sorted by split-column categories, so by iterating, each category will
                // handled in turn; the signal to handle the last category will be when index is
                // past the end of sortedIndexes
                // 
                for (size_t index = 0; index <= numSortedIndexes; index++) {
                    bool evaluateCategory = false;
                    double currentMeasure = 0.0;
                    
                    if (index == numSortedIndexes) {
                        // just finished last finished category; evaluate it
                        currentMeasure = entropyForSplit(currentTargetCategoryCounts,
                                                         totalTargetCategoryCounts);
                        
                        evaluateCategory = categoryCount > 0;
                        
                    } else {
                        size_t row = sortedIndexes[col][index];
                        
                        if (rowSelected[row]) {
                            // handle next row
                            
                            // all values should have been imputed by this point
                            RUNTIME_ERROR_IF(values[col][row].na, "encountered unimputed value");
                            
                            currentCategory = values[col][row].number.i;
                            
                            if (first) {
                                // beginning first split category
                                first = false;
                                
                            } else if (currentCategory != previousCategory) {
                                // category has changed, so evaluate accumulated statistics for
                                // previous split category
                                
                                currentMeasure = entropyForSplit(currentTargetCategoryCounts,
                                                                 totalTargetCategoryCounts);
                                
                                evaluateCategory = true;
                                
                                // after calculating measure, reset statistics for new split
                                // category
                                categoryCount = 0;
                                currentTargetCategoryCounts.assign(numTargetCategories, 0);
                            }
                            
                            // accumulate statistics for split category
                            index_t targetCategory = values[targetColumn][row].number.i;
                            size_t countsIndex = (size_t)(targetCategory - beginCategoryIndex);
                            currentTargetCategoryCounts[countsIndex]++;
                            categoryCount++;
                        }
                    }
                    
                    if (evaluateCategory) {
                        // after accumulating statistics for category, see if it is best so far
                        
                        if (bestSplit.value.na || currentMeasure < bestSplit.measure) {
                            // first candidate for split value, or improvement over previous
                            // best

                            bestSplit.measure = currentMeasure;
                            bestSplit.value.number.i = previousCategory;
                            bestSplit.value.na = false;
                            bestSplitName =
                                categoryMaps.at(col).getCategoryForIndex(previousCategory);
                            
                        } else if (currentMeasure == bestSplit.measure) {
                            // use name as tiebreaker, to eliminate dependence on order
                            // of categories
                            
                            string nextName =
                                categoryMaps.at(col).getCategoryForIndex(previousCategory);
                            
                            if (nextName < bestSplitName) {
                                bestSplit.measure = currentMeasure;
                                bestSplit.value.number.i = previousCategory;
                                bestSplit.value.na = false;
                                bestSplitName = nextName;
                            }
                        }
                    }
                    
                    previousCategory = currentCategory;
                }
            }
        }
            break;
    }
    
    return bestSplit;
}

// try to improve leaf that has potential for improvement (i.e., not already perfect)
bool improveImperfectLeaf(TreeNode *nodeP,
                          const vector<size_t>& subsetIndexes,
                          const vector< vector<Value> >& values,
                          const vector<ValueType>& valueTypes,
                          const vector<CategoryMaps>& categoryMaps,
                          const SelectIndexes& selectColumns,
                          size_t targetColumn,
                          const vector< vector<size_t> >& sortedIndexes,
                          const vector<string>& colNames,
                          double minImprovement,
                          index_t minLeafCount,
                          index_t maxSplitsPerNumericAttribute,
                          const vector<Value>& imputedValues)
{
    if (gVerbose) CERR << "improveImperfectLeaf" << endl;
    
    ValueType targetValueType = valueTypes.at(targetColumn);
    
    size_t numRows = values[0].size();
    SelectIndexes& selectRows = nodeP->selectRows;
    
    const vector<size_t>& selectColumnIndexes = selectColumns.indexVector();
    
    size_t numSubsetCols = subsetIndexes.size();
    
    vector<Value> splitValues(numSubsetCols);
    vector<Value> lessOrEqualValues(numSubsetCols);
    vector<Value> greaterOrNotValues(numSubsetCols);
    
    vector<double> colMeasures(numSubsetCols, 0.0);
    
    // find best column to use for split
    for (size_t siIndex = 0; siIndex < numSubsetCols; siIndex++) {
        size_t columnIndex = subsetIndexes[siIndex];
        size_t col = selectColumnIndexes[columnIndex];
        
        if (gVerbose) CERR << endl << colNames.at(col) << endl;
        
        ValueAndMeasure bestSplit =  { { { 0.0 }, false }, 0.0 };
        bestSplit.value = gNaValue;
        
        switch(valueTypes.at(col)) {
            case kCategorical:
            {
                // next trial column is categorical
                bestSplit = getBestCategoricalSplit(col, targetColumn, selectRows, values,
                                                    valueTypes, categoryMaps, sortedIndexes,
                                                    colNames);
                
                if (bestSplit.value.na) {
                    // no split found
                    splitValues[siIndex] = gNaValue;
                    
                    lessOrEqualValues[siIndex] = gNaValue;
                    greaterOrNotValues[siIndex] = gNaValue;
                    
                    if (gVerbose) {
                        CERR << "    no split" << endl;
                    }
                    
                } else {
                    // found best split for this column
                    splitValues[siIndex] = bestSplit.value;
                    colMeasures[siIndex] = bestSplit.measure;
                    
                    // determine rows that go to each side of split
                    SelectIndexes selectLessOrEqualTo(numRows, false);
                    SelectIndexes selectGreaterThan(numRows, false);
                    
                    const vector<size_t>& rowIndexes = selectRows.indexVector();
                    for (size_t rowIndex = 0; rowIndex < rowIndexes.size(); rowIndex++) {
                        size_t row = rowIndexes[rowIndex];
                        if (!values[col][row].na) {
                            if (values[col][row].number.i == bestSplit.value.number.i) {
                                selectLessOrEqualTo.select(row);
                                
                            } else {
                                selectGreaterThan.select(row);
                            }
                            
                        } else {
                            // TODO handle imputation and incorporate NA values here 
                        }
                    }
                    
                    // calculate leaf value for each side of split
                    switch (targetValueType) {
                        case kCategorical:
                            lessOrEqualValues[siIndex] = modeValue(values[targetColumn],
                                                                   selectLessOrEqualTo,
                                                                   categoryMaps.at(targetColumn));
                            
                            greaterOrNotValues[siIndex] = modeValue(values[targetColumn],
                                                                    selectGreaterThan,
                                                                    categoryMaps.at(targetColumn));
                            break;
                            
                        case kNumeric:
                            lessOrEqualValues[siIndex] = meanValue(values[targetColumn],
                                                                   selectLessOrEqualTo);
                            
                            greaterOrNotValues[siIndex] = meanValue(values[targetColumn],
                                                                    selectGreaterThan);
                            break;
                    }
                    
                    if (gVerbose) {
                        string category =
                            categoryMaps.at(col).getCategoryForIndex(bestSplit.value.number.i);
                        
                        CERR << "    best split " << category <<
                        " measure " << bestSplit.measure << endl;
                    }
                    
                }
            }
                break;
                
            case kNumeric:
            {
                // next trial column is numeric
                index_t usageCountForThisAttribute = NO_INDEX;
                
                if (maxSplitsPerNumericAttribute != NO_INDEX) {
                    // need to count
                    usageCountForThisAttribute = 0;
                    
                    TreeNode *nextNodeP = nodeP;
                    while (nextNodeP->parentNode != NULL) {
                        nextNodeP = nextNodeP->parentNode;
                        
                        size_t nextCol = selectColumnIndexes[(size_t)nextNodeP->splitColIndex];
                        if (col == nextCol) {
                            usageCountForThisAttribute++;    
                        }
                    }
                }
                
                if (usageCountForThisAttribute == NO_INDEX ||
                    usageCountForThisAttribute < maxSplitsPerNumericAttribute) {
                    
                    bestSplit = getBestNumericalSplit(col, targetColumn, selectRows, values,
                                                      valueTypes, categoryMaps, sortedIndexes,
                                                      colNames);
                }
                
                if (bestSplit.value.na) {
                    // no split found
                    splitValues[siIndex] = gNaValue;
                    
                    lessOrEqualValues[siIndex] = gNaValue;
                    greaterOrNotValues[siIndex] = gNaValue;
                    
                    colMeasures[siIndex] = -1;
                    
                    if (gVerbose) {
                        CERR << "    no split" << endl;
                    }
                    
                } else {
                    // found best split for this column
                    splitValues[siIndex] = bestSplit.value;
                    colMeasures[siIndex] = bestSplit.measure;
                    
                    // determine rows that go to each side of split
                    SelectIndexes selectLessOrEqualTo(numRows, false);
                    SelectIndexes selectGreaterThan(numRows, false);
                    
                    const vector<size_t>& rowIndexes = selectRows.indexVector();
                    for (size_t rowIndex = 0; rowIndex < rowIndexes.size(); rowIndex++) {
                        size_t row = rowIndexes[rowIndex];
                        if (!values[col][row].na) {
                            if (values[col][row].number.d <= bestSplit.value.number.d) {
                                selectLessOrEqualTo.select(row);
                                
                            } else {
                                selectGreaterThan.select(row);
                            }
                            
                        } else {
                            // TODO handle imputation and incorporate NA values here 
                        }
                    }
                    
                    // calculate leaf value for each side of split
                    switch (targetValueType) {
                        case kCategorical:
                            lessOrEqualValues[siIndex] = modeValue(values[targetColumn],
                                                                   selectLessOrEqualTo,
                                                                   categoryMaps.at(targetColumn));
                            
                            greaterOrNotValues[siIndex] = modeValue(values[targetColumn],
                                                                    selectGreaterThan,
                                                                    categoryMaps.at(targetColumn));
                            break;
                            
                        case kNumeric:
                            lessOrEqualValues[siIndex] = meanValue(values[targetColumn],
                                                                   selectLessOrEqualTo);
                            
                            greaterOrNotValues[siIndex] = meanValue(values[targetColumn],
                                                                    selectGreaterThan);
                            break;
                    }
                    
                    if (gVerbose) {
                        CERR << "    best split " << fixed << setprecision(8) <<
                        bestSplit.value.number.d << " measure " << bestSplit.measure << endl;
                    }
                }
            }
                break;
        }
    }
    
    // look at columns with split values, pick best one
    size_t bestSiIndex = 0;
    double bestColMeasure = colMeasures[bestSiIndex];
    bool foundBestCol = !splitValues[bestSiIndex].na;
    
    for (size_t siIndex = 0; siIndex < numSubsetCols; siIndex++) {
        if (!splitValues[siIndex].na && colMeasures[siIndex] < bestColMeasure) {
            bestSiIndex = siIndex;
            bestColMeasure = colMeasures[bestSiIndex];
            foundBestCol = true;
        }
    }
    
    if (gVerbose) {
        if (foundBestCol) {
            CERR << endl << "best column " <<
            colNames[selectColumnIndexes[subsetIndexes[bestSiIndex]]] << " measure " <<
            fixed << setprecision(8) << bestColMeasure << endl;
            
        } else {
            CERR << endl << "no best column " << endl;
        }
    }

    bool improved = false;

    if (foundBestCol) {
        // test for improvement of split over existing leaf
        switch (targetValueType) {
            case kCategorical:
            {
                // calculate leaf measure
                size_t numTargetCategories = categoryMaps.at(targetColumn).countAllCategories();
                index_t beginCategoryIndex = categoryMaps.at(targetColumn).beginIndex();
                
                int leafTotal = 0;
                vector<int> leafCounts(numTargetCategories, 0);
                
                const vector<size_t>& rowIndexes = selectRows.indexVector();
                for (size_t rowIndex = 0; rowIndex < rowIndexes.size(); rowIndex++) {
                    size_t row = rowIndexes[rowIndex];
                    leafTotal++;
                    
                    index_t targetCategoryIndex = values[targetColumn][row].number.i;
                    size_t countsIndex = (size_t)(targetCategoryIndex - beginCategoryIndex);
                    
                    leafCounts[countsIndex]++;
                }

                double leafMeasure = entropyForCounts(leafCounts);

                improved = bestColMeasure < leafMeasure;

                if (gVerbose) {
                    CERR << "improved = " << fixed << setprecision(8) <<
                    bestColMeasure << " < " << leafMeasure << (improved ? " (T)" : " (F)") <<
                    endl;
                }
                
                
            }
                break;
                
            case kNumeric:
            {
                // calculate leaf measure
                int leafTotal = 0;
                double leafSum = 0.0;
                double leafSum2 = 0.0;
                
                const vector<size_t>& rowIndexes = selectRows.indexVector();
                for (size_t rowIndex = 0; rowIndex < rowIndexes.size(); rowIndex++) {
                    size_t row = rowIndexes[rowIndex];
                    double value = values[targetColumn][row].number.d;
                    leafSum += value;
                    leafSum2 += value * value;
                    leafTotal++;
                }
                
                if (leafTotal > 0) {
                    double leafMeasure = stDev(leafTotal, leafSum, leafSum2);
                    double delta = leafMeasure - bestColMeasure;
                    
                    // correction factor for number of categories
                    size_t factor = 1;
                    size_t columnIndex = subsetIndexes[bestSiIndex];
                    size_t col = selectColumnIndexes[columnIndex];
                    if (valueTypes.at(col) == kCategorical) {
                        factor = categoryMaps.at(col).countAllCategories();   
                    }
                    
                    delta *= factor;
                    
                    // check for minImprovement ratio
                    improved = delta >= minImprovement * leafMeasure;
                    
                    if (false && gVerbose) {
                        CERR << "improved = " << fixed << setprecision(8) << bestColMeasure
                        << " < " << leafMeasure << (improved ? " (T)" : " (F)") << endl;
                    }
                }
            }
                break;
        }
    }
    
    if (improved) {
        // we have improvement - create split in tree
        
        Value splitValue = splitValues[bestSiIndex];
        
        // new TreeNode
        TreeNode *lessOrEqualNode = new TreeNode;
        lessOrEqualNode->leafValue = lessOrEqualValues[bestSiIndex];
        lessOrEqualNode->splitValue = gNaValue;
        lessOrEqualNode->parentNode = nodeP;
        lessOrEqualNode->lessOrEqualNode = NULL;
        lessOrEqualNode->greaterOrNotNode = NULL;
        lessOrEqualNode->toLessOrEqualIfNA = false;
        lessOrEqualNode->splitColIndex = NO_INDEX;          
        lessOrEqualNode->leafLessOrEqualCount = 0;          
        lessOrEqualNode->leafGreaterOrNotCount = 0;
        lessOrEqualNode->selectRows.clear(numRows);
        lessOrEqualNode->index = gNextIndex++;
        
        // new TreeNode
        TreeNode *greaterOrNotNode = new TreeNode;
        greaterOrNotNode->leafValue = greaterOrNotValues[bestSiIndex];
        greaterOrNotNode->splitValue = gNaValue;
        greaterOrNotNode->parentNode = nodeP;
        greaterOrNotNode->lessOrEqualNode = NULL;
        greaterOrNotNode->greaterOrNotNode = NULL;
        greaterOrNotNode->toLessOrEqualIfNA = false;
        greaterOrNotNode->splitColIndex = NO_INDEX;          
        greaterOrNotNode->leafLessOrEqualCount = 0;          
        greaterOrNotNode->leafGreaterOrNotCount = 0;
        greaterOrNotNode->selectRows.clear(numRows);
        greaterOrNotNode->index = gNextIndex++;
        
        size_t splitColIndex = subsetIndexes[bestSiIndex];
        size_t col = selectColumnIndexes[splitColIndex];
        
        // calculate for new nodes: selectRows, leafLessOrEqualCount, leafGreaterOrNotCount,
        // branchCorrectCount or branchSum2

        switch (targetValueType) {
            case kCategorical:
                lessOrEqualNode->branchCorrectCount = 0;
                greaterOrNotNode->branchCorrectCount = 0;
                break;
                
            case kNumeric:
                lessOrEqualNode->branchSum2 = 0.0;
                greaterOrNotNode->branchSum2 = 0.0;
                break;
        }

        const vector<size_t>& rowIndexes = selectRows.indexVector();
        for (size_t rowIndex = 0; rowIndex < rowIndexes.size(); rowIndex++) {
            size_t row = rowIndexes[rowIndex];
            
            bool isLessOrEqual = false;
            
            switch(valueTypes.at(col)) {
                case kCategorical:
                    isLessOrEqual = values.at(col)[row].number.i == splitValue.number.i;
                    break;
                    
                case kNumeric:
                    isLessOrEqual = values.at(col)[row].number.d <= splitValue.number.d;
                    break;
            }
            
            if (isLessOrEqual) {
                lessOrEqualNode->selectRows.select(row);

            } else {
                greaterOrNotNode->selectRows.select(row);
            }
            
            switch (targetValueType) {
                case kCategorical:
                {
                    index_t targetRowValue = values[targetColumn][row].number.i;
                    
                    if (isLessOrEqual) {
                        if (targetRowValue == lessOrEqualNode->leafValue.number.i) {
                            lessOrEqualNode->branchCorrectCount++;
                            lessOrEqualNode->leafLessOrEqualCount++;
                            
                        } else {
                            lessOrEqualNode->leafGreaterOrNotCount++;
                        }
                        
                    } else {
                        if (targetRowValue == greaterOrNotNode->leafValue.number.i) {
                            greaterOrNotNode->branchCorrectCount++;
                            greaterOrNotNode->leafLessOrEqualCount++;
                            
                        } else {
                            greaterOrNotNode->leafGreaterOrNotCount++;
                        }
                    }
                    
                }
                    break;
                    
                case kNumeric:
                {
                    double targetRowValue = values[targetColumn][row].number.d;
                    
                    if (isLessOrEqual) {
                        double delta = targetRowValue - lessOrEqualNode->leafValue.number.d;
                        lessOrEqualNode->branchSum2 += delta * delta;
                        
                        if (targetRowValue <= lessOrEqualNode->leafValue.number.d) {
                            lessOrEqualNode->leafLessOrEqualCount++;
                            
                        } else {
                            lessOrEqualNode->leafGreaterOrNotCount++;
                        }

                    } else {
                        double delta = targetRowValue - greaterOrNotNode->leafValue.number.d;
                        greaterOrNotNode->branchSum2 += delta * delta;
                        
                        if (targetRowValue <= greaterOrNotNode->leafValue.number.d) {
                            greaterOrNotNode->leafLessOrEqualCount++;
                            
                        } else {
                            greaterOrNotNode->leafGreaterOrNotCount++;
                        }
                    }
                }
                    break;
            }
        }

        index_t splitLessOrEqualCount = lessOrEqualNode->leafLessOrEqualCount +
            lessOrEqualNode->leafGreaterOrNotCount;
        
        index_t splitGreaterOrNotCount = greaterOrNotNode->leafLessOrEqualCount +
            greaterOrNotNode->leafGreaterOrNotCount;
        
        // make sure counts in both branches are at least minLeafCount;
        // set toLessOrEqualIfNA according to impute value
        
        if (splitLessOrEqualCount >= minLeafCount && splitGreaterOrNotCount >= minLeafCount) {
            // modify TreeNode for split
            nodeP->splitValue = splitValue;
            nodeP->splitColIndex = (index_t)splitColIndex;
            nodeP->lessOrEqualNode = lessOrEqualNode;
            nodeP->greaterOrNotNode = greaterOrNotNode;
            
            bool toLessOrEqualIfNA = false;
            Value imputed = imputedValues[col];
            
            switch(valueTypes.at(col)) {
                case kNumeric:
                    toLessOrEqualIfNA = !imputed.na && imputed.number.d <= splitValue.number.d;
                    break;
                    
                case kCategorical:
                    toLessOrEqualIfNA = !imputed.na && imputed.number.i == splitValue.number.i;
                    break;
            }
            
            nodeP->toLessOrEqualIfNA = toLessOrEqualIfNA;            
            
        } else {
            // back out - not improved
            delete lessOrEqualNode;
            delete greaterOrNotNode;
            improved = false;
        }
    }
    
    return improved;
}

// try improving the decision tree by splitting the specified leaf node; if sucess, return true;
// if not, return false and leave leaf unsplit
bool improveLeaf(TreeNode *nodeP,
                 const vector<size_t>& subsetIndexes,
                 const vector< vector<Value> >& values,
                 const vector<ValueType>& valueTypes,
                 const vector<CategoryMaps>& categoryMaps,
                 const SelectIndexes& selectColumns,
                 size_t targetColumn,
                 const vector< vector<size_t> >& sortedIndexes,
                 const vector<string>& colNames,
                 double minImprovement,
                 index_t minLeafCount,
                 index_t maxSplitsPerNumericAttribute,
                 const vector<Value>& imputedValues)
{
    bool improved = false;
    
    switch(valueTypes.at(targetColumn)) {
        case kCategorical:
        {
            index_t leafCount = nodeP->leafLessOrEqualCount + nodeP->leafGreaterOrNotCount;
            
            bool perfect = nodeP->branchCorrectCount == leafCount;
            
            if (perfect) {
                SKIP
                
            } else {
                improved = improveImperfectLeaf(nodeP, subsetIndexes, values, valueTypes,
                                                categoryMaps, selectColumns, targetColumn,
                                                sortedIndexes, colNames, minImprovement,
                                                minLeafCount, maxSplitsPerNumericAttribute,
                                                imputedValues);
            }
        }
            break;
            
        case kNumeric:
        {
            bool perfect = nodeP->branchSum2 == 0.0;
            
            if (perfect) {
                SKIP
                
            } else {
                improved = improveImperfectLeaf(nodeP, subsetIndexes, values, valueTypes,
                                                categoryMaps, selectColumns, targetColumn,
                                                sortedIndexes, colNames, minImprovement,
                                                minLeafCount, maxSplitsPerNumericAttribute,
                                                imputedValues);
            }
        }
            break;
    }
    
    return improved;
}

// create a decision tree using the specified subset of columns of the Values array
void evaluateTree(CompactTree& compactTree,
                  int maxDepth,
                  int maxNodes,
                  int& maxDepthUsed,
                  bool doPrune,
                  double minImprovement,
                  index_t minLeafCount,
                  index_t maxSplitsPerNumericAttribute,
                  const vector< vector<Value> >& values,
                  const vector<ValueType>& valueTypes, 
                  const vector<CategoryMaps>& categoryMaps,
                  const vector<size_t>& subsetIndexes,
                  const SelectIndexes& selectRows,
                  const SelectIndexes& selectColumns,
                  size_t targetColumn,
                  const vector< vector<size_t> >& sortedIndexes,
                  const vector<string>& colNames,
                  const vector<Value>& imputedValues)
{
    time_t startTime;
    time(&startTime);

    const vector<size_t>& selectColumnIndexes = selectColumns.indexVector();

    if (gVerbose3) {
        CERR << endl <<
        "~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~" << endl;
        CERR << "evaluateTree(";
        for (size_t k = 0; k < subsetIndexes.size(); k++) {
            if (k > 0) CERR << ", ";
            CERR << colNames[selectColumnIndexes[subsetIndexes[k]]];
        }
        CERR << ")" << endl;
    }
    
    // get default value for root (leaf) node
    Value defaultValue;
    switch(valueTypes.at(targetColumn)) {
        case kNumeric:
            defaultValue = meanValue(values[targetColumn], selectRows);
            break;
            
        case kCategorical:
            defaultValue = modeValue(values[targetColumn], selectRows,
                                     categoryMaps.at(targetColumn));
            break;
    }
    
    // begin with root node
    TreeNode root;
    root.parentNode = NULL;
    root.splitColIndex = NO_INDEX;
    root.leafValue = defaultValue;
    root.splitValue = gNaValue;
    root.lessOrEqualNode = NULL;
    root.greaterOrNotNode = NULL;
    root.leafLessOrEqualCount = 0;
    root.leafGreaterOrNotCount = 0;
    root.selectRows = selectRows;
    root.index = gNextIndex++;
    
    index_t numSelectedRows = 0;
    
    switch(valueTypes.at(targetColumn)) {
        case kNumeric:
        {
            root.branchSum2 = 0.0;
            
            const vector<size_t>& rowIndexes = root.selectRows.indexVector();
            for (size_t rowIndex = 0; rowIndex < rowIndexes.size(); rowIndex++) {
                size_t row = rowIndexes[rowIndex];
                numSelectedRows++;
                
                double delta = values[targetColumn][row].number.d - root.leafValue.number.d;
                root.branchSum2 += delta * delta;
                
                if (values[targetColumn][row].number.d <= root.leafValue.number.d) {
                    // less or equal
                    root.leafLessOrEqualCount++;
                    
                } else {
                    // greater or not
                    root.leafGreaterOrNotCount++;
                }
            }
        }
            break;
            
        case kCategorical:
        {
            root.branchCorrectCount = 0;
            
            const vector<size_t>& rowIndexes = root.selectRows.indexVector();
            for (size_t rowIndex = 0; rowIndex < rowIndexes.size(); rowIndex++) {
                size_t row = rowIndexes[rowIndex];
                numSelectedRows++;
                
                if (values[targetColumn][row].number.i == root.leafValue.number.i) {
                    // less or equal
                    root.leafLessOrEqualCount++;
                    root.branchCorrectCount++;
                    
                } else {
                    // greater or not
                    root.leafGreaterOrNotCount++;
                }
            }
        }
            break;
    }

    gNextIndex = 0; // start indexes from zero

    // initialize values updated in improveSubtree()
    maxDepthUsed = 1;
    index_t finalLeafCount = 0;
    
    // recursively improve tree beginning from root node
    improveSubtree(&root, 1, maxDepth, maxNodes, maxDepthUsed, subsetIndexes, values, valueTypes,
                   categoryMaps, selectColumns, targetColumn, sortedIndexes, colNames,
                   minImprovement, minLeafCount, maxSplitsPerNumericAttribute,finalLeafCount,
                   imputedValues);
    
    if (gVerbose2) {
        CERR << endl << "Before pruning:" << endl;
        printTree(&root, values, valueTypes, targetColumn, selectColumns, categoryMaps, colNames, 0,
                  numSelectedRows);
    }
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
    
    if (doPrune) {
        pruneTree(root, values, valueTypes, targetColumn, categoryMaps, sortedIndexes, colNames);
        
        if (gVerbose2) {
            CERR << endl << "After pruning:" << endl;
            printTree(&root, values, valueTypes, targetColumn, selectColumns, categoryMaps,
                      colNames, 0, numSelectedRows);
        }
    }
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
    
    makeCompactTree(compactTree, root);
    
    if (gVerbose2) {
        CERR << endl << "After compacting:" << endl;
        printTree(&root, values, valueTypes, targetColumn, selectColumns, categoryMaps, colNames, 0,
                  numSelectedRows);
    }

    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
    
    deleteSubtrees(&root);
    
}

// ========== Tests ================================================================================

// component tests
void ctest_train(int& totalPassed, int& totalFailed, bool verbose)
{
    int passed = 0;
    int failed = 0;
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // train 

    // ~~~~~~~~~~~~~~~~~~~~~~
    // deleteSubtrees
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // printCompactTrees

    // ~~~~~~~~~~~~~~~~~~~~~~
    // compareMatch
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // compareRms
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // evaluateTree

    // ~~~~~~~~~~~~~~~~~~~~~~
    // improveLeaf

    // ~~~~~~~~~~~~~~~~~~~~~~
    // improveSubtree

    // ~~~~~~~~~~~~~~~~~~~~~~
    // improveImperfectLeaf

    // ~~~~~~~~~~~~~~~~~~~~~~
    // countNodes

    // ~~~~~~~~~~~~~~~~~~~~~~
    // indexNodes

    // ~~~~~~~~~~~~~~~~~~~~~~
    // makeCompactTree

    // ~~~~~~~~~~~~~~~~~~~~~~
    // copyToCompact

    // ~~~~~~~~~~~~~~~~~~~~~~
    // printTree

    // ~~~~~~~~~~~~~~~~~~~~~~
    // selectRowsEntropy

    // ~~~~~~~~~~~~~~~~~~~~~~
    // entropyForCounts

    // ~~~~~~~~~~~~~~~~~~~~~~
    // entropyForSplit

    // ~~~~~~~~~~~~~~~~~~~~~~
    // selectRowsSd

    // ~~~~~~~~~~~~~~~~~~~~~~
    // stDev

    // ~~~~~~~~~~~~~~~~~~~~~~
    // sdForSplit

    // ~~~~~~~~~~~~~~~~~~~~~~
    // getBestNumericalSplit

    // ~~~~~~~~~~~~~~~~~~~~~~
    // getBestCategoricalSplit

    // ~~~~~~~~~~~~~~~~~~~~~~
    
    if (verbose) {
        CERR << "train.cpp" << "\t" << passed << " passed, " << failed << " failed" << endl;
    }
    
    totalPassed += passed;
    totalFailed += failed;
}

// code coverage
void cover_train(bool verbose)
{
    // ~~~~~~~~~~~~~~~~~~~~~~
    // train 
    // printCompactTrees
    // deleteSubtrees
    // evaluateTree
    // improveLeaf
    // improveSubtree
    // improveImperfectLeaf
    // countNodes
    // indexNodes
    // makeCompactTree
    // copyToCompact
    // printTree
    // selectRowsEntropy
    // entropyForCounts
    // entropyForSplit
    // selectRowsSd
    // stDev
    // sdForSplit
    // getBestNumericalSplit
    // getBestCategoricalSplit

    int maxDepth = 100;
    double minImprovement = 0.0;
    index_t minLeafCount = 1;
    index_t maxSplitsPerNumericAttribute = -1;
    index_t maxNodes = 100;
    
    string data =
    "       C0,     C1,     C2,     C3,     C4,     C5\n"
    "       1,      A,      0.5,    100,    NA,     42\n"
    "       1,      A,      0.4,    101,    \"Q\",  32\n"
    "       2,      B,      0.3,    XYZ,    \"P\",  NA\n"
    "       3,      B,      0.2,    XYZ,    \"P\",  22\n"
    "       5,      B,      0.1,    101,    \"R\",  NA\n";
    
    vector< vector<Value> > values;
    vector<ValueType> valueTypes;
    vector<CategoryMaps> categoryMaps;
    vector<string> colNames;
    
    vector< vector<string> > cells;
    vector< vector<bool> > quoted;
    
    readCsvString(data, cells, quoted, colNames);
    getDefaultValueTypes(cells, quoted, true, "NA", valueTypes);
    cellsToValues(cells, quoted, valueTypes, true, "NA", values, false, categoryMaps);

    size_t numCols = values.size();
    size_t numRows = values[0].size();
    
    SelectIndexes selectRows(numRows, true);
    vector<ImputeOption> imputeOptions(numCols, kToDefault);
    
    if (verbose) {
        printValues(values, valueTypes, categoryMaps, colNames);
    }
    
    {
        vector<CompactTree> trees;
        SelectIndexes selectColumns;
        
        index_t maxTrees = 100;
        index_t columnsPerTree = 2;
        int minDepth = 0;
        bool doPrune = true;
        size_t targetColumn = 0;
        
        SelectIndexes availableColumns(numCols, true);
        availableColumns.unselect(targetColumn);
        
        vector< vector<Value> > trainValues = values;
        
        train(trees, columnsPerTree, maxDepth, minDepth, doPrune, minImprovement, minLeafCount,
              maxSplitsPerNumericAttribute,maxTrees, maxNodes, selectRows, availableColumns,
              selectColumns, trainValues, valueTypes, categoryMaps, targetColumn, colNames,
              imputeOptions);
    }
    
    {
        vector<CompactTree> trees;
        SelectIndexes selectColumns;
        
        index_t maxTrees = 5;
        index_t columnsPerTree = -1;
        int minDepth = 2;
        bool doPrune = false;
        size_t targetColumn = 1;
        
        SelectIndexes availableColumns(numCols, true);
        availableColumns.unselect(targetColumn);
        
        vector< vector<Value> > trainValues = values;
        
        maxSplitsPerNumericAttribute = 1;
        
        train(trees, columnsPerTree, maxDepth, minDepth, doPrune, minImprovement, minLeafCount,
              maxSplitsPerNumericAttribute,maxTrees, maxNodes, selectRows, availableColumns,
              selectColumns, trainValues, valueTypes, categoryMaps, targetColumn, colNames,
              imputeOptions);
        
        if (verbose) {
            printCompactTrees(trees, valueTypes, targetColumn, selectColumns, colNames,
                              categoryMaps);
        }
    }
    
    {
        vector<CompactTree> trees;
        SelectIndexes selectColumns;
        
        index_t maxTrees = 100;
        index_t columnsPerTree = -1;
        int minDepth = 0;
        bool doPrune = true;
        size_t targetColumn = 0;
        
        SelectIndexes availableColumns(numCols, true);
        availableColumns.unselect(targetColumn);
        
        vector< vector<Value> > trainValues = values;
        
        train(trees, columnsPerTree, maxDepth, minDepth, doPrune, minImprovement, minLeafCount,
              maxSplitsPerNumericAttribute,maxTrees, maxNodes, selectRows, availableColumns,
              selectColumns, trainValues, valueTypes, categoryMaps, targetColumn, colNames,
              imputeOptions);
    }
    
    values.push_back(values[4]);
    valueTypes.push_back(valueTypes[4]);
    categoryMaps.push_back(categoryMaps[4]);
    imputeOptions.push_back(imputeOptions[4]);
    colNames.push_back("C6");
    numCols++;
    values[6][0].na = false;
    values[6][0].number.i = 0;
    values[6][3].na = true;
    
    for (size_t col = 0; col < numCols; col++) {
        for (size_t row = 0; row < numRows; row++) {
            values[col].push_back(values[col][row]);
        }
    }
    
    numRows *= 2;
    selectRows.selectAll(numRows);
    
    if (verbose) {
        printValues(values, valueTypes, categoryMaps, colNames);
    }
    
    {
        vector<CompactTree> trees;
        SelectIndexes selectColumns;
        
        index_t maxTrees = 100;
        index_t columnsPerTree = 20;
        int minDepth = 0;
        bool doPrune = true;
        size_t targetColumn = 1;
        
        SelectIndexes availableColumns(numCols, true);
        availableColumns.unselect(targetColumn);
        
        vector< vector<Value> > trainValues = values;
        
        train(trees, columnsPerTree, maxDepth, minDepth, doPrune, minImprovement, minLeafCount,
              maxSplitsPerNumericAttribute,maxTrees, maxNodes, selectRows, availableColumns,
              selectColumns, trainValues, valueTypes, categoryMaps, targetColumn, colNames,
              imputeOptions);
    }
    
    {
        vector<CompactTree> trees;
        SelectIndexes selectColumns;
        
        index_t maxTrees = 100;
        index_t columnsPerTree = 2;
        int minDepth = 0;
        bool doPrune = true;
        size_t targetColumn = 1;
        
        SelectIndexes availableColumns(numCols, true);
        availableColumns.unselect(targetColumn);
        
        vector< vector<Value> > trainValues = values;
        
        train(trees, columnsPerTree, maxDepth, minDepth, doPrune, minImprovement, minLeafCount,
              maxSplitsPerNumericAttribute,maxTrees, maxNodes, selectRows, availableColumns,
              selectColumns, trainValues, valueTypes, categoryMaps, targetColumn, colNames,
              imputeOptions);
    }
    
    imputeOptions[4] = kToMode;
    
    {
        vector<CompactTree> trees;
        SelectIndexes selectColumns;
        
        index_t maxTrees = 100;
        index_t columnsPerTree = 2;
        int minDepth = 0;
        bool doPrune = true;
        size_t targetColumn = 1;
        
        SelectIndexes availableColumns(numCols, true);
        availableColumns.unselect(targetColumn);
        
        vector< vector<Value> > trainValues = values;
        
        train(trees, columnsPerTree, maxDepth, minDepth, doPrune, minImprovement, minLeafCount,
              maxSplitsPerNumericAttribute,maxTrees, maxNodes, selectRows, availableColumns,
              selectColumns, trainValues, valueTypes, categoryMaps, targetColumn, colNames,
              imputeOptions);
    }
    
    values[1][2].number.i = categoryMaps[1].findOrInsertCategory("C");
    
    {
        vector<CompactTree> trees;
        SelectIndexes selectColumns;
        
        index_t maxTrees = 100;
        index_t columnsPerTree = 2;
        int minDepth = 0;
        bool doPrune = true;
        size_t targetColumn = 1;
        
        SelectIndexes availableColumns(numCols, true);
        availableColumns.unselect(targetColumn);
        
        vector< vector<Value> > trainValues = values;
        
        train(trees, columnsPerTree, maxDepth, minDepth, doPrune, minImprovement, minLeafCount,
              maxSplitsPerNumericAttribute,maxTrees, maxNodes, selectRows, availableColumns,
              selectColumns, trainValues, valueTypes, categoryMaps, targetColumn, colNames,
              imputeOptions);
    }
        
    // ~~~~~~~~~~~~~~~~~~~~~~
    // compareMatch
    
    Value v1;   v1.na = false;  v1.number.i = 1;
    Value v2;   v2.na = false;  v2.number.i = 2;
    
    vector<Value> values1;
    vector<Value> values2;

    compareMatch(values1, values2, SelectIndexes(0, true));

    values1.push_back(v1);
    values1.push_back(v2);
    
    values2.push_back(v1);

    compareMatch(values1, values2, SelectIndexes(2, true));

    values2.push_back(v2);
    
    compareMatch(values1, values2, SelectIndexes(2, true));

    values1.push_back(gNaValue);
    values2.push_back(gNaValue);
    
    compareMatch(values1, values2, SelectIndexes(3, true));
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // compareRms

    Value v3;   v3.na = false;  v3.number.d = 0.3;
    Value v4;   v4.na = false;  v4.number.d = 0.4;
    
    values1.clear();
    values2.clear();

    compareRms(values1, values2, SelectIndexes(0, true));
    
    values1.push_back(v3);
    values1.push_back(v4);
    
    values2.push_back(v3);
    
    compareRms(values1, values2, SelectIndexes(2, true));
    
    values2.push_back(v4);
    
    compareRms(values1, values2, SelectIndexes(2, true));
    
    values1.push_back(gNaValue);
    values2.push_back(gNaValue);
    
    compareRms(values1, values2, SelectIndexes(3, true));
}



