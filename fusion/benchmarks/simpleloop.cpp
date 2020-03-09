#include <iostream>

using namespace std;

int main(int argn, char **argc){
	if( argn < 2){
		cout << "Missing Argument\n";
		return 1;
	}

	int a = atoi(argc[1]) ;
	for(int i=0; i < 100; ++i)
		++a;

	int i = 0;
	while(i < 100){
		++a;
		++i;
	}

	cout << "Result: " << a << endl;
	return 0;
}
