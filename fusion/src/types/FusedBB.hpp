#include <set>

#include "llvm/Pass.h"
#include "llvm/IR/BasicBlock.h"

using namespace llvm;
using namespace std;

class FusedBB{
  private:
    set<BasicBlock*> *mergedBBs;

  public:
    void addMergedBB(BasicBlock *);

    // getter funcionts
    unsigned getNumMerges(void);

};

