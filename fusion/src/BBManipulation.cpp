#include "BBManipulation.hpp"


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
 * Creates safe variables for inputs not used in the offloaded function by 
 * basic blocks
 *
 * @param *O Type of the input to create safe variable
 * @param *M Module
 */
GlobalVariable *getSafePointer(Type *O, Module *M){
  // TODO: Check if there is a more efficient way to do this
  GlobalVariable *G;
  if(O->isPointerTy()){
    GlobalVariable *ret = getSafePointer(cast<PointerType>(O)->getPointerElementType(),M);
    G = new GlobalVariable(*M,O,false, GlobalValue::PrivateLinkage,ret);
  }
  else{
    G = new GlobalVariable(*M,O,false,GlobalValue::PrivateLinkage,
        Constant::getNullValue(O));
  }
  return G;
}

//TODO: Check if we have to create a (new) GV or we can just get one with 
//the same value troguht some getter funciont on the module or somethign
Value *getSafePointer1(PointerType *O, Module *M){
  Value *V =  new GlobalVariable(*M,O->getPointerElementType(),false,GlobalValue::ExternalLinkage,Constant::getNullValue(O->getPointerElementType()));
  return V;
}

/**
 * Transforms conditional branches to unconditional. This is done so operands
 * of conditional branches are considered LiveOut Values.
 * To do so we create an intermediate BB that will contain the original
 * conditional branch.
 *
 * @param *BB Basic Block to tranform branch
 */
void separateBr(BasicBlock *BB){
  Instruction *Br = BB->getTerminator();
  if(cast<BranchInst>(Br)->isConditional()){
    Instruction *Ipred = cast<Instruction>(Br->getOperand(0));
    MDNode* temp = MDNode::get(BB->getContext(),ArrayRef<Metadata*>());
    Ipred->setMetadata("is.liveout",temp);
    IRBuilder<> builder(BB->getContext());
    BasicBlock *newBB = BasicBlock::Create(BB->getContext(),"sep"+BB->getName(),
      BB->getParent());
    Br->moveBefore(*newBB,newBB->begin());
    builder.SetInsertPoint(BB);
    builder.CreateBr(newBB);

  }
}


/**
 * Assings positional arguments to LiveIn and LiveOut variables and stores those in 
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
    // If LiveIn variable does not come frum an instruction, it must be a
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
 * Checks if two instructions are mergeable, returns true if they are
 *
 * @param *Ia Instruction to check
 * @param *Ib Instruction to check
 * @return true if mergeable, false otherwise
 */
bool areInstMergeable(Instruction &Ia, Instruction &Ib){
	// Same opcode
	bool opcode = Ia.getOpcode() == Ib.getOpcode();

  // If operands are loads, they must be loading the same type of data
  // TODO: Check if this can be generalized to ints vs floats
  bool loadty = opcode;
  if (loadty and isa<LoadInst>(Ia))
    loadty = cast<LoadInst>(Ia).getPointerOperandType() ==
             cast<LoadInst>(Ib).getPointerOperandType();
  // Same for stores
  bool storety = opcode;
  if (storety and isa<StoreInst>(Ia))
    storety = cast<StoreInst>(Ia).getPointerOperandType() ==
              cast<StoreInst>(Ib).getPointerOperandType();
  // Some getelemptr have different amount of operands
  // TODO: Check if those can be merged, for now we are leaving them asside
  bool numops = Ia.getNumOperands() == Ib.getNumOperands();
  // Check operand compatibility
  bool samety = numops;
  if(samety)
    for(int i=0;i<Ia.getNumOperands();++i){
      samety &= Ia.getOperand(i)->getType() == Ib.getOperand(i)->getType();
    }

  // Do not merge select instructions, we will deal with this in later 
  // optimization steps
  bool noselect = !isa<SelectInst>(Ia);
	return opcode and loadty and storety and numops and noselect and samety;
}

/**
 * Checks if operands can be merged, returns true if they are
 *
 * @param &Ia instruction to check
 * @param &Ib instruction to check
 * @param &IRBuilder for inserting select instructions
 * @return operands merge
 */
bool areOpsMergeable(Value *opA, Value *opB, BasicBlock *A, BasicBlock *B){
	bool sameValue = false;
	bool isLiveIn = false;
	bool isInstMerg = false;
  bool isConstant = false;
  bool isSameType = false;

	sameValue = opA == opB;
  // Inst are same Value Constants
  bool isSame = sameValue;
  isConstant = isa<Constant>(opA) and isa<Constant>(opB);
  isSameType = opA->getType() == opB->getType();
	// If operands come from oustide the BB, they are fusable LiveIns
	isLiveIn = ((Instruction*)opA)->getParent() != A &&
			((Instruction*)opB)->getParent() != B;
		// If operands come from mergeable Instructions
	isInstMerg = areInstMergeable(*(Instruction*)opA,*(Instruction*)opB);
	return (sameValue and isConstant) or (isSameType and isLiveIn and !isConstant) or (isSameType and isInstMerg and !isConstant);
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
	if (N = ((Instruction*)opA)->getMetadata("fuse.livein")) {
		MDNode *T =(MDNode*)(N->getOperand(0).get());
		// TODO: Check if there is a more efficient way rather than copying all
		// operands again
		for(int i=0;i<N->getNumOperands();++i)
			Ops.push_back(N->getOperand(i));
    // TODO: This should be unified with the else case (same for liveout)
		Ops.push_back(ValueAsMetadata::get(opB));
	}
	else{
		Ops.push_back(ValueAsMetadata::get(opB));
	}
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

  /**
   * Merges two basic blocks
   *
   * @param *A BasicBlock to merge unmodifiedd
   * @param *B BasicBlock to merge accumulated modifications or empty
   
   * @return Returns a newly created BasicBlock that contains a fused list
   * of instructions from A and B. The block does not have a terminator.
   */
  BasicBlock* mergeBBs(BasicBlock &A, BasicBlock &B){
    LLVMContext & Context = A.getContext();
    BasicBlock *C = BasicBlock::Create(Context,A.getName()+B.getName());
    IRBuilder<> builder(Context);
    builder.SetInsertPoint(C);
    vector<bool> pendingB(B.size(), true);
    Instruction *mergedOpb;
    // TODO: Check if instead of creatingthis value in the middle of the fusion
    // we can just instantiate it here (new Arg) and not add it in the end
    Value *SelVal = NULL; // In case we need selection, a new selval is created
    SetVector<Value*> LiveIn, LiveOut;
    liveInOut(A, &LiveIn, &LiveOut);

    // 
    MDNode* emptyMD = MDNode::get(Context,ArrayRef<Metadata*>());


    // This map keeps track of the relation between old operand - new operand
    map<Value*,Value*> SubOp;
    // for each A instruction we iterate B until we find a mergeable instruction
    // if we find such instruction, we add to C and mark the instruction as 
    // merged in B. Otherwise we just add the new instruction from A.
    for(Instruction &Ia : A){
      int i = 0;
      int OpC = -1;
      Value *SelI = NULL;
      mergedOpb = NULL;
      if (!Ia.isTerminator() and !isa<IntrinsicInst>(Ia)){
        for(Instruction &Ib : B){
          // Check if instructions can be merged
          if(areInstMergeable(Ia,Ib) and pendingB[i]){
            pendingB[i] = false;
            // If instruction is merged, we must point uses of Ib to the new
            // Ia instruction added in C
            mergedOpb = &Ib;
            for(int j = 0; j < Ia.getNumOperands(); ++j){
              Value *opA = Ia.getOperand(j);
              Value *opB = Ib.getOperand(j);
              // Check if operands are mergeable
              if(areOpsMergeable(opA,opB,&A,&B)){
                //TODO:If mergeable and LiveIn, we must preserve opB
                // origin
                if(opA!=opB)
                  linkOps(opA,opB);

              }
              //  Operands cannot be merged, add select inst in case
              //  they are the same type
              else if( opA->getType() == opB->getType()){
                if(!SelVal) // Only create new argument if there wasnt one b4
                  SelVal = new Argument(builder.getInt1Ty(),"fuse.sel.op.arg");
                SelI = builder.CreateSelect(SelVal,opA,opB,"fuse.sel");
                linkArgs(SelI,&A);
                OpC = j;
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
            Value *G = getSafePointer1(cast<PointerType>(oPtr->getType()),Ia.getModule());
            Value *SelSafe = builder.CreateSelect(SelVal,oPtr,G,"fuse.sel.safe");
            linkArgs(SelSafe,&A);
            newInstA->setMetadata("fuse.issafe",emptyMD);
            newInstA->setOperand(index,SelSafe);
          } 
        }

        // Adding instruction from A. If merged, check if select was created
        // and set not merged operand as a result of the select inst.
        updateLiveOut(newInstA); // Migrate data from B if merged
        builder.Insert(newInstA);
        // We link liveOuts from A to C, so we can get positional arguments
        linkLiveOut((Value*)&Ia,(Value*)newInstA, &LiveOut);
        if(SelI)
          newInstA->setOperand(OpC,SelI);
        SubOp[cast<Value>(&Ia)] = cast<Value>(newInstA);
        if(mergedOpb){
          SubOp[cast<Value>(mergedOpb)] = newInstA;
          newInstA->copyMetadata(*mergedOpb);
          linkLiveOut((Value*)mergedOpb,(Value*)newInstA,&LiveOut); 
          annotateMerge(newInstA);
        }
      }
    }

    // Left-over instructions from B are added now
    // Traverse  in reverse, so dependent instructions are added last
    unsigned i = 0;
    for(auto itB = B.begin(); itB != B.end(); ++itB){
      Instruction &Ib = *itB;
      if (pendingB[i] and !Ib.isTerminator() and !isa<IntrinsicInst>(Ib)){
        Instruction *newInstB = Ib.clone();
        // TODO: Put this last bit into a function pls :)
        // If Load/Store we need to create safe select
        if(!Ib.getMetadata("fuse.issafe")){
          Value *oPtr = NULL;
          LoadInst *Li = NULL;
          StoreInst *Si = NULL;
          int index = -1;
          if(Li = (dyn_cast<LoadInst>(&Ib))){
            oPtr = Li->getPointerOperand();
            index = LoadInst::getPointerOperandIndex();
          }
          else if(Si = (dyn_cast<StoreInst>(&Ib))){
            oPtr = Si->getPointerOperand();
            index = StoreInst::getPointerOperandIndex();
          }
          if(Li or Si){
            if(!SelVal) // Only create new argument if there wasnt one b4
              SelVal = new Argument(builder.getInt1Ty(),"fuse.sel.safe.arg");
            Value *G = getSafePointer1(cast<PointerType>(oPtr->getType()),A.getModule());
            Value *SelSafe = builder.CreateSelect(SelVal,oPtr,G);
            newInstB->setMetadata("fuse.issafe",emptyMD);
            newInstB->setOperand(index,SelSafe);
          }   
        }

        /*for(auto U = Ib.use_begin(); U != Ib.use_end() & pendingB[i]; ++U){
          // This deals with the insertion point
          // Insert those instructions before their uses
          if(((Instruction*)U->getUser())->getParent() == &B){
            updateLiveOut(newInstB); // Migrate data from B if merged
            newInstB->insertBefore((Instruction*)U->getUser());
            linkLiveOut((Value*)&Ib,(Value*)newInstB,&LiveOut); 
            SubOp[cast<Value>(&Ib)] = cast<Value>(newInstB);
            pendingB[i] = false;
          }
        }*/
        // Is this possible? If no uses inside basic block append at end
        // Is this redundant w.r.t. the previous lines?
          Instruction *iPointer = findOpDef(&Ib,&SubOp,C);
          Instruction *firstUse = findFirstUse(&Ib,&B,C);
          if(iPointer)
            newInstB->insertAfter(iPointer);
          else if(firstUse)
          //if(firstUse)
            newInstB->insertBefore(firstUse);
          else
            builder.Insert(newInstB);
          updateLiveOut(newInstB); // Migrate data from B if merged
          linkLiveOut((Value*)&Ib,(Value*)newInstB,&LiveOut); 
          SubOp[cast<Value>(&Ib)] = cast<Value>(newInstB);
          pendingB[i] = false;

          /*
          for(auto &Ic : *C){
            if( &Ic == newInstB )
              errs() << Ic << " added\n";
            else
              Ic.dump();
          }*/
      }
      ++i;
    }

    // Target operands change name when clonned
    // Here we remap them to their new names to preserve producer-consumer 
    for(auto &Ic: *C){
      for(int j = 0; j < Ic.getNumOperands(); ++j){
        Value *Vc = Ic.getOperand(j);
        if(SubOp.count(Vc))
          Ic.setOperand(j,SubOp[Vc]);
      }
    }


    for(auto &I : *C)
      errs() << I << '\n';
    return C;
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

    f->dump();
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
        int pos;
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
          errs() << "Missing positional information\n";
        }

        Value *inGEPData = Builder.CreateStructGEP(inStruct,pos);
        Builder.CreateStore(inV,inGEPData);
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
			Value *outStruct = Builder.CreateCall((Value*)F,ArrayRef<Value*>(inStruct));
      
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
          mergeIout = cast<Instruction>(cast<ValueAsMetadata>
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
