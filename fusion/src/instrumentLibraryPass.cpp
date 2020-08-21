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
    map<BasicBlock*,GlobalVariable*> tableGV; // Globals for cycle count
    map<BasicBlock*,GlobalVariable*> countGV; // Globals for iteration count

		instrumentBBs() : ModulePass(ID) {}

		bool runOnModule(Module &M) override {
      IRBuilder<> Builder(M.getContext());
      FunctionType *rdtscTy = FunctionType::get(Builder.getInt64Ty(),false);
      Function *rdtsc = Function::Create(rdtscTy,Function::ExternalLinkage,
          "rdtsc",&M);

      // Get the exclusion list, those functions won't be instrumented
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
            // Only BBs of minimum size
            int num_phis = 0;
            for(auto &phi : BB.phis())
              num_phis++;
            if( BB.size()-num_phis > 4 and !BB.isLandingPad()){
              vector<Value*> CallVals;
              tableGV[&BB] = new GlobalVariable(M,Builder.getInt64Ty(),false,
                  GlobalValue::ExternalLinkage,
                  Constant::getNullValue(Builder.getInt64Ty()));
              countGV[&BB] = new GlobalVariable(M,Builder.getInt64Ty(),false,
                  GlobalValue::ExternalLinkage,
                  Constant::getNullValue(Builder.getInt64Ty()));
              Builder.SetInsertPoint(BB.getTerminator());
              Value *start = Builder.CreateCall(rdtsc);
              cast<Instruction>(start)->moveBefore(BB.getFirstNonPHI());
              Value *end = Builder.CreateCall(rdtsc);
              for(auto &I : BB){
                if(isa<CallInst>(&I) and cast<CallInst>(&I)
                    ->getCalledFunction() != rdtsc){
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
              // Cycle counts
              Value *accum = Builder.CreateLoad(Builder.getInt64Ty(),
                  tableGV[&BB]);
              Value *newaccum = Builder.CreateAdd(cycles,accum);
              Builder.CreateStore(newaccum,tableGV[&BB]);

              // Iteration counts
              Value *countAccum = Builder.CreateLoad(Builder.getInt64Ty(),
                  countGV[&BB]);
              Value *countNewAccum = Builder.CreateAdd(cast<Value>(ConstantInt::get(Builder.getInt64Ty(),1)),countAccum);
              Builder.CreateStore(countNewAccum,countGV[&BB]);
              
          }
       }
      }

      // Open file and get FileDescriptor
      FunctionType *collectTy = FunctionType::get(Builder.getVoidTy(),false);
      Function *collect = Function::Create(collectTy,Function::ExternalLinkage,
          "profiler_collector",&M);
      appendToGlobalDtors(M,collect,0);
      BasicBlock *collectBB = BasicBlock::Create(M.getContext(),"collectBB",
          collect);
      Builder.SetInsertPoint(collectBB);
      // Function Prototypes
      vector<Type*> openArgsTypes({Builder.getInt8PtrTy(),
          Builder.getInt32Ty()});
      FunctionType *openTy = FunctionType::get(Builder.getInt32Ty(),
          openArgsTypes,true);
      vector<Type*> sprintfArgsTypes({Builder.getInt8PtrTy(),
          Builder.getInt8PtrTy()});
      FunctionType *sprintfTy = FunctionType::get(Builder.getInt32Ty(),
          sprintfArgsTypes,true);
      vector<Type*> writeArgsTypes({Builder.getInt32Ty(),
          Builder.getInt8PtrTy(),Builder.getInt64Ty()});
      FunctionType *writeTy = FunctionType::get(Builder.getInt64Ty(),
          writeArgsTypes,false);

      Function *open = cast<Function>(M.getOrInsertFunction("open",openTy)
          .getCallee());
      Function *sprintf = cast<Function>(M.
          getOrInsertFunction("sprintf",sprintfTy).getCallee());
      Function *write = cast<Function>(M.getOrInsertFunction("write",writeTy)
          .getCallee());
      Value *openFile = Builder.CreateGlobalStringPtr("histogram.txt");
      Value *mask = Builder.getInt32(578);
      Value *permissions = Builder.getInt32(438); //666
      vector<Value*> openArgs({openFile,mask,permissions});

    
      Value *fd = Builder.CreateCall(open,openArgs);
      ArrayType *bufferTy = ArrayType::get(Builder.getInt8PtrTy(),100);
      Value *buffer = Builder.CreateAlloca(bufferTy);
      Value *bufferPtr = Builder.CreateBitCast(buffer,Builder.getInt8PtrTy());

      // Collect function to gather stats
      Value *sprintfString = Builder.CreateGlobalStringPtr("%s %zd %zd\n");
      for(auto GV : tableGV){

        GlobalVariable *bbName = Builder.CreateGlobalString(GV.
            first->getName());
        Value *counter = Builder.CreateLoad(Builder.getInt64Ty(),GV.second);
        Value *iterCounter = Builder.CreateLoad(Builder.getInt64Ty(),
            countGV[GV.first]);
        vector<Value*> sprintfArgs({bufferPtr,sprintfString,bbName,counter,
            iterCounter});
        Value *size = Builder.CreateCall(sprintf,sprintfArgs);
        Value *size64 = Builder.CreateZExtOrBitCast(size,Builder.getInt64Ty());
        vector<Value*> writeArgs({fd,bufferPtr,size64});
        Builder.CreateCall(write,writeArgs);
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
