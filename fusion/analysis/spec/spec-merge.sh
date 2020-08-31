scriptDir=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")

PWD=$(pwd)
SPEC_CPU=/home/dtrilla/spec/benchspec/CPU
CANDIDATES=10
MERGE=y


for dir in $(find $SPEC_CPU -mindepth 1 -maxdepth 1 -type d); do
  BITCODE=$(find $dir -mindepth 3 -maxdepth 3 -name *.bc -not -path '*/\.*')
  if [ ! -z "$BITCODE" ]; then
    name_ext="$(basename $BITCODE)"
    name="${name_ext%.*}"
    if [ -d "$name" ]; then
        cd $name
        ../../merge.sh $BITCODE 
        cd ..
    fi
  fi
done
