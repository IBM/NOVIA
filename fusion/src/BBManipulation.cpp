#include "BBManipulation.hpp"

/**
 * Checks if two instructions are mergeable
 *
 * @param *Ia Instruction to check
 * @param *Ib Instruction to check
 * @return true if mergeable, false otherwise
 */
bool is_mergeable(Instruction &Ia, Instruction &Ib){
	// Same opcode
	bool opcode = Ia.getOpcode() == Ib.getOpcode();
	bool sameIm = true;
	errs() << Ia << Ib << "\n";
	errs() << "Merging"  << opcode << '\n';
	// If immediates  are equal
	for(int i = 0; i < Ia.getNumOperands(); ++i){
		Value *opA = Ia.getOperand(i);
		for(int j = 0; j < Ib.getNumOperands(); ++j){
			Value *opB = Ib.getOperand(j);
			errs() << *opA << *opB << '\n';
			if( isa<ConstantInt>(opA) && isa<ConstantInt>(opB) ){
				sameIm = opA == opB;
				errs() << "success\n";
			}
		}
	}
	// Same operands
	// operands come from same operation
	return opcode;
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
	IRBuilder<> builder(C);
	vector<bool> pendingB(B.size(), false);

	// This map keeps track of the relation between old operand - new operand
	map<Value*,Value*> SubOp;
	// for each A instruction we iterate B until we find a mergeable instruction
	// if we find such instruction, we add to C and mark the instruction as 
	// merged in B. Otherwise we just add the new instruction from A.
	for(Instruction &Ia : A){
		unsigned i = 0;
		bool merged = false;
		if (!Ia.isTerminator()){
			for(Instruction &Ib : B){
				if(is_mergeable(Ia,Ib)){
					pendingB[i] = true;
					merged = true;
					break;
				}
			}
			if (not merged){
				Instruction *newInstA = Ia.clone();
				builder.Insert(newInstA);
				SubOp[cast<Value>(&Ia)] = cast<Value>(newInstA);
			}
		}
	}
	// Left-over instructions from B are added now
	unsigned i = 0;
	for(Instruction &Ib : B){
		if (not pendingB[i] or !Ib.isTerminator() ){
			Instruction *newInstB = Ib.clone();
			builder.Insert(newInstB);
			SubOp[cast<Value>(&Ib)] = cast<Value>(newInstB);
		}
		++i;
	}

	//for(Instruction &Ic : *C)
	//	errs() << Ic << '\n';
	// Target operands change name when clonned
	// Here we remap them to their new names to preserve producer-consumer 
	//for(auto &b : SubOp)
	//	errs() << *b.first << " " << *b.second << '\n';
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
	//Module *Mod = new Module("offload", TheContext);
	//Module *Mod = BB.getModule();

	// Instantiate function
	vector<Type *> Integers(1,Type::getInt32PtrTy(TheContext));
	FunctionType *funcType = FunctionType::get(Builder.getInt32Ty(),
			Integers,false);
	Function *f = Function::Create(funcType,
			Function::ExternalLinkage,"offload_func",Mod);

	auto arg = f->arg_begin();
	arg->setName("a");

	// Insert Fused Basic Block
	//BB.setName("entry");
	BB.insertInto(f);

	// Point all LiveIn parameters to Function Arguments
	auto a = BB.begin();
	a++;
	//a->setOperand(1,Builder.getInt32(4));
	//errs() << "orig " << a << " old " << *a->getOperand(1)->getName() << " " << a->getOperand(1)->getType() << '\n'; 
	a++;
	a->eraseFromParent();
	a = BB.begin();
	a->setOperand(0,(Value*)f->arg_begin());
	// TODO: Point all LiveOut parameters to return type

	Builder.SetInsertPoint(&BB);
	Builder.CreateRet(Builder.getInt32(0));

	// Check Consistency
	verifyFunction(*f);
	verifyModule(*Mod);

	// Output Result
	error_code EC;
	raw_fd_ostream of("offload.bc", EC,sys::fs::F_None);
	WriteBitcodeToFile(*Mod,of);
	of.flush();
	of.close();
	//delete Mod;
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
	vector<Value*> Inputs;
	for(auto BB: *bbList){
		errs() << "Next Block\n";
		Module *Mod = BB->getModule();
		// Predecessor blocks change their branches to a function call
		BasicBlock *BBpred = BB->getSinglePredecessor();
		if(BBpred){
			//Inputs.push_back(*BBpred->begin()->op_begin());
			Instruction *I = BBpred->getTerminator();
			Builder.SetInsertPoint(I);
			Value *B = Builder.getInt32(15);


			//Value *A = Builder.CreateAlloca(PointerType::get(Builder.getIntPtrTy(BBpred->getModule()->getDataLayout(),0),0));
			Value* A = Builder.CreateIntToPtr(B,Type::getInt32PtrTy(TheContext));
			Inputs.push_back(A);
			Builder.CreateCall((Value*)F,Inputs);

			// Predecessor blocks now jump to successors of offloaded BB
			BasicBlock *BBsucc = BB->getSingleSuccessor();
			I = BBpred->getTerminator();
			Builder.CreateBr(BBsucc);
			I->eraseFromParent();
			verifyFunction(*F);
			Inputs.clear();
		}
		BB->eraseFromParent();


		// TODO: Add wrappers to gather/scatter the data
		SetVector<Value*> LiveIn, LiveOut;
		liveInOut(F->front(), &LiveIn, &LiveOut);


	}
	return true;
}
