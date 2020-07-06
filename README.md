# fuseacc
LLVM IR fusion library

# Requirements
* wllvm
* cmake
* libgd-dev qt4-qmake libqt4-dev (for Graphviz)


# Compilation Instructions
* ./compile_llvm
* ./compile_graphviz
* cd fusion
* mkdir build
* cd build
* source ../../env.sh
* cmake ../
* make

## Known Compilation Issues
* Compiling llvm from source code is quite compute demanding. If you are just using
1 thread, this might take a while. To control the amount of threads change the 
variable in env.sh file. Setting the thread count too hight might break the linking
stage due to running out of memory. If that happens just go into llvm-project/build
type make and let the linking process finish with just 1 thread.

# Usage Instructions
1. Generate a bitcode file of the binary to analyze:
..1. Use wllvm to generate the binary
..2. Execute extract-bc $(BINARY_NAME) to generate the bitcode file
2. Apply methodology:
..1. cd fusion/analysis
..2. Follow instructions in that folder's README.md


## Known Usage Issues

# Files & Folders
* env.sh: Define number of threads and location of LLVM


