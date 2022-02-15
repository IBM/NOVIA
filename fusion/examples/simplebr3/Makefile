CFLAGS:=-O0 -std=c++11 -g
CLANG_FLAGS=-emit-llvm -c

all: simplebr3 

simplebr3: simplebr3.cpp
	clang++ $(CFLAGS) $(CLANG_FLAGS) $^ -o $@.bc

simplebr3_exe:
	g++ $(CFLAGS) $^ -o $@.bin 

clean:
	rm -f *.bc *.bin *.o *.ll
