CFLAGS:=-O0 -std=c++11 -g
CLANG_FLAGS=-emit-llvm -c

all: storeorder 

storeorder: storeorder.cpp
	clang++ $(CFLAGS) $(CLANG_FLAGS) $^ -o $@.bc

storeorder_exe:
	g++ $(CFLAGS) $^ -o $@.bin 

clean:
	rm -f *.bc *.bin *.o *.ll
