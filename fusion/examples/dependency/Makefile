CFLAGS:=-O0 -std=c++11 -g
CLANG_FLAGS=-emit-llvm -c

all: dependency 

dependency: dependency.cpp
	clang++ $(CFLAGS) $(CLANG_FLAGS) $^ -o $@.bc

dependency_exe:
	g++ $(CFLAGS) $^ -o $@.bin 

clean:
	rm -f *.bc *.bin *.o *.ll
