#!/bin/bash
source env.sh

cd $LLVM_TOP
mkdir -p build
cd build
cmake -G "Ninja" -DLLVM_ENABLE_PROJECTS="compiler-rt;libcxx;libcxxabi;lldb;clang;flang;mlir" -DFLANG_INCLUDE_TESTS=Off -DCMAKE_INSTALL_PREFIX=$LLVM_TOP/build/ -DLLVM_ENABLE_ASSERTIONS=ON -DCMAKE_BUILD_TYPE=Debug -DLLVM_TARGETS_TO_BUILD="X86" ../llvm
ninja -j4 install
