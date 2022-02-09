# NOVIA

NOVIA: A Framework for Discovering Non-Conventional Inline Accelerators, is an LLVM-based toolset that allows for automatic recognition, instantiation and code instrumentations of inline accelerators. NOVIA uses unified bitcode files to analyze a workload and propose inline accelerators to the architect.

The current NOVIA release does not provide the hardware support [NOVIA Functional Unit (NFU)] or the compiler support (clang patch) to compile NFU instructions to RISC-V. Future commits will provide those.

https://dl.acm.org/doi/abs/10.1145/3466752.3480094

# Publication

If you use NOVIA you can cite us:

```
@inbook{10.1145/3466752.3480094,
author = {Trilla, David and Wellman, John-David and Buyuktosunoglu, Alper and Bose, Pradip},
title = {NOVIA: A Framework for Discovering Non-Conventional Inline Accelerators},
year = {2021},
isbn = {9781450385572},
publisher = {Association for Computing Machinery},
address = {New York, NY, USA},
url = {https://doi.org/10.1145/3466752.3480094},
booktitle = {MICRO-54: 54th Annual IEEE/ACM International Symposium on Microarchitecture},
pages = {507–521},
numpages = {15}
}
```

# Dependencies

* cmake ninja-build gcc g++ (for build)
* libgd-dev qt4-qmake libqt4-dev autoconf automake libtool bison flex (for Graphviz)
* python3 python3-pandas python3-termcolor (for automated scripts)

## Automated install:
  * llvm 13.0.0 (https://github.com/llvm/llvm-project/tree/llvmorg-13.0.0)
  * graphviz commit 93d330be (https://gitlab.com/graphviz/graphviz/-/commit/93d330be85c25a5f0b0c86e73f40b4f8d7a42264out)

# Install

## Native
./scripts/install.sh 

## Docker Install
docker build . -t novia:v1

## Docker Run
docker run -it novia:v1 /bin/bash

# Getting Started

NOVIA comes with several automated examples in the subdirectory fusion/examples. The examples contain the base source code, a makefile that generates the input bitcode for novia and a configuration file needed for novia to compile bitcode (mainly the libraries and linking flags needed).

0. **source env.sh**
1. cd fusion/examples/incremental
2. make
3. novia incremental.bc

# Usage Instructions
0. **source env.sh** before usage.
1. Generate a unified LLVM IR bitcode file of the binary to analyze:
   1. Use clang and -emit-llvm flag to generate bitcode files 
   2. Use llvm-link to merge several bitcode fites into a unified bitcode file
2. Apply methodology:
   1. Generate a configuration file with two variables $EXECARGS (Execution arguments that will be used when profiling the workload) $LDFLAGS (Linking flags and libraries needed to compile the workload). **You can find examples of that in the example directory (e.g., fusion/examples/incremental/conf.sh)**
   2. Run the novia tool ( **novia** *bitcode_file* ) [source env.sh or use the full path to the tool in fusion/bin]
   3. The tool will generate a **novia** folder containing the intermediatte bitcode files of the analysis

# Directory Structure

- fusion: novia related scripts and files
   - fusion/examples: synthetically crafted examples
   - fusion/analysis/scripts: data treatment scripts
   - fusion/src: source code
   - fusion/bin: link to novia executable script
- scripts: installation scripts

# Generated Files

When the novia automated analysis script is executed on a bitcode file, the following directories and files will be generated in the bitcode's directory:

- novia folder:
  - **copy of original analyzed bitcode file**
  - **executable binary of instrumented bitcode file**
  - novia/output: annotated bitcode and configuration file for the SoC/Accelerator integration
  - novia/bitcode: folder with intermediate optimized bitcode files
  - novia/data: raw data files for analysis and output summaries:
    - **bblist.txt**: List of basic blocks analyzed by the tool
    - **histogram.txt**: Profiling data per basic block; data fields - [cycle count,total application cycle count,iterations]
    - **weights.txt**: Derived data analysis from histogram.txt; data fields - [time % of bitcode,time % of entire application,iterations] 
    - **orig.csv**/**merge.csv**/**split.csv**: Data metrics for different novia stages
    - **io_overhead.csv**: Metrics refering to input and output variables for the accelerators
    - **stats.csv**: Other metrics
    - **source.log**: Location in source code of basic blocks and accelerators
  - novia/imgs: DFGs of the analyzed basic blocks, merged ISs and final accelerator candidates in .png format

