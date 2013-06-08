## entree

---

### Description

This project is an implementation of decision tree ensembles, using binary splits and a 
deterministic (non-random) set of columns for each tree. The project includes the
following components:

* A set of C++ source files for the core algorithms; these can be used independently of R. 
* A set of R package files that encapsulate the C++ source and can be installed and used
within R.
* A set of Mac OS XCode project files that can be used for developing and testing the
C++ files, and for creating an OS X command-line tool that runs the algorithms.

It maybe useful on its own for machine learning, and it may be useful as an example of
writing an R extension that calls C++ code and can be developed and debugged on OS X.

### Usage

* C++

To use the C++ source files within a separate C++ project, copy the contents of
entree/src. Compile with the preprocessor definition `RPROJECT` equal to 0 to remove the
R-specific code, _i.e._, use `-DRPROJECT=0` in gcc or insert `#define RPROJECT 0` at the
top of the file `shim.h`. Messages will be sent to stderr. Logical and runtime
errors will be thrown with `std::logic_error` or `std::runtime_error`.

* R

To create a package usable within R, check and build the R package with the standard
command-line tools:

`cd path_to_entree_directory
R CMD check entree
R CMD build entree`

Then, install the package within R in the usual way:

`setwd("path_to_entree_directory")
install.packages("entree_0.10-1.tar.gz", repos = NULL, type = "source")
library('entree')`

When the C++ files are compiled for use in R, the preprocessor parameter `RPROJECT` is
automatically defined to 1. The code in `shim.h` then causes messages to be rerouted to
the R function `Rprintf()` instead of to stderr. Errors are signaled via the R
function `error()` instead of by throwing exceptions. `R_CheckUserInterrupt()` is called
periodically to allow interruption of lengthy calculations.

* Xcode

The XCode project includes `main.cpp` and other files outside of the R package. These
files contain some integration and unit tests, and provide a command-line interface to
the algorithms.

To compile the package with XCode copy the `entree` and `entreeXC` directories into the
same parent directory. Create an empty `bin` directory in the parent directory as well.
Launch the file `entreeXC.xcodeproj` to start XCode. The default scheme builds a
command-line tool in the neighboring bin/ directory, compiled under the Debug build
configuration. For use as a tool, you probably want the Release configuration, so choose
Edit Scheme, choose the "Run entree" section, select the Info tab, and change the Build
Configuration to Release.

To run the test code, create a New Scheme, and name it "entree test". Choose Edit Scheme,
choose the "Run entree" section, select the Arguments tab, and add "--test" to the section
Arguments Passed On Launch. Select the Info tab, and change the Build Configuration to
Release. If you want to dig into the test code, you can add the argument "-v" for verbose
output and keep the Build Configuration set to Debug.

To run ad-hoc test code or to experiment with other code in the project, modify the 
develop() function in develop.cpp. Create a New Scheme, and name it "entree develop".
Choose Edit Scheme, choose the "Run entree" section, select the Arguments tab, and add the
argument, "--develop".

### Development Environment

Max OS 10.6.8  
R 2.15.1  
XCode 4.2  

This project has not yet been tested or modified for Windows or Linux.

### License

BSD