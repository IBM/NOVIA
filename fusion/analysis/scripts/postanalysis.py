import subprocess
import os
import sys
import pandas as pd
import re
from termcolor import colored

def main(argv):
    merge_fstats = pd.read_csv(argv[0])
    split_fstats = pd.read_csv(argv[1])
    numbbs = int(argv[2])
    fsource = open(argv[3],'r')

    split_fstats = split_fstats.sort_values(by=['SpeedUp'])

    # General Stats
    if(len(split_fstats) < numbbs):
        print("Specified number of accelerators greater than proposed ones. Reporting the latter")
        numbbs = len(split_fstats)
    print("Summary for top {0} proposed inline accelerators:".format(numbbs))
    #print("Bitcode execution time: {0:.2f}% - Entire application execution time: {1:.2f}%.".format(fstats['Weight'][:numbbs].sum()*100,fstats['Weight with external Calls'][:numbbs].sum()*100))
    # Per BB stats
    for i in range(numbbs):
        start = -1
        finish = -1
        end = -1
        funcs = []
        code = ""
        for num, line in enumerate(fsource, 1):
            if str(split_fstats['BB'][i]) in line:
                start = num
                funcs = re.search("\[.*\]",line).group(0)[1:-2].split(';')
                for j in range(len(funcs)):
                    funcs[j]= subprocess.check_output(['llvm-cxxfilt',funcs[j]]).decode('UTF-8').rstrip()
                    print(funcs[j])
            elif start != -1 and not line[0].isnumeric() and line[0] != '\n' and line[0] != '\t':
                end = num
                break
            if start != -1:
                code += line
        if(end-start == 1):
            code += colored("Could not find source code;","red")+" use \"-g\" flag when compiling to include debug information in the bitcode"
        fsource.seek(0)

        print("Top {0} Accelerator - Name: {1} ; Functions: {2}".format(i+1,colored(split_fstats['BB'][i],"green"),colored('{'+funcs[j]+'}',"blue")))
        if(split_fstats['MergedBBs'][i] > 1):
            print(colored("Inline accelerator wirh merges from {0:.0f} Basic Blocks".format(split_fstats['MergedBBs'][i]),"green"))
        else:
            print(colored("Inline accelerators with NO merges","red"))
        print("Exec Time: {0:.2f}%".format(split_fstats['Weight'][i]*100))
        print("Speedup: {0:.2f}x".format(split_fstats['SpeedUp'][i]))
        print("Number of Instructions: {0:.0f} - of which Selects: TBD - TBD % Useful Instr"
              .format(split_fstats['Size'][i]))
        print("Source Code (green = merged instructions, red = unmerged):")
        print(code)

if __name__ == "__main__":
    if len(sys.argv) < 5:
        print(colored("Wrong number of arguments","red"))
        print("Usage: ptyhon3 postnalysis.py <merge_stats_file> <split_stats_file> <number_candidates> <source_file>")
        exit()

    main(sys.argv[1:])
    
