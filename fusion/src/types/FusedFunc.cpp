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
#include "FusedFunc.hpp"


void FusedFunc::init(){
  minLength = -1;
  maxLength = 0;
  depChain = new vector<vector<Instruction*> *>;
}

FusedFunc::~FusedFunc(){
  for(auto elem : *depChain)
    delete elem;
  delete depChain;
}

FusedFunc::FusedFunc(Function *F){
  init();
  this->F = F;
  BasicBlock *Acc = &F->getEntryBlock(); 
  SetVector<Value*> LiveIn, LiveOut;
  liveInOut(*Acc,&LiveIn,&LiveOut);

  // Construct BFS for each LiveOut
  for(auto var : LiveOut){
    depChain->push_back(new vector<Instruction*>);
    getDepChain(cast<Instruction>(var),(*depChain)[depChain->size()-1]);
    minLength = (*depChain)[depChain->size()-1]->size() < minLength? 
      (*depChain)[depChain->size()-1]->size() : minLength;
    maxLength = (*depChain)[depChain->size()-1]->size() > maxLength? 
      (*depChain)[depChain->size()-1]->size() : maxLength;
  }
  return;
}

/*
 * BFS exploration of dependency chain of an instruction within a BB
 */
void FusedFunc::getDepChain(Instruction *I, vector<Instruction*> *queue){
  int index = 0;
  queue->push_back(I);
  do{
    I = (*queue)[index++];
    for(auto Op : I->operand_values()){
      if(isa<Instruction>(Op) and 
          I->getParent() == cast<Instruction>(Op)->getParent()){
        queue->push_back(cast<Instruction>(Op));
      }
    }
    if(I->isCommutative())
      std::sort(queue->begin()+index,queue->end(),sortInst);
  }while(index != queue->size());
  return;
}
/*
 * Assess if two trees are the equal
 * TODO: Selects/Commutative
 */
bool FusedFunc::match(vector<Instruction*> *chain1, vector<Instruction*> *chain2){
  bool match = true;
  unsigned index = 0;
  while(match and index < chain1->size() and index < chain2->size()){
    match = (*chain1)[index]->getType() == (*chain2)[index]->getType() and
            (*chain1)[index]->getOpcode() == (*chain2)[index]->getOpcode();
    index++;
  }
  if(!match)
    LLVM_DEBUG(dbgs() << "Dbg: Failed match; Matched instructions: " << index-1 << "\n");
  else
    LLVM_DEBUG(dbgs() << "Dbg: Succesful match; Matched instructions: " << index << "\n");

  return match;
}

/*
 * Return the number succesful matched paths
 */
unsigned int FusedFunc::reuse(BasicBlock *BB){
  int matchedPaths = 0;
  SetVector<Value*> LiveIn, LiveOut;
  liveInOut(*BB,&LiveIn,&LiveOut);
  vector<vector<Instruction*> *> tmpChain;

  unsigned counter = 1;
  for(auto &I : *BB){
    if(counter >= minLength){
      tmpChain.push_back(new vector<Instruction*>);
      getDepChain(&I,tmpChain[tmpChain.size()-1]);
    }
    counter++;
  }

  LLVM_DEBUG(dbgs() << "Dbg: Searching Func: " << F->getName() << " in BB: "<< BB->getName() << " for " << tmpChain.size() << " potential matches: " << "\n");

  
  vector<bool> inlineUse(depChain->size(),false);
  vector<bool> matchingUse(tmpChain.size(),false);
  for(auto [iCit, iCindex] = tuple{depChain->begin(),0}; iCit != depChain->end() ;iCit++, iCindex++){
    if(!inlineUse[iCindex]){
      for(auto [mCit, mCindex]=tuple{tmpChain.begin(),0};mCit!=tmpChain.end();mCit++,mCindex++){
        if(!matchingUse[mCindex]){
          if(match(*iCit,*mCit)){
            inlineUse[iCindex] = matchingUse[mCindex] = true;
            matchedPaths++;
          }
        }
      }
    }
  }

  return matchedPaths;
}

