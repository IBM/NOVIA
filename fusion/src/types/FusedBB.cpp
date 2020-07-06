#include "FusedBB.hpp"

FusedBB::~FusedBB(){
  delete mergedBBs;
  delete LiveOut;
  for(auto E : *linkOps)
    delete E.second;
  delete linkOps;
  delete argsBB;
  for(auto E: *annotMerges)
    delete E.second;
  delete annotMerges;
  delete countMerges;
  delete storeDomain;
  for(auto E : *rawDeps)
    delete E.second;
  delete rawDeps;
  for(auto E : *fuseMap)
    delete E.second;
  delete fuseMap;
  delete safeMemI;
  delete selMap;
  for(auto E : *liveInPos)
    delete E.second;
  delete liveInPos;
  delete liveOutPos;
  delete BB;
  return;
}

FusedBB::FusedBB(FusedBB* Copy){
  ValueToValueMapTy VMap;
  BB = CloneBasicBlock(Copy->getBB(),VMap);
  SmallVector<BasicBlock*,1> bbList;
  bbList.push_back(this->BB);
  remapInstructionsInBlocks(bbList,VMap);
  
  this->Context = Copy->Context;
  mergedBBs = new set<BasicBlock*>(*Copy->mergedBBs);

  LiveOut = new map<Instruction*, Instruction* >;
  for(auto E : *Copy->LiveOut){
    (*LiveOut)[E.first] = cast<Instruction>(VMap[cast<Value>(E.second)]);
  }

  linkOps = new map<Value*,set<pair<Value*,BasicBlock*> >* >;
  for(auto E : *Copy->linkOps){
    if(VMap[E.first])
      (*linkOps)[VMap[E.first]] = new set<pair<Value*,BasicBlock*> >(*E.second);
    else
      (*linkOps)[E.first] = new set<pair<Value*,BasicBlock*> >(*E.second);
  }

  argsBB = new map<Value*,BasicBlock*>;
  for( auto E : *Copy->argsBB )
    if(VMap[E.first])
      (*argsBB)[VMap[E.first]] = E.second;
    else
      (*argsBB)[E.first] = E.second;

  annotMerges = new map<Instruction*,set<BasicBlock*>* >;
  for(auto E : *Copy->annotMerges)
    (*annotMerges)[cast<Instruction>(VMap[cast<Value>(E.first)])] = new set<BasicBlock*>(*E.second);

  countMerges = new map<Instruction*,int>;
  for(auto E : *Copy->countMerges)
    (*countMerges)[cast<Instruction>(VMap[cast<Value>(E.first)])] = E.second;

  storeDomain = new map<Instruction*,int>;
  for(auto E : *Copy->storeDomain)
    (*storeDomain)[cast<Instruction>(VMap[cast<Value>(E.first)])] = E.second;


  rawDeps = new map<Instruction*,set<Instruction*>* >;
  for(auto E : *Copy->rawDeps){
    set<Instruction*> *aux = new set<Instruction*>;
    (*rawDeps)[cast<Instruction>(VMap[cast<Value>(E.first)])] = aux;
    for(auto sE : *E.second)
      aux->insert(cast<Instruction>(VMap[cast<Value>(sE)]));
  }

  fuseMap = new map<Instruction*,set<Instruction*>* >;
  for(auto E : *Copy->fuseMap)
    fuseMap->insert(pair<Instruction*,set<Instruction*>* >(cast<Instruction>(VMap[cast<Value>(E.first)]),new set<Instruction*>(*E.second)));

  safeMemI = new map<Instruction*,unsigned>;
  for(auto E : *Copy->safeMemI)
    (*safeMemI)[cast<Instruction>(VMap[cast<Value>(E.first)])] = E.second;

  selMap = new map<Instruction*,BasicBlock*>;
  for(auto E : *Copy->selMap)
    (*selMap)[cast<Instruction>(VMap[cast<Value>(E.first)])] = E.second;

  liveInPos = new map<Value*,map<BasicBlock*,int>* >;
  for(auto E : *Copy->liveInPos)
    if(VMap[E.first])
      (*liveInPos)[VMap[E.first]] = new map<BasicBlock*,int> (*E.second);
    else
      (*liveInPos)[E.first] = new map<BasicBlock*,int> (*E.second);

  liveOutPos = new map<Value*,int>;
  for(auto E : *Copy->liveOutPos)
    (*liveOutPos)[VMap[E.first]] = E.second;
}

void FusedBB::init(LLVMContext *Context){
  mergedBBs = new set<BasicBlock*>; 
  LiveOut = new map<Instruction*, Instruction* >;
  linkOps = new map<Value*,set<pair<Value*,BasicBlock*> >* >;
  argsBB = new map<Value*,BasicBlock*>;
  annotMerges = new map<Instruction*,set<BasicBlock*>* >;
  countMerges = new map<Instruction*,int>;
  storeDomain = new map<Instruction*,int>;
  rawDeps = new map<Instruction*,set<Instruction*>* >;
  fuseMap = new map<Instruction*,set<Instruction*>* >;
  safeMemI = new map<Instruction*,unsigned>;
  selMap = new map<Instruction*,BasicBlock*>;
  liveInPos = new map<Value*,map<BasicBlock*,int>* >;
  liveOutPos = new map<Value*,int>;
  this->Context = Context;
  return;
}

FusedBB::FusedBB(LLVMContext *Context, string name){
  BB = BasicBlock::Create(*Context, name);
  init(Context);
  return;
}

void FusedBB::addMergedBB(BasicBlock *BB){
  mergedBBs->insert(BB);
  return;
}

void FusedBB::mergeOp(Instruction *Ifused, Instruction *Iorig,
    map<Value*,Value*> *SubOp, IRBuilder<> *Builder,
    Value *SelVal){
  map<int,int> match;
  set<int> used;
  for(int i = 0;i < Ifused->getNumOperands(); ++i){
    Value *Vfused = Ifused->getOperand(i);
    for(int j = 0; j < Iorig->getNumOperands() and !match.count(i); ++j){
      Value *Vorig = Iorig->getOperand(j);
      if(areOpsMergeable(Vfused,Vorig,Ifused->getParent(),Iorig->getParent(),SubOp)
          and !used.count(j)){
        match[i] = j;
        used.insert(j);
      }
    }
  }
  // Matchings found, now merge or add selects
  for(int i = 0;i<Ifused->getNumOperands();++i){
    Value *Vfused = Ifused->getOperand(i);
    Value *SelI = NULL;
    if(match.count(i)){
      Value *Vorig = Iorig->getOperand(match[i]);
      if(!linkOps->count(Vfused))
        (*linkOps)[Vfused] = new set<pair<Value*,BasicBlock*> >;
      (*linkOps)[Vfused]->insert(pair<Value*,BasicBlock*>(Vorig,Iorig->getParent())); 
    } 
    else{
      for(int j = 0; j < Iorig->getNumOperands() and !SelI; ++j){
        if(!used.count(j)){
          Value *Vorig = Iorig->getOperand(j);
          assert( Vfused->getType() == Vorig->getType() && " Types differ, wrong merge" );
          SelI = Builder->CreateSelect(SelVal,Vorig,Vfused,"fuse.sel");
          (*selMap)[cast<Instruction>(SelI)] = Iorig->getParent();
          Ifused->setOperand(j,SelI);
          used.insert(j);
        }
      }
    }
  }
}



/**
 *Generate safe loads
 *
 *
 */
void FusedBB::secureMem(Value *SelVal, BasicBlock *destBB){
  for(Instruction &I : *BB){
    // safe value of 1 means the load has been merged with all BBs and it's safe to use since
    // at least 1 instance will provide always a safe pointer
    // safe value of 2 means that the load was secured on a previous merge step and therefore
    // there exists a safe pointer in case the load is not used, hence no need to add it again
    if((isa<LoadInst>(&I) or isa<StoreInst>(&I)) and !safeMemI->count(&I)){
      Value *oPtr = NULL;
      LoadInst *Li = NULL;
      StoreInst *Si = NULL;
      int index = -1;
      if(Li = (dyn_cast<LoadInst>(&I))){
        oPtr = Li->getPointerOperand();
        index = LoadInst::getPointerOperandIndex();
      }
      else if(Si = (dyn_cast<StoreInst>(&I))){
        oPtr = Si->getPointerOperand();
        index = StoreInst::getPointerOperandIndex();
      }
      if(Li or Si){
        Value *SelSafe = NULL;
        Module *Mod = destBB->getParent()->getParent();
        IRBuilder<> builder(*Context);
        builder.SetInsertPoint(&I);
        Value *G = getSafePointer(cast<PointerType>(oPtr->getType()),Mod);
        // If the instruction comes from B we put in normal order, otherwise we swap
        for(auto Iorig = (*fuseMap)[&I]->begin(); Iorig != (*fuseMap)[&I]->end() and !SelSafe; ++Iorig)
          if((*Iorig)->getParent() ==  destBB)
            SelSafe = builder.CreateSelect(SelVal,oPtr,G,"fuse.sel.safe"); 
        if(!SelSafe)
          SelSafe = builder.CreateSelect(SelVal,G,oPtr,"fuse.sel.safe");
        (*selMap)[cast<Instruction>(SelSafe)] = destBB;
        (*safeMemI)[&I] = 2;
        I.setOperand(index,SelSafe);          
      }
    }    
  }
}


/*
 * Check wether merging this two instructions would create a cycle in the dep graph
 *
 * @param Ia Instruction from BB A to merge
 * @param Ib Instruction from BB B to merge
 * @param *C BasicBlock to merge
 */
bool FusedBB::checkNoLoop(Instruction *Iref, Instruction *Isearch, BasicBlock *BB){
  bool noloop = true;
  if(Iref == Isearch)
    return false;
  else{
      for(auto user = Isearch->user_begin(); user!= Isearch->user_end(); user++){
        Instruction *Up = (Instruction*)*user;
        if(fuseMap->count(Up)){
          for(auto Iorig : *(*fuseMap)[Up])
            if(Iorig->getParent() == BB)
              noloop &= checkNoLoop(Iref,Iorig,BB);  
        }
        else if(Up->getParent() == Isearch->getParent())
          noloop &= checkNoLoop(Iref,Up,BB);
      }
      if(rawDeps->count(Isearch)){
        for(auto I = (*rawDeps)[Isearch]->begin(); I != (*rawDeps)[Isearch]->end(); ++I)
            noloop &= checkNoLoop(Iref,*I,BB);
      }
    }

  return noloop;
}

void FusedBB::mergeBB(BasicBlock *BB){
  this->BB->setName(BB->getName()+this->BB->getName());
  IRBuilder<> builder(*Context);
  Value *SelVal = new Argument(builder.getInt1Ty(),"fuse.sel.arg");
  (*argsBB)[SelVal] = BB;
  builder.SetInsertPoint(this->BB);
  memRAWDepAnalysis(BB);
  
  set<Instruction*> merged;
  map<Value*,Value*> SubOp;

  // Stats
  SetVector<Value*> LiveInBB, LiveOutBB;
  liveInOut(*BB,&LiveInBB,&LiveOutBB);

  // Update Stats
  this->addMergedBB(BB);

  for(Instruction &Ib : *BB){
    if(!isa<IntrinsicInst>(Ib) and !isa<BranchInst>(Ib)){
      for(Instruction &Ia : *this->BB){
        if(!merged.count(&Ia) and areInstMergeable(Ia,Ib)
            and checkNoLoop(&Ib,&Ia,BB) 
            and getStoreDomain(&Ia,this->BB) == getStoreDomain(&Ib, BB)){
          merged.insert(&Ia);
          merged.insert(&Ib);
          this->mergeOp(&Ia,&Ib,&SubOp,&builder,SelVal); 
          this->annotateMerge(&Ib,&Ia,BB); 
          this->addSafety(&Ia);
          this->linkLiveOut(&Ib,&Ia,&LiveOutBB);
          SubOp[cast<Value>(&Ib)] = cast<Value>(&Ia);
          break;
        }
      }
      if(!merged.count(&Ib)){
        Instruction *newInstB = Ib.clone();
        merged.insert(newInstB);
        if(!fuseMap->count(newInstB))
          (*fuseMap)[newInstB] = new set<Instruction*>;
        (*fuseMap)[newInstB]->insert(&Ib);
        builder.Insert(newInstB);
        this->dropSafety(&Ib); 
        this->linkLiveOut(&Ib,newInstB,&LiveOutBB);
        SubOp[cast<Value>(&Ib)] = cast<Value>(newInstB);
      } 
    }
  }
  for(Instruction &Ic: *this->BB){
    for(int j =0;j<Ic.getNumOperands(); ++j){
      Value *Vc = Ic.getOperand(j);
      if(SubOp.count(Vc))
        Ic.setOperand(j,SubOp[Vc]);
    }
  }
  this->updateStoreDomain(&SubOp);
  this->updateRawDeps(&SubOp);
  // TODO: check if this can be made more efficient
  if (this->getNumMerges() > 1){
    /*for(auto &Ic: *(this->BB))
      Ic.dump();*/
    this->KahnSort();
    this->secureMem(SelVal,BB);
  }
  return;
}

// Linking Functions
/*
 * This function links LiveOut values, so later when recovering the values from
 * the offload function, we are able to known what is their position in
 * the liveout struct and what value are we substituting for
 *
 * @param *opA Value from the BB we want to merge (unmodified)
 * @param *opB Value from the new merged BB we want to bookeep (new BB)
 */
void FusedBB::linkLiveOut(Instruction *InstrOrig, Instruction *InstrFused,
    SetVector<Value*> *LiveOut){
  if(LiveOut->count(cast<Value>(InstrOrig))){
    if(this->LiveOut->count(InstrOrig)){
      //(*this->LiveOut)[InstrOrig]->insert(InstrFused);
    }
    else{
      //(*this->LiveOut)[InstrOrig] = new set<Instruction*>;
      //(*this->LiveOut)[InstrOrig]->insert(InstrFused);
      (*this->LiveOut)[InstrOrig] = InstrFused;
    }
  }
	return;
}

void FusedBB::memRAWDepAnalysis(BasicBlock *BB){
  StoreInst *St;
  LoadInst *Ld;
  map<Value*,Instruction*> Wdeps;
  map<Value*,int> StoreDomain;
  for (auto &I : *BB){
    // For each store, we record the address being used
    if(St = dyn_cast<StoreInst>(&I)){
      if(StoreDomain.count(St->getPointerOperand()))
        StoreDomain[St->getPointerOperand()]++;
      else
        StoreDomain[St->getPointerOperand()] = 1;
      Wdeps[St->getPointerOperand()] = &I;
      (*storeDomain)[&I] = StoreDomain[St->getPointerOperand()];
    }
    // If a Load has de same address as a store, we annotate the dependence
    if(Ld = dyn_cast<LoadInst>(&I)){
      if(Wdeps.count(Ld->getPointerOperand())){
        if(!rawDeps->count(Wdeps[Ld->getPointerOperand()]))
          (*rawDeps)[Wdeps[Ld->getPointerOperand()]] = new set<Instruction*>;
        (*rawDeps)[Wdeps[Ld->getPointerOperand()]]->insert(&I);
        
      }
    }
  }
}

void FusedBB::dropSafety(Instruction *I){
  if(safeMemI->count(I)){
    if((*safeMemI)[I] == 1)
      safeMemI->erase(I);
  }
}
void FusedBB::addSafety(Instruction *I){
  if(isa<LoadInst>(I) or isa<StoreInst>(I) and !safeMemI->count(I))
    (*safeMemI)[I] = 1;
}

/* Topological sort Kahn's algorithm */
void FusedBB::KahnSort(){
  list<Instruction*> S; // Nodes with no incoming edge
  list<Instruction*> L; // Sorted elements
  map<Instruction*,set<Instruction*> > EdgeList;

  for(auto &I : *BB){
    bool noincoming = true;
    for(auto U = I.op_begin();U != I.op_end(); U++){
      Instruction *IU = NULL;
      if((IU = dyn_cast<Instruction>(*U))){
         if(IU->getParent() == BB){
           noincoming = false;
         if(EdgeList.count(&I))
          EdgeList[&I].insert(IU);
         else{
          set<Instruction*> tmp;
          tmp.insert(IU);
          EdgeList[&I] = tmp;
          }
        }
      }
    }
    /*if(rawDeps->count(&I)){
      for(auto rD : *(*rawDeps)[&I])
        EdgeList[rD].insert(&I);
    }*/
    if(!EdgeList.count(&I))
      S.push_back(&I);
  }


  while(!S.empty()){
    Instruction *A = S.front();
    S.pop_front();
    L.push_back(A);
    for(auto M = A->user_begin();M != A->user_end();++M){
      Instruction *IU = cast<Instruction>(*M);
      if(IU->getParent() == BB){
        EdgeList[IU].erase(A);
        if(EdgeList[IU].empty())
          S.push_back(cast<Instruction>(IU));
      }
    }
    /*if(rawDeps->count(A)){
      for(auto I = (*rawDeps)[A]->begin(); I != (*rawDeps)[A]->end(); ++I){
        EdgeList[*I].erase(A);
        if(EdgeList[*I].empty())
          S.push_back(*I);
      }
    }*/
  }

  for(auto I = next(L.begin()); I != L.end(); ++I)
    (*I)->moveAfter(*prev(I));
} 





// Getter Functions
void FusedBB::updateRawDeps(map<Value*,Value*> *SubOp){
  set<Value*> delete_list;
  for(auto rD : *rawDeps){
    if(SubOp->count(rD.first)){
      if(!rawDeps->count(cast<Instruction>((*SubOp)[rD.first])))
        (*rawDeps)[cast<Instruction>((*SubOp)[rD.first])] = new set<Instruction*>;
      for(auto ld : *rD.second){
        if(SubOp->count(cast<Value>(ld))){
          (*rawDeps)[cast<Instruction>((*SubOp)[rD.first])]->insert(cast<Instruction>((*SubOp)[ld]));
        }
      }
      delete_list.insert(rD.first);
    }
  }
  for(auto rD : delete_list){
    rawDeps->erase(cast<Instruction>(rD));
  }
}

void FusedBB::updateStoreDomain(map<Value*,Value*> *SubOp){
  set<Value*> delete_list;
  for(auto StD : *storeDomain){
    if(SubOp->count(StD.first)){
      (*storeDomain)[cast<Instruction>((*SubOp)[StD.first])] = StD.second;
      delete_list.insert(StD.first);  
    }
  }
  for(auto StD : delete_list)
    storeDomain->erase(cast<Instruction>(StD));
}

// TODO: Sink on loads
unsigned FusedBB::getStoreDomain(Instruction *I, BasicBlock *BB){
  int domain = 1; // Default value with no stores>?
  if(storeDomain->count(I)){
    return (*storeDomain)[I];
  }
  for(auto user = I->user_begin(); user != I->user_end() and !domain; ++user){
    Instruction *Up = (Instruction*)*user;
    if(Up->getParent() == BB){
      domain = getStoreDomain(Up,BB);
    }
  }
  return domain;
}


unsigned FusedBB::getNumMerges(){
  return mergedBBs->size();
}

unsigned FusedBB::getMaxMerges(){
  unsigned max = 0;
  for(auto E : *countMerges){
    max = E.second > max ? E.second : max;
  }
  return max;
}

string FusedBB::getName(){
  return BB->getName().str();
}

BasicBlock* FusedBB::getBB(){
  return BB;
}
    
void FusedBB::annotateMerge(Instruction* Iorig, Instruction *Imerg, BasicBlock* BB){
  // What BB does this merge come from
  if(!annotMerges->count(Iorig))
    (*annotMerges)[Imerg] = new set<BasicBlock*>;
  (*annotMerges)[Imerg]->insert(BB);
  // How many merges does this Inst have?
  if(countMerges->count(Imerg)){
    (*countMerges)[Imerg]++;
  }
  else{
    (*countMerges)[Imerg] = 2;
  }
  // What is the original BB Instruction (for loop checking)
  if(!fuseMap->count(Imerg))
    (*fuseMap)[Imerg] = new set<Instruction*>;
  (*fuseMap)[Imerg]->insert(Iorig);

  return;
}


  /**
   * Substitute all given BBs for function calls and create wrappers to 
   * gather/scatter the data
   *
   * @param *F pointer to the function to be called
   * @param *bbList list of BBs to be substituted
   * @return true if success
   */
  bool FusedBB::insertCall(Function *F, vector<BasicBlock*> *bbList){
    static IRBuilder<> Builder(*Context);
    vector<Type*> inputTypes;
    //vector<GlobalVariable*> SafeInputs;
    vector<Instruction*> removeInst;

    // offloaded functions receive as input a pointer to input struct and
    // return a pointer to output struct
    Type *inputType = ((PointerType*)F->getArg(0)->getType())->getElementType();
    Type *outputType = ((PointerType*)F->getReturnType())->getElementType();

    // Restored Values must replace LiveIn values
    map<BasicBlock*,pair<Value*,Value*> > restoreMap;

    // Create Safe Globals for input vectors from calls that are missing
    // some of the LiveIn variables
    // TODO: Do not copy select variables
    // TODO: Create safe globals for pointers
    // TODO: This can now probably be removed, since we make safe potiner
    // access in the offloaded funciton
    // TODO: Remove, no longer needed, we protect loads in offload funciton
    //
    //for(auto &O : ((StructType*)inputType)->elements())
    //  SafeInputs.push_back(getSafePointer(O,F->getParent()));

    int i = 0; // Pointer to BB in bbList being processed
    // Record all instruction with metadata, so we can remove them later
    set<Instruction*> iMetadata;
    for(auto BB: *mergedBBs){
      SetVector<Value*> LiveIn, LiveOut;
      liveInOut(*BB, &LiveIn, &LiveOut);

      // TODO: Optimize if empty input/output
      Instruction *I = BB->getFirstNonPHIOrDbg();
      Builder.SetInsertPoint(I);
      Value *inStruct = Builder.CreateAlloca(inputType);

      // Send Input information
      //set<int> visited; // Mark those input positions that are filled
      vector<AllocaInst*> Allocas;
      vector<pair<Value*, Value*> > RestoreInst;
      for(auto inV : LiveIn){
        MDNode *N;
        int pos = -1;
        // Set input parameters in their position inside the 
        // input struct. Mark the position as filled. Record that there
        // is metadata to be removed
        // Each BasicBlock can have merged live in with different positions
        // if input is not recorded use default position
        if((*liveInPos)[inV]->count(BB))
          pos = (*((*liveInPos)[inV]))[BB];
        else
          pos = (*((*liveInPos)[inV]))[NULL];

        if(pos >= 0){
          Value *inGEPData = Builder.CreateStructGEP(inStruct,pos);
          Builder.CreateStore(inV,inGEPData);
        }
      }
        
        // Send function selection
        // TODO: This has to be optimized. Now we search in the function for
        // the select instructions to find it's position and to which BB they
        // belong to
      for(auto Varg : *argsBB){
        if(liveInPos->count(Varg.first)){
          int pos = -1;
          pos = (*(*liveInPos)[Varg.first])[NULL];
          BasicBlock *selBB = Varg.second;
          assert(pos>=0 && "Missing positional information for select arg");
      		Value *inGEPData = Builder.CreateStructGEP(inStruct,pos);
      	  Builder.CreateStore(Builder.getInt1(selBB==BB),inGEPData);
          
        }
      }
			Value *outStruct = Builder.CreateCall(F,ArrayRef<Value*>(inStruct));
      
      // Removing the offloaded instructions
      // Avoid removing the terminator
      auto rI = BB->rbegin();
      rI++;
      while(&(*rI) != (Instruction*)outStruct){ 
        removeInst.push_back(&(*rI));
        rI++;
      }
      // TODO: find a more efficient way of 
      // Removing metadata attached to debug instructions

      // If we have liveOut values we must now fill them with the function
      // computed ones and discard the instructions from this offloaded BB
      vector<AllocaInst*> oAllocas;
      Value *lastLoad;
      for(int arg = 0; arg < LiveOut.size(); ++arg){
        Value *Vout = LiveOut[arg];
        Value *fusedV = (*this->LiveOut)[cast<Instruction>(Vout)];
        int pos = (*liveOutPos)[fusedV];
        Value *outGEPData = Builder.CreateStructGEP(outStruct,pos);
        Value *out = Builder.CreateLoad(LiveOut[arg]->getType(),outGEPData);
        LiveOut[arg]->replaceAllUsesWith(out);
        //lastLoad = out;
      }

      BB->setName(BB->getName());
		++i;
	}

  for(auto I : removeInst)
    I->eraseFromParent();

}

/**
 * This function gathers and computes several staistics and metrics for a 
 * given BasicBlock.
 *
 * @param *BB BasicBlock to analyze and attach metadata
 * @param *data Vector to be filled with the statitstics value
 * @param *M LLVM Module
 */
/*
void FusedBB::getMetrics(vector<float> *data, Module *M){
  MDNode *N;
	DataLayout DL  = M->getDataLayout();
  float orig_inst = 0;
  float footprint = 0;
  float area = 0;
  float powersta = 0;
  float powerdyn = 0;
  float max_merges = 0;
  float num_merges = 0;
  float num_sel = 0;
  float mem_time = 0;
  float time = 0;
  float num_ld = 0, num_st = 0;
  float power = 0;
  pair<float,float> crit_path = getCriticalPathCost(BB);
  for(auto I = BB->begin(); I != BB->end() ; ++I){
    time += getDelay(&*I);
    if (isa<StoreInst>(I) or isa<LoadInst>(I)){
      if(isa<StoreInst>(I)){
        footprint += DL.getTypeSizeInBits(I->getOperand(0)->getType());
        num_st++;
      }
      else{
        footprint += DL.getTypeSizeInBits(I->getType());  
        num_ld++;
      }
    }
    else if (isa<SelectInst>(I)){
      num_sel++;
    }
    if(countMerges->count(&*I)){
      int tmp = (*countMerges)[&*I];
      max_merges = tmp>max_merges?tmp:max_merges;
      num_merges++;
      orig_inst += tmp;
    }
    orig_inst += 1;
    area += getArea(&*I); 
    powersta += getPowerSta(&*I);
    powersta = 0;
    powerdyn += getEnergyDyn(&*I);
  }
  power = powersta+powerdyn;
  mem_time = 10*num_ld+3*num_st;
  data->push_back(orig_inst-num_sel);  // Orig Inst
  data->push_back(max_merges); // Maximum number of merges
  data->push_back(num_merges); // Number of merges
  data->push_back(BB->size()); // Num Inst
  data->push_back(num_ld);     // Num Loads
  data->push_back(num_st);     // Num Stores
  data->push_back(num_sel);    // Select Overhead
  data->push_back(BB->size()-num_ld-num_st-num_sel); // Other Instructions
  data->push_back(time);       // Sequential Time Estimate
  data->push_back(footprint);  // Memory Footprint in bits
  data->push_back(crit_path.first);  // Critical Path estimate in Time units
  data->push_back(crit_path.second);  // Critical Path estimate in Time units
  data->push_back(area);       // Area Estimate
  data->push_back(powersta);     // Energy Estimate
  data->push_back(powerdyn);     // Energy Estimate
  float bw = (footprint/float(8))/mem_time; // Bandwidth 
  float ci = (BB->size()-num_sel)/(footprint/float(8));  // Compute 
  float useful = orig_inst-num_sel;
  float cp = useful/(crit_path.first+crit_path.second); 
  data->push_back(cp/(area*power)/1e9); // Efficiency Gi/smm2W
  float eff_comp = useful/(crit_path.second*area*power)/1e9;
  //data->push_back(eff_comp);
  return;
}*/



  /**
   * Creates an offload function from a given BB
   *
   * @param BB Basic Block reference
   * @return Pointer to a function that executes BB
   */
  //TODO: Optimize load/stores in case no LiveIn/LiveOut
  Function* FusedBB::createOffload(Module *Mod){
    IRBuilder<> Builder(*Context);
    vector<Type *> inputTypes, outputTypes;
    SetVector<Value*> LiveIn, LiveOut;
    liveInOut(*BB, &LiveIn, &LiveOut);
    
    // Once we merged the BBs, we must annotate liveIn values from different
    // locations with their respective locations in the input Struct
    // so that once we call the offload function, we know what fields must 
    // be filled and which ones should be filled with padding and safe inputs
    // This function adds metadata to the LiveIn values from different BBs
    // Same for Live Outs
    //linkPositionalLiveInOut(&BB);
    
    for(auto &I : *BB)
      I.dump();

    int pos = 0;
    for(auto Vin: LiveIn){
      if(isa<Instruction>(Vin)){
        if(linkOps->count(Vin)){
          for(auto Vorig: *(*linkOps)[Vin]){
            if(!liveInPos->count(Vorig.first))
              (*liveInPos)[Vorig.first] = new map<BasicBlock*,int>;
            (*(*liveInPos)[Vorig.first])[Vorig.second] = pos;
          }
        }
        if(!liveInPos->count(Vin))
          (*liveInPos)[Vin] = new map<BasicBlock*,int>;
        (*(*liveInPos)[Vin])[NULL] = pos;
      }
      else if (isa<Argument>(Vin)){
        if(!liveInPos->count(Vin))
          (*liveInPos)[Vin] = new map<BasicBlock*,int>;
        (*(*liveInPos)[Vin])[NULL] = pos;
      }
      pos++;
    }

    pos = 0;
    for(auto Vout : LiveOut){
      (*liveOutPos)[Vout] = pos;
      pos++;
    }


    // Instantate all LiveIn values as Inputs types
    for(Value *V: LiveIn){
      inputTypes.push_back(V->getType());
    }
    // Instantate all LiveOut values as Output types
    for(Value *V: LiveOut){
      outputTypes.push_back(V->getType());
    }

    // Input/Output structs
    StructType *inStruct = StructType::get(*Context,inputTypes);
    StructType *outStruct = StructType::get(*Context,outputTypes);


    // Instantiate function
    FunctionType *funcType = FunctionType::get(outStruct->getPointerTo(),
        ArrayRef<Type*>(inStruct->getPointerTo()),false);
    Function *f = Function::Create(funcType,
        Function::ExternalLinkage,"offload_func",Mod);

    // Insert Fused Basic Block
    BB->insertInto(f);

    // Load Data (Creating New BB for readibility)
    BasicBlock* loadBB = BasicBlock::Create(*Context,"loadBB",f,BB);
    Builder.SetInsertPoint(loadBB);
    // DEBUG PURPOSES
    //Function *dbgt = Intrinsic::getDeclaration(Mod,Intrinsic::debugtrap);
    vector<Value*> inData;
    for(int i=0;i<LiveIn.size();++i){
      Value *inGEPData = Builder.CreateStructGEP(f->getArg(0),i);
      inData.push_back(Builder.CreateLoad(inGEPData));
    }
    // DEBUG PURPOSES
    //Builder.CreateCall(dbgt->getFunctionType(),cast<Value>(dbgt));  
    Builder.CreateBr(BB);

    // TODO: Make this more efficient
    for(int i=0; i<LiveIn.size();++i){
      for(auto &I: *BB){
        for(int j=0;j<I.getNumOperands();++j){
          if(I.getOperand(j) == LiveIn[i]){
            I.setOperand(j,inData[i]);
          }
        }
      }
      // Preserve names for arguments 
      inData[i]->setName(LiveIn[i]->getName());
    }

    /*
    for(auto &I: BB){
      Builder.SetInsertPoint(&I);
      if(isa<StoreInst>(I))
        Builder.CreateCall(dbgt->getFunctionType(),cast<Value>(dbgt));  
    }*/

    // Store Data (Creating New BB for readibility)
      BasicBlock* storeBB = BasicBlock::Create(*Context,"storeBB",f);
      Builder.SetInsertPoint(storeBB);
      //Builder.CreateCall(dbgt->getFunctionType(),cast<Value>(dbgt));  
      Value *outData = Builder.CreateAlloca(outStruct);
      for(int i=0;i<LiveOut.size();++i){
        Value *outGEPData = Builder.CreateStructGEP(outData,i);
        Builder.CreateStore(LiveOut[i],outGEPData);
      }
      Builder.CreateRet(outData);
  
      // Merged block jumps to store output data
      Builder.SetInsertPoint(BB);
      Builder.CreateBr(storeBB);

    // Check Consistency
    verifyFunction(*f,&errs());
    verifyModule(*Mod,&errs());

    f->dump();
    //Mod->dump();

    // Output Result
    /* Disable this for now as it only generates problems and this can be
     * safely done with llvm-extract
    Module *NewMod = new Module("offload_mod",Context);
    verifyModule(*NewMod,&errs());
    ValueToValueMapTy Vmap;
    Function *Newf = CloneFunction(f,Vmap);
    for (inst_iterator I = inst_begin(Newf), E = inst_end(Newf); I != E; ++I){
        if(isa<CallInst>(*I)){
          ValueToValueMapTy Vmap;
          Function *tmp = cast<CallInst>(*I).getCalledFunction();
          Function *fc = Function::Create(tmp->getFunctionType(),
              Function::ExternalLinkage,tmp->getName(),NewMod);
          cast<CallInst>(*I).setCalledFunction(fc);
        }
    }
    Newf->removeFromParent();
    NewMod->getFunctionList().push_back(Newf);
    // Removing all unknown metadata as it cannot be written in the bitcode
    // TODO: Check if MDNode can actually be printed as bitcode
    // Right now maybe we are storing Metadata in a wrong way hence llvm 
    // complains about not being able to print the bitcode
    for (inst_iterator I = inst_begin(Newf), E = inst_end(Newf); I != E; ++I)
      if(I->hasMetadataOtherThanDebugLoc())
         I->dropUnknownNonDebugMetadata();
    verifyModule(*NewMod,&errs());
    error_code EC;
    raw_fd_ostream of("offload.bc", EC,sys::fs::F_None);
    verifyModule(*NewMod,&errs());
    WriteBitcodeToFile(*NewMod,of);
    of.flush();
    of.close();
    delete NewMod;
    */

    return f;
  }

bool FusedBB::isMergedBB(BasicBlock *BB){
  return mergedBBs->count(BB);
}

bool FusedBB::isMergedI(Instruction *I){
  return countMerges->count(I);
}

int FusedBB::size(){
  return mergedBBs->size();
}