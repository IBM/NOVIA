source ../../../env.sh
PASSES_DIR=../../build/src/
BENCH_DIR=../../benchmarks/multidep/
BENCHMARK_BIN=multidep.bin

test_inputs=('0' '1' '2' '3' '4' '20' '23' '-5')

i=0
for test in "${test_inputs[@]}"
do
  ((i=i+1))
  echo "Running test $BENCHMARK_BIN ($i/${#test_inputs[@]}): "
  ./a.out $test > 1.txt
  $BENCH_DIR/$BENCHMARK_BIN $test > 2.txt
  cmp --silent 1.txt 2.txt || echo "test failed"
done
rm 1.txt 2.txt
