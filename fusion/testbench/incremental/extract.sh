source ../../../env.sh
PASSES_DIR=../../build/src/
BENCH_DIR=../../benchmarks/incremental/

${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -renameBBs < ${BENCH_DIR}incremental.bc  > ${BENCH_DIR}incremental_rn.bc
${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -mergeBBList -bbs bblist.txt --graph_dir imgs --visualLevel 7 --dynInf weights.txt -fmerg < ${BENCH_DIR}incremental_rn.bc  > out.bc
${LLVM_ROOT}/bin/clang++ out.bc
${LLVM_ROOT}/bin/llvm-dis out.bc
