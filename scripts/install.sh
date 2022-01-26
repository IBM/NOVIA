#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
source $DIR/../env.sh

git clone -b llvmorg-13.0.0 https://github.com/llvm/llvm-project $LLVM_TOP 
git clone  https://gitlab.com/graphviz/graphviz $GRAPHVIZ_TOP 
cd $GRAPHVIZ_TOP
git checkout 93d330be
cd ../

$DIR/scripts/compile_graphviz.sh
$DIR/scripts/compile_llvm_minimal.sh

cd $DIR/fusion
mkdir -p build
cd build
cmake ../
make -j4
