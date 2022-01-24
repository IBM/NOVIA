#include <iostream>
#include <cstring>

using namespace std;

int main(int argn, char **argc){
/*	if( argn < 2){
		cout << "Missing Argument\n";
		return 1;
	}*/

	char a[64],b[64],c[64];
  memset(a,1,sizeof a);
  memset(b,2,sizeof b);
  memset(c,3,sizeof c);
  

  for(int i = 0; i < 64; ++i)
    a[i] += b[i]*c[i];

  for(int i = 0; i < 64; ++i)
    a[i] += b[i]*2.0*c[i];

  for(int i = 0; i < 64; ++i)
    a[i] += b[i]/c[i];

  for(int i = 0; i < 64; ++i)
    c[i] = a[i]-b[i];

  cout << "Result: " << endl;
  for(int i = 0; i < 64; ++i)
    cout << to_string(a[i]) << " ";
  for(int i = 0; i < 64; ++i)
    cout << to_string(c[i]) << " ";
  cout << endl;

	return 0;
}
