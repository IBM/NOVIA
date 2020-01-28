source ../../env.sh
PASSES_DIR=../build/src/
BENCH_DIR=../benchmarks/

${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -renameBBs < ${BENCH_DIR}helloworld.bc  > ${BENCH_DIR}helloworld_rn.bc
#${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -listBBs < ${BENCH_DIR}helloworld_rn.bc  > /dev/null
${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -mergeBBList -bbs bblist.txt < ${BENCH_DIR}helloworld_rn.bc  > out.bc
