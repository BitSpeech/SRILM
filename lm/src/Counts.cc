
/*
 * Counts.cc --
 *	Functions for handling counts
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 2006 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/Counts.cc,v 1.5 2006/11/17 11:33:55 stolcke Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>
#if !defined(_MSC_VER) && !defined(WIN32)
#include <sys/param.h>
#endif

#include "Boolean.h"
#include "Counts.h"

/*
 * Byte swapping
 */

static inline
Boolean isLittleEndian()
{
    static Boolean haveEndianness = false;
    static Boolean endianIsLittle;

    if (!haveEndianness) {
	union {
		char c[2];
		short i;
	} data;

	data.i = 1;

	endianIsLittle = (data.c[0] != 0);
	haveEndianness = true;
    }

    return endianIsLittle;
}

static inline
void byteSwap(void *data, unsigned size)
{
    if (isLittleEndian()) {
	assert(size <= sizeof(double));

	char byteArray[sizeof(double)];

	for (unsigned i = 0; i < size; i ++) {
	    byteArray[i] = ((char *)data)[size - i - 1];
	}
	memcpy(data, byteArray, size);
    }
}

/*
 * Integer I/O
 */

#ifndef NBBY
#define NBBY 8
#endif

const unsigned short isLongBit = (unsigned short)1 << (sizeof(short)*NBBY-1);
const unsigned int isLongLongBit = (unsigned int)1 << (sizeof(int)*NBBY-2);
const unsigned long long isTooLongBit =
			(unsigned long long)1 << (sizeof(long long)*NBBY-2);

unsigned
writeBinaryCount(FILE *fp, unsigned long long count, unsigned minBytes)
{
    if (minBytes <= sizeof(short) && count < isLongBit) {
	short int scount = count;

	byteSwap(&scount, sizeof(scount));
	fwrite(&scount, sizeof(scount), 1, fp);
	return sizeof(scount);
    } else if (minBytes <= sizeof(int) && count < isLongLongBit) {
	unsigned int lcount = count;
	lcount |= (unsigned int)isLongBit << (sizeof(short)*NBBY);

	byteSwap(&lcount, sizeof(lcount));
	fwrite(&lcount, sizeof(lcount), 1, fp);
	return sizeof(lcount);
    } else if (count < isTooLongBit) {
	unsigned int lcount = (count >> sizeof(int)*NBBY) |
			      ((unsigned int)isLongBit << (sizeof(short)*NBBY))|
			      isLongLongBit;

	byteSwap(&lcount, sizeof(lcount));
	fwrite(&lcount, sizeof(lcount), 1, fp);

	lcount = (unsigned int)count;
	byteSwap(&lcount, sizeof(lcount));
	fwrite(&lcount, sizeof(lcount), 1, fp);
	return 2*sizeof(lcount);
    } else {
	cerr << "writeBinaryCount: count " << count << " is too large\n";
	return 0;
    }
}

unsigned
readBinaryCount(FILE *fp, unsigned long long &count)
{
    unsigned short scount;

    if (fread(&scount, sizeof(scount), 1, fp) != 1) {
	return 0;
    } else {
	byteSwap(&scount, sizeof(scount));

	if (!(scount & isLongBit)) {
	    // short count
	    count = scount;
	    return sizeof(scount);
	} else {
	    // long or long long count
	    unsigned int lcount = (unsigned)(scount & ~isLongBit)
						<< sizeof(short)*NBBY;

	    // read second half of long count
	    if (fread(&scount, sizeof(scount), 1, fp) != 1) {
		cerr << "readBinaryCount: incomplete long count\n";
		return 0;
	    } else {
		byteSwap(&scount, sizeof(scount));
		

		// assemble long count from two shorts
		lcount |= scount;

		if (!(lcount & isLongLongBit)) {
		    // long count
		    count = lcount;
		    return sizeof(lcount);
		} else {
		    // long long count
		    count = (unsigned long long)(lcount & ~isLongLongBit)
						<< sizeof(unsigned)*NBBY;

		    // read second half of long count
		    if (fread(&lcount, sizeof(lcount), 1, fp) != 1) {
			cerr << "readBinaryCount: incomplete long long count\n";
			return 0;
		    } else {
			byteSwap(&lcount, sizeof(lcount));

			// assemble long long count from two longs
			count |= lcount;
			return 2*sizeof(lcount);
		    }
		}
	    }
	}
    }
}


/*
 * Floating point I/O
 */

unsigned
writeBinaryCount(FILE *fp, float count)
{
    byteSwap(&count, sizeof(count));

    if (fwrite(&count, sizeof(count), 1, fp) == 1) {
	return sizeof(count);
    } else {
	return 0;
    }
}

unsigned
writeBinaryCount(FILE *fp, double count)
{
    byteSwap(&count, sizeof(count));

    if (fwrite(&count, sizeof(count), 1, fp) == 1) {
	return sizeof(count);
    } else {
	return 0;
    }
}


unsigned
readBinaryCount(FILE *fp, float &count)
{
    if (fread(&count, sizeof(count), 1, fp) == 1) {
	byteSwap(&count, sizeof(count));
	return sizeof(count);
    } else {
	return 0;
    }
}

unsigned
readBinaryCount(FILE *fp, double &count)
{
    if (fread(&count, sizeof(count), 1, fp) == 1) {
	byteSwap(&count, sizeof(count));
	return sizeof(count);
    } else {
	return 0;
    }
}

