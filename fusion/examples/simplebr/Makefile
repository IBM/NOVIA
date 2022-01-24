CFLAGS:=-O0 -std=c++11 -g
CLANG_FLAGS=-emit-llvm -c

all: simplebr 

simplebr: simplebr.cpp
	clang++ $(CFLAGS) $(CLANG_FLAGS) $^ -o $@.bc

simplebr_exe:
	g++ $(CFLAGS) $^ -o $@.bin 

clean:
	rm -f *.bc *.bin *.o *.ll
