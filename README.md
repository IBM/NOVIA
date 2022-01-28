# NOVIA

NOVIA: A Framework for Discovering Non-Conventional Inline Accelerators, is an LLVM-based toolset that allows for automatic recognition, instantiation and code instrumentations of inline accelerators. NOVIA uses unified bitcode files to analyze a workload and propose to the architect 

The current NOVIA release does not provide the hardware support [NOVIA Functional Unit (NFU)] or the compiler support (clang patch) to compile NFU instructions to RISC-V. Future commits will provide those.

https://dl.acm.org/doi/abs/10.1145/3466752.3480094

# Dependencies
* cmake ninja-build gcc g++ (for build)
* libgd-dev qt4-qmake libqt4-dev autoconf automake libtool bison flex (for Graphviz)
* python3 python3-pandas python3-termcolor (for automated scripts)

# Install
## Native
* ./scripts/install.sh 
## Docker Install
docker build . -t novia:v1
## Docker Run
docker run -it novia:v1 /bin/bash

## Known Compilation Issues
* Compiling llvm from source code is quite compute demanding. If you are just using
1 thread, this might take a while. To control the amount of threads change the 
variable in env.sh file. Setting the thread count too hight might break the linking
stage due to the system running out of memory. If that happens just go into llvm-project/build
type make and let the linking process finish with just 1 thread.

# Usage Instructions
1. Generate a unified LLVM IR bitcode file of the binary to analyze:
   1. Use clang and -emit-llvm flag to generate bitcode files 
   2. Use llvm-link to merge several bitcode fites into a unified bitcode file
2. Apply methodology:
   1. Generate a configuration file with two variables $EXECARGS (Execution arguments that will be used when profiling the workload) $LDFLAGS (Linking flags and libraries needed to compile the workload)
   2. Run the novia tool ( *novia* bitcode ) [source env.sh or use the full path to the tool in fusion/bin]
   3. The tool will generate a *novia* folder containing the intermediatte bitcode files of the analysis

# Getting Started

NOVIA comes with several automated examples in the subdirectory fusion/examples. The examples contain the base source code, a makefile that generates the input bitcode for novia and a configuration file needed for novia to compile bitcode (mainly the libraries and linking flags needed).

1. cd fusion/examples/incremental
2. make
3. novia incremental.bc

# Directory Structure

* fusion: novia related scripts and files


