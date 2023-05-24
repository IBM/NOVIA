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
static cl::opt<string> includeList("incl",
    cl::desc("File with the inclusion list for functions"),cl::Optional);

namespace {
	struct callgraphFuncs : public ModulePass {
		static char ID;

		callgraphFuncs() : ModulePass(ID) {}

		bool runOnModule(Module &M) override {
      IRBuilder<> Builder(M.getContext());

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

      // Get the inclusion list, functions that will be instrumented
      set<string> include;
      if(!includeList.empty()){
			  fstream inclfile;
        string fname;
			  inclfile.open(includeList);
        while(inclfile >> fname)
          include.insert(fname);
        inclfile.close();
      }


      // Open file and get FileDescriptor
      //FunctionType *startCallTy = FunctionType::get(Builder.getVoidTy(),false);
      /*Function *startCall = Function::Create(startCallTy,Function::ExternalLinkage,
          "startCall",&M);*/
      Function *main = M.getFunction("main");
      BasicBlock *entryMain = &main->front();
      BasicBlock *startCallBB = BasicBlock::Create(M.getContext(),"startCallBB",
          main,entryMain);
      Builder.SetInsertPoint(startCallBB);
      Value *sprintfStringEntry = Builder.CreateGlobalStringPtr("%s1\n");
      Value *sprintfStringExit = Builder.CreateGlobalStringPtr("%s0\n");
      
      ArrayType *ArrayTy_0 = ArrayType::get(IntegerType::get(M.getContext(),8), 100);
      PointerType *PointerTy_1 = PointerType::get(ArrayTy_0, 0);

      GlobalVariable* bufferPtr = new GlobalVariable(M,cast<Type>(ArrayTy_0),false,
          GlobalVariable::ExternalLinkage,0,"ArrayPtr");
      ConstantAggregateZero *const_array_2 = ConstantAggregateZero::get(ArrayTy_0);
      bufferPtr->setInitializer(const_array_2);
      //appendToGlobalCtors(M,startCall,0);
      
      // Function Prototypes
      vector<Type*> openArgsTypes({Builder.getInt8PtrTy(),
          Builder.getInt32Ty()});
      FunctionType *openTy = FunctionType::get(Builder.getInt32Ty(),
          openArgsTypes,true);
      vector<Type*> sprintfArgsTypes({Builder.getInt8PtrTy(),
          Builder.getInt8PtrTy()});
      FunctionType *sprintfTy = FunctionType::get(Builder.getInt32Ty(),
          sprintfArgsTypes,true);
      vector<Type*> writeArgsTypes;
      FunctionType *writeTy = FunctionType::get(Builder.getInt32Ty(),
          writeArgsTypes,true);

      Function *open = cast<Function>(M.getOrInsertFunction("open",openTy)
          .getCallee());
      Function *sprintf = cast<Function>(M.
          getOrInsertFunction("sprintf",sprintfTy).getCallee());
      Function *write = cast<Function>(M.getOrInsertFunction("write",writeTy)
          .getCallee());
      Value *openFile = Builder.CreateGlobalStringPtr("callgraph.txt");
      Value *mask = Builder.getInt32(578);
      Value *permissions = Builder.getInt32(438); //666
      vector<Value*> openArgs({openFile,mask,permissions});

      Value *fd_local = Builder.CreateCall(open,openArgs);
      GlobalVariable* fd = new GlobalVariable(M,Builder.getInt32Ty(),false,
          GlobalVariable::ExternalLinkage,ConstantInt::get(Builder.getInt32Ty(),2),"fd");
      //Builder.CreateStore(fd_local,fd);
      //Builder.CreateRetVoid();
      Builder.CreateBr(entryMain);
      
      // Close Call
      vector<Type*> closeArgsTypes({Builder.getInt32Ty()});
      //FunctionType *closeCallTy = FunctionType::get(Builder.getVoidTy(),false); 
      //
      /*
      Function *closeCall = Function::Create(closeCallTy,Function::ExternalLinkage,
          "closeCall",&M);
      BasicBlock *closeCallBB = BasicBlock::Create(M.getContext(),"closeCallBB",
          main);
      Builder.SetInsertPoint(closeCallBB);
      FunctionType *closeTy = FunctionType::get(Builder.getInt32Ty(),
          closeArgsTypes,true);
      Function *close = cast<Function>(M.getOrInsertFunction("close",closeTy)
          .getCallee());
      fd_local = Builder.CreateLoad(Builder.getInt32Ty(),fd);
      vector<Value*> closeArgs({fd_local});
      Builder.CreateCall(close,closeArgs);
      Builder.CreateRet();*/
      //appendToGlobalDtors(M,closeCall,0);
      
     
      for(auto &F : M){
        //errs() << F.getName() << F.size() << '\n';
        bool excluded = false;
        bool included = true;
        for(auto entry : include){
          //included = F.getName().str().find(entry) != string::npos | included;
          //errs() << excluded << " " << F.getName().str() << " " << entry << "\n";
        }
        for(auto entry : exclude){
          excluded = F.getName().str().find(entry) != string::npos | excluded;
          //errs() << excluded << " " << F.getName().str() << " " << entry << "\n";
        }
        if(included and !excluded and F.size() > 0 and F.getName() != "closeCall" and
            F.getName() != "startCall"){
          //errs() << F.getName() << F.size() << '\n';
          // Entry
          BasicBlock *BB = &F.getEntryBlock();
          if(F.getName() == "main")
            BB = entryMain;
          Builder.SetInsertPoint(BB->getFirstNonPHI());

          //Sprintf
          GlobalVariable *fName = Builder.CreateGlobalString(F.getName());
          Value *bufferPtr8 = Builder.CreateBitCast(bufferPtr,Builder.getInt8PtrTy());
          Value *fd_local = Builder.CreateLoad(Builder.getInt32Ty(),fd);
          vector<Value*> sprintfArgs1({bufferPtr8,sprintfStringEntry,fName});
          Value *size = Builder.CreateCall(sprintf,sprintfArgs1);
         
          //Write 
          Value *size64 = Builder.CreateZExtOrBitCast(size,Builder.getInt64Ty());
          vector<Value*> writeArgs1({fd_local,bufferPtr8,size64});
          Builder.CreateCall(write,writeArgs1);

          // Exit
          Builder.SetInsertPoint(BB->getTerminator());
          vector<Value*> sprintfArgs2({bufferPtr8,sprintfStringExit,fName});
          size = Builder.CreateCall(sprintf,sprintfArgs2);
         
          //Write 
          size64 = Builder.CreateZExtOrBitCast(size,Builder.getInt64Ty());
          vector<Value*> writeArgs2({fd_local,bufferPtr8,size64});
          Builder.CreateCall(write,writeArgs2);
          if(verifyFunction(F)){
            BB->dump();
            //exit(-1);
          }
          //for(auto &BB : F){}
        }
      }

      return true;
    }
	};
}


char callgraphFuncs::ID = 0;
static RegisterPass<callgraphFuncs> a("callgraphFuncs", "Callgraph Pass",
									   false, false);

static RegisterStandardPasses A(
	PassManagerBuilder::EP_EarlyAsPossible,
	[](const PassManagerBuilder &Builder,
	   legacy::PassManagerBase &PM) { PM.add(new callgraphFuncs()); });
