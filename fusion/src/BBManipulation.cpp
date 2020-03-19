#include "BBManipulation.hpp"


/**
 * Assings positional arguments to LiveIn variables and stores those in 
 * metadata
 *
 * @param *BB Basic Block to analyze and assign positional data
 */
void linkPositionalLiveIn(BasicBlock *BB){
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

	return opcode and loadty and storety;
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
	bool sameConstValue = false;
	bool isLiveIn = false;
	bool isInstMerg = false;
	if(isa<ConstantInt>(opA) && isa<ConstantInt>(opB)){
		sameConstValue = opA == opB;
	}
  else if(isa<ConstantFP>(opA) && isa<ConstantFP>(opB)){
    sameConstValue = opA == opB;
  }
	else{
		// If operands come from oustide the BB, they are fusable LiveIns
		isLiveIn = ((Instruction*)opA)->getParent() != A &&
			((Instruction*)opB)->getParent() != B;
		// If operands come from mergeable Instructions
		isInstMerg = areInstMergeable(*(Instruction*)opA,*(Instruction*)opB);
	}
	return sameConstValue || isLiveIn || isInstMerg;
}

/**
 * Uses metadata to tell where the merged operand is coming from
 * This is useful when LiveIn values are merged, they might come from 
 * different locations, hence when calling the merged function, the input
 * struct must be loaded with the appropiate values
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
		Ops.push_back(ValueAsMetadata::get(opB));

	}
	else{
		Ops.push_back(ValueAsMetadata::get(opB));
	}
	N = MDTuple::get(Context, Ops);
	((Instruction*)opA)->setMetadata("fuse.livein", N);
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
   * @param *A BasicBlock to merge
   * @param *B BasicBlock to merge
   
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
      if (!Ia.isTerminator()){
        for(Instruction &Ib : B){
          // Check if instructions can be merged
          if(areInstMergeable(Ia,Ib)){
            pendingB[i] = false;
            for(int j = 0; j < Ia.getNumOperands(); ++j){
              Value *opA = Ia.getOperand(j);
              Value *opB = Ib.getOperand(j);
              // Check if operands are mergeable
              if(areOpsMergeable(opA,opB,&A,&B)){
                //TODO:If mergeable and LiveIn, we must preserve opB
                // origin
                linkOps(opA,opB);
                // If instruction is merged, we must point uses of Ib to the new
                // Ia instruction added in C
                mergedOpb = &Ib;

              }
              //  Operands cannot be merged, add select inst in case
              //  they are the same type
              else if( opA->getType() == opB->getType()){
                Value *SelVal = new Argument(builder.getInt1Ty());
                SelI = builder.CreateSelect(SelVal,opA,opB);
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
        // Adding instruction from A. If merged, check if select was created
        // and set not merged operand as a result of the select inst.
        Instruction *newInstA = Ia.clone();
        builder.Insert(newInstA);
        if(SelI)
          newInstA->setOperand(OpC,SelI);
        SubOp[cast<Value>(&Ia)] = cast<Value>(newInstA);
        if(mergedOpb)
          SubOp[cast<Value>(mergedOpb)] = newInstA;
      }
    }

    // Left-over instructions from B are added now
    unsigned i = 0;
    for(Instruction &Ib : B){
      if (pendingB[i] and !Ib.isTerminator() ){
        for(auto U = Ib.use_begin(); U != Ib.use_end() & pendingB[i]; ++U){
          // This deals with the insertion point
          // Insert those instructions before their uses
          if(((Value*)U->getUser())->isUsedInBasicBlock(C)){
            Instruction *newInstB = Ib.clone();
            newInstB->insertBefore((Instruction*)U->getUser());
            SubOp[cast<Value>(&Ib)] = cast<Value>(newInstB);
            pendingB[i] = false;
          }
        }
        // Is this possible? If no uses inside basic block append at end
        // Is this redundant w.r.t. the previous lines?
        if(pendingB[i] and !Ib.isTerminator()){
          Instruction *newInstB = Ib.clone();
          builder.Insert(newInstB);
          SubOp[cast<Value>(&Ib)] = cast<Value>(newInstB);
        }
      }
      ++i;
    }

    // Target operands change name when clonned
    // Here we remap them to their new names to preserve producer-consumer 
    for(Instruction &Ic: *C){
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
    linkPositionalLiveIn(&BB);


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
    vector<Value*> inData;
    for(int i=0;i<LiveIn.size();++i){
      Value *inGEPData = Builder.CreateStructGEP(f->getArg(0),i);
      inData.push_back(Builder.CreateLoad(inGEPData));
    }
    Builder.CreateBr(&BB);

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


    // Store Data (Creating New BB for readibility)
    BasicBlock* storeBB = BasicBlock::Create(Context,"storeBB",f);
    Builder.SetInsertPoint(storeBB);
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
    Module *NewMod = new Module("offload_mod",Context);
    verifyModule(*NewMod,&errs());
    /*Function *Newf = Function::Create(funcType,
        Function::ExternalLinkage,"offload_func",NewMod);*/
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
    vector<Value*> Inputs;
    vector<Type*> inputTypes;
    vector<GlobalVariable*> SafeInputs;
    vector<GlobalVariable*> RestoredInputs;

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
    for(auto &O : ((StructType*)inputType)->elements()){
      F->getParent()->getOrInsertGlobal("",O);
      SafeInputs.push_back(new GlobalVariable(*F->getParent(),O,true,
            GlobalValue::PrivateLinkage, Constant::getNullValue(O)));
    }

    int i = 0; // Pointer to BB in bbList being processed
    // Record all instruction with metadata, so we can remove them later
    set<Instruction*> iMetadata;
    for(auto BB: *bbList){
      SetVector<Value*> LiveIn, LiveOut;
      liveInOut(*BB, &LiveIn, &LiveOut);

      // Predecessor blocks change their branches to a function call
      // TODO: Optimize if empty input/output
      BasicBlock *BBpred = BB->getSinglePredecessor();
      if(BBpred){
        Instruction *I = BBpred->getTerminator();
        Builder.SetInsertPoint(I);
        Value *inStruct = Builder.CreateAlloca(inputType);

        // Send Input information
        set<int> visited; // Mark those input positions that are filled
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
            visited.insert(pos);
            iMetadata.insert((Instruction*)inV);
          }
          // We should only enter here if parameter is function select
          else{
            errs() << "Missing positional information\n";
          }

          Value *inGEPData = Builder.CreateStructGEP(inStruct,pos);
          Builder.CreateStore(inV,inGEPData);
          // Keep safe restore values
          if(inV->getType()->isPointerTy()){
            Allocas.push_back(Builder.CreateAlloca(((PointerType*)
                    inV->getType())->getElementType()));
            Builder.CreateStore(Builder.CreateLoad(inV),
                *Allocas.rbegin());
            RestoreInst.push_back(pair<Value*,Value*>(inV,
                  *Allocas.rbegin()));
          }
        }
        // Send function selection
        // TODO: This has to be optimized. Now we search in the function for
        // the select instructions to find it's position and to which BB they 
        // beling
        for(auto &BBtemp: *F)
          for(auto &Itemp: BBtemp){
            MDNode *N;
            BasicBlock *selBB = NULL;
            int pos = 0;
            if (N = (Itemp.getMetadata("fuse.sel.pos"))){
              selBB = cast<Instruction>(cast<ValueAsMetadata>
                (N->getOperand(0).get())->getValue())->getParent();
          }
				  if (N = (Itemp.getMetadata("fuse.livein.pos"))){
					  pos = cast<ConstantInt>(cast<ConstantAsMetadata>
						  	(N->getOperand(0))->getValue())->getSExtValue();
          }
          if(pos and selBB){
  				  Value *inGEPData = Builder.CreateStructGEP(inStruct,pos);
  				  Builder.CreateStore(Builder.getInt1(selBB==BB),inGEPData);
           	visited.insert(pos);
            // In principle the following lines is not needed, since we are 
            // droping the block all together, but just in case
					  iMetadata.insert(&Itemp);
          }
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
			for(int arg = 0; arg < ((StructType*)inputType)->getNumElements();++arg){
				if(!visited.count(arg)){
					Value *inGEPData = Builder.CreateStructGEP(inStruct,arg);
					Value *GlobDat = Builder.CreateLoad(SafeInputs[arg]);
					Builder.CreateStore(GlobDat,inGEPData);
				}
			}

			// Insert Call to offload function
			Builder.CreateCall((Value*)F,ArrayRef<Value*>(inStruct));

			// Predecessor blocks now jump to successors of offloaded BB
			BasicBlock *BBsucc = BB->getSingleSuccessor();
			BasicBlock *BBWrongSucc;
			I = BBpred->getTerminator();

      // We are dropping for now the speculation capability. It makes things 
      // simpler.
      // TODO: Execute speculatvely, and restore the values that changed in
      // memory
			if(((BranchInst*)I)->isConditional()){
				// Restoration Block in case wrong path is taken
				BasicBlock *RestoreBB = BasicBlock::Create(Context,
						"restore"+BB->getName(),BB->getParent());
				if ((*bbList)[i] == I->getSuccessor(0)){
					BBWrongSucc = I->getSuccessor(1);
					Builder.CreateCondBr(((BranchInst*)I)->getCondition(),
							BBsucc,RestoreBB);
				}
				else{
					BBWrongSucc = I->getSuccessor(0);
					Builder.CreateCondBr(((BranchInst*)I)->getCondition(),
							BBsucc,RestoreBB);
				}
				// Restore memory contents in case of wrong path taken
				// in a new Basic Block
				Builder.SetInsertPoint(RestoreBB);
				for(int j = 0;j < RestoreInst.size(); ++j){
					Value *RestoredI = Builder.CreateLoad(
							RestoreInst[j].second);
					Builder.CreateStore(RestoredI,RestoreInst[j].first);
				}
				Builder.CreateBr(BBWrongSucc);
			}
			else{
				Builder.CreateBr(BBsucc);
			}
			I->eraseFromParent();
			verifyFunction(*F);
			Inputs.clear();
		}
		// TODO: Add wrappers to gather/scatter the data

		++i;
	}

	// Remove offloaded BBs
  for(auto BB: *bbList)
		BB->eraseFromParent();

	// TODO: Should we clean metadata like this?
	for(auto I: iMetadata)
		I->dropUnknownNonDebugMetadata();
	// Remap restored Values in each BB
	for(auto Elem : restoreMap){
		Elem.second.first->replaceUsesWithIf(Elem.second.second,
				[Elem](Use &U){return
				((Instruction*)U.getUser())->getParent() == Elem.first;});
	}
	return true;
}
