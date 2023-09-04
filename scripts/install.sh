#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
BASEDIR=$DIR/../
source $BASEDIR/env.sh

git clone -b llvmorg-13.0.0 https://github.com/llvm/llvm-project $LLVM_TOP 
cd $LLVM_TOP
git apply $BASEDIR/patches/llvm.patch
cd ../

git clone  https://gitlab.com/graphviz/graphviz $GRAPHVIZ_TOP 
cd $GRAPHVIZ_TOP
git checkout 93d330be
cd ../

git clone https://github.com/openhwgroup/cva6.git $ARIANE_TOP 
cd $ARIANE_TOP
git checkout 6000e32b
git apply $BASEDIR/patches/ariane.patch
cd ../

git clone https://github.com/riscv/riscv-binutils-gdb.git $RVBINUTILS_TOP
cd $RVBINUTILS_TOP
git checkout f356740
git apply $BASEDIR/patches/binutils.patch
cd ../

git clone git@github.com:verilator/verilator.git $VERILATOR_TOP
cd $VERILATOR_TOP
cd ../

$BASEIDR/scripts/compile_tools.sh
$BASEDIR/scripts/compile_graphviz.sh
$BASEDIR/scripts/compile_llvm_minimal.sh

cd $BASEDIR/fusion
mkdir -p build
cd build
cmake ../
make -j4
