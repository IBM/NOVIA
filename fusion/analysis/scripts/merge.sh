name_novia="output/${name}_novia.bc"

echo "############################################################################################"
echo -n "Analyzing $name - Merge phase: "
if [ -f $name_novia ]; then
  echo "reused"
else
  echo ""
fi
if [ -f data/nfu.txt ]; then
  echo "Using data/nfu.txt list to implement inline accelerators"
else
  echo "NFU file does not exist, implementing all inline accelerators"
fi
echo "############################################################################################"

if [ -z $NOVIA_MODE ]; then
  $LLVM_BIN/opt -enable-new-pm=0 -load $FUSE_LIB/libfusionlib.so -mergeBBList -bbs \
  "data/bblist_inl.txt" --dynInf "data/weights_inl.txt" --graph_dir imgs \
  --visualFormat $NOVIA_VIZ_FORMAT --visualLevel $NOVIA_VIZ_LVL --nfus "data/nfu.txt" \
  < $name_inl > $name_novia
else
  $LLVM_BIN/opt -enable-new-pm=0 -load $FUSE_LIB/libfusionlib.so -mergeBBList -bbs \
  "data/bblist_inl.txt" --dynInf "data/weights_inl.txt" --graph_dir imgs --dbg \
  --visualFormat $NOVIA_VIZ_FORMAT --visualLevel $NOVIA_VIZ_LVL --nfus "data/nfu.txt" \
  < $name_inl > $name_novia
fi

if [ ! -z $VERBOSE ]; then
  echo "Generated inlined bitcode file ($name_novia)"
  echo "Generated merge/split statistics (data/stats.csv)"
  echo "Source code locations (data/source.log)"
  echo "Generated DFG images (imgs/)"
fi
