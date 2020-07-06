source ../../../env.sh

name_ext="$(basename $1)"
name="${name_ext%.*}"
name_rn="${name}_rn.bc"
name_rn_inl="${name}_rn_inl.bc"
name_ins="${name}_rn_ins.bc"
name_ins_bin="${name}_rn_ins.bin"
out="out.bc"

cd $name
$LLVM_BIN/opt -load $FUSE_LIB/libfusionlib.so -mergeBBList -bbs bblist.txt --dynInf weights.txt --graph_dir imgs < $name_rn_inl > "$out"
${LLVM_ROOT}/bin/llvm-dis out.bc
${LLVM_ROOT}/bin/clang++ -O0 -ggdb -lpython3.6m out.bc
