#include<string>
#include<map>
#include<iostream>
#include <fstream>

#include "types/FusedBB.hpp"

#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instruction.h"

using namespace llvm;
using namespace std;


float getWeight(vector<BasicBlock*>*,vector<vector<float>*>*,vector<float> *, FusedBB*,
    map<string,double>*);
float getMerit(vector<BasicBlock*>*,vector<vector<float>*>*,vector<float> *, FusedBB*,
    map<string,double>*);
float getSavedArea(vector<BasicBlock*>*,vector<vector<float>*>*,vector<float> *, FusedBB*,
    map<string,double>*);
void readDynInfo(string, map<string,double>*);
