CFLAGS:=-O0 -std=c++11 -g
CLANG_FLAGS=-emit-llvm -c

all: simplebr_nofuse 

simplebr_nofuse: simplebr_nofuse.cpp
	clang++ $(CFLAGS) $(CLANG_FLAGS) $^ -o $@.bc

simplebr_nofuse_exe:
	g++ $(CFLAGS) $^ -o $@.bin 

clean:
	rm -f *.bc *.bin *.o *.ll
