//
//  entree.cpp
//  entree
//
//  Created by MPB on 5/3/13.
//  Copyright (c) 2013 Quadrivio Corporation. All rights reserved.
//  License http://opensource.org/licenses/BSD-2-Clause
//          <YEAR> = 2013
//          <OWNER> = Quadrivio Corporation
//

//
//  C functions callable from R to handle entree() and predict.entree()
//

#include "entree.h"

#if RPACKAGE

#include "format.h"
#include "predict.h"
#include "shim.h"
#include "train.h"

#include <algorithm>
#include <cctype>
#include <vector>

using namespace std;

// ========== Local Headers ========================================================================

// convert R vector to vector of Value; update valueType and categoryMaps if requested
void convertRVector(SEXP rVector,
                    size_t rowCount,
                    bool constValueType,
                    ValueType& valueType,
                    vector<Value>& values,
                    bool constCategoryMaps,
                    CategoryMaps& categoryMaps,
                    const string& name);

// ========== Globals ==============================================================================

bool gTrace = false;    // for debugging

// ========== Functions ============================================================================

// call from R to train model from attributes and response
SEXP entree_C(SEXP x,
              SEXP y,
              SEXP s_maxDepth,
              SEXP s_minDepth,
              SEXP s_maxTrees,
              SEXP s_columnsPerTree,
              SEXP s_doPrune,
              SEXP s_minImprovement,
              SEXP s_minLeafCount,
              SEXP s_maxSplitsPerNumericAttribute,
              SEXP s_xValueTypes,
              SEXP s_yValueType,
              SEXP s_xImputeOptions)
{
    if (gTrace) CERR << "entree_C" << endl;
    
    // --------------- verify arg types ---------------

    if (gTrace) CERR << "verify arg types" << endl;
    
    if (!Rf_isVectorList(x)) {
        error("entree_C: wrong x type");
        
    } else if (!Rf_isVector(y)) {
        error("entree_C: wrong y type");
        
    } else if (!Rf_isInteger(s_maxDepth)) {
        error("entree_C: wrong maxDepth type");
        
    } else if (!Rf_isInteger(s_minDepth)) {
        error("entree_C: wrong minDepth type");
        
    } else if (!Rf_isInteger(s_maxTrees)) {
        error("entree_C: wrong maxTrees type");
        
    } else if (!Rf_isInteger(s_columnsPerTree)) {
        error("entree_C: wrong columnsPerTree type");
        
    } else if (!Rf_isInteger(s_doPrune)) {
        error("entree_C: wrong doPrune type");
        
    } else if (!Rf_isReal(s_minImprovement)) {
        error("entree_C: wrong minImprovement type");
        
    } else if (!Rf_isInteger(s_minLeafCount)) {
        error("entree_C: wrong minLeafCount type");
        
    } else if (!Rf_isInteger(s_maxSplitsPerNumericAttribute)) {
        error("entree_C: wrong maxSplitsPerNumericAttribute type");
        
    } else if (!Rf_isString(s_xValueTypes)) {
        error("entree_C: wrong xValueTypes type");
        
    } else if (!Rf_isString(s_yValueType)) {
        error("entree_C: wrong yValueType type");
        
    } else if (!Rf_isString(s_xImputeOptions)) {
        error("entree_C: wrong xImputeOptions type");
    } 
    
    // --------------- verify x is a data.frame ---------------
    
    if (gTrace) CERR << "verify x is a data.frame" << endl;
    
    {
        SEXP xClass;
        PROTECT(xClass = Rf_getAttrib(x, R_ClassSymbol));
        
        if (!Rf_isNull(xClass) && strcmp("data.frame", CHAR(STRING_ELT(xClass, 0))) != 0) {
            error("entree_C: wrong x class");
        }
        
        UNPROTECT(1);
    }
    
    // --------------- verify x and y dimensions match ---------------
    
    if (gTrace) CERR << "verify x and y dimensions match" << endl;
    
    size_t yLength = (size_t)Rf_length(y);

    size_t xCols = (size_t)Rf_length(x);
    
    size_t xRows = 0;
    if (xCols > 0) {
        xRows = (size_t)Rf_length(VECTOR_ELT(x, 0));
    }

    if (gTrace) CERR << "xCols = " << xCols << ", xRows = " << xRows << endl;
    
    if (yLength != xRows) {
        error("entree_C: arg dimensions mismatch");
    }
    
    // --------------- get x column names ---------------
    
    if (gTrace) CERR << "get x column names" << endl;
    
    vector<string> colNames;
    {
        SEXP names;
        PROTECT(names = Rf_getAttrib(x, R_NamesSymbol));
        
        int nameCount = Rf_length(names);
        for (int k = 0; k < nameCount; k++) {
            string str = CHAR(STRING_ELT(names, k));
            colNames.push_back(str);
        }
        
        UNPROTECT(1);
    }
    
    // --------------- convert y valueType ---------------
    
    if (gTrace) CERR << "convert y valueType" << endl;

    ValueType yValueType = kCategorical;    // default; to be adjusted later in convertRVector()
    bool constYValueType = false;

    if (Rf_length(s_yValueType) == 1 && Rf_isString(s_yValueType)) {
        string str = CHAR(STRING_ELT(s_yValueType, 0));
        
        yValueType = stringToValueType(str);
        constYValueType = true;    // do not modify in convertRVector()
    }
    
    // --------------- convert y ---------------
    
    if (gTrace) CERR << "convert y" << endl;
    
    vector<Value> yValues;
    CategoryMaps yCategoryMaps;
    
    convertRVector(y, yLength, constYValueType, yValueType, yValues, false, yCategoryMaps, "<Y>");
    
    // --------------- convert x valueType ---------------
    
    if (gTrace) CERR << "convert x valueType" << endl;

    vector<ValueType> xValueTypes(xCols, kNumeric);
    bool constXValueTypes = false;

    size_t numXValueTypes = (size_t)Rf_length(s_xValueTypes);
    
    if (gTrace) CERR << "numXValueTypes = " << numXValueTypes << endl;
    
    if (numXValueTypes == 0) {
        // default; to be adjusted later in convertRVector()
        
    } else if (numXValueTypes != xCols) {
        error("invalid length of xValueTypes");
        
    } else if (Rf_isString(s_xValueTypes)) {
        for (size_t k = 0; k < xCols; k++) {
            string str = CHAR(STRING_ELT(s_xValueTypes, (int)k));
            
            xValueTypes[k] = stringToValueType(str);
            constXValueTypes = true;    // do not modify in convertRVector()
        }
        
    } else {
        error("invalid type of xValueTypes");
    }
    
    // --------------- convert x ---------------
    
    if (gTrace) CERR << "convert x" << endl;
    
    vector< vector<Value> > xValues;
    vector<CategoryMaps> xCategoryMaps;
    
    for (size_t col = 0; col < xCols; col++) {
        xValues.push_back(vector<Value>());
        xCategoryMaps.push_back(CategoryMaps());
        
        convertRVector(VECTOR_ELT(x, (int)col), xRows, constXValueTypes, xValueTypes[col],
                       xValues[col], false, xCategoryMaps[col], colNames[col]);

    }

    // --------------- append y to x ---------------
    
    if (gTrace) CERR << "append y to x" << endl;
    
    size_t targetColumn = xValueTypes.size();   // last column is used for <Y>
    
    xValueTypes.push_back(yValueType);
    xValues.push_back(yValues);
    xCategoryMaps.push_back(yCategoryMaps);
    colNames.push_back("<Y>");
    
    // --------------- impute options ---------------
    
    if (gTrace) CERR << "impute options" << endl;

    vector<ImputeOption> imputeOptions;
    int numImputeOptions = Rf_length(s_xImputeOptions);

    if (gTrace) CERR << "numImputeOptions = " << numImputeOptions << endl;
    
    if (numImputeOptions == 0) {
        // default
        imputeOptions.assign(xCols, kToDefault);
        
    } else if (numImputeOptions != xCols) {
        error("invalid length of xImputeOptions");
                
    } else if (Rf_isString(s_xImputeOptions)) {
        for (size_t k = 0; k < xCols; k++) {
            string str = CHAR(STRING_ELT(s_xImputeOptions, (int)k));
            
            ImputeOption imputeOption = stringToImputeOption(str, xValueTypes[k]);

            imputeOptions.push_back(imputeOption);
        }

    } else {
        error("invalid type of xImputeOptions");
    }
    
    // for y
    imputeOptions.push_back(kNoImpute);


    // --------------- train ---------------
    
    if (gTrace) CERR << "train" << endl;
    
    vector<CompactTree> trees;
    
    index_t columnsPerTree = *INTEGER(s_columnsPerTree);
    int maxDepth = *INTEGER(s_maxDepth);
    int minDepth = *INTEGER(s_minDepth);
    index_t maxTrees = *INTEGER(s_maxTrees);
    index_t maxNodes = -1;
    bool doPrune = *INTEGER(s_doPrune);
    double minImprovement = *REAL(s_minImprovement);
    index_t minLeafCount = *INTEGER(s_minLeafCount);
    index_t maxSplitsPerNumericAttribute = *INTEGER(s_maxSplitsPerNumericAttribute);
    
    SelectIndexes selectRows(xRows, true);
    
    SelectIndexes availableColumns(xCols + 1, true);
    availableColumns.unselect(xCols);
    
    SelectIndexes selectColumns;
    
    train(trees, columnsPerTree, maxDepth, minDepth, doPrune, minImprovement, minLeafCount,
          maxSplitsPerNumericAttribute, maxTrees, maxNodes, selectRows, availableColumns,
          selectColumns, xValues, xValueTypes, xCategoryMaps, targetColumn, colNames,
          imputeOptions);
    
    if (trees.size() == 0) {
        CERR << "no trees found" << endl;
    }
    
    // --------------- package training results ---------------
    
    if (gTrace) CERR << "package training results" << endl;
    
    size_t numTrees = trees.size();
    int resultUnprotectCount = 0;

    int numResultFields = 9;
    int resultFieldIndex = -1;
    
	SEXP result;
	PROTECT(result = Rf_allocVector(VECSXP, numResultFields));
    resultUnprotectCount++;

	SEXP className;
    PROTECT(className = Rf_allocVector(STRSXP, 1));
    SET_STRING_ELT(className, 0, Rf_mkChar("entree"));
    Rf_classgets(result, className);
    resultUnprotectCount++;

	SEXP result_names;
	PROTECT(result_names = Rf_allocVector(STRSXP, numResultFields));
    resultUnprotectCount++;

    Rf_setAttrib(result, R_NamesSymbol, result_names);

    // ---------- result$trees ---------------------------------------------------------------------
    
    if (gTrace) CERR << "result$trees " << numTrees << endl;
    
    resultFieldIndex++;
    
    SET_STRING_ELT(result_names, resultFieldIndex, Rf_mkChar("trees"));
    
    SEXP result_trees;
    PROTECT(result_trees = Rf_allocVector(VECSXP, (int)numTrees));
    resultUnprotectCount++;

    SET_VECTOR_ELT(result, resultFieldIndex, result_trees);

    vector<SEXP> s_trees(numTrees);
    
    for (size_t k = 0; k < numTrees; k++) {
        int treeUnprotectCount = 0;
        
        const int numTreeFields = 7;
        SEXP s_splitColIndex;
        SEXP s_lessOrEqualIndex;
        SEXP s_greaterOrNotIndex;
        SEXP s_toLessOrEqualIfNA;
        SEXP s_value_switch;
        SEXP s_value_d;
        SEXP s_value_i;
        
        PROTECT(s_trees[k] = Rf_allocVector(VECSXP, numTreeFields));
        treeUnprotectCount++;

        SET_VECTOR_ELT(result_trees, (int)k, s_trees[k]);

        size_t numNodes = trees[k].splitColIndex.size();

        SEXP field_names;
        PROTECT(field_names = Rf_allocVector(STRSXP, numTreeFields));
        treeUnprotectCount++;
        
        int fieldNum;
        
        // --------------------------------------------------------------
        
        fieldNum = 0;
        
        SET_STRING_ELT(field_names, fieldNum, Rf_mkChar("splitCol"));
        PROTECT(s_splitColIndex = Rf_allocVector(INTSXP, (int)numNodes));
        treeUnprotectCount++;
        int *splitColIndex = INTEGER(s_splitColIndex);
        for (size_t n = 0; n < numNodes; n++) {
            splitColIndex[n] = (int)trees[k].splitColIndex[n];
        }

        SET_VECTOR_ELT(s_trees[k], fieldNum, s_splitColIndex);

        // --------------------------------------------------------------
        
        fieldNum = 1;
        
        SET_STRING_ELT(field_names, fieldNum, Rf_mkChar("lessOrEqualIndex"));
        PROTECT(s_lessOrEqualIndex = Rf_allocVector(INTSXP, (int)numNodes));
        treeUnprotectCount++;
        int *lessOrEqualIndex = INTEGER(s_lessOrEqualIndex);
        for (size_t n = 0; n < numNodes; n++) {
            lessOrEqualIndex[n] = (int)trees[k].lessOrEqualIndex[n];
        }
        
        SET_VECTOR_ELT(s_trees[k], fieldNum, s_lessOrEqualIndex);
        
        // --------------------------------------------------------------
        
        fieldNum = 2;
        
        SET_STRING_ELT(field_names, fieldNum, Rf_mkChar("greaterOrNotIndex"));
        PROTECT(s_greaterOrNotIndex = Rf_allocVector(INTSXP, (int)numNodes));
        treeUnprotectCount++;
        int *greaterOrNotIndex = INTEGER(s_greaterOrNotIndex);
        for (size_t n = 0; n < numNodes; n++) {
            greaterOrNotIndex[n] = (int)trees[k].greaterOrNotIndex[n];
        }
        
        SET_VECTOR_ELT(s_trees[k], fieldNum, s_greaterOrNotIndex);
        
        // --------------------------------------------------------------
        
        fieldNum = 3;
        
        SET_STRING_ELT(field_names, fieldNum, Rf_mkChar("toLessOrEqualIfNA"));
        PROTECT(s_toLessOrEqualIfNA = Rf_allocVector(INTSXP, (int)numNodes));
        treeUnprotectCount++;
        int *toLessOrEqualIfNA = INTEGER(s_toLessOrEqualIfNA);
        for (size_t n = 0; n < numNodes; n++) {
            toLessOrEqualIfNA[n] = (int)trees[k].toLessOrEqualIfNA[n];
        }
        
        SET_VECTOR_ELT(s_trees[k], fieldNum, s_toLessOrEqualIfNA);
        
        // --------------------------------------------------------------
        
        fieldNum = 4;
        
        SET_STRING_ELT(field_names, fieldNum, Rf_mkChar("value_switch"));
        PROTECT(s_value_switch = Rf_allocVector(INTSXP, (int)numNodes));
        treeUnprotectCount++;
        int *value_switch = INTEGER(s_value_switch);
        
        // --------------------------------------------------------------
        
        fieldNum = 5;
        
        SET_STRING_ELT(field_names, fieldNum, Rf_mkChar("value_d"));
        PROTECT(s_value_d = Rf_allocVector(REALSXP, (int)numNodes));
        treeUnprotectCount++;
        double *value_d = REAL(s_value_d);
        
        // --------------------------------------------------------------
        
        fieldNum = 6;
        
        SET_STRING_ELT(field_names, fieldNum, Rf_mkChar("value_i"));
        PROTECT(s_value_i = Rf_allocVector(INTSXP, (int)numNodes));
        treeUnprotectCount++;
        int *value_i = INTEGER(s_value_i);

        // --------------------------------------------------------------
        
        const vector<size_t>& selectColumnIndexes = selectColumns.indexVector();

        for (size_t n = 0; n < numNodes; n++) {
            index_t colIndex = splitColIndex[n];
            size_t col;
            if (colIndex == NO_INDEX) {
                col = targetColumn;
                
            } else {
                col = selectColumnIndexes.at((size_t)colIndex);    
            }
            
            switch (xValueTypes[col]) {
                case kCategorical:
                    value_switch[n] = kCategorical;
                    value_d[n] = 0.0;
                    value_i[n] = (int)trees[k].value[n].i;
                    break;
                    
                case kNumeric:
                    value_switch[n] = kNumeric;
                    value_d[n] = trees[k].value[n].d;
                    value_i[n] = 0;
                    break;
            }
        }

        fieldNum = 4;
        SET_VECTOR_ELT(s_trees[k], fieldNum, s_value_switch);
        
        fieldNum = 5;
        SET_VECTOR_ELT(s_trees[k], fieldNum, s_value_d);
        
        fieldNum = 6;
        SET_VECTOR_ELT(s_trees[k], fieldNum, s_value_i);

        Rf_setAttrib(s_trees[k], R_NamesSymbol, field_names);

    	UNPROTECT(treeUnprotectCount);
    }
    
    // ---------- result$xColumnCount ---------------------------------------------------------------------
    
    if (gTrace) CERR << "result$xColumnCount" << endl;
    
    resultFieldIndex++;
    
    SET_STRING_ELT(result_names, resultFieldIndex, Rf_mkChar("xColumnCount"));
    
    SEXP result_xColumnCount;
    PROTECT(result_xColumnCount = Rf_allocVector(INTSXP, 1));
    resultUnprotectCount++;
    
    *INTEGER(result_xColumnCount) = (int)xCols;
    
    SET_VECTOR_ELT(result, resultFieldIndex, result_xColumnCount);
    
    // ---------- result$selectColumns ---------------------------------------------------------------------
    
    if (gTrace) CERR << "result$selectColumns" << endl;
    
    resultFieldIndex++;
    
    SET_STRING_ELT(result_names, resultFieldIndex, Rf_mkChar("selectColumns"));
    
    size_t numSelectColumns = selectColumns.countSelected();
    
    SEXP result_selectColumns;
    PROTECT(result_selectColumns = Rf_allocVector(INTSXP, (int)numSelectColumns));
    resultUnprotectCount++;
    
    int *selectColumnsP = INTEGER(result_selectColumns);
    
    const vector<size_t>& selectColumnIndexes = selectColumns.indexVector();
    
    for (size_t k = 0; k < numSelectColumns; k++) {
        selectColumnsP[k] = (int)selectColumnIndexes[k];    
    }
    
    SET_VECTOR_ELT(result, resultFieldIndex, result_selectColumns);
    
    // ---------- result$columnsPerTree ---------------------------------------------------------------------
    
    if (gTrace) CERR << "result$columnsPerTree" << endl;
    
    resultFieldIndex++;
    
    SET_STRING_ELT(result_names, resultFieldIndex, Rf_mkChar("columnsPerTree"));
    
    SEXP result_columnsPerTree;
    PROTECT(result_columnsPerTree = Rf_allocVector(INTSXP, 1));
    resultUnprotectCount++;
    
    *INTEGER(result_columnsPerTree) = (int)columnsPerTree;
    
    SET_VECTOR_ELT(result, resultFieldIndex, result_columnsPerTree);
    
    // ---------- result$valueTypes ---------------------------------------------------------------------
    
    if (gTrace) CERR << "result$valueTypes" << endl;
    
    resultFieldIndex++;
    
    SET_STRING_ELT(result_names, resultFieldIndex, Rf_mkChar("valueTypes"));
    
    size_t numValueTypes = xValueTypes.size();
    
    SEXP result_valueTypes;
    PROTECT(result_valueTypes = Rf_allocVector(STRSXP, (int)numValueTypes));
    resultUnprotectCount++;
    
    for (size_t k = 0; k < numValueTypes; k++) {
        string str;
        valueTypeToString(xValueTypes[k], str);
        
        SET_STRING_ELT(result_valueTypes, (int)k, Rf_mkChar(str.c_str()));
    }
    
    SET_VECTOR_ELT(result, resultFieldIndex, result_valueTypes);
    
    // ---------- result$imputeOptions ---------------------------------------------------------------------
    
    if (gTrace) CERR << "result$imputeOptions" << endl;
    
    resultFieldIndex++;
    
    SET_STRING_ELT(result_names, resultFieldIndex, Rf_mkChar("imputeOptions"));
    
    numImputeOptions = (int)imputeOptions.size();
    
    SEXP result_imputeOptions;
    PROTECT(result_imputeOptions = Rf_allocVector(STRSXP, numImputeOptions));
    resultUnprotectCount++;
    
    for (size_t k = 0; k < numImputeOptions; k++) {
        string str;
        imputeOptionToString(imputeOptions[k], str);
        
        SET_STRING_ELT(result_imputeOptions, (int)k, Rf_mkChar(str.c_str()));
    }
    
    SET_VECTOR_ELT(result, resultFieldIndex, result_imputeOptions);
    
    // ---------- result$columnCategories ---------------------------------------------------------------------
    
    if (gTrace) CERR << "result$columnCategories" << endl;

    resultFieldIndex++;
    
    SET_STRING_ELT(result_names, resultFieldIndex, Rf_mkChar("columnCategories"));
    
    size_t numCategoryMaps = xCategoryMaps.size();
    
    SEXP result_columnCategories;
    PROTECT(result_columnCategories = Rf_allocVector(VECSXP, (int)numCategoryMaps));
    resultUnprotectCount++;
    
    vector<SEXP> s_columnCategories(numCategoryMaps);
    
    for (size_t k = 0; k < numCategoryMaps; k++) {
        int numCategories = (int)xCategoryMaps[k].countNamedCategories();
        
//        if (k < 10) CERR << "[" << k << "] " << colNames[k] << " " << numCategories << endl;
        
        PROTECT(s_columnCategories[k] = Rf_allocVector(STRSXP, numCategories));
        resultUnprotectCount++;
        
        for (int catIndex = 0; catIndex < numCategories; catIndex++) {
            string category = xCategoryMaps[k].getCategoryForIndex(catIndex);
            
            SET_STRING_ELT(s_columnCategories[k], catIndex, Rf_mkChar(category.c_str()));
        }
        
        SET_VECTOR_ELT(result_columnCategories, (int)k, s_columnCategories[k]);
    }
    
    SET_VECTOR_ELT(result, resultFieldIndex, result_columnCategories);

    // ---------- result$useNaCategory ---------------------------------------------------------------------
    
    if (gTrace) CERR << "result$useNaCategory" << endl;
    
    resultFieldIndex++;
    
    SET_STRING_ELT(result_names, resultFieldIndex, Rf_mkChar("useNaCategory"));
    
    SEXP result_useNaCategory;
    PROTECT(result_useNaCategory = Rf_allocVector(INTSXP, (int)numCategoryMaps));
    resultUnprotectCount++;
    
    int *useNaCategoryP = INTEGER(result_useNaCategory);
    
    for (size_t k = 0; k < numCategoryMaps; k++) {
        useNaCategoryP[k] = xCategoryMaps[k].getUseNaCategory();    
    }
    
    SET_VECTOR_ELT(result, resultFieldIndex, result_useNaCategory);
    
    // ---------- result$colNames ------------------------------------------------------------------
    
    if (gTrace) CERR << "entree_C" << endl;

    resultFieldIndex++;
    
    SET_STRING_ELT(result_names, resultFieldIndex, Rf_mkChar("colNames"));
    
    int numColNames = (int)colNames.size();
    
    SEXP result_colNames;
    PROTECT(result_colNames = Rf_allocVector(STRSXP, numColNames));
    resultUnprotectCount++;
    
    for (size_t k = 0; k < numColNames; k++) {
        SET_STRING_ELT(result_colNames, (int)k, Rf_mkChar(colNames[k].c_str()));
    }
    
    SET_VECTOR_ELT(result, resultFieldIndex, result_colNames);

    // ---------------------------------------------------------------------------------------------
    
	UNPROTECT(resultUnprotectCount);
    
    if (gTrace) CERR << "return " << resultUnprotectCount << endl;
    
	return(result);
}

// =================================================================================================

// call from R to predict response from model and attributes
SEXP entree_predict_C(SEXP object, SEXP x)
{
    // --------------- verify arg types ---------------

    if (gTrace) CERR << "verify arg types" << endl;

    if (!Rf_isVectorList(object)) {
        error("entree_predict_C: wrong object type");
        
    } else if (!Rf_isVectorList(x)) {
        error("entree_predict_C: wrong x type");
    }
    
    // --------------- verify object class ---------------
    
    if (gTrace) CERR << "verify object class" << endl;
    
    {
        SEXP objectClass;
        PROTECT(objectClass = Rf_getAttrib(object, R_ClassSymbol));
        
        if (!Rf_isNull(objectClass) && strcmp("entree", CHAR(STRING_ELT(objectClass, 0))) != 0) {
            error("entree_predict_C: wrong object class");
        }
        
        UNPROTECT(1);
    }
    
    // --------------- verify x is a data.frame ---------------

    if (gTrace) CERR << "verify x is a data.frame" << endl;
    
    {
        SEXP xClass;
        PROTECT(xClass = Rf_getAttrib(x, R_ClassSymbol));
        
        if (!Rf_isNull(xClass) && strcmp("data.frame", CHAR(STRING_ELT(xClass, 0))) != 0) {
            error("entree_predict_C: wrong x class");
        }
        
        UNPROTECT(1);
    }
    
    // --------------- unpack object ---------------
    
    if (gTrace) CERR << "unpack object" << endl;
    
    vector<CompactTree> trees;
    index_t xColumnCount;
    index_t columnsPerTree;
    vector<ValueType> valueTypes;
    vector<ImputeOption> imputeOptions;
    vector<CategoryMaps> categoryMaps;
    vector<string> colNames;
    
    int resultFieldIndex = -1;

    // order here must match order in entree_C
    
    // ---------- object$trees ---------------------------------------------------------------------
    
    if (gTrace) CERR << "object$trees" << endl;
    
    resultFieldIndex++;
    
    SEXP object_trees = VECTOR_ELT(object, resultFieldIndex);

    size_t numTrees = (size_t)Rf_length(object_trees);

    for (size_t treeIndex = 0; treeIndex < numTrees; treeIndex++) {
        trees.push_back(CompactTree());

        SEXP s_tree = VECTOR_ELT(object_trees, (int)treeIndex);
        
        SEXP s_splitColIndex = VECTOR_ELT(s_tree, 0);
        SEXP s_lessOrEqualIndex = VECTOR_ELT(s_tree, 1);
        SEXP s_greaterOrNotIndex = VECTOR_ELT(s_tree, 2);
        SEXP s_toLessOrEqualIfNA = VECTOR_ELT(s_tree, 3);
        SEXP s_value_switch = VECTOR_ELT(s_tree, 4);
        SEXP s_value_d = VECTOR_ELT(s_tree, 5);
        SEXP s_value_i = VECTOR_ELT(s_tree, 6);
        
        size_t numNodes = (size_t)Rf_length(s_splitColIndex);
        
        int *splitColIndex = INTEGER(s_splitColIndex);
        int *lessOrEqualIndex = INTEGER(s_lessOrEqualIndex);
        int *greaterOrNotIndex = INTEGER(s_greaterOrNotIndex);
        int *toLessOrEqualIfNA = INTEGER(s_toLessOrEqualIfNA);
        int *value_switch = INTEGER(s_value_switch);
        double *value_d = REAL(s_value_d);
        int *value_i = INTEGER(s_value_i);
        
        CompactTree& tree = trees[treeIndex];
        tree.splitColIndex.resize(numNodes);
        tree.lessOrEqualIndex.resize(numNodes);
        tree.greaterOrNotIndex.resize(numNodes);
        tree.toLessOrEqualIfNA.resize(numNodes);
        tree.value.resize(numNodes);
        
        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++) {
            tree.splitColIndex[nodeIndex] = splitColIndex[nodeIndex];
            tree.lessOrEqualIndex[nodeIndex] = lessOrEqualIndex[nodeIndex];
            tree.greaterOrNotIndex[nodeIndex] = greaterOrNotIndex[nodeIndex];
            tree.toLessOrEqualIfNA[nodeIndex] = toLessOrEqualIfNA[nodeIndex];
            
            switch(value_switch[nodeIndex]) {
                case kCategorical:
                    tree.value[nodeIndex].i = value_i[nodeIndex];
                    break;
                    
                case kNumeric:
                    tree.value[nodeIndex].d = value_d[nodeIndex];
                    break;
                    
                default:
                    RUNTIME_ERROR_IF(true, "corrupt object");
                    break;
            }
        }
    }
    
    // ---------- object$xColumnCount ---------------------------------------------------------------------
    
    if (gTrace) CERR << "object$xColumnCount" << endl;
    
    resultFieldIndex++;
    
    SEXP object_xColumnCount = VECTOR_ELT(object, resultFieldIndex);
    
    xColumnCount = *INTEGER(object_xColumnCount);
    
    // ---------- object$selectColumns ---------------------------------------------------------------------
    
    if (gTrace) CERR << "object$selectColumns" << endl;
    
    resultFieldIndex++;

    SEXP object_selectColumns = VECTOR_ELT(object, resultFieldIndex);

    size_t numSelectColumns = (size_t)Rf_length(object_selectColumns);
    
    SelectIndexes selectColumns((size_t)xColumnCount + 1, false);

    int *selectColumnsP = INTEGER(object_selectColumns);
    
    for (size_t k = 0; k < numSelectColumns; k++) {
        selectColumns.select((size_t)selectColumnsP[k]);    
    }
    
    // ---------- object$columnsPerTree ---------------------------------------------------------------------
    
    if (gTrace) CERR << "object$columnsPerTree" << endl;
    
    resultFieldIndex++;
    
    SEXP object_columnsPerTree = VECTOR_ELT(object, resultFieldIndex);
    
    columnsPerTree = *INTEGER(object_columnsPerTree);
    
    // ---------- object$valueTypes ---------------------------------------------------------------------
    
    if (gTrace) CERR << "object$valueTypes" << endl;
    
    resultFieldIndex++;
    
    SEXP object_valueTypes = VECTOR_ELT(object, resultFieldIndex);

    for (int k = 0; k < Rf_length(object_valueTypes); k++) {
        string nextValueType = CHAR(STRING_ELT(object_valueTypes, k));

        ValueType valueType = stringToValueType(nextValueType);
        
        valueTypes.push_back(valueType);
    }
    
    // ---------- object$imputeOptions ---------------------------------------------------------------------
    
    if (gTrace) CERR << "object$imputeOptions" << endl;
    
    resultFieldIndex++;
    
    SEXP object_imputeOptions = VECTOR_ELT(object, resultFieldIndex);

    for (int k = 0; k < Rf_length(object_imputeOptions); k++) {
        string nextImputeOption = CHAR(STRING_ELT(object_imputeOptions, k));
        
        ImputeOption imputeOption = stringToImputeOption(nextImputeOption, valueTypes[(size_t)k]);

        imputeOptions.push_back(imputeOption);    
    }
    
    // ---------- object$columnCategories ---------------------------------------------------------------------
    
    if (gTrace) CERR << "object$columnCategories" << endl;
    
    resultFieldIndex++;
    
    SEXP object_columnCategories = VECTOR_ELT(object, resultFieldIndex);

    size_t numCols = (size_t)Rf_length(object_columnCategories);
    
    for (size_t col = 0; col < numCols; col++) {
        categoryMaps.push_back(CategoryMaps());
        
        SEXP s_categories = VECTOR_ELT(object_columnCategories, (int)col);
        
        int numCategories = Rf_length(s_categories);
        
        for (int catIndex = 0; catIndex < numCategories; catIndex++) {
            string nextCategory = CHAR(STRING_ELT(s_categories, catIndex));
            
            categoryMaps[col].insertCategory(nextCategory);
        }
    }

    // ---------- object$useNaCategory ---------------------------------------------------------------------
    
    if (gTrace) CERR << "object$useNaCategory" << endl;
    
    resultFieldIndex++;
    
    SEXP object_useNaCategory = VECTOR_ELT(object, resultFieldIndex);
    
    int *useNaCategoryP = INTEGER(object_useNaCategory);
    
    size_t numUseNaCategory = (size_t)Rf_length(object_useNaCategory);
    
    for (size_t k = 0; k < numUseNaCategory; k++) {
        categoryMaps[k].setUseNaCategory((bool)useNaCategoryP[k]);
    }
    
    // ---------- object$colNames ------------------------------------------------------------------
    
    if (gTrace) CERR << "object$colNames" << endl;
    
    resultFieldIndex++;
    
    SEXP object_colNames = VECTOR_ELT(object, resultFieldIndex);

    for (int k = 0; k < Rf_length(object_colNames); k++) {
        string nextColName = CHAR(STRING_ELT(object_colNames, k));
        
        colNames.push_back(nextColName);    
    }

    // --------------- convert x ---------------
    
    if (gTrace) {
        for (size_t k = 0; k < xColumnCount + 1; k++) {
            char c = '?';
            if (valueTypes[k] == kCategorical) {
                c = 'C';
                
            } else if (valueTypes[k] == kNumeric) {
                c = 'N';
            }
            
            CERR << k << "] " << c << " " << colNames[k] << endl;
        }
    }
    
    if (gTrace) CERR << "convert x" << endl;
    
    vector< vector<Value> > values;
    
    size_t xCols = (size_t)Rf_length(x);
    
    size_t xRows = 0;
    if (xCols > 0) {
        xRows = (size_t)Rf_length(VECTOR_ELT(x, 0));
    }
    
    for (size_t col = 0; col < xCols; col++) {
        //        CERR << "col = " << col << endl;
        
        values.push_back(vector<Value>());
        
        convertRVector(VECTOR_ELT(x, (int)col), xRows, true, valueTypes[col], values[col], true,
                       categoryMaps[col], colNames[col]);
    }
    
    // --------------- add column for prediction ---------------
    
    if (gTrace) CERR << "add column for prediction" << endl;
    
    size_t targetColumn = xCols;
    
    values.push_back(vector<Value>(xRows, gNaValue));
    
    // --------------- verify dimensions match ---------------
    
    if (gTrace) CERR << "verify dimensions and value types match" << endl;
    
    RUNTIME_ERROR_IF(valueTypes.size() != xCols + 1, "x dimensions do not match training");

    // --------------- predict ---------------

    if (gTrace) CERR << "predict" << endl;
    
    SelectIndexes selectRows(xRows, true);

    predict(values, valueTypes, categoryMaps, targetColumn, selectRows, selectColumns, trees, colNames);

    // --------------- package prediction results ---------------
    
    if (gTrace) CERR << "package prediction results" << endl;
    
    int unprotectCount = 0;
	SEXP result;
    
    switch (valueTypes[targetColumn]) {
        case kCategorical:
        {
            PROTECT(result = Rf_allocVector(INTSXP, (int)xRows));
            unprotectCount++;
            
            int *int_result = INTEGER(result);
            
            for (size_t row = 0; row < xRows; row++) {
                if (values[targetColumn][row].na) {
                    int_result[row] = NA_INTEGER;
                    
                } else {
                    int_result[row] = (int)values[targetColumn][row].number.i + 1; // convert to 1-based
                }
            }
            
            SEXP className;
            PROTECT(className = Rf_allocVector(STRSXP, 1));
            unprotectCount++;

            SET_STRING_ELT(className, 0, Rf_mkChar("factor"));
            Rf_classgets(result, className);
            
            size_t numCategories = categoryMaps[targetColumn].countNamedCategories();

            SEXP levels;
            PROTECT(levels = Rf_allocVector(STRSXP, (int)numCategories));
            unprotectCount++;
            
            for (index_t k = 0; k < numCategories; k++) {
                string category = categoryMaps[targetColumn].getCategoryForIndex(k);
                
                SET_STRING_ELT(levels, (int)k, Rf_mkChar(category.c_str()));
            }
            
            Rf_setAttrib(result, R_LevelsSymbol, levels);

        }
            break;
            
        case kNumeric:
        {
            PROTECT(result = Rf_allocVector(REALSXP, (int)xRows));
            unprotectCount++;

            double *real_result = REAL(result);
            
            for (size_t row = 0; row < xRows; row++) {
                if (values[targetColumn][row].na) {
                    real_result[row] = NA_REAL;
                    
                } else {
                    real_result[row] = values[targetColumn][row].number.d;
                }
            }
            
        }
            break;
    }
    
	UNPROTECT(unprotectCount);
        
	return(result);
}

// ========== Local Functions ======================================================================

// convert R vector to vector of Value; update valueType and categoryMaps if requested
void convertRVector(SEXP rVector,
                    size_t rowCount,
                    bool constValueType,
                    ValueType& valueType,
                    vector<Value>& values,
                    bool constCategoryMaps,
                    CategoryMaps& categoryMaps,
                    const string& name)
{
    values.clear();
    values.reserve(rowCount);
    
    if (Rf_isString(rVector)) {
        if (gTrace) CERR << name << " " << "isString" << endl;
        if (constValueType && valueType != kCategorical) {
            ostringstream oss;
            oss << "column type for '" << name << "' does not match input";

            RUNTIME_ERROR_IF(true, oss.str());
        }
        
        valueType = kCategorical;
        
        for (int row = 0; row < rowCount; row++) {
            Value nextValue;
            nextValue.na = STRING_ELT(rVector, row) == NA_STRING;
            
            if (nextValue.na) {
                nextValue.number.i = 0;
                
            } else {
                string str = CHAR(STRING_ELT(rVector, row));
                
                index_t index;
                
                if (categoryMaps.findIndexForCategory(str, index)) {
                    nextValue.number.i = index;
                    
                } else if (constCategoryMaps) {
                    nextValue.na = true;
                    
                } else {
                    nextValue.number.i = categoryMaps.insertCategory(str);
                }
            }
            
            values.push_back(nextValue);
        }
        
    } else if (Rf_isInteger(rVector)) {
        if (gTrace) CERR << name << " " << "isInteger" << endl;
        if (constValueType && valueType == kCategorical) {
            for (int row = 0; row < rowCount; row++) {
                Value nextValue;
                nextValue.na = INTEGER(rVector)[row] == NA_INTEGER;
                
                if (nextValue.na) {
                    nextValue.number.i = 0;
                    
                } else {
                    ostringstream oss;
                    oss << INTEGER(rVector)[row];
                    string str = oss.str();
 
                    index_t index;
                    
                    if (categoryMaps.findIndexForCategory(str, index)) {
                        nextValue.number.i = index;
                        
                    } else if (constCategoryMaps) {
                        nextValue.na = true;
                        
                    } else {
                        nextValue.number.i = categoryMaps.insertCategory(str);
                    }
                }
                
                values.push_back(nextValue);
            }
            
        } else {
            valueType = kNumeric;
            
            for (int row = 0; row < rowCount; row++) {
                Value nextValue;
                nextValue.number.d = INTEGER(rVector)[row];
                nextValue.na = INTEGER(rVector)[row] == NA_INTEGER;
                
                values.push_back(nextValue);
            }
        }
        
    } else if (Rf_isReal(rVector)) {
        if (gTrace) CERR << name << " " << "isReal" << endl;
        if (constValueType && valueType == kCategorical) {
            for (int row = 0; row < rowCount; row++) {
                Value nextValue;
                nextValue.na = ISNA(REAL(rVector)[row]);
                
                if (nextValue.na) {
                    nextValue.number.i = 0;
                    
                } else {
                    ostringstream oss;
                    oss << REAL(rVector)[row];
                    string str = oss.str();

                    index_t index;
                    
                    if (categoryMaps.findIndexForCategory(str, index)) {
                        nextValue.number.i = index;
                        
                    } else if (constCategoryMaps) {
                        nextValue.na = true;
                        
                    } else {
                        nextValue.number.i = categoryMaps.insertCategory(str);
                    }
                }
                
                values.push_back(nextValue);
            }
            
        } else {
            valueType = kNumeric;
            
            for (int row = 0; row < rowCount; row++) {
                Value nextValue;
                nextValue.number.d = REAL(rVector)[row];
                nextValue.na = ISNA(nextValue.number.d);
                
                values.push_back(nextValue);
            }
        }
        
    } else if (Rf_isFactor(rVector)) {
        if (gTrace) CERR << name << " " << "isFactor" << endl;
        
        if (constValueType && valueType != kCategorical) {
            ostringstream oss;
            oss << "column type for '" << name << "' does not match input";
            
            RUNTIME_ERROR_IF(true, oss.str());
        }
        
        // get map of factors
        map<int, string> levelMap;
        
        // convert levels to entries in categoryMaps
        int levelCount;
        {
            SEXP levels;
            PROTECT(levels = Rf_getAttrib(rVector, R_LevelsSymbol));
            
            levelCount = Rf_length(levels);
            for (int level = 0; level < levelCount; level++) {
                string str = CHAR(STRING_ELT(levels, level));
                
                // R levels are 1-based
                levelMap.insert(make_pair(level + 1, str));
            }
            
            UNPROTECT(1);
        }
        
        valueType = kCategorical;
        
        for (int row = 0; row < rowCount; row++) {
            int level = INTEGER(rVector)[row];

            Value nextValue;
            
            if (level == NA_INTEGER) {
                nextValue = gNaValue;
                
            } else {
                map<int, string>::const_iterator iter = levelMap.find(level);
                RUNTIME_ERROR_IF(iter == levelMap.end(), "broken levels list");
                
                string category = iter->second;
                
                if (constCategoryMaps) {
                    // if not found in existing categoryMaps, use NA
                    if (categoryMaps.findIndexForCategory(category, nextValue.number.i)) {
                        nextValue.na = false;
                    }
                    
                } else {
                    // create entry in categoryMaps if necessary
                    nextValue.number.i = categoryMaps.findOrInsertCategory(category);
                    nextValue.na = false;
                }
            }
            
            values.push_back(nextValue);
        }
        
    } else {
        CERR << "dunno" << endl;
        RUNTIME_ERROR_IF(true, "unrecognized column type");
    }
    
}

#endif
