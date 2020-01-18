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
			for(BasicBlock &BB: F)
				errs() << BB.getName() << '\n';
		}
	};


	struct mergeBBList : public FunctionPass {
		static char ID;
		set<string> bbs;
		Function *oFunc;

		mergeBBList() : FunctionPass(ID) {}

		bool doInitialization(Module &M) override {
			// Initialize offload function

			// Read the list of BBs to merge
			fstream bbfile;
			bbfile.open(bbFileName);
			string bb_name;
			while( bbfile >> bb_name )
				bbs.insert(bb_name);
		}

		bool runOnFunction(Function &F) override {
			vector<BasicBlock*> bbList;
			BasicBlock *auxBBptr = NULL;
			BasicBlock *C;

			// Search BB to merge
			for(BasicBlock &BB: F){
				if(bbs.count(BB.getName())){
					bbList.push_back(&BB);
				}
			}

			if(bbList.size())
				C = BasicBlock::Create(F.getContext(),"n",&F);

			for(auto& BB: bbList){
				errs() << "Merging " << BB->getName() << " and " << C->getName() << '\n';
				errs() << " A:\n" << *BB << "B:\n" << *C << '\n';
				C = mergeBBs(*BB,*C);
				//for(succ_iterator sit = succ_begin(BB); sit != succ_end(BB);
				//	++sit)
				//	sit->removePredecessor(BB);
				//BB->replaceAllUsesWith(C);
				errs() << "Listing New BB" << '\n';
				for (Instruction &I : *C){
					errs() << I << '\n';
				}

				// Debug
				/*
				for(Instruction &I : *BB){
					errs() << "Instruction: "<< I << '\n';
					errs() << "Operands:\n";
					for(int j = 0; j < I.getNumOperands(); ++j){
						errs() << *(I.getOperand(j)->getType()) << '\n';
					}
				}*/
			}

			// We remove merged block from the code
			for(auto& BB: bbList)
				BB->removeFromParent();
			return false;
			errs() << F << '\n';
		}
	};

	struct renameBBs : public FunctionPass {
		static char ID;

		map<string,unsigned> Names;

		renameBBs() : FunctionPass(ID) {}

		bool runOnFunction(Function &F) override{
			for(BasicBlock &BB: F){
				if ( Names.count(BB.getName()) == 0)
					Names[BB.getName()] = 1;
				else {
					int num = Names[BB.getName()];
					Names[BB.getName()] += 1;
					BB.setName(BB.getName()+to_string(num));
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
