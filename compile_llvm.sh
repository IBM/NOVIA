#!/bin/bash
source env.sh

cd $LLVM_TOP
mkdir -p build
cd build
cmake -G "Unix Makefiles" -DLLVM_ENABLE_PROJECTS="clang;libcxx;libcxxabi" -DCMAKE_INSTALL_PREFIX=$pwd -DLLVM_ENABLE_ASSERTIONS=ON -DCMAKE_BUILD_TYPE=Release ../llvm
make -j$NUM_THREADS install
