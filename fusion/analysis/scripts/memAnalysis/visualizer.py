#!/usr/bin/python3
import cairo
import sys
import numpy as np
import math

def process_txt(file_name):
    file = open(file_name,"r")
    mapping = {}
    i = 0
    for line in file:
        if "#" not in line:
            splits = line.split(':')
            source = int(splits[0].split('(')[0])
            source_size = int(splits[0].split('(')[1].rstrip(')'))
            sinks = splits[1].split(',')[:-1]
            mapping[(source,source_size)] = [(int(i.split('(')[0]),int(i.split('(')[1].rstrip(')'))) for i in sinks]
        else:
            draw(mapping,i)
            i += 1 
            mapping = {}
    file.close()

def draw(mapping,bbn):
    mini =  float('inf')
    maxi = float('-inf')

    for k,v in mapping.items():
        if(k[0]+k[1] > maxi):
            maxi = k[0]+k[1]
        if(k[0] < mini): 
            mini = k[0]
        for pair in v:
            if pair[0]+pair[1] > maxi:
                maxi = pair[0]+pair[1]
            if pair[0] < mini:
                mini = pair[0]

    size = maxi - mini
    height = max(math.ceil(size/64),1)
    print(size,height)


    with cairo.SVGSurface("example{}.svg".format(bbn),64,height) as surface:
        context = cairo.Context(surface)
        context.scale(64,height)
        context.set_source_rgb(0.3,0.3,1.0)
    

        for k,v in mapping.items():
            print(k)
            for byte in range(int(k[1]/8)):
                context.set_source_rgb(0,0,1)
                context.fill()
                context.rectangle((k[0]+byte)%64,int((k[0]-mini+byte)/64),(k[0]-mini+byte+1)%64,int((k[0]-mini+byte+1)/64))
                print((k[0]-mini+byte)%64,int((k[0]-mini+byte)/64),(k[0]-mini+byte+1)%64,int((k[0]-mini+byte+1)/64))
                context.paint()
            for pair in v:
                print("p",pair)
                for byte in range(int(pair[1]/8)):
                    context.set_source_rgb(1,0,0)
                    context.fill()
                    context.rectangle((pair[0]+byte)%64,int((pair[0]-mini+byte)/64),(pair[0]-mini+byte+1)%64,int((pair[0]-mini+byte+1)/64))
                    print((pair[0]+byte)%64,int((pair[0]-mini+byte)/64),(pair[0]-mini+byte+1)%64,int((pair[0]-mini+byte+1)/64))
                    print((k[0]+byte)%64,(k[0]+byte)/64,(k[0]+byte+1)%64,(k[0]+byte+1)/64)
                    context.paint()

        #context.set_source_rgb(0,0,0)
        #context.set_line_width(0.05)
        #for i in range(128):
        #    context.move_to(i,0)
        #    context.line_to(i,height)
        #context.paint()



if __name__=="__main__":
    if(len(sys.argv) < 2):
        print("Missing arguments: ./histogram.py <file_name>")
        sys.exit(-1)
    process_txt(sys.argv[1])
