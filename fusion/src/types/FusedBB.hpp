#include <set>
#include <string>

#include "llvm/Pass.h"
#include "llvm/IR/BasicBlock.h"

using namespace llvm;
using namespace std;

class FusedBB{
  private:
    BasicBlock *BB;
    set<BasicBlock*> *mergedBBs;
  public:
    // constructore
    FusedBB();
    FusedBB(LLVMContext&,string);
    // destructor
    ~FusedBB();

    //
    void addMergedBB(BasicBlock *);
  
    // getter funcionts
    unsigned getNumMerges(void);
};

