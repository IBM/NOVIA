import sys

metric = (sys.argv[1])[1:-1]
f = open(sys.argv[2])

metricn = 0
count = 0
pmetric = -1
psize = 0
first = ''
for line in f:
    splitl = line.split(',')
    if 'BB' in line:
        mcount = 0
        for field in splitl:
            if metric == field.rstrip():
                metricn = mcount
            mcount += 1
    if line == '\n':
        count += 1
        if pmetric != -1:
            print(pmetric)
            pmetric = -1
    elif count == 1:
        if len(splitl[0]) < psize or (not first in splitl[0] and first != ''):
            print(pmetric+' ', end ='')
        psize = len(splitl[0])
        first = splitl[0]
        pmetric = splitl[metricn]


