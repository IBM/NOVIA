import subprocess
import os
import sys

def main(argv):
    if len(argv) < 2:
        print("Wrong number of arguments")
        print("Usage:")
        print("ptyhon multilink.py BITCODE_FOLDER1 BITCODE_FOLDER2 BITCODE_FOLDERN")
        exit()
    stream = os.popen('pwd')
    pwd = stream.read()[:-1]+'/'
    pwd = ''

    suffix_cnt = 0
    names = []
    bitcode_dirs = []
    bitcodes = []
    bitcode_rfs = []
    bitcode_exts = []

    mdir = pwd
    for i in argv:
        mdir += (i.split('/')[-1])[0]
    mdir += '_wk/'

    linked_bc = mdir+'linked.bc'
    for dir in argv:
        names += [dir.split('/')[-1]]
        bitcode_dirs += [pwd+dir+'/']
        bitcodes += [bitcode_dirs[-1]+names[-1]+'_rn.bc']
        bitcode_rfs += [mdir+names[-1]+'_rf.bc']
        bitcode_exts += [mdir+names[-1]+'_ext.bc']

    mkdir_string = "mkdir -p %NAME%"
    llvm_rename_string = "/home/dtrilla/git/fuseacc/llvm-project/build/bin/opt -load "\
        "/home/dtrilla/git/fuseacc/fusion/build/src/libfusionlib.so -renameSuffix -suffix %SUFFIX% "\
        "< %BITCODE% > %BITCODE_RF%"
    llvm_nm_string = "/home/dtrilla/git/fuseacc/llvm-project/build/bin/llvm-nm -defined-only --without-aliases %BITCODE%"
    llvm_extract_string = "/home/dtrilla/git/fuseacc/llvm-project/build/bin/llvm-extract %LIST% "\
        "%BITCODE% -o %EXTRACTED_BITCODE%"
    llvm_link_string = "/home/dtrilla/git/fuseacc/llvm-project/build/bin/llvm-link  "\
        "%BITCODES% -o %LINKED_BITCODE%"
    llvm_merge_string = "/home/dtrilla/git/fuseacc/llvm-project/build/bin/opt -load "\
        "/home/dtrilla/git/fuseacc/fusion/build/src/libfusionlib.so -mergeBBList -bbs "\
        "%BITCODE_DIR%bblist.txt -dynInf %BITCODE_DIR%weights.txt --graph_dir %BITCODE_DIR%imgs < %LINKED_BITCODE% "\
        "> %BITCODE_DIR%out.bc"
    #NO GRAPH
    llvm_merge_string = "/home/dtrilla/git/fuseacc/llvm-project/build/bin/opt -load "\
        "/home/dtrilla/git/fuseacc/fusion/build/src/libfusionlib.so -mergeBBList -bbs "\
        "%BITCODE_DIR%bblist.txt -dynInf %BITCODE_DIR%weights.txt < %LINKED_BITCODE% "\
        "> %BITCODE_DIR%out.bc"

    subprocess.call(mkdir_string.replace("%NAME%",mdir),shell=True)

    # Rename all functions and basic blocks to avoid collision
    for i in range(len(bitcodes)):
        #print(llvm_rename_string.replace("%BITCODE%",bitcodes[i])
        #      .replace("%SUFFIX%",str(i)).replace("%BITCODE_RF%",bitcode_rfs[i]))
        if(not os.path.isfile(bitcode_rfs[i])):
            subprocess.call(llvm_rename_string.replace("%BITCODE%",bitcodes[i])
                 .replace("%SUFFIX%",str(i)).replace("%BITCODE_RF%",bitcode_rfs[i]),shell=True)


    # Merge Weights
    fm = open(mdir+"/weights.txt",'w')
    fbbs = open(mdir+"bblist.txt",'w')
    #fapp = open('apps.txt','r')
    prop = [1/float(len(bitcode_dirs))]*len(bitcode_dirs)
    topbbs = [0]*len(bitcode_dirs)
    #for line in fapp:
    #    prop += [float(line)]
    print(prop,sum(prop[:len(bitcode_dirs)]))
    print(bitcode_dirs)
    #fapp.close()
    weights_list = []
    total = 0.0
    for i in range(len(bitcode_dirs)): 
        f = open(bitcode_dirs[i]+"weights.txt",'r')
        fh = open(bitcode_dirs[i]+"histogram.txt",'r')
        fb = open(bitcode_dirs[i]+"bblist.txt","r")
        for line in fh:
            spl = line.split()
            weights_list += [[spl[0]+str(i),float(spl[1])*prop[i],int(spl[2])]]
            total += float(spl[1])*prop[i]
        for line in fb:
            fbbs.write(line.rstrip()+str(i)+'\n')
    fbbs.close()
    for i in weights_list:
        i[1] = i[1]/total
        #for line in f:
        #    spl = line.split()
        #    weights_list += [(spl[0]+str(i),float(spl[1]),long(spl[2]))]
        f.close()
        fh.close()
    weights_list.sort(key=lambda x: x[1], reverse=True)
    count = 0
    i = 0
    for w in weights_list:
        fm.write(w[0]+' '+str(w[1])+' '+str(w[2])+'\n')
    while(count < len(bitcode_dirs)*10 or i < len(weights_list)):
        index = int(weights_list[i][0].split('r')[-1])
        if topbbs[index] < 10:
            #fbbs.write(weights_list[i][0]+'\n')
            count += 1
            topbbs[index] += 1
        i += 1
    fm.close()
    #fbbs.close()
        
    # Extract functions and link together
    func_list = []
    for i in range(len(bitcode_rfs)):
        stream = os.popen(llvm_nm_string.replace("%BITCODE%",bitcode_rfs[i]))
        output = stream.readlines()
        local_func_list = []
        
        for line in output:
            spl = line.split()
            if spl[1] == 'T' or spl[1] == 't':
                func_list += [spl[2]]
                local_func_list += [spl[2]]

        extract_list = open(mdir+names[i]+'_extract.txt','w')
        extract_list.write('\n'.join(local_func_list))
        extract_list.close()

        extract_string = ' -func='.join(local_func_list)
        extract_string = '-func='+extract_string

        #print(llvm_extract_string.replace("%LIST%",extract_string).replace("%BITCODE%",bitcode_rfs[i])
        #     .replace("%EXTRACTED_BITCODE%",bitcode_exts[i]))
        #print(bitcode_exts[i])
        #print(llvm_extract_string.replace("%LIST%",extract_string).replace("%BITCODE%",bitcode_rfs[i]).replace("%EXTRACTED_BITCODE%",bitcode_exts[i]))
        #print(bitcode_exts[i])
        #print(llvm_extract_string.replace("%LIST%",extract_string).replace("%BITCODE%",bitcode_rfs[i]).replace("%EXTRACTED_BITCODE%",bitcode_exts[i]))
        stream = subprocess.call(llvm_extract_string.replace("%LIST%",extract_string).replace("%BITCODE%",bitcode_rfs[i]).replace("%EXTRACTED_BITCODE%",bitcode_exts[i]).split())
        #print(stream)

    print("linking")
    if(not os.path.isfile(linked_bc)):
        stream = os.system(llvm_link_string.replace("%BITCODES%",' '.join(bitcode_rfs)).replace("%LINKED_BITCODE%",linked_bc))
    else:
        print("reused")
    #print(stream)


    #print(llvm_merge_string.replace("%BITCODE_DIR%",mdir).replace("%LINKED_BITCODE%",linked_bc))
    print("merging")
    stream = os.system(llvm_merge_string.replace("%BITCODE_DIR%",mdir).replace("%LINKED_BITCODE%",linked_bc))
    os.system('mv stats.csv %BITCODE_DIR%/.'.replace('%BITCODE_DIR%',mdir))


    
if __name__ == "__main__":
    main(sys.argv[1:])
    
