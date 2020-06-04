#include"FusedBB.hpp"

FusedBB::~FusedBB(){
  delete BB;
}

FusedBB::FusedBB(LLVMContext &Context, string name){
  BB = BasicBlock::Create(Context, name);
  mergedBBs = new set<BasicBlock*>; 
  return;
}

void FusedBB::addMergedBB(BasicBlock *BB){
  mergedBBs->insert(BB);
  return;
}

unsigned FusedBB::getNumMerges(){
  return mergedBBs->size();
}
