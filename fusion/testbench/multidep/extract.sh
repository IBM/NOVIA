source ../../../env.sh
PASSES_DIR=../../build/src/
BENCH_DIR=../../benchmarks/multidep/

${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -renameBBs < ${BENCH_DIR}multidep.bc  > ${BENCH_DIR}multidep_rn.bc
${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -mergeBBList -bbs bblist.txt --graph_dir imgs -fmerg < ${BENCH_DIR}multidep_rn.bc  > out.bc
${LLVM_ROOT}/bin/clang++ out.bc
${LLVM_ROOT}/bin/llvm-dis out.bc
