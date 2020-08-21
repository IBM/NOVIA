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

static cl::opt<bool> force("fmerg",
    cl::desc("Force all merges independently of benefit"),cl::Optional);
static cl::opt<int> inlineSteps("inlStep",
    cl::desc("Fraction of the number of functions to inline"),cl::Optional);
static cl::opt<string> bbFileName("bbs",
    cl::desc("Specify file with BB names to merge"));
static cl::opt<string> visualDir("graph_dir",
    cl::desc("Directory for graphviz files"),cl::Optional);
static cl::opt<string> dynamicInfoFile("dynInf",
    cl::desc("File with profiling information per BB"),cl::Optional);
static cl::opt<string> excludeList("excl",
    cl::desc("File with the exclusion list for functions"),cl::Optional);
static cl::opt<string> suffix("suffix",
    cl::desc("Suffix to be appended to all Fs and BBs"),cl::Optional);


namespace {
  bool compareFused(pair<float,FusedBB*> i,pair<float,FusedBB*> j){
   return i.first > j.first; 
  };

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
      // Mapping of BBs to relative weight in application (profileMap)
      // and number of times exectued (iterMap)
      map<string,double> profileMap;
      map<string,long> iterMap;
			BasicBlock *auxBBptr = NULL;
			FusedBB *C, *auxC;
      error_code EC;
      raw_fd_ostream stats("stats.csv",EC);
      // TODO: Unless specific changes in the implementation are made:
      // Do not alter anything before Efficiency
      // Merit must be last always
      stats << "BB,Original Inst (Sum(BBsize)),Maximum Stacked Merges,Number" 
        " of Merged Instructions,Num Inst,Num Loads,Num Stores,Num Muxs (S"
        "elects),Other Instructions,Sequential Time,Memory Footprint (bits)"
        ",Critical Path Communication,Critical Path Computation,Area,"
        "Static Power,Dynamic Power,Efficiency,%DynInstr,Saved Area,Overhead Area,"
        "%Saved Area,SpeedUp,MergedBBs,Merit\n";

      // Read the dynamic info file
      readDynInfo(dynamicInfoFile,&profileMap, &iterMap);
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
      
      // If no weights are defined, use equal weights for everybody
      if(profileMap.empty()){
        float size = bbs.size();
        for(auto E : bbs)
          profileMap[E.first] = 1.0/size;
      }
      else{
        float aux = 0;
        for(auto E: profileMap)
          aux += E.second;
        for(auto E : profileMap)
          profileMap[E.first] = E.second/aux;
      }
		
      // Search BB to merge
      for(Function &F: M)
			  for(BasicBlock &BB: F)
          for(int i = 0; i < bbs.size(); ++i)
            if( BB.getName() == bbs[i].first)
              bbs[i].second = &BB;

      for(auto bb : bbs)
        if(!bb.second){
          errs() << "Could not find BB: " << bb.first << "\n";
          exit(0);
        }

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
        float max_saved_area = 0;
        float last_merit = 0;
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
            FusedBBs[i]->getMetrics(fused[i],&M);
            //getMetadataMetrics(FusedBBs[i]->getBB(),fused[i],&M);
            ++i;
          }
  
          int max_index = -1;
          int deleted = 0;
          float merit = 0;
          for(int i = 0; i < FusedBBs.size();++i){
            if(fused_index[i]){
              float saved_area = getSavedArea(&bbList,&prebb,fused[i],
                  FusedBBs[i],&profileMap,&iterMap);
              float relative_saved_area = getRelativeSavedArea(&bbList,&prebb,
                  fused[i],FusedBBs[i],&profileMap,&iterMap);
              float speedup = getSpeedUp(&bbList,&prebb,fused[i],FusedBBs[i],&profileMap,
                  &iterMap);
              float overhead_area = FusedBBs[i]->getAreaOverhead();
              merit = getMerit(&bbList,&prebb,fused[i],FusedBBs[i],&profileMap,
                  &iterMap);
              if( saved_area > max_saved_area)
                max_index = i;
              else if (saved_area == max_saved_area)
                if(merit > max_merit)
                  max_index = i;
              //max_index = saved_area >= max_saved_area ? i: max_index;
              max_merit = merit > max_merit? merit: max_merit;
              max_saved_area = saved_area > max_saved_area? saved_area: max_saved_area;
              evol[evol_index][i].push_back(saved_area);
              fused[i]->push_back(getWeight(&bbList,&prebb,fused[i],FusedBBs[i],
                    &profileMap,&iterMap));
              fused[i]->push_back(saved_area);
              fused[i]->push_back(overhead_area);
              fused[i]->push_back(relative_saved_area);
              fused[i]->push_back(speedup);
              fused[i]->push_back(FusedBBs[i]->getNumMerges());
              fused[i]->push_back(merit);
              if(force)
                max_index = i;
            }
          }

          last_index = max_index;

            
          for(int i = 0; i < FusedBBs.size();++i){
            if(fused_index[i]){
              //last_merit = i == max_index? (*fused[i])[fused[i]->size()-1]:last_merit;
              if( (*fused[i])[fused[i]->size()-1] < 0 or i != max_index or 
                  (*fused[i])[fused[i]->size()-6] < 0){
                delete FusedBBs[i];
                deleted++;
              }  
            }
          }

          int count_fused = 0;
          for(auto e : fused_index )
            count_fused += e ? 1 : 0;
  
          if(max_index >= 0 and deleted != count_fused){
            stats << FusedBBs[max_index]->getName()  << ",";
            for(int j = 0; j < fused[max_index]->size(); ++j){
              stats << format("%.5e",(*fused[max_index])[j]) << ",";
            }
            stats<<"\n";

            candidate = FusedBBs[max_index];
            last_merit = (*fused[max_index])[fused[max_index]->size()-1];
            fused_index[max_index] = false;
            if(!visualDir.empty())
              drawBBGraph(candidate,(char*)candidate->getName().c_str(),visualDir);
            unmergedBBs.erase(unmergedBBs.begin()+index_map[max_index]);
            vCandidates.push_back(pair<float,FusedBB*>(max_saved_area,candidate));
          }
          else{
            fits_and_improves = false;
          }
          if(!candidate)
            go_hw = false;
          //fits_and_improves = (max_index != -1 or force) and unmergedBBs.size() and go_hw;
          fused.clear();
        }
      }
      
      
      for(auto P : evol){
        stats << "\n";
        int i = 0;
        stats << "Candidate,";
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
        if(force){
          max_sel = vCandidates.size()-1;
        }
        else{
          for(int i =0; i< vCandidates.size(); ++i){
            max_sel = vCandidates[i].first >= max ? i : max_sel;
            max = vCandidates[i].first >= max? vCandidates[i].first : max;
          }
        }
        stats.flush(); // Flush buffer before dangerous operations
				Foff = vCandidates[max_sel].second->createOffload(&M);
        
        // Split experimentation
        // Remove repeated candidates
        string name;
        vector<int> removeit;
        bool newr = true;
        
        for(int elem=0;elem<vCandidates.size();elem++){
          if(elem != 0){
            if(vCandidates[elem].second->getName().find(name) != string::npos){
              removeit.push_back(elem-1);
              newr = false;
            }
            else{
              newr = true;
            }
          }
          if(newr)
              name = vCandidates[elem].second->getName();
        }
        int counter = 0;
        for(auto &elem: removeit){
          delete vCandidates[elem-counter].second;
          vCandidates.erase(vCandidates.begin()+elem-counter);
          counter++;
        }

        std::sort(vCandidates.begin(),vCandidates.end(),compareFused);
        stats << "Subgraphs,Area Savings,Speedup,BBs,Size,Tseq,CritPath,Weight\n";
        vector<vector<string> > list_bbs;
        float acum_sub_area = 0;
        float acum_orig_area = 0;
        for(int spl=0;spl<vCandidates.size();++spl){
          vector<list<Instruction*>*> subgraphs;
          vCandidates[spl].second->splitBB(&subgraphs);
          for(auto *L : subgraphs){
            errs() << "Graph\n";
            for(auto *I : *L)
              I->dump();
          }
          vector<vector<float>*> data;
          pair<float,float> tmp_areas;
          vector<float> tseq_sub;
          tmp_areas = getSubgraphMetrics(&bbList,&prebb,NULL,vCandidates[spl].second,
              &profileMap,&iterMap,&subgraphs,&data,&tseq_sub);
          if(0)
            drawBBGraph(vCandidates[spl].second,(char*)(vCandidates[spl]
                .second->getName()).c_str(),visualDir,
                &subgraphs);
          errs() << vCandidates[spl].second->getName() << "\n";
          
          for(int i=0;i<subgraphs.size();++i){
            set<string> subset;
            vector<float> tseq_sub;
            BasicBlock *BBtmp = BasicBlock::Create(M.getContext(),"");  
            ValueToValueMapTy VMap;
            vector<float> vs;
            for (auto *I: *(subgraphs[i])){
              Instruction *NewInst = I->clone();
              NewInst->setName(I->getName());
              BBtmp->getInstList().push_back(NewInst);
              BBtmp->setName(vCandidates[spl].second->getName());
              VMap[I] = NewInst;
            }
            SmallVector<BasicBlock*,1> bblist_tmp;
            bblist_tmp.push_back(BBtmp);
            remapInstructionsInBlocks(bblist_tmp,VMap);
            FusedBB *fBB = new FusedBB(&M.getContext(),"");
            fBB->mergeBB(BBtmp);
            fBB->KahnSort();
            fBB->getMetrics(&vs,&M);
            errs() << "Subgraph " << i << " ";
            for(auto elem : vs)
              errs() << elem << " ";
            errs() << "\n";
            vector<float> orig_data;
            vCandidates[spl].second->getMetrics(&orig_data,&M);
            float orig_tseq = orig_data[8]; 
            float orig_cp = orig_data[11];
            float orig_weight = getWeight(&bbList,&prebb,&orig_data,vCandidates[spl].second,&profileMap,&iterMap);
            float red_tseq = orig_tseq/vs[8];
            float new_weight = orig_weight/red_tseq;
            FusedBB *tmp = vCandidates[spl].second;
            float iterations = 0;
            
            for(auto *I: *(subgraphs[i])){
              tmp->fillSubgraphsBBs(I,&subset);
            }
            for(auto name: subset){
              iterations += iterMap[name];
            }
            if((*data[i])[0] < 1){
              acum_sub_area += tmp_areas.first;
              acum_orig_area += tmp_areas.second;
              list_bbs.push_back(vector<string>());
              for(auto name: subset){
                list_bbs[list_bbs.size()-1].push_back(name);
              }
              if(1)
                drawBBGraph(fBB,(char*)(fBB->getName()+
                      string("spl")+to_string(spl)).c_str(),visualDir);
            }

            float sub_speed = 1/((1-new_weight)+new_weight/((orig_cp*iterations)/(vs[11]*iterations)));
            data[i]->push_back(sub_speed);
            data[i]->push_back(subset.size());
            data[i]->push_back(subgraphs[i]->size());
            data[i]->push_back(vs[8]);
            data[i]->push_back(vs[11]);
            data[i]->push_back(new_weight);

            delete fBB;
            delete BBtmp;
          }
            int x = 0;
            for(auto elem : data){
              if((*elem)[0] < 1){
                //stats << "Subgraph " << i << " ";
                for(auto datum : *elem)
                  stats << datum <<  ",";
                stats << "\n";
              }
              ++x;
            }
          for(auto datum: data)
            delete datum;
        }

        int count = 0;
        stats << "\nListings\n";
        for(auto elem : list_bbs){
          for(auto name : elem)
            stats << name << ",";
          stats << "\n";
          count++;
        }
        stats << "\n";
        stats << "Global Area Savings\n";
        stats << acum_sub_area/acum_orig_area << "\n";

        ////////////////////////
				vCandidates[0].second->insertCall(Foff,&bbList);
        vCandidates.erase(vCandidates.begin());
			}
     
      for(auto E : vCandidates) 
        delete E.second;
      
      verifyModule(M,&errs());
      	
      return true;
		}
	};

  struct markInline : public ModulePass {
    static char ID;

    markInline() : ModulePass(ID) {}

    bool runOnModule(Module &M) override {
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
      
      int counter = 0;
      int max_inline_func = M.size()/inlineSteps;
      for(Function &F: M)        
        if(!exclude.count(F.getName().str()) and !F.isIntrinsic() and 
            counter < max_inline_func){
          F.removeFnAttr(Attribute::NoInline);
          F.removeFnAttr(Attribute::OptimizeNone);
          F.addFnAttr(Attribute::AlwaysInline);
          counter++;
        }

      return true;
    }
  };

	struct renameSuffix: public ModulePass {
		static char ID;

		map<string,unsigned> Names;

		renameSuffix() : ModulePass(ID) {}

		bool runOnModule(Module &M) override{
      for(Function &F: M){
        if(!F.isIntrinsic())
          F.setName(F.getName()+suffix);
		  	for(BasicBlock &BB: F)
          BB.setName(BB.getName()+suffix);
      }
			return true;
		}
	};

	struct renameBBs : public ModulePass {
		static char ID;

		map<string,unsigned> Names;

		renameBBs() : ModulePass(ID) {}

		bool runOnModule(Module &M) override{
      for(Function &F: M){
		  	for(BasicBlock &BB: F){
          string name = BB.getName().str();
          string strip_name;
          for(char a: name)
            if(!isdigit(a))
              strip_name.push_back(a);
          int num = Names.count(strip_name);
		  		if(!num){
		  			Names[strip_name] = 1;
            strip_name += 'r';
          }
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
char markInline::ID = 3;
static RegisterPass<markInline> d("markInline", "Mark functions as always inline",
    false,false);

char renameSuffix::ID = 4;
static RegisterPass<renameSuffix> e("renameSuffix", "Renames BBs with  unique names",
								 false, false);

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
	   legacy::PassManagerBase &PM) {PM.add(new markInline()); });

static RegisterStandardPasses E(
	PassManagerBuilder::EP_EarlyAsPossible,
	[](const PassManagerBuilder &Builder,
	   legacy::PassManagerBase &PM) {PM.add(new renameSuffix()); });
