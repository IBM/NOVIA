#ifndef FUSEDBB_H
#define FUSEDBB_H

#include <set>
#include <map>
#include <string>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include "../BBAnalysis.hpp"
#include "../Maps.hpp"

#include "llvm/Pass.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "llvm/ADT/SetVector.h"

//the following are UBUNTU/LINUX, and MacOS ONLY terminal color codes.
#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

typedef struct {
  int line;
  int col;
  bool merged;
} debug_desc;
  
static bool operator< (const debug_desc &a, const debug_desc &b){ return a.line < b.line; }

using namespace llvm;
using namespace std;

class FusedBB{
  private:
    LLVMContext *Context;
    BasicBlock *BB;
    set<BasicBlock*> *mergedBBs; // What BB I merged
    
    map<Instruction*,int> *storeDomain;
    map<Instruction*,set<Instruction*>* > *rawDeps;
    //map<Instruction*,set<Instruction*>* > *warDeps;
    map<Instruction*,set<Instruction*>* > *fuseMap;
    map<Instruction*,pair<set<BasicBlock*>*,set<BasicBlock*>*> > *selMap;

    // Stats & Analytics
    //set<Instruction*> *annotInst;
    map<Instruction*,int> *countMerges;
    map<Instruction*,unsigned> *safeMemI;
    float orig_weight;

    // Positions
    vector<map<BasicBlock*,Value*>* > *liveInPos;
    vector<map<BasicBlock*,set<Value*>*> * > *liveOutPos;
    
    // Live Values
    map<Value*,set<pair<Value*,BasicBlock*> >* > *linkOps;
    set<Value*> *LiveIn;
    set<Value*> *LiveOut; // What LiveOuts are mapped
  public:
    // constructor
    FusedBB();
    FusedBB(LLVMContext*,string); // Empty BB Constructor
    FusedBB(BasicBlock*);         // Init BB Constructor
    FusedBB(FusedBB*,FusedBB*);   // Merge Constructor
    // copye
    FusedBB(FusedBB*);
    FusedBB(FusedBB*,list<Instruction*>*);
    // destructor
    ~FusedBB();
    //init
    void init(LLVMContext*);
    // mergers
    void mergeBB(BasicBlock *BB);
    void mergeOp(Instruction*, Instruction*,map<Value*,Value*> *,
        IRBuilder<>*,Value*,set<Value*>*);
    // spliters
    void splitBB(vector<list<Instruction*> *> *,vector<vector<double>*>*,
        vector<BasicBlock*>*);
    void traverseExclude(Instruction *I, set<Instruction*> *visited, 
        set<Instruction*> *excluded, list<Instruction*> *list);
    void traverseSubgraph(Instruction*, set<Instruction*> *, 
    set<Instruction*> *, list<Instruction*> *, vector<vector<double>*> *,
    vector<BasicBlock*> *, float , map<BasicBlock*,double> *);
    // enablers
    Function *createOffload(Module *);
    Function *createInline(Module *);
    void inlineOutputSelects(SelectInst *,int);
    void inlineInputSelects(SelectInst *,int);
    bool insertOffloadCall(Function *);
    bool insertInlineCall(Function *, map<Value*,Value*> *);
    void removeOrigInst();

    // helpers
    Value* searchSelects(Value*,Value*,Instruction*,Instruction*,
        map<Value*,Value*> *,IRBuilder<>*,Value*);
    Value* checkSelects(Value*,Value*,BasicBlock*,map<Value*,Value*>*,set<Value*> *);
    bool checkLinks(Value*,Value*,BasicBlock*);
    bool searchDfs(Instruction*,Instruction*,set<Instruction*>*);
    bool checkNoLoop2(Instruction*,Instruction*,BasicBlock*,map<Value*,Value*>*);
    bool checkNoLoop(Instruction*,Instruction*,BasicBlock*);
    void secureMem(Value*,BasicBlock*,set<Value*>*);
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
    void getDebugLoc(stringstream&);
    string getSelBB(Instruction*,Value*);
    
    // setter functions
    void setOrigWeight(float);

    // getter funcionts
    int size();
    float getOrigWeight();
    unsigned getNumMerges(Instruction*);
    unsigned getNumMerges();
    unsigned getMaxMerges();
    float getAreaOverhead();
    string getName();
    unsigned getStoreDomain(Instruction*,BasicBlock*);
    BasicBlock* getBB();
    void getMetrics(vector<double>*,Module*);
    void fillSubgraphsBBs(Instruction*,set<string>*);
    float getTseqSubgraph(list<Instruction*> *, map<string,long> *, 
        map<BasicBlock*,float> *);
    float getTseq();
    //Stats
    pair<float,pair<float,float> > overheadCosts(map<string,long> *);
    // Hardware
    //bool getVerilog(raw_fd_stream&);

    // tester functions
    bool isMergedBB(BasicBlock*);
    bool isMergedI(Instruction *I);
};
#endif
