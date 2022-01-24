CFLAGS:=-O0 -std=c++11 -g
CLANG_FLAGS=-emit-llvm -c

all: deploop 

deploop: deploop.cpp
	clang++ $(CFLAGS) $(CLANG_FLAGS) $^ -o $@.bc

deploop_exe:
	g++ $(CFLAGS) $^ -o $@.bin 

clean:
	rm -f *.bc *.bin *.o *.ll
