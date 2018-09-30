/*
 * WordMesh.h --
 *	Word Meshes (a simple type of word lattice with transitions between
 *	any two adjacent words).
 *
 * Copyright (c) 1998-2001 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/WordMesh.h,v 1.14 2001/10/31 00:53:40 stolcke Exp $
 *
 */

#ifndef _WordMesh_h_
#define _WordMesh_h_

#include "MultiAlign.h"
#include "VocabDistance.h"

#include "Array.h"
#include "LHash.h"
#include "SArray.h"

class WordMeshIter;

class WordMesh: public MultiAlign
{
    friend class WordMeshIter;

public:
    WordMesh(Vocab &vocab, VocabDistance *distance = 0);
    ~WordMesh();

    Boolean read(File &file);
    Boolean write(File &file);

    void alignWords(const VocabIndex *words, Prob score,
				Prob *wordScores = 0, const HypID *hypID = 0);
    void alignWords(const NBestWordInfo *winfo, Prob score,
				Prob *wordScores = 0, const HypID *hypID = 0);

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
    Array< LHash<VocabIndex,NBestWordInfo>* > wordInfo;
					// word backtrace info
    Array< LHash<VocabIndex,Array<HypID> >* > hypMap;
					// pointers from word hyps
					//       to sentence hyps
    SArray<HypID,HypID> allHyps;	// list of all aligned hyp IDs
					// 	(Note: only keys are used)
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

