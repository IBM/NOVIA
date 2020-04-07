#include <string>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <fstream>

#include "BBManipulation.hpp"
#include "BBAnalysis.hpp"

#include "llvm/Pass.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;
using namespace std;

static cl::opt<string> bbFileName("bbs", cl::desc("Specify file "
			"with BB names to merge"));

namespace {

	struct listBBs : public FunctionPass {
		static char ID;

		listBBs() : FunctionPass(ID) {}

		bool runOnFunction(Function &F) override {
			for(BasicBlock &BB: F)
				errs() << BB.getName() << '\n';
		}
	};

	struct memoryFootprint : public FunctionPass {
		static char ID;

		memoryFootprint() : FunctionPass(ID) {}

		bool runOnFunction(Function &F) override {
			memoryFootprintF(&F);
			LLVMContext &Context = F.getContext();
			SmallVector<pair<unsigned, MDNode*>, 4> MDs;
			F.getAllMetadata(MDs);
			for( auto&MD : MDs){
				if(MDNode *N = MD.second){
					Constant* val = dyn_cast<ConstantAsMetadata>(dyn_cast<MDNode>(N->getOperand(0))->getOperand(0))->getValue();
					errs() << F.getName() << " - " << cast<ConstantInt>(val)->getSExtValue() << "bits \n";
				}
			}
		}
	};


	struct mergeBBList : public ModulePass {
		static char ID;
		set<string> bbs;
		Function *Foff;

		mergeBBList() : ModulePass(ID) {}

		bool runOnModule(Module &M) override {
			vector<BasicBlock*> bbList;
			BasicBlock *auxBBptr = NULL;
			BasicBlock *C, *auxC;

			// Read the list of BBs to merge
			fstream bbfile;
			string bb_name;
			bbfile.open(bbFileName);
			while(bbfile >> bb_name)
				bbs.insert(bb_name);
		
      // Search BB to merge
      for(Function &F: M)
			  for(BasicBlock &BB: F)
		  		if(bbs.count(BB.getName())){
            separateBr(&BB);
		  			bbList.push_back(&BB);
          }

			if(bbList.size())
				C = BasicBlock::Create(M.getContext(),"");

			for(auto& BB: bbList){
        /*
				errs() << "Merging " << BB->getName() << " and " << C->getName() << '\n';
				errs() << " A:\n" << *BB << "B:\n" << '\n';
				for (Instruction &I : *C)
					errs() << I << '\n';*/
				auxC = mergeBBs(*BB,*C);
				delete C;
				C = auxC;

        vector<int> a;
        getMetadataMetrics(C,&a,&M);
				
        /*
        errs() << "Listing New BB" << '\n';
				for (Instruction &I : *C){
					errs() << I << '\n';
				}*/
			}
			if(bbList.size()){
				Foff = createOffload(*C,&M);
				insertCall(Foff,&bbList);
				//delete C;
			}
			return false;
		}
	};

	struct renameBBs : public ModulePass {
		static char ID;

		map<string,unsigned> Names;

		renameBBs() : ModulePass(ID) {}

		bool runOnModule(Module &M) override{
      for(Function &F: M){
		  	for(BasicBlock &BB: F){
          string name = BB.getName();
          string strip_name;
          for(char a: name)
            if(!isdigit(a))
              strip_name.push_back(a);
          int num = Names.count(strip_name);
		  		if(!num)
		  			Names[strip_name] = 1;
		  		else {
            num = Names[strip_name];
		  			Names[strip_name] += 1;
            strip_name += to_string(num);
		  		}
		  		BB.setName(strip_name);
		  	}
      }
			return true;
		}
	};
}


char mergeBBList::ID = 0;
static RegisterPass<mergeBBList> a("mergeBBList", "Hello World Pass",
									   false, false);
char renameBBs::ID = 1;
static RegisterPass<renameBBs> b("renameBBs", "Renames BBs with  unique names",
								 false, false);

char listBBs::ID = 2;
static RegisterPass<listBBs> c("listBBs", "List the names of all BBs", false,
							   false);

char memoryFootprint::ID = 0;
static RegisterPass<memoryFootprint> d("memoryFootprint", "Compute the memory "
		"footprint of basic blocks", false, false);

static RegisterStandardPasses A(
	PassManagerBuilder::EP_EarlyAsPossible,
	[](const PassManagerBuilder &Builder,
	   legacy::PassManagerBase &PM) { PM.add(new mergeBBList()); });

static RegisterStandardPasses B(
	PassManagerBuilder::EP_EarlyAsPossible,
	[](const PassManagerBuilder &Builder,
	   legacy::PassManagerBase &PM) {PM.add(new renameBBs()); });

static RegisterStandardPasses C(
	PassManagerBuilder::EP_EarlyAsPossible,
	[](const PassManagerBuilder &Builder,
	   legacy::PassManagerBase &PM) {PM.add(new listBBs()); });

static RegisterStandardPasses D(
	PassManagerBuilder::EP_EarlyAsPossible,
	[](const PassManagerBuilder &Builder,
	   legacy::PassManagerBase &PM) {PM.add(new memoryFootprint()); });
