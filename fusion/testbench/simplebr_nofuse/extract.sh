source ../../../env.sh
PASSES_DIR=../../build/src/
BENCH_DIR=../../benchmarks/simplebr_nofuse/

${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -renameBBs < ${BENCH_DIR}simplebr_nofuse.bc  > ${BENCH_DIR}simplebr_nofuse_rn.bc
${LLVM_ROOT}/bin/opt -load ${PASSES_DIR}libfusionlib.so -mergeBBList -bbs bblist.txt -fmerg < ${BENCH_DIR}simplebr_nofuse_rn.bc  > out.bc
${LLVM_ROOT}/bin/clang++ out.bc
${LLVM_ROOT}/bin/llvm-dis out.bc
