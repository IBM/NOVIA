# Make sure your spec benchmarks are at least compiled and run once:
# This script hardcoded to use the test size
# e.g.: LLVM_COMPILER=clang runcpu --config=wllvm-linux-x86.cfg --action=run --size=test --tune=peak specspeed

PWD=$(pwd)
SPEC_CPU=/home/dtrilla/spec/benchspec/CPU
CANDIDATES=2
MERGE=y

pushd $SPEC_CPU/../../
source shrc
popd 


for dir in $(find $SPEC_CPU -mindepth 1 -maxdepth 1 -type d); do
  BITCODE=$(find $dir -mindepth 3 -maxdepth 3 -name *.bc -not -path '*/\.*')
  if [ ! -z "$BITCODE" ]; then
    SPECCMD=$(find $dir -name speccmds.cmd)
    INPUT=$(specinvoke -n $SPECCMD | grep test | grep -v Invoked)
    i=0
    GLOBAL_INPUT=\"
    for param in $INPUT; do
      if [ $i -ne 0 ]; then
        INPUT_PARAM=$(find $dir/data/test -name $param)
        if [ -z $INPUT_PARAM ]; then
          GLOBAL_INPUT+=" $param"
        else
          GLOBAL_INPUT+=" $INPUT_PARAM"
        fi
      fi
      let i=i+1
    done
    GLOBAL_INPUT+=\"
    ../analyze.sh $BITCODE "$GLOBAL_INPUT" $CANDIDATES $MERGE
  fi
done
