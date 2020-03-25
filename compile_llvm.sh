#!/bin/bash
source env.sh

cd $LLVM_TOP
mkdir -p build
cd build
cmake -G "Unix Makefiles" -DLLVM_ENABLE_PROJECTS="clang;libcxx;libcxxabi;lldb" -DCMAKE_INSTALL_PREFIX=$LLVM_TOP/build/ -DLLVM_ENABLE_ASSERTIONS=ON -DCMAKE_BUILD_TYPE=Debug ../llvm
make -j$NUM_THREADS install
