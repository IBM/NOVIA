#include <string>
#include <map>

#include "llvm/ADT/SetVector.h"

#include "llvm/Pass.h"
#include "llvm/Analysis/DDG.h"
#include "llvm/ADT/DirectedGraph.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/CodeExtractor.h"

using namespace llvm;
using namespace std;


// Analysis functions
void liveInOut(BasicBlock&,SetVector<Value*>*,SetVector<Value*>*);
void memRAWDepAnalysis(BasicBlock*,map<Value*,Value*> *,LLVMContext &);


// Stats functions
void getMetadataMetrics(BasicBlock*,vector<float>*, Module*);
