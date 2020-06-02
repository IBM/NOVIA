#!/bin/bash
source env.sh

cd $GRAPHVIZ_TOP
mkdir -p build
./autogen.sh
./configure --prefix=$GRAPHVIZ_TOP/build/
make -j$NUM_THREADS install
