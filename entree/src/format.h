//
//  format.h
//  entree
//
//  Created by MPB on 5/7/13.
//  Copyright (c) 2013 Quadrivio Corporation. All rights reserved.
//  License http://opensource.org/licenses/BSD-2-Clause
//          <YEAR> = 2013
//          <OWNER> = Quadrivio Corporation
//

//
//  Utilities for handling values and categories
//

#ifndef entree_format_h
#define entree_format_h

#include <string>
#include <vector>
#include <map>

// ========== Types ================================================================================

// use for sizes and indexes
typedef signed long index_t;

// indicates missing or unknown index
const index_t NO_INDEX = -1;

// types of attribute values
enum ValueType {
    kNumeric,
    kCategorical
};

// union of possible types of values
union Number {
    double d;   // numeric value
    index_t i;  // category index
};
typedef union Number Number;

// generic attribute value
struct Value {
    Number number;
    bool na;        // not available (NA)
};
typedef struct Value Value;

// NA Value
extern const Value gNaValue;

// options for handling NA values in attributes
enum ImputeOption {
    kNoImpute,  // leave NA unchanged
    
    // for categorical types
    kToCategory,
    kToMode,
    
    // for numerical types
    kToMean,
    kToMedian,
    
    kToDefault
};

// TODO more imputeOptions:
//    kToFlagAndMode,
//    kToBranchMode,
//    kToFlagAndBranchMode,
//    kToFlagAndMean,
//    kToFlagAndMedian,
//    kToBranchMean,
//    kToBranchMedian,
//    kToFlagAndBranchMean,
//    kToFlagAndBranchMedian,

// ========== Class Declarations ===================================================================

// handles mapping between category indexes and category names; allows for treating NA as a
// separate category
class CategoryMaps {
public:
    static const std::string naCategory;

    CategoryMaps();
    virtual ~CategoryMaps();
    
    bool getUseNaCategory() const { return useNaCategory; };
    
    void setUseNaCategory(bool useNaCategory) { this->useNaCategory = useNaCategory; };
    
    // insert category if missing; return index
    index_t findOrInsertCategory(const std::string& category);
    
    // insert category; error if already present
    index_t insertCategory(const std::string& category);
    
    // look for category; if found, write index into param and return true, else write NO_INDEX into
    // param and return false
    bool findIndexForCategory(const std::string& category, index_t& index) const;

    // look for index; if found, write category into param and return true, else write " <NA> " into
    // param and return false
    bool findCategoryForIndex(index_t index, std::string& category) const;
    
    // look for category, return index; error if not found
    index_t getIndexForCategory(const std::string& category) const;

    // look for index, return category; error if not found
    std::string getCategoryForIndex(index_t index) const;
    
    // return lowest index number; for enumeration
    index_t beginIndex() const;

    // return higest index number + 1; for enumeration
    index_t endIndex() const;
    
    // return count of all categories, excluding NA category
    size_t countNamedCategories() const;
    
    // return count of all categories, including NA category if used
    size_t countAllCategories() const;
    
    // clear all named categories
    void clear();
    
    // for debugging; print info
    void dump() const;
    
private:
    // if true, treat NA values as a separate category with index NO_INDEX
    bool useNaCategory;

    // map in both directions
    std::vector<std::string> categories;
    std::map<std::string, index_t> categoryToIndex;
    
};

// -------------------------------------------------------------------------------------------------

// handles iterating through lists of selected indexes, either by testing an index to see if it is
// selected or by providing the index numbers of the selected indexes
class SelectIndexes {
public:
    
    // construct with empty lists
    SelectIndexes();

    // construct with all indexes (from 0 to size - 1) initially selected or unselected
    SelectIndexes(size_t size, bool selected);

    // copy constructor
    SelectIndexes(const SelectIndexes& other);

    virtual ~SelectIndexes();
    
    // change sizes of lists and unselect all indexes (from 0 to size - 1)
    void clear(size_t size);

    // change sizes of lists and select all indexes (from 0 to size - 1)
    void selectAll(size_t size);
    
    // select specified index; must be in existing size range 
    void select(size_t index);

    // unselect specified index; must be in existing size range 
    void unselect(size_t index);
    
    // for debugging; print state
    void dump() const;
    
    // return bool vector with item for each possible index (0 to size - 1), valued true if index is
    // selected and false if not selected
    const std::vector<bool>& boolVector() const;

    // return vector of indexes that are currently selected
    const std::vector<size_t>& indexVector() const;
    
    // return count of indexes currently selected
    size_t countSelected() const { return selected; };
    
private:
    std::vector<bool> bitMap;
    std::vector<size_t> indexes;
    size_t selected;
};

// -------------------------------------------------------------------------------------------------

// handles sorting a vector of indexes pointing to entries in a vector of Value structs; sorts
// indexes in ascending order of Value; NA sorts to beginning; index order is preserved for equal
// values
class SortValueVector {
public:
    // constructor; prepare to sort items in valueVector; type is specified to be valueType
    SortValueVector(const std::vector<Value>& valueVector, ValueType valueType);
    virtual ~SortValueVector();
    
    // sort all items in valueVector; indexVector will be set to result
    void sort(std::vector<size_t>& indexVector);
    
    // sort selected items in valueVector; indexVector will be set to result
    void sort(std::vector<size_t>& indexVector, const SelectIndexes& selectRows);
    
    // comparison operator for sort
    bool operator ()(size_t i, size_t j);
    
private:
    const std::vector<Value>& valueVector;
    ValueType valueType;
};

// ========== Function Headers =====================================================================

// calculate mean of selected rows in a vector of numerical Values
Value meanValue(const std::vector<Value>& valuesColumn, const SelectIndexes& selectRows);

// calculate median of selected rows in a vector of numerical Values; must supply vector of sorted
// indexes that at least includes all of the selected rows
Value medianValue(const std::vector<Value>& valuesColumn,
                  const SelectIndexes& selectRows,
                  const std::vector<size_t>& sortedIndexes);

// select modal value of selected rows in a vector of categorical Values; in case of tie, choose
// category with name that sorts earlier alphabetically; NA category (if used) has name " <NA> "
Value modeValue(const std::vector<Value>& valuesColumn,
                const SelectIndexes& selectRows,
                const CategoryMaps& categoryMaps);

// for debugging or logging; print vector of Value
void printValuesColumn(const std::vector<Value>& valuesColumn,
                       ValueType valueType,
                       const CategoryMaps& categoryMaps,
                       index_t maxRows = -1);

// for debugging or logging; print array of Values
void printValues(const std::vector< std::vector<Value> >& values,
                 const std::vector<ValueType>& valueTypes,
                 const std::vector<CategoryMaps>& categoryMaps,
                 const std::vector<std::string>& colNames);

// return Value to be used as replacement for NA for specified column and selection of rows 
Value imputedValue(size_t col,
                   const std::vector<ImputeOption>& convertTypes,
                   const std::vector< std::vector<Value> >& values,
                   const std::vector<ValueType>& valueTypes,
                   const SelectIndexes& selectRows,
                   const std::vector<CategoryMaps>& categoryMaps,
                   const std::vector< std::vector<size_t> >& sortedIndexes);

// replace NA values in selected columns and rows in array of Values
void imputeValues(const std::vector<ImputeOption>& convertTypes,
                  const std::vector<ValueType>& valueTypes,
                  std::vector< std::vector<Value> >& values,
                  const SelectIndexes& selectRows,
                  const SelectIndexes& selectColumns,
                  std::vector<CategoryMaps>& categoryMaps,
                  std::vector< std::vector<size_t> >& sortedIndexes,
                  std::vector<Value>& imputedValues);

// convert array of strings (as vector of rows) to array of Values (as vector of columns)
void cellsToValues(const std::vector< std::vector<std::string> >& cells,
                   const std::vector< std::vector<bool> >& quoted,
                   const std::vector<ValueType> valueTypes,
                   bool interpretNA,
                   const std::string& naString,
                   std::vector< std::vector<Value> >& values,
                   bool constCategories,
                   std::vector<CategoryMaps>& categoryMaps);

// convert array of Values (as vector of columns) to array of strings (as vector of rows)
void valuesToCells(const std::vector< std::vector<Value> >& values,
                   const std::vector<ValueType> valueTypes,
                   const std::vector<CategoryMaps>& categoryMaps,
                   bool writeNA,
                   const std::string& naString,
                   std::vector< std::vector<std::string> >& cells,
                   std::vector< std::vector<bool> >& quoted);

// create vector of sorted indexes for each selected column in array of Values
void makeSortedIndexes(const std::vector< std::vector<Value> >& values,
                       const std::vector<ValueType> valueTypes,
                       const SelectIndexes& selectColumns,
                       std::vector< std::vector<size_t> >& sortedIndexes);

// get default value types (assume numeric unless cells contain other than digits or period)
void getDefaultValueTypes(const std::vector< std::vector<std::string> >& cells,
                          const std::vector< std::vector<bool> >& quoted,
                          bool interpretNA,
                          const std::string& naString,
                          std::vector<ValueType>& valueTypes);

// get ValueType for name
ValueType stringToValueType(std::string str);

// get name of ValueType
void valueTypeToString(ValueType valueType, std::string& str);   

// get default ImputeOption
ImputeOption getDefaultImputeOption(ValueType valueType);

// get ImputeOption for name
ImputeOption stringToImputeOption(std::string str, ValueType valueType);

// get name of ImputeOption
void imputeOptionToString(ImputeOption imputeOption, std::string& str);

// component tests
void ctest_format(int& totalPassed, int& totalFailed, bool verbose);

// code coverage
void cover_format(bool verbose);

#endif
