source ../../env.sh

name_ext="$(basename $1)"
name="${name_ext%.*}"
name_rn="${name}_rn.bc"
name_rn_inl="${name}_rn_inl.bc"
name_ins="${name}_rn_ins.bc"
name_ins_bin="${name}_rn_ins.bin"

echo $name
mkdir -p $name
cp $1 $name/.
cd $name
$LLVM_BIN/opt -load $FUSE_LIB/libfusionlib.so --renameBBs -excl exclude.txt < $name_ext > "$name_rn"
$LLVM_BIN/opt -inline < $name_rn > $name_rn_inl
$LLVM_BIN/opt -load $FUSE_LIB/libinstrumentlib.so --instrumentBBs -excl exclude.txt < $name_rn_inl > $name_ins
$LLVM_BIN/clang++ $name_ins $FUSE_LIB/arch/x86/CMakeFiles/timer.dir/timer.c.o -lpython3.6m -o "$name_ins_bin"
cd ../../../../mini-era
../fuseacc/fusion/analysis/$name/$name_ins_bin $2 | grep "hist:" | cut -d ":" -f2  > ../fuseacc/fusion/analysis/$name/histogram.txt 
cd ../fuseacc/fusion/analysis/$name
python ../normalize.py histogram.txt weights.txt bblist.txt $3

if [ $4 == "y" ]; then
  ../merge.sh $1 
fi
