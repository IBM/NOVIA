# Automated Merge Analysis
The analyze script works in two steps:
1. Copy bitcode file, instrument and analyze:
⋅⋅* G 
2. Use the analysis infromation to perform merge: 

# Usage of Automated Analysis
./analyze.sh PATH_TO_BITCODE "INPUT EXECUTION PARAMETERS" NUMBER_OF_BB_CANDIDATES MERGE_YN

# Parameters
* PATH_TO_BITCODE: Full path to the .bc (llvm-ir) file containing the application to analyze
* "INPUT EXECUTION PARAMETERS": Execution parameters of the application to analyze
* NUMBER_OF_BB_CANDIDATES: Number of top candidates that will be used for the merge analysis
* MERGE_YN: If y then the merge methodology is performed


