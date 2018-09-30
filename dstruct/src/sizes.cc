
#include <stdio.h>

typedef struct foo {
	int x;
	short y;
} foo;

typedef struct bar {
	short y;
	foo x;
} bar;

main() 
{
	bar b;

	printf("sizeof struct foo = %d, bar = %d\n", sizeof(foo),
		sizeof(bar));

	b.x.x = 1;
	b.y = 2;
}
