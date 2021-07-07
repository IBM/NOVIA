import os
import sys

bench = ["perlbench_s","mcf_s","omnetpp_s","xalancbmk_s","x264_s","deepsjeng_s",
         "leela_s","xz_s","pmoxxdlx_wk"]
bench = ["perlbench_s","mcf_s","omnetpp_s","xalancbmk_s","x264_s","deepsjeng_s",
         "leela_s","xz_s"]

def main(argv):
    print("Benchmark,Percentage Execution Time,Average")
    sub_a = []
    sub_p = []
    for dir in bench:
        f = open(dir+"/stats.csv",'r')
        sub_a += [[dir[:-2]]]
        sub_p += [[dir[:-2]]]
        tdyn = 0
        count = 0
        paragraph = 0
        subgraphs = False
        for line in f:
            splited = line.split(',')
            if line == '\n' and count != 0:
                paragraph += 1
            elif count != 0 and paragraph == 0:
                tdyn += float(splited[17])
            count += 1
            if "Subgraphs" in line:
                subgraphs = True
            elif(subgraphs and line != '\n'):
                sub_a[-1] += [float(splited[0])*100]
                sub_p[-1] += [float(splited[1])]
            elif(subgraphs and line == '\n'):
                subgraphs = False
                


        tdyn *= 100
        tavg = tdyn/25
        print(dir[:-2]+",%.0f%%,%.0f%%" % (tdyn,tavg))

        f.close()
    print('Benchmark,Area Savings')
    for elem in sub_a:
        print(elem[0],end =",")
        print('%,'.join(format(x,".1f") for x in elem[1:]),end='%\n')
    print('Benchmark,Speed-up')
    for elem in sub_p:
        print(elem[0],end =",")
        print(','.join(format(x,".3f") for x in elem[1:]),end='\n')


if __name__ == "__main__":
    main(sys.argv[1:])

