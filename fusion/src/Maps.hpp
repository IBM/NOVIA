#include <map>

using namespace std;
using namespace llvm;

// Weight Delay model taken from Region Seeker - Giorgios et al. in nS
map<unsigned,float> delay = {
  {Instruction::Br,0.001},
  {Instruction::Add,0.92},
  {Instruction::Sub,0.92},
  {Instruction::Mul,1},
  {Instruction::UDiv,3.76},
  {Instruction::SDiv,3.76},
  {Instruction::URem,4.04},
  {Instruction::SRem,4.04},
  {Instruction::Shl,0.71},
  {Instruction::LShr,0.73},
  {Instruction::AShr,0.65},
  {Instruction::And,0.02},
  {Instruction::Or,0.03},
  {Instruction::Xor,0.03},
  {Instruction::Select,0.23},
  {Instruction::ICmp,0.15},
  {Instruction::FCmp,0.15},
  {Instruction::ZExt,0.01},
  {Instruction::SExt,0.01},
  {Instruction::FPToUI,0.01},
  {Instruction::FPToSI,0.01},
  {Instruction::FPExt,0.01},
  {Instruction::PtrToInt,0.01},
  {Instruction::IntToPtr,0.01},
  {Instruction::SIToFP,0.01},
  {Instruction::UIToFP,0.01},
  {Instruction::Trunc,0.01},
  {Instruction::FPTrunc,0.01},
  {Instruction::BitCast,0.01},
  {Instruction::Load,10},
  {Instruction::Store,3},
  {Instruction::GetElementPtr,1.92},
  {Instruction::Alloca,0.01},
  {Instruction::PHI,0.23},
  {Instruction::Call,0.01},
  {Instruction::Switch,0.23},
  {Instruction::Ret,0.01},
  {Instruction::FAdd,0.92},
  {Instruction::FSub,0.92},
  {Instruction::FMul,1},
  {Instruction::FDiv,3.76},
  {Instruction::FRem,4.04},
};

// Weight model taken from Region Seeker by Gioergios et al. in uM^2
map<unsigned,int> area = {
  {Instruction::Br,1},
  {Instruction::Add,160},
  {Instruction::Sub,173},
  {Instruction::Mul,2275},
  {Instruction::UDiv,16114},
  {Instruction::SDiv,16114},
  {Instruction::URem,17298},
  {Instruction::SRem,17298},
  {Instruction::Shl,187},
  {Instruction::LShr,187},
  {Instruction::AShr,311},
  {Instruction::And,26},
  {Instruction::Or,26},
  {Instruction::Xor,40},
  {Instruction::Select,40},
  {Instruction::ICmp,78},
  {Instruction::FCmp,78},
  {Instruction::ZExt,1},
  {Instruction::SExt,1},
  {Instruction::FPToUI,1},
  {Instruction::FPToSI,1},
  {Instruction::FPExt,1},
  {Instruction::PtrToInt,1},
  {Instruction::IntToPtr,1},
  {Instruction::SIToFP,1},
  {Instruction::UIToFP,1},
  {Instruction::Trunc,1},
  {Instruction::FPTrunc,1},
  {Instruction::BitCast,1},
  {Instruction::Load,1},
  {Instruction::Store,1},
  {Instruction::GetElementPtr,1},
  {Instruction::Alloca,1},
  {Instruction::PHI,40},
  {Instruction::Call,1},
  {Instruction::Switch,40},
  {Instruction::Ret,1},
  {Instruction::FAdd,160},
  {Instruction::FSub,173},
  {Instruction::FMul,2275},
  {Instruction::FDiv,16114},
  {Instruction::FRem,17298},
};

// In nJ
map<unsigned,int> energy = { 
  {Instruction::Br,1},
  {Instruction::Add,3},
  {Instruction::Sub,3},
  {Instruction::Mul,9},
  {Instruction::UDiv,12},
  {Instruction::SDiv,12},
  {Instruction::URem,14},
  {Instruction::SRem,14},
  {Instruction::Shl,3},
  {Instruction::LShr,3},
  {Instruction::AShr,3},
  {Instruction::And,2},
  {Instruction::Or,2},
  {Instruction::Xor,2},
  {Instruction::Select,1},
  {Instruction::ICmp,2},
  {Instruction::FCmp,2},
  {Instruction::ZExt,1},
  {Instruction::SExt,1},
  {Instruction::FPToUI,1},
  {Instruction::FPToSI,1},
  {Instruction::FPExt,1},
  {Instruction::PtrToInt,1},
  {Instruction::IntToPtr,1},
  {Instruction::SIToFP,1},
  {Instruction::UIToFP,1},
  {Instruction::Trunc,1},
  {Instruction::FPTrunc,1},
  {Instruction::BitCast,1},
  {Instruction::Load,500},
  {Instruction::Store,100},
  {Instruction::GetElementPtr,12},
  {Instruction::Alloca,1},
  {Instruction::PHI,1},
  {Instruction::Call,1},
  {Instruction::Switch,2},
  {Instruction::Ret,1},
  {Instruction::FAdd,6},
  {Instruction::FSub,6},
  {Instruction::FMul,18},
  {Instruction::FDiv,20},
  {Instruction::FRem,23},
};

// Static power in nwatts
map<unsigned,float> powersta = { 
  {Instruction::Br,0},
  {Instruction::Add,0.5},
  {Instruction::Sub,0.5},
  {Instruction::Mul,0.15},
  {Instruction::UDiv,0.3},
  {Instruction::SDiv,0.3},
  {Instruction::URem,0.35},
  {Instruction::SRem,0.35},
  {Instruction::Shl,0.1},
  {Instruction::LShr,0.1},
  {Instruction::AShr,0.1},
  {Instruction::And,0.2},
  {Instruction::Or,0.2},
  {Instruction::Xor,0.2},
  {Instruction::Select,0.1},
  {Instruction::ICmp,0.5},
  {Instruction::FCmp,0.5},
  {Instruction::ZExt,0.1},
  {Instruction::SExt,0.1},
  {Instruction::FPToUI,0.1},
  {Instruction::FPToSI,0.1},
  {Instruction::FPExt,0.1},
  {Instruction::PtrToInt,0.1},
  {Instruction::IntToPtr,0.1},
  {Instruction::SIToFP,0.1},
  {Instruction::UIToFP,0.1},
  {Instruction::Trunc,0.1},
  {Instruction::FPTrunc,0.1},
  {Instruction::BitCast,0.1},
  {Instruction::Load,0.5},
  {Instruction::Store,0.2},
  {Instruction::GetElementPtr,0.2},
  {Instruction::Alloca,0},
  {Instruction::PHI,0.3},
  {Instruction::Call,0},
  {Instruction::Switch,0.2},
  {Instruction::Ret,0},
  {Instruction::FAdd,0.15},
  {Instruction::FSub,0.15},
  {Instruction::FMul,0.4},
  {Instruction::FDiv,0.7},
  {Instruction::FRem,0.8},
};


float getDelay(Instruction *I){
  float ret = 0; // default value
  if(delay.count(I->getOpcode()))
    ret = delay[I->getOpcode()]/1000000000; // from ns to seconds
  else
    errs() << "Missing Delay Value (" << I->getOpcodeName() << ")\n";
  return ret;
}

int getEnergyDyn(Instruction *I){
  int ret = 0; // default value
  if(energy.count(I->getOpcode()))
    ret = energy[I->getOpcode()]/1000000000;
  else
    errs() << "Missing Energy Value (" << I->getOpcodeName() << ")\n";
  return ret;
}

float getPowerSta(Instruction *I){
  float ret = 0; // default value
  if(powersta.count(I->getOpcode()))
    ret = powersta[I->getOpcode()]/1000000000;
  else
    errs() << "Missing Energy Value (" << I->getOpcodeName() << ")\n";
  return ret;
}

int getArea(Instruction *I){
  int ret = 0; // default value
  if(area.count(I->getOpcode()))
    ret = area[I->getOpcode()];
  else
    errs() << "Missing Area Value (" << I->getOpcodeName() << ")\n";
  return ret;
}
