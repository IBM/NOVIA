source ../../../env.sh
PASSES_DIR=../../build/src/
BENCH_DIR=./

${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -renameBBs < test_loops.bc > test_loops_rn.bc
${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -mergeBBList -bbs bblist.txt --graph_dir imgs --dynInf weights.txt -fmerg --visualLevel 7 < ${BENCH_DIR}test_loops_rn.bc  > out.bc
${LLVM_ROOT}/bin/llvm-dis out.bc
${LLVM_ROOT}/bin/llc --relocation-model=pic -O0 -filetype=obj out.bc 
g++ -fno-tree-vectorize -O0 -ggdb out.o

