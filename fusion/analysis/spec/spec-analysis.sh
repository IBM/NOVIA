# Make sure your spec benchmarks are at least compiled and run once:
# This script hardcoded to use the test size
# e.g.: LLVM_COMPILER=clang runcpu --config=wllvm-linux-x86.cfg --action=run --size=test --tune=peak specspeed

PWD=$(pwd)
SPEC_CPU=/home/dtrilla/spec/benchspec/CPU
CANDIDATES=25
MERGE=y
EXCLUDE="cactu"

pushd $SPEC_CPU/../../
source shrc
popd 


for dir in $(find $SPEC_CPU -mindepth 1 -maxdepth 1 -type d); do
  BNAME=$(echo $dir | cut -d '/' -f7 | cut -d . -f2)
  echo $BNAME
  BITCODE=$(find $dir -mindepth 3 -maxdepth 3 -name $BNAME.bc -not -path '*/\.*')
  if [ $BNAME == "cactuBSSN_s" ]||[ $BNAME == "gcc_s" ]||[ $BNAME != "perlbench_s"  ]; then
    BITCODE=''
  fi
  echo $BITCODE
  if [ ! -z "$BITCODE" ]; then
    SPECCMD=$(find $dir -name speccmds.cmd | head -n 1)
    INPUT=$(specinvoke -n $SPECCMD | grep test | grep -v Invoked | head -n 1)
    i=0
    GLOBAL_INPUT=\"
    echo $BITCODE
    if [ -z $SPECCMD ]; then
      echo "Missing speccmd.cmd"
    else
      echo $INPUT
      for param in $INPUT; do
        if [ $i -ne 0 ]; then
          INPUT_PARAM=$(find $dir -maxdepth 3 -name $(basename -- $param)| grep run | head -n 1)
          if [ $BNAME == "perlbench_s" ]; then
            if [ $param == "-I." ]; then
              INPUT_PARAM="-I$dir"
            elif [ $param == "-I./lib" ]; then
              INPUT_PARAM="-I$dir/lib"
            fi
          fi
          if [ -z $INPUT_PARAM ]; then
            GLOBAL_INPUT+=" $param"
          else
            GLOBAL_INPUT+=" $INPUT_PARAM"
          fi
        fi
        let i=i+1
      done
      GLOBAL_INPUT+=\"
      echo " "
      echo $GLOBAL_INPUT
      echo " "
      #if [[ "$BITCODE" == *"x264"* ]]; then
        if [ ! -z $SPECCMD ]; then
          echo " "
          echo "../analyze.sh $BITCODE "$GLOBAL_INPUT" $CANDIDATES $MERGE"
          echo " "
          ../analyze.sh $BITCODE "$GLOBAL_INPUT" $CANDIDATES $MERGE
        fi
      #fi
    fi
  fi
done
