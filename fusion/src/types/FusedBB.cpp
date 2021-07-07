#include "FusedBB.hpp"


FusedBB::~FusedBB(){
  delete mergedBBs;
  delete LiveOut;
  for(auto E : *linkOps)
    delete E.second;
  delete linkOps;
  delete countMerges;
  delete storeDomain;
  for(auto E : *rawDeps)
    delete E.second;
  delete rawDeps;
  for(auto E : *fuseMap)
    delete E.second;
  delete fuseMap;
  delete safeMemI;
  for(auto E : *selMap){
    delete E.second.first;
    delete E.second.second;
  }
  delete selMap;
  for(auto E : *liveInPos)
    delete E;
  delete liveInPos;
  delete LiveIn;
  for(auto E : *liveOutPos)
    delete E;
  delete liveOutPos;
  delete BB;
  return;
}

FusedBB::FusedBB(FusedBB* Copy, list<Instruction*> *subgraph){
  init(Copy->Context);
  
  ValueToValueMapTy VMap;
  
  this->BB = BasicBlock::Create(*Copy->Context,"");  
  for(auto E : *Copy->fuseMap)
    fuseMap->insert(pair<Instruction*,set<Instruction*>* >
        (E.first,new set<Instruction*>(*E.second)));

  for(auto E : *Copy->selMap)
    (*selMap)[E.first] = 
      pair<set<BasicBlock*>*,set<BasicBlock*>*> (
        new set<BasicBlock*> (*E.second.first),
        new set<BasicBlock*> (*E.second.second));

  for (Instruction &I : *Copy->getBB()){
    if(find(subgraph->begin(),subgraph->end(),&I) != subgraph->end()){
      Instruction *NewInst = I.clone();
      NewInst->setName(I.getName());
      BB->getInstList().push_back(NewInst);
      VMap[&I] = NewInst;  


      if(Copy->fuseMap->count(&I)){

        if(!this->fuseMap->count(NewInst))
          (*this->fuseMap)[NewInst] = new set<Instruction*>;
        
        for(auto E : *(*Copy->fuseMap)[&I]){
          (*this->fuseMap)[NewInst]->insert(E);
          this->mergedBBs->insert(E->getParent());
        }
      }
      
      if(Copy->selMap->count(&I)){
        (*this->selMap)[NewInst] = pair<set<BasicBlock*>*, set<BasicBlock*>*> 
          (new set<BasicBlock*>(*(*Copy->selMap)[&I].first),
           new set<BasicBlock*>(*(*Copy->selMap)[&I].second));
        this->selMap->erase(&I);
      }
      
      if(Copy->isMergedI(&I))
        (*countMerges)[NewInst] = (*(Copy->countMerges))[&I];

      if(Copy->safeMemI->count(&I))
        (*safeMemI)[NewInst] = (*(Copy->safeMemI))[&I];

      if(Copy->LiveOut->count(&I))
        this->LiveOut->insert(NewInst);
      else{
        for(auto E : I.users()){
          if(!this->LiveOut->count(NewInst) and 
              find(subgraph->begin(),subgraph->end(),&(*E)) == subgraph->end())
            this->LiveOut->insert(NewInst);
        }
      }
    }
  }

  for(auto E: *Copy->linkOps){
    Value *index;
    if(VMap.count(E.first))
      index = VMap[E.first];
    else
      index = E.first;
    (*this->linkOps)[index] =  new set<pair<Value*, BasicBlock*> >;
    for( auto Orig : *E.second )
      if(this->mergedBBs->count(Orig.second))
        (*this->linkOps)[index]->insert(
            pair<Value*,BasicBlock*>(Orig.first,Orig.second));
  }
  
  SetVector<Value*> LiveIn, LiveOut;
  liveInOut(*this->BB,&LiveIn,&LiveOut);
  //Optimize this, we don't need to copy everything
  for(auto E: *Copy->LiveIn)
      this->LiveIn->insert(E);
  for(auto E: LiveIn){
    //if(isa<SelectInst>(E) and E->getName() == "fuse.sel")
    //  this->LiveIn->insert(E); 
    //else{
      if(isa<Instruction>(E))
        if(Copy->fuseMap->count(cast<Instruction>(E))){
          for(auto Iorig : *(*Copy->fuseMap)[cast<Instruction>(E)]){
            this->LiveIn->insert(Iorig);     
            if(!this->linkOps->count(E))
              (*this->linkOps)[E] = new set<pair<Value*,BasicBlock*> >;
            (*this->linkOps)[E]->insert(
                pair<Value*,BasicBlock*>(Iorig,Iorig->getParent()));
          }
        }
        else{
          this->LiveIn->insert(E);
          if(!this->linkOps->count(E))
            (*this->linkOps)[E] = new set<pair<Value*,BasicBlock*> >;
          (*this->linkOps)[E]->insert(
            pair<Value*,BasicBlock*>(E,cast<Instruction>(E)->getParent()));    
        }
        /*else{
          this->LiveIn->insert(E);
          for(auto Ufused : cast<Instruction>(E)->users()){
            if(!this->LiveIn->count(E))
              (*this->LiveIn)[E] = new set<BasicBlock*>;
            if(this->fuseMap->count(cast<Instruction>(Ufused))){
              for(auto Iorig : *(*this->fuseMap)[cast<Instruction>(Ufused)])
                (*this->LiveIn)[E]->insert(Iorig->getParent());
            }
          }
        }
      } 
    }*/
  }
  
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

  LiveOut = new set<Value*>;
  for(auto E : *Copy->LiveOut){
    this->LiveOut->insert(VMap[cast<Value>(E)]);
  }

  linkOps = new map<Value*,set<pair<Value*,BasicBlock*> >* >;
  for(auto E : *Copy->linkOps){
    if(VMap.count(E.first))
      (*linkOps)[VMap[E.first]] = new set<pair<Value*,BasicBlock*> >(*E.second);
    else
      (*linkOps)[E.first] = new set<pair<Value*,BasicBlock*> >(*E.second);
  }

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

  selMap = new map<Instruction*,pair<set<BasicBlock*>*,set<BasicBlock*>*> >;
  for(auto E : *Copy->selMap)
    (*selMap)[cast<Instruction>(VMap[cast<Value>(E.first)])] = 
      pair<set<BasicBlock*>*,set<BasicBlock*>*> (
        new set<BasicBlock*> (*E.second.first),
        new set<BasicBlock*> (*E.second.second));

  liveInPos = new vector<map<BasicBlock*,Value*>* >;
  for(auto E : *Copy->liveInPos)
    liveInPos->push_back(new map<BasicBlock*,Value*>(*E));

  LiveIn = new set<Value*>;
  for(auto E: *Copy->LiveIn)
    LiveIn->insert(E);

  liveOutPos = new vector<map<BasicBlock*,set<Value*>*>* >;
  for(auto E : *Copy->liveOutPos)
    liveOutPos->push_back(new map<BasicBlock*,set<Value*>*>(*E));
}

void FusedBB::init(LLVMContext *Context){
  mergedBBs = new set<BasicBlock*>; 
  LiveOut = new set<Value*>;
  linkOps = new map<Value*,set<pair<Value*,BasicBlock*> >* >;
  countMerges = new map<Instruction*,int>;
  storeDomain = new map<Instruction*,int>;
  rawDeps = new map<Instruction*,set<Instruction*>* >;
  fuseMap = new map<Instruction*,set<Instruction*>* >;
  safeMemI = new map<Instruction*,unsigned>;
  selMap = new map<Instruction*,pair<set<BasicBlock*>*,set<BasicBlock*>*> >;
  liveInPos = new vector<map<BasicBlock*,Value*>* >;
  LiveIn = new set<Value*>;
  liveOutPos = new vector<map<BasicBlock*,set<Value*>*>*>;
  this->Context = Context;
  return;
}

FusedBB::FusedBB(LLVMContext *Context, string name){
  BB = BasicBlock::Create(*Context, name);
  init(Context);
  return;
}

// This should be fusing selects with the same values
Value* FusedBB::checkSelects(Value *Vorig, Value *Vfused,BasicBlock *BB,
    map<Value*,Value*> *SubOp, set<Value*> *LiveInBB){
  Value *found = NULL;

  if(Vfused->getName() == "fuse.sel" or Vfused->getName() == "fuse.sel.safe"){
    Instruction *If = cast<Instruction>(Vfused);
    Value *Vcomp = SubOp->count(Vorig)? (*SubOp)[Vorig] : Vorig;
    if((If->getOperand(1)->getName() == "fuse.sel" or If->getOperand(1)->getName() == 
          "fuse.sel.safe") and !(*this->selMap)[If].second->count(BB)){
      found = checkSelects(Vorig,If->getOperand(1),BB,SubOp,LiveInBB);
    }
    if((If->getOperand(2)->getName() == "fuse.sel" or If->getOperand(2)->getName() == 
        "fuse.sel.safe") and !found and !(*this->selMap)[If].first->count(BB)){
      found = checkSelects(Vorig,If->getOperand(2),BB,SubOp,LiveInBB);
    }
    if((Vcomp == If->getOperand(1) or (this->LiveIn->count(If->getOperand(1)) and
          LiveInBB->count(Vcomp))) and !found and 
        !(*this->selMap)[If].second->count(BB)){
      (*selMap)[If].first->insert(BB);
      found = Vfused;
    }
    else if((Vcomp == If->getOperand(2) or (this->LiveIn->count(If->getOperand(2)) and
            LiveInBB->count(Vcomp))) and !found and 
        !(*this->selMap)[If].first->count(BB)){
      (*selMap)[If].second->insert(BB);
      found = Vfused;
    }
  }

  return found;
}

bool FusedBB::checkLinks(Value *Vfused, Value *Vorig, BasicBlock *BB){
  bool found = false;
  if(this->linkOps->count(Vfused)){
    for(auto inV : *(*this->linkOps)[Vfused]){
      found = inV.second == BB and inV.first != Vorig;
      if(found) break;
    }
  }
  return found;
}

Value* FusedBB::searchSelects(Value *Vfused, Value *Vorig, Instruction *Ifused,
    Instruction *Iorig, map<Value*,Value*> *SubOp, IRBuilder<> *Builder,
    Value *SelVal){
  Value *SelI = NULL;
  Value *Vcomp = SubOp->count(Vorig)? (*SubOp)[Vorig] : Vorig;
  bool found = false;
  bool found_rev = false;
  for(auto fSel : (*this->selMap)){
    found = fSel.first->getOperand(1) == Vcomp and fSel.first->getOperand(2) == Vfused;
    found_rev = fSel.first->getOperand(1) == Vfused and fSel.first->getOperand(2) == Vcomp;

    if(found and !fSel.second.second->count(Iorig->getParent())){ 
      SelI = cast<Value>(fSel.first);
      fSel.second.first->insert(Iorig->getParent());
      break; 
    }
    else if(found_rev and !fSel.second.first->count(Iorig->getParent())){
      SelI = cast<Value>(fSel.first);
      fSel.second.second->insert(Iorig->getParent());
      break;
    }
  }
  if(!SelI){
    SelI = Builder->CreateSelect(SelVal,Vorig,Vfused,"fuse.sel");
    (*this->selMap)[cast<Instruction>(SelI)] = 
      pair<set<BasicBlock*>*,set<BasicBlock*>*> (
          new set<BasicBlock*>,new set<BasicBlock*>);
    (*this->selMap)[cast<Instruction>(SelI)].first->insert(Iorig->getParent());
    if(isa<Instruction>(Vfused)){
      if(Vfused->getName() == "fuse.sel"){
        for(auto BBSel : *(*this->selMap)[cast<Instruction>(Vfused)].first)
          (*this->selMap)[cast<Instruction>(SelI)].second->insert(BBSel);
        for(auto BBSel : *(*this->selMap)[cast<Instruction>(Vfused)].second)
          (*this->selMap)[cast<Instruction>(SelI)].second->insert(BBSel);
       }
       else{
        for(auto VFalse : *(*this->fuseMap)[Ifused])
          (*this->selMap)[cast<Instruction>(SelI)].second->insert(VFalse->getParent());
        }
      }
      else{
        for(auto VFalse : *(*this->fuseMap)[Ifused])
          (*this->selMap)[cast<Instruction>(SelI)].second->insert(VFalse->getParent());
      }        
  }
  return SelI;
}

void FusedBB::mergeOp(Instruction *Ifused, Instruction *Iorig,
    map<Value*,Value*> *SubOp, IRBuilder<> *Builder,
    Value *SelVal, set<Value*> *LiveInBB){
  map<int,int> match;
  set<int> used;
  map<int,Value*> m_vChkSel;

  for(int i = 0;i < Ifused->getNumOperands(); ++i){
    Value *Vfused = Ifused->getOperand(i);
    for(int j = 0; j < Iorig->getNumOperands() and !match.count(i); ++j){
      Value *Vorig = Iorig->getOperand(j);
      Value *vChkSel = NULL;
      if(Ifused->isCommutative() or !Ifused->isCommutative() and i == j){
        if(!used.count(j) and !checkLinks(Vfused,Vorig,Iorig->getParent()) and 
           (areOpsMergeable(Vfused,Vorig,Ifused->getParent(),
                             Iorig->getParent(),SubOp,this->LiveIn,LiveInBB) or 
            (vChkSel = checkSelects(Vorig,Vfused,Iorig->getParent(),SubOp,LiveInBB)))){
          match[i] = j;
          used.insert(j);
          if(vChkSel)
            m_vChkSel[i] = vChkSel;
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
      if(LiveInBB->count(Vorig)){
        if(m_vChkSel.count(i))
          Vfused = m_vChkSel[i];
        if(!linkOps->count(Vfused))
          (*linkOps)[Vfused] = new set<pair<Value*,BasicBlock*> >;
        (*linkOps)[Vfused]->insert(pair<Value*,BasicBlock*>(Vorig,
              Iorig->getParent()));   
      }
    } 
    else{
      for(int j = 0; j < Iorig->getNumOperands() and !SelI; ++j){
        if(!used.count(j)){
          Value *Vorig = Iorig->getOperand(j);
          assert( Vfused->getType() == Vorig->getType() &&
              " Types differ, wrong merge" );
          SelI = searchSelects(Vfused,Vorig,Ifused,Iorig,SubOp,Builder,SelVal);
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
void FusedBB::secureMem(Value *SelVal, BasicBlock *destBB, set<Value*> *LiveInBB){
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
        if(LiveInBB->count(oPtr)){

        }
        else{
          // If the instruction comes from B we put in normal order, otherwise we swap
          for(auto Iorig = (*fuseMap)[&I]->begin(); Iorig != (*fuseMap)[&I]->end() 
              and !SelSafe; ++Iorig)
            if((*Iorig)->getParent() ==  destBB)
              SelSafe = builder.CreateSelect(SelVal,oPtr,G,"fuse.sel.safe"); 
          if(!SelSafe)
            SelSafe = builder.CreateSelect(SelVal,G,oPtr,"fuse.sel.safe");
          (*selMap)[cast<Instruction>(SelSafe)] = 
            pair<set<BasicBlock*>*,set<BasicBlock*>*> (
                new set<BasicBlock*>, new set<BasicBlock*>);
          (*selMap)[cast<Instruction>(SelSafe)].first->insert(destBB);
          I.setOperand(index,SelSafe);         
        } 
        (*safeMemI)[&I] = 2;
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
  builder.SetInsertPoint(this->BB);
  Value *SelVal = new Argument(builder.getInt1Ty(),"fuse.sel.arg");

  this->memRAWDepAnalysis(BB);
  
  set<Instruction*> merged;
  map<Value*,Value*> SubOp;

  // Stats
  SetVector<Value*> SetVLiveInBB, LiveOutBB;
  liveInOut(*BB,&SetVLiveInBB,&LiveOutBB);
  set<Value*> LiveInBB (SetVLiveInBB.begin(),SetVLiveInBB.end());


  // Update Stats
  this->mergedBBs->insert(BB);

  for(Instruction &Ib : *BB){
    bool bmerged = false;
    if(!isa<BranchInst>(Ib)){
      for(Instruction &Ia : *this->BB){
        if(!merged.count(&Ia) and areInstMergeable(Ia,Ib)
            and checkNoLoop2(&Ib,&Ia,BB,&SubOp)){ 
          merged.insert(&Ia);
          bmerged = true;
          this->mergeOp(&Ia,&Ib,&SubOp,&builder,SelVal,&LiveInBB); 
          this->annotateMerge(&Ib,&Ia,BB); 
          this->addSafety(&Ia);
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
            and (find(LiveOutBB.begin(),LiveOutBB.end(),Vc) == LiveOutBB.end() or merged.count(&Ic) or this->selMap->count(&Ic)) ){
            Ic.setOperand(j,SubOp[Vc]);
          }
        }
    }
          
    this->updateRawDeps(&SubOp);
  }
  
  for(auto inV : LiveInBB){
    bool found = false;
    this->LiveIn->insert(inV);
    for(auto linkedV : *this->linkOps){
      if(found) break;
      for(auto origV : *linkedV.second){
        if(found) break;
        found = (inV == origV.first and origV.second == BB);
      }
    }
    if(!found){
      if(!this->linkOps->count(inV)){
        (*this->linkOps)[inV] = new set<pair<Value*,BasicBlock*> >;
        (*this->linkOps)[inV]->insert(pair<Value*,BasicBlock*>(inV,BB));
      }
      else{
        Value *inAux = new Argument(inV->getType(),inV->getName()+"arg");
        (*this->linkOps)[inAux] = new set<pair<Value*,BasicBlock*> >;
        (*this->linkOps)[inAux]->insert(pair<Value*,BasicBlock*>(inV,BB));
        for(auto E : SubOp){
          Value *Orig = E.first;
          Value *Fused = E.second;
          for(int op=0;op < cast<Instruction>(Orig)->getNumOperands(); op++ ){
            if(cast<Instruction>(Orig)->getOperand(op) == inV){
              cast<Instruction>(Fused)->setOperand(op,inAux);
            }
          }
        }
      }
    }
  }

  for(auto E : *this->fuseMap)
    for(auto outV: *E.second)
      if(cast<Instruction>(outV)->getParent() == BB)
        if(find(LiveOutBB.begin(),LiveOutBB.end(),outV) != LiveOutBB.end())
          this->LiveOut->insert(cast<Value>(E.first));

  this->updateStoreDomain(&SubOp);
  this->updateRawDeps(&SubOp);
  // TODO: check if this can be made more efficient
  if (this->getNumMerges() > 1){
    this->KahnSort();
    this->secureMem(SelVal,BB,&LiveInBB);
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
  //if(LiveOut->count(cast<Value>(InstrOrig))){
  //  if(this->LiveOut->count(InstrOrig)){
      //(*this->LiveOut)[InstrOrig]->insert(InstrFused);
   // }
    //else{
      //(*this->LiveOut)[InstrOrig] = new set<Instruction*>;
      //(*this->LiveOut)[InstrOrig]->insert(InstrFused);
     // (*this->LiveOut)[InstrOrig] = InstrFused;
    //}
  //}
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
  //if(!annotInst->count(Iorig))
  //  annotInst->insert(Iorig);
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

bool FusedBB::insertInlineCall(Function *F, map<Value*, Value*> *VMap){
  static IRBuilder<> Builder(*Context);
  vector<Type*> inputTypes;
  vector<Instruction*> removeInst;

  //Type *inputType = ((PointerType*)F->getArg(0)->getType())->getElementType();
  //Type *outputType = ((PointerType*)F->getReturnType())->getElementType();

  for(auto BB : *mergedBBs){

      // Function Arguments
      SmallVector<Value*,20> Args(this->liveInPos->size(),NULL);

      // LiveIn Values
      int pos = 0;
      for(auto E : *this->liveInPos){
        if(E->count(BB)){
          Value *inV = VMap->count((*E)[BB])? (*VMap)[(*E)[BB]]:(*E)[BB];
          Args[pos] = inV;
        }
        else
          Args[pos] = Constant::getNullValue(F->getArg(pos)->getType());
        pos++;
      }
			
      
      // Removing inlined instructions
      auto rI = BB->rbegin();
      for(; rI != BB->rend(); rI++ ){
        if(find(Args.begin(),Args.end(),(Value*)&(*rI)) != Args.end())
          break;
      }
      Builder.SetInsertPoint(&(*--rI));
      Value *outStruct = Builder.CreateCall(F,Args);

      //LiveOuts
      pos = 0;
      for(auto E : *this->liveOutPos){
        if(E->count(BB)){
          Value *outGEPData = Builder.CreateStructGEP(outStruct,pos);
          Value *out = Builder.CreateLoad((*(*E)[BB]->begin())->getType(),outGEPData);
          for(auto outV : *(*E)[BB]){
            outV->replaceAllUsesWith(out);
            // If the out value is the new livein of a merged bb we copy it's pos
            (*VMap)[outV] = out;  
            // Sort Users
          }
        }
        pos++;
      }

	}
  //verifyModule(*F->getParent());

}

// Remove Inlined Instructions
void FusedBB::removeOrigInst(){
  for(auto Map : *this->fuseMap)
    for(auto I : *Map.second)
      if(Map.first->getParent() == this->BB){
        I->replaceAllUsesWith(llvm::UndefValue::get(I->getType()));
        I->eraseFromParent();
  }
}

  /**
   * Substitute all given BBs for function calls and create wrappers to 
   * gather/scatter the data
   *
   * @param *F pointer to the function to be called
   * @return true if success
   */
  bool FusedBB::insertOffloadCall(Function *F){
    /*
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
*/
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

void FusedBB::inlineOutputSelects(SelectInst *SelI, int pos){
  Value *vTrue = SelI->getTrueValue();
  Value *vFalse = SelI->getFalseValue();
  Instruction *iTrue = dyn_cast<Instruction>(vTrue);
  Instruction *iFalse = dyn_cast<Instruction>(vFalse);
  if(vTrue->getName() == "fuse.sel" or vTrue->getName() == "fuse.sel.safe")
    inlineOutputSelects(cast<SelectInst>(vTrue), pos);
  else{
    if(iTrue and this->fuseMap->count(iTrue)){
      for(auto E : *(*this->selMap)[SelI].first)
        for(auto origI : *(*this->fuseMap)[iTrue])
          if(origI->getParent() == E){
            if(!(*this->liveOutPos)[pos]->count(E))
              (*(*this->liveOutPos)[pos])[E] = new set<Value*>;
            (*(*this->liveOutPos)[pos])[E]->insert(cast<Value>(origI));
            break;
          }
    }
    else if (!isa<Constant>(vTrue)){
      for(auto E : *(*this->selMap)[SelI].first){
        if(!(*this->liveOutPos)[pos]->count(E))
          (*(*this->liveOutPos)[pos])[E] = new set<Value*>;
        (*(*this->liveOutPos)[pos])[E]->insert(vTrue);
      }
    }
  }
  if(vFalse->getName() == "fuse.sel" or vFalse->getName() == "fuse.sel.safe")
    inlineOutputSelects(cast<SelectInst>(vFalse), pos);
  else{
    if(iFalse and this->fuseMap->count(iFalse)){
      for(auto E : *(*this->selMap)[SelI].second){
        for(auto origI : *(*this->fuseMap)[iFalse])
          if(origI->getParent() == E){
            if(!(*this->liveOutPos)[pos]->count(E))
              (*(*this->liveOutPos)[pos])[E] = new set<Value*>;
            (*(*this->liveOutPos)[pos])[E]->insert(cast<Value>(origI));
              break;
          }    
      }
    }
    else if (!isa<Constant>(vFalse)){
      for(auto E : *(*this->selMap)[SelI].second){
        if(!(*this->liveOutPos)[pos]->count(E))
          (*(*this->liveOutPos)[pos])[E] = new set<Value*>;
        (*(*this->liveOutPos)[pos])[E]->insert(vFalse);
      }
    }
  }
}

void FusedBB::inlineInputSelects(SelectInst *SelI, int pos){
  Value *vTrue = SelI->getTrueValue();
  Value *vFalse = SelI->getFalseValue();
  Instruction *iTrue = dyn_cast<Instruction>(vTrue);
  Instruction *iFalse = dyn_cast<Instruction>(vFalse);
  /*if(isa<Argument>(vTrue)){
    assert((*this->linkOps)[vTrue]->size() < 2);
    vTrue = (*this->linkOps)[vTrue]->begin()->first;
  }
  if(isa<Argument>(vFalse)){
    assert((*this->linkOps)[vFalse]->size() < 2);
    vFalse = (*this->linkOps)[vFalse]->begin()->first;
  }*/
  if(vTrue->getName() == "fuse.sel")
    inlineInputSelects(cast<SelectInst>(vTrue), pos);
  else{
    if(iTrue and this->fuseMap->count(iTrue)){
      for(auto E : *(*this->selMap)[SelI].first)
        for(auto origI : *(*this->fuseMap)[iTrue])
          if(origI->getParent() == E){
            (*(*this->liveInPos)[pos])[E] = cast<Value>(origI);
            break;
          }
    }
    else
      for(auto E : *(*this->selMap)[SelI].first){
        Value *auxV = vTrue;
        if(this->linkOps->count(vTrue)){
          for(auto vLink : *(*this->linkOps)[vTrue]){
            if(vLink.second == E){
              auxV = vLink.first;
              break;  
            }
          }
        }
        (*(*this->liveInPos)[pos])[E] = auxV;
    }
  }
  if(vFalse->getName() == "fuse.sel")
    inlineInputSelects(cast<SelectInst>(vFalse), pos);
  //TODO: Use the following lines also above in the vTrue management (More efficient)
  else{
    if(iFalse and this->fuseMap->count(iFalse)){
      for(auto origI : *(*this->fuseMap)[iFalse])
        if((*this->selMap)[SelI].second->count(origI->getParent()))
          (*(*this->liveInPos)[pos])[origI->getParent()] = cast<Value>(origI);
    }
    else{
      for(auto E : *(*this->selMap)[SelI].second){
        Value *auxV = vFalse;
        if(this->linkOps->count(vFalse)){
          for(auto vLink : *(*this->linkOps)[vFalse]){
            if(vLink.second == E){
              auxV = vLink.first;
              break;
            }
          }
        }
        (*(*this->liveInPos)[pos])[E] = auxV;
      }
    }
  }
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
  
  int pos = 0;
  for(auto Vin: LiveIn){
    this->liveInPos->push_back(new map<BasicBlock*,Value*>);
    if(isa<Instruction>(Vin)){
      Instruction *Iin = cast<Instruction>(Vin);
      if(isa<SelectInst>(Vin) and (Vin->getName() == "fuse.sel" or
          Vin->getName() == "fuse.sel.safe")){
        inlineInputSelects(cast<SelectInst>(Vin),pos);
      }
      else{
        for(auto linkV : *(*this->linkOps)[Vin])
          (*(*this->liveInPos)[pos])[linkV.second] = linkV.first;            
      }
    }
    else if (isa<Argument>(Vin)){
      if(Vin->getName() == "fuse.sel.arg"){
        Instruction *SelI = cast<Instruction>(*Vin->user_begin());
        for(auto E : *(*this->selMap)[SelI].first)
          (*(*this->liveInPos)[pos])[E] = Builder.getInt1(true);
        for(auto E : *(*this->selMap)[SelI].second)
          (*(*this->liveInPos)[pos])[E] = Builder.getInt1(false);
      }
      else if(this->linkOps->count(Vin)){
        for(auto linkV : *(*this->linkOps)[Vin])
          (*(*this->liveInPos)[pos])[linkV.second] = linkV.first;            
      }
      else{
        for(auto user : Vin->users())
          if(this->mergedBBs->count(cast<Instruction>(user)->getParent()))
            (*(*this->liveInPos)[pos])[cast<Instruction>(user)->getParent()] = Vin;
      }
    }
    else{
      if(this->linkOps->count(Vin)){
        for(auto linkV : *(*this->linkOps)[Vin])
          (*(*this->liveInPos)[pos])[linkV.second] = linkV.first;            
      }
      /*for(auto user : Vin->users()){
        if(this->fuseMap->count(cast<Instruction>(user))){
          for(auto Iorig : *(*this->fuseMap)[cast<Instruction>(user)]){
            if(((*this->liveInPos)[pos])->count(Iorig->getParent())){
              this->liveInPos->push_back(new map<BasicBlock*,Value*>);
              pos++;
            }
            (*(*this->liveInPos)[pos])[Iorig->getParent()] = Vin;
          }
        }
      }*/
    }
    pos++;
  }

  pos = 0;
  for(auto outV : *this->LiveOut){
    this->liveOutPos->push_back(new map<BasicBlock*,set<Value*>*>);
    if(this->fuseMap->count(cast<Instruction>(outV))){
      for(auto origOut : *((*this->fuseMap)[cast<Instruction>(outV)])){
        if(!(*this->liveOutPos)[pos]->count(origOut->getParent()))
          (*(*this->liveOutPos)[pos])[origOut->getParent()] = new set<Value*>;
        (*(*this->liveOutPos)[pos])[origOut->getParent()]->insert(origOut);  
      }
    }
    else if(isa<SelectInst>(outV) and outV->getName() == "fuse.sel"){
      inlineOutputSelects(cast<SelectInst>(outV),pos);
    }
    pos++;
  }

  // Instantate all LiveIn values as Inputs types
  for(auto V: *this->liveInPos){
    inputTypes.push_back((V->begin()->second)->getType());
  }
  // Instantate all LiveOut values as Output types
  for(auto V: *this->LiveOut){
    outputTypes.push_back(V->getType());
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
    pos = 0;
    for(auto Vout : *this->LiveOut){
      Value *outGEPData = Builder.CreateStructGEP(outData,pos);
      Builder.CreateStore(Vout,outGEPData);
      pos++;
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
/*
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
*/
    /*
    for(auto &I: BB){
      Builder.SetInsertPoint(&I);
      if(isa<StoreInst>(I))
        Builder.CreateCall(dbgt->getFunctionType(),cast<Value>(dbgt));  
    }*/

    // Store Data (Creating New BB for readibility)
    // 
     /*
      BasicBlock* storeBB = BasicBlock::Create(*Context,"storeBB",f);
      Builder.SetInsertPoint(storeBB);
      //Builder.CreateCall(dbgt->getFunctionType(),cast<Value>(dbgt));  
      Value *outData = Builder.CreateAlloca(outStruct);
      /*for(int i=0;i<LiveOut.size();++i){
        Value *outGEPData = Builder.CreateStructGEP(outData,i);
        Builder.CreateStore(LiveOut[i],outGEPData);
      }*/
  /*
      for(auto Vout : *this->liveOutPos){
        Value *outGEPData = Builder.CreateStructGEP(outData,Vout.second);
        Builder.CreateStore(Vout.first,outGEPData);
      }
      Builder.CreateRet(outData);
  
      // Merged block jumps to store output data
      Builder.SetInsertPoint(BB);
      Builder.CreateBr(storeBB);
*/
    // Check Consistency
 //   verifyFunction(*f,&errs());

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
    //MDNode *N = MDNode::get(f->getContext(),NULL);
    //f->setMetadata("opencl.kernels",N);

    //return f;
    return NULL;
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

void FusedBB::traverseExclude(Instruction *I, set<Instruction*> *visited,
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
    if(this->rawDeps->count(I))
      for(auto rDep : *(*rawDeps)[I])
        traverseExclude(rDep,visited,excluded,list);
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
          for(int i=0;i<(*I)->getNumOperands() and nouse;++i)
            for(auto O = (*subgraphs)[subgraphs->size()-1]->begin();
                     O != (*subgraphs)[subgraphs->size()-1]->end() and nouse;++O)
          nouse &=  (*O) != (*I)->getOperand(i);
          if(nouse){
            (*subgraphs)[subgraphs->size()-1]->erase(I--);
            removed = true;
          }
          // Disabling this. Probably better if the output values are collapsed ino
          // a single output, rather than avoiding a bunch of selects but having
          // more output values
          // TL;DR: Removing trailing selects increases number of outputs
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
  map<string,set<debug_desc> >files;
  output << BB->getName().str() << ":\n";
  for(auto &I : *this->BB){
    if(this->fuseMap->count(&I)){
      for(auto Iorig : *(*this->fuseMap)[&I]){
        DebugLoc DL = Iorig->getDebugLoc();
        if(DL){
          int line = DL.getLine();
          int col = DL.getCol();
          string file = cast<DIScope>(DL.getScope())->getFilename().str();
          string dir = cast<DIScope>(DL.getScope())->getDirectory().str();
          debug_desc aux = {line,col,this->isMergedI(&I)};
          files[dir+"/"+file].insert(aux);
        } 
      }
    }
  }
  for(auto file : files){
    ifstream source;
    set<int> lines;
    string sourcecode;
    source.open(file.first, ifstream::in);
    output << "\t" << file.first << ":\n\n";
    for(auto line : file.second){
      if(!lines.count(line.line)){
        for(int i=0;i<line.line;++i)
          getline(source,sourcecode);
        int stop = sourcecode.find(")",line.col);
        if(stop == string::npos)
          stop = sourcecode.find("]",line.col);
        if(stop == string::npos)
          stop = sourcecode.find("}",line.col);
        if(stop == string::npos)
          stop = sourcecode.length();
        if(stop != string::npos)
          sourcecode.insert(stop,RESET);
        string color = line.merged?GREEN:RED;
        sourcecode.insert(line.col?line.col-1:line.col,color);
        sourcecode = to_string(line.line)+sourcecode;
        output << sourcecode << "\n";
        source.seekg(0);
      }
      lines.insert(line.line);
    }
    output << "\n";
    source.close();
  }
}

void FusedBB::setOrigWeight(float orig_weight){
  this->orig_weight = orig_weight;
}
float FusedBB::getOrigWeight(){
  return this->orig_weight;
}

string FusedBB::getSelBB(Instruction *SelI, Value *Op){
  string ret;

  if(this->selMap->count(SelI)){
    if(Op == SelI->getOperand(1)){
      for(auto BB : *(*this->selMap)[SelI].first){
        ret += BB->getName().str()+",";
      }
    }
    else if(Op == SelI->getOperand(2)){
      for(auto BB : *(*this->selMap)[SelI].second){
        ret += BB->getName().str()+",";
      }
    }
  }
  ret += ".";
  return ret;
}

pair<float,pair<float,float> > FusedBB::overheadCosts(map<string,long> *iterMap){
  float ins, outs, invok;
  ins = outs = invok = 0;
  for(auto BBs : *this->mergedBBs)
    invok += (*iterMap)[BBs->getName().str()];

  if(this->liveInPos->size()){
    for(auto E : *this->liveInPos){
      for(auto In : *E){
        ins += 1*(*iterMap)[In.first->getName().str()];
      }
    }
  }
  if(this->liveOutPos->size()){
    for(auto E : *this->liveOutPos){
      for(auto Out : *E){
        outs += 1*(*iterMap)[Out.first->getName().str()];
      }
    }
  }
  return pair<float,pair<float,float> >(invok, pair<float,float>(ins,outs));
}
