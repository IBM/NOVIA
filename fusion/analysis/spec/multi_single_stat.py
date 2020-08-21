import sys
import os

bench = ["perlbench_s","mcf_s","omnetpp_s","xalancbmk_s","x264_s","deepsjeng_s",
         "leela_s","xz_s"]

tseq = [{},{},{},{},{},{},{},{}]
iters = [{},{},{},{},{},{},{},{}]
weights = [{},{},{},{},{},{},{},{}]
def main(argv):
    bbn = 0
    for b in bench:
        f = open(b+"/stats.csv")
        count = 0
        for line in f:
            if line == "\n":
                break
            elif count != 0:
                data = line.split(',')
                tseq[bbn][data[0]] = float(data[9])
            count += 1
        f.close()
        f = open(b+"/weights.txt")
        count = 0
        for line in f:
            if count > 25:
                break
            else:
                data = line.split(' ')
                iters[bbn][data[0]] = float(data[3])
                weights[bbn][data[0]] = float(data[1])

            count +=1

        f.close()
        bbn += 1

    fmerg = open("pmoxxdlx_wk/stats.csv","r")
    parses = False
    parsel = False
    spl = []
    spd = []
    for line in fmerg:
        if 'Subgraphs' in line:
            parses = True
        elif 'Listings' in line:
            parsel = True
            parses = False
        elif(parses and not parsel and line != '\n'):
            data = line.split(',')
            spd += [[float(data[4]),float(data[5])]]
        elif(parsel and line != "\n"):
            spl += [line.rstrip('\n').split(',')[:-1]]
    print(spl)
    print(spd)

    speedups_global = [0]*8
    weights_global = [0]*8
    for i in range(len(spl)):
        acum_tseq = [0]*8
        acum_iters = [0]*8
        acum_weights = [0]*8
        acum_cp = [0]*8
        mtseq = spd[1][0]
        cp = spd[i][1]
        for j in range(len(spl[i])):
            bbn = int(spl[i][j][-1])
            name = spl[i][j][:-1]
            acum_tseq[bbn] += tseq[bbn][name]*iters[bbn][name]
            acum_iters[bbn] += iters[bbn][name]
            acum_weights[bbn] += weights[bbn][name]
        for j in range(8):
            if acum_tseq[j] != 0:
                speed_up = 1/((1-acum_weights[j])+acum_weights[j]/(acum_tseq[j]/(cp*acum_iters[j])))
                print(bench[j]+','+str(acum_weights[j])+','+str(speed_up))
                speedups_global[j] += speed_up-1
                weights_global[j] += acum_weights[j]

    print("Weights Global")
    for i in range(len(speedups_global)):
        print(bench[i],',',weights_global[i])

    print("Global Speedups")
    for i in range(len(speedups_global)):
        print(bench[i],',',speedups_global[i]+1,1/(1-weights_global[i]))





            




if __name__ == "__main__":
    main(sys.argv[1:])
