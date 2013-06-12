//
//  entree.h
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

#ifndef _entree_entree_h
#define _entree_entree_h

#ifndef RPACKAGE
#define RPACKAGE 1
#endif

#if RPACKAGE

// ---------- use this block to include R headers ----------
#ifndef NO_C_HEADERS
#define NO_C_HEADERS
#endif
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <cmath>
#include <iostream>
#include <sstream>
#include <R.h>
#define R_NO_REMAP
#include <Rinternals.h>
// ---------------------------------------------------------

// ========== Function Headers =====================================================================

extern "C" {

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
                  SEXP s_xImputeOptions);
    
    // call from R to predict response from model and attributes
	SEXP entree_predict_C(SEXP object, SEXP x);
	
}

#endif
#endif
