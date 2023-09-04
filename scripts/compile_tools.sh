#!/bin/bash
cd $ARIANE_TOP
mkdir -p build
cd build
../configure --prefix=$ARIANE_TOP/build/
make -j $NUM_THREADS
make -j $NUM_THREADS install

cd $RVBINUTILS_TOP
mkdir -p build
cd build
../configure --prefix=$RVBINUTILS_TOP/build/
make -j $NUM_THREADS
make -j $NUM_THREADS install

cd $VERILATOR_TOP
mkdir -p build
cd build
../autoconf
../configure --prefix=$VERILATOR_TOP/build/
make -j $NUM_THREADS
make -j $NUM_THREADS install
