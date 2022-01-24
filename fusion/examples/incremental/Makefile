CFLAGS:=-O0 -std=c++11 -g 
CLANG_FLAGS=-emit-llvm -c

all: incremental 

incremental: incremental.cpp
	clang++ $(CFLAGS) $(CLANG_FLAGS) $^ -o $@.bc

incremental_exe:
	g++ $(CFLAGS) $^ -o $@.bin 

clean:
	rm -f *.bc *.bin *.o *.ll
