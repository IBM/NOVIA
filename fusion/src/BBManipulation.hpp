#include <string>
#include <map>

#include "BBAnalysis.hpp"

#include "llvm/Analysis/DDG.h"
#include "llvm/ADT/DirectedGraph.h"
#include "llvm/Pass.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/ADT/SetVector.h"

using namespace llvm;
using namespace std;

bool is_mergeable(Instruction&, Instruction&);
BasicBlock* mergeBBs(BasicBlock&, BasicBlock&);
void listBBInst(BasicBlock&);
