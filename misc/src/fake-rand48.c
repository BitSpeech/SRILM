/*
 * fake-rand48.c --
 *	Fake the *rand48 functions based on srand() and rand()
 *	(for systems that don't have them)
 *
 * $Header: /home/srilm/devel/misc/src/RCS/fake-rand48.c,v 1.2 2005/07/29 03:23:57 stolcke Exp $
 */

#ifdef NEED_RAND48

#include <stdlib.h>

void srand48(long seed)
{
    srand((int)seed);
}

long lrand48()
{
    return rand()*(RAND_MAX + 1) + rand();
}

double drand48()
{
    return (double)rand()/RAND_MAX;
}

#endif /* NEED_RAND48 */
