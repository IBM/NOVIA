#include <string>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <iomanip>

#include "BBManipulation.hpp"
#include "BBAnalysis.hpp"
#include "BBVisualization.hpp"
#include "FuseSupport.hpp"

#include "types/FusedBB.hpp"

#include "llvm/Pass.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/CommandLine.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;
using namespace std;

static cl::opt<string> bbFileName("bbs",
    cl::desc("Specify file with BB names to merge"));
static cl::opt<string> visualDir("graph_dir",
    cl::desc("Directory for graphviz files"),cl::Optional);
static cl::opt<string> dynamicInfoFile("dynInf",
    cl::desc("File with profiling information per BB"),cl::Optional);

namespace {

	struct listBBs : public FunctionPass {
		static char ID;

		listBBs() : FunctionPass(ID) {}

		bool runOnFunction(Function &F) override {
			for(BasicBlock &BB: F)
				errs() << BB.getName() << '\n';
		}
	};

	struct mergeBBList : public ModulePass {
		static char ID;
		vector<pair<string,BasicBlock*> > bbs;
		Function *Foff;
    vector<vector<float>* > prebb, fused, postbb, last;

		mergeBBList() : ModulePass(ID) {}

		bool runOnModule(Module &M) override {
			vector<BasicBlock*> bbList;
      map<string,float> profileMap;
			BasicBlock *auxBBptr = NULL;
			BasicBlock *C, *auxC;
      error_code EC;
      raw_fd_ostream stats("stats.csv",EC);
      stats << "BB,Original Inst (Sum(BBsize)),Maximum Stacked Merges,Number" 
        " of Merged Instructions,Num Inst,Num Loads,Num Stores,Num Muxs (S"
        "elects),Other Instructions,Sequential Time,Memory Footprint (bits)"
        ",Critical Path Communication,Critical Path Computation,Area,"
        "Static Power,Dynamic Power,Efficiency\n";

      // Read the dynamic info file
      readDynInfo(dynamicInfoFile,&profileMap);

			// Read the list of BBs to merge
			fstream bbfile;
			string bb_name;
			bbfile.open(bbFileName);
			while(bbfile >> bb_name)
				bbs.push_back(pair<string,BasicBlock*> (bb_name,NULL));
		
      // Search BB to merge
      for(Function &F: M)
			  for(BasicBlock &BB: F)
          for(int i = 0; i < bbs.size(); ++i)
            if( BB.getName() == bbs[i].first)
              bbs[i].second = &BB;

      for(int i = 0; i < bbs.size(); ++i){
        prebb.push_back(new vector<float>);
        getMetadataMetrics(bbs[i].second,prebb[prebb.size()-1],&M);
        separateBr(bbs[i].second);
		  	bbList.push_back(bbs[i].second);
      }

			if(bbList.size()){
        FusedBB Fus = FusedBB(M.getContext(),"");
				C = BasicBlock::Create(M.getContext(),"");
        //FusedBB *test = (FusedBB*)FusedBB::Create(M.getContext(),"");
        //test->addMergedBB(C);
        //errs() << test->getNumMerges();
      }
    
      //mergeBBs(*bbList[0],*bbList[1]); 

			for(auto& BB: bbList){
				auxC = mergeBBs(*BB,*C);
				delete C;
				C = auxC;
        if(!visualDir.empty())
          drawBBGraph(C,(char*)C->getName().str().c_str(),visualDir);

				
        fused.push_back(new vector<float>);
        getMetadataMetrics(C,fused[fused.size()-1],&M);
        
        stats << C->getName()  << ",";
        for(int j = 0; j < fused[fused.size()-1]->size(); ++j){
          stats << format("%.5e",(*fused[fused.size()-1])[j]) << ",";
        }
        stats << "\n";
        
        /* 
        errs() << "Listing New BB" << '\n';
				for (Instruction &I : *C){
					errs() << I << '\n';
				}*/
        
			}
			if(bbList.size()){
				Foff = createOffload(*C,&M);
				insertCall(Foff,&bbList);
			}

      //STATS
      stats << "\n";
      for(auto &BB: *Foff){
        last.push_back(new vector<float>);
        getMetadataMetrics(&BB,last[last.size()-1],&M);
        stats << BB.getName() << ",";
        for(int j = 0; j < last[last.size()-1]->size(); ++j){
          stats << format("%.5e",(*last[last.size()-1])[j]) << ",";
        }
        stats << "\n";
      }
      for(auto BB: bbList){
        postbb.push_back(new vector<float>);
        getMetadataMetrics(BB,postbb[postbb.size()-1],&M);
      }
      stats << "\n";
      for(int i= 0; i < prebb.size(); ++i){
        stats << bbList[i]->getName() << ",";
        for(int j = 0; j < prebb[i]->size(); ++j){
          stats << format("%.5e",(*prebb[i])[j]) << ",";
        }
        stats << "\n";
      }
      stats << "\n";
      for(int i= 0; i < postbb.size(); ++i){
        stats << bbList[i]->getName() << "_overhead,";
        for(int j = 0; j < postbb[i]->size(); ++j){
          stats << format("%.5e",(*postbb[i])[j]) << ",";
        }
        stats << "\n";
      }
      stats << "\n";
			
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
