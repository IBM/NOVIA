import subprocess
import os
import sys
import pandas as pd
import re
from termcolor import colored

def main(argv):
    fstats = pd.read_csv(argv[0])
    numbbs = int(argv[1])
    fsource = open(argv[2],'r')


    # General Stats
    if(len(fstats) < numbbs):
        print("Specified number of Basic Blocks greater than analyzed ones. Reporting the latter")
        numbbs = len(fstats)
    print("Summary for top {0} basic block regions:".format(numbbs))
    print("Bitcode execution time: {0:.2f}% - Entire application execution time: {1:.2f}%.".format(fstats['Weight'][:numbbs].sum()*100,fstats['Weight with external Calls'][:numbbs].sum()*100))
    # Per BB stats
    for i in range(numbbs):
        start = -1
        finish = -1
        end = -1
        funcs = []
        code = ""
        for num, line in enumerate(fsource, 1):
            if str(fstats['BB'][i]) in line:
                start = num
                if re.search("\[.*\]", line) is not None:
                    funcs = re.search("\[.*\]",line).group(0)[1:-2].split(';')
                    if(funcs[0] != ''):
                        for j in range(len(funcs)):
                            funcs[j]= subprocess.check_output(['llvm-cxxfilt',funcs[j]]).decode('UTF-8').rstrip()
                    else:
                        funcs[0] = colored("Missing Info","red")
            elif start != -1 and not line[0].isnumeric() and line[0] != '\n' and line[0] != '\t':
                end = num
                break
            if start != -1:
                code += line
        if(end-start == 1):
            code += colored("Could not find source code;","red")+" use \"-g\" flag when compiling to include debug information in the bitcode"
        fsource.seek(0)

        print("Top {0} Basic Block - Name: {1} ; Function: {2}".format(i+1,colored(fstats['BB'][i],"green"),colored('{'+','.join(funcs)+'}',"blue")))
        print("Exec Time (with External Calls): {0:.2f}% ({1:.2f}%)".format(fstats['Weight'][i]*100,fstats['Weight with external Calls'][i]*100))
        print("Number of Instructions: {0:.0f} - of which Loads: {1:.0f} Stores {2:.0f} - {3:.2f} % Mem Instr"
              .format(fstats['Size'][i],fstats['Num Loads'][i],fstats['Num Stores'][0],100*(fstats['Num Loads'][i]+fstats['Num Stores'][i])/fstats['Size'][i]))
        print("DFG image (imgs/{0}.png)".format(fstats['BB'][i]))
        print("Source Code (Contents in red):")
        print(code)

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print(colored("Wrong number of arguments","red"))
        print("Usage: ptyhon3 preanalysis.py <orig_stats_file> <number_candidates> <source_file>")
        exit()

    main(sys.argv[1:])
    
