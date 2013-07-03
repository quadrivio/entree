//
//  prune.cpp
//  entree
//
//  Created by MPB on 5/7/13.
//  Copyright (c) 2013 Quadrivio Corporation. All rights reserved.
//  License http://opensource.org/licenses/BSD-2-Clause
//          <YEAR> = 2013
//          <OWNER> = Quadrivio Corporation
//

//
// Prune size of decision tree using algorithms in Witten & Frank. Data Mining. 2000 (1st ed.).
//

#include "prune.h"

#include "format.h"

#include <cmath>
#include <iostream>
#include <set>

using namespace std;

// ========== Local Headers ========================================================================

void updateBranchCategorical(TreeNode *nodeP);

void updateBranchNumeric(TreeNode *nodeP);

// recursively find all branch nodes and their respective depths
void findBranchNodes(vector< pair<int, TreeNode *> >& nodes, int depth, TreeNode *nodeP);

// compare two <depth, TreeNode> pairs, return true if the first has greater depth than the second
bool compareDepth(const pair<int, TreeNode *>& i, const pair<int, TreeNode *>& j);

double pessimisticErrorEstimate(index_t categoryCount, index_t totalCount);

// for debugging; print set of leaf nodes
void printLeaves(const set<TreeNode *> leafSet);

// for debugging; print list of depth/node pairs
void printNodes(const vector< pair<int, TreeNode *> >& nodes);

// return true if subtree should be replaced with a leaf node; for case when split parameter at top
// of subtree is categorical type
bool testReplaceSubtreeCategorical(const TreeNode *nodeP);

// return true if subtree should be replaced with a leaf node; for case when split parameter at top
// of subtree is numeric type
bool testReplaceSubtreeNumeric(const TreeNode *nodeP);

// if all the terminal nodes of specified subtree point to the same category, return index of
// category, else return NO_INDEX
index_t sameCategoryForAllLeaves(const TreeNode *nodeP);

// ========== Globals ========================================================================

namespace ns_train {
    // for debugging
    extern bool gVerbose;    
    extern bool gVerbose1;    
    extern bool gVerbose2;    
    extern bool gVerbose3; 
    extern bool gVerbose4; 
};

using namespace ns_train;

// ========== Functions ============================================================================

// try reducing size of tree
void pruneTree(TreeNode& root,
               const vector< vector<Value> >& values,
               const vector<ValueType>& valueTypes,
               size_t targetColumn,
               const vector<CategoryMaps>& categoryMaps,
               const vector< vector<size_t> >& sortedIndexes,
               const vector<string>& colNames)
{
    // recursively update branchCorrectCount or branchSum2 for each node in tree
    switch (valueTypes.at(targetColumn)) {
        case kCategorical:  updateBranchCategorical(&root);   break;
        case kNumeric:      updateBranchNumeric(&root);       break;
    }
    
    // get branch nodes sorted by ascending depth (deepest first)
    vector< pair<int, TreeNode *> > nodes;
    findBranchNodes(nodes, 0, &root);
    sort(nodes.begin(), nodes.end(), compareDepth);
    
    if (gVerbose2) {
        CERR << "before: ";
        printNodes(nodes);
    }
    
    // from bottom up, see if should remove subtrees
    switch (valueTypes.at(targetColumn)) {
        case kCategorical:
        {
            for (size_t k = 0; k < nodes.size(); k++) {
                TreeNode *nextNode = nodes[k].second;
                
                if (testReplaceSubtreeCategorical(nextNode)) {
                    // modify TreeNode
                    deleteSubtrees(nextNode);
                    nextNode->splitColIndex = NO_INDEX;
                }
            }
        }
            break;
            
        case kNumeric:
        {
            for (size_t k = 0; k < nodes.size(); k++) {
                TreeNode *nextNode = nodes[k].second;
                
                if (testReplaceSubtreeNumeric(nextNode)) {
                    // modify TreeNode
                    deleteSubtrees(nextNode);
                    nextNode->splitColIndex = NO_INDEX;
                }
            }
        }
            break;
    }
    
    if (gVerbose2) {
        nodes.clear();
        findBranchNodes(nodes, 0, &root);
        sort(nodes.begin(), nodes.end(), compareDepth);
        
        CERR << "after:  ";
        printNodes(nodes);
    }
}

// ========== Local Functions ======================================================================

void printLeaves(const set<TreeNode *> leafSet)
{
    for (set<TreeNode *>::const_iterator iter = leafSet.begin(); iter != leafSet.end(); iter++) {
        if (iter != leafSet.begin()) {
            CERR << ", ";
        }
        
        CERR << (*iter)->index;
    }
    
    CERR << endl;
}

void printNodes(const vector< pair<int, TreeNode *> >& nodes)
{
    for (size_t k = 0; k < nodes.size(); k++) {
        if (k > 0) {
            CERR << ", ";
        }
        
        CERR << nodes[k].second->index;
    }
    
    CERR << endl;
}

void updateBranchCategorical(TreeNode *nodeP)
{
    TreeNode *lessOrEqualNode = nodeP->lessOrEqualNode;
    
    if (lessOrEqualNode != NULL) {
        TreeNode *greaterOrNotNode = nodeP->greaterOrNotNode;
        
        updateBranchCategorical(lessOrEqualNode);
        updateBranchCategorical(greaterOrNotNode);
        
        nodeP->branchCorrectCount = lessOrEqualNode->branchCorrectCount +
            greaterOrNotNode->branchCorrectCount;
    }
}

void updateBranchNumeric(TreeNode *nodeP)
{
    TreeNode *lessOrEqualNode = nodeP->lessOrEqualNode;
    
    if (lessOrEqualNode != NULL) {
        TreeNode *greaterOrNotNode = nodeP->greaterOrNotNode;
        
        updateBranchNumeric(lessOrEqualNode);
        updateBranchNumeric(greaterOrNotNode);
        
        nodeP->branchSum2 = lessOrEqualNode->branchSum2 +
            greaterOrNotNode->branchSum2;
    }
}

void findBranchNodes(vector< pair<int, TreeNode *> >& nodes, int depth, TreeNode *nodeP)
{
    if (nodeP->lessOrEqualNode != NULL) {
        nodes.push_back(make_pair(depth, nodeP));
        
        findBranchNodes(nodes, depth + 1, nodeP->lessOrEqualNode);
        findBranchNodes(nodes, depth + 1, nodeP->greaterOrNotNode);
    }
}

// compare two <depth, TreeNode> pairs, return true if the first has greater depth than the second
bool compareDepth(const pair<int, TreeNode *>& i, const pair<int, TreeNode *>& j)
{
    return i.first > j.first;    
}

double pessimisticErrorEstimate(index_t categoryCount, index_t totalCount)
{
    // see Witten & Frank. Data Mining. 2000 (1st ed.). p. 165.
    
    // pessimistic estimate (c = 25%)
    const double z = 0.69;
    double n = totalCount;
    double f = (n - categoryCount) / n;
    
    double numerator = f + z * z / (2 * n) + z * sqrt( f / n - f * f / n + z * z / (4 * n * n) );
    double denominator = 1 + z * z / n;
    
    double e = numerator / denominator;
    
    LOGIC_ERROR_IF(isnan(e), "e = nan");
    
    return e;
}

// return true if subtree should be replaced with a leaf node; for case when target column is
// categorical type
bool testReplaceSubtreeCategorical(const TreeNode *nodeP)
{
    // compare error estimate if leaf node vs. estimate if subtree is retained;
    // or, in case where all leaves of subtree end up with same classification, return true
    
    index_t nodeCorrect = nodeP->leafLessOrEqualCount;
    index_t nodeCount = nodeP->leafLessOrEqualCount + nodeP->leafGreaterOrNotCount;
    double nodeEstimate = pessimisticErrorEstimate(nodeCorrect, nodeCount);
    
    TreeNode * leftNode = nodeP->lessOrEqualNode;
    index_t leftChildCorrect = leftNode->branchCorrectCount;
    index_t leftChildCount = leftNode->leafLessOrEqualCount + leftNode->leafGreaterOrNotCount;
    double leftChildEstimate = leftChildCount == 0 ? 0.0 : pessimisticErrorEstimate(leftChildCorrect, leftChildCount);
    
    TreeNode * rightNode = nodeP->lessOrEqualNode;
    index_t rightChildCorrect = rightNode->branchCorrectCount;
    index_t rightChildCount = rightNode->leafLessOrEqualCount + rightNode->leafGreaterOrNotCount;
    double rightChildEstimate = rightChildCount == 0 ? 0.0 : pessimisticErrorEstimate(rightChildCorrect, rightChildCount);
    
    double weightedChildEstimate = leftChildEstimate * leftChildCount / nodeCount +
    rightChildEstimate * rightChildCount / nodeCount;
    
    bool result = nodeEstimate < weightedChildEstimate || sameCategoryForAllLeaves(nodeP) != NO_INDEX;
    
//    CERR << "cPrune(" << nodeP->index << ") = " << (result ? "true" : "false") << endl;
    
    return result;
}

// return true if subtree should be replaced with a leaf node; for case when target column is
// numeric type
bool testReplaceSubtreeNumeric(const TreeNode *nodeP)
{
    // compare error estimate if leaf node vs. estimate if subtree is retained
    
    // uses rms error, with compensation factor from Witten & Frank. Data Mining. 2000 (1st ed.).
    // p. 203.

    index_t nodeCount = nodeP->leafLessOrEqualCount + nodeP->leafGreaterOrNotCount;
    double nodeRms = sqrt(nodeP->branchSum2 / nodeCount);
    double nodeFactor = (nodeCount + 1.0) / (nodeCount - 1.0);
    double nodeEstimate = nodeFactor * nodeRms;
    
    TreeNode * leftNode = nodeP->lessOrEqualNode;
    index_t leftCount = leftNode->leafLessOrEqualCount + leftNode->leafGreaterOrNotCount;
    double leftRms = sqrt(leftNode->branchSum2 / leftCount);
    double leftFactor = (leftCount + 1.0) / (leftCount - 1.0);
    double leftEstimate = leftFactor * leftRms;
    
    TreeNode * rightNode = nodeP->lessOrEqualNode;
    index_t rightCount = rightNode->leafLessOrEqualCount + rightNode->leafGreaterOrNotCount;
    double rightRms = sqrt(rightNode->branchSum2 / rightCount);
    double rightFactor = (rightCount + 1.0) / (rightCount - 1.0);
    double rightEstimate = rightFactor * rightRms;
    
    double weightedChildEstimate = leftEstimate * leftCount / nodeCount +
    rightEstimate * rightCount / nodeCount;
    
    bool result = nodeEstimate < weightedChildEstimate;
    
//    CERR << "nPrune(" << nodeP->index << ") = " << (result ? "true" : "false") << endl;
    
    return result;
}

// if all the terminal nodes of specified subtree point to the same category, return index of
// category, else return NO_INDEX
index_t sameCategoryForAllLeaves(const TreeNode *nodeP)
{
    index_t result;
    
    if (nodeP->lessOrEqualNode == NULL) {
        result = nodeP->leafValue.number.i;
        
    } else {
        index_t leftResult = sameCategoryForAllLeaves(nodeP->lessOrEqualNode);
        
        if (leftResult == NO_INDEX) {
            result = NO_INDEX;
            
        } else {
            index_t rightResult = sameCategoryForAllLeaves(nodeP->greaterOrNotNode);
            
            if (leftResult == rightResult) {
                result = leftResult;
                
            } else {
                result = NO_INDEX;
            }
        }
    }
    
    return result;
}

// ========== Tests ================================================================================

// component tests
void ctest_prune(int& totalPassed, int& totalFailed, bool verbose)
{
    int passed = 0;
    int failed = 0;
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // pruneTree
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // updateBranchCategorical
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // updateBranchNumeric
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // findBranchNodes
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // compareDepth
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // pessimisticErrorEstimate
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // printLeaves
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // printNodes
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // testReplaceSubtreeCategorical
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // testReplaceSubtreeNumeric
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // sameCategoryForAllLeaves
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    
    if (verbose) {
        CERR << "prune.cpp" << "\t" << passed << " passed, " << failed << " failed" << endl;
    }
    
    totalPassed += passed;
    totalFailed += failed;    
}

// code coverage
void cover_prune(bool verbose)
{
    // ~~~~~~~~~~~~~~~~~~~~~~
    // pruneTree
    // updateBranchCategorical
    // updateBranchNumeric
    // findBranchNodes
    // compareDepth
    // pessimisticErrorEstimate
    // testReplaceSubtreeCategorical
    // testReplaceSubtreeNumeric
    
    // handled via cover_train()
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // sameCategoryForAllLeaves

    // handled via cover_train() plus following additional code

    Value niValue;
    niValue.na = false;
    niValue.number.i = NO_INDEX;
    
    TreeNode root;
    root.parentNode = NULL;
    root.splitColIndex = NO_INDEX;
    root.leafValue = niValue;
    root.splitValue = gNaValue;
    root.lessOrEqualNode = NULL;
    root.greaterOrNotNode = NULL;
    root.leafLessOrEqualCount = 0;
    root.leafGreaterOrNotCount = 0;
    root.selectRows = SelectIndexes(0, true);
    root.index = 0;
    
    TreeNode left;
    left.parentNode = &root;
    left.splitColIndex = NO_INDEX;
    left.leafValue = niValue;
    left.splitValue = gNaValue;
    left.lessOrEqualNode = NULL;
    left.greaterOrNotNode = NULL;
    left.leafLessOrEqualCount = 0;
    left.leafGreaterOrNotCount = 0;
    left.selectRows = SelectIndexes(0, true);
    left.index = 1;
    
    root.lessOrEqualNode = &left;
    
    TreeNode right;
    right.parentNode = &root;
    right.splitColIndex = NO_INDEX;
    right.leafValue = niValue;
    right.splitValue = gNaValue;
    right.lessOrEqualNode = NULL;
    right.greaterOrNotNode = NULL;
    right.leafLessOrEqualCount = 0;
    right.leafGreaterOrNotCount = 0;
    right.selectRows = SelectIndexes(0, true);
    right.index = 2;
    
    root.greaterOrNotNode = &right;
    
    sameCategoryForAllLeaves(&root);

    // ~~~~~~~~~~~~~~~~~~~~~~
    // printLeaves
    
    if (verbose) {
        set<TreeNode *> leafSet;
        leafSet.insert(&root);
        leafSet.insert(&left);
        leafSet.insert(&right);
        
        printLeaves(leafSet);
    }
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // printNodes
    
    if (verbose) {
        vector<pair<int, TreeNode *> > nodes;
        nodes.push_back(make_pair(0, &root));
        nodes.push_back(make_pair(1, &left));
        nodes.push_back(make_pair(2, &right));
        
        printNodes(nodes);
    }
}



