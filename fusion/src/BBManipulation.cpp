#include "BBManipulation.hpp"

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

	return opcode;
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
 * Merges two basic blocks
 *
 * @param *A BasicBlock to merge
 * @param *B BasicBlock to merge
 
 * @return Returns a newly created BasicBlock that contains a fused list
 * of instructions from A and B. The block does not have a terminator.
 */
BasicBlock* mergeBBs(BasicBlock &A, BasicBlock &B){
	BasicBlock *C = BasicBlock::Create(A.getContext(),A.getName()+B.getName());
	LLVMContext & Context = A.getContext();
	IRBuilder<> builder(Context);
	builder.SetInsertPoint(C);
	vector<bool> pendingB(B.size(), true);

	// This map keeps track of the relation between old operand - new operand
	map<Value*,Value*> SubOp;
	// for each A instruction we iterate B until we find a mergeable instruction
	// if we find such instruction, we add to C and mark the instruction as 
	// merged in B. Otherwise we just add the new instruction from A.
	for(Instruction &Ia : A){
		int i = 0;
		int OpC = -1;
		Value *SelI = NULL;
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

						}
						//  Operands cannot be merged, add select inst in case
						//  they are the same type
						else if( opA->getType() == opB->getType()){
							Value *SelVal = new Argument(builder.getInt1Ty());
							SelI = builder.CreateSelect(SelVal,opA,opB);
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
	for(Instruction &Ic : *C){
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
Function* createOffload(BasicBlock &BB, Module *Mod){
	LLVMContext &Context = BB.getContext();
	static IRBuilder<> Builder(Context);
	vector<Type *> Inputs;
	SetVector<Value*> LiveIn, LiveOut;
	liveInOut(BB, &LiveIn, &LiveOut);


	// Instantiate function
	// Instantate all LiveIn values as Inputs
	for(Value *V: LiveIn){
		Inputs.push_back(V->getType());
	}
	// Input struct
	StructType *Transfer = StructType::create(Context, Inputs);
	vector<Type*> Aux;
	Aux.push_back(Transfer);

	//vector<Type *> Integers(1,Type::getInt32PtrTy(Context));
	FunctionType *funcType = FunctionType::get(Builder.getInt32Ty(),
			Inputs,false);
	Function *f = Function::Create(funcType,
			Function::ExternalLinkage,"offload_func",Mod);

	// Insert Fused Basic Block
	BB.insertInto(f);

	errs() << "ok " << funcType->getNumParams() << "\n";
	errs() << "ok " << Transfer->getNumElements() << "\n";
	errs() << "ok " << ((StructType*)f->getArg(0))->getNumElements() << "\n";
	for(auto &Elem : ((StructType*)f->getArg(0))->elements()){
		errs() << Elem;
	}
	errs() << "ok\n";

	for(int i=0; i<LiveIn.size();++i){
		for(auto &I: BB){
			for(int j=0;j<I.getNumOperands();++j){
				if(I.getOperand(j) == LiveIn[i]){
					I.setOperand(j,f->getArg(i));
				}
			}
		}
		//LiveIn[i]->replaceAllUsesWith(f->getArg(i));
		// Preserve names for arguments 
		f->getArg(i)->setName(LiveIn[i]->getName());
	}

	// TODO: Point all LiveOut parameters to return type

	Builder.SetInsertPoint(&BB);
	Builder.CreateRet(Builder.getInt32(0));
	verifyModule(*Mod);

	// Check Consistency
	verifyFunction(*f);
	f->dump();
	Mod->dump();


	// Add Metadata (footprint)
	int footprint = 0;
	for(auto &A: f->args()){
		if(A.getType()->isPtrOrPtrVectorTy()){
			footprint += ((PointerType*)A.getType())->getElementType()
				->getScalarSizeInBits();
		}
		else{
			footprint += A.getType()->getScalarSizeInBits();
		}
	}
	MDNode* temp = MDNode::get(Context,ConstantAsMetadata::get(
				ConstantInt::get(Context,llvm::APInt(64,footprint,false))));
	MDNode* N = MDNode::get(Context, temp);
	f->setMetadata("memoryFootprint",N);

	// Output Result
	Module *NewMod = new Module("offload_mod",Context);
	/*Function *Newf = Function::Create(funcType,
			Function::ExternalLinkage,"offload_func",NewMod);*/
	ValueToValueMapTy Vmap;
	Function *Newf = CloneFunction(f,Vmap);
	Newf->removeFromParent();
	NewMod->getFunctionList().push_back(Newf);
	error_code EC;
	raw_fd_ostream of("offload.bc", EC,sys::fs::F_None);
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
	vector<GlobalVariable*> SafeInputs;
	vector<GlobalVariable*> RestoredInputs;

	// Restored Values must replace LiveIn values
	map<BasicBlock*,pair<Value*,Value*> > restoreMap;
	int i = 0; // Pointer to BB in bbList being processed

	// Create Safe Globals for Input vectors
	// TODO: Do not copy select variables
	for(auto &O : F->args()){
		SafeInputs.push_back(new GlobalVariable(O.getType(),true,
					GlobalValue::PrivateLinkage));

	}


	for(auto BB: *bbList){
		int arg = 0;
		ValueSymbolTable *VS = BB->getValueSymbolTable();
		errs() << "Next Block\n";

		// Search for Inputs and assign as arguments
		for(auto &O: F->args()){
			// If Input is within BB visbility assign Value, otherwise assign
			// dummy Input
			if(Value *V = VS->lookup(O.getName()))
				Inputs.push_back(V);
			else if(arg < F->getFunctionType()->getNumParams() - bbList->size()){
				Inputs.push_back(SafeInputs[arg]);
			}
			// Once we reach selection variables, assign proper value
			else{
				V = Builder.getInt1(arg==i+(F->getFunctionType()->getNumParams()-bbList->size()));
				Inputs.push_back(V);
			}
			++arg;
		}

		// Predecessor blocks change their branches to a function call
		BasicBlock *BBpred = BB->getSinglePredecessor();
		if(BBpred){
			Instruction *I = BBpred->getTerminator();
			Builder.SetInsertPoint(I);

			// Sending Data to Memory
			vector<AllocaInst*> Allocas;
			vector<pair<Value*, Value*> > RestoreInst;
			for(int j = 0; j < Inputs.size() - (bbList->size()-1) ; ++j){
				if(Inputs[j]->getType()->isPointerTy()){
					Allocas.push_back(Builder.CreateAlloca(((PointerType*)
									Inputs[j]->getType())->getElementType()));
					Builder.CreateStore(Builder.CreateLoad(Inputs[j]),
							*Allocas.rbegin());
					RestoreInst.push_back(pair<Value*,Value*>(Inputs[j],
								*Allocas.rbegin()));
				}
			}

			// Insert Call to offload function
			Builder.CreateCall((Value*)F,Inputs);

			// Predecessor blocks now jump to successors of offloaded BB
			BasicBlock *BBsucc = BB->getSingleSuccessor();
			BasicBlock *BBWrongSucc;
			I = BBpred->getTerminator();

			if(((BranchInst*)I)->isConditional()){
				// Restoratio Block in case wrong path is taken
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
		// Remove offloaded BB
		BB->eraseFromParent();
		// TODO: Add wrappers to gather/scatter the data

		++i;
	}
	// Remap restored Values in each BB
	for(auto Elem : restoreMap){
		Elem.second.first->replaceUsesWithIf(Elem.second.second,
				[Elem](Use &U){return
				((Instruction*)U.getUser())->getParent() == Elem.first;});
	}
	return true;
}
