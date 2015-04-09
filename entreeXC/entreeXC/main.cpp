//
//  main.cpp
//  entree
//
//  Created by MPB on 5/27/13.
//  Copyright (c) 2015 Quadrivio Corporation. All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted
// provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list of conditions
// and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice, this list of conditions
// and the following disclaimer in the documentation and/or other materials provided with the
// distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
// WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

//
// Interpret command-line arguments
//

#include "call.h"
#include "develop.h"
#include "test.h"

#include <iostream>
#include <stdexcept>

using namespace std;

void version();
void usage();

void version()
{
    cerr << "0.10-3" << endl;
}

int main (int argc, const char * argv[])
{
    // Arguments and (Flags):
    //
    //  -T  (train)
    //  -P  (predict)
    //
    //  -a  path to attributes csv file
    //  -r  path to response csv file
    //  -m  path to serialized model
    //  -y  path to value types csv file
    //  -i  path to impute options csv file
    //
    //  -c  columnsPerTree
    //  -d  maxDepth
    //  -l  minLeafCount
    //  -s  maxSplitsPerNumericAttribute
    //  -t  maxTrees
    //  -u  doPrune
    //  -e  minDepth
    //  -n  maxNodes
    //  -i  minImprovement
    //
    //  -v  verbose
    //
    //  --develop   (run development code)
    //  --test      (run test code)
    //  --version   (print version number)
    
    int status = 1;

    try {
        bool printUsage = argc <= 1; 
        bool developFlag = false;
        bool testFlag = false;
        bool trainFlag = false;
        bool predictFlag = false;
        bool verboseFlag = false;
        
        string columnsPerTree("");
        string maxDepth("");
        string minLeafCount("");
        string maxSplitsPerNumericAttribute("");
        string maxTrees("");
        string doPrune("");
        string minDepth("");
        string maxNodes("");
        string minImprovement("");
        
        string attributesFile("");
        string responseFile("");
        string modelFile("");
        string typeFile("");
        string imputeFile("");
        
        for (int index = 1; index < argc; index++) {
            if (strcmp(argv[index], "--version") == 0) {
                version(); 
                
            } else if (strcmp(argv[index], "--develop") == 0) {
                developFlag = true; 
                
            } else if (strcmp(argv[index], "--test") == 0) {
                testFlag = true;
                
            } else if (strcmp(argv[index], "-T") == 0) {
                trainFlag = true;
                
            } else if (strcmp(argv[index], "-P") == 0) {
                predictFlag = true;
                
            } else if (strcmp(argv[index], "-v") == 0) {
                verboseFlag = true;
                
            } else if (strcmp(argv[index], "-a") == 0 && index + 1 < argc) {
                attributesFile = argv[++index];
                
            } else if (strcmp(argv[index], "-r") == 0 && index + 1 < argc) {
                responseFile = argv[++index];
                
            } else if (strcmp(argv[index], "-m") == 0 && index + 1 < argc) {
                modelFile = argv[++index];
                
            } else if (strcmp(argv[index], "-y") == 0 && index + 1 < argc) {
                typeFile = argv[++index];
                
            } else if (strcmp(argv[index], "-i") == 0 && index + 1 < argc) {
                imputeFile = argv[++index];
                
            } else if (strcmp(argv[index], "-c") == 0 && index + 1 < argc) {
                columnsPerTree = argv[++index];
                
            } else if (strcmp(argv[index], "-d") == 0 && index + 1 < argc) {
                maxDepth = argv[++index];
                
            } else if (strcmp(argv[index], "-l") == 0 && index + 1 < argc) {
                minLeafCount = argv[++index];
                
            } else if (strcmp(argv[index], "-s") == 0 && index + 1 < argc) {
                maxSplitsPerNumericAttribute = argv[++index];
                
            } else if (strcmp(argv[index], "-t") == 0 && index + 1 < argc) {
                maxTrees = argv[++index];
                
            } else if (strcmp(argv[index], "-u") == 0 && index + 1 < argc) {
                doPrune = argv[++index];
                
            } else if (strcmp(argv[index], "-e") == 0 && index + 1 < argc) {
                minDepth = argv[++index];
                
            } else if (strcmp(argv[index], "-n") == 0 && index + 1 < argc) {
                maxNodes = argv[++index];
                
            } else if (strcmp(argv[index], "-i") == 0 && index + 1 < argc) {
                minImprovement = argv[++index];
                
            } else {
                printUsage = true;
            }
        }
        
        if (printUsage) {
            usage();
            
        } else if (developFlag) {
            develop();   
            
        } else if (testFlag) {
            test(verboseFlag);
            
        } else if (predictFlag) {
            callPredict(attributesFile, responseFile, modelFile);
            
        } else if (trainFlag) {
            callTrain(attributesFile, responseFile, modelFile, typeFile, imputeFile, columnsPerTree,
                      maxDepth, minLeafCount, maxSplitsPerNumericAttribute, maxTrees, doPrune,
                      minDepth, maxNodes, minImprovement);
        }
        
        status = 0;
        
    } catch (const logic_error& x) {
        std::cerr << "logic_error: " << x.what() << endl;
        
    } catch (const runtime_error& x) {
        std::cerr << "runtime_error: " << x.what() << endl;
        
    } catch (...) {
        std::cerr << "unknown error" << endl;
    }

    return status;
}

void usage()
{
    cerr <<
    "usage: entree [-T] [-P] [-a attributesFile] [-r responseFile]" << endl <<
    "              [-m modelFile] [-y typeFile] [-i imputeFile]" << endl <<
    "              [-c columnsPerTree] [-d maxDepth] [-l minLeafCount]" << endl <<
    "              [-s maxSplitsPerNumericAttribute] [-t maxTrees]" << endl <<
    "              [-u prune] [-e minDepth] [-n maxNodes] [-i minImprovement]" << endl <<
    endl <<
    "  To train model, supply -T -a -r -m and optional parameters" << endl <<
    "  To predict from model, supply -P -a -m -r only" << endl;
}
