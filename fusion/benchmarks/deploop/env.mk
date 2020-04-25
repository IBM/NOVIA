GXX := g++
CFLAGS := -O0 -std=c++11

LLVM_ROOT := ../../../llvm-project/build/bin/
LLVM := $(LLVM_ROOT)llvm
CLANG := $(LLVM_ROOT)clang
CLANGXX := $(LLVM_ROOT)clang++
CLANG_FLAGS := -emit-llvm -c
