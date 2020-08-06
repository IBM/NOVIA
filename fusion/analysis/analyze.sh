scriptDir=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")
source $scriptDir/../../env.sh

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
name_rn_reg="${name}_rn_reg.bc"
name_rn_inl="${name}_rn_inl.bc"
name_ins="${name}_rn_ins.bc"
name_o="${name}_rn_ins.o"
name_ins_bin="${name}_rn_ins.bin"

LIBS="-lpython3.6m -lm -lomp -fopenmp"
INLINE_STEPS=0

#set -x

mkdir -p $name
cp $1 $name/.
cd $name
echo "Analyzing $name"
echo -n "Renaming: "
if [ ! -f $name_rn ]; then
  $LLVM_BIN/opt -load $FUSE_LIB/libfusionlib.so --renameBBs < $name_ext > "$name_rn"
  echo "done"
else
  echo "reused"
fi

echo -n "Optimizing Memory: "
if [ ! -f $name_rn_inl ]; then
  $LLVM_BIN/opt -mem2reg < $name_rn > $name_rn_reg
  echo "done"
else
  echo "reused"
fi

if [ $INLINE_STEPS == 0 ]; then
  cp $name_rn_reg $name_rn_inl
else
  echo -n "Inlining: "
  if [ ! -f $name_rn_inl ]; then
    tmp_in=tmp_in.bc
    tmp_out=tmp_out.bc
    for i in $(seq $INLINE_STEPS)
    do
      echo -n "Step $i "
      if [ $i  == 1 ]; then
        $LLVM_BIN/opt -load $FUSE_LIB/libfusionlib.so -markInline -inlStep $INLINE_STEPS < $name_rn_reg > $tmp_out
      else
        $LLVM_BIN/opt -load $FUSE_LIB/libfusionlib.so -markInline -inlStep $INLINE_STEPS < $tmp_in > $tmp_out
      fi
      $LLVM_BIN/opt -inline < $tmp_out > $tmp_in
    done
    cp $tmp_in $name_rn_inl
    echo "done"
  else
    echo "reused"
  fi
fi

echo -n "Instrumenting: "
if [ ! -f $name_ins ]; then
  $LLVM_BIN/opt -load $FUSE_LIB/libinstrumentlib.so --instrumentBBs < $name_rn_inl > $name_ins
  echo "done"
else
  echo "reused"
fi

echo -n "Linking Timers and Compiling: "
if [ ! -f $name_o ]; then
  $LLVM_BIN/llc -filetype=obj $name_ins 
fi
if [ ! -f $name_ins_bin ]; then
  $LLVM_BIN/clang++ -no-pie $name_o $FUSE_LIB/arch/x86/CMakeFiles/timer.dir/timer.c.o $LIBS -o "$name_ins_bin"
  echo "done"
else
  echo "reused"
fi

echo -n "Profiling: ./$name_ins_bin $(echo $2 | tr -d \"):"
if [ ! -f histogram.txt ]; then
  EVAL=$(echo "./$name_ins_bin $(echo $2 | tr -d \")")
  eval $EVAL
  echo "done"
else
  echo "reused"
fi

echo "Processing Histogram"
python $scriptDir/normalize.py histogram.txt weights.txt bblist.txt $3

if [ $4 == "y" ]; then
  echo -n "Merging:"
  if [ ! -f out.bc ]; then
    $scriptDir/merge.sh $1 
    echo "done"
  else
    echo "reused"
  fi
fi
