#include"FusedBB.hpp"

void FusedBB::addMergedBB(BasicBlock *BB){
  mergedBBs->insert(BB);
  return;
}

unsigned FusedBB::getNumMerges(){
  return mergedBBs->size();
}
