#include "FuseSupport.hpp"


void readDynInfo(string filename, map<string,float> *profileMap){
  fstream file;

  if(filename.empty()){
    errs() << "No dynamic info specified - assuming equal weights\n";
    return;
  }
  else
    file.open(filename);

  string bb_name;
  float sigvalue = 0;
	while(file >> bb_name >> sigvalue){
		(*profileMap)[bb_name] = sigvalue;
    errs() << bb_name << " " << sigvalue;
  }
}


