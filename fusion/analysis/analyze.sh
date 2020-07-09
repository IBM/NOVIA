source /home/dtrilla/git/fuseacc/env.sh

if [ "$#" -ne 4 ]; then
  echo "Wrong Number of Arguments ($#, 4 required). Usage:"
  echo "./analyze.sh BITCODE_PATH \"EXECUTABLE_INPUT_ARGUMENTS\" NUM_BB_CANDIDATES MERGE_Y/N"
  echo -e "\tBITCODE_PATH: Path to the bitcode file to analyze"
  echo -e "\tEXECUTABLE_INPUT_ARGUMENTS: Input arguments to execute the binary"
  echo -e "\tNUM_BB_CANDIDATES: Number of bb candidates to merge"
  echo -e "\tMERGE_Y/N: y or n. Indicating whether to apply the merge methodology or not"
  exit
fi

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
$LLVM_BIN/opt -load $FUSE_LIB/libfusionlib.so --renameBBs < $name_ext > "$name_rn"
$LLVM_BIN/opt -inline < $name_rn > $name_rn_inl
$LLVM_BIN/opt -load $FUSE_LIB/libinstrumentlib.so --instrumentBBs < $name_rn_inl > $name_ins
$LLVM_BIN/clang++ $name_ins $FUSE_LIB/arch/x86/CMakeFiles/timer.dir/timer.c.o -lpython3.6m -o "$name_ins_bin"
./$name_ins_bin $2 | grep "hist:" | cut -d ":" -f2  > histogram.txt 
python ../normalize.py histogram.txt weights.txt bblist.txt $3

if [ $4 == "y" ]; then
  ../merge.sh $1 
fi
