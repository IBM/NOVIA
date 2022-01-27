if [ "$#" -ne 5 ]; then
  echo "Wrong Number of Arguments ($#, 5 required). Usage:"
  echo "./analyze.sh BITCODE_PATH \"EXECUTABLE_INPUT_ARGUMENTS\" NUM_BB_CANDIDATES MERGE_Y/N"
  echo -e "\tBITCODE_PATH: Path to the bitcode file to analyze"
  echo -e "\tLIBRARY_FLAGS: Flags to compile bitcode into executable"
  echo -e "\tEXECUTABLE_INPUT_ARGUMENTS: Input arguments to execute the binary"
  echo -e "\tNUM_BB_CANDIDATES: Number of bb candidates to merge"
  echo -e "\tMERGE_Y/N: y or n. Indicating whether to apply the merge methodology or not"
  exit
fi

name_ext="$(basename $1)"
name="${name_ext%.*}"
name_inl="bitcode/${name}_inl.bc"
name_reg="bitcode/${name}_reg.bc"
name_rn="bitcode/${name}_rn.bc"
name_ins="bitcode/${name}_ins.bc"
name_o="${name}_ins.o"
name_ins_bin="${name}_ins.bin"

INLINE_STEPS=0

mkdir -p bitcode
mkdir -p data
mkdir -p output
echo "############################################################################################"
echo "Analyzing $name: Preparation phase"
echo "############################################################################################"
if [ $INLINE_STEPS == 0 ]; then
  cp $name_ext $name_inl
else
  echo -n "Inlining: "
  if [ ! -f $name_inl ]; then
    tmp_in=tmp_in.bc
    tmp_out=tmp_out.bc
    for i in $(seq $INLINE_STEPS)
    do
      echo -n "Step $i "
      if [ $i  == 1 ]; then
        $LLVM_BIN/opt -enable-new-pm=0 -load $FUSE_LIB/libfusionlib.so -markInline -inlStep $INLINE_STEPS < $name_ext > $tmp_out
      else
        $LLVM_BIN/opt -enable-new-pm=0 -load $FUSE_LIB/libfusionlib.so -markInline -inlStep $INLINE_STEPS < $tmp_in > $tmp_out
      fi
      $LLVM_BIN/opt -enable-new-pm=0 -inline < $tmp_out > $tmp_in
    done
    cp $tmp_in $name_inl
    echo "done"
  else
    echo "reused"
  fi
fi

echo -n "Optimizing Memory: "
if [ ! -f $name_reg ]; then
  $LLVM_BIN/opt -enable-new-pm=0 -mem2reg < $name_inl > $name_reg
  echo "done"
else
  echo "reused"
fi

echo -n "Renaming: "
if [ ! -f $name_rn ]; then
  $LLVM_BIN/opt -enable-new-pm=0 -load $FUSE_LIB/libfusionlib.so --renameBBs < $name_reg > "$name_rn"
  echo "done"
else
  echo "reused"
fi

echo -n "Instrumenting: "
if [ ! -f $name_ins ]; then
  $LLVM_BIN/opt -enable-new-pm=0 -load $FUSE_LIB/libinstrumentlib.so --instrumentBBs < $name_rn > $name_ins
  echo "done"
else
  echo "reused"
fi

echo -n "Linking Timers and Compiling: "
#if [ ! -f $name_o ]; then
#  $LLVM_BIN/llc -filetype=obj $name_ins 
#fi
if [ ! -f $name_ins_bin ]; then
  $LLVM_BIN/clang++ $name_ins -static -L $FUSE_LIB -ltimer_x86 $2 -o "$name_ins_bin"
  echo $2
  echo "done"
else
  echo "reused"
fi

echo -n "Profiling: ./$name_ins_bin $(echo $3 | tr -d \"):"
if [ ! -f data/histogram.txt ]; then
  EVAL=$(echo "./$name_ins_bin $(echo $3 | tr -d \")")
  eval $EVAL
  mv histogram.txt data/histogram.txt
  echo "done"
else
  echo "reused"
fi

echo "Processing Histogram"
python3 $scriptDir/normalize.py data/histogram.txt data/weights.txt data/bblist.txt $4
if [ $? -ne 0 ]; then
  exit 1
fi

if [ $5 == "y" ]; then
  source $scriptDir/merge.sh $1 $4 
fi

echo "############################################################################################"
echo "Analyzing $name - Pre-merge Report:"
echo "############################################################################################"

python3 $scriptDir/preanalysis.py "data/orig.csv" 5 "data/source.log" 

if [ $5 == "y" ]; then
  echo "############################################################################################"
  echo "Analyzing $name - Post-merge Report:"
  echo "############################################################################################"
  python3 $scriptDir/postanalysis.py "data/merge.csv" "data/split.csv" 5 "data/source.log"
fi

