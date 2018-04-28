#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int fact(int n);
int foo(int x);
int func(int a, int b, int c);
int i = 42;

int main()
{
	puts("Hello!");
//  printf("fact(5) = %d\n", fact(5));
  foo(10);

	// char* segfault = NULL;
	// char s = segfault[0];

	func(10, 20, 30);

	puts("World!");

	return i;
}

int fact(int n) {
  int test = 18298;
  int test2 = 378;
  if (n <= 1)
    return 1;
  return n * fact(n-1);
}

int foo(int x) {
  int j = x;
  j++;
  for(int i = 0; i < x; i++){
    fact(i);
  }
}

int func(int a, int b, int c) {
	int temp = a + b;
	temp -= c;

	temp = (a + b) / c;

	return temp;
}
