#include <unistd.h>
#include <stdio.h>

int fibonacci(int x)
{
	if(x == 0)
		return 0;

	if(x == 1)
		return 1;

	return fibonacci(x-1) + fibonacci(x-2);
}

void step_next(int x, int y, int z)
{
	int a = x;
	int b = y+z+x;
	int c = z*10/30+100;
}

int main()
{
	step_next(100, 200, 300);
	fibonacci(10);
	return 0;
}
