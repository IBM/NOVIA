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
#ifndef FUSEDFUNC_H
#define FUSEDFUNC_H

#include <set>
#include <map>
#include <string>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>

#include "../BBAnalysis.hpp"

#include "llvm/Pass.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "llvm/ADT/SetVector.h"

using namespace llvm;
using namespace std;
    

class FusedFunc{
  private:
    LLVMContext *Context;
    Function *F;
    // Stats
    unsigned int minLength;
    unsigned int maxLength;
    
    // analysis
    void getDepChain(Instruction*,vector<Instruction*> *);
    bool match(vector<Instruction*> *,vector<Instruction*> *);
  
  public:
    // Vars
    vector<vector<Instruction*> *> *depChain; 

    // constructor
    FusedFunc();
    FusedFunc(Function*);         // Init Func Constructor
    // destructor
    ~FusedFunc();
    //init
    void init();
    // mergers
    unsigned int reuse(BasicBlock*);
};
#endif
