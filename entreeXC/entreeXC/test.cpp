//
//  test.cpp
//  entree
//
//  Created by MPB on 5/29/13.
//  Copyright (c) 2015 Quadrivio Corporation. All rights reserved.
//  License http://opensource.org/licenses/BSD-2-Clause
//          <YEAR> = 2015
//          <OWNER> = Quadrivio Corporation
//

//
// Component, code coverage, and integration tests
//

#include "test.h"

#include "crime.h"
#include "csv.h"
#include "format.h"
#include "iris.h"
#include "predict.h"
#include "prune.h"
#include "subsets.h"
#include "train.h"
#include "utils.h"

#include <cmath>
#include <iostream>
#include <iomanip>

using namespace std;

void test(bool verbose)
{
    if (verbose) {
        CERR << "Testing" << endl;
        CERR << "Working directory: " << getWorkingDirectory() << endl;
    }
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // Component tests
    
    if (verbose) {
        CERR << endl << "Component tests" << endl;
    }
    
    int totalPassed = 0;
    int totalFailed = 0;
    
    ctest_csv(totalPassed, totalFailed, verbose);
    ctest_format(totalPassed, totalFailed, verbose);
    ctest_predict(totalPassed, totalFailed, verbose);
    ctest_prune(totalPassed, totalFailed, verbose);
    ctest_subsets(totalPassed, totalFailed, verbose);
    ctest_train(totalPassed, totalFailed, verbose);
    ctest_utils(totalPassed, totalFailed, verbose);
    
    if (verbose) {
        CERR << "Total" << "\t\t" << totalPassed << " passed, " << totalFailed << " failed" << endl;
    }

    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // Code coverage
    
    if (verbose) {
        CERR << endl << "Code coverage" << endl;
    }
    
    cover_csv(verbose);
    cover_format(verbose);
    cover_predict(verbose);
    cover_prune(verbose);
    cover_subsets(verbose);
    cover_train(verbose);
    cover_utils(verbose);
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // Integration

    if (verbose) {
        CERR << endl << "Integration tests" << endl;
    }

    if (test_iris(verbose)) {
        totalPassed++;
        
    } else {
        CERR << "test_iris() failed" << endl;
        totalFailed++;
    }
    
    if (test_commandLine(verbose)) {
        totalPassed++;
        
    } else {
        CERR << "test_commandLine() failed" << endl;
        totalFailed++;
    }
    
    if (test_allCategorical(verbose)) {
        totalPassed++;
        
    } else {
        CERR << "test_allCategorical() failed" << endl;
        totalFailed++;
    }
    
    if (test_crime(verbose)) {
        totalPassed++;
        
    } else {
        CERR << "test_crime() failed" << endl;
        totalFailed++;
    }
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    
    if (totalFailed > 0) {
        CERR << "Tests failed" << endl;
        
    } else {
        CERR << "Tests OK" << endl;
    }
}

// used in test_commandLine()
int main (int argc, const char * argv[]);

// test command-line interface
bool test_commandLine(bool verbose)
{
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // write test files
    
#define ATTRIBUTES_PATH "iris.attributes.csv"
#define RESPONSE_PATH "iris.response.csv"
#define MODEL_PATH "iris.model.csv"
#define PREDICT_PATH "iris.predict.csv"
    
    vector< vector<string> > cells;
    vector< vector<bool> > quoted;
    vector<string> colNames;
    
    readCsvString(gIris, cells, quoted, colNames);
    
    size_t numRows = cells.size();
    size_t numCols = cells[0].size();
    
    vector< vector<string> > attributeCells;
    vector< vector<bool> > attributeQuoted;
    
    vector<string> attributeColNames = colNames;
    attributeColNames.resize(numCols - 1);
    
    for (size_t row = 0; row < numRows; row++) {
        vector<string> line = cells[row];
        line.resize(numCols - 1);
        attributeCells.push_back(line);
        
        vector<bool> lineQuoted = quoted[row];
        lineQuoted.resize(numCols - 1);
        attributeQuoted.push_back(lineQuoted);
    }
    
    writeCsvPath(ATTRIBUTES_PATH, attributeCells, attributeQuoted, attributeColNames);
    
    vector< vector<string> > responseCells;
    vector< vector<bool> > responseQuoted;
    
    vector<string> responseColNames;
    responseColNames.push_back(colNames[numCols - 1]);

    for (size_t row = 0; row < numRows; row++) {
        responseCells.push_back(vector<string>(1, cells[row][numCols - 1]));
        
        responseQuoted.push_back(vector<bool>(1, quoted[row][numCols - 1]));
    }
    
    writeCsvPath(RESPONSE_PATH, responseCells, responseQuoted, responseColNames);
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // call train
    
    {
        int argc = 24;
        const char *argv[] = {
            (char *)"entree",
            (char *)"-T",
            
            (char *)"-a",
            (char *)ATTRIBUTES_PATH,
            
            (char *)"-r",
            (char *)RESPONSE_PATH,
            
            (char *)"-m",
            (char *)MODEL_PATH,
            
            (char *)"-c",
            (char *)"4",    // columnsPerTree
            
            (char *)"-d",
            (char *)"100",  // maxDepth
            
            (char *)"-l",
            (char *)"1",    // minLeafCount
            
            (char *)"-s",
            (char *)"-1",   // maxSplitsPerNumericAttribute
            
            (char *)"-t",
            (char *)"1",    // maxTrees
            
            (char *)"-u",
            (char *)"1",    // doPrune
            
            (char *)"-e",
            (char *)"0",    // minDepth
            
            (char *)"-n",
            (char *)"100"   // maxNodes
        };

        main(argc, argv);
    }
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // call predict
    
    {
        int argc = 8;
        const char *argv[] = {
            (char *)"entree",
            (char *)"-P",
            (char *)"-a",
            (char *)ATTRIBUTES_PATH,
            (char *)"-r",
            (char *)PREDICT_PATH,
            (char *)"-m",
            (char *)MODEL_PATH
        };
        
        main(argc, argv);
    }
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // check results
    
    vector< vector<Value> > values;
    vector<ValueType> valueTypes;
    vector<CategoryMaps> categoryMaps;

    getDefaultValueTypes(cells, quoted, true, "NA", valueTypes);
    cellsToValues(cells, quoted, valueTypes, true, "NA", values, false, categoryMaps);

    size_t targetColumn = numCols - 1;
    
    vector< vector<string> > predictCells;
    vector< vector<bool> > predictQuoted;
    vector<string> predictColNames;
    
    readCsvPath(PREDICT_PATH, predictCells, predictQuoted, predictColNames);
    
    vector< vector<Value> > yValues(1, vector<Value>(numRows, gNaValue));
    vector<ValueType> yValueTypes(1, valueTypes[targetColumn]);
    vector<CategoryMaps> yCategoryMaps(1, categoryMaps[targetColumn]); 
    
    cellsToValues(predictCells, predictQuoted, yValueTypes, true, "NA", yValues, true, yCategoryMaps);
    
    SelectIndexes selectRows(numRows, true);
    
    double result = compareMatch(values[targetColumn], yValues[0], selectRows);
    
    bool success = (int)round(100 * result) == 100;
    
    if (verbose || !success) {
        CERR << "command line iris data compareMatch = " << fixed << setprecision(2) << result << endl;
    }

    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // delete test files
    
    const bool deleteFilesWhenDone = false;    // change to keep files for debugging
    
    if (deleteFilesWhenDone) {
        remove(ATTRIBUTES_PATH);    
        remove(RESPONSE_PATH);    
        remove(MODEL_PATH);    
        remove(PREDICT_PATH);    
    }
    
    return success;
}

// test classic iris data set
bool test_iris(bool verbose)
{
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // read and format data
    
    vector< vector<Value> > values;
    vector<ValueType> valueTypes;
    vector<CategoryMaps> categoryMaps;
    vector<string> colNames;
    
    vector< vector<string> > cells;
    vector< vector<bool> > quoted;
    
    readCsvString(gIris, cells, quoted, colNames);
    getDefaultValueTypes(cells, quoted, true, "NA", valueTypes);
    cellsToValues(cells, quoted, valueTypes, true, "NA", values, false, categoryMaps);
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // prepare training parameters
    
    size_t numCols = values.size();
    size_t numRows = values[0].size();
    
    SelectIndexes selectRows(numRows, true);
    vector<ImputeOption> imputeOptions(numCols, kToDefault);
    
    vector<CompactTree> trees;
    SelectIndexes selectColumns;
    
    int maxDepth = 100;
    double minImprovement = 0.0;
    index_t minLeafCount = 1;
    index_t maxSplitsPerNumericAttribute = -1;
    index_t maxNodes = 100;
    
    index_t maxTrees = 1;
    index_t columnsPerTree = 4;
    int minDepth = 0;
    bool doPrune = true;
    size_t targetColumn = values.size() - 1;
    
    SelectIndexes availableColumns(numCols, true);
    availableColumns.unselect(targetColumn);
    
    vector< vector<Value> > trainValues = values;
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // train
    
    train(trees, columnsPerTree, maxDepth, minDepth, doPrune, minImprovement, minLeafCount,
          maxSplitsPerNumericAttribute, maxTrees, maxNodes, selectRows, availableColumns,
          selectColumns, trainValues, valueTypes, categoryMaps, targetColumn, colNames,
          imputeOptions);
    
    if (verbose) {
        printCompactTrees(trees, valueTypes, targetColumn, selectColumns, colNames,
                          categoryMaps);
    }
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // predict from training data
    
    vector< vector<Value> > predictValues = values;
    
    predict(predictValues, valueTypes, categoryMaps, targetColumn, selectRows, selectColumns, trees,
            colNames);
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
    // expect 100% match
    
    double result = compareMatch(trainValues[targetColumn], predictValues[targetColumn],
                                 selectRows);
    
    bool success = (int)round(100 * result) == 100;
    
    if (verbose || !success) {
        CERR << "iris data compareMatch = " << fixed << setprecision(2) << result << endl;
    }
    
    return success;
}

// test crime data regression
bool test_crime(bool verbose)
{
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // read and format data
    
    vector< vector<Value> > values;
    vector<ValueType> valueTypes;
    vector<CategoryMaps> categoryMaps;
    vector<string> colNames;
    
    vector< vector<string> > cells;
    vector< vector<bool> > quoted;
    
    readCsvString(gCrime, cells, quoted, colNames);
    getDefaultValueTypes(cells, quoted, true, "?", valueTypes);
    cellsToValues(cells, quoted, valueTypes, true, "?", values, false, categoryMaps);
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // prepare training parameters
    
    size_t numCols = values.size();
    size_t numRows = values[0].size();
    
    if (verbose) {
        CERR << "Crime data " << numRows << " rows, " << numCols << " columns" << endl;
    }
    
    vector<ImputeOption> imputeOptions(numCols, kToDefault);
    
    vector<CompactTree> trees;
    SelectIndexes selectColumns;
    
    int maxDepth = 10;
    double minImprovement = 0.0;
    index_t minLeafCount = 4;
    index_t maxSplitsPerNumericAttribute = 2;
    index_t maxNodes = 1000;
    
    index_t maxTrees = 20;
    index_t columnsPerTree = -1;
    int minDepth = 2;
    bool doPrune = false;
    
    SelectIndexes availableColumns(numCols, true);
    
    for (size_t k = 0; k <= 4; k++) {
        availableColumns.unselect(k);
    }
    
    for (size_t k = numCols - 18; k < numCols; k++) {
        availableColumns.unselect(k);
    }
    
    size_t targetColumn = numCols - 2;
    
    SelectIndexes selectRows(numRows, false);
    for (size_t row = 0; row < numRows; row++) {
        if (!values[targetColumn][row].na) {
            selectRows.select(row);   
        }
    }
    
    vector< vector<Value> > trainValues = values;
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // train
    
    train(trees, columnsPerTree, maxDepth, minDepth, doPrune, minImprovement, minLeafCount,
          maxSplitsPerNumericAttribute, maxTrees, maxNodes, selectRows, availableColumns,
          selectColumns, trainValues, valueTypes, categoryMaps, targetColumn, colNames,
          imputeOptions);
    
    if (verbose) {
        printCompactTrees(trees, valueTypes, targetColumn, selectColumns, colNames,
                          categoryMaps);
    }
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // predict from training data
    
    vector< vector<Value> > predictValues = values;
    
    predict(predictValues, valueTypes, categoryMaps, targetColumn, selectRows, selectColumns, trees,
            colNames);
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
    // expect rms error match within 0.1%
    
    double result = compareRms(trainValues[targetColumn], predictValues[targetColumn], selectRows);
    
    double benchmark = 235.702179;
    
    // probably safe against this much difference due to rounding or numerics on different systems
    bool success = abs(benchmark - result) < 0.001 * benchmark;
    
    if (verbose || !success) {
        CERR << "crime data compareRms = " << fixed << setprecision(6) << result << endl;
        CERR << "crime data benchmark  = " << fixed << setprecision(6) << benchmark << endl;
    }
    
    return success;
}

// test simple all-categorical data set
bool test_allCategorical(bool verbose)
{
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // read and format data
    
    string data =
    "       C0,     C1,     C2,     C3,     C4,     C5\n"
    "       A,      C,      F,      G,      I,      X\n"
    "       B,      C,      E,      G,      J,      X\n"
    "       B,      D,      E,      G,      J,      X\n"
    "       B,      D,      F,      G,      J,      Y\n"
    "       B,      D,      F,      H,      K,      Y\n";
    
    vector< vector<Value> > values;
    vector<ValueType> valueTypes;
    vector<CategoryMaps> categoryMaps;
    vector<string> colNames;
    
    vector< vector<string> > cells;
    vector< vector<bool> > quoted;
    
    readCsvString(data, cells, quoted, colNames);
    getDefaultValueTypes(cells, quoted, true, "NA", valueTypes);
    cellsToValues(cells, quoted, valueTypes, true, "NA", values, false, categoryMaps);
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // prepare training parameters
    
    size_t numCols = values.size();
    size_t numRows = values[0].size();
    
    SelectIndexes selectRows(numRows, true);
    vector<ImputeOption> imputeOptions(numCols, kToDefault);
    
    if (verbose) {
        printValues(values, valueTypes, categoryMaps, colNames);
    }
    
    vector<CompactTree> trees;
    SelectIndexes selectColumns;
    
    int maxDepth = 100;
    double minImprovement = 0.0;
    index_t minLeafCount = 1;
    index_t maxSplitsPerNumericAttribute = -1;
    index_t maxNodes = 100;
    
    index_t maxTrees = 1;
    index_t columnsPerTree = 5;
    int minDepth = 0;
    bool doPrune = false;
    size_t targetColumn = 5;
    
    SelectIndexes availableColumns(numCols, true);
    availableColumns.unselect(targetColumn);
    
    vector< vector<Value> > trainValues = values;
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // train
    
    train(trees, columnsPerTree, maxDepth, minDepth, doPrune, minImprovement, minLeafCount,
          maxSplitsPerNumericAttribute, maxTrees, maxNodes, selectRows, availableColumns,
          selectColumns, trainValues, valueTypes, categoryMaps, targetColumn, colNames,
          imputeOptions);
    
    if (verbose) {
        printCompactTrees(trees, valueTypes, targetColumn, selectColumns, colNames, categoryMaps);
    }
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
    // predict from training data
    
    vector< vector<Value> > predictValues = values;
    
    predict(predictValues, valueTypes, categoryMaps, targetColumn, selectRows, selectColumns, trees,
            colNames);
    
    // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
    // expect 100% match
    
    double result = compareMatch(trainValues[targetColumn], predictValues[targetColumn],
                                 selectRows);
    
    bool success = (int)round(100 * result) == 100;
    
    if (verbose || !success) {
        CERR << "allCategorical compareMatch = " << fixed << setprecision(2) << result << endl;
    }
    
    return success;
}
