#include <iostream>

using namespace std;

int main(int argn, char **argc){
	if( argn < 3){
		cout << "Missing Arguments\n";
		return 1;
	}

	int a = atoi(argc[1]) ;
  char b  = *argc[2];
	if (a%2) {
		a += 1;
	}
	else {
		b = 'a';
	}
	cout << "Result: " << a << " " << b << endl;
	return 0;
}
