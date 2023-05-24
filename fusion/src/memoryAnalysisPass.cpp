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
#include <stack>

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

namespace {
	struct memoryAnalysisFuncs : public ModulePass {
		static char ID;

	  memoryAnalysisFuncs() : ModulePass(ID) {}

    void genDepChain(Instruction *I, map<Instruction*,int> &Ilist, bool *depMatrix){
      BasicBlock *BB = I->getParent();
      stack<Instruction*> DFS;
      int source = Ilist[I];
      int size = Ilist.size();
      DFS.push(I);
      while(!DFS.empty()){
        I = DFS.top();
        DFS.pop();
        for(auto *U : I->users()){
          Instruction *User = cast<Instruction>(U);
          if(User->getParent() == BB){
            DFS.push(User);
            if(Ilist.count(User)){
              int sink = Ilist[User];
  
              depMatrix[source*size+sink] = true;
            }
          }
        }  
      }
    }

		bool runOnModule(Module &M) override {
      IRBuilder<> Builder(M.getContext());
      IRBuilder<> initBuilder(M.getContext());
      DataLayout DL = M.getDataLayout();

      // New Function memory matrix definition
      FunctionType *initFuncTy = FunctionType::get(Builder.getVoidTy(),ArrayRef<Type*>(),false);
      Function *initFunc = Function::Create(initFuncTy,Function::ExternalLinkage,"novia_decFunc",&M);
      // OpenFile Novia Support initialization
      Function *initFile = Function::Create(initFuncTy,Function::ExternalLinkage,"init_noviaMemAnalysis",&M);
      appendToGlobalCtors(M,initFunc,0);
      appendToGlobalCtors(M,initFile,0);
      BasicBlock *startInitFunc = BasicBlock::Create(M.getContext(),"startInitFunc",
          initFunc);
      // Register memory dep metrix
      SmallVector<Type*,4> regGraphInputsTy;
      regGraphInputsTy.push_back(Builder.getInt1Ty()->getPointerTo());
      regGraphInputsTy.push_back(Builder.getInt64Ty()->getPointerTo());
      regGraphInputsTy.push_back(Builder.getInt8Ty()->getPointerTo());
      regGraphInputsTy.push_back(Builder.getInt32Ty());
      FunctionType *regGraphTy = FunctionType::get(Builder.getVoidTy(),ArrayRef<Type*>(regGraphInputsTy),false);
      Function *regGraph = Function::Create(regGraphTy,Function::ExternalLinkage,"novia_regGraph",&M);

      set<string> excludedFuncs;
      excludedFuncs.insert("novia_decFunc");

      // Register Input/Ouput Matrices per function
      for(auto &F: M){
        //Value *fName = Builder.CreateGlobalStringPtr(F.getName());
        //vector<Value*> funcNameArgs({fName});
        //Builder.CreateCall(initFunc,funcNameArgs);
        if(not excludedFuncs.count(F.getName().str())){
          for(auto &BB: F){
            int memLocations = 0;
            map<Instruction*,int> Ilist;
            Builder.SetInsertPoint(&BB);
            initBuilder.SetInsertPoint(startInitFunc);
  
            // Count the memory references
            for(auto &I: BB)
              if(I.mayReadOrWriteMemory() and not isa<CallInst>(&I)){
                Ilist[&I] = memLocations;
                memLocations++;
              }
  
  
            if(memLocations){
              // Create Matrices and Arrays of each BB
              Value* DepMatrix = new GlobalVariable(M,
                  ArrayType::get(Builder.getInt1Ty(),memLocations*memLocations),
                  false,
                  GlobalValue::LinkageTypes::ExternalLinkage,
                  Constant::getNullValue(ArrayType::get(Builder.getInt1Ty(),memLocations*memLocations)),
                  "DepMatrix");
              
              Value* AddrArray = new GlobalVariable(M,
                  ArrayType::get(Builder.getInt64Ty(),memLocations),
                  false,
                  GlobalValue::LinkageTypes::ExternalLinkage,
                  Constant::getNullValue(ArrayType::get(Builder.getInt64Ty(),memLocations)),
                  "AddrArray");
              
              
              Value* SizeArray = new GlobalVariable(M,
                  ArrayType::get(Builder.getInt8Ty(),memLocations),
                  false,
                  GlobalValue::LinkageTypes::ExternalLinkage,
                  Constant::getNullValue(ArrayType::get(Builder.getInt8Ty(),memLocations)),
                  "SizeArray");

              unsigned int i = 0;
              int footprint = 0;
              for(auto &I: BB){
                if(Ilist.count(&I)){
                  Builder.SetInsertPoint(I.getNextNonDebugInstruction());
                  Value *PtrOp;
                  if(isa<StoreInst>(&I)){
                    footprint = DL.getTypeSizeInBits(cast<StoreInst>(&I)
                        ->getValueOperand()->getType());
                    PtrOp = cast<StoreInst>(&I)->getPointerOperand();
                  }
                  else if(isa<LoadInst>(&I)){
                    footprint = DL.getTypeSizeInBits(I.getType());
                    PtrOp = cast<LoadInst>(&I)->getPointerOperand();
                  }

                  bool *tmp_depMatrix = new bool[memLocations*memLocations]();
                  genDepChain(&I,Ilist,tmp_depMatrix);
                  for(int indexi = 0; indexi < memLocations ; indexi++){
                    for(int indexj = 0; indexj < memLocations ; indexj++){
                      if(tmp_depMatrix[indexi*memLocations+indexj]){
                        Value *depPtr = initBuilder.CreateGEP(ArrayType::get(initBuilder.getInt1Ty(),memLocations*memLocations),DepMatrix,ArrayRef<Value*>({initBuilder.getInt8(0),initBuilder.getInt8(indexi*memLocations+indexj)}));
                        Value *depPtr2 = initBuilder.CreateBitCast(depPtr,initBuilder.getInt1Ty()->getPointerTo());
                        initBuilder.CreateStore(initBuilder.getInt1(true),depPtr2);
                      }
                    }
                  }
    
                  // TODO: More efficient data structure addressing. Move GEP instructions to init and make them global values so they are only generated once
                  Value *SizePtr = initBuilder.CreateGEP(ArrayType::get(initBuilder.getInt8Ty(),memLocations),SizeArray,ArrayRef<Value*>({initBuilder.getInt8(0),initBuilder.getInt8(i)}));
                  Value *SizePtr2 = initBuilder.CreateBitCast(SizePtr,initBuilder.getInt8PtrTy());
                  
                  Value *AddrPtr = Builder.CreateGEP(ArrayType::get(initBuilder.getInt64Ty(),memLocations),AddrArray, ArrayRef<Value*>({initBuilder.getInt8(0),initBuilder.getInt8(i)}));
                  Value *AddrPtr2 = Builder.CreateBitCast(AddrPtr,initBuilder.getInt64Ty()->getPointerTo());
    
    
                  initBuilder.CreateStore(initBuilder.getInt8(footprint),SizePtr2);
                  PtrOp = Builder.CreatePtrToInt(PtrOp,Builder.getInt64Ty());
                  Builder.CreateStore(PtrOp,AddrPtr2);
                  
                  i++;
                }  
              }
              SmallVector<Value*,4> regGraphInputs;
              regGraphInputs.push_back(initBuilder.CreateBitCast(DepMatrix,initBuilder.getInt1Ty()->getPointerTo()));
              regGraphInputs.push_back(initBuilder.CreateBitCast(AddrArray,initBuilder.getInt64Ty()->getPointerTo()));
              regGraphInputs.push_back(initBuilder.CreateBitCast(SizeArray,initBuilder.getInt8PtrTy()));
              regGraphInputs.push_back(Builder.getInt32(memLocations));
              Builder.CreateCall(regGraphTy,regGraph,
                  ArrayRef<Value*>(regGraphInputs));
            }
          }
        }
      }
      initBuilder.CreateRetVoid();



      return true;
    }
	};
}


char memoryAnalysisFuncs::ID = 0;
static RegisterPass<memoryAnalysisFuncs> a("memoryAnalysisFuncs", "PiM Pattern Analysis",
									   false, false);

static RegisterStandardPasses A(
	PassManagerBuilder::EP_EarlyAsPossible,
	[](const PassManagerBuilder &Builder,
	   legacy::PassManagerBase &PM) { PM.add(new memoryAnalysisFuncs()); });
