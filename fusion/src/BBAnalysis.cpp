//===-----------------------------------------------------------------------------------------===// 
// Copyright 2022 IBM
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//===-----------------------------------------------------------------------------------------===// 
#include "BBAnalysis.hpp"
#include "Maps.hpp"

//TODO: Check if we have to create a (new) GV or we can just get one with 
//the same value trought some getter function on the module.
Value *getSafePointer(PointerType *O, Module *M){
  Value *V =  new GlobalVariable(*M,O->getPointerElementType(),false,GlobalValue::ExternalLinkage,Constant::getNullValue(O->getPointerElementType()));
  return V;
}

/**
 * Checks if operands can be merged, returns true if they are
 *
 * @param &Ia instruction to check
 * @param &Ib instruction to check
 * @param &IRBuilder for inserting select instructions
 * @return operands merge
 */
bool areOpsMergeable(Value *opA, Value *opB, BasicBlock *A, BasicBlock *B,
                    map<Value*,Value*> *SubOp, set<Value*> *LiveInA, set<Value*> *LiveInB){
	bool sameValue = false;
	bool isLiveIn = false;
	bool isInstMerg = false;
  bool isConstant = false;
  bool isSameType = false;
  bool sameOpAfterMerge = false;

	sameValue = opA == opB;
  // Inst are same Value Constants
  bool isSame = sameValue;
  isConstant = isa<Constant>(opA) or isa<Constant>(opB);
  isSameType = opA->getType() == opB->getType();
	// If operands come from oustide the BB, they are fusable LiveIns
	isLiveIn = LiveInA->count(opA) and LiveInB->count(opB);
		// If operands come from mergeable Instructions
	isInstMerg = areInstMergeable(*(Instruction*)opA,*(Instruction*)opB);
  if(SubOp->count(opB)){
    sameOpAfterMerge = opA == (*SubOp)[opB];
  }
	return (sameValue and isConstant) or (isSameType and isLiveIn) or (isSameType and isInstMerg and !isConstant and sameOpAfterMerge);
}

/**
 * Checks if two instructions are mergeable, returns true if they are
 *
 * @param *Ia Instruction to check
 * @param *Ib Instruction to check
 * @return true if mergeable, false otherwise
 */
bool areInstMergeable(Instruction &Ia, Instruction &Ib){
	// Same opcode
	bool opcode = Ia.getOpcode() == Ib.getOpcode();
  if(opcode and isa<CmpInst>(Ia)){
    opcode &= cast<CmpInst>(Ia).getPredicate() == cast<CmpInst>(Ib).getPredicate();
  }

  // If operands are loads, they must be loading the same type of data
  // TODO: Check if this can be generalized to ints vs floats
  bool loadty = opcode;
  if (loadty and isa<LoadInst>(Ia))
    loadty = cast<LoadInst>(Ia).getPointerOperandType() ==
             cast<LoadInst>(Ib).getPointerOperandType();
  // Same for stores
  bool storety = opcode;
  if (storety and isa<StoreInst>(Ia))
    storety = cast<StoreInst>(Ia).getPointerOperandType() ==
              cast<StoreInst>(Ib).getPointerOperandType();
  // Some getelemptr have different amount of operands
  // TODO: Check if those can be merged, for now we are leaving them asside
  bool numops = Ia.getNumOperands() == Ib.getNumOperands();
  // Check operand compatibility
  bool samety = Ia.getType() == Ib.getType();
  if(opcode and samety and numops)
    for(int i=0;i<Ia.getNumOperands();++i){
      samety &= Ia.getOperand(i)->getType() == Ib.getOperand(i)->getType();
    }

  // Do not merge select instructions, we will deal with this in later 
  // optimization steps
  bool noselect = !isa<SelectInst>(Ia);
  bool samecall = true;
  if(isa<CallInst>(Ia) and isa<CallInst>(Ib))
    samecall = cast<CallInst>(Ia).getCaller() == cast<CallInst>(Ib).getCaller();
  samecall &= !isa<InvokeInst>(Ia) and !isa<InvokeInst>(Ib);
  bool samegetelemptr = true;
  if(isa<GetElementPtrInst>(Ia) and isa<GetElementPtrInst>(Ib) and samety and numops)
    if(cast<PointerType>(Ia.getOperand(0)->getType())->getElementType()->isStructTy())
    for(int i =0;i<Ia.getNumOperands();++i)
      samegetelemptr &= Ia.getOperand(i) == Ib.getOperand(i);

	return opcode and loadty and storety and numops and noselect and samety and samecall and samegetelemptr;
}

/**
 * This function annotates all the RAW memory depedences. It attaches a Value
 * as Metadata to each store indicating what load reads that data. Dependence 
 * is Analyzed per BB
 *
 * @param *BB The BasicBlock to analyze
 * @param *RAWdeps Map with a relation of dependences (TODO: deprecate this)
 */
void memRAWDepAnalysis(BasicBlock *BB, map<Instruction*,set<Instruction*> *> *rawDeps){
  StoreInst *St;
  LoadInst *Ld;
  map<Value*,Instruction*> Wdeps;
  map<Instruction*,int> storeDomain;
  map<Value*,int> StoreDomain;
  for (auto &I : *BB){
    // For each store, we record the address being used
    if(St = dyn_cast<StoreInst>(&I)){
      if(StoreDomain.count(St->getPointerOperand()))
        StoreDomain[St->getPointerOperand()]++;
      else
        StoreDomain[St->getPointerOperand()] = 1;
      Wdeps[St->getPointerOperand()] = &I;
      storeDomain[&I] = StoreDomain[St->getPointerOperand()];
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

/**
 * Computes the variables that are LiveIn in a BasicBlock
 *
 * @param BB BasicBlock to Analyze
 * @return List of live out Variables
 */
void liveInOut(BasicBlock &BB, SetVector<Value*> *LiveIn,
			   SetVector<Value*> *LiveOut){
	using ValueSet = SetVector<Value *>;
  ValueSet Allocas;
	/*ArrayRef<BasicBlock*> *BBs = new ArrayRef<BasicBlock*>(&BB);
	CodeExtractor CE = CodeExtractor(*BBs);
	CE.findInputsOutputs((ValueSet&)*LiveIn,(ValueSet&)*LiveOut,Allocas);
  delete BBs;*/
  
  
  for(auto &I: BB){
    for(int i = 0; i < I.getNumOperands(); ++i){
      Value *V = I.getOperand(i);
      if(isa<Instruction>(V) && cast<Instruction>(V)->getParent() != &BB or
          isa<Argument>(V) or isa<GlobalValue>(V))
        LiveIn->insert(V);
    }
    for(auto U : I.users()){
      Instruction *UI = (Instruction*)U;
      if(UI->getParent() != &BB)
        LiveOut->insert(cast<Value>(&I));
    }
  }
  // If a variable is annotated as liveout, we add it to the list of LiveOuts
  // Developer can force out values this way.
  for(auto I=BB.begin(), E=BB.end(); I != E; ++I){
    if(I->getMetadata("is.liveout"))
      LiveOut->insert(cast<Value>(I));
  }
	
  return;
}

float dfsCrit(Instruction *I, BasicBlock *BB,map<Instruction*,float> *visited){
  float cost = 0;
  if ( I->getParent() == BB ){
    for(User *U : I->users()){
      float tmp;
      if(visited->count((Instruction*)U)){
        tmp = (*visited)[(Instruction*)U];
      }
      else{
        tmp = dfsCrit((Instruction*)U,BB,visited);
      }
      if(tmp>cost)
        cost = tmp;
    }
    (*visited)[I] = cost+getHwDelay(I);
    return cost + getHwDelay(I);
  }
  return 0;
}


/**
 * This functions computes the critical path delay in a given BasicBlock.
 * It traverses the instructions and dependence trees and computes the delays
 * for each one of them.
 * If the BasicBlock is from and offload funcion with send/receive (loadBB,
 * storeBB) basicblocks preceeding and succeding it. Then the memory worst case
 * critical path is also computed
 *
 * @param *BB Reference Basic Block to compute the critical path in
 * @retrurn a pair of floats. first contains the overhead/worst case memory
 * critical path delay. second contains the computation critical path delay (
 * the delay of the BasicBlock used as input)
 */
// TODO: Improve efficiency by not traversing instructions already visited
// by other critical path computaions
//

pair<float,float> getCriticalPathCost(BasicBlock *BB){
  pair<float,float> cost = pair<int,int>(0,0); // Overhead, Computation
  float tpred = 0, tsucc = 0;
  BasicBlock *pred, *succ;
  map<Instruction*,float> visited;
  // If we have a receive data BB preceeding, compute it's cirtical path
  if((pred = BB->getSinglePredecessor()) and pred->getName() == "loadBB"){
    for(auto &I: *pred){
      float tmp = dfsCrit(&I,pred,&visited);
      if(tmp>tpred)
        tpred = tmp;
    }
  }
  // If we have a send data BB succeding, compute it's cirtical path
  if((succ = BB->getSingleSuccessor()) and succ->getName() == "storeBB"){
    for(auto &I: *BB){
      float tmp = dfsCrit(&I,succ,&visited);
      if(tmp>tsucc)
        tsucc = tmp;
    }
  }

  for(auto &I: *BB){
    float tmp = dfsCrit(&I,BB,&visited);
    if(tmp>cost.second)
      cost.second = tmp;
  }
  // Factor in the cost of the cpu side send/receive. Each memory access will 
  // have a complementary cpu memory access. Therefore we multiply by 2
  cost.first = (tpred+tsucc)*2;

  return cost;
}

/**
 * This function gathers and computes several staistics and metrics for a 
 * given BasicBlock.
 *
 * @param *BB BasicBlock to analyze and attach metadata
 * @param *data Vector to be filled with the statitstics value
 * @param *M LLVM Module
 */
void getMetadataMetrics(BasicBlock *BB, vector<double> *data, Module *M){
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
  double time = 0;
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
    if (N = (I->getMetadata("fuse.merged"))){
      int tmp = cast<ConstantInt>(cast<ConstantAsMetadata>(
            N->getOperand(0))->getValue())->getSExtValue();
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

void getDebugLoc(BasicBlock *BB, stringstream &output){
  map<string,set<debug_desc> >files;
  set<string> funcs;
  output << BB->getName().str() << "[";
  for(auto &Iorig : *BB){
        DebugLoc DL = Iorig.getDebugLoc();
        if(DL){
          int line = DL.getLine();
          int col = DL.getCol();
          string file = cast<DIScope>(DL.getScope())->getFilename().str();
          string dir = cast<DIScope>(DL.getScope())->getDirectory().str();
          funcs.insert(Iorig.getParent()->getParent()->getName().str());
          debug_desc aux = {line,col,false};
          if(file[0]=='/')
            files[file].insert(aux);
          else
            files[dir+"/"+file].insert(aux);
        } 
  }
  for(auto func : funcs){
    output << func << ";";
  }
  output << "]\n";
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
