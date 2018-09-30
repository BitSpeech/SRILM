/*
 * WordMesh.h --
 *	Word Meshes (a simple type of word lattice with transitions between
 *	any two adjacent words).
 *
 * Copyright (c) 1998-2000 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/WordMesh.h,v 1.10 2001/06/08 05:56:16 stolcke Exp $
 *
 */

#ifndef _WordMesh_h_
#define _WordMesh_h_

#include "MultiAlign.h"
#include "VocabDistance.h"

#include "Array.h"
#include "LHash.h"

typedef unsigned short HypID;	/* index identifying a sentence in the
				 * alignment (short to save space) */

const HypID refID = (HypID)-1; 	/* pseudo-hyp ID used to identify the reference
				 * in an N-best alignment */

class WordMeshIter;

class WordMesh: public MultiAlign
{
    friend WordMeshIter;

public:
    WordMesh(Vocab &vocab, VocabDistance *distance = 0);
    ~WordMesh();

    Boolean read(File &file);
    Boolean write(File &file);

    void addWords(const VocabIndex *words, Prob score)
	{ alignWords(words, score); };
    void alignWords(const VocabIndex *words, Prob score, Prob *wordScores = 0)
	{ alignWords(words, score, wordScores, 0); };
    void alignWords(const VocabIndex *words, Prob score,
				    Prob *wordScores, const HypID *hypID);

    void alignReference(const VocabIndex *words)
	{ HypID id = refID;
	  alignWords(words, 0.0, 0, &id); };

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
    Array< LHash<VocabIndex,Array<HypID> >* > hypMap;
					// pointers from word hyps
					//       to sentence hyps
    Array<HypID> allHyps;		// list of all aligned hyp IDs

    unsigned numAligns;			// number of alignment columns
    Array<unsigned> sortedAligns;	// topoligical order of alignment

    Prob totalPosterior;		// accumulated sample scores

    VocabDistance *distance;		// word distance (or null)
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
    Array<HypID> *next(VocabIndex &word)
	{ return myIter.next(word); };

private:
    WordMesh &myWordMesh;
    LHashIter<VocabIndex, Array<HypID> > myIter;
};

#endif /* _WordMesh_h_ */

