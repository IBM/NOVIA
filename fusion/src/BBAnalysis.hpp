//===-----------------------------------------------------------------------------------------===// 
// Copyright 2022 IBM
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//===-----------------------------------------------------------------------------------------===// 
#ifndef BBANALYSIS
#define BBANALYSIS

#include <string>
#include <map>
#include <set>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>

#include "llvm/ADT/SetVector.h"

#include "llvm/Pass.h"
#include "llvm/Analysis/DDG.h"
#include "llvm/ADT/DirectedGraph.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#define DEBUG_TYPE "novia"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/CodeExtractor.h"

//the following are UBUNTU/LINUX, and MacOS ONLY terminal color codes.
#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

#ifndef ANALYSIS
#define ANALYSIS
typedef struct {
  int line;
  int col;
  bool merged;
} debug_desc;
  
static bool operator< (const debug_desc &a, const debug_desc &b){ return a.line < b.line; }
#endif


using namespace llvm;
using namespace std;

static bool sortInst(Instruction *i, Instruction *j){
  if(i->getOpcode() == j->getOpcode())
    return i->getType() < j->getType();
  return i->getOpcode() < j->getOpcode();
}


Value *getSafePointer(PointerType*, Module*);
// Analysis functions
bool areOpsMergeable(Value*,Value*,BasicBlock*,BasicBlock*,
                    map<Value*,Value*>*,set<Value*>*,
                    set<Value*>*);
bool areInstMergeable(Instruction&,Instruction&);
void liveInOut(BasicBlock&,SetVector<Value*>*,SetVector<Value*>*);
void memRAWDepAnalysis(BasicBlock*,map<Instruction*,set<Instruction*>*> *);
void memWARDepAnalysis(BasicBlock*,map<Instruction*,set<Instruction*>*> *);
pair<float,float> getCriticalPathCost(BasicBlock *);
void getDebugLoc(BasicBlock*,stringstream&);

// Stats functions
void getMetadataMetrics(BasicBlock*,vector<double>*, Module*);

#endif
