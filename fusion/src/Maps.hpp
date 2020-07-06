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
  {Instruction::Load,1},
  {Instruction::Store,1},
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

// Energy model In nJ callibrated with Power8/9 measurements
map<unsigned,float> energy = { 
  {Instruction::Br,0.5},
  {Instruction::Add,40.6},
  {Instruction::Sub,40.6},
  {Instruction::Mul,49.2},
  {Instruction::UDiv,482.3},
  {Instruction::SDiv,489.3},
  {Instruction::URem,601.9},
  {Instruction::SRem,601.9},
  {Instruction::Shl,42.5},
  {Instruction::LShr,46.9},
  {Instruction::AShr,61.5},
  {Instruction::And,38.7},
  {Instruction::Or,39.1},
  {Instruction::Xor,39.9},
  {Instruction::Select,48.5},
  {Instruction::ICmp,58.8},
  {Instruction::FCmp,58.4},
  {Instruction::ZExt,20},
  {Instruction::SExt,20},
  {Instruction::FPToUI,44.0},
  {Instruction::FPToSI,44.0 },
  {Instruction::FPExt,44.0},
  {Instruction::PtrToInt,0.5},
  {Instruction::IntToPtr,0.5},
  {Instruction::SIToFP,44.0},
  {Instruction::UIToFP,44.0},
  {Instruction::Trunc,20},
  {Instruction::FPTrunc,20},
  {Instruction::BitCast,0.5},
  {Instruction::Load,75.1},
  {Instruction::Store,93.1},
  {Instruction::GetElementPtr,40.6},
  {Instruction::Alloca,40.6},
  {Instruction::PHI,0.5},
  {Instruction::Call,0.5},
  {Instruction::Switch,0.5},
  {Instruction::Ret,0.5},
  {Instruction::FAdd,50.4},
  {Instruction::FSub,50.4},
  {Instruction::FMul,50.4},
  {Instruction::FDiv,968.4},
  {Instruction::FRem,1069.2},
};

// Static power in nwatts not callibrated
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


/**
 * Get the delay in seconds 
 *
 * @param I Instruction to get the delay from
 */
float getDelay(Instruction *I){
  float ret = 0; // default value
  if(delay.count(I->getOpcode()))
    ret = delay[I->getOpcode()]/1000000000; // from ns to seconds
  else
    errs() << "Missing Delay Value (" << I->getOpcodeName() << ")\n";
  return ret;
}

/**
 * Get the dynamic energy consumption in nanoJoules
 *
 * @param I Instruction to get the energy from
 */
float getEnergyDyn(Instruction *I){
  float ret = 0; // default value
  if(energy.count(I->getOpcode())){
    ret = energy[I->getOpcode()]-28;
    if(ret<0) ret = 0.5;
  }
  else
    errs() << "Missing Energy Value (" << I->getOpcodeName() << ")\n";
  return ret;
}

/**
 * Get the static power dissipation in nanoWatts
 *
 * @param I Instruction to get the static power from
 */
float getPowerSta(Instruction *I){
  float ret = 0; // default value
  if(powersta.count(I->getOpcode()))
    ret = powersta[I->getOpcode()];
  else
    errs() << "Missing Energy Value (" << I->getOpcodeName() << ")\n";
  return ret;
}

/**
 * Get hardware area in microns squared
 *
 * @param I Instruction to get the area from
 */
int getArea(Instruction *I){
  int ret = 0; // default value
  if(area.count(I->getOpcode()))
    ret = area[I->getOpcode()];
  else
    errs() << "Missing Area Value (" << I->getOpcodeName() << ")\n";
  return ret;
}
