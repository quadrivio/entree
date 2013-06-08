//
//  prune.h
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

#ifndef entree_prune_h
#define entree_prune_h

#include "format.h"
#include "train.h"

// ========== Function Headers =====================================================================

// try reducing size of tree
void pruneTree(TreeNode& root,
               const std::vector< std::vector<Value> >& values,
               const std::vector<ValueType>& valueTypes,
               size_t targetColumn,
               const std::vector<CategoryMaps>& categoryMaps,
               const std::vector< std::vector<size_t> >& sortedIndexes,
               const std::vector<std::string>& colNames);

// component tests
void ctest_prune(int& totalPassed, int& totalFailed, bool verbose);

// code coverage
void cover_prune(bool verbose);

#endif
