/*
 * testLattice --
 *	Test for Lattice class
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1997, SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lattice/src/RCS/testLattice.cc,v 1.4 2002/03/01 16:31:26 stolcke Exp $";
#endif

#include <stdio.h>

#include "Vocab.h"
#include "Lattice.h"

int
main (int argc, char *argv[])
{
    Vocab vocab;
    Lattice *lat = new Lattice(vocab);
    assert(lat != 0);

    if (argc == 2) {
	File file(argv[1], "r");

	lat->readPFSG(file);
    }

    File f(stdout);
    lat->writePFSG(f);

    delete lat;
    lat = 0;

    exit(0);
}
