#!/bin/bash
source env.sh

cd $LLVM_TOP
mkdir -p build
cd build
cmake -G "Unix Makefiles" -DLLVM_ENABLE_PROJECTS="compiler-rt;libcxx;libcxxabi;lldb;clang;flang" -DFLANG_INCLUDE_TESTS=Off -DCMAKE_INSTALL_PREFIX=$LLVM_TOP/build/ -DLLVM_ENABLE_ASSERTIONS=ON -DCMAKE_BUILD_TYPE=Debug ../llvm
make -j$NUM_THREADS install
