source ../../../env.sh
PASSES_DIR=../../build/src/
BENCH_DIR=../../benchmarks/simplebr3/

${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -renameBBs < ${BENCH_DIR}simplebr3.bc  > ${BENCH_DIR}simplebr3_rn.bc
${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -mergeBBList -bbs bblist.txt --graph_dir imgs < ${BENCH_DIR}simplebr3_rn.bc  > out.bc
${LLVM_ROOT}/bin/llvm-dis out.bc
${LLVM_ROOT}/bin/clang++ -O0 -ggdb out.bc


