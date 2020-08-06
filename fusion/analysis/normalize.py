import sys

def main():
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
    for i in range(min(len(sorted_list),int(sys.argv[4]))):
        bblist.write(sorted_list[i][0])
        bblist.write('\n')
    bblist.close()


if __name__ == "__main__":
    main()
