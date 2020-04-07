#include "BBAnalysis.hpp"





/**
 * Returns the estimate cost in area of an instruction
 *
 * @param *I Instruction to estimate
 */
int areaEstimate(Instruction &I){
  //TODO: Change this to be input file and not hardcode in an indexed map or something
  int cost = 0;
  if(string(I.getOpcodeName()) == "and"){cost=1;}
  else if(string(I.getOpcodeName()) == "shl"){cost=1;}
  else if(string(I.getOpcodeName()) == "or"){cost=1;}
  else if(string(I.getOpcodeName()) == "add"){cost=1;}
  else if(string(I.getOpcodeName()) == "ashr"){cost=1;}
  else if(string(I.getOpcodeName()) == "lshr"){cost=1;}
  else if(string(I.getOpcodeName()) == "xor"){cost=1;}
  else if(string(I.getOpcodeName()) == "sub"){cost=3;}
  else if(string(I.getOpcodeName()) == "icmp"){cost=3;}
  else if(string(I.getOpcodeName()) == "fadd"){cost=4;}
  else if(string(I.getOpcodeName()) == "mul"){cost=4;}
  else if(string(I.getOpcodeName()) == "fmul"){cost=4;}
  else if(string(I.getOpcodeName()) == "fcmp"){cost=4;}
  else if(string(I.getOpcodeName()) == "fsub"){cost=4;}
  else if(string(I.getOpcodeName()) == "srem"){cost=50;}
  else if(string(I.getOpcodeName()) == "urem"){cost=50;}
  else if(string(I.getOpcodeName()) == "div"){cost=50;}
  else if(string(I.getOpcodeName()) == "sdiv"){cost=50;}
  else if(string(I.getOpcodeName()) == "udiv"){cost=50;}
  else if(string(I.getOpcodeName()) == "fdiv"){cost=120;}
  else if(string(I.getOpcodeName()) == "frem"){cost=120;}
  // TODO: Get the values from a reliable place this is a place holder to hide the error messages 
  // Missing Opcodes
  else if(string(I.getOpcodeName()) == "alloca"){cost=1;}
  else if(string(I.getOpcodeName()) == "getelementptr"){cost=1;}
  else if(string(I.getOpcodeName()) == "call"){cost=1;}
  else if(string(I.getOpcodeName()) == "br"){cost=1;}
  else if(string(I.getOpcodeName()) == "trunc"){cost=1;}
  else if(string(I.getOpcodeName()) == "zext"){cost=1;}
  else if(string(I.getOpcodeName()) == "sext"){cost=1;}
  else if(string(I.getOpcodeName()) == "select"){cost=1;}
  else if(string(I.getOpcodeName()) == "load"){cost=1;}
  else if(string(string(I.getOpcodeName())) == "store"){cost=1;}
  else {errs() << "missing op " << I.getOpcodeName() << "\n"; cost=1;}

  return cost;
}

/*
 * Returns the estimate cost in energy of an instruction
 *
 * @param *I Instruction to estimate
 */
int energyEstimate(Instruction &I){
  return 0;
}


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

/**
 * Attaches the different metrics of a BB to it's terminator instruction
 *
 * @param *BB BasicBlock to analyze and attach metadata
 */
void getMetadataMetrics(BasicBlock *BB, vector<int> *data, Module *M){
  MDNode *N;
	DataLayout DL  = M->getDataLayout();
  int footprint = 0;
  int area = 0;
  int energy = 0;
  int max_merges = 0;
  for(auto I = BB->begin(); I != BB->end() ; ++I){
    if (isa<StoreInst>(I) or isa<LoadInst>(I)){
      if(isa<StoreInst>(I))
        footprint += DL.getTypeSizeInBits(I->getOperand(0)->getType());
      else
        footprint += DL.getTypeSizeInBits(I->getType());
    }
    if (N = (I->getMetadata("fuse.merged"))){
      int tmp = cast<ConstantInt>(cast<ConstantAsMetadata>(N->getOperand(0))->getValue())
        ->getSExtValue();
      max_merges = tmp>max_merges?tmp:max_merges;

    }
    area += areaEstimate(*I); 
    energy += energyEstimate(*I);
  }
  data->push_back(BB->size());
  data->push_back(area);
  data->push_back(energy);
  data->push_back(footprint);
  data->push_back(max_merges);
  errs() << BB->getName() << " " << BB->size() << " " << area << " " << energy << " " << footprint << " " << max_merges << "\n";
  return;
}

/**
 * Attaches the different metrics of a BB to it's terminator instruction
 *
 * @param *BB BasicBlock to analyze and attach metadata
 */
void addMetadataMetrics(BasicBlock *BB){
	LLVMContext &Context = BB->getContext();
  MDNode *N;
  Instruction *term = BB->getTerminator();
	DataLayout DL  = BB->getParent()->getParent()->getDataLayout();
  int footprint = 0;
  int area = 0;
  int energy = 0;
  int max_merges = 0;
  for(auto I = BB->begin(); I != BB->end() ; ++I){
    if (isa<StoreInst>(I) or isa<LoadInst>(I)){
      if(isa<StoreInst>(I))
        footprint += DL.getTypeSizeInBits(I->getOperand(0)->getType());
      else
        footprint += DL.getTypeSizeInBits(I->getType());
    }
    if (N = (I->getMetadata("fuse.merged"))){
      int tmp = cast<ConstantInt>(cast<ConstantAsMetadata>(N->getOperand(0))->getValue())
        ->getSExtValue();
      max_merges = tmp>max_merges?tmp:max_merges;

    }
    area += areaEstimate(*I); 
    energy += energyEstimate(*I);
  }
	N = MDNode::get(Context,ConstantAsMetadata::get(ConstantInt::get(Context,llvm::APInt(64,footprint,false))));
	term->setMetadata("fuse.stat.mem",N);
  N = MDNode::get(Context,ConstantAsMetadata::get(ConstantInt::get(Context,llvm::APInt(64,area,false))));
	term->setMetadata("fuse.stat.area",N);
	N = MDNode::get(Context,ConstantAsMetadata::get(ConstantInt::get(Context,llvm::APInt(64,energy,false))));
	term->setMetadata("fuse.stat.energy",N);
	N = MDNode::get(Context,ConstantAsMetadata::get(ConstantInt::get(Context,llvm::APInt(64,max_merges,false))));
	term->setMetadata("fuse.stat.max_merge",N);
  return;
}

void dumpAddedMetadataMetrics(BasicBlock *BB){
  MDNode *N;
  int footprint, area, energy, max_merges;
  footprint = area = energy = max_merges = -1;
  Instruction *term = BB->getTerminator();
  if(N = (term->getMetadata("fuse.stat.mem"))){
    footprint = cast<ConstantInt>(cast<ConstantAsMetadata>(N->getOperand(0))->getValue())->getSExtValue();
  }
  if(N = (term->getMetadata("fuse.stat.area"))){
    area = cast<ConstantInt>(cast<ConstantAsMetadata>(N->getOperand(0))->getValue())
      ->getSExtValue();
  }
  if(N = (term->getMetadata("fuse.stat.energy"))){
    energy = cast<ConstantInt>(cast<ConstantAsMetadata>(N->getOperand(0))->getValue())
      ->getSExtValue();
  }
  if(N = (term->getMetadata("fuse.stat.max_merge"))){
    max_merges = cast<ConstantInt>(cast<ConstantAsMetadata>(N->getOperand(0))->getValue())
      ->getSExtValue();
  }

  errs() << BB->getName() << " " << BB->size() << " " << area << " " << energy << " " << footprint << " " << max_merges << "\n";
  return;
}


void getAddedMetadataMetrics(BasicBlock *BB, vector<int> *data){
  MDNode *N;
  int footprint, area, energy, max_merges;
  footprint = area = energy = max_merges = -1;
  Instruction *term = BB->getTerminator();
  if(N = (term->getMetadata("fuse.stat.mem"))){
    footprint = cast<ConstantInt>(cast<ConstantAsMetadata>(N->getOperand(0))->getValue())->getSExtValue();
  }
  if(N = (term->getMetadata("fuse.stat.area"))){
    area = cast<ConstantInt>(cast<ConstantAsMetadata>(N->getOperand(0))->getValue())
      ->getSExtValue();
  }
  if(N = (term->getMetadata("fuse.stat.energy"))){
    energy = cast<ConstantInt>(cast<ConstantAsMetadata>(N->getOperand(0))->getValue())
      ->getSExtValue();
  }
  if(N = (term->getMetadata("fuse.stat.max_merge"))){
    max_merges = cast<ConstantInt>(cast<ConstantAsMetadata>(N->getOperand(0))->getValue())
      ->getSExtValue();
  }
  data->push_back(BB->size());
  data->push_back(area);
  data->push_back(energy);
  data->push_back(footprint);
  data->push_back(max_merges);

  return;
}
