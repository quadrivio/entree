//
//  call.h
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

#ifndef entree_call_h
#define entree_call_h

#include <string>

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
               const std::string& minImprovementStr);

void callPredict(const std::string& attributesFile,
                 const std::string& responseFile,
                 const std::string& modelFile);

#endif
