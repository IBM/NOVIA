#include <string>
#include <unordered_set>
#include <map>

#include "BBAnalysis.hpp"
#include "FuseSupport.hpp"

#include "llvm/Analysis/DDG.h"
#include "llvm/ADT/DirectedGraph.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Pass.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/ADT/SetVector.h"

using namespace llvm;
using namespace std;

void separateBr(BasicBlock *BB);
void KahnSort(BasicBlock *BB);
