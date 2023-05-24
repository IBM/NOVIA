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
#include <iostream>
#include <fstream>

std::ofstream *outFile;

extern "C" void init_noviaMemAnalysis(){
  std::ofstream *tmp = new std::ofstream;
  tmp->open("memDep.txt");
  outFile = tmp;
  return;
}

extern "C" void novia_regGraph(bool *depMatrix, long *addrArray, char *sizeArray,
    int size){

  for(int i = 0; i < size; ++i){
    (*outFile) << (unsigned long)addrArray[i] << "(" << (int)sizeArray[i] << ")" << ":";
    for(int j = 0; j < size; ++j){
      int index = i*size+j;
      if(i != j and depMatrix[i*size+j])
        (*outFile) << (unsigned long)addrArray[j] << "(" << (int)sizeArray[j] << "),";
    }
    (*outFile) << std::endl;
  }
  (*outFile) << "##########\n";

  return;
}

/*extern "C" void novia_decFunc(int *, int size){
  return;
}*/
