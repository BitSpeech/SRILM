/*
 * WordMesh.h --
 *	Word Meshes (a simple type of word lattice with transitions between
 *	any two adjacent words).
 *
 * Copyright (c) 1998 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/WordMesh.h,v 1.7 2000/05/08 06:05:54 stolcke Exp $
 *
 */

#ifndef _WordMesh_h_
#define _WordMesh_h_

#include "MultiAlign.h"

#include "Array.h"
#include "LHash.h"

class WordMeshIter;

class WordMesh: public MultiAlign
{
    friend WordMeshIter;

public:
    WordMesh(Vocab &vocab);
    ~WordMesh();

    Boolean read(File &file);
    Boolean write(File &file);

    void addWords(const VocabIndex *words, Prob score)
	{ alignWords(words, score); };
    void alignWords(const VocabIndex *words, Prob score, Prob *wordScores = 0)
	{ alignWords(words, score, wordScores, 0); };
    void alignWords(const VocabIndex *words, Prob score,
				    Prob *wordScores, const int *hypID);

    unsigned wordError(const VocabIndex *words,
				unsigned &sub, unsigned &ins, unsigned &del);

    double minimizeWordError(VocabIndex *words, unsigned length,
				double &sub, double &ins, double &del,
				unsigned flags = 0, double delBias = 1.0);

    Boolean isEmpty();
    unsigned length() { return numAligns; };
    
    VocabIndex deleteIndex;		// pseudo-word representing deletions

private:
    Array< LHash<VocabIndex,Prob>* > aligns;	// alignment columns
    Array< LHash<VocabIndex,Array<int> >* > hypMap;
					// pointers from word hyps
					//       to sentence hyps
    Array<int> allHyps;			// list of all aligned hyp IDs

    unsigned numAligns;			// number of alignment columns
    Array<unsigned> sortedAligns;	// topoligical order of alignment

    Prob totalPosterior;		// accumulated sample scores
};

/*
 * Enumeration of words in alignment and their associated hypMaps
 */
class WordMeshIter
{
public:
    WordMeshIter(WordMesh &mesh, unsigned position)
       : myWordMesh(mesh), myIter(*mesh.hypMap[mesh.sortedAligns[position]]) {};

    void init()
	{ myIter.init(); };
    Array<int> *next(VocabIndex &word)
	{ return myIter.next(word); };

private:
    WordMesh &myWordMesh;
    LHashIter<VocabIndex, Array<int> > myIter;
};

#endif /* _WordMesh_h_ */

