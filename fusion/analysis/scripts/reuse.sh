scriptDir=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")
source $scriptDir/../../env.sh

if [ "$#" -ne 2 ]; then
  echo "Wrong Number of Arguments ($#, 2 required). Usage:"
  echo "./reuse.sh ANALYZED_DIR REUSE_DIR"
  echo -e "\tANALYZED_DIR: Directory with inlined accelerators"
  echo -e "\tREUSE_DIR: Bitcode to search reuse"
  exit
fi

inline="$(basename $1)"
reuse="$(basename $2)"
echo $inline
reuse_dir="reuse_$inline-$reuse"

mkdir -p $reuse_dir
cp $1/out.bc $reuse_dir/.
cp $2/$2_rn.bc $reuse_dir/.
cp $2/bblist.txt $reuse_dir/.

cd $reuse_dir
if [ ! -f inlined_funcs.bc ]; then
  llvm-extract -rfunc="inline_func*"  < out.bc > inlined_funcs.bc
fi

if [ ! -f reuse.bc ]; then
  llvm-link $2_rn.bc inlined_funcs.bc -o reuse.bc
fi

opt -enable-new-pm=0 -load $FUSE_LIB/libreuselib.so --reuseAcc -debug -bbs bblist.txt < reuse.bc > /dev/null

