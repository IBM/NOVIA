#!/bin/bash
source env.sh

cd $LLVM_TOP
mkdir -p build
cd build
cmake -G "Ninja" -DLLVM_ENABLE_PROJECTS="compiler-rt;libcxxabi;libcxx;lldb;clang;mlir;flang;openmp" -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=$LLVM_TOP/build/ -DLLVM_ENABLE_ASSERTIONS=ON -DCMAKE_BUILD_TYPE=Debug -DLLVM_TARGETS_TO_BUILD="X86;RISCV" -DCMAKE_LINKER=gold ../llvm
ninja -j16 install
