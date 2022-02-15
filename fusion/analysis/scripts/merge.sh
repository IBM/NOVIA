name_novia="output/${name}_novia.bc"

echo "############################################################################################"
echo -n "Analyzing $name - Merge phase: "
if [ -f $name_novia ]; then
  echo "reused"
else
  echo ""
fi
echo "############################################################################################"

$LLVM_BIN/opt -enable-new-pm=0 -load $FUSE_LIB/libfusionlib.so -mergeBBList -bbs \
  "data/bblist.txt" --dynInf "data/weights.txt" --graph_dir imgs --visualLevel 7 \
  --nfus nfu.txt < $name_rn > $name_novia

if [ ! -z $VERBOSE ]; then
  echo "Generated inlined bitcode file ($name_novia)"
  echo "Generated merge/split statistics (data/stats.csv)"
  echo "Source code locations (data/source.log)"
  echo "Generated DFG images (imgs/)"
fi
