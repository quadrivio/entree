//
//  subsets.cpp
//  entree
//
//  Created by MPB on 5/7/13.
//  Copyright (c) 2013 Quadrivio Corporation. All rights reserved.
//  License http://opensource.org/licenses/BSD-2-Clause
//          <YEAR> = 2013
//          <OWNER> = Quadrivio Corporation
//

//
// Create subsets of attribute columns to be used in individual decision trees in an ensemble
//

#include "subsets.h"

#include "utils.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <set>

using namespace std;

// ========== Local Headers ========================================================================

// for debugging; print list of subsets
void printSubsets(size_t columnCount, vector< vector<size_t> >& subsets);

// recursively list all combinations of k items chosen from n items
void iterateCombinations(size_t n,
                         size_t k,
                         index_t append,
                         vector< vector<size_t> >& combinations,
                         size_t& count,
                         index_t limit);

// for debugging; print list of combination
void printCombinations(const vector< vector<size_t> >& combinations);

// number of possible combinations of k items chosen from n items
double nChooseK(size_t n, size_t k);

// compare usage pairs (column index, usage count); sort by ascending order of usage, break ties by
// ascending column index
bool compareUsagePair(const pair<size_t, size_t>& i, const pair<size_t, size_t>& j);

// ========== Globals ========================================================================

namespace ns_train {
    // for debugging
    extern bool gVerbose;    
    extern bool gVerbose1;    
    extern bool gVerbose2;    
    extern bool gVerbose3; 
    extern bool gVerbose4; 
};

using namespace ns_train;

// ========== Functions ============================================================================

// make up to maxSubsets of columns, with column numbers in the range (0 to columnCount - 1),
// where each subset contains columnsPerSubset columns; subsets are generated by ordered
// deterministic permutations of small sets of column indexes
void makeSelectColSubsets(size_t columnsCount,
                          size_t columnsPerSubset,
                          index_t maxSubsets,
                          vector< vector<size_t> >& subsets)
{
    subsets.clear();
    
    LOGIC_ERROR_IF(columnsPerSubset > columnsCount,
                   "makeSelectColSubsets: columnsPerSubset > columnsCount")
    
    if (gVerbose3) {
        CERR << "makeSelectColSubsets(columnsCount = " << columnsCount <<
        ", columnsPerSubset = " << columnsPerSubset <<
        ", maxSubsets = " << maxSubsets << ")" << endl;
    }
    
    // the full set of columns is divided into groups, then to generate the subsets, combinations
    // of these groups are chosen
    
    // if the columnsCount is not evenly divisible by the group size, there will be a number of
    // full-sized groups, plus one additional short group
    
    // first step is to figure out the correct number of columns per group; we want smallest
    // groups, so as to best cover the space of possible subsets, consist with not exceeding
    // maxSubsets
    
    // begin with group exactly equal to columnsPerSubset, so each subset consists of exactly
    // one full-sized group (or of columns chosen from the short group plus a full-sized group)
    
    // values for next trial
    size_t columnsPerFullGroupNext = columnsPerSubset;
    size_t nFullGroupsNext = columnsCount / columnsPerFullGroupNext;
    size_t columnsPerShortGroupNext = columnsCount - nFullGroupsNext * columnsPerFullGroupNext;
    size_t kChooseNext = 1;
    bool specialCaseShortGroupNext = columnsPerShortGroupNext != 0;
    
    // values for most-recent sucessful trial
    size_t nFullGroups = nFullGroupsNext;
    size_t columnsPerFullGroup = columnsPerFullGroupNext;
    size_t columnsPerShortGroup = columnsPerShortGroupNext;
    size_t kChoose = kChooseNext;
    bool specialCaseShortGroup = specialCaseShortGroupNext;
    
    if (gVerbose3) {
        if (specialCaseShortGroup) {
            CERR << "* " << kChooseNext << " from " << nFullGroupsNext <<
            " [+1] -> " << 2 * nFullGroups <<
            " combinations (" << columnsPerFullGroupNext <<
            " [" << columnsPerShortGroupNext << "]" <<
            " column(s) per group)" << endl;
            
        } else if (columnsPerShortGroupNext != 0) {
            CERR << "* " << kChooseNext << " from " << nFullGroupsNext <<
            " + 1 -> " << nFullGroups + 1 <<
            " combinations (" << columnsPerFullGroupNext <<
            " [" << columnsPerShortGroupNext << "]" <<
            " column(s) per group)" << endl;
            
        } else {
            CERR << "* " << kChooseNext << " from " << nFullGroupsNext <<
            " -> " << nFullGroups <<
            " combinations (" << columnsPerFullGroupNext <<
            " column(s) per group)" << endl;
        }
    }
    
    bool done = columnsPerFullGroupNext <= 1;
    
    while (!done) {
        // next trial; reduce group size by 1, see if number of combinations is <= maxSubsets
        
        columnsPerFullGroupNext--;
        nFullGroupsNext = columnsCount / columnsPerFullGroupNext;
        columnsPerShortGroupNext = columnsCount - nFullGroupsNext * columnsPerFullGroupNext;
        kChooseNext = (columnsPerSubset + columnsPerFullGroupNext - 1) / columnsPerFullGroupNext;
        
        // special case is when there is a short group and (kChooseNext - 1) full groups plus the
        // short group does not contain enough columns to cover columnsPerSubset; this is handled
        // by iterating over the full groups, and then repeating the iteration with the columns
        // from the short group added
        
        specialCaseShortGroupNext = columnsPerShortGroupNext != 0 &&
        (kChooseNext - 1) * columnsPerFullGroupNext + columnsPerShortGroupNext < columnsPerSubset;
        
        index_t comboCount;
        if (specialCaseShortGroupNext) {
            // iterations with and without the short group
            comboCount = 2 * nChooseK(nFullGroupsNext, kChooseNext);
            
        } else if (columnsPerShortGroupNext != 0) {
            // can treat short group same as full group
            comboCount = nChooseK(nFullGroupsNext + 1, kChooseNext);
            
        } else {
            // no short group; iterate over full groups
            comboCount = nChooseK(nFullGroupsNext, kChooseNext);   
        }
        
        bool limitOK = comboCount <= maxSubsets;
        
        if (gVerbose3) {
            if (specialCaseShortGroupNext) {
                CERR << (limitOK ? "* " : "  ") << kChooseNext << " from " << nFullGroupsNext <<
                " [+1] -> " << comboCount <<
                " combinations (" << columnsPerFullGroupNext <<
                " [" << columnsPerShortGroupNext << "]" <<
                " column(s) per group)" << endl;
                
            } else if (columnsPerShortGroupNext != 0) {
                CERR << (limitOK ? "* " : "  ") << kChooseNext << " from " << nFullGroupsNext <<
                " + 1 -> " << comboCount <<
                " combinations (" << columnsPerFullGroupNext <<
                " [" << columnsPerShortGroupNext << "]" <<
                " column(s) per group)" << endl;
                
            } else {
                CERR << (limitOK ? "* " : "  ") << kChooseNext << " from " << nFullGroupsNext <<
                " -> " << comboCount <<
                " combinations (" << columnsPerFullGroupNext <<
                " column(s) per group)" << endl;
            }
        }
        
        if (limitOK) {
            // this trial is within limit; save trial as successful
            nFullGroups = nFullGroupsNext;
            columnsPerFullGroup = columnsPerFullGroupNext;
            columnsPerShortGroup = columnsPerShortGroupNext;
            kChoose = kChooseNext;
            specialCaseShortGroup = specialCaseShortGroupNext;
        }
        
        if (columnsPerFullGroup == 1 || comboCount >= maxSubsets) {
            // reached minimum group size or exceeded limit
            done = true;
        }
        
        if (gVerbose4) {
            CERR << "next loop" << endl;
        }
        
    }
    
    if (gVerbose4) {
        CERR << "done first loop" << endl;
    }
    
    size_t count = 0;
    vector< vector<size_t> > combinations;
    
    // next step, collect iterations
    
    if (specialCaseShortGroup) {
        if (gVerbose3) {
            CERR << "columnsCount = " << columnsCount << ", nFullGroups = " << nFullGroups <<
            "[+1], columnsPerFullGroup = " << columnsPerFullGroup <<
            " [" << columnsPerShortGroup << "]" << endl;
        }
        
        iterateCombinations(nFullGroups, kChoose, NO_INDEX, combinations, count, maxSubsets);
        iterateCombinations(nFullGroups, kChoose, (index_t)nFullGroups, combinations, count,
                            maxSubsets);
        
    } else if (columnsPerShortGroup != 0) {
        if (gVerbose3) {
            CERR << "columnsCount = " << columnsCount << ", nFullGroups = " << nFullGroups <<
            " + 1, columnsPerFullGroup = " << columnsPerFullGroup <<
            " [" << columnsPerShortGroup << "]" << endl;
        }
        
        iterateCombinations(nFullGroups + 1, kChoose, NO_INDEX, combinations, count, maxSubsets);
        
    } else {
        if (gVerbose3) {
            CERR << "columnsCount = " << columnsCount << ", nFullGroups = " << nFullGroups <<
            ", columnsPerFullGroup = " << columnsPerFullGroup << endl;
        }
        
        iterateCombinations(nFullGroups, kChoose, NO_INDEX, combinations, count, maxSubsets);
    }
    
    if (gVerbose2) {
        CERR << "printCombinations:" << endl;
        printCombinations(combinations);
    }
    
    // the combinations may contain more columns than columnsPerSubset, since columnsPerSubset may
    // not be evenly divisable by the group size(s); if so, then need to select columns to use -
    // this is done by iterating through combinations, and for each one, select the columnsPerSubset
    // least-used columns available in the combination 
    
    // construct vector of usage-pairs: each pair has column number as first value and accumulated
    // use count as second value (initialized to zero)
    vector< pair<size_t, size_t> > usagePairs(columnsCount);
    for (size_t k = 0; k < columnsCount; k++) {
        usagePairs[k] = make_pair(k, 0);
    }
    
    for (size_t k = 0; k < combinations.size(); k++) {
        // for each combination...
        
        if (gVerbose2) CERR << k << "] ";
        
        // build vector of the usage pairs for all columns available in this combination
        vector< pair<size_t, size_t> > nextUsagePairs;
        
        for (size_t j = 0; j < combinations[k].size(); j++) {
            size_t groupIndex = combinations[k][j];
            if (gVerbose2) CERR << "(" << groupIndex << ") ";
            
            for (size_t colIndex = groupIndex * columnsPerFullGroup;
                 colIndex < (groupIndex + 1) * columnsPerFullGroup && colIndex < columnsCount;
                 colIndex++) {
                
                nextUsagePairs.push_back(usagePairs[colIndex]);
                if (gVerbose2) CERR << colIndex % columnsCount << " ";
            }
        }
        
        if (gVerbose2) CERR << endl;
        
        // sort them in increasing order of usage; break ties by using column number
        sort(nextUsagePairs.begin(), nextUsagePairs.end(), compareUsagePair);
        
        LOGIC_ERROR_IF(nextUsagePairs.size() < columnsPerSubset, "broken");
        
        // pick the top columnsPerSubset entries to use in this subset; increment usage counts for
        // the columns that were picked
        vector<size_t> nextSubset;
        for (size_t j = 0; j < columnsPerSubset; j++) {
            size_t index = nextUsagePairs[j].first;
            nextSubset.push_back(index);
            usagePairs[index].second++;
        }
        
        // save subset
        subsets.push_back(nextSubset);
    }
    
    //    if (count < maxSubsets && nGroups < nGroupsNext) {
    //        CERR << "try for " << maxSubsets - count << " more" << endl;
    //    }
}

// ========== Local Functions ======================================================================

// for debugging; print list of subsets
void printSubsets(size_t columnCount, vector< vector<size_t> >& subsets)
{
    
    for (size_t k = 0; k < subsets.size(); k++) {
        set<size_t> subset;
        for (size_t j = 0; j < subsets[k].size(); j++) {
            subset.insert(subsets[k][j]);
        }
        
        for (size_t i = 0; i < columnCount; i++) {
            if (i > 0) CERR << " ";
            CERR << (int)(subset.find(i) != subset.end());
        }
        CERR << endl;
    }
}

// compare usage pairs (column index, usage count); sort by ascending order of usage, break ties by
// ascending column index
bool compareUsagePair(const pair<size_t, size_t>& i, const pair<size_t, size_t>& j)
{
    bool result;
    
    if (i.second < j.second) {
        result = true;
        
    } else if (i.second > j.second) {
        result = false;
        
    } else if (i.first < j.first) {
        result = true;
        
    } else {
        result = false;
    }
    
    return result;
}

// for debugging; print list of combination
void printCombinations(const vector< vector<size_t> >& combinations)
{
    for (size_t k = 0; k < combinations.size(); k++) {
        bool first = true;
        for (vector<size_t>::const_iterator iter = combinations[k].begin();
             iter != combinations[k].end();
             iter++) {
            
            if (!first) {
                CERR << ", ";
            } else {
                first = false;
            }
            
            CERR << *iter;
        }
        CERR << endl;
    }
}

// number of possible combinations of k items chosen from n items
double nChooseK(size_t n, size_t k)
{
    //          n!
    //      -----------
    //      k! (n - k)!
    
    if (n - k > k) {
        k = n - k;
    }
    
    double result = 1.0;
    for (size_t i = n; i > k; i--) {
        result *= i;
    }
    
    for (size_t i = n - k; i > 1; i--) {
        result /= i;
    }
    
    return result;
}

// recursively list all combinations of k items chosen from n items
// call initially:
//      size_t count = 0;       // initialize to zero
//      index_t limit = 10000;  // big number to prevent runaway
//
//      vector< vector<size_t> > combinations; // empty vector, to be filled in
//
//      iterateCombinations(n, k, NO_INDEX, combinations, count, limit);
//
void iterateCombinations(size_t n,
                         size_t k,
                         index_t append,
                         vector< vector<size_t> >& combinations,
                         size_t& count,
                         index_t limit)
{
    vector< vector<size_t> > nextCombinations;
    
    if (n == 0) {
        // done
        
    } else if (k == 0) {
        // choose no items
        vector<size_t> combination;
        
        if (limit == NO_INDEX || (index_t)count < limit) {
            nextCombinations.push_back(combination);
            count++;
        }
        
    } else if (n == k) {
        // choose all items
        vector<size_t> combination;
        for (size_t i = 0; i < k; i++) {
            combination.push_back(i);
        }
        
        if (limit == NO_INDEX || (index_t)count < limit) {
            nextCombinations.push_back(combination);
            count++;
        }
        
    } else {
        // recursion happens here
        
        // add all combinations of size k that don't contain last index
        if (limit == NO_INDEX || (index_t)count < limit) {
            iterateCombinations(n - 1, k, NO_INDEX, nextCombinations, count, limit); 
        }
        
        // add all combinations of size k that do contain last index, by adding all combinations
        // of size k - 1 that don't contain last index, then appending last index to them
        if (limit == NO_INDEX || (index_t)count < limit) {
            iterateCombinations(n - 1, k - 1, (index_t)n - 1, nextCombinations, count, limit); 
        }
    }
    
    if (append != NO_INDEX) {
        // append specified index to each combination generated in this pass
        for (size_t i = 0; i < nextCombinations.size(); i++) {
            nextCombinations[i].push_back((size_t)append);
        }
    }
    
    // add all combinations generated in this pass
    combinations.insert(combinations.end(), nextCombinations.begin(), nextCombinations.end());
}

// ========== Tests ================================================================================

// component tests
void ctest_subsets(int& totalPassed, int& totalFailed, bool verbose)
{
    int passed = 0;
    int failed = 0;
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // makeSelectColSubsets
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // printSubsets
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // iterateCombinations
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // printCombinations
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // nChooseK
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // compareUsagePair
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    
    if (verbose) {
        CERR << "subset.cpp" << "\t" << passed << " passed, " << failed << " failed" << endl;
    }
    
    totalPassed += passed;
    totalFailed += failed;
}

// code coverage
void cover_subsets(bool verbose)
{
    // ~~~~~~~~~~~~~~~~~~~~~~
    // makeSelectColSubsets
    // iterateCombinations
    // nChooseK
    
    vector< vector<size_t> > subsets;
    
    makeSelectColSubsets(8, 6, 100, subsets);
    makeSelectColSubsets(11, 3, 100, subsets);
    makeSelectColSubsets(8, 6, 3, subsets);
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // printSubsets
    
    if (verbose) {
        printSubsets(8, subsets);
    }
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // printCombinations
    
    if (verbose) {
        size_t count = 0;
        vector< vector<size_t> > combinations;
        iterateCombinations(5, 3, NO_INDEX, combinations, count, 100);
        
        printCombinations(combinations);
    }
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // compareUsagePair
    
    pair<size_t, size_t> p1 = make_pair(1, 1);
    pair<size_t, size_t> p2 = make_pair(1, 2);
    pair<size_t, size_t> p3 = make_pair(2, 1);
    
    compareUsagePair(p1, p2);
    compareUsagePair(p2, p1);
    compareUsagePair(p1, p3);
    compareUsagePair(p3, p1);
}
