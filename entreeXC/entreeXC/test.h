//
//  test.h
//  entree
//
//  Created by MPB on 5/29/13.
//  Copyright (c) 2013 Quadrivio Corporation. All rights reserved.
//  License http://opensource.org/licenses/BSD-2-Clause
//          <YEAR> = 2013
//          <OWNER> = Quadrivio Corporation
//

//
// Component, code coverage, and integration tests
//

#ifndef entree_test_h
#define entree_test_h

void test(bool verbose);

// test classic iris data set
bool test_iris(bool verbose);

// test crime data regression
bool test_crime(bool verbose);

// test simple all-categorical data set
bool test_allCategorical(bool verbose);

// test command-line interface
bool test_commandLine(bool verbose);

#endif
