/*
 * Lattice.h --
 *	Word lattices
 *
 * Copyright (c) 1997-2002, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lattice/src/RCS/Lattice.h,v 1.38 2003/02/19 07:38:25 stolcke Exp $
 *
 */

#ifndef _Lattice_h_
#define _Lattice_h_

/* ******************************************************************
   header files 
   ****************************************************************** */
#include <math.h>

#include "Prob.h"
#include "Boolean.h"
#include "LHash.h"
#include "SArray.h"
#include "Map2.h"
#include "Vocab.h"
#include "SubVocab.h"
#include "File.h"
#include "Debug.h"
#include "LM.h"
#include "Ngram.h"

class Lattice;               /* forward declaration */

typedef const VocabIndex *VocabContext;

typedef unsigned NodeIndex;

/* ******************************************************************
   flags for node
   ****************************************************************** */
const unsigned markedFlag = 8;     //to indicate that this node is processed
const unsigned nullNodeFlag = 64;  //to indicate whether a null node has been
                                   //  processed.

/* ******************************************************************
   flags for transition
   ****************************************************************** */
const unsigned pauseTFlag = 2;     //there was a pause node on the link
const unsigned directTFlag = 4;    //this is a non-pause link between two nodes
const unsigned markedTFlag = 8;    //to indicate that this trans has been 
                                   //processed
const unsigned reservedTFlag = 16; //to indicate that this trans needs to be 
                                   //  reserved for bigram;

/* ******************************************************************
   other constants
   ****************************************************************** */

const int minIntlog = -250000;	   // minumum intlog values used when
				   // printing PFSGs.  Value is chosen such
				   // that PFSG probs can be safely converted
				   // to bytelogs in the recognizer.

#define LATTICE_OR	"or"
#define LATTICE_CONCATE	"concatenate"

/* ******************************************************************
   structure
   ****************************************************************** */


inline LogP combWeights(LogP weight1, LogP weight2)
{
  return (weight1+weight2); 
}

inline LogP 
unionWeights(LogP weight1, LogP weight2)
{
  return (weight1 > weight2 ? weight1 : weight2); 
}

inline LogP unit()
{
  return 0;
}    

inline int
nodeSort(NodeIndex n1, NodeIndex n2)
{
  return (n1 - n2);
}

class NodeQueue;

typedef struct SelfLoopDB {

  // initA
  NodeIndex preNodeIndex; 
  NodeIndex postNodeIndex2;
  NodeIndex postNodeIndex3;
  NodeIndex nodeIndex;
  VocabIndex wordName; 
  unsigned selfTransFlags; 
  LogP loopProb;

  // initB
  NodeIndex fromNodeIndex;
  VocabIndex fromWordName; 
  LogP fromPreProb; 
  LogP prePostProb; 
  unsigned fromSelfTransFlags; 

  // initC
  NodeIndex toNodeIndex;
  unsigned selfToTransFlags;

} SelfLoopDB;

#ifdef USE_SARRAY
#define TRANS_T		SArray
#define TRANSITER_T	SArrayIter
#else
#define TRANS_T		LHash
#define TRANSITER_T	LHashIter
#endif

/* *************************
 * A transition in a lattice
 * ************************* */
class LatticeTransition 
{ 
  public: 
    LatticeTransition() : weight(0), flags(0) {};
    LatticeTransition(LogP weight, unsigned flags)
	: weight(weight), flags(flags) {};
    
    void markTrans(unsigned flag) { flags |= flag; }; 

    void setWeight(LogP givenWeight) { weight = givenWeight; }; 

    Boolean getFlag(unsigned flag) { return (flags & flag); }; 

    LogP weight;		// weight (e.g., probability) of transition
    unsigned flags;		// miscellaneous flags;
}; 

/* *************************
 * A node in a lattice
 ************************* */
class LatticeNode
{
  friend class LatticeTransition;

public:
    LatticeNode();     // initializing lattice node;

    unsigned flags; 
    VocabIndex word;		// word associated with this node
    LogP2 posterior;		// node posterior (unnormalized)

    TRANS_T<NodeIndex,LatticeTransition> outTransitions;// outgoing transitions
    TRANS_T<NodeIndex,LatticeTransition> inTransitions; // incoming transitions;

    void
      markNode(unsigned flag) { flags |= flag; }; 
    // set to one the bits indicated by flag;
 
    void
      unmarkNode(unsigned flag) { flags &= ~flag; }; 
    // set to zero the bits indicated by flag;
 
    Boolean
      getFlag(unsigned flag) { return (flags & flag); };
};


/* *************************
 * A lattice 
 ************************* */
class Lattice: public Debug
{
  friend class NodeQueue;
  friend class PackedNodeList; 
  friend class LatticeTransition;
  friend class LatticeNode;

public:

    /* *************************************************
       within single lattice operations
       ************************************************* */
    Lattice(Vocab &vocab, const char *name = "NONAME");
    ~Lattice();

    Boolean computeNodeEntropy(); 
    LogP detectSelfLoop(NodeIndex nodeIndex);
    Boolean recoverPauses(Boolean loop = true); 
    Boolean recoverCompactPauses(Boolean loop = true); 
    Boolean removeAllXNodes(VocabIndex xWord);
    Boolean replaceWeights(LM &lm); 
    Boolean simplePackBigramLattice(unsigned iters = 0, Boolean maxAdd = false);
    Boolean approxRedBigramLattice(unsigned iters, int base, double ratio);
    Boolean expandToTrigram(LM &lm, unsigned maxNodes = 0);
    Boolean expandToCompactTrigram(Ngram &ngram, unsigned maxNodes = 0);
    Boolean expandToLM(LM &lm, unsigned maxNodes = 0);
    Boolean collapseSameWordNodes(SubVocab &exceptions);

    /* *************************************************
        operations with two lattices
       ************************************************* */
    Boolean implantLattice(NodeIndex nodeIndex, Lattice *lat);
    Boolean implantLatticeXCopies(Lattice *lat);
    Boolean latticeCat(Lattice *lat1, Lattice *lat2);
    Boolean latticeOr(Lattice *lat1, Lattice *lat2);

    /* ********************************************************* 
       lattice input and output 
       ********************************************************* */

    Boolean readPFSG(File &file);
    Boolean readPFSGs(File &file);
    Boolean readPFSGFile(File &file);
    Boolean readRecPFSGs(File &file);

    Boolean writePFSG(File &file);
    Boolean writeCompactPFSG(File &file);
    Boolean writePFSGFile(File &file);

    /* ********************************************************* 
       nodes and transitions
       ********************************************************* */
    Boolean insertNode(const char *word, NodeIndex nodeIndex); 
    Boolean insertNode(const char *word) {
	return insertNode(word, maxIndex++); };
    NodeIndex dupNode(VocabIndex windex, unsigned markedFlag = 0);     
    // duplicate a node with the same word name; 
    Boolean removeNode(NodeIndex nodeIndex);  
    // all the edges connected with this node will be removed;  
    LatticeNode *findNode(NodeIndex nodeIndex) {
        return nodes.find(nodeIndex); };

    Boolean insertTrans(NodeIndex fromNodeIndex, NodeIndex toNodeIndex, 
			const LatticeTransition &trans, Boolean maxAdd = false);
    Boolean findTrans(NodeIndex fromNodeIndex, NodeIndex toNodeIndex);
    Boolean setWeightTrans(NodeIndex fromNodeIndex, 
			   NodeIndex toNodeIndex, LogP weight); 
    void markTrans(NodeIndex fromNodeIndex, NodeIndex toNodeIndex, 
		   unsigned flag);
    // Boolean dupTrans(NodeIndex fromNodeIndex, NodeIndex toNodeIndex); 
    Boolean removeTrans(NodeIndex fromNodeIndex, NodeIndex toNodeIndex);

    void dumpFlags();
    void clearMarkOnAllNodes(unsigned flag = ~0); 
    void clearMarkOnAllTrans(unsigned flag = ~0);

    // topological sorting
    unsigned sortNodes(NodeIndex *sortedNodes, Boolean reversed = false); 
    
    /* ********************************************************* 
       get pretected class values
       ********************************************************* */
    NodeIndex getInitial() { return initial; }
    NodeIndex getFinal() { return final; }
    const char *getName() { return name; }
    NodeIndex getMaxIndex() { return maxIndex; }
    const char *getWord(VocabIndex word) { return vocab.getWord(word); }
    unsigned getNumNodes() { return nodes.numEntries(); }
    unsigned getNumTransitions();

    NodeIndex setInitial(NodeIndex index) { return (initial=index); }
    NodeIndex setFinal(NodeIndex index) { return (final=index); }

    /* ********************************************************* 
       diagnostic tools
       ********************************************************* */
    Boolean printNodeIndexNamePair(File &file); 
    Boolean areConnected(NodeIndex fromNodeIndex, NodeIndex toNodeIndex,
			 unsigned direction = 0); 
    unsigned latticeWER(const VocabIndex *words,
			unsigned &sub, unsigned &ins, unsigned &del)
        { SubVocab ignoreWords(vocab);
	  return latticeWER(words, sub, ins, del, ignoreWords);
	};
    unsigned latticeWER(const VocabIndex *words,
			unsigned &sub, unsigned &ins, unsigned &del,
			SubVocab &ignoreWords); 

    LogP2 computePosteriors(double posteriorScale = 1.0);
    Boolean writePosteriors(File &file, double posteriorScale = 1.0);
    Boolean prunePosteriors(Prob threshold, double posteriorScale = 1.0);

    /* ********************************************************* 
       operations on nullNodes
       ********************************************************* */
    void clearMarkOnNulls(); 
    Boolean markNodeNulls(NodeIndex nodeIndex, unsigned flag); 
    Boolean getFlagOfNull(NodeIndex nodeIndex, unsigned flag);

    /* ********************************************************* 
       self-loop processing
       ********************************************************* */
    static void initASelfLoopDB(SelfLoopDB &selfLoopDB, LM &lm,
				NodeIndex nodeIndex, LatticeNode *node,
				LatticeTransition *trans);
    static void initBSelfLoopDB(SelfLoopDB &selfLoopDB, LM &lm, 
				NodeIndex fromNodeIndex, LatticeNode *fromNode,
				LatticeTransition *fromTrans);
    static void initCSelfLoopDB(SelfLoopDB &selfLoopDB, NodeIndex toNodeIndex,
				LatticeTransition *toTrans);

    Boolean expandSelfLoop(LM &lm, SelfLoopDB &selfLoopDB, 
				PackedNodeList &packedSelfLoopNodeList);

    /* ********************************************************* 
       internal data structure
       ********************************************************* */
    Vocab &vocab;		// vocabulary used for words
    LHash<NodeIndex,LatticeNode> nodes;	// node list; 

protected: 

    LHash<NodeIndex,LatticeNode> nullNodes; //
    LHash<VocabIndex, Lattice *> subPFSGs;  // for process subPFSGs

    NodeIndex maxIndex;		// the current largest node index
    const char *name;		// name string for lattice

    NodeIndex initial;		// start node index
    NodeIndex final;		// final node index;

    Boolean top;                // an indicator for whether two null nodes 
				// (initial and final) have been converted
				// to <s> and </s>.

    Lattice *getNonRecPFSG(VocabIndex nodeVocab);
    Boolean recoverPause(NodeIndex nodeIndex, Boolean loop = true);
    Boolean recoverCompactPause(NodeIndex nodeIndex, Boolean loop = true); 

    void sortNodesRecursive(NodeIndex nodeIndex, unsigned &numVisited,
			    NodeIndex *sortedNodes, Boolean *visitedNodes);

    LogP2 computeForwardBackward(LogP2 forwardProbs[], LogP2 backwardProbs[],
							double posteriorScale);

    Boolean
      expandNodeToTrigram(NodeIndex nodeIndex, LM &lm, unsigned maxNodes = 0);

    Boolean
      expandNodeToCompactTrigram(NodeIndex nodeIndex, Ngram &ngram,
							unsigned maxNodes = 0);

    Boolean
      expandNodeToLM(VocabIndex node, LM &ngram, unsigned maxNodes, 
			Map2<NodeIndex, VocabContext, NodeIndex> &expandMap);

    void
      mergeNodes(NodeIndex nodeIndex1, NodeIndex nodeIndex2,
			LatticeNode *node1 = 0, LatticeNode *node2 = 0,
			Boolean maxAdd = false);

    Boolean 
      approxMatchInTrans(NodeIndex nodeIndexI, NodeIndex nodeIndexJ,
			 int overlap); 

    Boolean 
      approxMatchOutTrans(NodeIndex nodeIndexI, NodeIndex nodeIndexJ,
			  int overlap); 

    void
      packNodeF(NodeIndex nodeIndex, Boolean maxAdd = false);

    void
      packNodeB(NodeIndex nodeIndex, Boolean maxAdd = false);

    Boolean 
      approxRedNodeF(NodeIndex nodeIndex, NodeQueue &nodeQueue, 
		     int base, double ratio);

    Boolean 
      approxRedNodeB(NodeIndex nodeIndex, NodeQueue &nodeQueue, 
		     int base, double ratio);
};

class QueueItem: public Debug
{
  // for template class, we need to add;
  // friend class Queue<Type>; 

friend class NodeQueue; 
  
public:
    QueueItem(NodeIndex nodeIndex, unsigned clevel = 0, 
	      LogP cweight = 0.0); 
    
    NodeIndex QueueGetItem() { return item; }
    unsigned QueueGetLevel() { return level; }
    LogP QueueGetWeight() { return weight; }

private:
    NodeIndex item;
    unsigned level; 
    LogP weight; 
    QueueItem *next; 
};

class NodeQueue: public Debug
{
public:
    NodeQueue() { queueHead = queueTail = 0; }

    ~NodeQueue(); 

    NodeIndex popNodeQueue(); 
    QueueItem *popQueueItem(); 

    Boolean is_empty() {
      return queueHead == 0 ? true : false; }

    // Boolean is_full();
      
    Boolean addToNodeQueue(NodeIndex nodeIndex, unsigned level = 0, 
			   LogP weight = 0.0); 

    Boolean addToNodeQueueNoSamePrev(NodeIndex nodeIndex, unsigned level = 0, 
			   LogP weight = 0.0); 

    // Boolean pushQueueItem(QueueItem queueItem); 

    Boolean inNodeQueue(NodeIndex nodeIndex);

private:

    QueueItem *queueHead;      // the head of the node queue;
    QueueItem *queueTail;      // the tail of the node queue;
    
};


typedef struct PackedNode {
  // currently, this information is not used.
  // In a later version, I will test this List first, and if the wordName
  //   is not found, then compute trigram prob. This way, I can gain
  //   some efficiency.
  // if there is a trigramProg, both bigramProg and backoffWeight are 0;
  // if there is a bigramProg,  trigramProg will be 0;

  // In the latest version, no distinction made for the order of ngram.
  // All the weights are treated as same, and they are based only their
  // positions.
  // LogP inWeight;
  LogP outWeight;
  unsigned toNodeId; 
  NodeIndex toNode; 
  NodeIndex midNodeIndex; 
} PackedNode; 

// this is an input structure for PackedNodeList::packingNodes
typedef struct PackInput {

  VocabIndex fromWordName;  // starting node name
  VocabIndex wordName;      // current node name
  VocabIndex toWordName;    // ending node name
  LM *lm;		    // LM to compute outWeight on demand

  unsigned toNodeId; 
  NodeIndex fromNodeIndex;  // starting node
  NodeIndex toNodeIndex;    // ending node, 
		  // a new node will be created in between these two nodes.  
  NodeIndex nodeIndex;      // for debugging purpose

  LogP inWeight;            // weight for an in-coming trans to the new node
  LogP outWeight;           // weight for an out-going trans from the new node
  unsigned inFlag; 
  unsigned outFlag; 

} PackInput; 

class PackedNodeList: public Debug 
{
public: 
  PackedNodeList() : lastPackedNode(0) {};
  Boolean packNodes(Lattice &lat, PackInput &packInput);

private:
  LHash<VocabIndex,PackedNode> packedNodesByFromNode;
  PackedNode *lastPackedNode;
}; 


#endif /* _Lattice_h_ */

