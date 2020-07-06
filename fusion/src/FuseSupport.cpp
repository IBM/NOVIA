#include "FuseSupport.hpp"

float getWeight(vector<BasicBlock*> *bblist, vector<vector<float>*> *prebb,
    vector<float> *fused, FusedBB *candidate, map<string,double> *profileMap){
  float merged_weight = 0;
  for(int i = 0; i < prebb->size() ; ++i){
    if(candidate->isMergedBB((*bblist)[i]))
      merged_weight = (*profileMap)[(*bblist)[i]->getName().str()];
  }
  return merged_weight;
}

float getMerit(vector<BasicBlock*> *bblist,vector<vector<float>*> *prebb,
    vector<float> *fused, FusedBB *candidate,map<string,double> *profileMap){
  float merit = 0;
  float cp = (*fused)[11];
  float merged_tseq = 0;
  float total_tseq = 0;
  float merged_weight = 0;
  for(int i =0 ; i<prebb->size(); ++i){
    float weight = 1;
    if(!profileMap->empty()){
      if(!profileMap->count((*bblist)[i]->getName().str())){
        errs() << (*bblist)[i]->getName();
        assert(0 && "Did not find weights for this bb");

      }
      weight = (*profileMap)[(*bblist)[i]->getName().str()];
    }
    if(!candidate->isMergedBB((*bblist)[i])){
      merged_tseq += (*(*prebb)[i])[8] * weight;
    }
    else{
      merged_weight += weight;
    }
    total_tseq += (*(*prebb)[i])[8] * weight;
  }
  if(!profileMap->empty())
    merit = total_tseq - (merged_tseq + merged_weight*cp);
  else
    merit = total_tseq - (merged_tseq + candidate->size()*cp);

  return merit;

}


void readDynInfo(string filename, map<string,double> *profileMap){
  fstream file;

  if(filename.empty()){
    errs() << "No dynamic info specified - assuming equal weights\n";
    return;
  }
  else
    file.open(filename);

  string bb_name;
  double sigvalue = 0;
	while(file >> bb_name >> sigvalue){
		(*profileMap)[bb_name] = sigvalue;
    errs() << bb_name << " " << sigvalue;
  }
}


