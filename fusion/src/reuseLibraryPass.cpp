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
#include <string>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <iomanip>

#include "BBManipulation.hpp"
#include "BBAnalysis.hpp"
#include "BBVisualization.hpp"
#include "types/FusedFunc.hpp"

#include "llvm/Pass.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/CommandLine.h"
  
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;
using namespace std;

static cl::opt<string> bbFileName("bbs",
    cl::desc("Specify basic blocks to search matches"));
static cl::opt<string> accFileName("accs",
    cl::desc("Specify file with accelerator names to search"),cl::Optional);
static cl::opt<string> visualDir("graph_dir",
    cl::desc("Directory for graphviz files"),cl::Optional);
static cl::opt<int> visLev("visualLevel",
    cl::desc("Level of visualization output:\n"
      "0: No visual output\n"
      "1: Only Original BB DFGs\n"
      "2: Only Merged BB DFGs\n"
      "4: Only Split BB DFGs\n"
      "3: Original and Merged BB DFGs\n"
      "5: Original and Split BB DFGs\n"
      "6: Merged and Split BB DFGs\n"
      "7: All BB DFGs"),cl::Optional);
static cl::opt<bool> debug("dbg",
    cl::desc("Create bitcode with redundant original BBs to detect errors in"
      " the inline/offload process"), cl::Optional);

namespace {

	struct reuseAcc: public ModulePass {
		static char ID;
		vector<pair<string,BasicBlock*> > bbs;
		Function *Foff;
    vector<vector<double>* > prebb, fused, postbb, last;

		reuseAcc() : ModulePass(ID) {}

		bool runOnModule(Module &M) override {
      vector<FusedFunc*> searchAcc;

      // Search for merged/accelerated functions
      for(auto &F : M){
        if(F.hasFnAttribute("novia_acc")){
          searchAcc.push_back(new FusedFunc(&F));
        }
      }


      // Stop if no accelerated functions are found
      if(searchAcc.empty()){
        errs() << "Err: No searchable accelerators have been found\n";
        errs() << "Err: Use llvm-extract/llvm-link to extract/attach searchable functions to code \
          under analysis\n";
        return false;
      }
      
      // Get BB Names to analyze 
      set<string> bbNames;
			fstream tmp_bbFile;
			string tmp_bbName;
			tmp_bbFile.open(bbFileName);
			while(tmp_bbFile >> tmp_bbName)
				bbNames.insert(tmp_bbName);

      if(bbNames.empty()){
        errs() << "Err: No searchable Basicblocks specified\n";
        errs() << "Err: Use -bbs parameter to specify the basic block list names\n";
        return false;
      }


      unsigned bbcounter = 0;
      for(auto &F : M){
        for(auto &BB : F){
          if(bbNames.count(BB.getName().str())){
            bbcounter++;
            for(auto FusedF : searchAcc){
              unsigned matches = FusedF->reuse(&BB);
              LLVM_DEBUG(dbgs() << "Dbg: Number of matches " << matches << "\n");
            }
          }
        }
      }

      if(!bbcounter){
        errs() << "Err: No Basic Block names matched\n";
        return false; 
      }

      return true;
    }
  };
}

char reuseAcc::ID = 0;
static RegisterPass<reuseAcc> a("reuseAcc", "Find matches to an existing accelerator",
									   false, false);

static RegisterStandardPasses A(
	PassManagerBuilder::EP_EarlyAsPossible,
	[](const PassManagerBuilder &Builder,
	   legacy::PassManagerBase &PM) { PM.add(new reuseAcc()); });
