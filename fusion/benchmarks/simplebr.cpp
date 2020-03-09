#include <iostream>

using namespace std;

int main(int argn, char **argc){
	if( argn < 2){
		cout << "Missing Argument\n";
		return 1;
	}

	int a = atoi(argc[1]) ;
	if (a%2) {
		a += 1;
	}
	else {
		a += 2;
	}
	cout << "Result: " << a << endl;
	return 0;
}
