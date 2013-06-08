//
//  develop.cpp
//  entree
//
//  Created by MPB on 5/29/13.
//  Copyright (c) 2013 Quadrivio Corporation. All rights reserved.
//  License http://opensource.org/licenses/BSD-2-Clause
//          <YEAR> = 2013
//          <OWNER> = Quadrivio Corporation
//

//
// Hook called when --develop argument is used
//

#include "develop.h"

#include "crime.h"
#include "csv.h"
#include "format.h"
#include "predict.h"
#include "shim.h"
#include "test.h"
#include "train.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace std;

void develop()
{
    time_t t;
    time(&t);
    CERR << localTimeString(t) << " start develop" << endl;
    
    // ad-hoc testing and debugging here
    
    time(&t);
    CERR << localTimeString(t) << " done develop" << endl;
}
