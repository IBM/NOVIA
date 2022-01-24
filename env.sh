DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

export PATH=$PATH:$DIR/fusion/bin

export LLVM_COMPILER_PATH=$DIR/llvm-project/build/bin
export LLVM_COMPILER=clang

export GRAPHVIZ_TOP=$DIR/graphviz/
export GRAPHVIZ_ROOT=$DIR/graphviz/build/

export LLVM_TOP=$DIR/llvm-project/
export LLVM_ROOT=$DIR/llvm-project/build/
export LLVM_BIN=$DIR/llvm-project/build/bin/

export FUSE_LIB=$DIR/fusion/build/lib/

export NUM_THREADS=1
