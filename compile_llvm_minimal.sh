#!/bin/bash

# Tips found in : https://llvm.org/devmtg/2015-04/slides/eurollvm-2015-build.pdf
source env.sh

cd $LLVM_TOP
mkdir -p build
cd build
cmake -G "Ninja" -DLLVM_ENABLE_PROJECTS="libcxx;libcxxabi;lldb;clang;" -DCMAKE_INSTALL_PREFIX=$LLVM_TOP/build/ -DLLVM_ENABLE_ASSERTIONS=ON -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DLLVM_TARGETS_TO_BUILD="X86" -DLLVM_ABI_BREAKING_CHECKS=FORCE_OFF -DLLVM_ENABLE_RTTI=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ../llvm
ninja -j8 install
