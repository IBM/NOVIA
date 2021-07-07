#include <stdio.h>
#include <gvc.h>
#include <color.h>
#include <sys/stat.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

#include "types/FusedBB.hpp"

#include "llvm/Pass.h"
#include "llvm/Analysis/DDG.h"
#include "llvm/ADT/DirectedGraph.h"
#include "llvm/ADT/APFloat.h"
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
void drawBBGraph(FusedBB*,char*,string,vector<list<Instruction*>*>* subgraphs = NULL);
