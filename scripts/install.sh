#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
echo $DIR
exit

source $DIR/../env.sh

git clone -b llvmorg-13.0.0 https://github.com/llvm/llvm-project $LLVM_TOP 
git clone -b 93d330be https://gitlab.com/graphviz/graphviz $GRAPHVIZ_TOP 

$DIR/scripts/compile_llvm.sh
$DIR/scripts/compile_graphviz.sh

cd $DIR/../fusion
mkdir -p build
cd build
cmake ../
make -j4
