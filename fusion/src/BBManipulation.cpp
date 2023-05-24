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
#include "BBManipulation.hpp"

/**
 * Transforms conditional branches to unconditional. This is done so operands
 * of conditional branches are considered LiveOut Values.
 * To do so we create an intermediate BB that will contain the original
 * conditional branch.
 *
 * @param *BB Basic Block to tranform branch
 */
// TODO: Merge those ifs
void separateBr(BasicBlock *BB){
  // Deal with PHIs
  if (isa<PHINode>(BB->begin())){
    IRBuilder<> builder(BB->getContext());
    BasicBlock *newBB = BasicBlock::Create(BB->getContext(),"phis"+BB->getName(),
        BB->getParent(),BB);
    builder.SetInsertPoint(newBB);
    builder.CreateBr(BB);
    vector<BasicBlock*> pred_list;
    for(BasicBlock *pred: predecessors(BB))
      if(pred != newBB)
        pred_list.push_back(pred);

    for(BasicBlock *pred : pred_list){
        Instruction *term = pred->getTerminator();
        if(isa<BranchInst>(term)){
          BranchInst *termb = cast<BranchInst>(term);
          for(int i = 0; i < termb->getNumSuccessors(); ++i)
            if(termb->getSuccessor(i) == BB)
              termb->setSuccessor(i,newBB);
        }
        else if(isa<InvokeInst>(term)){
          InvokeInst *termi = cast<InvokeInst>(term);
          for(int i = 0; i < termi->getNumSuccessors(); ++i)
            if(termi->getSuccessor(i) == BB)
              termi->setSuccessor(i,newBB);
        }
        else if(isa<SwitchInst>(term)){
          SwitchInst *terms = cast<SwitchInst>(term);
          for(int i = 0; i < terms->getNumSuccessors(); ++i)
            if(terms->getSuccessor(i) == BB)
              terms->setSuccessor(i,newBB);
        }
        else if(isa<IndirectBrInst>(term)){
          IndirectBrInst *termbr = cast<IndirectBrInst>(term);
          for(int i = 0; i < termbr->getNumSuccessors(); ++i)
            if(termbr->getSuccessor(i) == BB)
              termbr->setSuccessor(i,newBB);
        }
      }
    while(isa<PHINode>(BB->begin()))
      BB->begin()->moveBefore(*newBB,newBB->begin());
  }
  // Deal with Terminators
  Instruction *I = BB->getTerminator();
  if(isa<BranchInst>(I)){
    if(cast<BranchInst>(I)->isConditional()){
      if(isa<Instruction>(I->getOperand(0))){
        Instruction *Ipred = cast<Instruction>(I->getOperand(0));
        MDNode* temp = MDNode::get(BB->getContext(),ArrayRef<Metadata*>());
        Ipred->setMetadata("is.liveout",temp);
      }
      IRBuilder<> builder(BB->getContext());
      BasicBlock *newBB = BasicBlock::Create(BB->getContext(),"sep"+BB->getName(),
        BB->getParent());
      BB->replaceSuccessorsPhiUsesWith(newBB);
      I->moveBefore(*newBB,newBB->begin());
      builder.SetInsertPoint(BB);
      builder.CreateBr(newBB);
    }
  }
  else if(isa<SwitchInst>(I) or isa<InvokeInst>(I)){
    if(isa<Instruction>(I->getOperand(0))){
      Instruction *Ipred = cast<Instruction>(I->getOperand(0));
      MDNode* temp = MDNode::get(BB->getContext(),ArrayRef<Metadata*>());
      Ipred->setMetadata("is.liveout",temp);
    }
    IRBuilder<> builder(BB->getContext());
    BasicBlock *newBB = BasicBlock::Create(BB->getContext(),"sep"+BB->getName(),
      BB->getParent());
    BB->replaceSuccessorsPhiUsesWith(newBB);
    I->moveBefore(*newBB,newBB->begin());
    builder.SetInsertPoint(BB);
    builder.CreateBr(newBB);
  }
  else if(dyn_cast<ReturnInst>(I)){
    if(cast<ReturnInst>(I)->getReturnValue() and 
      isa<Instruction>(cast<ReturnInst>(I)->getReturnValue())){
      if(isa<Instruction>(cast<ReturnInst>(I)->getReturnValue())){
        Instruction *Ipred = cast<Instruction>(cast<ReturnInst>(I)->getReturnValue());
        MDNode* temp = MDNode::get(BB->getContext(),ArrayRef<Metadata*>());
        Ipred->setMetadata("is.liveout",temp);  
      }
    }
    IRBuilder<> builder(BB->getContext());
    BasicBlock *newBB = BasicBlock::Create(BB->getContext(),"sep"+BB->getName(),
      BB->getParent());
    BB->replaceSuccessorsPhiUsesWith(newBB);
    I->moveBefore(*newBB,newBB->begin());
    builder.SetInsertPoint(BB);
    builder.CreateBr(newBB);
  }
}

void KahnSort(BasicBlock *BB){
  map<Instruction*,set<Instruction*>* > rawDeps;
  memRAWDepAnalysis(BB,&rawDeps);


  list<Instruction*> S; // Nodes with no incoming edge
  list<Instruction*> L; // Sorted elements
  map<Instruction*,set<Instruction*> > EdgeList;
  Instruction *Term = BB->getTerminator();

  for(auto &I : *BB){
    bool noincoming = true;
    for(auto U = I.op_begin();U != I.op_end(); U++){
      Instruction *IU = NULL;
      if((IU = dyn_cast<Instruction>(*U))){
         if(IU->getParent() == BB){
           noincoming = false;
         if(EdgeList.count(&I))
          EdgeList[&I].insert(IU);
         else{
          set<Instruction*> tmp;
          tmp.insert(IU);
          EdgeList[&I] = tmp;
          }
        }
      }
    }
    if(rawDeps.count(&I)){
      for(auto rD : *rawDeps[&I])
        EdgeList[rD].insert(&I);
    }
    if(!EdgeList.count(&I))
      S.push_back(&I);
  }


  while(!S.empty()){
    Instruction *A = S.front();
    S.pop_front();
    L.push_back(A);
    for(auto M = A->user_begin();M != A->user_end();++M){
      Instruction *IU = cast<Instruction>(*M);
      if(IU->getParent() == BB){
        EdgeList[IU].erase(A);
        if(EdgeList[IU].empty())
          S.push_back(cast<Instruction>(IU));
      }
    }
    if(rawDeps.count(A)){
      for(auto I = rawDeps[A]->begin(); I != rawDeps[A]->end(); ++I){
        EdgeList[*I].erase(A);
        if(EdgeList[*I].empty())
          S.push_back(*I);
      }
    }
  }

  for(auto I = next(L.begin()); I != L.end(); ++I)
    (*I)->moveAfter(*prev(I));
  Term->moveAfter(&(*BB->rbegin()));

}
