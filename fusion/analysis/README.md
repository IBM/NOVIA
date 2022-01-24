# Automated Merge Analysis
The analyze script works in two steps:
1. Copy bitcode file, instrument and analyze:
    1. Generates bitcodes with renamed BBs and instrumentation
    2. Creates histogram.txt (instrumentation count per BB)
    3. Creates weights.txt (relative execution time per BB)
    4. Creates bblist.txt (top X indicated BBs to merge)
2. Use the analysis infromation to perform merge: 
    1. Creates out.bc / out.ll an efficient merge of the bblist.txt BBs

# Usage of Automated Analysis
./analyze.sh PATH_TO_BITCODE "LINK PARAMETERS FOR COMPILATION TO BINARY" "INPUT EXECUTION PARAMETERS" NUMBER_OF_BB_CANDIDATES MERGE_YN

# Parameters
* PATH_TO_BITCODE: Full path to the .bc (llvm-ir) file containing the application to analyze
* "LINK PARAMETERS FOR COMPILATION TO BINARY": The library paths and needed libraries to compile the instrumented bitcode version for runnning and profiling
* "INPUT EXECUTION PARAMETERS": Execution parameters of the application to analyze
* NUMBER_OF_BB_CANDIDATES: Number of top candidates that will be used for the merge analysis
* MERGE_YN: If y then the merge methodology is performed


