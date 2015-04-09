//
//  csv.cpp
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

//  read and write csv files and strings
//      * leading spaces and tabs within each comma-separated cell are ignored
//      * cell contents can be quoted with "
//      * quotes within cell are represented by ""
//      * newlines cannot be quoted
//      * reading is terminated by eof or blank line
//
//  no guarantee that each line has same number of cells; use uniformRowLengths to check
//

#include <fstream>  // must preceed .h includes

#include "csv.h"

#include "shim.h"
#include "utils.h"

#include <iostream>
#include <stdexcept>

using namespace std;

// ========== Local Headers ========================================================================

// read next line of csv from stream, parsed into cells, note whether cells quoted or not; blank
// line terminates reading; return true if more to read, false if done reading
bool nextCsvLine(std::istream& is,
                 std::vector<std::string>& line,
                 std::vector<bool>& lineQuoted);

// ========== Functions ============================================================================

// read csv data from stream; return cells, column names, and whether cells are quoted
void readCsv(std::istream& is,
             bool readHeader,
             std::vector< std::vector<std::string> >& cells,
             std::vector< std::vector<bool> >& quoted,
             std::vector<std::string>& colNames)
{
    cells.clear();
    quoted.clear();
    colNames.clear();
    
    vector<string> line;
    vector<bool> lineQuoted;
    
    if (readHeader) {
        nextCsvLine(is, colNames, lineQuoted);
    }
    
    while (nextCsvLine(is, line, lineQuoted)) {
        cells.push_back(line);
        quoted.push_back(lineQuoted);
    }
}

// -------------------------------------------------------------------------------------------------

// write csv data to stream
void writeCsv(std::ostream& os,
              bool writeHeader,
              const std::vector< std::vector<std::string> >& cells,
              const std::vector< std::vector<bool> >& quoted,
              const std::vector<std::string>& colNames)
{
    size_t numRows = cells.size();
    size_t numCols = cells[0].size();
    
    if (writeHeader) {
        for (size_t col = 0; col < colNames.size(); col++) {
            if (col > 0) {
                os << ",";
            }
            
            // quote column names in header
            os << '"';
            
            for (size_t k = 0; k < colNames[col].length(); k++) {
                if (colNames[col][k] == '"') {
                    // quotes within cell are output as double quote ""
                    os << '"' << '"';
                    
                } else {
                    os << colNames[col][k];
                }
            }
            
            os << '"';
        }
        
        os << endl;
    }
    
    for (size_t row = 0; row < numRows; row++) {
        for (size_t col = 0; col < numCols; col++) {
            if (col > 0) {
                os << ",";
            }
            
            bool needQuotes = quoted[row][col];
            
            for (size_t k = 0; k < cells[row][col].length() && !needQuotes; k++) {
                char c = cells[row][col][k];
                if ( ((c >= '0') && (c <= '9')) || c == '.') {
                    // no quote needed
                    
                } else {
                    // quote anything that isn't a number or period
                    needQuotes = true;
                }
            }
            
            if (needQuotes) os << '"';
            
            for (size_t k = 0; k < cells[row][col].length(); k++) {
                if (cells[row][col][k] == '"') {
                    // quotes within cell are output as double quote ""
                    os << '"' << '"';
                    
                } else {
                    os << cells[row][col][k];
                }
            }
            
            if (needQuotes) os << '"';
        }
        
        os << endl;
    }
}

// -------------------------------------------------------------------------------------------------

// write data and header row to csv file
void writeCsvPath(const std::string& path,
                  const std::vector< std::vector<std::string> >& cells,
                  const std::vector< std::vector<bool> >& quoted,
                  const std::vector<std::string>& colNames)
{
    ofstream ofs(path.c_str());
    
    RUNTIME_ERROR_IF(!ofs.good(), badPathErrorMessage(path));
    
    bool writeHeader = true;
    
    writeCsv(ofs, writeHeader, cells, quoted, colNames);   
    
    ofs.close();
    
}

// write data to csv file
void writeCsvPath(const std::string& path,
                  const std::vector< std::vector<std::string> >& cells,
                  const std::vector< std::vector<bool> >& quoted)
{
    fstream ofs(path.c_str());
    
    RUNTIME_ERROR_IF(!ofs.good(), badPathErrorMessage(path));
    
    bool writeHeader = false;
    std::vector<std::string> colNames;  // ignored
    
    writeCsv(ofs, writeHeader, cells, quoted, colNames);   
    
    ofs.close();
    
}

// -------------------------------------------------------------------------------------------------

// read csv file with header row; return cells, column names, and whether cells are quoted
void readCsvPath(const std::string& path,
                 std::vector< std::vector<std::string> >& cells,
                 std::vector< std::vector<bool> >& quoted,
                 std::vector<std::string>& colNames)
{
    ifstream ifs(path.c_str());
    
    RUNTIME_ERROR_IF(!ifs.good(), badPathErrorMessage(path));
    
    bool readHeader = true;
    
    readCsv(ifs, readHeader, cells, quoted, colNames) ;   
    
    ifs.close();
}

// read csv file without header row; return cells, and whether cells are quoted
void readCsvPath(const std::string& path,
                 std::vector< std::vector<std::string> >& cells,
                 std::vector< std::vector<bool> >& quoted)
{
    ifstream ifs(path.c_str());
    
    RUNTIME_ERROR_IF(!ifs.good(), badPathErrorMessage(path));
    
    bool readHeader = false;
    std::vector<std::string> colNames;  // ignored
    
    readCsv(ifs, readHeader, cells, quoted, colNames) ;   
    
    ifs.close();
}

// -------------------------------------------------------------------------------------------------

// read csv string with header row; return cells, column names, and whether cells are quoted
void readCsvString(const std::string& csvString,
                   std::vector< std::vector<std::string> >& cells,
                   std::vector< std::vector<bool> >& quoted,
                   std::vector<std::string>& colNames)
{
    istringstream iss(csvString);
    
    bool readHeader = true;
    
    readCsv(iss, readHeader, cells, quoted, colNames) ;   
}

// read csv file without header row; return cells, and whether cells are quoted
void readCsvString(const std::string& csvString,
                   std::vector< std::vector<std::string> >& cells,
                   std::vector< std::vector<bool> >& quoted)
{
    istringstream iss(csvString);
    
    bool readHeader = false;
    std::vector<std::string> colNames;  // ignored
    
    readCsv(iss, readHeader, cells, quoted, colNames) ;   
}

// -------------------------------------------------------------------------------------------------

// for debugging or logging; print cells and column names to CERR
void printCells(const std::vector< std::vector<std::string> >& cells,
                const std::vector< std::vector<bool> >& quoted,
                const std::vector<std::string>& colNames)
{
    size_t numRows = cells.size();
    
    if (colNames.size() > 0) {
        for (size_t col = 0; col < colNames.size(); col++) {
            if (col > 0) CERR << '\t';
            
            CERR << colNames[col];
        }
        
        CERR << endl;
        
        for (size_t col = 0; col < colNames.size(); col++) {
            if (col > 0) CERR << '\t';
            
            CERR << "-";
        }
        
        CERR << endl;
    }
    
    for (size_t row = 0; row < numRows; row++) {
        for (size_t col = 0; col < cells[row].size(); col++) {
            if (col > 0) CERR << '\t';
            
            if (quoted[row][col]) CERR << '"';
            
            CERR << cells[row][col];

            if (quoted[row][col]) CERR << '"';
        }
        
        CERR << endl;
    }
}

// for debugging or logging; print cells to CERR
void printCells(const std::vector< std::vector<std::string> >& cells,
                const std::vector< std::vector<bool> >& quoted)
{
    vector<string> colNames;
    
    printCells(cells, quoted, colNames);
}

// -------------------------------------------------------------------------------------------------

// check number of cells in each row; if lengths are all the same, return true, else false
bool uniformRowLengths(const std::vector< std::vector<std::string> >& cells)
{
    bool result = true;
    
    size_t numCols = 0;
    
    for (size_t row = 0; row < cells.size() && result; row++) {
        if (row == 0) {
            numCols = cells[row].size();
            
        } else if (numCols != cells[row].size()) {
            result = false;
        }
    }
    
    return result;
}

// check number of cells in each row; if lengths are all the same as number of column names, return
// true, else false
bool uniformRowLengths(const std::vector< std::vector<std::string> >& cells,
                       const std::vector<std::string>& colNames)
{
    bool result = true;
    
    size_t numCols = colNames.size();
    
    for (size_t row = 0; row < cells.size() && result; row++) {
        if (numCols != cells[row].size()) {
            result = false;
        }
    }
    
    return result;
}

// ========== Local Functions ======================================================================

// read next line of csv from stream, parsed into cells, note whether cells quoted or not; blank
// line terminates reading; return true if more to read, false if done reading
bool nextCsvLine(std::istream& is,
                 std::vector<std::string>& line,
                 std::vector<bool>& lineQuoted)
{
    line.clear();
    lineQuoted.clear();
    
    string str;
    getline(is, str);
    
    size_t strLength = str.length();
    
    // in case line ends with CRLF, exclude CR appearing at end of line
    if (strLength > 0 && str[strLength - 1] == '\r') {
        strLength--;
    }

    bool success = strLength > 0;
    
    size_t startIndex = 0;  // beginning of current cell

    // loop for each cell in line
    while (success && startIndex < strLength) {
        // skip leading spaces and tabs
        while (str[startIndex] == ' ' || str[startIndex] == '\t') {
            startIndex++;
        }
        
        // continue if anything left in line
        if (startIndex < strLength) {
            bool quotedCell = false;
            if (str[startIndex] == '"') {
                quotedCell = true;
                startIndex++;
            }
            
            size_t endIndex = startIndex;   // advance endCell until past end of current cell
            bool done = false;  // true when done with current cell
            
            if (quotedCell) {
                string cell;
                
                while (!done) {
                    if (endIndex == strLength) {
                        // no closed quote before end of line
                        cell.append(str.substr(startIndex, endIndex - startIndex));
                        done = true;
                        
                    } else if (str[endIndex] == '"') {
                        // found quote; handle special cases
                        if (endIndex + 1 == strLength) {
                            // end of line follows closing quote
                            cell.append(str.substr(startIndex, endIndex - startIndex));
                            done = true;
                            
                        } else if (str[endIndex + 1] == '"') {
                            // pair of quotes: converts to a single quote
                            endIndex++;
                            cell.append(str.substr(startIndex, endIndex - startIndex));
                            endIndex++;
                            startIndex = endIndex;
                            
                        } else if (str[endIndex + 1] == ',') {
                            // comma follows closing quote
                            cell.append(str.substr(startIndex, endIndex - startIndex));
                            endIndex++;
                            done = true;
                            
                        } else {
                            // closing quote; cell continues
                            cell.append(str.substr(startIndex, endIndex - startIndex));
                            endIndex++;
                            startIndex = endIndex;
                        }
                        
                    } else {
                        // not an embedded quote; keep adding characters
                        endIndex++;
                    }
                }
                
                line.push_back(cell);
                lineQuoted.push_back(true);
                
            } else {
                while (!done) {
                    if (endIndex == strLength) {
                        // cell terminates at end of line
                        done = true;
                        
                    } else if (str[endIndex] == ',') {
                        // cell terminates at comma
                        done = true;
                        
                    } else {
                        endIndex++;
                    }
                    
                }
                
                string cell = str.substr(startIndex, endIndex - startIndex);
                line.push_back(cell);
                lineQuoted.push_back(false);
            }
            
            // skip over comma; continue with next cell
            startIndex = endIndex + 1;
        }
    }
    
    return success;
}

// ========== Tests ================================================================================

// component tests
void ctest_csv(int& totalPassed, int& totalFailed, bool verbose)
{
    int passed = 0;
    int failed = 0;

    // ~~~~~~~~~~~~~~~~~~~~~~
    // readCsv
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // writeCsv
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // readCsvPath
    // readCsvPath
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // readCsvString
    // readCsvString
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // printCells
    // printCells
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // writeCsvPath
    // writeCsvPath
    
    // ~~~~~~~~~~~~~~~~~~~~~~
    // uniformRowLengths
    // uniformRowLengths

    vector< vector<string> > cells;
    vector< vector<bool> > quoted;
    vector<string> colNames;

    readCsvString("C1, \"C\"\"2\", C3\n 1, \"A\", 3\n 4, 5, 6", cells, quoted, colNames);
    
    if (uniformRowLengths(cells, colNames)) passed++; else failed++;;
    
    cells.push_back(vector<string>(1, "X"));
    
    if (!uniformRowLengths(cells, colNames)) passed++; else failed++;;
    
    readCsvString("1, B, 3\n 4, \"C\"\"\", 6", cells, quoted);
    
    if (uniformRowLengths(cells)) passed++; else failed++;;
    
    cells.push_back(vector<string>(1, "X"));
    
    if (!uniformRowLengths(cells)) passed++; else failed++;;

    // ~~~~~~~~~~~~~~~~~~~~~~
    // nextCsvLine
    
    vector<string> line;
    vector<bool> lineQuoted;
    
    istringstream iss1(" \t1,2,\"A\",\"BC\"\"D\",\"E\"");
    nextCsvLine(iss1, line, lineQuoted);
    if (line[0] == "1" && !lineQuoted[0] &&
        line[1] == "2" && !lineQuoted[1] &&
        line[2] == "A" && lineQuoted[2] &&
        line[3] == "BC\"D" && lineQuoted[3] &&
        line[4] == "E" && lineQuoted[4]) {
        passed++;
    } else {
        failed++;
    }
    
    istringstream iss2("\"F");
    nextCsvLine(iss2, line, lineQuoted);
    if (line[0] == "F" && lineQuoted[0]) {
        passed++;
    } else {
        failed++;
    }
    
    istringstream iss3("3");
    nextCsvLine(iss3, line, lineQuoted);
    if (line[0] == "3" && !lineQuoted[0]) {
        passed++;
    } else {
        failed++;
    }
    
    istringstream iss4("\"G\"H");
    nextCsvLine(iss4, line, lineQuoted);
    if (line[0] == "GH" && lineQuoted[0]) {
        passed++;
    } else {
        failed++;
    }

    // ~~~~~~~~~~~~~~~~~~~~~~

    if (verbose) {
        CERR << "csv.cpp" << "\t\t" << passed << " passed, " << failed << " failed" << endl;
    }
    
    totalPassed += passed;
    totalFailed += failed;
}

// code coverage
void cover_csv(bool verbose)
{
    // ~~~~~~~~~~~~~~~~~~~~~~
    // readCsv
    // readCsvPath
    // readCsvPath
    // printCells
    // printCells
    
    stringToFile("C1, C2, C3\n 1, \"2\", 3\n 4, 5, 6", "foo.csv");

    vector< vector<string> > cells;
    vector< vector<bool> > quoted;
    vector<string> colNames;
    
    readCsvPath("foo.csv", cells, quoted, colNames);
    if (verbose) {
        printCells(cells, quoted, colNames);
    }

    stringToFile("1, \"2\", 3\n 4, 5, 6", "foo.csv");

    readCsvPath("foo.csv", cells, quoted);
    if (verbose) {
        printCells(cells, quoted);
    }

    remove("foo.csv");

    // ~~~~~~~~~~~~~~~~~~~~~~
    // readCsvString
    // readCsvString

    readCsvString("C1, C2, C3\n 1, \"2\", 3\n 4, 5, 6", cells, quoted, colNames);
    readCsvString("1, \"2\", 3\n 4, 5, 6", cells, quoted);

    // ~~~~~~~~~~~~~~~~~~~~~~
    // writeCsv
    // writeCsvPath
    // writeCsvPath
    // uniformRowLengths
    // uniformRowLengths

    readCsvString("C1, \"C\"\"2\", C3\n 1, \"A\", 3\n 4, 5, 6", cells, quoted, colNames);
    writeCsvPath("foo.csv", cells, quoted, colNames);
    uniformRowLengths(cells, colNames);
    
    cells.push_back(vector<string>(1, "X"));
    uniformRowLengths(cells, colNames);
    
    readCsvString("1, B, 3\n 4, \"C\"\"\", 6", cells, quoted);
    writeCsvPath("foo.csv", cells, quoted);
    uniformRowLengths(cells);

    cells.push_back(vector<string>(1, "X"));
    uniformRowLengths(cells);

    remove("foo.csv");

    // ~~~~~~~~~~~~~~~~~~~~~~
    // nextCsvLine

    vector<string> line;
    vector<bool> lineQuoted;

    istringstream iss1(" \t1,2,\"A\",\"BC\"\"D\",\"E\"");
    nextCsvLine(iss1, line, lineQuoted);
    if (verbose) {
        for (size_t k = 0; k < line.size(); k++) {
            CERR << k << "]\t" << (lineQuoted.at(k) ? "T" : "F") << "\t" << line.at(k) << endl;
        }
    }
    
    istringstream iss2("\"F");
    nextCsvLine(iss2, line, lineQuoted);
    if (verbose) {
        for (size_t k = 0; k < line.size(); k++) {
            CERR << k << "]\t" << (lineQuoted.at(k) ? "T" : "F") << "\t" << line.at(k) << endl;
        }
    }
    
    istringstream iss3("3");
    nextCsvLine(iss3, line, lineQuoted);
    if (verbose) {
        for (size_t k = 0; k < line.size(); k++) {
            CERR << k << "]\t" << (lineQuoted.at(k) ? "T" : "F") << "\t" << line.at(k) << endl;
        }
    }
    
    istringstream iss4("\"G\"H");
    nextCsvLine(iss4, line, lineQuoted);
    if (verbose) {
        for (size_t k = 0; k < line.size(); k++) {
            CERR << k << "]\t" << (lineQuoted.at(k) ? "T" : "F") << "\t" << line.at(k) << endl;
        }
    }

}
