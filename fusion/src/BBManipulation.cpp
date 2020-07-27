#include "BBManipulation.hpp"


// TODO: Optimize all getContext into just 1 and send it as parameter
// This func is only to differentiate fuse.orig from fuseorig old
// basicaly to identify if a B instruction is merged, whats its correspondant
// instruction in A
void linkInsOld(Instruction *newIns, Instruction *oldIns){
	SmallVector<Metadata*,32> Ops;
  Ops.push_back(ValueAsMetadata::get(cast<Value>(oldIns)));
  MDNode *N = MDTuple::get(oldIns->getParent()->getContext(),Ops);
  newIns->setMetadata("fuse.origold",N);
  return;
}

void linkIns(Instruction *newIns, Instruction *oldIns){
	SmallVector<Metadata*,32> Ops;
  Ops.push_back(ValueAsMetadata::get(cast<Value>(oldIns)));
  MDNode *N = MDTuple::get(oldIns->getParent()->getContext(),Ops);
  newIns->setMetadata("fuse.orig",N);
  return;
}


// TODO: Sink on loads
int getStoreDomain(Instruction *I, BasicBlock *BB){
  int domain = 1; // Default value with no stores>?
  MDNode* N;
  if((N = I->getMetadata("fuse.storedomain"))){
    return cast<ConstantInt>(cast<ConstantAsMetadata>(
          N->getOperand(0))->getValue())->getSExtValue();
  }
  for(auto user = I->user_begin(); user != I->user_end() and !domain; ++user){
    Instruction *Up = (Instruction*)*user;
    if(Up->getParent() == BB){
      domain = getStoreDomain(Up,BB);
    }
  }
  return domain;
}


/**
 *Generate safe loads
 *
 *
 */
void secureMem(BasicBlock *BB, Value *SelVal, IRBuilder<> &builder, BasicBlock *destBB, Module *Mod){
  for(Instruction &I : *BB){
    // safe value of 1 means the load has been merged with all BBs and it's safe to use since
    // at least 1 instance will provide always a safe pointer
    // safe value of 2 means that the load was secured on a previous merge step and therefore
    // there exists a safe pointer in case the load is not used, hence no need to add it again
    if((isa<LoadInst>(&I) or isa<StoreInst>(&I)) and !I.getMetadata("fuse.issafe")){
      MDNode* safe2MD = MDNode::get(builder.getContext(),ConstantAsMetadata::get(
              ConstantInt::get(builder.getContext(),APInt(64,2,false))));
      Value *oPtr = NULL;
      LoadInst *Li = NULL;
      StoreInst *Si = NULL;
      int index = -1;
      if(Li = (dyn_cast<LoadInst>(&I))){
        oPtr = Li->getPointerOperand();
        index = LoadInst::getPointerOperandIndex();
      }
      else if(Si = (dyn_cast<StoreInst>(&I))){
        oPtr = Si->getPointerOperand();
        index = StoreInst::getPointerOperandIndex();
      }
      if(Li or Si){
        Value *SelSafe;
        Value *G = getSafePointer(cast<PointerType>(oPtr->getType()),Mod);
        // If the instruction comes from B we put in normal order, otherwise we swap
        Value *orig = cast<ValueAsMetadata>(I.getMetadata("fuse.orig")->getOperand(0))->getValue();
        if(cast<Instruction>(orig)->getParent() ==  destBB)
          SelSafe = builder.CreateSelect(SelVal,oPtr,G,"fuse.sel.safe");
        else
          SelSafe = builder.CreateSelect(SelVal,G,oPtr,"fuse.sel.safe");

        cast<Instruction>(SelSafe)->moveBefore(&I);
        linkArgs(SelSafe,destBB);
        I.setMetadata("fuse.issafe",safe2MD);
        I.setOperand(index,SelSafe);          
      }
    }    
  }
}




/**
 * Check wether merging this two instructions would create a cycle in the dep graph
 *
 * @param Ia Instruction from BB A to merge
 * @param Ib Instruction from BB B to merge
 * @param *C BasicBlock to merge
 */
bool checkNoLoop(Instruction &Iref, Instruction &Isearch, BasicBlock *BB){
  bool noloop = true;
  MDNode *N = NULL;
  //if((N = Isearch.getMetadata("fuse.orig")) and (Isearch.getParent() == BB)){
  if(&Iref == &Isearch)
    return false;
  else{
      for(auto user = Isearch.user_begin(); user!= Isearch.user_end(); user++){
        Instruction *Up = (Instruction*)*user;
        if((N = Up->getMetadata("fuse.origold"))){
          Instruction *Iorig = cast<Instruction>(cast<ValueAsMetadata>(N->getOperand(0))->getValue());
          if(Iorig->getParent() == BB)
            noloop &= checkNoLoop(Iref,*Iorig,BB);  
        }
        else if(Up->getParent() == Isearch.getParent())
          noloop &= checkNoLoop(Iref,*Up,BB);
      }
      if((N = Isearch.getMetadata("fuse.rawdep"))){
        Instruction *RAWdep = cast<Instruction>(cast<ValueAsMetadata>(N->getOperand(0))->getValue());
        noloop &= checkNoLoop(Iref,*RAWdep,BB);
      }
    }

  return noloop;
}


vector<pair<Instruction*,int> > merge_vecs(vector< vector< pair<Instruction*,int> > > &recursions,
    Instruction *I, int tier){
  bool finished = false;
  vector<pair<Instruction*, int> > sorted;
  vector<int> index(recursions.size(),0);
  while(!finished or I){
    int i = 0;
    int cur_i = -1;
  int cur_tier = 10000; //TODO: implement better initialization

    assert(tier < cur_tier and "recursion exceeds initialization");
    Instruction *cur_ins = NULL;
    while(i < recursions.size()){
      if(index[i] < recursions[i].size() and recursions[i][index[i]].second <= cur_tier){
        cur_i = i;
        cur_tier = recursions[i][index[i]].second;
        cur_ins = recursions[i][index[i]].first;
      }
      ++i;
    }
    if(I and tier < cur_tier){
       cur_tier = tier;
       cur_ins = I;
    }
    if(cur_ins != I)
      index[cur_i]++;
    else
      I = NULL;

    sorted.push_back(pair<Instruction*,int>(cur_ins,cur_tier));

    i = 0;
    finished = true;
    while(i < recursions.size()){
      finished &= index[i] >= recursions[i].size();
      ++i;
    }
  }
  return sorted;

}


/**
 * Explores the instruction dependence tree and returns a sorted instruction list 
 *
 * @param *I insertion point t
 * @param *BB BasicBlock to sort
 * @return Sorted list of consumer execution
 */

vector<pair<Instruction*,int> > bfsBB(Instruction *I, BasicBlock *BB, int tier,
    set<Instruction*> *visited){
    MDNode *N;
    vector<vector<pair<Instruction*, int> > > recursions;

    if(I->getParent() == BB and !visited->count(I)){

      visited->insert(I);
      for(int i =0; i < I->getNumOperands(); ++i){
        Instruction *Op;
        if((Op = dyn_cast<Instruction>(I->getOperand(i))))
            recursions.push_back(bfsBB(Op,BB,tier-1,visited));
      }
      if((N = I->getMetadata("fuse.wardep"))){
          Instruction *WARdep = cast<Instruction>(cast<ValueAsMetadata>(N->getOperand(0))->getValue());
          recursions.push_back(bfsBB(WARdep,BB,tier+1,visited));
      }

      for(auto user = I->user_begin(); user != I->user_end();  ++user){
        Instruction *Up = (Instruction*)*user;
        recursions.push_back(bfsBB(Up,BB,tier+1,visited));
      }
      if((N = I->getMetadata("fuse.rawdep"))){
          Instruction *RAWdep = cast<Instruction>(cast<ValueAsMetadata>(N->getOperand(0))->getValue());
          recursions.push_back(bfsBB(RAWdep,BB,tier+1,visited));
      }
    }
    return merge_vecs(recursions, I, tier);

/*
    for(auto user = I->user_beg
      if (Up->getParent() == BB and !visited->count(Up))
        Up->moveAfter(I);
    }

    for(int i =0; i < I->getNumOperands(); ++i){
      Instruction *Op;
      if((Op = dyn_cast<Instruction>(I->getOperand(i))) and !visited->count(Op))
        if(Op->getParent() == BB)
          Op->moveBefore(I);
    }
      return;*/
}


/* Topological sort Kahn's algorithm */
void KahnSort(BasicBlock *BB){
  list<Instruction*> S; // Nodes with no incoming edge
  list<Instruction*> L; // Sorted elements
  map<Instruction*,set<Instruction*> > EdgeList;

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
    if(noincoming)
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
    // TODO: SOLVE FOR MORE THAN 1 RAW DEP!!!!! TODO TODO TODO ERROR
    MDNode *N = NULL;
    if((N = A->getMetadata("fuse.rawdep"))){
      Instruction *RAWdep = cast<Instruction>(cast<ValueAsMetadata>(N->getOperand(0))->getValue());
      EdgeList[RAWdep].erase(A);
      if(EdgeList[RAWdep].empty())
        S.push_back(RAWdep);
    }
  }
  
  for(auto I = next(L.begin()); I != L.end();++I)
    (*I)->moveAfter(*prev(I));

  return;
}

/**
 * Check Consistency reorders instructions that were embedded in between their consujmer and 
 * producer
 *
 * @param *C BasicBlock to reorder
 */
void sortBB(BasicBlock *BB){
  set<Instruction*> visited;
  vector<vector<pair<Instruction*,int> > > total_order;

  auto it = BB->begin();
  while( visited.size() != BB->size()){
    if(!visited.count(&(*it)))
      total_order.push_back(bfsBB(&(*it),BB,1,&visited));
    it++;
  }

  for(int i = 0; i < total_order.size(); ++i){
      for(int j = 0; j < total_order[i].size()-1 ; ++j){
        total_order[i][j+1].first->moveAfter(total_order[i][j].first);
      }
  }

  return;
}

/**
 * This function recursively explores the dependencies of an instruction to find where is the 
 * latest producer of it's operands. The instruction must be inserted after that.
 *
 * @param *I Instruction to search dependences 
 * @param *SubOp Map of substituted values in the new basicblock
 * @param *C merged BasicBlock to explore dependences to 
 *
 */
Instruction *findOpDef(Instruction *I, map<Value*,Value*> *SubOp, BasicBlock *C){
  set<Value*> OpSet;
  Instruction *iPoint = NULL;

  for(int i = 0 ; i < I->getNumOperands();++i){
    if(SubOp->count(I->getOperand(i)))
      OpSet.insert((*SubOp)[I->getOperand(i)]);
  }
  //TODO:  Optimize to stop when NumOps founded
  for(auto &Ic : *C){
    if(OpSet.count(cast<Value>(&Ic))){
      iPoint = &Ic;
    }
  }    
  
  return iPoint;
}

/**
 * This function finds the first Use of a non merged instruction in the merged
 * BB
 *
 * @param *Ib Instruction to find use of in C
 * @param *BBorig Original basic block
 * @param *BB Basic Block to find use of
 */
Instruction *findFirstUse(Instruction *I, BasicBlock *BBorig,BasicBlock *BB){
  Instruction *fU = NULL;
  if ( I->getParent() == BBorig ){
    for(auto U = I->user_begin(); U != I->user_end() and !fU; ++U){
      if( cast<Instruction>(*U)->getParent() == BB ){
        fU = cast<Instruction>(*U);
      }
      else{
        fU = findFirstUse(cast<Instruction>(*U),BBorig,BB);
      }   
    }
  }
  return fU;
}


/**
 * Manages the number of times this instructions was merged
 *
 * @param *I Merged Instruction to annotate
 */
// TODO: Find a more efficient way to increment integer metadata
void annotateMerge(Instruction *I){
  MDNode *N;
  long merged = 1;
  if( N = (I->getMetadata("fuse.merged"))){
    merged += cast<ConstantInt>(cast<ConstantAsMetadata>
		 	  	(N->getOperand(0))->getValue())->getSExtValue();
  }
  N = MDNode::get(I->getParent()->getContext(),ConstantAsMetadata::get(
	  ConstantInt::get(I->getParent()->getContext(),llvm::APInt(64,merged,false))));
	I->setMetadata("fuse.merged",N);
  return;
}


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
          BranchInst *termb = cast<BranchInst>(pred->getTerminator());
          for(int i = 0; i < termb->getNumSuccessors(); ++i)
            if(termb->getSuccessor(i) == BB)
              termb->setSuccessor(i,newBB);
        }
        else{
          SwitchInst *terms = cast<SwitchInst>(pred->getTerminator());
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
      Instruction *Ipred = cast<Instruction>(I->getOperand(0));
      MDNode* temp = MDNode::get(BB->getContext(),ArrayRef<Metadata*>());
      Ipred->setMetadata("is.liveout",temp);
      IRBuilder<> builder(BB->getContext());
      BasicBlock *newBB = BasicBlock::Create(BB->getContext(),"sep"+BB->getName(),
        BB->getParent());
      BB->replaceSuccessorsPhiUsesWith(newBB);
      I->moveBefore(*newBB,newBB->begin());
      builder.SetInsertPoint(BB);
      builder.CreateBr(newBB);
    }
  }
  else if(isa<SwitchInst>(I)){
    Instruction *Ipred = cast<Instruction>(I->getOperand(0));
    MDNode* temp = MDNode::get(BB->getContext(),ArrayRef<Metadata*>());
    Ipred->setMetadata("is.liveout",temp);
    IRBuilder<> builder(BB->getContext());
    BasicBlock *newBB = BasicBlock::Create(BB->getContext(),"sep"+BB->getName(),
      BB->getParent());
    BB->replaceSuccessorsPhiUsesWith(newBB);
    I->moveBefore(*newBB,newBB->begin());
    builder.SetInsertPoint(BB);
    builder.CreateBr(newBB);
  }
  else if(dyn_cast<ReturnInst>(I) and cast<ReturnInst>(I)->getReturnValue()){
    if(isa<Instruction>(cast<ReturnInst>(I)->getReturnValue())){
      Instruction *Ipred = cast<Instruction>(cast<ReturnInst>(I)->getReturnValue());
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


/**
 * Assings positional arguments to LiveIn and LiveOut variables and store those in 
 * metadata
 *
 * @param *BB Basic Block to analyze and assign positional data
 */
void linkPositionalLiveInOut(BasicBlock *BB){
	SetVector<Value*> LiveIn, LiveOut;
	SmallVector<Metadata*,32> Ops;

	LLVMContext & Context = BB->getContext();
	liveInOut(*BB, &LiveIn, &LiveOut);

	int pos = 0;
	MDNode *N;
	for(auto Vin: LiveIn){
		if (isa<Instruction>(Vin)){
			if (N = ((Instruction*)Vin)->getMetadata("fuse.livein.pos")) {
			}
			else{
				MDNode* temp = MDNode::get(Context,ConstantAsMetadata::get(
						ConstantInt::get(Context,llvm::APInt(64,pos,false))));
				((Instruction*)Vin)->setMetadata("fuse.livein.pos",temp);
				//If the Value has Metadata, this is a Merged operand and
				// we need to document position across all BBs
				if (N = ((Instruction*)Vin)->getMetadata("fuse.livein")) {
					for(int i=0;i<N->getNumOperands();++i){
						Value *Vtemp =
							cast<ValueAsMetadata>(N->getOperand(i))->getValue();
						MDNode* temp =
							MDNode::get(Context,ConstantAsMetadata::get(
							ConstantInt::get(Context,
								llvm::APInt(64,pos,false))));
						((Instruction*)Vtemp)
							->setMetadata("fuse.livein.pos",temp);
					}
				}
			}
		}
    // If LiveIn variable does not come from an instruction, it must be a
    // selection function argument, add positional information to the select
    // instruction
    else if (isa<Argument>(Vin)){
      User *uTmp = Vin->use_begin()->getUser();
				MDNode* temp = MDNode::get(Context,ConstantAsMetadata::get(
						ConstantInt::get(Context,llvm::APInt(64,pos,false))));
				((Instruction*)uTmp)->setMetadata("fuse.livein.pos",temp);
      
    }
		pos++;
	}

  // Now we do the same but for liveOut values
  pos = 0;
  for(auto Vout: LiveOut){
    if(N = ((Instruction*)Vout)->getMetadata("fuse.liveout.pos")){
    }
    else{
  		MDNode* temp = MDNode::get(Context,ConstantAsMetadata::get(
						ConstantInt::get(Context,llvm::APInt(64,pos,false))));
				((Instruction*)Vout)->setMetadata("fuse.liveout.pos",temp);
    }
    pos++;
  }
}


/**
 * Uses metadata to tell where the merged operand is coming from
 * This is useful when LiveIn values are merged, they might come from 
 * different locations, hence when calling the merged function, the input
 * struct must be loaded with the appropiate value. This is also done in 
 * reverse for liveOut operands.s
 *
 * @param opA Operand from Block A
 * @param opB Operand from Block B
 * @return void
 */
void linkOps(Value *opA, Value *opB){
	LLVMContext &Context = ((Instruction*)opA)->getContext();
	SmallVector<Metadata*,32> Ops;
	MDNode *N;
	// TODO: Check if there is a more efficient way rather than copying all
	// operands again
	if (N = ((Instruction*)opB)->getMetadata("fuse.livein")) {
		for(int i=0;i<N->getNumOperands();++i)
			Ops.push_back(N->getOperand(i));
	}
	if (N = ((Instruction*)opA)->getMetadata("fuse.livein")) {
		for(int i=0;i<N->getNumOperands();++i)
			Ops.push_back(N->getOperand(i));
	}
	Ops.push_back(ValueAsMetadata::get(opB));
	N = MDTuple::get(Context, Ops);
	((Instruction*)opA)->setMetadata("fuse.livein", N);
  return;
}

/*
 * This functions goes through the linkedLiveOut values in other BBs
 * and updates them to the new Instruction they should be pointing now
 * This function assumes that data has already been copied
 *
 * @param *Inew Instruction that will update the linked listsd
 */
void updateLiveOut(Instruction *Inew){
  MDNode *N, *P;
  if ( N  = (Inew->getMetadata("fuse.liveout"))){
      LLVMContext &Context = Inew->getContext();
  	  SmallVector<Metadata*,32> Ops;
      Ops.push_back(ValueAsMetadata::get(cast<Value>(Inew)));
      P = MDTuple::get(Context, Ops);
  		for(int i=0;i<N->getNumOperands();++i){
        Value *vBB = cast<ValueAsMetadata>(N->getOperand(i))->getValue();
        cast<Instruction>(vBB)->setMetadata("fuse.liveout",P);
      }
  }
  return;
}

/*
 * This function links LiveOut values, so later when recovering the values from
 * the offload function, we are able to known what is their position in
 * the liveout struct and what value are we substituting for
 *
 * @param *opA Value from the BB we want to merge (unmodified)
 * @param *opB Value from the new merged BB we want to bookeep (new BB)
 */
void linkLiveOut(Value *opA, Value *opB, SetVector<Value*> *LiveOut){
  if(LiveOut->count(opA)){
	  LLVMContext &Context = ((Instruction*)opA)->getContext();
  	SmallVector<Metadata*,32> OpsA,OpsB;
  	MDNode *N, *O, *P, *Q;
    OpsA.push_back(ValueAsMetadata::get(opA));
    OpsB.push_back(ValueAsMetadata::get(opB));
    P = MDTuple::get(Context, OpsA);
    Q = MDTuple::get(Context, OpsB);
    // If the merged value has metadata pointing to him we must update that
    // so it now points to the new value
    if ( N  = ((Instruction*)opB)->getMetadata("fuse.liveout")) {
		  for(int i=0;i<N->getNumOperands();++i)
			  OpsA.push_back(N->getOperand(i));
      P = MDTuple::get(Context, OpsA);
    }
    cast<Instruction>(opA)->setMetadata("fuse.liveout", Q);
    cast<Instruction>(opB)->setMetadata("fuse.liveout", P);
  }
	return;
}

/**
 * Uses metadata to tell to which BB this selection arguments belongs to.
 * This enables the insertCall function to set the selection bits to the
 * appropiate values depending on the functinality to execute
 *
 * @param vA Argument of selection
 * @param BB BasicBlock to which it belongs 
 * @return void
 */
void linkArgs(Value *selI, BasicBlock *BB){
	LLVMContext &Context = BB->getContext();
	SmallVector<Metadata*,32> Ops;
	MDNode *N;
  // Aparently we cannot transform BasicBlocks into Metadata through this 
  // function at, hence we will embed the first instruction and extract the
  // BB it belongs to.
  // original code: Ops.push_back(ValueAsMetadata::get(cast<Value>(BB)));
    Instruction *Itemp = &(*BB->begin());
    Ops.push_back(ValueAsMetadata::get(cast<Value>(Itemp)));
    N = MDTuple::get(Context, Ops);
    cast<Instruction>(selI)->setMetadata("fuse.sel.pos", N);
    return;
  }


  BasicBlock* splitBBs(BasicBlock *BB){

    return BB;
  }
  /**
   * Merges two basic blocks
   *
   * @param *A BasicBlock to merge unmodifiedd
   * @param *B BasicBlock to merge accumulated modifications or empty
   
   * @return Returns a newly created BasicBlock that contains a fused list
   * of instructions from A and B. The block does not have a terminator.
   */
  FusedBB* mergeBBs(BasicBlock &A, FusedBB *FB){
    LLVMContext & Context = A.getContext();
    BasicBlock &B = *FB->getBB();
    FusedBB *FC = new FusedBB(&Context,(A.getName()+B.getName()).str());
    BasicBlock *C = FC->getBB();
    IRBuilder<> builder(Context);
    builder.SetInsertPoint(C);
    vector<bool> pendingB(B.size(), true);
    Instruction *mergedOpb;
    map<Value*,Value*> ARAWdeps, BRAWdeps;
    // TODO: Check if instead of creatingthis value in the middle of the fusion
    // we can just instantiate it here (new Arg) and not add it in the end
    //Value *SelVal = NULL; // In case we need selection, a new selval is created
    Value *SelVal = new Argument(builder.getInt1Ty(),"fuse.sel.arg");
    SetVector<Value*> LiveIn, LiveOut;
    liveInOut(A, &LiveIn, &LiveOut);
    memRAWDepAnalysis(&A,&ARAWdeps,Context);
    //memRAWDepAnalysis(&B,&BRAWdeps,Context);

    // 


    // This map keeps track of the relation between old operand - new operand
    map<Value*,Value*> SubOp;
    // for each A instruction we iterate B until we find a mergeable instruction
    // if we find such instruction, we add to C and mark the instruction as 
    // merged in B. Otherwise we just add the new instruction from A.
    for(Instruction &Ia : A){
      int i = 0;
      vector<int> OpC;
      vector<Value*> SelI;
      mergedOpb = NULL;
      if (!Ia.isTerminator() and !isa<IntrinsicInst>(Ia)){
        for(Instruction &Ib : B){
          // Check if instructions can be merged
          if(areInstMergeable(Ia,Ib) and pendingB[i] and checkNoLoop(Ia,Ib,&A)
              and getStoreDomain(&Ia,&A) == getStoreDomain(&Ib,&B)){
            pendingB[i] = false;
            // If instruction is merged, we must point uses of Ib to the new
            // Ia instruction added in C
            mergedOpb = &Ib;
            for(int j = 0; j < Ia.getNumOperands(); ++j){
              Value *opA = Ia.getOperand(j);
              Value *opB = Ib.getOperand(j);
              // Check if operands are mergeable
              //if(areOpsMergeable(opA,opB,&A,&B,&SubOp)){
                //TODO:If mergeable and LiveIn, we must preserve opB
                // origin
                if(opA!=opB)
                  linkOps(opA,opB);

              //}
              //  Operands cannot be merged, add select inst in case
              //  they are the same type
              else if( opA->getType() == opB->getType()){
                SelI.push_back(builder.CreateSelect(SelVal,opA,opB,"fuse.sel"));
                linkArgs(SelI[SelI.size()-1],&A);
                OpC.push_back(j);
              }
              else {
              }
            }
            break;
          }
          ++i;
        }
        // TODO: This whole following section has to be reordered so it is more 
        // comprehensive.
        // Before Adding the instruction we must check if it accesses memory
        // if it does, and it has not been merged, we must protect that
        // We only do this, if we did not merge the load, otherwise it is 
        // already a safe load
        // TODO: Check if there is a more sensible way to do this 
        // like a generit getPointerOperand for instructions
        // Right now we need to cast them to their specific mem instructions
        // TODO: Put this thing in a function please :)e
        // TODO: Make this reasonable pls
        Instruction *newInstA = Ia.clone();
        // TODO: In case B is empty we don't need to add safe operands since t is not 
        // a real BB, test whether this can be optimized
        // 
        /*
        if(!Ia.getMetadata("fuse.issafe") and !mergedOpb){
          Value *oPtr = NULL;
          LoadInst *Li = NULL;
          StoreInst *Si = NULL;
          int index = -1;
          if(Li = (dyn_cast<LoadInst>(&Ia))){
            oPtr = Li->getPointerOperand();
            index = LoadInst::getPointerOperandIndex();
          }
          else if(Si = (dyn_cast<StoreInst>(&Ia))){
            oPtr = Si->getPointerOperand();
            index = StoreInst::getPointerOperandIndex();
          }
          if(Li or Si){
            if(!SelVal) // Only create new argument if there wasnt one b4
              SelVal = new Argument(builder.getInt1Ty(),"fuse.sel.safe.arg");
            Value *G = getSafePointer(cast<PointerType>(oPtr->getType()),Ia.getModule());
            Value *SelSafe = builder.CreateSelect(SelVal,oPtr,G,"fuse.sel.safe");
            linkArgs(SelSafe,&A);
            newInstA->setMetadata("fuse.issafe",emptyMD);
            newInstA->setOperand(index,SelSafe);
          } 
        }*/

        // Adding instruction from A. If merged, check if select was created
        // and set not merged operand as a result of the select inst.
        builder.Insert(newInstA);
        // We link the instructions to the original BB instruction
        // TODO: Analyze if this can be merged with linkLiveOut, since it kind of has the same
        // job and we are doing it anyways for LiveOut values
        // We link liveOuts from A to C, so we can get positional arguments
        for(int j = 0; j < OpC.size() ; ++j)  
          newInstA->setOperand(OpC[j],SelI[j]);

        SubOp[cast<Value>(&Ia)] = cast<Value>(newInstA);
        if(mergedOpb){
          SubOp[cast<Value>(mergedOpb)] = newInstA;
          newInstA->copyMetadata(*mergedOpb);
          updateLiveOut(newInstA); // Migrate data from B if merged
          linkInsOld(mergedOpb,&Ia);
          FC->linkLiveOut(mergedOpb,newInstA,&LiveOut);
          //linkLiveOut((Value*)mergedOpb,(Value*)newInstA,&LiveOut); 
          annotateMerge(newInstA);
          if(isa<LoadInst>(newInstA) or isa<StoreInst>(newInstA)){
            MDNode* safe1MD = MDNode::get(builder.getContext(),ConstantAsMetadata::get(
              ConstantInt::get(builder.getContext(),APInt(64,1,false))));
            newInstA->setMetadata("fuse.issafe",safe1MD);
          }
        }
        FC->linkLiveOut(&Ia,newInstA,&LiveOut);
        //linkLiveOut((Value*)&Ia,(Value*)newInstA, &LiveOut);
        linkIns(newInstA,&Ia);  
      }
    }

    // Left-over instructions from B are added now
    // Traverse  in reverse, so dependent instructions are added last
    unsigned i = 0;
    for(auto itB = B.begin(); itB != B.end(); ++itB){
      Instruction &Ib = *itB;
      if (pendingB[i] and !Ib.isTerminator() and !isa<IntrinsicInst>(Ib)){
        Instruction *newInstB = Ib.clone();
        linkIns(newInstB,&Ib);  
        // Drop the safety tag if we didn't merge the memory instruction
        // Unless this is an already safe memory instruction
        MDNode *N;
        if( N = (Ib.getMetadata("fuse.issafe")) ){
          if(cast<ConstantInt>(cast<ConstantAsMetadata>(N->getOperand(0))->getValue())
              ->getSExtValue() == 1){
            Ib.setMetadata("fuse.issafe",NULL);
          }
        }

        builder.Insert(newInstB);
        updateLiveOut(newInstB); // Migrate data from B if merged
        FC->linkLiveOut(&Ib,newInstB,&LiveOut);
        //linkLiveOut((Value*)&Ib,(Value*)newInstB,&LiveOut); 
        SubOp[cast<Value>(&Ib)] = cast<Value>(newInstB);
        pendingB[i] = false;

      }
      ++i;
    }

    // Target operands change name when clonned
    // Here we remap them to their new names to preserve producer-consumer 
    for(auto &Ic: *C){
      MDNode *N;
      if ((N = Ic.getMetadata("fuse.rawdep"))){
        Value *V = cast<ValueAsMetadata>(N->getOperand(0))->getValue();
        if(SubOp.count(V)){
	        SmallVector<Metadata*,32> Ops;
          Ops.push_back(ValueAsMetadata::get(SubOp[V]));
          MDNode *N = MDTuple::get(Context,Ops);
          Ic.setMetadata("fuse.rawdep",N);
        }
      }
      if ((N = Ic.getMetadata("fuse.wardep"))){
        Value *V = cast<ValueAsMetadata>(N->getOperand(0))->getValue();
        if(SubOp.count(V)){
	        SmallVector<Metadata*,32> Ops;
          Ops.push_back(ValueAsMetadata::get(SubOp[V]));
          MDNode *N = MDTuple::get(Context,Ops);
          Ic.setMetadata("fuse.wardep",N);
        }
      }
      for(int j = 0; j < Ic.getNumOperands(); ++j){
        Value *Vc = Ic.getOperand(j);
        if(SubOp.count(Vc))
          Ic.setOperand(j,SubOp[Vc]);
      }
    }
    if(!B.empty()){ // Only if we merge with something
      KahnSort(C); // Sort instructions by producer-consumer relationship
      secureMem(C,SelVal,builder,&A, A.getModule()); // If loads are not merged, make sure they are safe to execute
    }
    for(auto &Ic: *C)
      Ic.setMetadata("fuse.origold",NULL);

    return FC;
  }

  /**
   * List instructions in BB
   *
   * @param BB Basic Block Reference
   * @return N/A
   */
  void listBBInst(BasicBlock &BB){

    for(Instruction &I : BB){
      errs() << I << '\n';
    }

    return;
  }

  /**
   * Creates an offload function from a given BB
   *
   * @param BB Basic Block reference
   * @return Pointer to a function that executes BB
   */
  //TODO: Optimize load/stores in case no LiveIn/LiveOut
  Function* createOffload(BasicBlock &BB, Module *Mod){
    LLVMContext &Context = BB.getContext();
    static IRBuilder<> Builder(Context);
    vector<Type *> inputTypes, outputTypes;
    SetVector<Value*> LiveIn, LiveOut;
    liveInOut(BB, &LiveIn, &LiveOut);
    
    // Once we merged the BBs, we must annotate liveIn values from different
    // locations with their respective locations in the input Struct
    // so that once we call the offload function, we know what fields must 
    // be filled and which ones should be filled with padding and safe inputs
    // This function adds metadata to the LiveIn values from different BBs
    // Same for Live Outs
    linkPositionalLiveInOut(&BB);


    // Instantate all LiveIn values as Inputs types
    for(Value *V: LiveIn){
      inputTypes.push_back(V->getType());
    }
    // Instantate all LiveOut values as Output types
    for(Value *V: LiveOut){
      outputTypes.push_back(V->getType());
    }

    // Input/Output structs
    StructType *inStruct = StructType::get(Context,inputTypes);
    StructType *outStruct = StructType::get(Context,outputTypes);


    // Instantiate function
    FunctionType *funcType = FunctionType::get(outStruct->getPointerTo(),
        ArrayRef<Type*>(inStruct->getPointerTo()),false);
    Function *f = Function::Create(funcType,
        Function::ExternalLinkage,"offload_func",Mod);

    // Insert Fused Basic Block
    BB.insertInto(f);

    // Load Data (Creating New BB for readibility)
    BasicBlock* loadBB = BasicBlock::Create(Context,"loadBB",f,&BB);
    Builder.SetInsertPoint(loadBB);
    // DEBUG PURPOSES
    //Function *dbgt = Intrinsic::getDeclaration(Mod,Intrinsic::debugtrap);
    vector<Value*> inData;
    for(int i=0;i<LiveIn.size();++i){
      Value *inGEPData = Builder.CreateStructGEP(f->getArg(0),i);
      inData.push_back(Builder.CreateLoad(inGEPData));
    }
    // DEBUG PURPOSES
    //Builder.CreateCall(dbgt->getFunctionType(),cast<Value>(dbgt));  
    Builder.CreateBr(&BB);

    // TODO: Make this more efficient
    for(int i=0; i<LiveIn.size();++i){
      for(auto &I: BB){
        for(int j=0;j<I.getNumOperands();++j){
          if(I.getOperand(j) == LiveIn[i]){
            I.setOperand(j,inData[i]);
          }
        }
      }
      // Preserve names for arguments 
      inData[i]->setName(LiveIn[i]->getName());
    }

    /*
    for(auto &I: BB){
      Builder.SetInsertPoint(&I);
      if(isa<StoreInst>(I))
        Builder.CreateCall(dbgt->getFunctionType(),cast<Value>(dbgt));  
    }*/

    // Store Data (Creating New BB for readibility)
      BasicBlock* storeBB = BasicBlock::Create(Context,"storeBB",f);
      Builder.SetInsertPoint(storeBB);
      //Builder.CreateCall(dbgt->getFunctionType(),cast<Value>(dbgt));  
      Value *outData = Builder.CreateAlloca(outStruct);
      for(int i=0;i<LiveOut.size();++i){
        Value *outGEPData = Builder.CreateStructGEP(outData,i);
        Builder.CreateStore(LiveOut[i],outGEPData);
      }
      Builder.CreateRet(outData);
  
      // Merged block jumps to store output data
      Builder.SetInsertPoint(&BB);
      Builder.CreateBr(storeBB);

    // Check Consistency
    verifyFunction(*f,&errs());
    verifyModule(*Mod,&errs());

    //f->dump();
    //Mod->dump();

    // Output Result
    /* Disable this for now as it only generates problems and this can be
     * safely done with llvm-extract
    Module *NewMod = new Module("offload_mod",Context);
    verifyModule(*NewMod,&errs());
    ValueToValueMapTy Vmap;
    Function *Newf = CloneFunction(f,Vmap);
    for (inst_iterator I = inst_begin(Newf), E = inst_end(Newf); I != E; ++I){
        if(isa<CallInst>(*I)){
          ValueToValueMapTy Vmap;
          Function *tmp = cast<CallInst>(*I).getCalledFunction();
          Function *fc = Function::Create(tmp->getFunctionType(),
              Function::ExternalLinkage,tmp->getName(),NewMod);
          cast<CallInst>(*I).setCalledFunction(fc);
        }
    }
    Newf->removeFromParent();
    NewMod->getFunctionList().push_back(Newf);
    // Removing all unknown metadata as it cannot be written in the bitcode
    // TODO: Check if MDNode can actually be printed as bitcode
    // Right now maybe we are storing Metadata in a wrong way hence llvm 
    // complains about not being able to print the bitcode
    for (inst_iterator I = inst_begin(Newf), E = inst_end(Newf); I != E; ++I)
      if(I->hasMetadataOtherThanDebugLoc())
         I->dropUnknownNonDebugMetadata();
    verifyModule(*NewMod,&errs());
    error_code EC;
    raw_fd_ostream of("offload.bc", EC,sys::fs::F_None);
    verifyModule(*NewMod,&errs());
    WriteBitcodeToFile(*NewMod,of);
    of.flush();
    of.close();
    delete NewMod;
    */

    return f;
  }

  /**
   * Substitute all given BBs for function calls and create wrappers to 
   * gather/scatter the data
   *
   * @param *F pointer to the function to be called
   * @param *bbList list of BBs to be substituted
   * @return true if success
   */
  bool insertCall(Function *F, vector<BasicBlock*> *bbList){
    LLVMContext &Context = F->getContext();
    static IRBuilder<> Builder(Context);
    vector<Type*> inputTypes;
    //vector<GlobalVariable*> SafeInputs;
    vector<Instruction*> removeInst;

    // offloaded functions receive as input a pointer to input struct and
    // return a pointer to output struct
    Type *inputType = ((PointerType*)F->getArg(0)->getType())->getElementType();
    Type *outputType = ((PointerType*)F->getReturnType())->getElementType();

    // Restored Values must replace LiveIn values
    map<BasicBlock*,pair<Value*,Value*> > restoreMap;

    // Create Safe Globals for input vectors from calls that are missing
    // some of the LiveIn variables
    // TODO: Do not copy select variables
    // TODO: Create safe globals for pointers
    // TODO: This can now probably be removed, since we make safe potiner
    // access in the offloaded funciton
    // TODO: Remove, no longer needed, we protect loads in offload funciton
    //
    //for(auto &O : ((StructType*)inputType)->elements())
    //  SafeInputs.push_back(getSafePointer(O,F->getParent()));

    int i = 0; // Pointer to BB in bbList being processed
    // Record all instruction with metadata, so we can remove them later
    set<Instruction*> iMetadata;
    for(auto BB: *bbList){
      SetVector<Value*> LiveIn, LiveOut;
      liveInOut(*BB, &LiveIn, &LiveOut);

      // TODO: Optimize if empty input/output
      Instruction *I = BB->getFirstNonPHIOrDbg();
      Builder.SetInsertPoint(I);
      Value *inStruct = Builder.CreateAlloca(inputType);

      // Send Input information
      //set<int> visited; // Mark those input positions that are filled
      vector<AllocaInst*> Allocas;
      vector<pair<Value*, Value*> > RestoreInst;
      for(auto inV : LiveIn){
        MDNode *N;
        int pos = -1;
        // Set input parameters in their position inside the 
        // input struct. Mark the position as filled. Record that there
        // is metadata to be removed
        if (N = ((Instruction*)inV)->getMetadata("fuse.livein.pos")){
          pos = cast<ConstantInt>(cast<ConstantAsMetadata>
               (N->getOperand(0))->getValue())->getSExtValue();
          //visited.insert(pos);
          iMetadata.insert((Instruction*)inV);
        }
        // We should only enter here if parameter is function select
        else{
          // TODO: Merging of alloca instructions might be tricky
         errs() << "pos = "<< pos << "<=-1 && Missing positional information\n This might be an error or might be that the LiveIn variable got merged in\n"; 
        }

        if(pos >= 0){
          Value *inGEPData = Builder.CreateStructGEP(inStruct,pos);
          Builder.CreateStore(inV,inGEPData);
        }
        // Keep safe restore values
        // We dropped speculation,. hence nothing to restore in memory
        // TODO: Add specualtion and restora values if wrong path is taken
        /*
        if(inV->getType()->isPointerTy()){
          Allocas.push_back(Builder.CreateAlloca(((PointerType*)
            inV->getType())->getElementType()));
          Builder.CreateStore(Builder.CreateLoad(inV),
            *Allocas.rbegin());
          RestoreInst.push_back(pair<Value*,Value*>(inV,
            *Allocas.rbegin()));
          }*/
        }
        
        // Send function selection
        // TODO: This has to be optimized. Now we search in the function for
        // the select instructions to find it's position and to which BB they
        // belong to
        for(auto &BBtemp: *F)
          for(auto &Itemp: BBtemp){
            MDNode *N;
            BasicBlock *selBB = NULL;
            int pos = -1;
            if (N = (Itemp.getMetadata("fuse.sel.pos"))){
              selBB = cast<Instruction>(cast<ValueAsMetadata>
                (N->getOperand(0).get())->getValue())->getParent();
            }
				    if (N = (Itemp.getMetadata("fuse.livein.pos"))){
					   pos = cast<ConstantInt>(cast<ConstantAsMetadata>
					  	  	(N->getOperand(0))->getValue())->getSExtValue();
            }
            if(pos>=0 and selBB){
  				    Value *inGEPData = Builder.CreateStructGEP(inStruct,pos);
  				    Builder.CreateStore(Builder.getInt1(selBB==BB),inGEPData);
           	  //visited.insert(pos);
            }
            if(Itemp.hasMetadata())
					    iMetadata.insert(&Itemp);
          }
/*
			int iOffset = ((StructType*)inputType)->getNumElements()
				-(bbList->size()-1);
			for(int j=1;j<bbList->size();++j){
				Value *inGEPData = Builder.CreateStructGEP(inStruct,
						iOffset+(j-1));
				Builder.CreateStore(Builder.getInt1(j==i),inGEPData);
				visited.insert(iOffset+(j-1));
			}
*/
			// If not visited we must store safe parameters in the sturct
			//for(int arg = 0; arg < ((StructType*)inputType)->getNumElements();++arg){
			//	if(!visited.count(arg)){
			//		Value *inGEPData = Builder.CreateStructGEP(inStruct,arg);
			//		Value *GlobDat = Builder.CreateLoad(SafeInputs[arg]);
			//		Builder.CreateStore(GlobDat,inGEPData);
			//	}
			//}

			// Insert Call to offload function
			Value *outStruct = Builder.CreateCall(F,ArrayRef<Value*>(inStruct));
      
      // Removing the offloaded instructions
      // Avoid removing the terminator
      auto rI = BB->rbegin();
      rI++;
      while(&(*rI) != (Instruction*)outStruct){ 
        removeInst.push_back(&(*rI));
        rI++;
      }
      // TODO: find a more efficient way of 
      // Removing metadata attached to debug instructions
      while(rI != BB->rend()){
        if(rI->getMetadata("fuse.liveout"))
          iMetadata.insert(&(*rI));
          rI++;
      }

      // If we have liveOut values we must now fill them with the function
      // computed ones and discard the instructions from this offloaded BB
      vector<AllocaInst*> oAllocas;
      Value *lastLoad;
      for(int arg = 0; arg < LiveOut.size(); ++arg){
        Value *Vout = LiveOut[arg];
        Instruction *mergeIout;
        MDNode *N;
        int pos;
	      if (N = ((Instruction*)Vout)->getMetadata("fuse.liveout")) {
          mergeIout = cast<Instruction>(cast<ValueAsMetadata> // If this segfaults, likely we deleted the BB we were pointing at
                (N->getOperand(0).get())->getValue());
          MDNode *T = mergeIout->getMetadata("fuse.liveout.pos");
					pos = cast<ConstantInt>(cast<ConstantAsMetadata>
					  	  	(T->getOperand(0))->getValue())->getSExtValue();
          // TODO: Check if needed since we destroy those instructions once
          // we insert the function
          iMetadata.insert((Instruction*)Vout);
        }
        Value *outGEPData = Builder.CreateStructGEP(outStruct,pos);
        Value *out = Builder.CreateLoad(LiveOut[arg]->getType(),outGEPData);
        LiveOut[arg]->replaceAllUsesWith(out);
        lastLoad = out;
      }

      BB->setName(BB->getName());
      // We are dropping for now the speculation capability. It makes things 
      // simpler.
      // TODO: Execute speculatvely, and restore the values that changed in
      // memory
		++i;
	}

  for(auto I: removeInst)
    I->eraseFromParent();
	// TODO: Should we clean metadata like this, ValueAsMetadata cannot be saved as bitcode?
	for(auto I: iMetadata){
    I->setMetadata("fuse.liveout",NULL);
    I->setMetadata("fuse.liveout.pos",NULL);
    I->setMetadata("fuse.livein",NULL);
    I->setMetadata("fuse.livein.pos",NULL);
    I->setMetadata("fuse.sel.pos", NULL);
    I->setMetadata("fuse.orig", NULL);
    I->setMetadata("fuse.rawdep",NULL);
    I->setMetadata("fuse.wardep",NULL);
    //I->setMetadata("fuse.issafe",NULL);
		//I->setMetadata("fuse.livein.pos",NULL);
		//I->dropUnknownNonDebugMetadata();
  }
	// Remap restored Values in each BB
	for(auto Elem : restoreMap){
		Elem.second.first->replaceUsesWithIf(Elem.second.second,
				[Elem](Use &U){return
				((Instruction*)U.getUser())->getParent() == Elem.first;});
	}
	return true;
}
