# Make sure your spec benchmarks are at least compiled and run once:
# This script hardcoded to use the test size
# e.g.: LLVM_COMPILER=clang runcpu --config=wllvm-linux-x86.cfg --action=run --size=test --tune=peak specspeed

PWD=$(pwd)
SPEC_CPU=/home/dtrilla/spec/benchspec/CPU
CANDIDATES=(0.4)
MERGE=y

pushd $SPEC_CPU/../../
source shrc
popd 

for exp in ${CANDIDATES[@]}; do
  BENCHS=()
  rm */out.bc
  for dir in $(find $SPEC_CPU -mindepth 1 -maxdepth 1 -type d); do
    BNAME=$(echo $dir | cut -d '/' -f7 | cut -d . -f2)
    echo $BNAME
    BITCODE=$(find $dir -mindepth 3 -maxdepth 3 -name $BNAME.bc -not -path '*/\.*')
    if ! [[ $BNAME == "perlbench_s" || $BNAME == "gcc_s" || $BNAME == "mcf_s" || $BNAME == "omnetpp_s" || $BNAME == "xalancbmk_s" || $BNAME == "x264_s" || $BNAME == "deepsjeng_s" || $BNAME == "leela_s" || $BNAME == "xz_s" || $BNAME = "anmd_r" || $BNAME == "parest_r"  || $BNAME == "povray_r" || $BNAME == "lbm_s" || $BNAME == "blender_r" || $BNAME == "imagick_s" || $BNAME = "nab_s"  || $BNAME = "namd_r" ]]; then
      BITCODE=""
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
        if [ $BNAME == "parest_r" ]; then
          GLOBAL_INPUT+=$(find $dir -maxdepth 3 -name $(basename -- "test.prm")| grep run | head -n 1)
          INPUT=""
        fi
        for param in $INPUT; do
          if [ $i -ne 0 ]; then
            INPUT_PARAM=$(find $dir -maxdepth 3 -name $(basename -- $param)| grep run | head -n 1)
            if [ $BNAME == "perlbench_s" ]; then
              if [ $param == "-I." ]; then
                INPUT_PARAM="-I$dir"
              elif [ $param == "-I./lib" ]; then
                INPUT_PARAM="-I$dir/lib"
              fi
            elif [ $BNAME == "nab_s" ]; then
              if [ $param == "hkrdenq" ]; then
                INPUT_PARAM="hkrdenq"
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
        if [ ! -z $SPECCMD ]; then
          echo " "
          echo "../analyze.sh $BITCODE "$GLOBAL_INPUT" $exp $MERGE"
          echo " "
          ../analyze.sh $BITCODE "$GLOBAL_INPUT" $exp $MERGE
          if [ $? -eq 0 ]; then
            BENCHS=${BENCHS:+$BENCHS }$BNAME
          fi
        fi
      fi
    fi
  done
  echo ${BENCHS}
  #if (( $(echo "$exp < 1" | bc -l) )); then
    #BENCH=("perlbench_s gcc_s mcf_s omnetpp_s xalancbmk_s x264_s deepsjeng_s leela_s xz_s namd_r parest_r povray_r lbm_s imagick_s nab_s")
    #python3 isca_results.py ${BENCHS} 
    #mv isca_res/numbbsf.csv isca_res/numbbsf60.csv
    #mv isca_res/areasf.csv isca_res/areasf60.csv
    #mv isca_res/speedf.csv isca_res/speedf60.csv
   # rm */out.bc
  #else
    BENCHINT=("perlbench_s gcc_s mcf_s omnetpp_s xalancbmk_s x264_s deepsjeng_s leela_s xz_s")
    BENCHFLOAT=("namd_r parest_r povray_r lbm_s blender_r imagick_s nab_s")
    python3 ../multilink.py ${BENCHINT} 
    python3 ../multilink.py ${BENCHFLOAT}
    MULTIWK="pgmoxxdlx_wk"
    python3 isca_results.py ${BENCHINT} $MULTIWK
    mv isca_res/numbbsf.csv isca_res/numbbsfint$exp.csv
    mv isca_res/mw_speeds.csv isca_res/mw_speedsint$exp.csv
    mv isca_res/mw_tweights.csv isca_res/mw_tweightsint$exp.csv
    mv isca_res/areasf.csv isca_res/areasfint$exp.csv
    mv isca_res/speedf.csv isca_res/speedfint$exp.csv
    mv isca_res/area_acum.csv isca_res/area_acumint$exp.csv
    mv isca_res/speed_acum.csv isca_res/speed_acumint$exp.csv
    mv isca_res/overhead.csv isca_res/overheadint$exp.csv
    MULTIWK="npplbin_wk"
    python3 isca_results.py ${BENCHFLOAT} $MULTIWK
    mv isca_res/numbbsf.csv isca_res/numbbsffloat$exp.csv
    mv isca_res/mw_speeds.csv isca_res/mw_speedsfloat$exp.csv
    mv isca_res/mw_tweights.csv isca_res/mw_tweightsfloat$exp.csv
    mv isca_res/areasf.csv isca_res/areasffloat$exp.csv
    mv isca_res/speedf.csv isca_res/speedffloat$exp.csv
    mv isca_res/area_acum.csv isca_res/area_acumfloat$exp.csv
    mv isca_res/speed_acum.csv isca_res/speed_acumfloat$exp.csv
    mv isca_res/overhead.csv isca_res/overheadfloat$exp.csv
  #fi
done
