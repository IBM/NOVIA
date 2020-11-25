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
void readDynInfo(string, map<string,double>*, map<string,long>*);
