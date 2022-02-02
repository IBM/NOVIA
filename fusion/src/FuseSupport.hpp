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
#include<string>
#include<map>
#include<iostream>
#include <fstream>

#include "types/FusedBB.hpp"

#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instruction.h"

using namespace llvm;
using namespace std;


float getTseq(vector<BasicBlock*>*, vector<vector<double>*>*,
    vector<double> *, FusedBB *, map<string,double> *,
    map<string,long> *, set<string> *);
pair<float,float> getSubgraphMetrics(vector<BasicBlock*> *bblist, vector<vector<double>*> *,
    vector<double> *, FusedBB *, map<string,double> *, map<string,long> *,
    vector<list<Instruction*>*> *,vector<vector<float>*>*, vector<float>*,
    vector<map<BasicBlock*,float> *> *, vector<pair<float,float> >*);
float getSpeedUp(vector<BasicBlock*>*,vector<vector<double>*>*,vector<double> *, FusedBB*,
    map<string,double>*,map<string,long>*);
float getWeight(vector<BasicBlock*>*,vector<vector<double>*>*,vector<double> *, FusedBB*,
    map<string,double>*,map<string,long>*);
float getMerit(vector<BasicBlock*>*,vector<vector<double>*>*,vector<double> *, FusedBB*,
    map<string,double>*,map<string,long>*);
float getSavedArea(vector<BasicBlock*>*,vector<vector<double>*>*,vector<double> *, FusedBB*,
    map<string,double>*,map<string,long>*);
float getRelativeSavedArea(vector<BasicBlock*>*,vector<vector<double>*>*,vector<double> *, FusedBB*,
    map<string,double>*,map<string,long>*);
void readDynInfo(string, map<string,double>*, map<string,double>*, map<string,long>*);
pair<float,float> BinPacking(vector<pair<FusedBB*,pair<float,float> > >::iterator, 
    vector<pair<FusedBB*,pair<float,float> > >::iterator, float, float, float);
pair<float,float> BinPacking2(vector<pair<FusedBB*,pair<float,float> > >::iterator, 
    vector<pair<FusedBB*,pair<float,float> > >::iterator, float, float, 
    vector<float>*,vector<pair<float,float> >*);

bool mysort(pair<FusedBB*,pair<float,float> >&, pair<FusedBB*,pair<float,float> >&);
