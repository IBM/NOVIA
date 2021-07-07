#include "FuseSupport.hpp"

pair<float,float> getSubgraphMetrics(vector<BasicBlock*> *bblist, vector<vector<double>*> *prebb,
    vector<double> *fused, FusedBB *candidate, map<string,double> *profileMap,
    map<string,long> *iterMap,vector<list<Instruction*>*> *subgraphs,
    vector<vector<float>*> *data,vector<float>*tseq_sub, vector<
    map<BasicBlock*,float>*> *tseqblock,vector<pair<float,float> >*areas){

  int count = 0;
  float acum_sub_area = 0;
  float acum_orig_area = 0;
  for(auto *v : *subgraphs){
    float sub_area = 0;
    float sub_cp = 0;
    float sub_tseq = 0;
    float orig_area = 0;
    int num_merges = 0;
    set<Instruction*> subset;
    map<Instruction*,float> visited;
    data->push_back(new vector<float>);

    tseqblock->push_back(new map<BasicBlock*,float>);
    float tseq = candidate->getTseqSubgraph(v,iterMap,(*tseqblock)[tseqblock->size()-1]);
    tseq_sub->push_back(tseq);

    for(auto *I : *v)
      subset.insert(I);

    for(auto *I : *v){
      //if(!(isa<SelectInst>(I) and !subset.count((Instruction*)(I->getOperand(1))) 
      //    and !subset.count((Instruction*)(I->getOperand(2))))){
        sub_area += getArea(I);
        float imerges = candidate->getNumMerges(I);
        if(!isa<SelectInst>(I))
          orig_area += imerges?imerges*getArea(I):getArea(I);
        num_merges += imerges?1:0;
        sub_tseq += getSwDelay(I);
      //}
      //sub_cp = dfsCritSub(I,*v,&visited);
    }
    (*data)[data->size()-1]->push_back(sub_area/orig_area);
    areas->push_back(pair<float,float>(sub_area,orig_area));
    acum_sub_area += sub_area;
    acum_orig_area += orig_area;
    //errs() << "Subgraph " << count << ": " << num_merges << " " << sub_area/orig_area << "\n";
    count++;
  }
  return pair<float,float>(acum_sub_area,acum_orig_area);
}

float getTseq(vector<BasicBlock*> *bblist, vector<vector<double>*> *prebb,
    vector<double> *fused, FusedBB *candidate, map<string,double> *profileMap,
    map<string,long> *iterMap, set<string> *subset){
  float total_tseq = 0;
  float merged_iterations = 0;
  float merged_weight = 0;
  float merged_tseq = 0;
  float unmerged_tseq =0;
  float unmerged_weight = 0;
  float total_weight = 0;
  for(int i =0; i<prebb->size(); ++i){
    //if(subset->count((*bblist)[i]->getName().str())){
      long iterations = (*iterMap)[(*bblist)[i]->getName().str()];
      float weight = (*profileMap)[(*bblist)[i]->getName().str()];
      if(candidate->isMergedBB((*bblist)[i])){
        merged_iterations += iterations;
        merged_weight += weight;
        merged_tseq += (*(*prebb)[i])[8] * iterations;
      }
      else{
        unmerged_tseq += (*(*prebb)[i])[8] * iterations;
        unmerged_weight += weight;
      }
      total_tseq += (*(*prebb)[i])[8] * iterations;
      total_weight += (*profileMap)[(*bblist)[i]->getName().str()];  
    //}
  }
  return merged_tseq;
  
}

float getSpeedUp(vector<BasicBlock*> *bblist, vector<vector<double>*> *prebb,
    vector<double> *fused, FusedBB *candidate, map<string,double> *profileMap,
    map<string,long> *iterMap){
  float cp = (*fused)[11];
  float speedup = 0;
  float unmerged_tseq = 0;
  float total_tseq = 0;
  float merged_tseq = 0;
  float weight = 0;
  float merged_weight = 0;
  float unmerged_weight = 0;
  float total_weight = 0;
  long merged_iterations = 0;
  for(int i =0; i<prebb->size(); ++i){
    long iterations = (*iterMap)[(*bblist)[i]->getName().str()];
    float weight = (*profileMap)[(*bblist)[i]->getName().str()];
    if(candidate->isMergedBB((*bblist)[i])){
      merged_iterations += iterations;
      merged_weight += weight;
      merged_tseq += (*(*prebb)[i])[8] * iterations;
    }
    else{
      unmerged_tseq += (*(*prebb)[i])[8] * iterations;
      unmerged_weight += weight;
    }
    total_tseq += (*(*prebb)[i])[8] * iterations;
    total_weight += (*profileMap)[(*bblist)[i]->getName().str()];
  }
  float non_accel = total_tseq/weight;
  //speedup = non_accel/(non_accel*(1-weight)+unmerged_tseq+merged_iterations*cp);
  speedup = 1/((1-merged_weight)+merged_weight/(merged_tseq/(merged_iterations*cp)));
  return speedup;
}

float getWeight(vector<BasicBlock*> *bblist, vector<vector<double>*> *prebb,
    vector<double> *fused, FusedBB *candidate, map<string,double> *profileMap,
    map<string,long> *iterMap){
  float merged_weight = 0;
  for(int i = 0; i < prebb->size() ; ++i){
    if(candidate->isMergedBB((*bblist)[i]))
      merged_weight += (*profileMap)[(*bblist)[i]->getName().str()];
  }
  return merged_weight;
}

float getMerit(vector<BasicBlock*> *bblist,vector<vector<double>*> *prebb,
    vector<double> *fused, FusedBB *candidate,map<string,double> *profileMap,
    map<string,long> *iterMap){
  float merit = 0;
  float cp = (*fused)[11];
  float merged_tseq = 0;
  float total_tseq = 0;
  float merged_weight = 0;
  long iter = 0;
  long merged_iter = 0;
  float weight = 1;
  for(int i =0 ; i<prebb->size(); ++i){
    if(!profileMap->empty()){
      if(!profileMap->count((*bblist)[i]->getName().str())){
        errs() << (*bblist)[i]->getName();
        assert(0 && "Did not find weights for this bb");

      }
      weight = (*profileMap)[(*bblist)[i]->getName().str()];
      iter = (*iterMap)[(*bblist)[i]->getName().str()];
    }
    if(candidate->isMergedBB((*bblist)[i])){
      merged_tseq += (*(*prebb)[i])[8] * iter;
      merged_iter += iter;
    }
  }
  if(!profileMap->empty())
    merit = merged_tseq - merged_iter*cp;
  else
    merit = total_tseq - (merged_tseq + candidate->size()*cp);

  return merit;

}

float getSavedArea(vector<BasicBlock*> *bblist,vector<vector<double>*> *prebb,
    vector<double> *fused, FusedBB *candidate,map<string,double> *profileMap,
    map<string,long> *iterMap){
  float tarea = 0;
  for(int i = 0; i < prebb->size(); ++i){
    if(candidate->isMergedBB((*bblist)[i])){
      tarea += (*(*prebb)[i])[12];
    }
  }
  return tarea - (*fused)[12];
}

float getRelativeSavedArea(vector<BasicBlock*> *bblist,vector<vector<double>*> *prebb,
    vector<double> *fused, FusedBB *candidate,map<string,double> *profileMap,
    map<string,long> *iterMap){
  float tarea = 0;
  for(int i = 0; i < prebb->size(); ++i){
    if(candidate->isMergedBB((*bblist)[i])){
      tarea += (*(*prebb)[i])[12];
    }
  }
  return ((*fused)[12]/tarea);
}

void readDynInfo(string filename, map<string,double>* profileMap,
    map<string,long>* iterMap){
  fstream file;

  if(filename.empty()){
    errs() << "No dynamic info specified - assuming equal weights\n";
    return;
  }
  else
    file.open(filename);

  string bb_name;
  double sigvalue = 0;
  long itervalue = 0;
	while(file >> bb_name >> sigvalue >> itervalue){
		(*profileMap)[bb_name] = sigvalue;
    (*iterMap)[bb_name] = itervalue;
  }
}

bool mysort(pair<FusedBB*,pair<float,float> >& it1, 
    pair<FusedBB*,pair<float,float> >& it2){
  return it1.second.first < it2.second.first;
}

pair<float,float> BinPacking(vector<pair<FusedBB*,pair<float,float> > >::iterator it, 
    vector<pair<FusedBB*,pair<float,float> > >::iterator end, float area, float speedup,
    float area_limit){
  if ( it == end or area > area_limit)
    return pair<float,float>(area,speedup);
  pair<float,float> with, without;
  with.first = with.second = without.first = without.second = 0;  
  without = BinPacking(next(it),end,area,speedup,area_limit);
  if(it->second.first + area <= area_limit)
    with = BinPacking(next(it),end,area+it->second.first,
        1/(1-((1-1/speedup)+(1-1/it->second.second))),area_limit);
  if(with.second > without.second){
    return with;
  }
  else{
    return without;
  }

}
