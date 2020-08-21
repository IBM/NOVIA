source ../../../env.sh
PASSES_DIR=../../build/src/
BENCH_DIR=../../benchmarks/incremental/
BENCHMARK_BIN=incremental.bin

echo "Running test $BENCHMARK_BIN (1/1): "
./a.out  > 1.txt
$BENCH_DIR/$BENCHMARK_BIN > 2.txt
cmp --silent 1.txt 2.txt || echo "test failed"
rm 1.txt 2.txt
