//
//  format.cpp
//  entree
//
//  Created by MPB on 5/7/13.
//  Copyright (c) 2015 Quadrivio Corporation. All rights reserved.
//  License http://opensource.org/licenses/BSD-2-Clause
//          <YEAR> = 2015
//          <OWNER> = Quadrivio Corporation
//

//
//  Utilities for handling values and categories
//

#include <iostream>
#include <iomanip>
#include <sstream>

#include "csv.h"
#include "format.h"
#include "shim.h"
#include "utils.h"

using namespace std;

// ========== Globals ==============================================================================

const Value gNaValue = { { 0.0 }, true };

// ========== Classes ==============================================================================

// handles mapping between category indexes and category names; allows for treating NA as a
// separate category

const string CategoryMaps::naCategory = " <NA> ";

CategoryMaps::CategoryMaps() :
useNaCategory(false)
{
}

CategoryMaps::~CategoryMaps()
{
}

// insert category if missing; return index
index_t CategoryMaps::findOrInsertCategory(const std::string& category)
{
    index_t index;
    
    if (!findIndexForCategory(category, index)) {
        categories.push_back(category);
        index = (index_t)categoryToIndex.size();
        categoryToIndex.insert(make_pair(category, index));
    }
    
    return index;
}

// insert category; error if already present
index_t CategoryMaps::insertCategory(const std::string& category)
{
    index_t index;
    
    if (findIndexForCategory(category, index)) {
        RUNTIME_ERROR_IF(true, "insertCategory: duplicate category name");
        
    } else {
        categories.push_back(category);
        index = (index_t)categoryToIndex.size();
        categoryToIndex.insert(make_pair(category, index));
    }
    
    return index;
}

// look for category; if found, write index into param and return true, else write NO_INDEX into
// param and return false
bool CategoryMaps::findIndexForCategory(const std::string& category, index_t& index) const
{
    bool found = false;
    index = NO_INDEX;
    
    map<string, index_t>::const_iterator iter = categoryToIndex.find(category);
    if (iter != categoryToIndex.end()) {
        found = true;
        index = iter->second;
    }
    
    return found;
}

// look for index; if found, write category into param and return true, else write " <NA> " into
// param and return false
bool CategoryMaps::findCategoryForIndex(index_t index, std::string& category) const
{
    bool found = false;
    category = naCategory;
    
    if (index == NO_INDEX && useNaCategory) {
        found = true;
        
    } else if (index >= 0 && index < categories.size()) {
        category = categories[(size_t)index];
        found = true;
    }
    
    return found;
}

// look for category, return index; error if not found
index_t CategoryMaps::getIndexForCategory(const std::string& category) const
{
    index_t index;
    
    if (!findIndexForCategory(category, index)) {
        LOGIC_ERROR_IF(true, "getIndexForCategory: category not found")    
    }
    
    return index;
}

// look for index, return category; error if not found
std::string CategoryMaps::getCategoryForIndex(index_t index) const
{
    string category;
    
    if (!findCategoryForIndex(index, category)) {
        LOGIC_ERROR_IF(true, "getCategoryForIndex: index not found")    
    }
    
    return category;
}

// return lowest index number; for enumeration
index_t CategoryMaps::beginIndex() const
{
    return useNaCategory ? NO_INDEX : 0;
}

// return higest index number + 1; for enumeration
index_t CategoryMaps::endIndex() const
{
    return (index_t)categories.size();
}

// return count of all categories, excluding NA category
size_t CategoryMaps::countNamedCategories() const
{
    return categories.size();
}

// return count of all categories, including NA category if used
size_t CategoryMaps::countAllCategories() const
{
    return useNaCategory ? categories.size() + 1 : categories.size();
}

// clear all named categories
void CategoryMaps::clear()
{
    categories.clear();
    categoryToIndex.clear();
}

// for debugging; print info
void CategoryMaps::dump() const
{
    CERR << "~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~" << endl;
    CERR << "useNaCategory = " << (useNaCategory ? "T" : "F") << endl;    
    CERR << "categories.size() = " << categories.size() << endl;    
    CERR << "categoryToIndex.size() = " << categoryToIndex.size() << endl;    
    
    for (index_t index = beginIndex(); index < endIndex(); index++) {
        CERR << index << "\t" << getCategoryForIndex(index) << endl;
    }
    
    CERR << endl;
    
    map<string, index_t>::const_iterator iter2 = categoryToIndex.begin();
    while (iter2 != categoryToIndex.end()) {
        CERR << iter2->first << "\t" << iter2->second << endl;
        iter2++;
    }
    
    CERR << endl;
    CERR << "~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~" << endl;
}

// -------------------------------------------------------------------------------------------------

// handles iterating through lists of selected indexes, either by testing an index to see if it is
// selected or by providing the index numbers of the selected indexes

// construct with empty lists
SelectIndexes::SelectIndexes() :
bitMap(vector<bool>(0)),
indexes(vector<size_t>(0)),
selected(0)
{
}

// construct with all indexes (from 0 to size - 1) initially selected or unselected
SelectIndexes::SelectIndexes(size_t size, bool selectAll) :
bitMap(vector<bool>(size, selectAll)),
indexes(vector<size_t>(selectAll ? size : 0)),
selected(selectAll ? size : 0)
{
    if (selectAll) {
        for (size_t k = 0; k < size; k++) {
            indexes[k] = k;    
        }
    }
}

// copy constructor
SelectIndexes::SelectIndexes(const SelectIndexes& other) :
bitMap(other.bitMap),
indexes(other.indexes),
selected(other.selected)
{
}

SelectIndexes::~SelectIndexes()
{
}

// change sizes of lists and unselect all indexes (from 0 to size - 1)
void SelectIndexes::clear(size_t size)
{
    bitMap.assign(size, false);
    indexes.clear();
    selected = 0;
}

// change sizes of lists and select all indexes (from 0 to size - 1)
void SelectIndexes::selectAll(size_t size)
{
    bitMap.assign(size, true);
    indexes.resize(size);
    
    for (size_t k = 0; k < size; k++) {
        indexes[k] = k;    
    }
    
    selected = size;
}

// select specified index; must be in existing size range 
void SelectIndexes::select(size_t index)
{
    LOGIC_ERROR_IF(index >= bitMap.size(), "SelectIndexes::select out of range")
    
    if (!bitMap[index]) {
        bitMap[index] = true;
        indexes.push_back(index);
        selected++;
    }
}

// unselect specified index; must be in existing size range 
void SelectIndexes::unselect(size_t index)
{
    LOGIC_ERROR_IF(index >= bitMap.size(), "SelectIndexes::unselect out of range")
    
    if (bitMap[index]) {
        bitMap[index] = false;
        
        bool found = false;
        vector<size_t>::iterator iter = indexes.begin();
        
        while (!found && iter != indexes.end()) {
            if (index == *iter) {
                indexes.erase(iter);
                found = true;
                
            } else {
                iter++;
            }
        }
        
        selected--;
    }
}

// return bool vector with item for each possible index (0 to size - 1), valued true if index is
// selected and false if not selected
const vector<bool>& SelectIndexes::boolVector() const
{
    return bitMap;    
}

// return vector of indexes that are currently selected
const vector<size_t>& SelectIndexes::indexVector() const
{
    return indexes;    
}

// for debugging; print state
void SelectIndexes::dump() const
{
    for (size_t k = 0; k < bitMap.size(); k++) {
        if (bitMap[k]) {
            CERR << "1 ";
            
        } else {
            CERR << "0 ";
        }
    }
    
    CERR << endl;
    
    for (size_t k = 0; k < indexes.size(); k++) {
        CERR << indexes[k] << endl;
    }
}

// -------------------------------------------------------------------------------------------------

// handles sorting a vector of indexes pointing to entries in a vector of Value structs; sorts
// indexes in ascending order of Value; NA sorts to beginning; index order is preserved for equal
// values

// constructor; prepare to sort items in valueVector; type is specified to be valueType
SortValueVector::SortValueVector(const vector<Value>& valueVector, ValueType valueType) :
valueVector(valueVector),
valueType(valueType)
{
}

SortValueVector::~SortValueVector()
{
}

// comparison operator for sort
bool SortValueVector::operator ()(size_t i, size_t j)
{
    bool result;
    
    switch (valueType) {
        case kNumeric:
            if (valueVector[i].na && valueVector[j].na) {
                result = i < j;
                
            } else if (valueVector[i].na && !valueVector[j].na) {
                result = true;
                
            } else if (!valueVector[i].na && valueVector[j].na) {
                result = false;
                
            } else if (valueVector[i].number.d < valueVector[j].number.d) {
                result = true;
                
            } else if (valueVector[i].number.d > valueVector[j].number.d) {
                result = false;
                
            } else {
                result = i < j;
            }
            break;
            
        case kCategorical: 
            if (valueVector[i].na && valueVector[j].na) {
                result = i < j;
                
            } else if (valueVector[i].na && !valueVector[j].na) {
                result = true;
                
            } else if (!valueVector[i].na && valueVector[j].na) {
                result = false;
                
            } else if (valueVector[i].number.i < valueVector[j].number.i) {
                result = true;
                
            } else if (valueVector[i].number.i > valueVector[j].number.i) {
                result = false;
                
            } else {
                result = i < j;
            }
            break;
    }
    
    return result;
}

// sort all items in valueVector; indexVector will be set to result
void SortValueVector::sort(vector<size_t>& indexVector)
{
    indexVector.resize(valueVector.size());
    for (size_t k = 0; k < indexVector.size(); k++) {
        indexVector[k] = k;
    }
    
    
    std::sort(indexVector.begin(), indexVector.end(), *this);
}

// sort selected items in valueVector; indexVector will be set to result
void SortValueVector::sort(vector<size_t>& indexVector, const SelectIndexes& selectIndexes)
{
    indexVector = selectIndexes.indexVector();
    for (size_t k = 0; k < indexVector.size(); k++) {
        indexVector[k] = k;
    }
    
    
    std::sort(indexVector.begin(), indexVector.end(), *this);
}

// ========== Functions ============================================================================

// for debugging or logging; print vector of Value
void printValuesColumn(const std::vector<Value>& valuesColumn,
                       ValueType valueType,
                       const CategoryMaps& categoryMaps,
                       index_t maxRows)
{
    switch (valueType) {
        case kCategorical:
            if (valuesColumn.size() == 1) {
                CERR << "1 kCategorical value" << endl;
                
            } else {
                CERR << valuesColumn.size() << " kCategorical values" << endl;
            }
            break;
            
        case kNumeric:
            if (valuesColumn.size() == 1) {
                CERR << "1 kNumeric value" << endl;
                
            } else {
                CERR << valuesColumn.size() << " kNumeric values" << endl;
            }
            break;
    }
    
    size_t numRows = valuesColumn.size();
    if (maxRows > 0 && numRows > maxRows) {
        numRows = (size_t)maxRows;
    }
    
    for (size_t k = 0; k < numRows; k++) {
        Value value = valuesColumn.at(k);
        
        if (value.na) {
            CERR << k << "]\t<NA>" << endl;
            
        } else {
            switch (valueType) {
                case kCategorical:
                {
                    index_t index = value.number.i;
                    string category;
                    
                    if (categoryMaps.findCategoryForIndex(index, category)) {
                        CERR << k << "]\t" << category << endl;
                        
                    } else {
                        CERR << k << "]\t<Missing Category>" << endl;
                    }
                }
                    break;
                    
                case kNumeric:
                {
                    CERR << k << "]\t" << fixed << setprecision(8) << value.number.d << endl;
                }
                    break;
            }
        }
    }
    
    CERR << endl;
}

// for debugging or logging; print array of Values
void printValues(const std::vector< std::vector<Value> >& values,
                 const std::vector<ValueType>& valueTypes,
                 const std::vector<CategoryMaps>& categoryMaps,
                 const std::vector<std::string>& colNames)
{
    size_t numCols = values.size();
    size_t numRows = values[0].size();
    
    if (colNames.size() > 0) {
        CERR << colNames[0];
        for (size_t col = 1; col < colNames.size(); col++) {
            CERR << '\t' << colNames.at(col);
        }
        
        CERR << endl;
    }
    
    for (size_t row = 0; row < numRows; row++) {
        for (size_t col = 0; col < numCols; col++) {
            if (col > 0) {
                CERR << '\t';
            }
            
            Value value = values.at(col).at(row);
            
            if (value.na) {
                CERR << "<NA>";
                
            } else {
                switch (valueTypes.at(col)) {
                    case kCategorical:
                    {
                        index_t index = value.number.i;
                        string category;
                        
                        if (categoryMaps.at(col).findCategoryForIndex(index, category)) {
                            CERR << category;
                            
                        } else {
                            CERR << "<Missing Category>";
                        }
                    }
                        break;
                        
                    case kNumeric:
                    {
                        CERR << fixed << setprecision(8) << value.number.d;
                    }
                        break;
                }
            }
        }
        
        CERR << endl;
    }
}

// -------------------------------------------------------------------------------------------------

// calculate mean of selected rows in a vector of numerical Values
Value meanValue(const std::vector<Value>& valuesColumn, const SelectIndexes& selectRows)
{
    Value value;
    
    double sum = 0.0;
    index_t count = 0;
    
    const vector<size_t>& rowIndexes = selectRows.indexVector();
    for (size_t rowIndex = 0; rowIndex < rowIndexes.size(); rowIndex++) {
        size_t row = rowIndexes[rowIndex];
        LOGIC_ERROR_IF(row >= valuesColumn.size(), "out of range");
        
        if (!valuesColumn[row].na) {
            sum += valuesColumn[row].number.d;
            count++;
        }
    }
    
    if (count > 0) {
        value.na = false;
        value.number.d = sum / count;
        
    } else {
        // if empty vector, return NA as mean
        value = gNaValue;
    }
    
    return value;
}

// calculate median of selected rows in a vector of numerical Values; must supply vector of sorted
// indexes that at least includes all of the selected rows
Value medianValue(const std::vector<Value>& valuesColumn,
                  const SelectIndexes& selectRows,
                  const std::vector<size_t>& sortedIndexes)
{
    vector<size_t> valueRows;
    valueRows.reserve(selectRows.countSelected());
    
    const vector<bool>& rowSelected = selectRows.boolVector();
    for (size_t index = 0; index < sortedIndexes.size(); index++) {
        size_t row = sortedIndexes[index];
        LOGIC_ERROR_IF(row >= valuesColumn.size() || row >= rowSelected.size(), "out of range");

        // ignore NA values
        if (rowSelected[row] && !valuesColumn[row].na) {
            valueRows.push_back(row);
        }
    }
    
    Value value;
    if (valueRows.size() == 0) {
        // if empty vector, return NA as mean
        value = gNaValue;
        
    } else {
        // values were added in sorted order, so pick middle value
        size_t index = valueRows.size() / 2;
        size_t row = valueRows[index];
        value = valuesColumn.at(row);
    }
    
    return value;
}

// select modal value of selected rows in a vector of categorical Values; in case of tie, choose
// category with name that sorts earlier alphabetically; NA category (if used) has name " <NA> "
Value modeValue(const std::vector<Value>& valuesColumn,
                const SelectIndexes& selectRows,
                const CategoryMaps& categoryMaps)
{
    Value value = gNaValue; // if no values or no categories, return NA
    
    size_t categoryCount = categoryMaps.countAllCategories();
    
    index_t beginCategoryIndex = categoryMaps.beginIndex();
    index_t endCategoryIndex = categoryMaps.endIndex();
    
    if (valuesColumn.size() > 0 && categoryCount > 0) {
        
        vector<size_t> counts(categoryCount, 0);
        
        const vector<size_t>& rowIndexes = selectRows.indexVector();
        for (size_t rowIndex = 0; rowIndex < rowIndexes.size(); rowIndex++) {
            size_t row = rowIndexes[rowIndex];
            LOGIC_ERROR_IF(row >= valuesColumn.size(), "out of range");

            if (!valuesColumn[row].na) {
                // for each selected row that is NA, get category index and tally
                index_t categoryIndex = valuesColumn[row].number.i;
                LOGIC_ERROR_IF(categoryIndex < 0, "out of range");
                
                if (categoryIndex < categoryCount) {
                    // countsIndex differs from categoryIndex if using NA category
                    size_t countsIndex = (size_t)(categoryIndex - beginCategoryIndex);
                    counts[countsIndex]++;
                }
            }
        }
        
        size_t selectedCount;
        string selectedName;
        
        for (index_t categoryIndex = beginCategoryIndex;
             categoryIndex < endCategoryIndex;
             categoryIndex++) {
            
            // iterate over category indexes, see if each is the biggest so far
            string nextName = categoryMaps.getCategoryForIndex(categoryIndex);
            size_t countsIndex = (size_t)(categoryIndex - beginCategoryIndex);
            size_t nextCount = counts[countsIndex];
            
            bool pickThis = false;
            
            if (nextCount == 0) {
                SKIP
                
            } else if (value.na) {
                // first (non-empty) category
                pickThis = true;
                
            } else if (nextCount > selectedCount) {
                // bigger than previous so far
                pickThis = true;

            } else if (nextCount == selectedCount) {
                // same size; use name as tie breaker, to enforce deterministic sort order
                if (nextName < selectedName) {
                    pickThis = true;
                }
            }
            
            if (pickThis) {
                value.na = false;
                value.number.i = categoryIndex;
                selectedCount = nextCount;
                selectedName = nextName;
            }
        }
    }
    
    return value;
}

// -------------------------------------------------------------------------------------------------

// return Value to be used as replacement for NA for specified column and selection of rows 
Value imputedValue(size_t col,
                   const std::vector<ImputeOption>& convertTypes,
                   const std::vector< std::vector<Value> >& values,
                   const std::vector<ValueType>& valueTypes,
                   const SelectIndexes& selectRows,
                   const std::vector<CategoryMaps>& categoryMaps,
                   const std::vector< std::vector<size_t> >& sortedIndexes)
{
    size_t numCols = values.size();

    Value value = gNaValue;

    LOGIC_ERROR_IF(col >= numCols, "out of range");
    
    switch (valueTypes[col]) {
        case kCategorical:
        {
            switch (convertTypes[col]) {
                case kNoImpute:
                    break;
                    
                case kToCategory:
                {
                    value.na = false;
                    value.number.i = NO_INDEX;
                }
                    break;
                    
                case kToMode:
                {
                    value = modeValue(values[col], selectRows, categoryMaps[col]);
                }
                    break;
                    
                case kToMean:
                case kToMedian:
                    RUNTIME_ERROR_IF(true, "invalid NA conversion for categorical type")
                    break;
                    
                case kToDefault:
                    LOGIC_ERROR_IF(true, "unconverted kToDefault")
                    break;
            }
        }
            break;
            
        case kNumeric:
        {
            switch (convertTypes[col]) {
                case kNoImpute:
                    break;
                    
                case kToCategory:
                case kToMode:
                    RUNTIME_ERROR_IF(true, "invalid NA conversion for numerical type")
                    break;
                    
                case kToMean:
                {
                    value = meanValue(values[col], selectRows);
                }
                    break;
                    
                case kToMedian:
                {
                    value = medianValue(values[col], selectRows, sortedIndexes[col]);
                }
                    break;
                    
                case kToDefault:
                    LOGIC_ERROR_IF(true, "unconverted kToDefault")
                    break;                    
            }
        }
            break;
    }

    return value;
}

// replace NA values in selected columns and rows in array of Values
void imputeValues(const std::vector<ImputeOption>& convertTypes,
                  const std::vector<ValueType>& valueTypes,
                  std::vector< std::vector<Value> >& values,
                  const SelectIndexes& selectRows,
                  const SelectIndexes& selectColumns,
                  std::vector<CategoryMaps>& categoryMaps,
                  std::vector< std::vector<size_t> >& sortedIndexes,
                  std::vector<Value>& imputedValues)
{
    size_t numSelectCols = selectColumns.countSelected();
    size_t numCols = values.size();
    size_t numRows = values[0].size();
    
    LOGIC_ERROR_IF(numCols != valueTypes.size(), "size mismatch valueTypes vs. values");
    LOGIC_ERROR_IF(numCols != convertTypes.size(), "size mismatch convertTypes vs. values");
    LOGIC_ERROR_IF(numCols != sortedIndexes.size(), "size mismatch sortedIndexes vs. values");
    LOGIC_ERROR_IF(numCols != categoryMaps.size(), "size mismatch categoryMaps vs. values");
    
    const vector<size_t>& selectColumnIndexes = selectColumns.indexVector();
    const vector<size_t>& selectRowIndexes = selectRows.indexVector();
    
    imputedValues.assign(numCols, gNaValue);
    
    for (size_t columnIndex = 0; columnIndex < numSelectCols; columnIndex++) {
        size_t col = selectColumnIndexes.at(columnIndex);
        
        LOGIC_ERROR_IF(col >= numCols, "out of range");
        LOGIC_ERROR_IF(numRows != values.at(col).size(), "size mismatch within values");

        if (convertTypes[col] == kToCategory) {
            categoryMaps[col].setUseNaCategory(true);    
        }
        
        LOGIC_ERROR_IF(convertTypes[col] == kToDefault, "unconverted kToDefault");
        
        if (convertTypes[col] != kNoImpute) {
            // get imputed value to be used for entire column
            imputedValues[col] = imputedValue(col, convertTypes, values, valueTypes, selectRows,
                                              categoryMaps, sortedIndexes);
            
            bool changedCol = false;
            
            for (size_t rowIndex = 0; rowIndex < selectRowIndexes.size(); rowIndex++) {
                size_t row = selectRowIndexes[rowIndex];
                
                if (values[col][row].na) {
                    values[col][row] = imputedValues[col];
                    changedCol = true;
                }
            }
            
            // re-sort if any values in column were changed
            if (changedCol) {
                SortValueVector sortValueVector(values[col], valueTypes[col]);
                sortValueVector.sort(sortedIndexes.at(col), selectRows);
            }
        }
    }
}

// -------------------------------------------------------------------------------------------------

// convert array of Values (as vector of columns) to array of strings (as vector of rows);
// write numbers in %.8f format;
//  writeNA: if true, write unquoted NA for na values, else just use blank
//  cells: value in text form, excluding any surrounding quote marks
//  quoted: if true, value should be surrounded by quote marks on output to file or log
void valuesToCells(const std::vector< std::vector<Value> >& values,
                   const std::vector<ValueType> valueTypes,
                   const std::vector<CategoryMaps>& categoryMaps,
                   bool writeNA,
                   const std::string& naString,
                   std::vector< std::vector<std::string> >& cells,
                   std::vector< std::vector<bool> >& quoted)
{
    size_t numRows = values[0].size();
    size_t numCols = values.size();
    
    cells.assign(numRows, vector<string>(numCols, ""));
    quoted.assign(numRows, vector<bool>(numCols, false));
    
    for (size_t row = 0; row < numRows; row++) {
        for (size_t col = 0; col < numCols; col++) {
            Value value = values[col][row];
            
            if (value.na) {
                if (writeNA) {
                    cells[row][col] = naString;
                }
                
            } else {
                switch(valueTypes[col]) {
                    case kCategorical:
                    {
                        string category;
                        if (categoryMaps[col].findCategoryForIndex(value.number.i, category)) {
                            // found
                            cells[row][col] = category;

                            // require category names to be quoted on output
                            quoted[row][col] = true;
                            
                        } else {
                            // not found
                            if (writeNA) {
                                cells[row][col] = naString;
                            }
                        }
                    }
                        break;
                        
                    case kNumeric:
                    {
                        ostringstream oss;
                        oss << fixed << setprecision(8) << value.number.d;
                        cells[row][col] = oss.str();
                    }
                        break;
                }
            }
        }
    }
}

// convert array of strings (as vector of rows) to array of Values (as vector of columns);
// unquoted empty cell is treated as NA; quoted empty string is treated as string of length zero;
//  interpretNA: also interpret unquoted NA as missing value
//  constCategories: if true, treat any unrecognized category as NA; if false, update
//      categoryMaps to include any new categories found
void cellsToValues(const std::vector< std::vector<std::string> >& cells,
                   const std::vector< std::vector<bool> >& quoted,
                   const std::vector<ValueType> valueTypes,
                   bool interpretNA,
                   const std::string& naString,
                   std::vector< std::vector<Value> >& values,
                   bool constCategories,
                   std::vector<CategoryMaps>& categoryMaps)
{
    size_t numRows = cells.size();
    size_t numCols = cells[0].size();

    LOGIC_ERROR_IF(numCols != valueTypes.size(), "mismatch valueTypes vs. cells");

    if (constCategories) {
        LOGIC_ERROR_IF(numCols != categoryMaps.size(), "mismatch categoryMaps vs. cells");
    }
    
    values.clear();
    
    for (size_t col = 0; col < numCols; col++) {        
        values.push_back(vector<Value>());
        
        if (categoryMaps.size() <= col) {
            categoryMaps.push_back(CategoryMaps());
        }
        
        vector<Value>& columnValues = values.at(col);
        CategoryMaps& categoryMap = categoryMaps.at(col);
        
        switch (valueTypes.at(col)) {
            case kNumeric:
            {
                Value value;
                
                for (size_t row = 0; row < numRows; row++) {
                    const string& cell = cells[row].at(col);
                    bool isQuoted = quoted[row].at(col);
                    
                    if (interpretNA && cell == naString && !isQuoted) {
                        value = gNaValue;
                    
                    } else if (cell.length() == 0 && !isQuoted) {
                        value = gNaValue;
                        
                    } else {
                        istringstream iss(cell);
                        iss >> value.number.d;
                        value.na = iss.fail();
                    }
                    
                    columnValues.push_back(value);
                }
            }
                break;
                
            case kCategorical:
                for (size_t row = 0; row < numRows; row++) {
                    const string& cell = cells[row].at(col);
                    bool isQuoted = quoted[row].at(col);

                    Value value;
                    
                    if (interpretNA && cell == naString && !isQuoted) {
                        value = gNaValue;
                        
                    } else if (cell.length() == 0 && !isQuoted) {
                        value = gNaValue;
                        
                    } else {
                        if (categoryMap.findIndexForCategory(cell, value.number.i)) {
                            // found
                            value.na = false;
                            
                        } else {
                            // not found
                            if (constCategories) {
                                // treat as NA
                                value.na = true;
                                
                            } else {
                                // add new category
                                value.na = false;
                                value.number.i = categoryMap.insertCategory(cell);
                            }
                        }
                    }
                    
                    columnValues.push_back(value);
                }
                break;
                
            default:
                LOGIC_ERROR_IF(true, "unknown valueType")
                break;
        }
    }
}

// -------------------------------------------------------------------------------------------------

// create vector of sorted indexes for each selected column in array of Values
void makeSortedIndexes(const std::vector< std::vector<Value> >& values,
                       const std::vector<ValueType> valueTypes,
                       const SelectIndexes& selectColumns,
                       std::vector< std::vector<size_t> >& sortedIndexes)
{
    sortedIndexes.clear();
    
    size_t numCols = values.size();
    
    const vector<bool>& columnIsSelected = selectColumns.boolVector();
    
    size_t numRows = values[0].size();
    
    for (size_t col = 0; col < numCols; col++) {
        if (columnIsSelected.at(col)) {
            sortedIndexes.push_back(vector<size_t>(numRows));
            
            SortValueVector sortValueVector(values.at(col), valueTypes.at(col));
            sortValueVector.sort(sortedIndexes.at(col));
            
        } else {
            sortedIndexes.push_back(vector<size_t>(0));
        }
    }
}

// -------------------------------------------------------------------------------------------------

// get default value types (assume numeric if can be parsed as numeric and no remaining characters)
void getDefaultValueTypes(const std::vector< std::vector<std::string> >& cells,
                          const std::vector< std::vector<bool> >& quoted,
                          bool interpretNA,
                          const std::string& naString,
                          std::vector<ValueType>& valueTypes)
{
    size_t numCols = cells.at(0).size();
    size_t numRows = cells.size();
    
    valueTypes.clear();
    
    for (size_t col = 0; col < numCols; col++) {
        ValueType valueType = kNumeric;
        for (size_t row = 0; row < numRows && valueType == kNumeric; row++) {
            string str = cells.at(row).at(col);
            if (str.length() > 0) {
                if (interpretNA && str == naString && !quoted.at(row).at(col)) {
                    SKIP
                    
                } else {
                    if (!isNumeric(str)) {
                        valueType = kCategorical;
                    }
                }
            }
        }
        
        valueTypes.push_back(valueType);
    }
}

// get ValueType for name
ValueType stringToValueType(string str)
{
    ValueType valueType = kCategorical;
    
    transform(str.begin(), str.end(), str.begin(), ::tolower);
    
    if (str.length() == 0) {
        RUNTIME_ERROR_IF(true, "invalid valueType")
        
    } else if (str.find("c") == 0) {
        valueType = kCategorical;
        
    } else if (str.find("n") == 0) {
        valueType = kNumeric;
        
    } else {
        RUNTIME_ERROR_IF(true, "invalid valueType")
    }
    
    return valueType;
}

// get name of ValueType
void valueTypeToString(ValueType valueType, string& str)
{
    switch(valueType) {
        case kCategorical:  str = "categorical";    break;
        case kNumeric:      str = "numeric";        break;
    }
}

// get name of ImputeOption
void imputeOptionToString(ImputeOption imputeOption, string& str)
{
    switch(imputeOption) {
        case kNoImpute:                 str = "none";       break;
        case kToDefault:                str = "default";    break;
        case kToCategory:               str = "category";   break;
        case kToMode:                   str = "mode";       break;
        case kToMean:                   str = "mean";       break;
        case kToMedian:                 str = "median";     break;
    }
}

// get default ImputeOption
ImputeOption getDefaultImputeOption(ValueType valueType)
{
    ImputeOption imputeOption;
    
    switch (valueType) {
        case kCategorical:  imputeOption = kToCategory; break;
        case kNumeric:      imputeOption = kToMedian;   break;
    }
    
    return imputeOption;
}

// get ImputeOption for name
ImputeOption stringToImputeOption(string str, ValueType valueType)
{
    ImputeOption imputeOption = kToDefault;
    
    transform(str.begin(), str.end(), str.begin(), ::tolower);
    
    if (str.length() == 0) {
        RUNTIME_ERROR_IF(true, "invalid imputeOption")
        
    } else {
        switch (valueType) {
            case kCategorical:
                if (str.find("c") == 0) {
                    imputeOption = kToCategory;
                    
                } else if (str.find("mo") == 0) {
                    imputeOption = kToMode;
                    
                } else if (str.find("d") == 0) {
                    imputeOption = kToDefault;
                    
                } else if (str.find("no") == 0) {
                    imputeOption = kNoImpute;
                    
                } else {
                    RUNTIME_ERROR_IF(true, "invalid imputeOption")
                }    
                break;
                
            case kNumeric:
                if (str.find("mea") == 0) {
                    imputeOption = kToMean;
                    
                } else if (str.find("med") == 0) {
                    imputeOption = kToMedian;
                    
                } else if (str.find("d") == 0) {
                    imputeOption = kToDefault;
                    
                } else if (str.find("no") == 0) {
                    imputeOption = kNoImpute;
                    
                } else {
                    RUNTIME_ERROR_IF(true, "invalid imputeOption")
                }    
                break;
                
        }
    }
    
    return imputeOption;
}

// ========== Tests ================================================================================

// component tests
void ctest_format(int& totalPassed, int& totalFailed, bool verbose)
{
    int passed = 0;
    int failed = 0;
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // CategoryMaps
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // SelectIndexes
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // SortValueVector
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // meanValue
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // medianValue
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // modeValue
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // printValuesColumn
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // printValues
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // imputedValue
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // imputeValues
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // cellsToValues
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // valuesToCells
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // makeSortedIndexes
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // getDefaultValueTypes
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // stringToValueType
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // valueTypeToString   
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // getDefaultImputeOption

    // ~~~~~~~~~~~~~~~~~~~~~~
    // stringToImputeOption
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // imputeOptionToString
    
    // ~~~~~~~~~~~~~~~~~~~~~~

    if (verbose) {
        CERR << "format.cpp" << "\t" << passed << " passed, " << failed << " failed" << endl;
    }
    
    totalPassed += passed;
    totalFailed += failed;
}

// fill vectors with test values
void fillValues(vector< vector<Value> >& values, vector<ValueType>& valueTypes);
void fillValues(vector< vector<Value> >& values, vector<ValueType>& valueTypes)
{
    values.clear();
    valueTypes.clear();
    
    Value v1 = { { 1.0 }, false };
    Value v2 = { { 2.0 }, false };
    Value v3 = { { 3.0 }, false };
    vector<Value> c1;
    c1.push_back(gNaValue);
    c1.push_back(v1);
    c1.push_back(v3);
    c1.push_back(v2);
    c1.push_back(gNaValue);
    c1.push_back(v2);
    values.push_back(c1);
    valueTypes.push_back(kNumeric);
    
    Value v4 = { { 0 }, false };
    Value v5 = { { 0 }, false };
    Value v6 = { { 0 }, false };
    v4.number.i = 0;
    v5.number.i = 1;
    v6.number.i = 2;
    vector<Value> c2;
    c2.push_back(gNaValue);
    c2.push_back(v4);
    c2.push_back(v6);
    c2.push_back(v5);
    c2.push_back(gNaValue);
    c2.push_back(v5);
    values.push_back(c2);
    valueTypes.push_back(kCategorical);
    
    vector<Value> c3;
    c3.push_back(v4);
    c3.push_back(v5);
    c3.push_back(v6);
    c3.push_back(v6);
    c3.push_back(v6);
    c3.push_back(gNaValue);
    values.push_back(c3);
    valueTypes.push_back(kCategorical);
}

// code coverage
void cover_format(bool verbose)
{
    vector< std::vector<std::string> > cells;
    vector< std::vector<bool> > quoted;
    vector< vector<Value> > values;
    vector<ValueType> valueTypes;
    vector<CategoryMaps> categoryMaps;
    vector< vector<size_t> > sortedIndexes;
    SelectIndexes selectRows;
    SelectIndexes selectCols;
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // CategoryMaps
    
    {
        CategoryMaps oneCategoryMap;
        
#ifdef DEBUG
        CERR << "one \"duplicate category name\" error follows:" << endl;
#endif
        
        oneCategoryMap.insertCategory("alpha");
        oneCategoryMap.findOrInsertCategory("alpha");
        oneCategoryMap.findOrInsertCategory("bravo");
        
        try {
            oneCategoryMap.insertCategory("alpha");
        } catch(...) { }
        
        index_t index;
        oneCategoryMap.findIndexForCategory("bravo", index);
        
        string category;
        oneCategoryMap.findCategoryForIndex(1, category);
        
        oneCategoryMap.getIndexForCategory("alpha");
        oneCategoryMap.getCategoryForIndex(1);
        
        try {
            oneCategoryMap.getIndexForCategory("charlie");
        } catch(...) { }
        
        try {
            oneCategoryMap.getCategoryForIndex(42);
        } catch(...) { }
        
        
        oneCategoryMap.beginIndex();
        oneCategoryMap.endIndex();
        oneCategoryMap.countNamedCategories();
        oneCategoryMap.countAllCategories();
        
        if (verbose) {
            oneCategoryMap.dump();
        }
        
        oneCategoryMap.setUseNaCategory(true);
        oneCategoryMap.findCategoryForIndex(NO_INDEX, category);
        
    }

    // ~~~~~~~~~~~~~~~~~~~~~~
    // SelectIndexes
    
    {
        SelectIndexes selectIndexes;
        
        selectIndexes.clear(3);
        selectIndexes.select(1);

        selectIndexes.selectAll(5);
        selectIndexes.unselect(2);
        
        if (verbose) {
            CERR << "~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~" << endl;
            CERR << "selectIndexes.dump()" << endl;
            selectIndexes.dump();
            CERR << "~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~" << endl;
        }
        
        selectIndexes.boolVector();
        selectIndexes.indexVector();
        
        selectIndexes.countSelected();
    }
    
    {
        SelectIndexes selectIndexes(3, false);
        SelectIndexes other(selectIndexes);
    }
    
    {
        SelectIndexes selectIndexes(3, true);
    }

    // ~~~~~~~~~~~~~~~~~~~~~~
    // SortValueVector
    
    fillValues(values, valueTypes);
    
    for (size_t col = 0; col <= 1; col++) {
        SortValueVector sorter(values[col], valueTypes[col]);
        
        vector<size_t> indexVector;
        sorter.sort(indexVector);
        
        size_t numRows = values[col].size();
        selectRows.selectAll(numRows);
        
        sorter.sort(indexVector, selectRows);
    }
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // meanValue
    
    size_t numRows = values[0].size();
    selectRows.selectAll(numRows);
    
    meanValue(values[0], selectRows);
    
    meanValue(vector<Value>(0), SelectIndexes(0, false));
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // medianValue
    
    selectCols.selectAll(values.size());
    makeSortedIndexes(values, valueTypes, selectCols, sortedIndexes);
    medianValue(values[0], selectRows, sortedIndexes[0]);
    
    medianValue(vector<Value>(0), SelectIndexes(0, false), vector<size_t>(0));

    // ~~~~~~~~~~~~~~~~~~~~~~
    // modeValue
    
    CategoryMaps categoryMaps1;
    categoryMaps1.insertCategory("B");
    categoryMaps1.insertCategory("A");
    categoryMaps1.insertCategory("C");
    
    categoryMaps.clear();
    categoryMaps.push_back(CategoryMaps());
    categoryMaps.push_back(categoryMaps1);
    categoryMaps.push_back(categoryMaps1);
    
    modeValue(values[1], selectRows, categoryMaps[1]);
    
    modeValue(values[2], selectRows, categoryMaps[2]);
    
    modeValue(vector<Value>(0), SelectIndexes(0, false), CategoryMaps());
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // printValuesColumn
    
    if (verbose) {
        printValuesColumn(values[0], valueTypes[0], categoryMaps[0]);
        printValuesColumn(values[2], valueTypes[2], categoryMaps[1]);
    }

    // ~~~~~~~~~~~~~~~~~~~~~~
    // printValues
    
    vector<string> colNames;
    colNames.push_back("C1");
    colNames.push_back("C2");
    colNames.push_back("C3");
    
    if (verbose) {
        printValues(values, valueTypes, categoryMaps, colNames);    
    }

    // ~~~~~~~~~~~~~~~~~~~~~~
    // imputedValue

    vector<ImputeOption> convertTypes;
    
    convertTypes.clear();
    convertTypes.push_back(kToDefault);
    convertTypes.push_back(kNoImpute);
    convertTypes.push_back(kNoImpute);
    
    try {
        imputedValue(0, convertTypes, values, valueTypes, selectRows, categoryMaps, sortedIndexes);        
    } catch(...) { }
    
    imputedValue(1, convertTypes, values, valueTypes, selectRows, categoryMaps, sortedIndexes);
    imputedValue(2, convertTypes, values, valueTypes, selectRows, categoryMaps, sortedIndexes);
    
    convertTypes.clear();
    convertTypes.push_back(kToMean);
    convertTypes.push_back(kToCategory);
    convertTypes.push_back(kToMode);
    
    imputedValue(0, convertTypes, values, valueTypes, selectRows, categoryMaps, sortedIndexes);
    imputedValue(1, convertTypes, values, valueTypes, selectRows, categoryMaps, sortedIndexes);
    imputedValue(2, convertTypes, values, valueTypes, selectRows, categoryMaps, sortedIndexes);
    
    convertTypes.clear();
    convertTypes.push_back(kToMedian);
    convertTypes.push_back(kToCategory);
    convertTypes.push_back(kNoImpute);
    
    imputedValue(0, convertTypes, values, valueTypes, selectRows, categoryMaps, sortedIndexes);

    // ~~~~~~~~~~~~~~~~~~~~~~
    // imputeValues
    
    vector<Value> imputedValues;

    imputeValues(convertTypes, valueTypes, values, selectRows, selectCols, categoryMaps,
                 sortedIndexes, imputedValues);

    // ~~~~~~~~~~~~~~~~~~~~~~
    // cellsToValues
    // valuesToCells

    readCsvString("1, 2,      A, 4,      5\n"
                  " , \"7\",   , \"NA\", NA\n"
                  " , \"7\", NA, \"NA\", NA", cells, quoted);
    
    getDefaultValueTypes(cells, quoted, true, "NA", valueTypes);
    
    cellsToValues(cells, quoted, valueTypes, false, "", values, false, categoryMaps);
    cellsToValues(cells, quoted, valueTypes, false, "", values, true, categoryMaps);
    
    categoryMaps.assign(5, CategoryMaps());
    cellsToValues(cells, quoted, valueTypes, false, "", values, true, categoryMaps);
    
    categoryMaps.clear();
    cellsToValues(cells, quoted, valueTypes, true, "NA", values, false, categoryMaps);
    cellsToValues(cells, quoted, valueTypes, true, "NA", values, true, categoryMaps);
    
    valuesToCells(values, valueTypes, categoryMaps, true, "NA", cells, quoted);

    categoryMaps.assign(5, CategoryMaps());
    valuesToCells(values, valueTypes, categoryMaps, true, "NA", cells, quoted);

    // ~~~~~~~~~~~~~~~~~~~~~~
    // makeSortedIndexes
    
    fillValues(values, valueTypes);
    
    selectCols.selectAll(3);
    selectCols.unselect(2);
    
    makeSortedIndexes(values, valueTypes, selectCols, sortedIndexes);

    // ~~~~~~~~~~~~~~~~~~~~~~
    // getDefaultValueTypes
    
    readCsvString("1, 2,     A, 4, 5\n"
                  "6, \"7\", 8, \"NA\", NA", cells, quoted);

    getDefaultValueTypes(cells, quoted, true, "NA", valueTypes);
    if (verbose) {
        for (size_t k = 0; k < valueTypes.size(); k++) {
            string str;
            valueTypeToString(valueTypes[k], str);
            CERR << str << " ";
        }
        CERR << endl;
    }
    
    getDefaultValueTypes(cells, quoted, false, "", valueTypes);
    if (verbose) {
        for (size_t k = 0; k < valueTypes.size(); k++) {
            string str;
            valueTypeToString(valueTypes[k], str);
            CERR << str << " ";
        }
        CERR << endl;
    }

    // ~~~~~~~~~~~~~~~~~~~~~~
    // stringToValueType
    // valueTypeToString
    
    string str;
    
    valueTypeToString(kCategorical, str);
    stringToValueType(str);
    
    valueTypeToString(kNumeric, str);
    stringToValueType(str);
    
#ifdef DEBUG
    CERR << "two \"invalid valueType\" errors follow:" << endl;
#endif
    
    try {
        stringToValueType("");
    } catch(...) { }
    
    try {
        stringToValueType("xxx");
    } catch(...) { }
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // getDefaultImputeOption
    
    getDefaultImputeOption(kCategorical);
    getDefaultImputeOption(kNumeric);
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // stringToImputeOption
    // imputeOptionToString
    
    {
        string str;
        
        // for categorical types
        
        imputeOptionToString(kNoImpute, str);
        stringToImputeOption(str, kCategorical);
        
        imputeOptionToString(kToCategory, str);
        stringToImputeOption(str, kCategorical);
        
        imputeOptionToString(kToMode, str);
        stringToImputeOption(str, kCategorical);
        
        imputeOptionToString(kToDefault, str);
        stringToImputeOption(str, kCategorical);
        
        // for numerical types
        
        imputeOptionToString(kNoImpute, str);
        stringToImputeOption(str, kNumeric);
        
        imputeOptionToString(kToMean, str);
        stringToImputeOption(str, kNumeric);
        
        imputeOptionToString(kToMedian, str);
        stringToImputeOption(str, kNumeric);
        
        imputeOptionToString(kToDefault, str);
        stringToImputeOption(str, kNumeric);
    }

#ifdef DEBUG
    CERR << "one \"invalid imputeOption\" error follows:" << endl;
#endif

    try {
        stringToImputeOption("", kCategorical);
    } catch(...) { }
    
}

