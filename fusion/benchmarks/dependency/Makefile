include env.mk

all: dependency 

dependency: dependency.cpp
	$(GXX) $(CFLAGS) $^ -o $@.bin 
	$(CLANGXX) $(CFLAGS) $(CLANG_FLAGS) $^ -o $@.bc

clean:
	rm -f *.bc *.bin *.o *.ll
