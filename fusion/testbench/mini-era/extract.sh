source ../../../env.sh
PASSES_DIR=../../build/src/
BENCH_DIR=~/git/mini-era/

${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -renameBBs < ${BENCH_DIR}main.bc  > main_rn.bc
${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -mergeBBList -bbs bblist.txt --graph_dir imgs < main_rn.bc  > out.bc
#${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -memoryFootprint < out.bc > out2.bc
${LLVM_ROOT}/bin/clang++ -O0 -ggdb -lpython3.6m out.bc
${LLVM_ROOT}/bin/llvm-dis out.bc 
${LLVM_ROOT}/bin/llvm-objdump -d a.out > dis.txt


