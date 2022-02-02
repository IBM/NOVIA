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
#include <map>

using namespace std;
using namespace llvm;

#define CORE_FREQ 78000000// Core frequency in Hz

// Delay for operation in cycles
// Setting it to 1 cycle
static map<unsigned, unsigned> swdelay = {
  {Instruction::Br,0},
  {Instruction::Add,1},
  {Instruction::Sub,1},
  {Instruction::Mul,1},
  {Instruction::UDiv,1},
  {Instruction::SDiv,1},
  {Instruction::URem,1},
  {Instruction::SRem,1},
  {Instruction::Shl,1},
  {Instruction::LShr,1},
  {Instruction::AShr,1},
  {Instruction::And,1},
  {Instruction::Or,1},
  {Instruction::Xor,1},
  {Instruction::Select,1},
  {Instruction::ICmp,1},
  {Instruction::FCmp,1},
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
  {Instruction::PHI,1},
  {Instruction::Call,1},
  {Instruction::Invoke,1},
  {Instruction::Switch,0},
  {Instruction::Ret,0},
  {Instruction::FNeg,1},
  {Instruction::FAdd,1},
  {Instruction::FSub,1},
  {Instruction::FMul,1},
  {Instruction::FDiv,1},
  {Instruction::FRem,1},
  {Instruction::ShuffleVector,1},
  {Instruction::ExtractElement,1},
  {Instruction::InsertElement,1},
  {Instruction::InsertValue,1}
};

// Weight Delay model taken from Region Seeker - Giorgios et al. in nS
static map<unsigned,double> hwdelay = {
  {Instruction::Br,0.0001},
  {Instruction::Add,1.514}, //HLS
  {Instruction::Sub,1.514}, // HLS
  {Instruction::Mul,6.282}, //HLS
  {Instruction::UDiv,74.69}, // HLS more than 1 cycle (35)
  {Instruction::SDiv,74.69}, //HLS more than 1 cycle (35)
  {Instruction::URem,74.69}, // HLS more than 1 cycle (35)
  {Instruction::SRem,74.69}, // HLS more than 1 cycle (35)
  {Instruction::Shl,1.89}, //HLS
  {Instruction::LShr,1.89},
  {Instruction::AShr,1.89}, //HLS
  {Instruction::And,0.521}, //HLS
  {Instruction::Or,0.521}, //HLS
  {Instruction::Xor,0.521}, //HLS
  {Instruction::Select,0.87}, //HLS
  {Instruction::ICmp,1.26}, //HLS
  {Instruction::FCmp,12.25}, //HLS
  {Instruction::ZExt,0},//HLS
  {Instruction::SExt,0},//HLS
  {Instruction::FPToUI,3.693},//HLS
  {Instruction::FPToSI,6.077},//HLS
  {Instruction::FPExt,10.315},//HLS
  {Instruction::PtrToInt,0.01},
  {Instruction::IntToPtr,0.01},
  {Instruction::SIToFP,39.785},//HLS
  {Instruction::UIToFP,39.785},//HLS
  {Instruction::Trunc,0},//HLS
  {Instruction::FPTrunc,12.895},//HLS
  {Instruction::BitCast,0}, // HLS
  {Instruction::Load,1},
  {Instruction::Store,1},
  {Instruction::GetElementPtr,1.185},
  {Instruction::Alloca,1},
  {Instruction::PHI,0.87},
  {Instruction::Call,1},
  {Instruction::Invoke,1},
  {Instruction::Switch,0.23},
  {Instruction::Ret,1},
  {Instruction::FNeg,0.92},
  {Instruction::FAdd,53.57}, //HLS
  {Instruction::FSub,53.57}, //HLS
  {Instruction::FMul,44.36}, //HLS
  {Instruction::FDiv,185}, //HLS
  {Instruction::FRem,725.76}, //HLS more than 1 cycle (35)
  {Instruction::ShuffleVector,1},
  {Instruction::ExtractElement,1},
  {Instruction::InsertElement,1},
  {Instruction::InsertValue,1}
};

// Weight model taken from Region Seeker by Gioergios et al. in uM^2
static map<unsigned,int> area = {
  {Instruction::Br,0},
  {Instruction::Add,39}, //HLS
  {Instruction::Sub,39}, //HLS
  {Instruction::Mul,1042},//HLS
  {Instruction::UDiv,6090}, // HLS more than 1 cycle (35)
  {Instruction::SDiv,6090},//HLS more than 1 cvcle (35)
  {Instruction::URem,6090}, // HLS more than 1 cycle (35)
  {Instruction::SRem,6090}, //HLS more than 1 cycle (35)
  {Instruction::Shl,85},//HLS
  {Instruction::LShr,85}, //HLS
  {Instruction::AShr,85}, // HLS
  {Instruction::And,32}, // HLS
  {Instruction::Or,32},//HLS
  {Instruction::Xor,32}, //HLS
  {Instruction::Select,32}, //HLS
  {Instruction::ICmp,18}, // HLS
  {Instruction::FCmp,68}, //HLS
  {Instruction::ZExt,0},//HLS
  {Instruction::SExt,1},
  {Instruction::FPToUI,558},//HLS
  {Instruction::FPToSI,629},//HLS
  {Instruction::FPExt,76},
  {Instruction::PtrToInt,1},
  {Instruction::IntToPtr,1},
  {Instruction::SIToFP,629},
  {Instruction::UIToFP,558},
  {Instruction::Trunc,0},//HLS
  {Instruction::FPTrunc,115},
  {Instruction::BitCast,0},
  {Instruction::Load,0},
  {Instruction::Store,0},
  {Instruction::GetElementPtr,15}, //HLS just vector indexing
  {Instruction::Alloca,1},
  {Instruction::PHI,1},
  {Instruction::Call,1},
  {Instruction::Invoke,1},
  {Instruction::Switch,1},
  {Instruction::Ret,1},
  {Instruction::FNeg,40},
  {Instruction::FAdd,375}, //HLS
  {Instruction::FSub,375}, //HLS
  {Instruction::FMul,678}, //HLS
  {Instruction::FDiv,3608},
  {Instruction::FRem,10716}, // HLS more than 1 cycle (2)
  {Instruction::ShuffleVector,1},
  {Instruction::ExtractElement,1},
  {Instruction::InsertElement,1},
  {Instruction::InsertValue,1}
};

// Energy model In nJ callibrated with Power8/9 measurements
static map<unsigned,float> energy = { 
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
  {Instruction::Invoke,1},
  {Instruction::Switch,0.5},
  {Instruction::Ret,0.5},
  {Instruction::FNeg,25},
  {Instruction::FAdd,50.4},
  {Instruction::FSub,50.4},
  {Instruction::FMul,50.4},
  {Instruction::FDiv,968.4},
  {Instruction::FRem,1069.2},
  {Instruction::ShuffleVector,968.4},
  {Instruction::ExtractElement,1069.2},
  {Instruction::InsertElement,50},
  {Instruction::InsertValue,40.6}
};

// Static power in nwatts not callibrated
static map<unsigned,float> powersta = { 
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
  {Instruction::Invoke,0},
  {Instruction::Switch,0.2},
  {Instruction::Ret,0},
  {Instruction::FNeg,0.07},
  {Instruction::FAdd,0.15},
  {Instruction::FSub,0.15},
  {Instruction::FMul,0.4},
  {Instruction::FDiv,0.7},
  {Instruction::FRem,0.8},
  {Instruction::ShuffleVector,0.7},
  {Instruction::ExtractElement,0.8},
  {Instruction::InsertElement,0.7},
  {Instruction::InsertValue,0.2}
};


/**
 * Get the delay in seconds 
 *
 * @param I Instruction to get the delay from
 */
static double getSwDelay(Instruction *I){
  double ret = 0; // default value
  if(swdelay.count(I->getOpcode()))
    ret = (float)swdelay[I->getOpcode()]/(float)CORE_FREQ; // from ns to seconds
  else
    errs() << "Missing Delay Value (" << I->getOpcodeName() << ")\n";
  return ret;
}

/**
 * Get the delay in seconds 
 *
 * @param I Instruction to get the delay from
 */
static double getHwDelay(Instruction *I){
  double ret = 0; // default value
  if(hwdelay.count(I->getOpcode()))
    ret = hwdelay[I->getOpcode()]/1000000000; // from ns to seconds
  else
    errs() << "Missing Delay Value (" << I->getOpcodeName() << ")\n";
  return ret;
}

/**
 * Get the dynamic energy consumption in nanoJoules
 *
 * @param I Instruction to get the energy from
 */
static float getEnergyDyn(Instruction *I){
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
static float getPowerSta(Instruction *I){
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
static int getArea(Instruction *I){
  int ret = 0; // default value
  if(area.count(I->getOpcode()))
    ret = area[I->getOpcode()];
  else
    errs() << "Missing Area Value (" << I->getOpcodeName() << ")\n";
  return ret;
}
