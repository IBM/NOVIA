#include "FusedBB.hpp"


FusedBB::~FusedBB(){
  delete mergedBBs;
  delete LiveOut;
  for(auto E : *linkOps)
    delete E.second;
  delete linkOps;
  delete argsBB;
  delete annotInst;
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
  delete LiveIn;
  delete liveOutPos;
  delete BB;
  return;
}

FusedBB::FusedBB(FusedBB* Copy, list<Instruction*> *subgraph){
  init(Copy->Context);
  
  ValueToValueMapTy VMap;
  
  this->BB = BasicBlock::Create(*Copy->Context,"");  
  for (Instruction &I : *Copy->getBB()){
    if(find(subgraph->begin(),subgraph->end(),&I) != subgraph->end()){
      Instruction *NewInst = I.clone();
      NewInst->setName(I.getName());
      BB->getInstList().push_back(NewInst);
      VMap[&I] = NewInst;  


      if(Copy->fuseMap->count(&I))
        for(auto E : *(*Copy->fuseMap)[&I])
          if(!mergedBBs->count(E->getParent()))
            mergedBBs->insert(E->getParent());
      
      //if(VMap[E.first])
      if(Copy->linkOps->count(&I))
        (*linkOps)[NewInst] = new set<pair<Value*,BasicBlock*> >(*(*(Copy->linkOps))[&I]);
      //else
      //  (*linkOps)[E.first] = new set<pair<Value*,BasicBlock*> >(*E.second);

      if(isa<SelectInst>(&I))
        if(!argsBB->count(I.getOperand(0)))
          (*argsBB)[I.getOperand(0)] = (*Copy->argsBB)[I.getOperand(0)];

      if(Copy->fuseMap->count(&I))
        for(auto Iorig : *(*Copy->fuseMap)[&I])
          annotInst->insert(Iorig);
      
      if(Copy->isMergedI(&I))
        (*countMerges)[NewInst] = (*(Copy->countMerges))[&I];

      if(Copy->safeMemI->count(&I))
        (*safeMemI)[NewInst] = (*(Copy->safeMemI))[&I];

      if(Copy->selMap->count(&I))
        (*selMap)[NewInst] = new set<BasicBlock*>(*(*(Copy->selMap))[&I]);

    }
  }
  
  for(auto E: *Copy->LiveIn)
    LiveIn->insert(E);

  for(auto E : *Copy->LiveOut){
    if(VMap.count(E.second))
      (*LiveOut)[E.first] = cast<Instruction>(VMap[cast<Value>(E.second)]);
  }

  SmallVector<BasicBlock*,1> bbList;
  bbList.push_back(this->BB);
  remapInstructionsInBlocks(bbList,VMap);
  this->BB->setName(Copy->getName());
}

FusedBB::FusedBB(FusedBB* Copy){
  ValueToValueMapTy VMap;
  // There is a bug, if a parentless basicblock (without funciton) with allocas 
  // is Cloned, the isStaticAlloca() check will fail, cause it will try to
  // retrieve the parent of the basic block, which is empty and get the first
  // basic block of a null operator
  //BB = CloneBasicBlock(Copy->getBB(),VMap);
  this->Context = Copy->Context;
  
  this->BB = BasicBlock::Create(*Copy->Context,"");  
  this->BB->setName(Copy->getName());
  for (const Instruction &I : *Copy->getBB()){
    Instruction *NewInst = I.clone();
    NewInst->setName(I.getName());
    BB->getInstList().push_back(NewInst);
    VMap[&I] = NewInst;
  }
  
  SmallVector<BasicBlock*,1> bbList;
  bbList.push_back(this->BB);
  remapInstructionsInBlocks(bbList,VMap);
  
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

  annotInst = new set<Instruction*>;
  for(auto E : *Copy->annotInst)
    annotInst->insert(E);

  countMerges = new map<Instruction*,int>;
  for(auto E : *Copy->countMerges)
    (*countMerges)[cast<Instruction>(VMap[cast<Value>(E.first)])] = E.second;

  storeDomain = new map<Instruction*,int>;
  for(auto E : *Copy->storeDomain)
    (*storeDomain)[cast<Instruction>(VMap[cast<Value>(E.first)])] = E.second;


  rawDeps = new map<Instruction*,set<Instruction*>* >;
  for(auto E : *Copy->rawDeps){
    set<Instruction*> *aux = new set<Instruction*>;
    if(VMap.count(cast<Value>(E.first))){
      (*rawDeps)[cast<Instruction>(VMap[cast<Value>(E.first)])] = aux;
      for(auto sE : *E.second)
        if(VMap.count(cast<Value>(sE)))
          aux->insert(cast<Instruction>(VMap[cast<Value>(sE)]));  
    }
  }

  fuseMap = new map<Instruction*,set<Instruction*>* >;
  for(auto E : *Copy->fuseMap)
    fuseMap->insert(pair<Instruction*,set<Instruction*>* >(cast<Instruction>(VMap[cast<Value>(E.first)]),new set<Instruction*>(*E.second)));

  safeMemI = new map<Instruction*,unsigned>;
  for(auto E : *Copy->safeMemI)
    (*safeMemI)[cast<Instruction>(VMap[cast<Value>(E.first)])] = E.second;

  selMap = new map<Instruction*,set<BasicBlock*>*>;
  for(auto E : *Copy->selMap)
    (*selMap)[cast<Instruction>(VMap[cast<Value>(E.first)])] = 
      new set<BasicBlock*> (*E.second);

  liveInPos = new map<Value*,map<BasicBlock*,int>* >;
  for(auto E : *Copy->liveInPos)
    if(VMap[E.first])
      (*liveInPos)[VMap[E.first]] = new map<BasicBlock*,int> (*E.second);
    else
      (*liveInPos)[E.first] = new map<BasicBlock*,int> (*E.second);

  LiveIn = new set<Value*>;
  for(auto E: *Copy->LiveIn)
    LiveIn->insert(E);

  liveOutPos = new map<Value*,int>;
  for(auto E : *Copy->liveOutPos)
    (*liveOutPos)[VMap[E.first]] = E.second;
}

void FusedBB::init(LLVMContext *Context){
  mergedBBs = new set<BasicBlock*>; 
  LiveOut = new map<Instruction*, Instruction* >;
  linkOps = new map<Value*,set<pair<Value*,BasicBlock*> >* >;
  argsBB = new map<Value*,BasicBlock*>;
  annotInst = new set<Instruction*>;
  countMerges = new map<Instruction*,int>;
  storeDomain = new map<Instruction*,int>;
  rawDeps = new map<Instruction*,set<Instruction*>* >;
  fuseMap = new map<Instruction*,set<Instruction*>* >;
  safeMemI = new map<Instruction*,unsigned>;
  selMap = new map<Instruction*,set<BasicBlock*>*>;
  liveInPos = new map<Value*,map<BasicBlock*,int>* >;
  LiveIn = new set<Value*>;
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

bool FusedBB::checkSelects(Value *Vorig, Value *Vfused,BasicBlock *BB,
    map<Value*,Value*> *SubOp, set<Value*> *LiveInBB){
  bool found = false;
  if(Vfused->getName() == "fuse.sel"){
    Instruction *If = cast<Instruction>(Vfused);
    if(If->getOperand(1)->getName() == "fuse.sel"){
      found = checkSelects(Vorig,If->getOperand(1),BB,SubOp,LiveInBB);
    }
    else if(areOpsMergeable(If->getOperand(1),Vorig,this->BB,BB,SubOp,
          this->LiveIn,LiveInBB)){
      Value *SelVal = new Argument(Type::getInt1Ty(*this->Context),
          "fuse.sel.arg");
      If->setOperand(0,SelVal);
      (*selMap)[If]->insert(BB);
      found = true;
    }
    else if(If->getOperand(2)->getName() == "fuse.sel"){
      found = checkSelects(Vorig,If->getOperand(2),BB,SubOp,LiveInBB);
    }
    else if(areOpsMergeable(If->getOperand(2),Vorig,this->BB,BB,SubOp,
          this->LiveIn,LiveInBB)){
      found = true;
    }
  }

  return false;
}

void FusedBB::mergeOp(Instruction *Ifused, Instruction *Iorig,
    map<Value*,Value*> *SubOp, IRBuilder<> *Builder,
    Value *SelVal, set<Value*> *LiveInBB){
  map<int,int> match;
  set<int> used;
  for(int i = 0;i < Ifused->getNumOperands(); ++i){
    Value *Vfused = Ifused->getOperand(i);
    for(int j = 0; j < Iorig->getNumOperands() and !match.count(i); ++j){
      Value *Vorig = Iorig->getOperand(j);
      if(Ifused->isCommutative() or !Ifused->isCommutative() and i == j){
        if(!used.count(j) and 
           (areOpsMergeable(Vfused,Vorig,Ifused->getParent(),
                             Iorig->getParent(),SubOp,this->LiveIn,LiveInBB) or 
            checkSelects(Vorig,Vfused,Iorig->getParent(),SubOp,LiveInBB))){
          match[i] = j;
          used.insert(j);
        }
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
      (*linkOps)[Vfused]->insert(pair<Value*,BasicBlock*>(Vorig,
            Iorig->getParent())); 
    } 
    else{
      for(int j = 0; j < Iorig->getNumOperands() and !SelI; ++j){
        if(!used.count(j)){
          Value *Vorig = Iorig->getOperand(j);
          assert( Vfused->getType() == Vorig->getType() &&
              " Types differ, wrong merge" );
          SelI = Builder->CreateSelect(SelVal,Vorig,Vfused,"fuse.sel");
          (*selMap)[cast<Instruction>(SelI)] = new set<BasicBlock*>;
          (*selMap)[cast<Instruction>(SelI)]->insert(Iorig->getParent());
          Ifused->setOperand(i,SelI);
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
        (*selMap)[cast<Instruction>(SelSafe)] = new set<BasicBlock*>;
        (*selMap)[cast<Instruction>(SelSafe)]->insert(destBB);
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
bool FusedBB::searchDfs(Instruction *Iref,Instruction* Isearch, 
    set<Instruction*> *visited){
  bool noloop = true;
  if(Iref == Isearch)
    return false;
  else if(!visited->count(Isearch)){
    for(auto user = Isearch->user_begin(); user!= Isearch->user_end() and noloop; user++){
      Instruction *Up = (Instruction*)*user;
      if(Up->getParent() == Iref->getParent())
        noloop &= searchDfs(Iref,Up,visited);
    }
    if(rawDeps->count(Isearch)){
      for(auto I = (*rawDeps)[Isearch]->begin(); I != (*rawDeps)[Isearch]->end() and noloop; ++I)
        noloop &= searchDfs(Iref,*I,visited);

    }
    visited->insert(Isearch);
  }
  return noloop;
}
bool FusedBB::checkNoLoop2(Instruction *Iref, Instruction *Isearch, BasicBlock *BB,
    map<Value*,Value*> *SubOp){
  bool noloop = true;
  set<Instruction*> visited;
  // Consumers
  for(auto user = Iref->user_begin(); user!= Iref->user_end() and noloop; user++){
    Value *Up = (Value*)*user;
    if(SubOp->count(Up)){
      visited.clear();
      noloop &= searchDfs(Isearch,(Instruction*)(*SubOp)[Up],&visited);
    }
  }
  // Producers
  for(int i = 0; i < Iref->getNumOperands() and noloop ; ++i){
    if(SubOp->count(Iref->getOperand(i))){
      visited.clear();
      noloop &= searchDfs((Instruction*)(*SubOp)[Iref->getOperand(i)],Isearch,
          &visited);
    }
  }
  // Memory Consumers
  if(rawDeps->count(Iref)){
    for(auto I = (*rawDeps)[Iref]->begin(); I != (*rawDeps)[Iref]->end() and noloop; ++I)
      if(SubOp->count((Value*)&*I)){
        visited.clear();
        noloop &= searchDfs(Isearch,(Instruction*)(*SubOp)[(Value*)&*I],
            &visited);
        
      }
  }
  // Memory Producers
  if(noloop)
    for(auto Elem : (*rawDeps))
      for(auto Ild : *Elem.second)
        if(Ild == Iref)
          if(SubOp->count((Value*)Elem.first)){
            visited.clear();
            noloop &= searchDfs((Instruction*)(*SubOp)[(Value*)Elem.first],
                Isearch,&visited);
          }
  
  return noloop;
}


static set<Instruction*> repeated;
static int dbgcount;
bool FusedBB::checkNoLoop(Instruction *Iref, Instruction *Isearch, BasicBlock *BB){
  bool noloop = true;
  if(repeated.count(Isearch)){
    dbgcount += 1;
    errs() << "Error";
  }
  else{
    if(repeated.empty())
      errs() << "Start\n";
    repeated.insert(Isearch);
    errs() << Isearch << " " << *Isearch << '\n';
  }
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
        if(Up->getParent() == Isearch->getParent())
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
  this->BB->setName(this->BB->getName()+BB->getName());
  IRBuilder<> builder(*Context);
  Value *SelVal = new Argument(builder.getInt1Ty(),"fuse.sel.arg");
  (*argsBB)[SelVal] = BB;
  builder.SetInsertPoint(this->BB);
  memRAWDepAnalysis(BB);
  
  set<Instruction*> merged;
  map<Value*,Value*> SubOp;

  // Stats
  SetVector<Value*> SetVLiveInBB, LiveOutBB;
  liveInOut(*BB,&SetVLiveInBB,&LiveOutBB);
  set<Value*> LiveInBB (SetVLiveInBB.begin(),SetVLiveInBB.end());


  // Update Stats
  this->addMergedBB(BB);

  for(Instruction &Ib : *BB){
    bool bmerged = false;
    if(!isa<BranchInst>(Ib)){
      for(Instruction &Ia : *this->BB){
        //dbgcount = 0;
        if(!merged.count(&Ia) and areInstMergeable(Ia,Ib)
            and checkNoLoop2(&Ib,&Ia,BB,&SubOp)){ 
            //and getStoreDomain(&Ia,this->BB) == getStoreDomain(&Ib, BB)){
          //repeated.clear();
          //errs() << "merged\n";
          merged.insert(&Ia);
          bmerged = true;
          //this->CycleDetector(BB->getParent());
          //merged.insert(&Ib);
          this->mergeOp(&Ia,&Ib,&SubOp,&builder,SelVal,&LiveInBB); 
          this->annotateMerge(&Ib,&Ia,BB); 
          this->addSafety(&Ia);
          this->linkLiveOut(&Ib,&Ia,&LiveOutBB);
          SubOp[cast<Value>(&Ib)] = cast<Value>(&Ia);
          break;
        }
      }
      if(!bmerged){
        Instruction *newInstB = Ib.clone();
        newInstB->setName(Ib.getName());
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
    // TODO: This needs to be optimized
    // We do not need to search all instruction to substitute the operands
    // just the ones that have left out operands to substitute
    // TODO: Apply exceptions to livein liveout that are from created select
    // instructions!!!!!
    for(Instruction &Ic: *this->BB){
        for(int j =0;j<Ic.getNumOperands(); ++j){
          Value *Vc = Ic.getOperand(j);
          if(SubOp.count(Vc) and find(LiveInBB.begin(),LiveInBB.end(),Vc) == LiveInBB.end()
            and find(this->LiveIn->begin(),this->LiveIn->end(),Vc) == this->LiveIn->end()
            and (find(LiveOutBB.begin(),LiveOutBB.end(),Vc) == LiveOutBB.end() or merged.count(&Ic)) ){
            Ic.setOperand(j,SubOp[Vc]);
          }
        }
    }
      
    bool jiji = false;
          if(jiji){
            FunctionType *funcType = FunctionType::get(builder.getVoidTy(),false);
            Function *f = Function::Create(funcType,
                Function::ExternalLinkage,"offload_func",BB->getModule());
            this->BB->insertInto(f);
			      Instruction *V = builder.CreateCall(f);
            V->moveBefore(BB->getTerminator());
            DominatorTree DT = DominatorTree(*f);
            this->KahnSort();
            verifyFunction(*f);
            for(auto &I: *this->BB){
              for(auto *U : I.users()){
                if(DT.dominates((Instruction*)U,&I)){
                  errs() << "Alert!\n";
                }
              }
            }
          }
          
    this->updateRawDeps(&SubOp);
  }
  /*for(Instruction &Ic: *this->BB){
    for(int j =0;j<Ic.getNumOperands(); ++j){
      Value *Vc = Ic.getOperand(j);
      if(SubOp.count(Vc) and find(LiveInBB.begin(),LiveInBB.end(),Vc) == LiveInBB.end() and find(LiveOutBB.begin(),LiveOutBB.end(),Vc) == LiveOutBB.end())
        Ic.setOperand(j,SubOp[Vc]);
    }
  }*/
  for(auto inV : LiveInBB)
    this->LiveIn->insert(inV);

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
  /*for(auto elem : (*rawDeps))
    for(auto Ild : elem.second){
      if(!warDeps->count(Ild))
        warDeps[Ild] = new set<Instruction*>;
      warDeps[Ild]->insert(elem.first);
    }*/
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
    if(rawDeps->count(&I)){
      for(auto rD : *(*rawDeps)[&I])
        EdgeList[rD].insert(&I);
    }
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
    if(rawDeps->count(A)){
      for(auto I = (*rawDeps)[A]->begin(); I != (*rawDeps)[A]->end(); ++I){
        EdgeList[*I].erase(A);
        if(EdgeList[*I].empty())
          S.push_back(*I);
      }
    }
  }

  for(auto I = next(L.begin()); I != L.end(); ++I)
    (*I)->moveAfter(*prev(I));
}





// Getter Functions
void FusedBB::updateRawDeps(map<Value*,Value*> *SubOp){
  set<Value*> delete_list;
  for(auto rD : *rawDeps){
    Value *St = NULL;
    if(SubOp->count(rD.first)){
      St = (*SubOp)[rD.first];
      if(!rawDeps->count(cast<Instruction>(St)))
        (*rawDeps)[cast<Instruction>(St)] = new set<Instruction*>;
      //delete_list.insert(rD.first);
    }
    else
      St = rD.first;
    for(auto ld : *rD.second){
      if(SubOp->count(cast<Value>(ld))){
        (*rawDeps)[cast<Instruction>(St)]->insert(cast<Instruction>((*SubOp)[ld]));
        //(*rawDeps)[cast<Instruction>(St)]->erase(ld);
      }
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

float FusedBB::getAreaOverhead(){
  float areaOverhead = 0;
  for(auto &I : *this->BB){
    if(isa<SelectInst>(I))
      if(I.getOperand(0)->getName() == "fuse.sel.arg") 
        areaOverhead += getArea(&I); 
  }
  return areaOverhead;
}

unsigned FusedBB::getNumMerges(Instruction *I){
  unsigned merges = 0;
  if(countMerges->count(I))
    merges = (*countMerges)[I];
  return merges;
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
/*
bool FusedBB::getVerilog(raw_fd_stream &out){
  SetVector<Value*> LiveIn, LiveOut;
  liveInOut(this->BB, &LiveIn, &LiveOut);
  out() << "module " << this->BB->getName() << " (\n";
  for (auto inV : LiveIn){
    out() "input wire [" << inV->getIntegerBitWidth()-1 << ":0] " << inV->getName() << ",\n";
  }
  for (auto outV : LiveOut){
    out() "output wire [" << outV->getIntegerBitWidth()-1 << ":0] " << outV->getName() << ",\n";
  }
  out() << ");\n\n";

  map<Instruction*,string> wireNames;
  int count = 0;
  for(auto I : *this->BB){
    out() << "wire [" << I->getType()->getIntegerBitWidth()-1 << ":0] "


  }


  out() << "endmodule";
  return true;
}*/
    
void FusedBB::annotateMerge(Instruction* Iorig, Instruction *Imerg, BasicBlock* BB){
  // What BB does this merge come from
  if(!annotInst->count(Iorig))
    annotInst->insert(Iorig);
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

  // Only for debug purposes
  MDNode *N = MDNode::get(Iorig->getContext(),NULL);
  Imerg->setMetadata("is.merged",N);
  //Imerg->dump();

  return;
}

bool FusedBB::insertInlineCall(Function *F){
  static IRBuilder<> Builder(*Context);
  vector<Type*> inputTypes;
  vector<Instruction*> removeInst;

  //Type *inputType = ((PointerType*)F->getArg(0)->getType())->getElementType();
  //Type *outputType = ((PointerType*)F->getReturnType())->getElementType();

  for(auto BB : *mergedBBs){
      Instruction *I = BB->getFirstNonPHIOrDbg();
      Builder.SetInsertPoint(I);

      // Function Arguments
      SmallVector<Value*,20> Args(this->liveInPos->size(),NULL);

      // Live in Values
      set<int> filled;
      for(int i = 0; i < this->liveInPos->size(); ++i)
        filled.insert(i);

      for(auto E : *this->liveInPos){
        Value *inV = E.first;
        BasicBlock *targetBB = NULL;
        for(auto U : E.first->users())
          targetBB = cast<Instruction>(U)->getParent() == BB? cast<Instruction>(U)->getParent() : NULL;
        if(targetBB){
          int pos = (*E.second)[targetBB];
          Args[pos] = inV;
          filled.erase(pos);
        }
      }
        
      // Function Selection arguments 
      for(auto Varg : *argsBB){
        int pos = (*(*liveInPos)[Varg.first])[NULL];
        BasicBlock *selBB = Varg.second;
        Args[pos] = Builder.getInt1(selBB==BB);
        filled.erase(pos);
      }

      for(auto pos : filled)
        Args[pos] = Constant::getNullValue(F->getArg(pos)->getType());

			
      Value *outStruct = Builder.CreateCall(F,Args);
      
      // Removing inlined instructions
      auto rI = BB->rbegin();
      // Avoid removing the terminator
      rI++;
      while(&(*rI) != (Instruction*)outStruct and annotInst->count(&*rI)){ 
        removeInst.push_back(&(*rI));
        rI++;
      }

      //LiveOuts
      for(auto E : *this->LiveOut){
        if(E.first->getParent() == BB){
          int pos = (*liveOutPos)[E.second];
          Value *outGEPData = Builder.CreateStructGEP(outStruct,pos);
          Value *out = Builder.CreateLoad(E.first->getType(),outGEPData);
          E.first->replaceAllUsesWith(out);
        
          // If the out value is the new livein of a merged bb we copy it's pos
          if(liveInPos->count(E.first))
            (*liveInPos)[out] = (*liveInPos)[E.first];
        }
      }

/*
      vector<AllocaInst*> oAllocas;
      for(int arg = 0; arg < LiveOut.size(); ++arg){
        Value *Vout = LiveOut[arg];
        Value *fusedV = (*this->LiveOut)[cast<Instruction>(Vout)];
        int pos = (*liveOutPos)[fusedV];
        Value *outGEPData = Builder.CreateStructGEP(outStruct,pos);
        Value *out = Builder.CreateLoad(LiveOut[arg]->getType(),outGEPData);
        LiveOut[arg]->replaceAllUsesWith(out);

        // If the out value is the new livein of a merged bb we copy it's pos
        if(liveInPos->count(LiveOut[arg])){
          (*liveInPos)[out] = (*liveInPos)[LiveOut[arg]] ;
        }
      }*/
	}

  for(auto I : removeInst)
    I->eraseFromParent();
  verifyModule(*F->getParent());

}

  /**
   * Substitute all given BBs for function calls and create wrappers to 
   * gather/scatter the data
   *
   * @param *F pointer to the function to be called
   * @return true if success
   */
  bool FusedBB::insertOffloadCall(Function *F){
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
      F->dump();
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

        // If the out value is the new livein of a merged bb we copy it's pos
        if(liveInPos->count(LiveOut[arg])){
          (*liveInPos)[out] = (*liveInPos)[LiveOut[arg]] ;
        }
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

void FusedBB::getMetrics(vector<double> *data, Module *M){
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
    time += getSwDelay(&*I);
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
}

/**
 * Creates an inline function from a given BB
 *
 * @param BB Basic Block reference
 * @return Pointer to a function that executes BB
 */
//TODO: Optimize load/stores in case no LiveIn/LiveOut
Function* FusedBB::createInline(Module *Mod){
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
  
  /*for(auto &I : *BB)
    I.dump();
  errs() << '\n';*/

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
      (*(*liveInPos)[Vin])[cast<Instruction>(Vin)->getParent()] = pos;
    }
    else if (isa<Argument>(Vin)){
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
    pos++;
  }

  for(auto Vout : *this->LiveOut)
    if(!liveOutPos->count(Vout.second))
      (*liveOutPos)[Vout.second] = -1;
      
  pos = 0;
  for(auto Vout : *this->liveOutPos)
    (*liveOutPos)[Vout.first] = pos++;


  // Instantate all LiveIn values as Inputs types
  for(auto V: LiveIn){
    inputTypes.push_back(V->getType());
  }
  // Instantate all LiveOut values as Output types
  for(auto V: *this->liveOutPos){
    outputTypes.push_back(V.first->getType());
  }

  // Input/Output structs
  //StructType *inStruct = StructType::get(*Context,inputTypes);
  StructType *outStruct = StructType::get(*Context,outputTypes);


  // Instantiate function
  FunctionType *funcType = FunctionType::get(outStruct->getPointerTo(),
      ArrayRef<Type*>(inputTypes),false);
  Function *f = Function::Create(funcType,
      Function::ExternalLinkage,"inline_func",Mod);

  // Insert Fused Basic Block
  BB->insertInto(f);

  // DEBUG PURPOSES
  //Builder.CreateCall(dbgt->getFunctionType(),cast<Value>(dbgt));  

  // TODO: Make this more efficient
  for(int i=0; i<LiveIn.size();++i){
    for(auto &I: *BB){
      for(int j=0;j<I.getNumOperands();++j){
        if(I.getOperand(j) == LiveIn[i]){
          I.setOperand(j,f->getArg(i));
        }
      }
    }
    // Preserve names for arguments 
    f->getArg(i)->setName(LiveIn[i]->getName());
  }

  // Store Data (Creating New BB for readibility)
    BasicBlock* storeBB = BasicBlock::Create(*Context,"storeBB",f);
    Builder.SetInsertPoint(storeBB);
    //Builder.CreateCall(dbgt->getFunctionType(),cast<Value>(dbgt));  
    Value *outData = Builder.CreateAlloca(outStruct);
    /*for(int i=0;i<LiveOut.size();++i){
      Value *outGEPData = Builder.CreateStructGEP(outData,i);
      Builder.CreateStore(LiveOut[i],outGEPData);
    }*/
    for(auto Vout : *this->liveOutPos){
      Value *outGEPData = Builder.CreateStructGEP(outData,Vout.second);
      Builder.CreateStore(Vout.first,outGEPData);
    }
    Builder.CreateRet(outData);

    // Merged block jumps to store output data
    Builder.SetInsertPoint(BB);
    Builder.CreateBr(storeBB);

  // Check Consistency
  verifyFunction(*f,&errs());

  // HLS and OpenCL Stuff
  MDNode *N = MDNode::get(f->getContext(),NULL);
  f->setMetadata("opencl.kernels",N);

  return f;
}


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
    
    /*for(auto &I : *BB)
      I.dump();
    errs() << '\n';*/

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
        (*(*liveInPos)[Vin])[cast<Instruction>(Vin)->getParent()] = pos;
      }
      else if (isa<Argument>(Vin)){
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
      pos++;
    }

    for(auto Vout : *this->LiveOut)
      if(!liveOutPos->count(Vout.second))
        (*liveOutPos)[Vout.second] = -1;
        
    pos = 0;
    for(auto Vout : *this->liveOutPos)
      (*liveOutPos)[Vout.first] = pos++;


    // Instantate all LiveIn values as Inputs types
    for(Value *V: LiveIn){
      inputTypes.push_back(V->getType());
    }
    // Instantate all LiveOut values as Output types
    for(auto V: *this->liveOutPos){
      outputTypes.push_back(V.first->getType());
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
      /*for(int i=0;i<LiveOut.size();++i){
        Value *outGEPData = Builder.CreateStructGEP(outData,i);
        Builder.CreateStore(LiveOut[i],outGEPData);
      }*/
      for(auto Vout : *this->liveOutPos){
        Value *outGEPData = Builder.CreateStructGEP(outData,Vout.second);
        Builder.CreateStore(Vout.first,outGEPData);
      }
      Builder.CreateRet(outData);
  
      // Merged block jumps to store output data
      Builder.SetInsertPoint(BB);
      Builder.CreateBr(storeBB);

    // Check Consistency
    verifyFunction(*f,&errs());

    //f->dump();
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
    // HLS and OpenCL Stuff
    MDNode *N = MDNode::get(f->getContext(),NULL);
    f->setMetadata("opencl.kernels",N);

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

void traverseExclude(Instruction *I, set<Instruction*> *visited,
    set<Instruction*> *excluded, list<Instruction*> *list){
  if(!excluded->count(I)){
    excluded->insert(I);
    auto it = find(list->begin(),list->end(),I);
    if(it != list->end()){
      list->erase(it);
      auto it2 = find(visited->begin(),visited->end(),I);
      visited->erase(it2);
    }
    //visited->insert(I);
    for(auto U : I->users()){
      if(cast<Instruction>(U)->getParent() == I->getParent())
        traverseExclude(cast<Instruction>(U),visited,excluded,list);
    }
  }
}


void FusedBB::traverseSubgraph(Instruction *I, set<Instruction*> *visited, 
    set<Instruction*> *excluded, list<Instruction*> *list, vector<vector<double>*> *prebb, vector<BasicBlock*> *bblist, float cp, map<BasicBlock*,double> *tseq){
  float cp_max = 1000000;
  for(auto Il : *list)
    if(fuseMap->count(I))
      for(auto Ip : *(*fuseMap)[I])
        if( tseq->count(Ip->getParent()) )
          if( (*tseq)[Ip->getParent()] < cp_max )
            cp_max =  (*tseq)[Ip->getParent()];

  // We remove here instructions on able to be handled by our substrate
  if(isa<LoadInst>(I) or isa<StoreInst>(I) or isa<CallInst>(I) or
      isa<InsertElementInst>(I) or isa<ExtractElementInst>(I) or
      isa<ShuffleVectorInst>(I) or isa<InsertValueInst>(I) or
      (visited->count(I) and find(list->begin(),list->end(),I) == list->end())){
    visited->insert(I);
    traverseExclude(I,visited,excluded,list);
  }
  else if(cp+getHwDelay(I) > cp_max){
    traverseExclude(I,visited,excluded,list);
  }
  else if(!excluded->count(I) and !visited->count(I)){
    visited->insert(I);
    list->push_back(I);
    for(auto U : I->users())
      if(cast<Instruction>(U)->getParent() == I->getParent())
        traverseSubgraph(cast<Instruction>(U),visited,excluded,list,prebb,
            bblist,cp+getHwDelay(I),tseq);
  }
  
  return;
}

void FusedBB::splitBB(vector<list<Instruction*> *> *subgraphs, vector<vector<double> *> *prebb, vector<BasicBlock*> *bblist){
  set<Instruction*> visited;
  set<Instruction*> excluded;
  map<BasicBlock*,double> tseq;
  for(int i=0;i<bblist->size();i++){
    tseq[(*bblist)[i]] = (*(*prebb)[i])[8];
  }

  while( visited.size() != this->BB->size() ){
    subgraphs->push_back(new list<Instruction*>);
    excluded.clear();
    for(auto &I : *this->BB){
      if(isa<SelectInst>(&I) and (I.getName() == "fuse.sel" or I.getName() == 
            "fuse.sel.safe") and !excluded.count(&I))
        visited.insert(&I);
      else if(!visited.count(&I))
        traverseSubgraph(&I,&visited,&excluded,(*subgraphs)[subgraphs->size()-1], prebb, bblist,0,&tseq);
    }

    // Compute Sequential Time of each BB involved
    for(auto I: *(*subgraphs)[subgraphs->size()-1]){
      if(fuseMap->count(I))
        for( auto Iorig : *(*fuseMap)[I] )
          tseq[Iorig->getParent()] -= getSwDelay(I);
    }
    
    // Remove Leading or Trailing Selects
    bool removed = true;
    while(removed){
      removed = false;
      for(auto I = (*subgraphs)[subgraphs->size()-1]->begin();
        I != (*subgraphs)[subgraphs->size()-1]->end();++I){
        if(isa<SelectInst>(*I)){
        bool nouse = true;
        for(int i=0;i<(*I)->getNumOperands();++i)
          for(auto O = (*subgraphs)[subgraphs->size()-1]->begin();
                   O != (*subgraphs)[subgraphs->size()-1]->end();++O)
            nouse &=  (*O) != (*I)->getOperand(i);
            if(nouse){
              (*subgraphs)[subgraphs->size()-1]->erase(I--);
              removed = true;
            }
            if(!nouse){
              nouse = true;
              for(auto U : (*I)->users())
                nouse &= find((*subgraphs)[subgraphs->size()-1]->begin(),
                      (*subgraphs)[subgraphs->size()-1]->end(),(Instruction*)U)
                    == (*subgraphs)[subgraphs->size()-1]->end();
                if(nouse){
                  (*subgraphs)[subgraphs->size()-1]->erase(I--);
                  removed = true;
                }  
            }
          }
        }
      }
    }
  return;
}


bool FusedBB::rCycleDetector(Instruction *I, set<Instruction*> *visited,
    list<Instruction*> *streami, set<Instruction*> *processed, int level){
  bool loopdet = false;
  errs() << level << "\n";
  if(visited->count(I)){
    streami->push_front(I);
    return true;
  }
  else{
    visited->insert(I);
    processed->insert(I);
    for(auto Us : I->users()){
      if(cast<Instruction>(Us)->getParent() == I->getParent())
        loopdet = rCycleDetector(cast<Instruction>(Us),visited,streami,processed,level+1);
      if(loopdet){
        streami->push_front(I);
        break;
      }
    }
    if(!loopdet){
      visited->erase(I);  
    }
    return loopdet;
  }

}

void FusedBB::CycleDetector(Function *F){
  set<Instruction*> processed;
  set<Instruction*> visited;
  list<Instruction*> streami;
  for(auto &I : *this->BB){
    visited.clear();
    if(!processed.count(&I))
      if(rCycleDetector(&I,&visited,&streami,&processed,0)){
        this->BB->insertInto(F);
        for(auto elem : streami)
          elem->dump();
        break;
      }
  }
}
    
void FusedBB::fillSubgraphsBBs(Instruction *I,set<string>*subset){
  if(fuseMap->count(I))
    for(auto *Iorig : (*(*fuseMap)[I])){
      subset->insert(Iorig->getParent()->getName().str());
    }
  return;

}


float FusedBB::getTseqSubgraph(list<Instruction*> *subgraph, map<string,long> *iterMap, map<BasicBlock*,float> *tseq_block){
  float tseq = 0;
  for(auto I=subgraph->begin();I!=subgraph->end();++I){
    if((*I)->getName().str().find("fuse.sel") == string::npos)
      for(auto Iorig : *(*fuseMap)[*I]){
        tseq += getSwDelay(*I)*((*iterMap)[Iorig->getParent()->getName().str()]);
      if(!tseq_block->count(Iorig->getParent()))
        (*tseq_block)[Iorig->getParent()] = 0;
      (*tseq_block)[Iorig->getParent()] += getSwDelay(*I);
        
    }
  }
  return tseq;
}


void FusedBB::getDebugLoc(stringstream &output){
  for (BasicBlock *BB : *mergedBBs){
    output << BB->getParent()->getName().str() << ":\n";
    errs() << BB->getName().str() << "\n";
    for(auto &I : *BB){
      DebugLoc DL = I.getDebugLoc();
      if(DL){
        DL.dump();
        
      }
    }
  }
}

void FusedBB::setOrigWeight(float orig_weight){
  this->orig_weight = orig_weight;
}
float FusedBB::getOrigWeight(){
  return this->orig_weight;
}
