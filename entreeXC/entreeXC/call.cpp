//
//  call.cpp
//  entree
//
//  Created by MPB on 5/29/13.
//  Copyright (c) 2013 Quadrivio Corporation. All rights reserved.
//  License http://opensource.org/licenses/BSD-2-Clause
//          <YEAR> = 2013
//          <OWNER> = Quadrivio Corporation
//

//
// Process command-line arguments and call train() or predict()
//

#include <fstream>  // must preceed .h includes

#include "call.h"

#include "csv.h"
#include "format.h"
#include "predict.h"
#include "train.h"

#include <iomanip>

using namespace std;

// ========== Local Headers ========================================================================

void writeModel(const string& modelFile,
                const vector<ValueType>& valueTypes,
                const vector<CategoryMaps>& categoryMaps,
                size_t targetColumn,
                const SelectIndexes& selectColumns,
                const vector<ImputeOption>& imputeOptions,
                const vector<CompactTree>& trees,
                const vector<string>& colNames);

void readModel(const string& modelFile,
               vector<ValueType>& valueTypes,
               vector<CategoryMaps>& categoryMaps,
               size_t& targetColumn,
               SelectIndexes& selectColumns,
               vector<ImputeOption>& imputeOptions,
               vector<CompactTree>& trees,
               vector<string>& colNames);

// ========== Functions ============================================================================

void callTrain(const std::string& attributesFile,
               const std::string& responseFile,
               const std::string& modelFile,
               const std::string& typeFile,
               const std::string& imputeFile,
               const std::string& columnsPerTreeStr,
               const std::string& maxDepthStr,
               const std::string& minLeafCountStr,
               const std::string& maxSplitsPerNumericAttributeStr,
               const std::string& maxTreesStr,
               const std::string& doPruneStr,
               const std::string& minDepthStr,
               const std::string& maxNodesStr,
               const std::string& minImprovementStr)
{
    vector<CompactTree> trees;
    index_t columnsPerTree = -1;
    int maxDepth = 500;
    int minDepth = 1;
    bool doPrune = false;
    double minImprovement = 0.0;
    index_t minLeafCount = 4;
    index_t maxSplitsPerNumericAttribute = -1;
    index_t maxTrees = 1000;
    index_t maxNodes = -1;
    SelectIndexes selectRows;
    SelectIndexes availableColumns;
    SelectIndexes selectColumns;
    vector< vector<Value> > values;
    vector<ValueType> valueTypes;
    vector<CategoryMaps> categoryMaps;
    size_t targetColumn;
    vector<string> colNames;
    vector<ImputeOption> imputeOptions;
    
    // read parameters
    
    if (!columnsPerTreeStr.empty()) {
        columnsPerTree = (index_t)toLong(columnsPerTreeStr);    
    }
    
    if (!maxDepthStr.empty()) {
        maxDepth = (int)toLong(maxDepthStr);    
    }
    
    if (!minLeafCountStr.empty()) {
        minLeafCount = (index_t)toLong(minLeafCountStr);    
    }
    
    if (!maxSplitsPerNumericAttributeStr.empty()) {
        maxSplitsPerNumericAttribute = (index_t)toLong(maxSplitsPerNumericAttributeStr);    
    }
    
    if (!maxTreesStr.empty()) {
        maxTrees = (index_t)toLong(maxTreesStr);    
    }
    
    if (!doPruneStr.empty()) {
        doPrune = toLong(doPruneStr) != 0;    
    }
    
    if (!minDepthStr.empty()) {
        minDepth = (int)toLong(minDepthStr);    
    }
    
    if (!maxNodesStr.empty()) {
        maxNodes = (index_t)toLong(maxNodesStr);    
    }
    
    if (!minImprovementStr.empty()) {
        minImprovement = toDouble(minImprovementStr);    
    }
    
    // read files
    
    if (!typeFile.empty()) {
        vector< vector<string> > cells;
        vector< vector<bool> > quoted;
        
        readCsvPath(typeFile, cells, quoted);
        
        valueTypes.clear();
        
        for (size_t k = 0; k < cells.at(0).size(); k++) {
            valueTypes.push_back(stringToValueType(cells.at(0).at(k)));    
        }
    }
    
    bool deduceValueTypes = valueTypes.empty();
    
    size_t numRows = 0;
    
    if (!attributesFile.empty()) {
        vector< vector<string> > cells;
        vector< vector<bool> > quoted;
        
        readCsvPath(attributesFile, cells, quoted, colNames);
        
        RUNTIME_ERROR_IF(!uniformRowLengths(cells, colNames), "mismatched row lengths in attributes");
        
        if (deduceValueTypes) {
            getDefaultValueTypes(cells, quoted, true, "NA", valueTypes);
            
        } else {
            RUNTIME_ERROR_IF(valueTypes.size() != colNames.size(), "valueTypes and attributes size mismatch");
        }
                
        cellsToValues(cells, quoted, valueTypes, true, "NA", values, false, categoryMaps);
        
        numRows = values[0].size();
        
    } else {
        RUNTIME_ERROR_IF(true, "empty attributes file");    
    }
    
    targetColumn = values.size();
    
    if (!responseFile.empty()) {
        vector< vector<string> > cells;
        vector< vector<bool> > quoted;
        vector<string> yColNames;
        
        readCsvPath(responseFile, cells, quoted, yColNames);
        
        RUNTIME_ERROR_IF(!uniformRowLengths(cells, yColNames), "mismatched row lengths in response");

        colNames.push_back(yColNames.at(0));

        vector< vector<Value> > yValues;
        vector<ValueType> yValueTypes;
        vector<CategoryMaps> yCategoryMaps;

        if (deduceValueTypes) {
            getDefaultValueTypes(cells, quoted, true, "NA", yValueTypes);    
            valueTypes.push_back(yValueTypes.at(0));
            
        } else {
            yValueTypes.push_back(valueTypes[targetColumn]);
        }

        cellsToValues(cells, quoted, yValueTypes, true, "NA", yValues, false, yCategoryMaps);
        
        RUNTIME_ERROR_IF(numRows != yValues[0].size(), "attributes and response size mismatch");
        
        values.push_back(yValues.at(0));
        categoryMaps.push_back(yCategoryMaps.at(0));
        
    } else {
        RUNTIME_ERROR_IF(true, "empty response file");    
    }
    
    size_t numCols = values.size();
    
    if (!imputeFile.empty()) {
        vector< vector<string> > cells;
        vector< vector<bool> > quoted;
        
        readCsvPath(imputeFile, cells, quoted);

        RUNTIME_ERROR_IF(!uniformRowLengths(cells), "mismatched row lengths in impute options");

        for (size_t k = 0; k < cells.at(0).size(); k++) {
            imputeOptions.push_back(stringToImputeOption(cells.at(0).at(k), valueTypes.at(k)));    
        }
        
        RUNTIME_ERROR_IF(numCols - 1 != imputeOptions.size(),
                         "attributes and impute options size mismatch");
        
    } else {
        for (size_t k = 0; k < numCols - 1; k++) {
            imputeOptions.push_back(kToDefault);    
        }
    }
    
    imputeOptions.push_back(kNoImpute);    // target column
    
    selectRows.selectAll(numRows);
    
    availableColumns.selectAll(numCols);
    availableColumns.unselect(targetColumn);

    // train

    train(trees, columnsPerTree, maxDepth, minDepth, doPrune, minImprovement, minLeafCount,
          maxSplitsPerNumericAttribute, maxTrees, maxNodes, selectRows, availableColumns,
          selectColumns, values, valueTypes, categoryMaps, targetColumn, colNames, imputeOptions);
    
    // write model
    
    writeModel(modelFile, valueTypes, categoryMaps, targetColumn, selectColumns, imputeOptions,
               trees, colNames);
    
}

void callPredict(const std::string& attributesFile,
                 const std::string& responseFile,
                 const std::string& modelFile)
{
    vector< vector<Value> > values;
    vector<ValueType> valueTypes;
    vector<CategoryMaps> categoryMaps;
    size_t targetColumn;
    vector<ImputeOption> imputeOptions;
    SelectIndexes selectRows;
    SelectIndexes selectColumns;
    vector<CompactTree> trees;
    vector<string> colNames;
    
    // read files

    readModel(modelFile, valueTypes, categoryMaps, targetColumn, selectColumns, imputeOptions,
              trees, colNames);
    
    size_t numCols = valueTypes.size();
    size_t numRows = 0;
    
    targetColumn = numCols - 1;
    
    if (!attributesFile.empty()) {
        vector< vector<string> > cells;
        vector< vector<bool> > quoted;
        vector<string> attributeColNames;
        
        readCsvPath(attributesFile, cells, quoted, attributeColNames);
        
        RUNTIME_ERROR_IF(!uniformRowLengths(cells, attributeColNames), "mismatched row lengths in attributes");
        
        RUNTIME_ERROR_IF(attributeColNames.size() != numCols - 1, "attributes and model size mismatch");
        
        for (size_t col = 0; col < attributeColNames.size(); col++) {
            RUNTIME_ERROR_IF(attributeColNames[col] != colNames[col], "attributes and model columns mismatch");
        }
        
        ValueType yValueType = valueTypes.at(numCols - 1);
        CategoryMaps yCategoryMaps = categoryMaps.at(numCols - 1);
        
        valueTypes.resize(numCols - 1);
        categoryMaps.resize(numCols - 1);
        
        cellsToValues(cells, quoted, valueTypes, true, "NA", values, true, categoryMaps);
        
        numRows = values[0].size();
        
        valueTypes.push_back(yValueType);
        categoryMaps.push_back(yCategoryMaps);
        
    } else {
        RUNTIME_ERROR_IF(true, "empty attributes file");    
    }
    
    values.push_back(vector<Value>(numRows, gNaValue));
    selectRows.selectAll(numRows);

    // predict

    predict(values, valueTypes, categoryMaps, targetColumn, selectRows, selectColumns, trees,
            colNames);
   
    // write prediction

    vector< vector<Value> > yValues;
    vector<ValueType> yValueTypes;
    vector<CategoryMaps> yCategoryMaps;
    vector<string> yColNames;
    
    yValues.push_back(values.at(targetColumn));
    yValueTypes.push_back(valueTypes.at(targetColumn));
    yCategoryMaps.push_back(categoryMaps.at(targetColumn));
    yColNames.push_back(colNames.at(targetColumn));
    
    vector< vector<string> > yCells;
    vector< vector<bool> > yQuoted;

    valuesToCells(yValues, yValueTypes, yCategoryMaps, true, "NA", yCells, yQuoted);
    
    writeCsvPath(responseFile, yCells, yQuoted, yColNames);
    
}

// ========== Local Functions ======================================================================

void writeModel(const string& modelFile,
                const vector<ValueType>& valueTypes,
                const vector<CategoryMaps>& categoryMaps,
                size_t targetColumn,
                const SelectIndexes& selectColumns,
                const vector<ImputeOption>& imputeOptions,
                const vector<CompactTree>& trees,
                const vector<string>& colNames)
{
    ofstream ofs(modelFile.c_str());
    
    RUNTIME_ERROR_IF(!ofs.good(), badPathErrorMessage(modelFile));

    // ~~~~~~~~ valueTypes ~~~~~~~~

    ofs << "valueTypes" << endl;
        
    for (size_t col = 0; col < valueTypes.size(); col++) {
        string str;
        valueTypeToString(valueTypes[col], str);
        ofs << '"' << str << '"' << endl;
    }

    ofs << endl;
    
    // ~~~~~~~~ categoryMaps ~~~~~~~~
    
    ofs << "useNaCategory" << endl;
    
    for (size_t col = 0; col < categoryMaps.size(); col++) {
        ofs << (int)categoryMaps[col].getUseNaCategory() << endl;
    }
    
    ofs << endl;

    for (size_t col = 0; col < categoryMaps.size(); col++) {
        ofs << "categories." << col << endl;
        
        size_t numCategories = categoryMaps[col].countNamedCategories();
        for (index_t index = 0; index < (index_t)numCategories; index++) {
            string str = categoryMaps[col].getCategoryForIndex(index);
            ofs << '"' << str << '"' << endl;
        }
        
        ofs << endl;
    }

    // ~~~~~~~~ targetColumn ~~~~~~~~
    
    ofs << "targetColumn" << endl;
    ofs << targetColumn << endl;
    ofs << endl;
    
    // ~~~~~~~~ selectColumns ~~~~~~~~

    ofs << "selectColumns" << endl;

    const vector<size_t>& selectColumnIndexes = selectColumns.indexVector();
    
    for (size_t index = 0; index < selectColumnIndexes.size(); index++) {
        ofs << selectColumnIndexes[index] << endl;
    }
    
    ofs << endl;
    
    // ~~~~~~~~ imputeOptions ~~~~~~~~
    
    ofs << "imputeOptions" << endl;
    
    for (size_t col = 0; col < imputeOptions.size(); col++) {
        string str;
        imputeOptionToString(imputeOptions[col], str);
        
        ofs << '"' << str << '"' << endl;
    }
    
    ofs << endl;
    
    // ~~~~~~~~ trees ~~~~~~~~
    
    ofs << "numTrees" << endl;
    ofs << trees.size() << endl;
    ofs << endl;
    
    for (size_t tree = 0; tree < trees.size(); tree++) {
        ofs << "splitColIndex." << tree << endl;
        for (size_t k = 0; k < trees[tree].splitColIndex.size(); k++) {
            ofs << trees[tree].splitColIndex[k] << endl;
        }
        
        ofs << endl;
        
        ofs << "lessOrEqualIndex." << tree << endl;
        for (size_t k = 0; k < trees[tree].lessOrEqualIndex.size(); k++) {
            ofs << trees[tree].lessOrEqualIndex[k] << endl;
        }
        
        ofs << endl;
        
        ofs << "greaterOrNotIndex." << tree << endl;
        for (size_t k = 0; k < trees[tree].greaterOrNotIndex.size(); k++) {
            ofs << trees[tree].greaterOrNotIndex[k] << endl;
        }
        
        ofs << endl;
        
        ofs << "toLessOrEqualIfNA." << tree << endl;
        for (size_t k = 0; k < trees[tree].toLessOrEqualIfNA.size(); k++) {
            ofs << (int)trees[tree].toLessOrEqualIfNA[k] << endl;
        }
        
        ofs << endl;
        
        const vector<size_t>& selectColumnIndexes = selectColumns.indexVector();
        
        ofs << "value." << tree << endl;
        for (size_t k = 0; k < trees[tree].value.size(); k++) {
            index_t splitColIndex = trees[tree].splitColIndex[k];
            size_t col;
            
            if (splitColIndex == NO_INDEX) {
                col = targetColumn;
                
            } else {
                col = selectColumnIndexes[(size_t)splitColIndex];
            }
            
            ValueType valueType = valueTypes.at(col);
            
            switch (valueType) {
                case kCategorical:
                    ofs << trees[tree].value[k].i << endl;
                    break;
                    
                case kNumeric:
                    ofs << scientific << setprecision(17) << trees[tree].value[k].d << endl;
                    break;
            }
        }
        
        ofs << endl;
        
    }
    
    // ~~~~~~~~ colNames ~~~~~~~~
    
    ofs << "colNames" << endl;
    
    for (size_t col = 0; col < colNames.size(); col++) {
        ofs << '"' << colNames[col] << '"' << endl;
    }
    
    // ~~~~~~~~ ~~~~~~~~ ~~~~~~~~
    
    ofs.close();
}

void readModel(const string& modelFile,
               vector<ValueType>& valueTypes,
               vector<CategoryMaps>& categoryMaps,
               size_t& targetColumn,
               SelectIndexes& selectColumns,
               vector<ImputeOption>& imputeOptions,
               vector<CompactTree>& trees,
               vector<string>& colNames)
{
    valueTypes.clear();
    categoryMaps.clear();
    imputeOptions.clear();
    trees.clear();
    colNames.clear();
    
    ifstream ifs(modelFile.c_str());
    
    RUNTIME_ERROR_IF(!ifs.good(), badPathErrorMessage(modelFile));
    
    vector< vector<string> > cells;
    vector< vector<bool> > quoted;
    vector<string> cellColNames;
    
    // ~~~~~~~~ valueTypes ~~~~~~~~
    
    readCsv(ifs, true, cells, quoted, cellColNames);
    
    for (size_t row = 0; row < cells.size(); row++) {
        valueTypes.push_back(stringToValueType(cells[row][0]));
    }
    
    // ~~~~~~~~ categoryMaps ~~~~~~~~
    
    readCsv(ifs, true, cells, quoted, cellColNames);
    
    categoryMaps.assign(cells.size(), CategoryMaps());
    
    for (size_t row = 0; row < cells.size(); row++) {
        categoryMaps[row].setUseNaCategory((bool)toLong(cells[row][0]));
    }
    
    for (size_t category = 0; category < categoryMaps.size(); category++) {
        readCsv(ifs, true, cells, quoted, cellColNames);

        for (size_t row = 0; row < cells.size(); row++) {
            categoryMaps[category].insertCategory(cells[row][0]);
        }
    }
    
    // ~~~~~~~~ targetColumn ~~~~~~~~

    readCsv(ifs, true, cells, quoted, cellColNames);
    
    targetColumn = (size_t)toLong(cells.at(0).at(0));
    
    // ~~~~~~~~ selectColumns ~~~~~~~~
    
    readCsv(ifs, true, cells, quoted, cellColNames);
    
    size_t numCols = valueTypes.size();
    selectColumns.clear(numCols);
    
    for (size_t row = 0; row < cells.size(); row++) {
        selectColumns.select((size_t)toLong(cells[row][0]));
    }
    
    // ~~~~~~~~ imputeOptions ~~~~~~~~
    
    readCsv(ifs, true, cells, quoted, cellColNames);
    
    for (size_t row = 0; row < cells.size(); row++) {
        imputeOptions.push_back(stringToImputeOption(cells[row][0], valueTypes.at(row)));
    }
    
    // ~~~~~~~~ trees ~~~~~~~~
    
    readCsv(ifs, true, cells, quoted, cellColNames);
    
    size_t numTrees = (size_t)toLong(cells.at(0).at(0));
    
    trees.assign(numTrees, CompactTree());
    
    for (size_t tree = 0; tree < numTrees; tree++) {
        readCsv(ifs, true, cells, quoted, cellColNames);
        
        for (size_t row = 0; row < cells.size(); row++) {
            trees[tree].splitColIndex.push_back((index_t)toLong(cells[row][0]));  
        }
        
        readCsv(ifs, true, cells, quoted, cellColNames);
        
        for (size_t row = 0; row < cells.size(); row++) {
            trees[tree].lessOrEqualIndex.push_back((index_t)toLong(cells[row][0]));  
        }
        
        readCsv(ifs, true, cells, quoted, cellColNames);
        
        for (size_t row = 0; row < cells.size(); row++) {
            trees[tree].greaterOrNotIndex.push_back((index_t)toLong(cells[row][0]));  
        }
        
        readCsv(ifs, true, cells, quoted, cellColNames);
        
        for (size_t row = 0; row < cells.size(); row++) {
            trees[tree].toLessOrEqualIfNA.push_back((bool)toLong(cells[row][0]));  
        }

        readCsv(ifs, true, cells, quoted, cellColNames);

        const vector<size_t>& selectColumnIndexes = selectColumns.indexVector();
        
        for (size_t row = 0; row < cells.size(); row++) {
            index_t splitColIndex = trees[tree].splitColIndex.at(row);
            size_t col;
            
            if (splitColIndex == NO_INDEX) {
                col = targetColumn;
                
            } else {
                col = selectColumnIndexes[(size_t)splitColIndex];
            }
            
            ValueType valueType = valueTypes.at(col);
            
            Number number;
            
            switch (valueType) {
                case kCategorical:
                    number.i = (index_t)toLong(cells[row][0]);
                    break;
                    
                case kNumeric:
                    number.d = toDouble(cells[row][0]);
                    break;
            }
            
            trees[tree].value.push_back(number);  
        }
    }
    
    // ~~~~~~~~ colNames ~~~~~~~~
    
    readCsv(ifs, true, cells, quoted, cellColNames);
    
    for (size_t row = 0; row < cells.size(); row++) {
        colNames.push_back(cells[row][0]);
    }
    
    // ~~~~~~~~ ~~~~~~~~ ~~~~~~~~
    
    ifs.close();
}
