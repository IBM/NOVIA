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
        else{
          SwitchInst *terms = cast<SwitchInst>(term);
          for(int i = 0; i < terms->getNumSuccessors(); ++i)
            if(terms->getSuccessor(i) == BB)
              terms->setSuccessor(i,newBB);
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
  // Remove Debug Intrinsics
  
  vector<Instruction*> rmIntr;
  for(auto &I : *BB)
    if(isa<DbgInfoIntrinsic>(&I))
      rmIntr.push_back(&I);
  for(auto I : rmIntr)
      I->eraseFromParent();
}
