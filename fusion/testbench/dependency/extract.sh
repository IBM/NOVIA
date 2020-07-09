source ../../../env.sh
PASSES_DIR=../../build/src/
BENCH_DIR=../../benchmarks/dependency/

${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -renameBBs < ${BENCH_DIR}dependency.bc  > ${BENCH_DIR}dependency_rn.bc
${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -mergeBBList -bbs bblist.txt -dynInf weights.txt < ${BENCH_DIR}dependency_rn.bc  > out.bc
${LLVM_ROOT}/bin/clang++ out.bc
${LLVM_ROOT}/bin/llvm-dis out.bc
