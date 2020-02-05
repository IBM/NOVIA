source ../../../env.sh
PASSES_DIR=../../build/src/
BENCH_DIR=../../benchmarks/

${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -renameBBs < ${BENCH_DIR}simplebr3.bc  > ${BENCH_DIR}simplebr3_rn.bc
#${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -listBBs < ${BENCH_DIR}simplebr3_rn.bc  > /dev/null
${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -mergeBBList -bbs bblist.txt < ${BENCH_DIR}simplebr3_rn.bc  > out.bc
${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -memoryFootprint < out.bc  > /dev/null
