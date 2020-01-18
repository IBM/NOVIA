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
	errs() << opcode << '\n';
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
	//SetVector<Value*> LiveIn, LiveOut;
	//liveInOut(A,&LiveIn,&LiveOut);

	// Generate a copy of the List of instructions to be merged
	// If C points to either A or B adding instructions will impact the merging
	// process
	/*ValueToValueMapTy VMap;
	SmallVector<BasicBlock*,1> auxV;
	BasicBlock *Acopy = CloneBasicBlock(&A, VMap);
	auxV.push_back(Acopy);
	for(Instruction &I : *Acopy)
		errs() << I << '\n';
	errs() << "-------\n";
	remapInstructionsInBlocks(auxV, VMap);
	for(auto it : VMap)
		errs() << *it->first << " " << *it->second << '\n';
	errs() << "-------\n";
	//errs() << A;
	Value *Op = Acopy->front().getOperand(0);
	Op = &cast<Value>(A.front());
	for(Instruction &I : *Acopy)
		errs() << I << '\n';
	errs() << "-------\n";
	BasicBlock *Bcopy = CloneBasicBlock(&B, VMap);*/

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
	for(auto &b : SubOp)
		errs() << *b.first << " " << *b.second << '\n';
	for(Instruction &Ic : *C){
		for(int j = 0; j < Ic.getNumOperands(); ++j){
			Value *Vc = Ic.getOperand(j);
			errs() << "1: " << *Vc << "\n";
			if ( SubOp.count(Vc) ){
			errs() << "2 " << *SubOp[Vc] << '\n';
				//Ic.setOperand(j,SubOp[Vc]);
				//Vc = SubOp[Vc];
				//Vc->replaceAllUsesWith(SubOp[Vc]);
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
