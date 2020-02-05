#include "BBAnalysis.hpp"

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
	//ValueSet LiveIn, LiveOut, Allocas;
	ValueSet Allocas;
	ValueSet Li, Lo;
	CE.findInputsOutputs((ValueSet&)*LiveIn,(ValueSet&)*LiveOut,Allocas);
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

void doNothing(SetVector<Value*> A){
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
