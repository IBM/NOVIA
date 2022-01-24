[![Build Status](https://travis.ibm.com/ibm-llvm/crucible.svg?token=5Gz4ZkcRXojuPzsm6v4k&branch=develop)](https://travis.ibm.com/ibm-llvm/crucible)

# NOVIA
A Framework for Discovering Non-Conventional Inline Accelerators

https://dl.acm.org/doi/abs/10.1145/3466752.3480094
LLVM IR Open-Source Tool Release

# Dependencies
* cmake
* ninja-build
* libgd-dev qt4-qmake libqt4-dev (for Graphviz)
* python3 (for automated scripts)


# Install
* ./scripts/install.sh 

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

## Known Usage Issues

# Directory Structure


