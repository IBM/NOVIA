#include <stdio.h>
#include <gvc.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>


#include "llvm/Pass.h"
#include "llvm/Analysis/DDG.h"
#include "llvm/ADT/DirectedGraph.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IRBuilder.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/CodeExtractor.h"

#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace std;

void drawBBGraph(BasicBlock*,char*,string);
