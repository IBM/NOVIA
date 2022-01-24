#include <iostream>

using namespace std;

int main(int argn, char **argc){
	if( argn < 2){
		cout << "Missing Arguments\n";
		return 1;
	}

	int a = atoi(argc[1]) ;
  int b;
	if (a%2) {
    int b = a*3;
    if ((a-1)%2){
      a += 2;
      b -= 4;
      int c = a + b;
    }
    else{
      a -= 2;
    }
	}
  else{
    b = a*4;
  }
	
  cout << "Result: " << a*b << endl;
	return 0;
}
