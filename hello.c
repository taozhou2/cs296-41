#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int fact(int n);
int i = 42;

int main()
{
	puts("Hello!");
  printf("fact(5) = %d\n", fact(5));

	puts("World!");

	// char* segfault = NULL;
	// char s = segfault[0];

	return i;
}

int fact(int n) {
  if (n <= 1)
    return 1;
  return n * fact(n-1);
}
