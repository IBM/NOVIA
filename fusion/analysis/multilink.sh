scriptDir=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")
source $scriptDir/../../env.sh

name_ext="$(basename $1)"
name_ext2="$(basename $2)"
name="${name_ext%.*}"
name2="${name_ext2%.*}"
name_list="${name}_list.txt"
name_list2="${name2}_list.txt"

$LLVM_BIN/llvm-nm $1 | grep -v main | grep 'T' > $name_list
$LLVM_BIN/llvm-nm $2 | grep -v main | grep 'T' > $name_list2

LIST1=''
while IFS= read -r i
do
  echo $i,'l'
  if [ "$(echo "$i" | cut -d ' ' -f2)" == 'T' ]; then
    echo $i
    echo $(echo $i | cut -d ' ' -f1)
    LIST1+='-func='$(echo $i | cut -d ' ' -f3)' ' 
  fi
done < "$name_list"
echo $LIST1

$LLVM_BIN/llvm-extract $LIST1 $1 -o ${name}_f1.bc
$LLVM_BIN/llvm-link ${name}_f1.bc $2 -o linked.bc
