#!/bin/bash

# Tips found in : https://llvm.org/devmtg/2015-04/slides/eurollvm-2015-build.pdf
source env.sh

cd $LLVM_TOP
mkdir -p build
cd build
cmake -G "Unix Makefiles" -DLLVM_ENABLE_PROJECTS="libcxx;libcxxabi;lldb;clang;" -DCMAKE_INSTALL_PREFIX=$LLVM_TOP/build/ -DLLVM_ENABLE_ASSERTIONS=ON -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_LINK_EXECUTABLE=gold -DCMAKE_CXX_LINK_EXECUTABLE=gold ../llvm
make -j$NUM_THREADS install
