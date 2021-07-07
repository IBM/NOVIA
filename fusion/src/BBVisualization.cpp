#include "BBVisualization.hpp"

// https://www.codespeedy.com/convert-rgb-to-hex-color-code-in-cpp/
string rgb2hex(int r, int g, int b, bool with_head) {
  stringstream ss; 
  if (with_head) 
    ss << "#"; 
  ss << std::hex << std::setfill('0') << std::setw(6) << (r << 16 | g << 8 | b ); 
  return ss.str();
}


void drawBBGraph(BasicBlock *BB,char *file,string dir){
  struct stat buffer;

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

  map<Value*,Agnode_t*> visited;
  map<string,int> names;
  string name;

  // Add Nodes
  bool first = true;
  for(auto &I: *BB){
    if(&I != BB->getTerminator()){
      name = string(I.getOpcodeName());
      if(names.count(name)){
        names[name]++;
        name = name+to_string(names[name]);
      }
      else{
        names[name] = 0;
      }
  
      n = agnode(G,(char*)name.c_str(),1);
      visited.insert(pair<Value*,Agnode_t*>(&I,n));
      agsafeset(n,(char*)"shape",(char*)"box",(char*)"");
      //agsafeset(n,"fillcolor","skyblue","");
      agsafeset(n,(char*)"fontsize",(char*)"24",(char*)"24");
      agsafeset(n,(char*)"fontname",(char*)"helvetica::bold",(char*)"helvetica::bold");
      
      if(isa<StoreInst>(I) or isa<LoadInst>(I)){
        agsafeset(n,(char*)"style",(char*)"filled",(char*)"");
        agsafeset(n,(char*)"fillcolor",(char*)"red",(char*)"white");
        agsafeset(n,(char*)"fontcolor",(char*)"white",(char*)"black");
      }
    }
  }

  // Add Edges
  for( auto &I : *BB ){
    if(&I != BB->getTerminator()){
      for(int i = 0; i < I.getNumOperands();++i){
        Value *op = dyn_cast<Value>(I.getOperand(i));
        ConstantInt *cint = dyn_cast<ConstantInt>(I.getOperand(i));
        ConstantFP *cfp = dyn_cast<ConstantFP>(I.getOperand(i));
        if(cint){
          m = agnode(G,(char*)to_string(cint->getSExtValue()).c_str(),1);
        }
        else if(cfp){
          double valued = cfp->getType()->isDoubleTy()? cfp->getValue().convertToDouble():
            cfp->getValue().convertToFloat();
          m = agnode(G,(char*)to_string(valued).c_str(),1);
        }
        else{
          if(visited.count(op)){
            m = visited[op];
          }
          else{
            name = string(I.getOperand(i)->getName());
            if(name == "")
              name = "inVal";
            if( name.find("fuse.sel.arg") == string::npos){
              if(names.count(name)){
                names[name]++;
                name = name+to_string(names[name]);
              }
              else{
                names[name] = 0;
              }
              m = agnode(G,(char*)name.c_str(),1);
              agsafeset(m,(char*)"shape",(char*)"rarrow",(char*)"");
              agsafeset(m,(char*)"style",(char*)"filled",(char*)"");
              agsafeset(m,(char*)"color",(char*)"red",(char*)"");
              agsafeset(m,(char*)"fillcolor",(char*)"#b19cd9",(char*)"white");
              agsafeset(m,(char*)"fontcolor",(char*)"black",(char*)"black");
              visited[op] = m;
            }
            else{
              m = NULL;
            }
          }
        }
        n = visited[cast<Value>(&I)];
        if(m)
          agedge(G,m,n,NULL,1);
      }  
      // Get Live Outs
      bool liveout = false;
      Value *Uptr = NULL;
      for(auto *U : I.users()){
        Uptr = (Value*)U;
        liveout = ((Instruction*)U)->getParent() != BB;
        if(liveout)
          break;
      }
      if(liveout){
        if(visited.count(Uptr)){
          m = visited[Uptr];
        }
        else{
          name = "outVal";
          if(names.count(name)){
            names[name]++;
            name = name+to_string(names[name]);
          }
          else{
            names[name] = 0;
          }
          m = agnode(G,(char*)name.c_str(),1);
          visited[Uptr] = m;
          agsafeset(m,(char*)"shape",(char*)"larrow",(char*)"");
          agsafeset(m,(char*)"style",(char*)"filled",(char*)"");
          agsafeset(m,(char*)"color",(char*)"red",(char*)"");
          agsafeset(m,(char*)"fillcolor",(char*)"#9932cc",(char*)"white");
          agsafeset(m,(char*)"fontcolor",(char*)"white",(char*)"black");
        }
        agedge(G,n,m,NULL,1);
      }
    }
  }

  gvLayout(gvc,G,"dot");
  gvRenderFilename(gvc,G,"png",string(dir+"/"+string(file)+".png").c_str());
  gvFreeLayout(gvc,G);
  agclose(G);
  gvFreeContext(gvc);
  return;
}

/*
 * Create a dot file symbolizing BB DFG
 *
 * @param BB BasicBlock to Visualize
 */
void drawBBGraph(FusedBB *fBB,char *file,string dir,
    vector<list<Instruction*> *> * subgraphs){
  struct stat buffer;
  BasicBlock *BB = fBB->getBB();

  int max_merges = fBB->getMaxMerges();
  int col_steps = 10;
  int sub_steps = 10;
  if(max_merges)
    col_steps = (205-35)/max_merges;
  if(subgraphs){
    int max_subgraphs = subgraphs->size();
    if(max_subgraphs)
      sub_steps = (205-10)/max_subgraphs;
  }

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

  map<Value*,Agnode_t*> visited;
  map<string,int> names;
  string name;

  // Add Nodes
  bool first = true;
  for(auto &I: *BB){
    if(&I != BB->getTerminator()){
      name = string(I.getOpcodeName());
      if(names.count(name)){
        names[name]++;
        name = name+to_string(names[name]);
      }
      else{
        names[name] = 0;
      }
  
      n = agnode(G,(char*)name.c_str(),1);
      visited.insert(pair<Value*,Agnode_t*>(&I,n));
      agsafeset(n,(char*)"shape",(char*)"box",(char*)"");
      //agsafeset(n,"fillcolor","skyblue","");
      agsafeset(n,(char*)"fontsize",(char*)"24",(char*)"24");
      agsafeset(n,(char*)"fontname",(char*)"helvetica::bold",(char*)"helvetica::bold");
      
      if(subgraphs and !isa<LoadInst>(I) and !isa<StoreInst>(I)){
        bool found = false;
        for(int sub=0; sub<subgraphs->size() and !found;++sub){
          for(auto *It : *(*subgraphs)[sub]){
            if(&I == It){
              string color = rgb2hex(0,0,255-sub*sub_steps,true);
              agsafeset(n,(char*)"style",(char*)"filled",(char*)"");
              agsafeset(n,(char*)"fillcolor",(char*)color.c_str(),(char*)"white");
              agsafeset(n,(char*)"fontcolor",(char*)"white",(char*)"black");
              found = true;
              break;
            }
          }
        }
      }

      if(isa<StoreInst>(I) or isa<LoadInst>(I)){
        agsafeset(n,(char*)"style",(char*)"filled",(char*)"");
        agsafeset(n,(char*)"fillcolor",(char*)"red",(char*)"white");
        agsafeset(n,(char*)"fontcolor",(char*)"white",(char*)"black");
      }
      else if(I.getName().str().find("fuse.sel") != string::npos or
              I.getName().str().find("fuse.sel.safe") != string::npos ){
        agsafeset(n,(char*)"style",(char*)"filled",(char*)"");
        agsafeset(n,(char*)"fillcolor",(char*)"#ffd700",(char*)"white");
        agsafeset(n,(char*)"fontcolor",(char*)"black",(char*)"black");
      }
      else if(fBB->isMergedI(&I)){
        string color = rgb2hex(0,255-col_steps*fBB->getNumMerges(&I),0,true);
  
        agsafeset(n,(char*)"style",(char*)"filled",(char*)"");
        agsafeset(n,(char*)"fillcolor",(char*)color.c_str(),(char*)"white");
        agsafeset(n,(char*)"fontsize",(char*)"26",(char*)"26");
        agsafeset(n,(char*)"fontcolor",(char*)"white",(char*)"black");
      }
  
    }
  }

  // Add Edges
  for( auto &I : *BB ){
    if(&I != BB->getTerminator()){
      for(int i = 0; i < I.getNumOperands();++i){
        Value *op = dyn_cast<Value>(I.getOperand(i));
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
            if(name == "")
              name = "inVal";
            if( name.find("fuse.sel.arg") == string::npos){
              if(names.count(name)){
                names[name]++;
                name = name+to_string(names[name]);
              }
              else{
                names[name] = 0;
              }
              string aux = "inVal"+to_string(names["inVal"]++);
              m = agnode(G,(char*)aux.c_str(),1);
              //m = agnode(G,(char*)name.c_str(),1);
              agsafeset(m,(char*)"shape",(char*)"rarrow",(char*)"");
              agsafeset(m,(char*)"style",(char*)"filled",(char*)"");
              agsafeset(m,(char*)"color",(char*)"red",(char*)"");
              agsafeset(m,(char*)"fillcolor",(char*)"#b19cd9",(char*)"white");
              agsafeset(m,(char*)"fontcolor",(char*)"black",(char*)"black");
              visited[op] = m;
            }
            else{
              m = NULL;
            }
          }
        }
        n = visited[cast<Value>(&I)];
        if(m){
          e = agedge(G,m,n,NULL,1);
          if(I.getName().str().find("fuse.sel") != string::npos or
             I.getName().str().find("fuse.sel.safe") != string::npos ){
            string bbs = fBB->getSelBB(&I,op);
            //agsafeset(e,(char*)"label",(char*)bbs.c_str(),(char*)"");
          }  
        }
      }  
      // Get Live Outs
      bool liveout = false;
      Value *Uptr = NULL;
      for(auto *U : I.users()){
        Uptr = (Value*)U;
        liveout = ((Instruction*)U)->getParent() != BB;
        if(liveout)
          break;
      }
      if(liveout){
        if(visited.count(Uptr)){
          m = visited[Uptr];
        }
        else{
          name = "outVal";
          if(names.count(name)){
            names[name]++;
            name = name+to_string(names[name]);
          }
          else{
            names[name] = 0;
          }
          m = agnode(G,(char*)name.c_str(),1);
          visited[Uptr] = m;
          agsafeset(m,(char*)"shape",(char*)"larrow",(char*)"");
          agsafeset(m,(char*)"style",(char*)"filled",(char*)"");
          agsafeset(m,(char*)"color",(char*)"red",(char*)"");
          agsafeset(m,(char*)"fillcolor",(char*)"#9932cc",(char*)"white");
          agsafeset(m,(char*)"fontcolor",(char*)"white",(char*)"black");
        }
        agedge(G,n,m,NULL,1);
      }
    }
  }

  gvLayout(gvc,G,"dot");
  gvRenderFilename(gvc,G,"png",string(dir+"/"+string(file)+".png").c_str());
  gvFreeLayout(gvc,G);
  agclose(G);
  gvFreeContext(gvc);
  return;
}
