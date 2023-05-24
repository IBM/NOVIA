if [ "$#" -ne 2 ]; then
  echo "Wrong Number of Arguments ($#, 2 required). Usage:"
  echo "./reuse.sh ANALYZED_DIR REUSE_DIR"
  echo -e "\tANALYZED_DIR: Directory with inlined accelerators"
  echo -e "\tREUSE_DIR: Bitcode to search reuse"
  exit
fi

inline="$(basename $1)"
reuse="$(basename $2)"
bbfile=$(dirname $1)/../data/bblist.txt

inline="${inline%.*}"
reuse="${reuse%.*}"

reuse_dir="reuse_$inline-$reuse"
reuse_bitcode_dir="$reuse_dir/bitcode"
reuse_data_dir="$reuse_dir/data"

search_bc="bitcode/$reuse.bc"
inline_bc="bitcode/$inline.bc"
inline_funcs="bitcode/inlined_funcs.bc"
reuse_bc="bitcode/"$reuse"_reuse.bc"

mkdir -p $reuse_dir
mkdir -p $reuse_dir/bitcode
mkdir -p $reuse_dir/output
mkdir -p $reuse_dir/data

cp $1 $reuse_bitcode_dir/.
cp $2 $reuse_bitcode_dir/.
cp $bbfile $reuse_data_dir/.

cd $reuse_dir
if [ ! -f $inline_funcs ]; then
  llvm-extract -rfunc="inline_func*" < $inline_bc > $inline_funcs
fi

if [ ! -f reuse.bc ]; then
  llvm-link $search_bc $inline_funcs -o $reuse_bc
fi

opt -enable-new-pm=0 -load $FUSE_LIB/libreuselib.so --reuseAcc -debug -bbs  data/bblist.txt < $reuse_bc > /dev/null

