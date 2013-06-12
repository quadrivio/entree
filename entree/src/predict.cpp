//
//  predict.cpp
//  entree
//
//  Created by MPB on 5/7/13.
//  Copyright (c) 2013 Quadrivio Corporation. All rights reserved.
//  License http://opensource.org/licenses/BSD-2-Clause
//          <YEAR> = 2013
//          <OWNER> = Quadrivio Corporation
//

//
//  Predict response from attributes and ensemble of decision trees
//

#include "predict.h"

#include "train.h"

#include <iomanip>
#include <iostream>

using namespace std;

// ========== Local Headers ========================================================================

// predict response from one decision tree
void predictOne(const vector< vector<Value> >& values,
                const std::vector<ValueType>& valueTypes,
                const SelectIndexes& selectRows,
                size_t targetColumn,
                const std::vector<CategoryMaps>& categoryMaps,
                const SelectIndexes& selectColumns,
                const CompactTree& tree,
                vector<Value>& predictVector,
                const std::vector<std::string>& colNames);

// ========== Functions ============================================================================

// predict response from ensemble of decision trees and array of Values
void predict(std::vector< std::vector<Value> >& values,
             const std::vector<ValueType>& valueTypes,
             const std::vector<CategoryMaps>& categoryMaps,
             size_t targetColumn,
             const SelectIndexes& selectRows,
             const SelectIndexes& selectColumns,
             const std::vector<CompactTree>& trees,
             const std::vector<std::string>& colNames)
{
    size_t numRows = values[targetColumn].size();
    size_t numTrees = trees.size();
    
    vector<Value>& predictVector = values.at(targetColumn);

    LOGIC_ERROR_IF(values.size() != valueTypes.size(), "values vs. valueTypes size mismatch");
    LOGIC_ERROR_IF(values.size() != selectColumns.boolVector().size(),
                   "values vs. selectColumns size mismatch");
    
    vector< vector<size_t> > sortedIndexes;
    makeSortedIndexes(values, valueTypes, selectColumns, sortedIndexes);
    
    const vector<size_t>& selectRowIndexes = selectRows.indexVector();

    switch (valueTypes.at(targetColumn)) {
        case kCategorical:
        {
            predictVector.assign(numRows, gNaValue);
            
            size_t numTargetCategories = categoryMaps.at(targetColumn).countAllCategories();
            
            index_t beginCategoryIndex = categoryMaps.at(targetColumn).beginIndex();
            index_t endCategoryIndex = categoryMaps.at(targetColumn).endIndex();
            
            // count number of times each category is predicted for each row
            
            vector< vector<index_t> > counts(numRows);
            for (size_t k = 0; k < numRows; k++) {
                counts[k].assign(numTargetCategories, 0);
            }
            
            for (size_t k = 0; k < numTrees; k++) {
                vector<Value> onePredictVector;
                
                predictOne(values, valueTypes, selectRows, targetColumn, categoryMaps,
                           selectColumns, trees[k], onePredictVector, colNames);
                
                for (size_t rowIndex = 0; rowIndex < selectRowIndexes.size(); rowIndex++) {
                    size_t row = selectRowIndexes[rowIndex];
                    index_t categoryIndex = onePredictVector[row].number.i;
                    size_t countsIndex = (size_t)(categoryIndex - beginCategoryIndex);
                    counts[row][countsIndex]++;
                }
            }
            
            // find most frequently predicted category for each row over trees
            
            for (size_t rowIndex = 0; rowIndex < selectRowIndexes.size(); rowIndex++) {
                size_t row = selectRowIndexes[rowIndex];
                
                index_t maxCount = 0;
                for (index_t categoryIndex = beginCategoryIndex;
                     categoryIndex < endCategoryIndex;
                     categoryIndex++) {
                    
                    size_t countsIndex = (size_t)(categoryIndex - beginCategoryIndex);
                    index_t nextCount = counts[row][countsIndex];
 
                    if (maxCount < nextCount) {
                        // most frequent so far
                        predictVector[row].number.i = categoryIndex;
                        predictVector[row].na = false;
                        maxCount = nextCount;
                        
                    } else if (nextCount > 0 && maxCount == nextCount) {
                        // same counts; use name as tie breaker, to enforce deterministic result w/o
                        // regard to order of categories
                        
                        index_t currentIndex = predictVector[row].number.i;
                        if (categoryMaps[targetColumn].getCategoryForIndex(categoryIndex) <
                            categoryMaps[targetColumn].getCategoryForIndex(currentIndex)) {
                            
                            predictVector[row].number.i = categoryIndex;
                            predictVector[row].na = false;
                            maxCount = nextCount;
                        }
                        
                    }
                }
            }
        }
            break;
            
        case kNumeric:
        {
            Value zeroValue;
            zeroValue.na = false;
            zeroValue.number.d = 0.0;
            
            // calculate average result over trees
            
            for (size_t rowIndex = 0; rowIndex < selectRowIndexes.size(); rowIndex++) {
                size_t row = selectRowIndexes[rowIndex];
                
                predictVector[row] = zeroValue;
            }   
            
            for (size_t treeIndex = 0; treeIndex < numTrees; treeIndex++) {
                vector<Value> onePredictVector;
                
                predictOne(values, valueTypes, selectRows, targetColumn, categoryMaps,
                           selectColumns, trees[treeIndex], onePredictVector, colNames);
                
                for (size_t rowIndex = 0; rowIndex < selectRowIndexes.size(); rowIndex++) {
                    size_t row = selectRowIndexes[rowIndex];
                    
                    predictVector[row].number.d += onePredictVector[row].number.d;
                }
            }
            
            for (size_t rowIndex = 0; rowIndex < selectRowIndexes.size(); rowIndex++) {
                size_t row = selectRowIndexes[rowIndex];
                
                predictVector[row].number.d /= numTrees;
            }
        }
            break;
    }
    
}

// ========== Local Functions ======================================================================

// predict response from one decision tree
void predictOne(const vector< vector<Value> >& values,
                const std::vector<ValueType>& valueTypes,
                const SelectIndexes& selectRows,
                size_t targetColumn,
                const std::vector<CategoryMaps>& categoryMaps,
                const SelectIndexes& selectColumns,
                const CompactTree& tree,
                vector<Value>& predictVector,
                const std::vector<std::string>& colNames)
{
    bool trace = false; // for debugging
    
    size_t numCols = values.size();
    size_t numRows = values[0].size();
    
    predictVector.resize(numRows, gNaValue);

    const vector<size_t>& selectColumnIndexes = selectColumns.indexVector();

    const vector<size_t>& rowIndexes = selectRows.indexVector();
    for (size_t rowIndex = 0; rowIndex < rowIndexes.size(); rowIndex++) {
        size_t row = rowIndexes[rowIndex];
        LOGIC_ERROR_IF(row >= numRows, "out of range");

        size_t nodeIndex = 0;
        
        if (trace) {
            CERR << "~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~" << endl;
            CERR << "trace " << rowIndex << endl;
        }
        
        while (tree.lessOrEqualIndex[nodeIndex] != NO_INDEX) {
            // keep looping until reach leaf
            
            index_t splitColIndex = tree.splitColIndex[nodeIndex];
            LOGIC_ERROR_IF(splitColIndex < 0, "out of range");
            
            size_t col = selectColumnIndexes[(size_t)splitColIndex];
            LOGIC_ERROR_IF(col >= numCols, "out of range");
            
            Value compareValue = values[col][row];

            if (trace) {
                index_t colIndex = tree.splitColIndex[nodeIndex];
                size_t col;
                if (colIndex == NO_INDEX) {
                    col = targetColumn;
                    
                } else {
                    col = selectColumnIndexes.at((size_t)colIndex);    
                }
                
                switch(valueTypes.at(col)) {
                    case kCategorical:
                    {
                        index_t index = tree.value[nodeIndex].i;
                        string category = categoryMaps.at(col).getCategoryForIndex(index);

                        if (compareValue.na) {
                            CERR << nodeIndex << "]\t" << colNames.at(col) <<
                            "\t" << tree.lessOrEqualIndex[nodeIndex] <<
                            "\t" << tree.greaterOrNotIndex[nodeIndex] <<
                            "\t" << category << "\t(value = NA)" << endl;
                            
                        } else {
                            string valueCategory =
                                categoryMaps.at(col).getCategoryForIndex(compareValue.number.i);
                            
                            CERR << nodeIndex << "]\t" << colNames.at(col) <<
                            "\t" << tree.lessOrEqualIndex[nodeIndex] <<
                            "\t" << tree.greaterOrNotIndex[nodeIndex] <<
                            "\t" << category << "\t(value = '" << valueCategory << "')" << endl;
                        }
                    }
                        break;
                        
                    case kNumeric:
                        CERR << nodeIndex << "]\t" << colNames.at(col) <<
                        "\t" << tree.lessOrEqualIndex[nodeIndex] <<
                        "\t" << tree.greaterOrNotIndex[nodeIndex] <<
                        "\t" << fixed << setprecision(8) << tree.value[nodeIndex].d <<
                        "\t(value = " << compareValue.number.d << ")" << endl;
                        break;
                }
            }
            
            bool useLessOrEqual;

            if (compareValue.na) {
                useLessOrEqual = tree.toLessOrEqualIfNA[nodeIndex];
                
            } else {
                switch (valueTypes[col]) {
                    case kCategorical:
                        useLessOrEqual = compareValue.number.i == tree.value[nodeIndex].i;
                        break;
                        
                    case kNumeric:
                        useLessOrEqual = compareValue.number.d <= tree.value[nodeIndex].d;
                        break;
                }
            }
            
            nodeIndex = useLessOrEqual ?
                (size_t)tree.lessOrEqualIndex[nodeIndex] :
                (size_t)tree.greaterOrNotIndex[nodeIndex];
        }

        if (trace) {
            index_t colIndex = tree.splitColIndex[nodeIndex];
            size_t col;
            if (colIndex == NO_INDEX) {
                col = targetColumn;
                
            } else {
                col = selectColumnIndexes.at((size_t)colIndex);    
            }
            
            switch(valueTypes.at(col)) {
                case kCategorical:
                {
                    index_t index = tree.value[nodeIndex].i;
                    string category = categoryMaps.at(col).getCategoryForIndex(index);
                    CERR << nodeIndex << "]\t" << colNames.at(col) <<
                    "\t" << tree.lessOrEqualIndex[nodeIndex] <<
                    "\t" << tree.greaterOrNotIndex[nodeIndex] <<
                    "\t" << category << endl;
                }
                    break;
                    
                case kNumeric:
                    CERR << nodeIndex << "]\t" << colNames.at(col) <<
                    "\t" << tree.lessOrEqualIndex[nodeIndex] <<
                    "\t" << tree.greaterOrNotIndex[nodeIndex] <<
                    "\t" << fixed << setprecision(8) << tree.value[nodeIndex].d << endl;
                    break;
            }
            
            CERR << "~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~" << endl;
        }

        predictVector[row].number = tree.value[nodeIndex];
        predictVector[row].na = false;
    }
}

// ========== Tests ================================================================================

#include "csv.h"
#include "train.h"

// component tests
void ctest_predict(int& totalPassed, int& totalFailed, bool verbose)
{
    int passed = 0;
    int failed = 0;
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // predict
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // predictOne
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    
    if (verbose) {
        CERR << "utils.cpp" << "\t" << passed << " passed, " << failed << " failed" << endl;
    }
    
    totalPassed += passed;
    totalFailed += failed;
}

// code coverage
void cover_predict(bool verbose)
{
    // ~~~~~~~~~~~~~~~~~~~~~~
    // predict
    // predictOne
    
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
              maxSplitsPerNumericAttribute, maxTrees, maxNodes, selectRows, availableColumns,
              selectColumns, trainValues, valueTypes, categoryMaps, targetColumn, colNames,
              imputeOptions);
        
        vector< vector<Value> > predictValues = values;
        
        predict(predictValues, valueTypes, categoryMaps, targetColumn, selectRows, selectColumns,
                trees, colNames);
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
              maxSplitsPerNumericAttribute, maxTrees, maxNodes, selectRows, availableColumns,
              selectColumns, trainValues, valueTypes, categoryMaps, targetColumn, colNames,
              imputeOptions);
        
        vector< vector<Value> > predictValues = values;
        
        predict(predictValues, valueTypes, categoryMaps, targetColumn, selectRows, selectColumns,
                trees, colNames);
    }
    
    {
        // to get to tie breaker code in predict()
        
        vector<CompactTree> trees(2);
        
        size_t targetColumn = 2;
        
        vector<CategoryMaps> categoryMaps2(numCols);
        
        categoryMaps2[0].insertCategory("A");
        categoryMaps2[0].insertCategory("B");
        categoryMaps2[1].insertCategory("D");
        categoryMaps2[1].insertCategory("C");
        categoryMaps2[2].insertCategory("F");
        categoryMaps2[2].insertCategory("E");
        
        size_t numCols = 3;
        size_t numRows = 2;
        
        SelectIndexes selectColumns(numCols, true);
        selectColumns.unselect(targetColumn);
        
        Value v0;   v0.na = false;  v0.number.i = 0;
        Value v1;   v1.na = false;  v1.number.i = 1;
        
        vector< vector<Value> > predictValues(numCols, vector<Value>(numRows, v0));
        predictValues[0][1] = v1;
        predictValues[1][1] = v1;
        predictValues[2][1] = v1;
        
        vector<string>colNames2;
        colNames.push_back("C1");
        colNames.push_back("C2");
        colNames.push_back("Y");
        
        vector<ValueType> valueTypes2(3, kCategorical);
        
        SelectIndexes selectRows2(numRows, true);

        std::vector<index_t> splitColIndex;          // NO_INDEX if leaf
        std::vector<index_t> lessOrEqualIndex;       // NO_INDEX if leaf
        std::vector<index_t> greaterOrNotIndex;      // NO_INDEX if leaf
        std::vector<bool> toLessOrEqualIfNA;
        std::vector<Number> value;

        trees[0].splitColIndex.assign(1, NO_INDEX);
        trees[0].lessOrEqualIndex.assign(1, NO_INDEX);
        trees[0].greaterOrNotIndex.assign(1, NO_INDEX);
        trees[0].toLessOrEqualIfNA.assign(1, true);
        trees[0].value.assign(1, v0.number);
        
        trees[1].splitColIndex.assign(1, NO_INDEX);
        trees[1].lessOrEqualIndex.assign(1, NO_INDEX);
        trees[1].greaterOrNotIndex.assign(1, NO_INDEX);
        trees[1].toLessOrEqualIfNA.assign(1, true);
        trees[1].value.assign(1, v1.number);
        
        predict(predictValues, valueTypes2, categoryMaps2, targetColumn, selectRows2, selectColumns,
                trees, colNames2);
    }
}

