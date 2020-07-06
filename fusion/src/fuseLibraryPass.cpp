#include <string>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <iomanip>

#include "BBManipulation.hpp"
#include "BBAnalysis.hpp"
#include "BBVisualization.hpp"

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
    cl::desc("Specify file with BB names to merge"));
static cl::opt<string> visualDir("graph_dir",
    cl::desc("Directory for graphviz files"),cl::Optional);
static cl::opt<string> dynamicInfoFile("dynInf",
    cl::desc("File with profiling information per BB"),cl::Optional);
static cl::opt<string> excludeList("excl",
    cl::desc("File with the exclusion list for functions"),cl::Optional);

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
      map<string,double> profileMap;
			BasicBlock *auxBBptr = NULL;
			FusedBB *C, *auxC;
      error_code EC;
      raw_fd_ostream stats("stats.csv",EC);
      stats << "BB,Original Inst (Sum(BBsize)),Maximum Stacked Merges,Number" 
        " of Merged Instructions,Num Inst,Num Loads,Num Stores,Num Muxs (S"
        "elects),Other Instructions,Sequential Time,Memory Footprint (bits)"
        ",Critical Path Communication,Critical Path Computation,Area,"
        "Static Power,Dynamic Power,Efficiency,Merit,%DynInstr\n";

      // Read the dynamic info file
      readDynInfo(dynamicInfoFile,&profileMap);
      double total_percent = 0; 

			// Read the list of BBs to merge
			fstream bbfile;
			string bb_name;
			bbfile.open(bbFileName);
			while(bbfile >> bb_name){
				bbs.push_back(pair<string,BasicBlock*> (bb_name,NULL));
        if(profileMap.count(bb_name))
          total_percent += profileMap[bb_name];
      }
      float aux = 0;
      for(auto E: profileMap)
        aux += E.second;
      
      for(auto E : profileMap)
        profileMap[E.first] = E.second/total_percent;
		
      // Search BB to merge
      for(Function &F: M)
			  for(BasicBlock &BB: F)
          for(int i = 0; i < bbs.size(); ++i)
            if( BB.getName() == bbs[i].first)
              bbs[i].second = &BB;

      for(int i = 0; i < bbs.size(); ++i){
        prebb.push_back(new vector<float>);
        separateBr(bbs[i].second);
        getMetadataMetrics(bbs[i].second,prebb[prebb.size()-1],&M);
        prebb[prebb.size()-1]->push_back(profileMap[bbs[i].second->getName().str()]);
		  	bbList.push_back(bbs[i].second);
      }
      
      //STATS
      for(int i= 0; i < prebb.size(); ++i){
        stats << bbList[i]->getName() << ",";
        for(int j = 0; j < prebb[i]->size(); ++j){
          stats << format("%.5e",(*prebb[i])[j]) << ",";
        }
        stats << "\n";
      }
      stats << "\n";

      // Merge Orderings
      vector<vector<int> > order;
      vector<pair<float,FusedBB*> > vCandidates;
      vector<BasicBlock*> unmergedBBs;
      float area_threshold = 10000000;
      vector<vector<vector<float> > > evol;
      bool go_hw = true;


      for(auto BB : bbList)
        unmergedBBs.push_back(BB);

      while(!unmergedBBs.empty() and go_hw){
        FusedBB *candidate = NULL;
        bool fits_and_improves = true;
        float max_merit = 0;
        int last_index = -1;
        evol.push_back(vector<vector<float> >(unmergedBBs.size()));
        int evol_index = evol.size()-1;
        vector<bool> fused_index(unmergedBBs.size(),true);
        vector<FusedBB*> FusedBBs(unmergedBBs.size());
        vector<vector<float>*> fused(unmergedBBs.size());
        while(fits_and_improves){
          int i =0;
          map<int,int> index_map;
          for(int j =0; j < unmergedBBs.size(); ++j){
            while(!fused_index[i])
              ++i;
            if(!candidate)
              FusedBBs[i] = new FusedBB(&M.getContext(),"");
            else
              FusedBBs[i] = new FusedBB(candidate);
            FusedBBs[i]->mergeBB(unmergedBBs[j]);
            index_map.insert(pair<int,int>(i,j));
    
            fused[i] = new vector<float>;
            getMetadataMetrics(FusedBBs[i]->getBB(),fused[i],&M);
            ++i;
          }
  
          int max_index = -1;
          int deleted = 0;
          float merit = 0;
          for(int i = 0; i < FusedBBs.size();++i){
            if(fused_index[i]){
              merit = getMerit(&bbList,&prebb,fused[i],FusedBBs[i],&profileMap);
              max_index = merit > max_merit? i: max_index;
              max_merit = merit > max_merit? merit: max_merit;
              evol[evol_index][i].push_back(merit);
              fused[i]->push_back(getWeight(&bbList,&prebb,fused[i],FusedBBs[i],&profileMap));
              fused[i]->push_back(merit);
            }
          }
          last_index = max_index;
            
          for(int i = 0; i < FusedBBs.size();++i){
            if(fused_index[i]){
              if( (*fused[i])[12] > area_threshold or i != max_index){
                delete FusedBBs[i];
                deleted++;
              }  
            }
          }
  
          if(max_index >= 0){
            stats << FusedBBs[max_index]->getName()  << ",";
            for(int j = 0; j < fused[max_index]->size(); ++j){
              stats << format("%.5e",(*fused[max_index])[j]) << ",";
            }
            stats<<"\n";

            candidate = FusedBBs[max_index];
            fused_index[max_index] = false;
            if(!visualDir.empty())
              drawBBGraph(candidate,(char*)candidate->getName().c_str(),visualDir);
            unmergedBBs.erase(unmergedBBs.begin()+index_map[max_index]);
            vCandidates.push_back(pair<float,FusedBB*>(max_merit,candidate));
          }
          else if(!candidate)
            go_hw = false;
          fits_and_improves = max_index != -1;
          fused.clear();
        }
      }
      
      
      for(auto P : evol){
        stats << "\n,";
        int i = 0;
        for(auto I : P){
          stats << "Merge Step " << i << ",";
          ++i;
        }
        stats << "\n";
        i = 0;
        for(auto I : P){
          stats << "Candidate " << i << ",";
          for(auto J : I)
            stats << J << ',';
          stats << "\n";
          ++i;
        }
        stats << "\n";
      }
    
			if(vCandidates.size()){
        int max_sel = 0;
        float max = 0;
        FusedBB *candidate;
        for(int i =0; i< vCandidates.size(); ++i){
          max_sel = vCandidates[i].first >= max ? i : max_sel;
          max = vCandidates[i].first >= max? vCandidates[i].first : max;
        }
        stats.flush(); // Flush buffer before dangerous operations
				Foff = vCandidates[max_sel].second->createOffload(&M);
				vCandidates[max_sel].second->insertCall(Foff,&bbList);
        vCandidates.erase(vCandidates.begin()+max_sel);
			}
     
      for(auto E : vCandidates) 
        delete E.second;
      	
      return false;
		}
	};

	struct renameBBs : public ModulePass {
		static char ID;

		map<string,unsigned> Names;

		renameBBs() : ModulePass(ID) {}

		bool runOnModule(Module &M) override{
      // Get the exclusion list
      set<string> exclude;
      if(!excludeList.empty()){
			  fstream exclfile;
        string fname;
			  exclfile.open(excludeList);
        while(exclfile >> fname)
          exclude.insert(fname);
        exclfile.close();
      }

      for(Function &F: M){
        if(!exclude.count(F.getName().str())){
          F.removeFnAttr(Attribute::NoInline);
          F.removeFnAttr(Attribute::OptimizeNone);
          F.addFnAttr(Attribute::AlwaysInline);
        }
		  	for(BasicBlock &BB: F){
          string name = BB.getName().str();
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
            strip_name += to_string(num) + 'r';
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
