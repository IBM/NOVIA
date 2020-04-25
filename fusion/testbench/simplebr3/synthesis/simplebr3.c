#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int func(int a ){
  if (a%3 == 1){
		a *= 1;
	}
	else if (a%3 ==2) {
		a *= 2;
	}
	else if(a%3 == 0){
		a *= 3;
	}
 return a; 
}

int main(){
  int a = 2;
  int b = func(a);
	printf("Result: %d",b);
	return 0;
}
