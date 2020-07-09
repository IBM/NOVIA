#include "BBVisualization.hpp"

/*
 * Create a dot file symbolizing BB DFG
 *
 * @param BB BasicBlock to Visualize
 */
void drawBBGraph(FusedBB *fBB,char *file,string dir){
  struct stat buffer;
  BasicBlock *BB = fBB->getBB();

  if(dir.empty()){
    errs() << "No visualization directory specified - assuming default\n";
    dir = "imgs";
  }
  if(stat(dir.c_str(),&buffer)){
    errs() << "Visualization directory does not exist - creating it...\n";
    mkdir(dir.c_str(),S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  }

  GVC_t *gvc = gvContext();
  Agraph_t *G = agopen((char*)BB->getName().take_front(10).str().c_str(),
      Agstrictdirected, NULL);

  Agnode_t *n, *m;
  Agedge_t *e;
  Agsym_t *a;

  map<Instruction*,Agnode_t*> visited;
  map<string,int> names;
  string name;

  // Add Nodes
  for(auto &I: *BB){
    name = string(I.getOpcodeName());
    if(names.count(name)){
      names[name]++;
      name = name+to_string(names[name]);
    }
    else{
      names[name] = 0;
    }

    n = agnode(G,(char*)name.c_str(),1);
    visited.insert(pair<Instruction*,Agnode_t*>(&I,n));
    agsafeset(n,"shape","box","");
    //agsafeset(n,"fillcolor","skyblue","");
    agsafeset(n,"fontsize","24","24");

    // If the node is merged change color
    if(fBB->isMergedI(&I)){
      agsafeset(n,"style","filled","");
      agsafeset(n,"fillcolor","purple","");
      agsafeset(n,"fontsize","24","24");
      agsafeset(n,"fontcolor","white","black");
    }
  }

  // Add Edges
  for( auto &I : *BB ){
    for(int i = 0; i < I.getNumOperands();++i){
      Instruction *op = dyn_cast<Instruction>(I.getOperand(i));
      ConstantInt *cint = dyn_cast<ConstantInt>(I.getOperand(i));
      if(cint){
        m = agnode(G,(char*)to_string(cint->getSExtValue()).c_str(),1);
      }
      else{
        if(visited.count(op)){
          m = visited[op];
        }
        else{
          name = string(I.getOperand(i)->getName());
          if(names.count(name)){
            names[name]++;
            name = name+to_string(names[name]);
          }
          else{
            names[name] = 0;
          }
          m = agnode(G,(char*)I.getOperand(i)->getName().str().c_str(),1);
        }
      }
      n = visited[&I];
      agedge(G,m,n,NULL,1);
    }
  }

  gvLayout(gvc,G,"dot");
  gvRenderFilename(gvc,G,"png",string(dir+"/"+string(file)+".png").c_str());
  gvFreeLayout(gvc,G);
  agclose(G);
  gvFreeContext(gvc);
  return;
}
