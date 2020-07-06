#include <string>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>

#include "BBManipulation.hpp"
#include "BBAnalysis.hpp"
#include "BBVisualization.hpp"

#include "llvm/Pass.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;
using namespace std;

static cl::opt<string> excludeList("excl",
    cl::desc("File with the exclusion list for functions"),cl::Optional);

namespace {
	struct instrumentBBs : public ModulePass {
		static char ID;
    map<BasicBlock*,GlobalVariable*> tableGV;

		instrumentBBs() : ModulePass(ID) {}

		bool runOnModule(Module &M) override {
      IRBuilder<> Builder(M.getContext());
      FunctionType *rdtscTy = FunctionType::get(Builder.getInt64Ty(),false);
      Function *rdtsc = Function::Create(rdtscTy,Function::ExternalLinkage,"rdtsc",&M);

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

      for(auto &F : M){
        if(!exclude.count(F.getName().str()))
          for(auto &BB : F){
            vector<Value*> CallVals;
            tableGV[&BB] = new GlobalVariable(M,Builder.getInt64Ty(),false,GlobalValue::ExternalLinkage,Constant::getNullValue(Builder.getInt64Ty()));
            Builder.SetInsertPoint(BB.getTerminator());
            Value *start = Builder.CreateCall(rdtsc);
            cast<Instruction>(start)->moveBefore(BB.getFirstNonPHI());
            Value *end = Builder.CreateCall(rdtsc);
            for(auto &I : BB){
              if(isa<CallInst>(&I) and cast<CallInst>(&I)->getCalledFunction() != rdtsc){
                Value *scall = Builder.CreateCall(rdtsc);
                cast<Instruction>(scall)->moveBefore(&I);
                Value *ecall = Builder.CreateCall(rdtsc);
                cast<Instruction>(ecall)->moveAfter(&I);
                Value *elapsed = Builder.CreateSub(ecall,scall);
                CallVals.push_back(elapsed);
              }
            }
            Value *cycles = Builder.CreateSub(end,start);
            for(auto V : CallVals){
              cycles = Builder.CreateSub(cycles,V);
            }
            Value *accum = Builder.CreateLoad(Builder.getInt64Ty(),tableGV[&BB]);
            Value *newaccum = Builder.CreateAdd(cycles,accum);
            Builder.CreateStore(newaccum,tableGV[&BB]);
          }
      }

      vector<Type*> printfArgsTypes({Builder.getInt8PtrTy()});
      FunctionType *printfTy = FunctionType::get(Builder.getInt32Ty(),printfArgsTypes,true);
      //Function *printf =Function::Create(printfTy,Function::ExternalLinkage,"printf",&M);
      Function *printf = cast<Function>(M.getOrInsertFunction("printf",printfTy).getCallee());
      FunctionType *collectTy = FunctionType::get(Builder.getVoidTy(),false);
      Function *collect = Function::Create(collectTy,Function::ExternalLinkage,"profiler_collector",&M);
      appendToGlobalDtors(M,collect,0);
      BasicBlock *collectBB = BasicBlock::Create(M.getContext(),"collectBB",collect);
      Builder.SetInsertPoint(collectBB);
      Value *printfString = Builder.CreateGlobalStringPtr("hist:%s %zd\n");
      for(auto GV : tableGV){

        GlobalVariable *bbName = Builder.CreateGlobalString(GV.first->getName());
        Value *counter = Builder.CreateLoad(Builder.getInt64Ty(),GV.second);
        vector<Value*> Args({printfString,bbName,counter});
        Builder.CreateCall(printf,Args);
      }
      Builder.CreateRetVoid();

      return true;
    }
	};
}


char instrumentBBs::ID = 0;
static RegisterPass<instrumentBBs> a("instrumentBBs", "Instrumentation Pass",
									   false, false);

static RegisterStandardPasses A(
	PassManagerBuilder::EP_EarlyAsPossible,
	[](const PassManagerBuilder &Builder,
	   legacy::PassManagerBase &PM) { PM.add(new instrumentBBs()); });
