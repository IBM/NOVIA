source ../../../env.sh
PASSES_DIR=../../build/src/
BENCH_DIR=../../benchmarks/simplebr_nofuse/
BENCHMARK_BIN=simplebr_nofuse.bin

declare -a test_inputs=('0 b' '1 b' '2 b' '3 b' '4 b' '20 b' '23 b' '-5 b')


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
