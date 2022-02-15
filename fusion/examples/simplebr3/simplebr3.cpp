#include <iostream>

using namespace std;

int main(int argn, char **argc){
	if( argn < 2){
		cout << "Missing Argument\n";
		return 1;
	}

	int a = atoi(argc[1]) ;
	if (a%3 == 1){
		a *= 1;
	}
	else if (a%3 ==2) {
		a *= 2;
	}
	else if(a%3 == 0){
		a *= 3;
	}
	cout << "Result: " << a << endl;
	return 0;
}
