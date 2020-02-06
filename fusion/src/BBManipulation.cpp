#include "BBManipulation.hpp"

/**
 * Frees BasicBlock and pointer
 *
 * @param BB BasicBlock to free
 * @return
 */
void freeBB(BasicBlock* BB){
	for(auto &I: *BB){
		I.dropAllReferences();
	}
	if(BB->getParent())
		BB->eraseFromParent();
	else
		delete BB;
	return;
}


/**
 * Checks if two instructions are mergeable
 *
 * @param *Ia Instruction to check
 * @param *Ib Instruction to check
 * @return true if mergeable, false otherwise
 */
bool areInstMergeable(Instruction &Ia, Instruction &Ib){
	// Same opcode
	bool opcode = Ia.getOpcode() == Ib.getOpcode();
	bool sameIm = true;
	//errs() << Ia << Ib << "\n";
	//errs() << "Merging"  << opcode << '\n';
	// If immediates  are equal
	for(int i = 0; i < Ia.getNumOperands(); ++i){
		Value *opA = Ia.getOperand(i);
		for(int j = 0; j < Ib.getNumOperands(); ++j){
			Value *opB = Ib.getOperand(j);
			//errs() << *opA << *opB << '\n';
			if(isa<ConstantInt>(opA) && isa<ConstantInt>(opB)){
				sameIm = opA == opB;
				//errs() << "success\n";
			}
		}
	}
	// Same operands
	// operands come from same operation
	return opcode;
}

/**
 * Checks if operands can be merged
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
 * @param *C resultant BasicBlock
 * @return success/failure
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
				if(areInstMergeable(Ia,Ib)){
					pendingB[i] = false;
					for(int j = 0; j < Ia.getNumOperands(); ++j){
						Value *opA = Ia.getOperand(j);
						Value *opB = Ib.getOperand(j);
						if(areOpsMergeable(opA,opB,&A,&B)){

						}
						//else if( isa<ConstantInt>(opA) && isa<ConstantInt>(opB)){
						else if( opA->getType() == opB->getType()){
							errs() << "Creating Select\n";
							//Value *SelVal = UndefValue::get(builder.getInt1Ty());
							Value *SelVal = new Argument(builder.getInt1Ty());
							//Value *SelI = SelectInst::Create(SelVal,opA,opB,"select",C);
							SelI = builder.CreateSelect(SelVal,opA,opB);
							OpC = j;
							//builder.CreateAdd(SelVal32,SelI);
							//builder.Insert((Instruction*)SelI);
						}
						else {
						}
					}
					break;
				}
				++i;
			}
			Instruction *newInstA = Ia.clone();
			builder.Insert(newInstA);
			if(SelI)
				newInstA->setOperand(OpC,SelI);
			SubOp[cast<Value>(&Ia)] = cast<Value>(newInstA);
		}
		errs() << "BB Build\n";
		for(auto &I:*C)
			errs() << I << '\n';
	}
	// Left-over instructions from B are added now
	unsigned i = 0;
	for(Instruction &Ib : B){
		if (pendingB[i] and !Ib.isTerminator() ){
			for(auto U = Ib.use_begin(); U != Ib.use_end() & pendingB[i]; ++U){
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
	for(auto &b : SubOp)
		errs() << *b.first << " " << *b.second << '\n';
	for(Instruction &Ic : *C){
		for(int j = 0; j < Ic.getNumOperands(); ++j){
			Value *Vc = Ic.getOperand(j);
			//errs() << "1: " << *Vc << "\n";
			if ( SubOp.count(Vc) ){
				//errs() << "2 " << *SubOp[Vc] << '\n';
					Ic.setOperand(j,SubOp[Vc]);
					//Vc = SubOp[Vc];
					//Vc->replaceAllUsesWith(SubOp[Vc]);
				}
				else if ( ConstantInt *CI = dyn_cast<ConstantInt>(Vc)){
					//Ic.setOperand(j,builder.getInt32(CI->getSExtValue()));
				}
			}
		}

	errs()<< "Inst :\n";
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
	LLVMContext &TheContext = BB.getContext();
	static IRBuilder<> Builder(TheContext);
	vector<Type *> Inputs;
	SetVector<Value*> LiveIn, LiveOut;
	liveInOut(BB, &LiveIn, &LiveOut);


	// Instantiate function
	for(Value *V: LiveIn){
		Inputs.push_back(V->getType());
	}
	// Input struct
	StructType *Transfer = StructType::get(TheContext, Inputs);
	vector<Type*> Aux;
	Aux.push_back(Transfer);

	//vector<Type *> Integers(1,Type::getInt32PtrTy(TheContext));
	FunctionType *funcType = FunctionType::get(Builder.getInt32Ty(),
			Inputs,false);
	Function *f = Function::Create(funcType,
			Function::ExternalLinkage,"offload_func",Mod);

	// Insert Fused Basic Block
	BB.insertInto(f);

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

	/*auto a = BB.begin();
	a++;
	//a->setOperand(1,Builder.getInt32(4));
	//errs() << "orig " << a << " old " << *a->getOperand(1)->getName() << " " << a->getOperand(1)->getType() << '\n'; 
	a++;
	a->eraseFromParent();
	a = BB.begin();
	a->setOperand(0,(Value*)f->arg_begin());*/
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
			footprint += ((PointerType*)A.getType())->getElementType()->getScalarSizeInBits();
		}
		else{
			footprint += A.getType()->getScalarSizeInBits();
		}
	}
	MDNode* temp = MDNode::get(TheContext, ConstantAsMetadata::get(ConstantInt::get(
					TheContext, llvm::APInt(64,footprint,false))));
	MDNode* N = MDNode::get(TheContext, temp);
	f->setMetadata("memoryFootprint",N);

	// Output Result
	Module *NewMod = new Module("offload_mod",TheContext);
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
	LLVMContext &TheContext = F->getContext();
	static IRBuilder<> Builder(TheContext);
	int i = 0, arg = 0;
	for(auto BB: *bbList){
		ValueSymbolTable *VS = BB->getValueSymbolTable();
		errs() << "Next Block\n";
		vector<Value*> Inputs;

		// Search for Inputs and assign as arguments
		for(auto &O: F->args()){
			if(Value *V = VS->lookup(O.getName()))
				Inputs.push_back(V);
			else{
				V = Builder.getInt1(i==arg);
				Inputs.push_back(V);
			}
			++arg;
		}

		// Predecessor blocks change their branches to a function call
		BasicBlock *BBpred = BB->getSinglePredecessor();
		if(BBpred){
			//Inputs.push_back(*BBpred->begin()->op_begin());
			Instruction *I = BBpred->getTerminator();
			if(((BranchInst*)I)->isConditional()){
					Inputs[1+i] =
						I->getOperand(0);
			}
			Builder.SetInsertPoint(I);

			// Sending Data to Memory
			vector<AllocaInst*> Allocas;
			for(auto &O: F->args()){
				Allocas.push_back(Builder.CreateAlloca(O.getType()));
			}
			for(int j = 0; j < Inputs.size() ; ++j){
				Builder.CreateStore(Inputs[j],Allocas[j]);
			}



			// Insert Call to offload function
			Builder.CreateCall((Value*)F,Inputs);

			// Predecessor blocks now jump to successors of offloaded BB
			BasicBlock *BBsucc = BB->getSingleSuccessor();
			I = BBpred->getTerminator();
			Builder.CreateBr(BBsucc);
			I->eraseFromParent();
			verifyFunction(*F);
			Inputs.clear();
		}
		// Remove offloaded BB
		BB->eraseFromParent();
		// TODO: Add wrappers to gather/scatter the data

		++i;
	}
	return true;
}
