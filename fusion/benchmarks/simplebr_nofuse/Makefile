include env.mk

all: simplebr_nofuse

simplebr_nofuse: simplebr_nofuse.cpp
	$(GXX) $(CFLAGS) $^ -o $@.bin 
	$(CLANGXX) $(CFLAGS) $(CLANG_FLAGS) $^ -o $@.bc

clean:
	rm -f *.bc *.bin *.o *.ll
