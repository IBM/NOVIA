#include "BBAnalysis.hpp"


/**
 * Compute the memoryfootprint of a function and embedded that info as
 * Metadata
 *
 * @param F Function to Analyze
 * @return void
 */
void memoryFootprintF(Function *F){
	LLVMContext &Context = F->getContext();
	DataLayout DL  = F->getParent()->getDataLayout();
	int footprint = 0;
	for(auto &A: F->args()){
		footprint += DL.getTypeSizeInBits(A.getType());
	}
	MDNode* temp = MDNode::get(Context,ConstantAsMetadata::get(
				ConstantInt::get(Context,llvm::APInt(64,footprint,false))));
	MDNode* N = MDNode::get(Context, temp);
	F->setMetadata("stat.memoryFootprint",N);
}


/**
 * Computes the variables that are LiveIn in a BasicBlock
 *
 * @param BB BasicBlock to Analyze
 * @return List of live out Variables
 */
void liveInOut(BasicBlock &BB, SetVector<Value*> *LiveIn,
			   SetVector<Value*> *LiveOut){
	using ValueSet = SetVector<Value *>;
	ArrayRef<BasicBlock*> BBs(&BB);
	CodeExtractor CE = CodeExtractor(BBs);

	ValueSet Allocas;
	ValueSet Li, Lo;

	CE.findInputsOutputs((ValueSet&)*LiveIn,(ValueSet&)*LiveOut,Allocas);
  // We consider the branch conditions to be a liveOut variable since we cannot
  // branc from an offload functions, and it's value might be computed inside
  if(auto *I = dyn_cast_or_null<BranchInst>(BB.getTerminator()))
    if(I->isConditional())
      LiveOut->insert(cast<Value>(I->getOperand(0)));
  // Annotate liveout values are added now
  for(auto I=BB.begin(), E=BB.end(); I != E; ++I){
    if(I->getMetadata("is.liveout"))
      LiveOut->insert(cast<Value>(I));
  }
	for(Value *V: *LiveIn){
		errs() << "Live In Value " << V->getName() << '\n';
	}
	for(Value *V: *LiveOut){
		errs() << "Live Out Value " << V->getName() << '\n';
	}
	//for(Value *V: Allocas){
	//	errs() << "Alloc " << V->getName() << '\n';
	//}
	//errs() << LiveIn << '\n';
	return;
}

void buildDAG(BasicBlock &BB, DirectedGraph<SimpleDDGNode,DDGEdge> *G){
	for(Instruction &I : BB){
		SimpleDDGNode N = SimpleDDGNode(I);
		G->addNode(N);
	}
	errs() << "PRINTING GRAPH\n";
	errs() << G;
	return;
}
