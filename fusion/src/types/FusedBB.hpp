#ifndef FUSEDBB_H
#define FUSEDBB_H

#include <set>
#include <map>
#include <string>
#include <algorithm>

#include "../BBAnalysis.hpp"
#include "../Maps.hpp"

#include "llvm/Pass.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/CFG.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "llvm/ADT/SetVector.h"

using namespace llvm;
using namespace std;

class FusedBB{
  private:
    LLVMContext *Context;
    BasicBlock *BB;
    set<BasicBlock*> *mergedBBs; // What BB I merged
    map<Instruction*,Instruction*> *LiveOut; // What LiveOuts are mapped
    //map<Instruction*,Instruction*> LiveIn; // LiveIn mapping
    map<Value*,BasicBlock*> *argsBB; // Selection value maping to BBs
    map<Value*,set<pair<Value*,BasicBlock*> >* > *linkOps;
    map<Instruction*,int> *storeDomain;
    map<Instruction*,set<Instruction*>* > *rawDeps;
    //map<Instruction*,set<Instruction*>* > *warDeps;
    map<Instruction*,set<Instruction*>* > *fuseMap;
    map<Instruction*,BasicBlock*> *selMap;

    // Stats & Analytics
    map<Instruction*,set<BasicBlock*>* > *annotMerges;
    map<Instruction*,int> *countMerges;
    map<Instruction*,unsigned> *safeMemI;

    // Positions
    map<Value*,map<BasicBlock*,int>* > *liveInPos;
    map<Value*,int> *liveOutPos;
    //set<Value*> LiveOut;
    set<Value*> *LiveIn;
  public:
    // constructor
    FusedBB();
    FusedBB(LLVMContext*,string); // Empty BB Constructor
    FusedBB(BasicBlock*);         // Init BB Constructor
    FusedBB(FusedBB*,FusedBB*);   // Merge Constructor
    // copye
    FusedBB(FusedBB*);
    // destructor
    ~FusedBB();
    //init
    void init(LLVMContext*);
    // mergers
    void mergeBB(BasicBlock *BB);
    void mergeOp(Instruction*, Instruction*,map<Value*,Value*> *,
        IRBuilder<>*,Value*,set<Value*>*);
    // spliters
    void splitBB(vector<list<Instruction*> *> *);
    // enablers
    Function *createOffload(Module *);
    bool insertCall(Function *, vector<BasicBlock*> *);

    // helpers
    bool searchDfs(Instruction*,Instruction*,set<Instruction*>*);
    bool checkNoLoop2(Instruction*,Instruction*,BasicBlock*,map<Value*,Value*>*);
    bool checkNoLoop(Instruction*,Instruction*,BasicBlock*);
    void secureMem(Value*,BasicBlock*);
    void KahnSort(); 
    void dropSafety(Instruction*);
    void addSafety(Instruction*);
    void updateRawDeps(map<Value*,Value*>*);
    void updateStoreDomain(map<Value*,Value*>*);

    bool rCycleDetector(Instruction *I, set<Instruction*> *, list<Instruction*> *,set<Instruction*>*,int);
    void CycleDetector(Function*);

    // linkage functions
    void linkLiveOut(Instruction*,Instruction*,SetVector<Value*> *);
    void linkIns(Instruction*,Instruction*);
    void memRAWDepAnalysis(BasicBlock*);


    // info functions 
    void addMergedBB(BasicBlock *);
    void annotateMerge(Instruction*,Instruction*,BasicBlock*);

    // getter funcionts
    int size();
    unsigned getNumMerges(Instruction*);
    unsigned getNumMerges();
    unsigned getMaxMerges();
    float getAreaOverhead();
    string getName();
    unsigned getStoreDomain(Instruction*,BasicBlock*);
    BasicBlock* getBB();
    void getMetrics(vector<float>*,Module*);
    void fillSubgraphsBBs(Instruction*,set<string>*);
    float getTseqSubgraph(list<Instruction*> *, map<string,long> *);
    float getTseq();
  
    // Hardware
    //bool getVerilog(raw_fd_stream&);

    // tester functions
    bool isMergedBB(BasicBlock*);
    bool isMergedI(Instruction *I);
};
#endif
