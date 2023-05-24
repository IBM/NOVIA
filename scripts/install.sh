#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
source $DIR/../env.sh

git clone -b llvmorg-13.0.0 https://github.com/llvm/llvm-project $LLVM_TOP 
cd $LLVM_TOP
patch -s -p0 < $DIR/patches/llvm.patch
cd ../

git clone  https://gitlab.com/graphviz/graphviz $GRAPHVIZ_TOP 
cd $GRAPHVIZ_TOP
git checkout 93d330be
cd ../

git clone https://github.com/openhwgroup/cva6.git $ARIANE_TOP 
cd $ARIANE_TOP
git checkout 6000e32b
patch -s -p0 < $DIR/patches/ariane.patch
cd ../

git clone https://github.ibm.com/NOVIA/riscv-binutils.git $RVBINUTILS_TOP
cd $RVBINUTILS_TOP
git checkout f356740
patch -s -p0 < $DIR/patches/binutils.patch
cd ../

$DIR/scripts/compile_graphviz.sh
$DIR/scripts/compile_llvm_minimal.sh

cd $DIR/fusion
mkdir -p build
cd build
cmake ../
make -j4
