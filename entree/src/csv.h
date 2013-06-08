//
//  csv.h
//  entree
//
//  Created by MPB on 5/13/13.
//  Copyright (c) 2013 Quadrivio Corporation. All rights reserved.
//  License http://opensource.org/licenses/BSD-2-Clause
//          <YEAR> = 2013
//          <OWNER> = Quadrivio Corporation
//

//
//  Utilities for reading and writing comma-separated-value (.csv) files and strings
//

#ifndef entree_csv_h
#define entree_csv_h

#include <istream>
#include <sstream>
#include <string>
#include <vector>

// ========== Function Headers =====================================================================

// read csv data from stream; return cells, column names, and whether cells are quoted
void readCsv(std::istream& is,
             bool readHeader,
             std::vector< std::vector<std::string> >& cells,
             std::vector< std::vector<bool> >& quoted,
             std::vector<std::string>& colNames);

// write csv data to stream
void writeCsv(std::ostream& os,
              bool writeHeader,
              const std::vector< std::vector<std::string> >& cells,
              const std::vector< std::vector<bool> >& quoted,
              const std::vector<std::string>& colNames);

// read csv file with header row; return cells, column names, and whether cells are quoted
void readCsvPath(const std::string& path,
                 std::vector< std::vector<std::string> >& cells,
                 std::vector< std::vector<bool> >& quoted,
                 std::vector<std::string>& colNames);

// read csv file without header row; return cells, and whether cells are quoted
void readCsvPath(const std::string& path,
                 std::vector< std::vector<std::string> >& cells,
                 std::vector< std::vector<bool> >& quoted);

// read csv string with header row; return cells, column names, and whether cells are quoted
void readCsvString(const std::string& csvString,
                   std::vector< std::vector<std::string> >& cells,
                   std::vector< std::vector<bool> >& quoted,
                   std::vector<std::string>& colNames);

// read csv file without header row; return cells, and whether cells are quoted
void readCsvString(const std::string& csvString,
                   std::vector< std::vector<std::string> >& cells,
                   std::vector< std::vector<bool> >& quoted);

// for debugging or logging; print cells and column names to CERR
void printCells(const std::vector< std::vector<std::string> >& cells,
                const std::vector< std::vector<bool> >& quoted,
                const std::vector<std::string>& colNames);

// for debugging or logging; print cells to CERR
void printCells(const std::vector< std::vector<std::string> >& cells,
                const std::vector< std::vector<bool> >& quoted);

// write data and header row to csv file
void writeCsvPath(const std::string& path,
                  const std::vector< std::vector<std::string> >& cells,
                  const std::vector< std::vector<bool> >& quoted,
                  const std::vector<std::string>& colNames);

// write data to csv file
void writeCsvPath(const std::string& path,
                  const std::vector< std::vector<std::string> >& cells,
                  const std::vector< std::vector<bool> >& quoted);

// check number of cells in each row; if lengths are all the same, return true, else false
bool uniformRowLengths(const std::vector< std::vector<std::string> >& cells);

// check number of cells in each row; if lengths are all the same as number of column names, return
// true, else false
bool uniformRowLengths(const std::vector< std::vector<std::string> >& cells,
                       const std::vector<std::string>& colNames);

// component tests
void ctest_csv(int& totalPassed, int& totalFailed, bool verbose);

// code coverage
void cover_csv(bool verbose);

#endif
