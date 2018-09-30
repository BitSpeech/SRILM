
#include <stdio.h>
#include <Trie.cc>


#if defined(__SUNPRO_CC)
# pragma pack(2)
#endif
class foo {
public:
#if defined(__INTEL_COMPILER) || defined(__GNUC__)
	int x __attribute__ ((packed));
#else
	int x;
#endif
	short y;
};
#if defined(__SUNPRO_CC)
# pragma pack()
#endif

class bar {
public:
	foo x;
	short y;
};



int main() 
{
	bar b;

	printf("sizeof class foo = %d, bar = %d\n", sizeof(foo),
		sizeof(bar));

	printf("sizeof Trie<short,short> = %d\n", sizeof(Trie<short,short>));
	printf("sizeof Trie<int,int> = %d\n", sizeof(Trie<int,int>));
	printf("sizeof Trie<short,double> = %d\n", sizeof(Trie<short,double>));

	b.x.x = 1;
	b.y = 2;
}
