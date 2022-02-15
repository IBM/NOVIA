#!/usr/bin/python3

import graphviz
import sys
import subprocess

def main(argv,argc):
    if(argc < 2):
        printf("Missing arguments")
        exit(1)

    file = open(argv[1],'r')

    dot = graphviz.Graph(comment='Callgraph')
    #dot.attr(rankdir='LR')
    current = []
    total = {}
    for line in file:
        line = line.rstrip()
        if line[-1] == '1':
            num = 0
            name = line[:-1]
            #name = subprocess.run(['llvm-cxxfilt',line[:-1]], stdout=subprocess.PIPE).stdout.decode('utf-8')
            name = ''.join(name.rstrip().split(':'))
            if name in total.keys():
                num = total[name]+1
                total[name] += 1
            else:
                total[name] = 0
            dot.node(name+str(num),name)
            current += [name+str(num)]
            if(len(current) > 1):
                dot.edge(current[-2],current[-1])
        elif line[-1] == '0':
            current = current[:-1]

    dot.render('callgraph')


if __name__ == '__main__':
    main(sys.argv, len(sys.argv))

