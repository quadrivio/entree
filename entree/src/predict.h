//
//  predict.h
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

#ifndef entree_predict_h
#define entree_predict_h

#include "format.h"
#include "train.h"

// ========== Function Headers =====================================================================

// predict response from ensemble of decision trees and array of Values
void predict(std::vector< std::vector<Value> >& values,
             const std::vector<ValueType>& valueTypes,
             const std::vector<CategoryMaps>& categoryMaps,
             size_t targetColumn,
             const SelectIndexes& selectRows,
             const SelectIndexes& selectColumns,
             const std::vector<CompactTree>& trees,
             const std::vector<std::string>& colNames);

// component tests
void ctest_predict(int& totalPassed, int& totalFailed, bool verbose);

// code coverage
void cover_predict(bool verbose);

#endif
