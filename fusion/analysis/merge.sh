scriptDir=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")
source $scriptDir/../../env.sh

LIBS="-lpython3.6m -lm -lomp -fopenmp"

name_ext="$(basename $1)"
name="${name_ext%.*}"
name_rn="${name}_rn.bc"
name_inl="${name}_inl.bc"
name_ins="${name}_ins.bc"
name_ins_bin="${name}_rn_ins.bin"
out="out.bc"

#$LLVM_BIN/opt -load $FUSE_LIB/libfusionlib.so -mergeBBList -bbs bblist.txt --dynInf weights.txt --graph_dir imgs < $name_rn_inl > "$out"
$LLVM_BIN/opt -load $FUSE_LIB/libfusionlib.so -mergeBBList -bbs bblist.txt --dynInf weights.txt < $name_rn > "$out"
${LLVM_ROOT}/bin/llvm-dis out.bc
#${LLVM_ROOT}/bin/clang++ -O0 -ggdb $LIBS out.bc
