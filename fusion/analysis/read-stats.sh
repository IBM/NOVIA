STATS_LIST=$(find $1 -name stats.csv)
declare -a METRICS=("Num Muxs (Selects)" "Memory Footprint (bits)" "Number of Merged Instructions" "Num Inst" "Maximum Stacked Merges" "Num Stores" "Num Loads" "Merit" "%DynInstr" "Saved Area" "%Saved Area" "SpeedUp")
declare -a METRICS=("Num Inst" "Area" "Number of Merged Instructions" "Saved Area" "Overhead Area" "%Saved Area" "SpeedUp" "%DynInstr" "MergedBBs")
for metric in "${METRICS[@]}"
do
  echo "$metric" | tr ' ' _
  for file in ${STATS_LIST[@]}
  do
    BENCH=$(echo $file | cut -d "/" -f2)
    echo -n "$BENCH "
    python3 parse-stats.py \""$metric"\" $file
  done
done
