import sys
import os

def main():
    if not os.path.exists(sys.argv[1]):
        sys.exit(1)

    fi = open(sys.argv[1])
    fo = open(sys.argv[2],'w+')
    total = 0
    for line in fi:
        data = line.split()
        total += float(data[1])
    fi.seek(0)
    list = []
    for line in fi:
        data = line.split()
        value = float(data[1])/total
        list += [(data[0],value,data[2])]

    sorted_list = sorted(list, key=lambda tup: tup[1],reverse=True)
    for elem in sorted_list:
        fo.write(elem[0])
        fo.write(" %.10f " % elem[1])
        fo.write(" %d\n" % int(elem[2]))
    fi.close()
    fo.close()

    bblist = open(sys.argv[3],'w+')
    amount = float(sys.argv[4])
    if amount < 1 and amount > 0:
        wtotal = 0.0
        i = 0
        while wtotal < amount:
            wtotal += sorted_list[i][1]
            bblist.write(sorted_list[i][0])
            bblist.write('\n')
            i += 1
    else:
        for i in range(min(len(sorted_list),int(sys.argv[4]))):
            bblist.write(sorted_list[i][0])
            bblist.write('\n')
    bblist.close()


if __name__ == "__main__":
    main()
