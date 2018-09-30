/*
 * WordMesh.h --
 *	Word Meshes (a simple type of word lattice with transitions between
 *	any two adjacent words).
 *
 * Copyright (c) 1998 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/WordMesh.h,v 1.3 1998/02/21 00:31:24 stolcke Exp $
 *
 */

#ifndef _WordMesh_h_
#define _WordMesh_h_

#include "MultiAlign.h"

#include "Array.h"
#include "LHash.h"

class WordMesh: public MultiAlign
{
public:
    WordMesh(Vocab &vocab);
    ~WordMesh();

    Boolean read(File &file);
    Boolean write(File &file);

    void addWords(const VocabIndex *words, Prob score)
	{ alignWords(words, score); };
    void alignWords(const VocabIndex *words, Prob score, Prob *wordScores = 0);

    unsigned wordError(const VocabIndex *words,
				unsigned &sub, unsigned &ins, unsigned &del);

    double minimizeWordError(VocabIndex *words, unsigned length,
				double &sub, double &ins, double &del,
				unsigned flags = 0);

    Boolean isEmpty();
    
private:
    Array< LHash<VocabIndex,Prob>* > aligns;	// alignment columns
    unsigned numAligns;			// number of alignment columns
    Array<unsigned> sortedAligns;	// topoligical order of alignment

    VocabIndex deleteIndex;		// pseudo-word representing deletions
    Prob totalPosterior;		// accumulated sample scores
};

#endif /* _WordMesh_h_ */

