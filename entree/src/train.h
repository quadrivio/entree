//
//  train.h
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

#ifndef entree_train_h
#define entree_train_h

#include "format.h"
#include "utils.h"

#include <string>
#include <vector>

// ========== Types ================================================================================

// compact form of decision tree, containing only necessary and sufficient data for model
// (no intermediate or diagnostic results included)
struct CompactTree {
    std::vector<index_t> splitColIndex;     // index into selectColumns of column of split attribute
                                            // NO_INDEX if leaf
    
    std::vector<index_t> lessOrEqualIndex;  // NO_INDEX if leaf
    std::vector<index_t> greaterOrNotIndex; // NO_INDEX if leaf
    std::vector<bool> toLessOrEqualIfNA;    // when have NA to compare with splitValue, choose
                                            // lessOrEqualNode if true else choose greaterOrNotNode
    
    std::vector<Number> value;              // value for leaf or split
};
typedef struct CompactTree CompactTree;

// one node of a decision tree; contains intermediate results
struct TreeNode {
    Value leafValue;    // value to use if this is leaf node
    Value splitValue;   // value to use for splitting into two branches
    TreeNode *parentNode;           // NULL if root
    TreeNode *lessOrEqualNode;      // NULL if leaf
    TreeNode *greaterOrNotNode;     // NULL if leaf
    bool toLessOrEqualIfNA;         // when have NA to compare with splitValue, choose
                                    // lessOrEqualNode if true else choose greaterOrNotNode
    
    index_t splitColIndex;          // index into selectColumns of column of split attribute
    index_t leafLessOrEqualCount;   // count of training rows that go to lessOrEqualNode
    index_t leafGreaterOrNotCount;  // count of training rows that go to greaterOrNotNode
    
    union {
        double branchSum2;          // for calculating quality when attribute is numeric
        index_t branchCorrectCount; // for calculating quality when attribute is categorical
    };
    
    SelectIndexes selectRows;   // training rows that reach this node
    size_t index;               // serial index of node
};
typedef struct TreeNode TreeNode;

// ========== Function Headers =====================================================================

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
           std::vector<ImputeOption>& imputeOptions);

// delete all nodes for which specified node is ancestor
void deleteSubtrees(TreeNode *nodeP);

// for debugging; print list of decision trees
void printCompactTrees(const std::vector<CompactTree>& trees,
                       const std::vector<ValueType>& valueTypes,
                       size_t targetColumn,
                       const SelectIndexes& selectColumns,
                       const std::vector<std::string>& colNames,
                       const std::vector<CategoryMaps>& categoryMaps);

// for debugging and testing; return the fraction of matches between selected rows of two vectors of
// categorical Values
double compareMatch(const std::vector<Value>& values1,
                    const std::vector<Value>& values2,
                    const SelectIndexes& selectRows);

// for debugging and testing; return the rms difference between selected rows of two vectors of
// numerical Values
double compareRms(const std::vector<Value>& values1,
                  const std::vector<Value>& values2,
                  const SelectIndexes& selectRows);

// component tests
void ctest_train(int& totalPassed, int& totalFailed, bool verbose);

// code coverage
void cover_train(bool verbose);

#endif
