/*
 * Lattice.cc --
 *	Word lattices
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1997-2002 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Header: /home/srilm/devel/lattice/src/RCS/Lattice.cc,v 1.57 2003/02/19 07:38:25 stolcke Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "Lattice.h"

#include "SArray.cc"
#include "LHash.cc"
#include "Map2.cc"
#include "Array.cc"
#include "WordAlign.h"                  /* for *_COST constants */

#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_MAP2(NodeIndex, VocabContext, NodeIndex);
#endif

/*
 * If the intlog weights of two transitions differ by no more than this
 * they are considered identical in PackedNodeList::packNodes().
 */
#define PACK_TOLERANCE			0

#define DebugPrintFatalMessages         1 

#define DebugPrintFunctionality         1 
// for large functionality listed in options of the program
#define DebugPrintOutLoop               2
// for out loop of the large functions or small functions
#define DebugPrintInnerLoop             3
// for inner loop of the large functions or outloop of small functions

#ifdef INSTANTIATE_TEMPLATES
#ifdef USE_SARRAY
INSTANTIATE_SARRAY(NodeIndex,LatticeTransition);
#else
INSTANTIATE_LHASH(NodeIndex,LatticeTransition);
#endif
INSTANTIATE_LHASH(NodeIndex,LatticeNode);
INSTANTIATE_LHASH(VocabIndex,PackedNode);
#endif

/* **************************************************
   the queue related methods
   ************************************************** */

NodeQueue::~NodeQueue()
{
    while (is_empty() != true) {
	(void) popNodeQueue();
    }
}

NodeIndex NodeQueue::popNodeQueue()
{
    if (is_empty() == true) {
        cerr << "popNodeQueue() on an empty queue\n";
        exit(-1); 
    }

    QueueItem *pt = queueHead; 
    queueHead = queueHead->next;
    if (queueHead == 0) { 
	queueTail = 0;
    }
    NodeIndex retval = pt->item;
    delete pt;
    return retval;
} 

QueueItem * NodeQueue::popQueueItem()
{
    if (is_empty() == true) {
        cerr << "popQueueItem() on an empty queue\n";
        exit(-1); 
    }

    QueueItem *pt = queueHead; 
    queueHead = queueHead->next;

    return pt;
} 
    
Boolean NodeQueue::addToNodeQueue(NodeIndex nodeIndex, 
				  unsigned level, LogP weight)
{
    QueueItem *pt = new QueueItem(nodeIndex, level, weight);
    assert(pt != 0); 

    if (is_empty() == true) {
        queueHead = queueTail = pt;
    } else {
        queueTail->next = pt;
	queueTail = pt;
    }

    return true;
}

// add to NodeQueue if the previous element is not the same
// as the element to be added.
Boolean 
NodeQueue::addToNodeQueueNoSamePrev(NodeIndex nodeIndex, 
				    unsigned level, LogP weight)
{
    if (is_empty() == false && nodeIndex == queueTail->item) {
        if (debug(DebugPrintOutLoop)) {
	  dout() << "In addToNodeQueueNoSamePrev: skip the current nodeIndex ("
		 << nodeIndex << ")" << endl;
	}
        return true;
    }

    if (debug(DebugPrintOutLoop)) {
      dout() << "In addToNodeQueueNoSamePrev: add the current nodeIndex ("
	     << nodeIndex << ")" << endl;
    }
    QueueItem *pt = new QueueItem(nodeIndex, level, weight);
    assert(pt != 0); 

    if (is_empty() == true) {
        queueHead = queueTail = pt;
    } else {
        queueTail->next = pt;
	queueTail = pt;
    }

    return true;
}

Boolean NodeQueue::inNodeQueue(NodeIndex nodeIndex)
{
  
    QueueItem *pt = queueHead;

    while (pt != 0) {
	if (pt->item == nodeIndex) {
	    return true; 
	}
	pt = pt->next;
    }

    return false;
}

QueueItem::QueueItem(NodeIndex nodeIndex, unsigned clevel, LogP cweight)
{
    item = nodeIndex;
    level = clevel;
    weight = cweight; 
    next = 0; 
}


/* ************************************************************
   function definitions for class LatticeNode
   ************************************************************ */
LatticeNode::LatticeNode()
    : word(Vocab_None), posterior(LogP_Zero)
{
}

/* ************************************************************
   function definitions for class LatticeTransition
   ************************************************************ */

/* ************************************************************
   function definitions for class Lattice
   ************************************************************ */

Lattice::Lattice(Vocab &vocab, const char *name)
    : name(strdup(name)), vocab(vocab)
{
    assert(name != 0);

    initial = 0;
    final = 0;
    top = 1; 

    /*
     * To create a legal lattice, create single node that is both initial
     * and final.
     */
    nodes.insert(0)->word = Vocab_None;

    maxIndex = 0; 
}

Lattice::~Lattice()
{
    free((void *)name);
    name = 0;

    LHashIter<NodeIndex, LatticeNode> nodeIter(nodes);
    NodeIndex nodeIndex;

    while (LatticeNode *node = nodeIter.next(nodeIndex)) {
	node->~LatticeNode();
    }
}

/* To insert a node with name *word in the NodeLattice */
Boolean 
Lattice::insertNode(const char *word, NodeIndex nodeIndex)
{
    VocabIndex windex;

    if (strcmp(word, "NULL") == 0) {
	windex = Vocab_None;
    } else {
	windex = vocab.addWord(word);
    }

    LatticeNode *node = nodes.insert(nodeIndex);

    node->word = windex;
    return true; 
    
}

/* To duplicate a node with a same word name without making 
   any commitment on edges
   
   */
NodeIndex Lattice::dupNode(VocabIndex windex, unsigned markedFlag) 
{

    LatticeNode *node = nodes.insert(maxIndex);

    node->word = windex;

    node->flags = markedFlag; 

    return maxIndex++; 
}

/* remove the node and all of its edges (incoming and outgoing)
   and check to see whether it makes the graph disconnected
*/

Boolean 
Lattice::removeNode(NodeIndex nodeIndex) 
{
    LatticeNode *node = findNode(nodeIndex);
    if (!node) {
      if (debug(DebugPrintOutLoop)) {
	dout() << " In Lattice::removeNode: undefined node in graph " 
	       << nodeIndex << endl;
      }
      return false; 
    }

    // delete incoming transitions -- need do only the fromNode->outTransitions
    // node->inTransition will be freed shortly
    TRANSITER_T<NodeIndex,LatticeTransition> 
      inTransIter(node->inTransitions);
    NodeIndex fromNodeIndex;
    while (inTransIter.next(fromNodeIndex)) {
	LatticeNode *fromNode = findNode(fromNodeIndex);
	assert(fromNode != 0);
	fromNode->outTransitions.remove(nodeIndex);
    } 

    // delete outgoing transitions -- need do only the toNode->inTransitions
    // node->outTransition will be freed shortly
    TRANSITER_T<NodeIndex,LatticeTransition> 
      outTransIter(node->outTransitions);
    NodeIndex toNodeIndex;
    while (outTransIter.next(toNodeIndex)) {
	LatticeNode *toNode = findNode(toNodeIndex);
	assert(toNode != 0);
	toNode->inTransitions.remove(nodeIndex);
    } 

    // remove this node
    LatticeNode *removedNode = nodes.remove(nodeIndex);
    if (debug(DebugPrintOutLoop)) {
      dout() << "In Lattice::removeNode: remove node " << nodeIndex << endl; 
    }

    removedNode->~LatticeNode();
    
    return true;
}

// try to find a transition between the two given nodes.
// return 0, if there is no transition.
// return 1, if there is a transition.
Boolean 
Lattice::findTrans(NodeIndex fromNodeIndex, NodeIndex toNodeIndex)
{
    LatticeNode *toNode = nodes.find(toNodeIndex); 
    if (!toNode) {
      // the end node does not exist.
      return false;
    }

    LatticeNode *fromNode = nodes.find(fromNodeIndex);
    if (!fromNode) {
      // the begin node does not exist.
      return false;
    }

    LatticeTransition * trans = toNode->inTransitions.find(fromNodeIndex); 
    if (!trans) {
      // the transition does not exist.
      return false;
    }

#ifdef DEBUG
    LatticeTransition *outTrans = fromNode->outTransitions.find(toNodeIndex);
    if (!outTrans) {
      // asymmetric transition.
      if (debug(DebugPrintFatalMessages)) {
	dout() << "nonFatal error in Lattice::findTrans: asymmetric transition."
	       << endl;
      }
      return false;
    }
#endif

    return true;
}


// to insert a transition between two nodes. If the transition exists already
// union their weights. maxAdd == 0, take the max of the existing and the new 
// weights; maxAdd == 1, take the added weights of the two (notice that the 
// weights are in log scale, so log(x+y) = logx + log (y/x + 1)
Boolean
Lattice::insertTrans(NodeIndex fromNodeIndex, NodeIndex toNodeIndex, 
			     const LatticeTransition &t, Boolean maxAdd)  
{

    LatticeNode *toNode = nodes.find(toNodeIndex); 
    if (!toNode) {
        if (debug(DebugPrintFatalMessages)) {
	  dout() << "Fatal error in Lattice::insertTrans: can't find toNode"
		 << toNodeIndex << endl;
	}
	exit(-1); 
    }

    LatticeNode *fromNode = nodes.find(fromNodeIndex);
    if (!fromNode) {
        if (debug(DebugPrintFatalMessages)) {
	  dout() << "Fatal error in Lattice::insertTrans: can't find fromNode"
		 << fromNodeIndex << endl;
	}
	exit(-1); 
    }

    Boolean found;
    LatticeTransition *trans =
			toNode->inTransitions.insert(fromNodeIndex, found); 

    if (!found) {
      // it's a new edge; 
      *trans = t;
    } else {
      // there is already an edge 
      if (!maxAdd) {
        trans->weight = unionWeights(trans->weight, t.weight); 
      } else { 
        trans->weight = AddLogP(trans->weight, t.weight); 
      }
      trans->flags = trans->flags | t.flags |
			(!trans->getFlag(pauseTFlag) ? directTFlag : 0);
    }

    // duplicate intransition as outtransition
    *fromNode->outTransitions.insert(toNodeIndex) = *trans; 
    
    return true;
}

Boolean
Lattice::setWeightTrans(NodeIndex fromNodeIndex, NodeIndex toNodeIndex, 
			LogP weight)  
{

    LatticeNode *toNode = nodes.find(toNodeIndex); 
    if (!toNode) {
        if (debug(DebugPrintFatalMessages)) {
	  dout() << "Fatal error in Lattice::setWeightTrans: can't find toNode" 
		 << toNodeIndex << endl;
	}
	exit(-1); 
    }

    LatticeTransition * trans = toNode->inTransitions.find(fromNodeIndex); 
    if (!trans) {
        if (debug(DebugPrintFatalMessages)) {
	  dout() << "Fatal error in Lattice::setWeightTrans: can't find inTrans(" 
		 << fromNodeIndex << "," << toNodeIndex << ")\n";
	}
	exit(-1); 
    }

    trans->weight = weight;

    LatticeNode *fromNode = nodes.find(fromNodeIndex);
    if (!fromNode) {
        if (debug(DebugPrintFatalMessages)) {
	  dout() << "Fatal error in Lattice::setWeightTrans: can't find fromNode" 
		 << fromNodeIndex << endl;
	}
	exit(-1); 
    }

    trans = fromNode->outTransitions.find(toNodeIndex); 
    if (!trans) {
        if (debug(DebugPrintFatalMessages)) {
	  dout() << "Fatal error in Lattice::setWeightTrans: can't find outTrans (" 
		 << fromNodeIndex << "," << toNodeIndex << ")\n";
	}
	exit(-1); 
    }

    trans->weight = weight;

    return true;

}

Boolean 
Lattice::removeTrans(NodeIndex fromNodeIndex, NodeIndex toNodeIndex)
{
    LatticeNode *fromNode = nodes.find(fromNodeIndex);
    
    if (!fromNode) {
        if (debug(DebugPrintOutLoop)) {
	  dout() << "nonFatal error in Lattice::removeTrans:\n" 
		 << "   undefined source node in transition " 
		 << (fromNodeIndex, toNodeIndex) << endl;
	}
	return false;
    }

    LatticeTransition * outTrans = 
        fromNode->outTransitions.remove(toNodeIndex); 
    if (!outTrans) {
        if (debug(DebugPrintOutLoop)) {
	  dout() << "nonFatal error in Lattice::removeTrans:\n" 
		 << "   no outTrans (" << fromNodeIndex << ", " 
		 << toNodeIndex << ")\n";
	}
	return false;
    }

    LatticeNode *toNode = nodes.find(toNodeIndex);
    
    if (!toNode) {
        if (debug(DebugPrintOutLoop)) {
	  dout() << "nonFatal error in Lattice::removeTrans:\n" 
		 << "undefined sink node " << toNodeIndex << endl;
	}
	return false;
    }

    LatticeTransition * inTrans = 
      toNode->inTransitions.remove(fromNodeIndex); 

    if (!inTrans) {
        if (debug(DebugPrintOutLoop)) {
	  dout() << "nonFatal error in Lattice::removeTrans:\n" 
		 << "   no inTrans (" << fromNodeIndex << ", " 
		 << toNodeIndex << ")\n";
	}
	return false;
    }

    return true;    
}   


void 
Lattice::markTrans(NodeIndex fromNodeIndex, NodeIndex toNodeIndex, unsigned flag)
{
    LatticeNode *fromNode = nodes.find(fromNodeIndex); 
    if (!fromNode) {
        if (debug(DebugPrintFatalMessages)) {
	  dout() << "Fatal Error in Lattice::markTrans: can't find source node" 
		 << fromNodeIndex << endl;
	}
	exit(-1); 
    }

    LatticeTransition * trans = fromNode->outTransitions.find(toNodeIndex); 
    if (!trans) {
        if (debug(DebugPrintFatalMessages)) {
	  dout() << "Fatal Error in Lattice::markTrans: can't find outTrans ("
		 << fromNodeIndex << ", " << toNodeIndex << ")\n"; 
	}
	exit(-1); 
    }
    trans->flags |= flag; 

     LatticeNode *toNode = nodes.find(toNodeIndex); 
    if (!toNode) {
        if (debug(DebugPrintFatalMessages)) {
	  dout() << "Fatal Error in Lattice::markTrans: can't find sink node" 
		 << toNodeIndex << endl;
	}
	exit(-1); 
    }

    trans = toNode->inTransitions.find(fromNodeIndex); 
    if (!trans) {
        if (debug(DebugPrintFatalMessages)) {
	  dout() << "Fatal Error in Lattice::markTrans: can't find inTrans ("
		 << toNodeIndex << ", " << toNodeIndex << ")\n"; 
	}
	exit(-1); 
    }
    trans->flags |= flag; 
}

void
Lattice::clearMarkOnAllNodes(unsigned flag) {
    LHashIter<NodeIndex, LatticeNode> nodeIter(nodes);
    NodeIndex nodeIndex;
    while (LatticeNode *node = nodeIter.next(nodeIndex)) {
      node->flags &= ~flag;
    }
}

void
Lattice::dumpFlags() {
    LHashIter<NodeIndex, LatticeNode> nodeIter(nodes, nodeSort);
    NodeIndex nodeIndex;

    while (LatticeNode *node = nodeIter.next(nodeIndex)) {
      dout() << "node " << nodeIndex << " flags = " << node->flags << endl;

      TRANSITER_T<NodeIndex,LatticeTransition> outIter(node->outTransitions);
      NodeIndex outIndex; 
      while (LatticeTransition *outTrans = outIter.next(outIndex)) {
        dout() << "  trans -> " << outIndex 
	       << " flags = " << outTrans->flags << endl;
      }

      TRANSITER_T<NodeIndex,LatticeTransition> inIter(node->inTransitions);
      NodeIndex inIndex; 
      while (LatticeTransition *inTrans = inIter.next(inIndex)) {
        dout() << "  trans <- " << inIndex 
	       << " flags = " << inTrans->flags << endl;
      }
    }
}

void
Lattice::clearMarkOnAllTrans(unsigned flag) {

    LHashIter<NodeIndex, LatticeNode> nodeIter(nodes);
    NodeIndex nodeIndex;

    while (LatticeNode *node = nodeIter.next(nodeIndex)) {
      TRANSITER_T<NodeIndex,LatticeTransition> outIter(node->outTransitions);
      NodeIndex outIndex; 
      while (LatticeTransition *outTrans = outIter.next(outIndex)) {
	outTrans->flags &= ~flag;
      }

      TRANSITER_T<NodeIndex,LatticeTransition> inIter(node->inTransitions);
      NodeIndex inIndex; 
      while (LatticeTransition *inTrans = inIter.next(inIndex)) {
	inTrans->flags &= ~flag;
      }
    }
}

void
Lattice::clearMarkOnNulls() {
    LHashIter<NodeIndex, LatticeNode> nullNodeIter(nullNodes);
    NodeIndex nodeIndex;
    while (LatticeNode *node = nullNodeIter.next(nodeIndex)) {
      node->flags = 0;
    }
}

Boolean
Lattice::markNodeNulls(NodeIndex nodeIndex, unsigned flag) {
    LatticeNode * node = nullNodes.find(nodeIndex); 
    if (!node) {
        if (debug(DebugPrintOutLoop)) {
	  dout() << "nonFatal Error in Lattice::markNodeNulls: "
		 << nodeIndex << " is not a Null node\n";
	}
	return false;
    }
    node->flags = flag;
    return true;
}

Boolean 
Lattice::getFlagOfNull(NodeIndex nodeIndex, unsigned flag) {
    LatticeNode * node = nullNodes.find(nodeIndex); 
    if (!node) {
        if (debug(DebugPrintOutLoop)) {
	  dout() << "nonFatal Error in Lattice::getFlagOfNull: "
		 << nodeIndex << " is not a Null node\n";
	}
	return false;
    }
    return (node->flags & flag);
}

// compute 'or' operation on two input lattices
// using implantLattice, 
// assume that the initial state of this lattice is empty.

Boolean
Lattice::latticeOr(Lattice *lat1, Lattice *lat2)
{
    initial = 1;
    final = 0;

    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::latticeOr: doing OR operation\n"; 
	     
    }

    LatticeNode *newNode = nodes.insert(0);
    newNode->word = Vocab_None;
    newNode->flags = 0;

    newNode = nodes.insert(1);
    newNode->word = Vocab_None;
    newNode->flags = 0;

    newNode = nodes.insert(2);
    newNode->word = Vocab_None;
    newNode->flags = 0;

    newNode = nodes.insert(3);
    newNode->word = Vocab_None;
    newNode->flags = 0;

    maxIndex = 4;

    LatticeTransition t(0, 0);
    insertTrans(1, 2, t);
    insertTrans(1, 3, t);
    insertTrans(2, 0, t);
    insertTrans(3, 0, t);
    
    if (!(implantLattice(2, lat1))) { return false; }

    if (!(implantLattice(3, lat2))) { return false; }

    return true;

}

Boolean
Lattice::latticeCat(Lattice *lat1, Lattice *lat2)
{
    initial = 1;
    final = 0;

    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::latticeCat: doing CONCATENATE operation\n"; 
	     
    }

    LatticeNode *newNode = nodes.insert(0);
    newNode->word = Vocab_None;
    newNode->flags = 0;

    newNode = nodes.insert(1);
    newNode->word = Vocab_None;
    newNode->flags = 0;

    newNode = nodes.insert(2);
    newNode->word = Vocab_None;
    newNode->flags = 0;

    newNode = nodes.insert(3);
    newNode->word = Vocab_None;
    newNode->flags = 0;

    maxIndex = 4;

    LatticeTransition t(0, 0);;
    insertTrans(1, 2, t);
    insertTrans(2, 3, t);
    insertTrans(3, 0, t);
    
    if (!(implantLattice(2, lat1))) { return false; }

    if (!(implantLattice(3, lat2))) { return false; }

    return true;

}

// reading in recursively defined PFSGs
// 

Boolean
Lattice::readRecPFSGs(File &file)
{
    if (debug(DebugPrintFunctionality)) {
      dout()  << "Lattice::readRecPFSGs: "
	      << "reading in dag PFSG......\n"; 
    }

    NodeQueue nodeQueue; 

    // Vocab locaVocab;
    readPFSG(file); 

    while (fgetc(file) != EOF) {
      fseek(file, -1, SEEK_CUR); 

      Lattice *lat = new Lattice(vocab); 
      Boolean succ = lat->readPFSG(file);
      if (!succ) {
	if (debug(DebugPrintFunctionality)) {
	  dout()  << "Lattice::readRecPFSGs: "
		  << "failed in reading in dag PFSGs.\n";
	}
	dout() << "Test: 0\n"; 
	delete lat;
	return false; 
      }
      
      char * subPFSGName = strdup(lat->getName()); 
      VocabIndex word = vocab.addWord(subPFSGName);
      Lattice **pt = subPFSGs.insert(word);
      *pt = lat; 

      if (debug(DebugPrintOutLoop)) {
	dout()  << "Lattice::readRecPFSGs: got "
		<< "sub-PFSG (" << (*pt)->getName() << ").\n";
      }      
    }

    LHashIter<NodeIndex, LatticeNode> nodeIter(nodes, nodeSort);
    NodeIndex nodeIndex;

    while (LatticeNode *node = nodeIter.next(nodeIndex)) {
      // for non-terminal node, i.e., sub-PFSGs, get its non
      // recursive equivalent PFSG.
      dout() << "Processing node (" << node->word
	     << ")\n";
      Lattice ** pt = subPFSGs.find(node->word);
      if (!pt) {
	// terminal node
	dout() << "It's terminal node!\n";
	continue;
      }
      Lattice * lat = *pt; 
      dout() << "It's NON-terminal node!\n";
      nodeQueue.addToNodeQueue(nodeIndex); 
    }

    if (debug(DebugPrintOutLoop)) {
      dout()  << "Lattice::readRecPFSGs: done with "
	      << "preparing vtmNodes for implants\n"; 
    }

    while (nodeQueue.is_empty() == false) {
      nodeIndex = nodeQueue.popNodeQueue();
      // duplicate lat within this current lattice.
      LatticeNode * node = nodes.find(nodeIndex);
      
      if (debug(DebugPrintOutLoop)) {
	dout() << "Lattice::readRecPFSGs: processing subPFSG (" 
	       << nodeIndex << ", " << vocab.getWord(node->word) << ")\n";
      }

      Lattice * lat = getNonRecPFSG(node->word);
      if (!lat) {
	continue; 
      }
      implantLattice(nodeIndex, lat);

      dout() << "Lattice::readRecPFSGs: maxIndex (" 
	     << maxIndex << ")\n";
    }


    // release all the memory after all the subPFSGs have implanted.
    LHashIter<VocabIndex, Lattice *> subPFSGsIter(subPFSGs);
    VocabIndex word;
    while (Lattice **pt = subPFSGsIter.next(word)) {
      delete (Lattice *) (*pt); 
    }

    return true;
}

// return a flattened PFSG with nodeVocab as its PFSG name.
Lattice *
Lattice::getNonRecPFSG(VocabIndex nodeVocab)
{

    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::getNonRecPFSG: processing subPFSG (" 
	     << vocab.getWord(nodeVocab) << ")\n";
    }

    NodeQueue nodeQueue; 
    Lattice **pt = subPFSGs.find(nodeVocab); 
    if (!pt) {
      // this is a terminal node
      if (debug(DebugPrintInnerLoop)) {
	dout() << "Lattice::getNonRecPFSG: terminal node (" 
	       << nodeVocab << ")\n";
      }
      return 0;
    }
    
    Lattice * subPFSG = *pt; 
    LHashIter<NodeIndex, LatticeNode> nodeIter(subPFSG->nodes, nodeSort);
    NodeIndex nodeIndex;
    while (LatticeNode *node = nodeIter.next(nodeIndex)) { 
      Lattice **pt = subPFSGs.find(node->word);
      if (!(pt)) {
	// terminal node
	continue;
      }
      nodeQueue.addToNodeQueue(nodeIndex); 
    }

    while (nodeQueue.is_empty() == false) {
      nodeIndex = nodeQueue.popNodeQueue();
      // duplicate lat within this current lattice.
      LatticeNode * node = nodes.find(nodeIndex);

      if (debug(DebugPrintOutLoop)) {
	dout() << "Lattice::getNonRecPFSG: processing subPFSG (" 
	       << nodeIndex << ", " << vocab.getWord(node->word) << ")\n";
      }

      Lattice * lat = getNonRecPFSG(node->word);
      if (!lat) {
	continue;
      }
      subPFSG->implantLattice(nodeIndex, lat);
    }

    return subPFSG; 
}

// substitute the current occurence of lat->word in subPFSG with
// lat
Boolean
Lattice::implantLattice(NodeIndex vtmNodeIndex, Lattice *lat)
{

    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::implantLattice: processing subPFSG (" 
	     << lat->getName() << ", " << vtmNodeIndex << ")\n";
    }

    // going through lat to duplicate all its nodes in the current lattice
    LHashIter<NodeIndex, LatticeNode> nodeIter(lat->nodes, nodeSort);
    NodeIndex nodeIndex;

    while (LatticeNode *node = nodeIter.next(nodeIndex)) { 
      // make one copy of the subPFSG' nodes in this current PFSG
	LatticeNode *newNode = nodes.insert(maxIndex+nodeIndex);
	newNode->flags = node->flags; 
	VocabString wn = lat->vocab.getWord(node->word);
	newNode->word = vocab.addWord(wn);

	// clone transition
	{ // for incoming nodes
	  TRANSITER_T<NodeIndex,LatticeTransition> 
	    transIter(node->inTransitions);
	  NodeIndex fromNodeIndex;
	  while (LatticeTransition *trans = transIter.next(fromNodeIndex)) {
	    *(newNode->inTransitions.insert(maxIndex+fromNodeIndex)) = *trans;
	  }
	}
	{ // for outgoing nodes
	  TRANSITER_T<NodeIndex,LatticeTransition> 
	    transIter(node->outTransitions);
	  NodeIndex toNodeIndex;
	  while (LatticeTransition *trans = transIter.next(toNodeIndex)) {
	    *(newNode->outTransitions.insert(maxIndex+toNodeIndex)) = *trans;
	  }
	}
    }

    NodeIndex subInitial = lat->getInitial();
    LatticeNode * initialNode;
    if (!(initialNode = 
          nodes.find(maxIndex+subInitial))) {
        // check initial node
        if (debug(DebugPrintFatalMessages)) {
          dout()  << "Fatal Error in Lattice::implantLattice: (" 
		  << lat->getName() 
                  << ") undefined initial node Index ("
		  << subInitial << ")\n";
        }
        exit(-1); 
    } else {
      initialNode->word = Vocab_None;
    }

    NodeIndex subFinal = lat->getFinal();
    LatticeNode *finalNode; 
    if (!(finalNode = 
          nodes.find(maxIndex+subFinal))) {
        // only check the last final node
        if (debug(DebugPrintFatalMessages)) {
          dout()  << "Fatal Error in Lattice::implantLattice: (" 
		  << lat->getName() 
                  << ") undefined initial node Index ("
		  << subFinal << ")\n";
        } 
        exit(-1); 
    } else {
      finalNode->word = Vocab_None;
    }
    // done with checking the nodes

    // connecting initial and final subPFSG nodes with the existing PFSG
    {

	// processing incoming and outgoing nodes of node vtmNodeIndex;
	LatticeNode * node = nodes.find(vtmNodeIndex);

	NodeIndex subInitial = maxIndex+lat->getInitial();
	NodeIndex subFinal = maxIndex+lat->getFinal();
	{
	  TRANSITER_T<NodeIndex,LatticeTransition> 
	    transIter(node->inTransitions);
	  NodeIndex fromNodeIndex;
	  while (LatticeTransition *trans = transIter.next(fromNodeIndex)) {
	    insertTrans(fromNodeIndex, subInitial, *trans);
          }
	  // end of processing incoming nodes of node
	}
	{
	  // processing outgoing nodes of node
	  TRANSITER_T<NodeIndex,LatticeTransition> 
	    transIter(node->outTransitions);
	  NodeIndex toNodeIndex;
	  while (LatticeTransition *trans = transIter.next(toNodeIndex)) {
	    insertTrans(subFinal, toNodeIndex, *trans);
          }
	  // end of processing outgoing nodes of node
	}
    }

    removeNode(vtmNodeIndex);
    maxIndex += lat->getMaxIndex();

    return true; 
}

// substitute all the occurences of lat->word in subPFSG with
// lat
Boolean
Lattice::implantLatticeXCopies(Lattice *lat)
{

    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::implantLatticeXCopies: processing subPFSG (" 
	     << lat->getName() << ")\n";
    }

    unsigned numCopies = 0;
    {
      LHashIter<NodeIndex, LatticeNode> nodeIter(nodes, nodeSort);
      NodeIndex nodeIndex;
      VocabIndex subPFSGWord = vocab.getIndex(lat->getName());
      while (LatticeNode *node = nodeIter.next(nodeIndex)) { 
	if (node->word == subPFSGWord) {
	  numCopies++;
	}
      }  
    }

    // need to check whether numNodes is the same as lat->maxIndex
    unsigned numNodes = lat->getNumNodes(); 
    LHashIter<NodeIndex, LatticeNode> nodeIter(lat->nodes, nodeSort);
    NodeIndex nodeIndex;

    while (LatticeNode *node = nodeIter.next(nodeIndex)) { 
      // need to make numCopies
      // make "copies" of the subPFSG' nodes in this current PFSG
      for (unsigned k = 0; k < numCopies; k++) {
	LatticeNode *newNode = nodes.insert(maxIndex+k*numNodes+nodeIndex);
	newNode->word = node->word;
	newNode->flags = node->flags; 

	// clone transition
	{ // for incoming nodes
	  TRANSITER_T<NodeIndex,LatticeTransition> 
	    transIter(node->inTransitions);
	  NodeIndex fromNodeIndex;
	  while (transIter.next(fromNodeIndex)) {
	    newNode->inTransitions.insert(maxIndex+k*numNodes+fromNodeIndex);
	  }
	}
	{ // for outgoing nodes
	  TRANSITER_T<NodeIndex,LatticeTransition> 
	    transIter(node->outTransitions);
	  NodeIndex toNodeIndex;
	  while (transIter.next(toNodeIndex)) {
	    newNode->outTransitions.insert(maxIndex+k*numNodes+toNodeIndex);
	  }
	}
      }
    }

    NodeIndex subInitial = lat->getInitial();
    LatticeNode * initialNode;
    if (!(initialNode = 
          nodes.find(subInitial+maxIndex+(numCopies-1)*numNodes))) {
        // only check the last initial node
        if (debug(DebugPrintFatalMessages)) {
          dout()  << "Fatal Error in Lattice::implantLatticeXCopies: (" 
		  << lat->getName() 
                  << ") undefined initial node Index\n";
        }
        exit(-1); 
    }

    NodeIndex subFinal = lat->getFinal();
    LatticeNode *finalNode; 
    if (!(finalNode = 
          nodes.find(subFinal+maxIndex+(numCopies-1)*numNodes))) {
        // only check the last final node
        if (debug(DebugPrintFatalMessages)) {
          dout()  << "Fatal Error in Lattice::implantLatticeXCopies: "
		  << lat->getName() 
                  << "undefined final node Index\n";
        }
        exit(-1); 
    }
    // done with checking the nodes

    // connecting initial and final subPFSG nodes with the existing PFSG
    {
      unsigned k = 0;
      LHashIter<NodeIndex, LatticeNode> nodeIter(nodes, nodeSort);
      NodeIndex nodeIndex, fromNodeIndex, toNodeIndex;
      VocabIndex subPFSGWord = vocab.getIndex(lat->getName());
      for (nodeIndex = 0; nodeIndex< maxIndex; nodeIndex++) {
	
	LatticeNode * node = nodes.find(nodeIndex); 
	if (node->word == subPFSGWord) {
	  // processing incoming and outgoing nodes of node

	  NodeIndex subInitial = 
	    lat->getInitial()+maxIndex+(numCopies-1)*numNodes;
	  NodeIndex subFinal = 
	    lat->getFinal()+maxIndex+(numCopies-1)*numNodes;
	  {
	  TRANSITER_T<NodeIndex,LatticeTransition> 
	    transIter(node->inTransitions);
	  while (LatticeTransition *trans = transIter.next(fromNodeIndex)) {
	    insertTrans(fromNodeIndex, subInitial, *trans);
            removeTrans(fromNodeIndex, nodeIndex);
          }
	  // end of processing incoming nodes of node
	  }
	  {
	  // processing outgoing nodes of node
	  TRANSITER_T<NodeIndex,LatticeTransition> 
	    transIter(node->outTransitions);
	  while (LatticeTransition *trans = transIter.next(toNodeIndex)) {
	    insertTrans(subFinal, toNodeIndex, *trans);
            removeTrans(nodeIndex, toNodeIndex);
          }
	  // end of processing outgoing nodes of node
	  }
	  k++;
	  if (k > numCopies) {
	    break;
	  }
	} // end of processing k-th copy of implanted node
      } // end of going through the existing lattices
    }

    maxIndex += numCopies* (lat->getMaxIndex());

    return true; 
}

Boolean
Lattice::readPFSGs(File &file)
{
  if (debug(DebugPrintFunctionality)) {
    dout()  << "Lattice::readPFSGs: reading in nested PFSGs...\n"; 
  }

  char buffer[1024];
  char * subName = 0;

  // usually, it is the main PFSG
  readPFSG(file);

  while (fscanf(file, " name %1024s", buffer) == 1) {

    // name is the current PFSG name
    if (subName) {
      free((void *)subName); }
    subName = strdup(buffer);
    assert(subName != 0);

    // get the number of copies needed for each subPFSG.
    unsigned copies = 0; 
    {
      VocabIndex windex = vocab.addWord(subName);
      LHashIter<NodeIndex, LatticeNode> nodeIter(nodes);
      NodeIndex nodeIndex;
      while (LatticeNode *node = nodeIter.next(nodeIndex)) {
	if (node->word == windex) {
	    copies++;
	}
      }
    }
    
    unsigned numNodes;

    if (fscanf(file, " nodes %u", &numNodes) != 1) {
        if (debug(DebugPrintFatalMessages)) {
	  dout()  << "Fatal Error in Lattice::readPFSGs: "
		  << "missing nodes in sub-PFSG of the PFSG file\n";
	}
	exit(-1); 
    }
    
    
    // Parse node names and create nodes
    for (NodeIndex n = 0; n < numNodes; n ++) {
	if (fscanf(file, "%1024s", buffer) != 1) {
  	    if (debug(DebugPrintFatalMessages)) {
	      dout()  << "Fatal Error in Lattice::readPFSGs: "
		      << "missing node " << n << " name\n";
	    }
	    exit(-1); 
	}

	/*
	 * Map word string to VocabIndex
	 */
	VocabIndex windex;
	if (strcmp(buffer, "NULL") == 0) {
	    windex = Vocab_None;

	    LatticeNode *nullNode = nullNodes.insert(n+maxIndex); 
	    nullNode->flags = 0; 

	} else {
	    windex = vocab.addWord(buffer);
	}

	// make "copies" of the subPFSGs' nodes
	for (unsigned k = 0; k < copies; k++) {
	  LatticeNode *node = nodes.insert(maxIndex+k*numNodes+n);
	  node->word = windex;
	  node->flags = 0; 
	}
    }

    unsigned initialNodeIndex;
    if (fscanf(file, " initial %u", &initialNodeIndex) != 1) {
        if (debug(DebugPrintFatalMessages)) {
	  dout()  << "Fatal Error in Lattice::readPFSGs: "
		  << "missing initial node Index in PFSG file\n";
	}
	exit(-1); 
    }

    LatticeNode * initialNode;
    if (!(initialNode = 
	  nodes.find(initialNodeIndex+maxIndex+(copies-1)*numNodes))) {
        // only check the last initial node
        if (debug(DebugPrintFatalMessages)) {
	  dout()  << "Fatal Error in Lattice::readPFSGs: "
		  << "undefined initial node Index\n";
	}
	exit(-1); 
    }

    unsigned finalNodeIndex;
    if (fscanf(file, " final %u", &finalNodeIndex) != 1) {
        if (debug(DebugPrintFatalMessages)) {
	  dout()  << "Fatal Error in Lattice::readPFSGs: "
		  << "missing final node Index in PFSG file\n";
	}
	exit(-1); 
    }

    LatticeNode *finalNode; 
    if (!(finalNode = 
	  nodes.find(finalNodeIndex+maxIndex+(copies-1)*numNodes))) {
        // only check the last final node
        if (debug(DebugPrintFatalMessages)) {
	  dout()  << "Fatal Error in Lattice::readPFSGs: "
		  << "undefined final node Index\n";
	}
	exit(-1); 
    }

    // reading in all the transitions for the current subPFSG
    unsigned numTransitions;
    if (fscanf(file, " transitions %u", &numTransitions) != 1) {
        if (debug(DebugPrintFatalMessages)) {
	  dout()  << "Fatal Error in Lattice::readPFSGs: "
		  << "missing transitions in PFSG file\n";
	}
	exit(-1); 
    }
    
    for (unsigned i = 0; i < numTransitions; i ++) {
	unsigned from, to;
	double prob;

	if (fscanf(file, " %u %u %lf", &from, &to, &prob) != 3) {
  	    if (debug(DebugPrintFatalMessages)) {
	      dout()  << "Fatal Error in Lattice::readPFSGs: "
		      << "missing transition " << i << " in PFSG file\n";
	    }
	    exit(-1); 
	}

	for (unsigned k = 0; k < copies; k++) {
	  LatticeTransition t(IntlogToLogP(prob), 0);
	  insertTrans(maxIndex+k*numNodes+from, maxIndex+k*numNodes+to, t);
	}
	
    }
    if (debug(DebugPrintOutLoop)) {
      dout()  << "Lattice::readPFSGs: done with reading "
	      << copies << " copies of sub pfsg (" << subName << ")\n";
    }

    // going through all the nodes to flaten the current PFSG
    VocabIndex windex = vocab.addWord(subName);
    unsigned k = 0;            // number of copies consumed
    initialNodeIndex += maxIndex;
    finalNodeIndex += maxIndex;
    LHashIter<NodeIndex, LatticeNode> nodeIter(nodes, nodeSort);
    NodeIndex nodeIndex;
    LatticeNode *node;
    while ((node = nodeIter.next(nodeIndex)) && (k < copies)) {
    
      // do a substitution
      if (node->word == windex) {

	LatticeNode * initSubPFSGNode = nodes.find(initialNodeIndex);
	NodeIndex fromNodeIndex, toNodeIndex;

	// connecting incoming nodes to initialNodeIndex
	{
	  TRANSITER_T<NodeIndex,LatticeTransition>
	    transIter(node->inTransitions); 
	  while (LatticeTransition *trans = transIter.next(fromNodeIndex)) {
	    *(initSubPFSGNode->inTransitions.insert(fromNodeIndex)) = *trans;
	  }
	}
	if (debug(DebugPrintInnerLoop)) {
	  dout()  << "Lattice::readPFSGs: done with removing orig connection\n";
	}
	{
	  // add initSubPFSG to the incoming nodes
	  TRANSITER_T<NodeIndex,LatticeTransition> 
	    transIter(initSubPFSGNode->inTransitions); 
	  while (LatticeTransition *trans = transIter.next(fromNodeIndex)) {
	    LatticeNode * fromNode = nodes.find(fromNodeIndex);
	    *(fromNode->outTransitions.insert(initialNodeIndex)) = *trans;
	  }
	}
	if (debug(DebugPrintInnerLoop)) {
	  dout()  << "Lattice::readPFSGs: done with adding new connection\n"; 
	}

	// connecting subPfsg->final to outgoing nodes
	LatticeNode * finalSubPFSGNode = nodes.find(finalNodeIndex);
	{
	  TRANSITER_T<NodeIndex,LatticeTransition>
	    transIter(node->outTransitions); 
	  while (LatticeTransition *trans = transIter.next(toNodeIndex)) {
	    *(finalSubPFSGNode->outTransitions.insert(toNodeIndex)) = *trans;
	  }
	}
	if (debug(DebugPrintInnerLoop)) {
	  dout()  << "Lattice::readPFSGs: done with "
		  << "removing its old transitions for outgoing\n"; 
	}

	{
	  // add finalSubPFSG to the outgoing nodes
	  TRANSITER_T<NodeIndex,LatticeTransition> 
	    transIter(finalSubPFSGNode->outTransitions); 
	  while (LatticeTransition *trans = transIter.next(toNodeIndex)) {
	    LatticeNode * toNode = nodes.find(toNodeIndex);
	    *(toNode->inTransitions.insert(finalNodeIndex)) = *trans;
	  }
	}
	if (debug(DebugPrintInnerLoop)) {
	  dout()  << "Lattice::readPFSGs: done with "
		  << "adding new connection for outgoing\n"; 
	}

	// removing this pseudo word, finally
	removeNode(nodeIndex); 

	if (debug(DebugPrintInnerLoop)) {
	  dout()  << "Lattice::readPFSGs: done with "
		  << "removing node (" << nodeIndex << ")\n"; 
	}

	k++;
	initialNodeIndex += numNodes;
	finalNodeIndex += numNodes;

	if (debug(DebugPrintInnerLoop)) {
	  dout()  << "Lattice::readPFSGs: done with "
		  << "single flatening (" << subName << ")\n"; 
	}

      } // end of single flatening
    } // end of flatening loop

    maxIndex += copies*numNodes;
  }

  return true;
}


Boolean
Lattice::readPFSG(File &file)
{
    char buffer[1024];

    if (debug(DebugPrintOutLoop)) {
        dout() << "Lattice::readPFSG: reading in PFSG....\n"; 
    }

    if (fscanf(file, " name %1024s", buffer) != 1) {
        if (debug(DebugPrintFatalMessages)) {
	  dout()  << "Fatal Error in Lattice::readPFSG: "
		  << "missing name in PFSG file\n";
	}
	exit(-1); 
    }

    if (name) {
      free((void *)name); }
    name = strdup(buffer);
    assert(name != 0);

    unsigned numNodes;

    if (fscanf(file, " nodes %u", &numNodes) != 1) {
        if (debug(DebugPrintFatalMessages)) {
	  dout()  << "Fatal Error in Lattice::readPFSG: "
		  << "missing nodes in PFSG file\n";
	}
	exit(-1); 
    }

    maxIndex = numNodes; 
    /*
     * Parse node names and create nodes
     */
    for (NodeIndex n = 0; n < numNodes; n ++) {
	if (fscanf(file, "%1024s", buffer) != 1) {
  	    if (debug(DebugPrintInnerLoop)) {
	      dout()  << "Fatal Error in Lattice::readPFSG: "
		      << "missing node " << n << " name\n";
	    }
	    exit(-1); 
	}

	/*
	 * Map word string to VocabIndex
	 */
	VocabIndex windex;
	if (strcmp(buffer, "NULL") == 0) {
	    windex = Vocab_None;

	    LatticeNode *nullNode = nullNodes.insert(n); 
	    nullNode->flags = 0; 

	} else {
	    windex = vocab.addWord(buffer);
	}

	LatticeNode *node = nodes.insert(n);

	node->word = windex;
	node->flags = 0; 
    }

    unsigned initialNodeIndex;
    if (fscanf(file, " initial %u", &initialNodeIndex) != 1) {
        if (debug(DebugPrintFatalMessages)) {
	  dout()  << "Fatal Error in Lattice::readPFSG: "
		  << "missing initial node Index in PFSG file\n";
	}
	exit(-1); 
    }

    LatticeNode * initialNode;
    if (! (initialNode = nodes.find(initialNodeIndex))) {
        if (debug(DebugPrintFatalMessages)) {
	  dout()  << "Fatal Error in Lattice::readPFSG: "
		  << "undefined initial node Index\n";
	}
	exit(-1); 
    }

    initial = initialNodeIndex;

    unsigned finalNodeIndex;
    if (fscanf(file, " final %u", &finalNodeIndex) != 1) {
        if (debug(DebugPrintFatalMessages)) {
	  dout()  << "Fatal Error in Lattice::readPFSG: "
		  << "missing final node Index in PFSG file\n";
	}
	exit(-1); 
    }

    LatticeNode *finalNode; 
    if (! (finalNode = nodes.find(finalNodeIndex))) {
        if (debug(DebugPrintFatalMessages)) {
	  dout()  << "Fatal Error in Lattice::readPFSG: "
		  << "undefined final node Index\n";
	}
	exit(-1); 
    }

    final = finalNodeIndex;

    // the definitions for ssIndex and seIndex are located in
    // class Vocab of file /home/srilm/devel/lm/src/Vocab.h
    if (top) {
      initialNode->word = vocab.ssIndex; 
      finalNode->word = vocab.seIndex; 
      nullNodes.remove(initialNodeIndex); 
      nullNodes.remove(finalNodeIndex); 

      top = 0;
    }

    unsigned numTransitions;
    if (fscanf(file, " transitions %u", &numTransitions) != 1) {
        if (debug(DebugPrintFatalMessages)) {
	  dout()  << "Fatal Error in Lattice::readPFSG: "
		  << "missing transitions in PFSG file\n";
	}
	exit(-1); 
    }
    
    for (unsigned i = 0; i < numTransitions; i ++) {
	unsigned from, to;
	double prob;

	if (fscanf(file, " %u %u %lf", &from, &to, &prob) != 3) {
  	    if (debug(DebugPrintFatalMessages)) {
	      dout()  << "Fatal Error in Lattice::readPFSG: "
		      << "missing transition " << i << " in PFSG file\n";
	    }
	    exit(-1); 
	}

	LatticeNode *fromNode = nodes.find(from);
	if (!fromNode) {
  	    if (debug(DebugPrintFatalMessages)) {
	      dout()  << "Fatal Error in Lattice::readPFSG: "
		      << "undefined source node in transition " << i << "\n";
	    }
	    exit(-1); 
	}

	LatticeNode *toNode = nodes.find(to);
	if (!toNode) {
  	    if (debug(DebugPrintFatalMessages)) {
	      dout()  << "Fatal Error in Lattice::readPFSG: "
		      << "undefined target node in transition " << i << "\n";
	    }
	    exit(-1); 
	}

	LogP weight = IntlogToLogP(prob);

	LatticeTransition *trans = fromNode->outTransitions.insert(to);
	trans->weight = weight;
	trans->flags = 0;

	trans = toNode->inTransitions.insert(from);
	trans->weight = weight;
	trans->flags = 0;
    }

    return true;
}

#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_SARRAY(NodeIndex,unsigned);
#endif
Boolean
Lattice::writeCompactPFSG(File &file)
{
    if (debug(DebugPrintFunctionality)) {
      dout()  << "Lattice::writeCompactPFSG: writing ";
    }

    fprintf(file, "name %s\n", name);
	
    /*
     * We remap the internal node indices to consecutive unsigned integers
     * to allow a compact output representation.
     * We iterate over all nodes, renumbering them, and also counting the
     * number of transitions overall.
     */

    // map nodeIndex to unsigned
    SArray<NodeIndex,unsigned> nodeMap;
    unsigned numNodes = 0;
    unsigned numTransitions = 0;

    fprintf(file, "nodes %d", getNumNodes());

    LHashIter<NodeIndex, LatticeNode> nodeIter(nodes, nodeSort);
    NodeIndex nodeIndex;

    while (LatticeNode *node = nodeIter.next(nodeIndex)) {
	*nodeMap.insert(nodeIndex) = numNodes ++;
	numTransitions += node->outTransitions.numEntries();

	VocabString nodeName = "NULL";

	if (nodeIndex != initial &&
	    nodeIndex != final &&
	    node->word != Vocab_None) 
	{
	    nodeName = vocab.getWord(node->word);
	}
	fprintf(file, " %s", nodeName);
    }

    fprintf(file, "\n");

    fprintf(file, "initial %u\n", *nodeMap.find(initial));
    fprintf(file, "final %u\n", *nodeMap.find(final));

    fprintf(file, "transitions %u\n", numTransitions);

    if (debug(DebugPrintFunctionality)) {
      dout()  << getNumNodes() << " nodes, "
	      << numTransitions << " transitions\n";
    }

    nodeIter.init(); 
    while (LatticeNode *node = nodeIter.next(nodeIndex)) {
 	NodeIndex toNode;

	TRANSITER_T<NodeIndex,LatticeTransition>
	  transIter(node->outTransitions);
	while (LatticeTransition *trans = transIter.next(toNode)) {
	    unsigned int *toNodeId = nodeMap.find(toNode); 
	    if (! (toNodeId)) {
	        if (debug(DebugPrintInnerLoop)) {
		  dout() << "nonFatal Error in Lattice::writeCompactPFSG: "
			 << "node Map problem" << toNode <<"\n";
		}
		continue;
	    }

	    int logToPrint = LogPtoIntlog(trans->weight);

	    if (logToPrint < minIntlog) {
		logToPrint = minIntlog;
	    }

	    fprintf(file, "%u %u %d\n",
			*nodeMap.find(nodeIndex),
			*toNodeId, 
			logToPrint);
	}
    }

    fprintf(file, "\n");

    return true;
}

Boolean
Lattice::writePFSG(File &file)
{
    if (debug(DebugPrintFunctionality)) {
      dout()  << "Lattice::writePFSG: writing ";
    }

    fprintf(file, "name %s\n", name);
	
    NodeIndex nodeIndex;

    unsigned numTransitions = 0;

    fprintf(file, "nodes %d", maxIndex);

    for (nodeIndex = 0; nodeIndex < maxIndex; nodeIndex ++) {
        LatticeNode *node = nodes.find(nodeIndex);

	if (node) {
	   numTransitions += node->outTransitions.numEntries();
	}

	VocabString nodeName = "NULL";

	if (nodeIndex != initial &&
	    nodeIndex != final &&
	    node && node->word != Vocab_None) 
	{
	    nodeName = vocab.getWord(node->word);
	}
	fprintf(file, " %s", nodeName);
    }

    fprintf(file, "\n");

    fprintf(file, "initial %u\n", initial);
    fprintf(file, "final %u\n", final);

    fprintf(file, "transitions %u\n", numTransitions);

    if (debug(DebugPrintFunctionality)) {
      dout()  << maxIndex << " nodes, " << numTransitions << " transitions\n";
    }

    LHashIter<NodeIndex, LatticeNode> nodeIter(nodes, nodeSort);

    while (LatticeNode *node = nodeIter.next(nodeIndex)) {
 	NodeIndex toNode;

	TRANSITER_T<NodeIndex,LatticeTransition>
	  transIter(node->outTransitions);
	while (LatticeTransition *trans = transIter.next(toNode)) {
	    int logToPrint = LogPtoIntlog(trans->weight);

	    if (logToPrint < minIntlog) {
		logToPrint = minIntlog;
	    }

	    fprintf(file, "%u %u %d\n",
			nodeIndex,
			toNode, 
			logToPrint);
	}
    }

    fprintf(file, "\n");

    return true;
}

unsigned
Lattice::getNumTransitions()
{
    unsigned numTransitions = 0;

    LHashIter<NodeIndex, LatticeNode> nodeIter(nodes);
    NodeIndex nodeIndex;

    while (LatticeNode *node = nodeIter.next(nodeIndex)) {
	numTransitions += node->outTransitions.numEntries();
    }

    return numTransitions;
}

// this is for debugging purpose
Boolean
Lattice::printNodeIndexNamePair(File &file)
{
    if (debug(DebugPrintFunctionality)) {
      dout() << "Lattice::printNodeIndexNamePair: "
	     << "printing Index-Name pairs!\n";
    }

    LHashIter<NodeIndex, LatticeNode> nodeIter(nodes, nodeSort);
    NodeIndex nodeIndex;
    while (LatticeNode *node = nodeIter.next(nodeIndex)) {

        VocabString nodeName =
		(node->word == Vocab_None) ? "NULL" :
					vocab.getWord(node->word);
	fprintf(file, "%d %s (%d)\n", nodeIndex, nodeName, node->word);
    }

    return true;
}

Boolean
Lattice::readPFSGFile(File &file)
{
    Boolean val; 

    while (fgetc(file) != EOF) {

      fseek(file, -1, SEEK_CUR); 

      val = readPFSG(file); 
      
      while (fgetc(file) == '\n' || fgetc(file) == ' ') {} 

      fseek(file, -1, SEEK_CUR); 

    }

    return val;
}

Boolean
Lattice::writePFSGFile(File &file)
{
    return true;
}

/* **************************************************
   some more complex functions of Lattice class
   ************************************************** */

// *****************************************************
// *******************algorithm*************************
// going through all the Null nodes, 
//   if nodeIndex is the initial or final node, skip, 
//   if nodeIndex is not a Null node, skip
//   
//   if nodeIndex is a Null node, 
//     going through all the inTransitions,
//       collect weight for the inTransition,
//       collect the source node s,
//       remove the inTransition, 
//       going through all the outTransitions,
//         collect the weight for the outTransition,
//         combine it with the inTransition weight,
//         insert an outTransition to s,
//         remove the outTransition

LogP 
Lattice::detectSelfLoop(NodeIndex nodeIndex)
{
    LogP base = 10;
    LogP weight = unit();

    LatticeNode *node = nodes.find(nodeIndex);
    if (!node) {
      if (debug(DebugPrintFatalMessages)) {
	dout() << "Fatal Error in Lattice::detectSelfLoop: "
	       << nodeIndex << "\n";
      }
      exit(-1); 
    }

    LatticeTransition *trans;

    trans = node->outTransitions.find(nodeIndex);

    if (!trans) {
      return weight;
    } else {
      weight = combWeights(trans->weight, weight);
    }

    if (!weight) {
      return weight; }
    else {
      return (-log(1-exp(weight*log(base)))/log(base)); 
    }
}

// it removes all the nodes that have given word
Boolean
Lattice::removeAllXNodes(VocabIndex xWord)
{
    if (debug(DebugPrintFunctionality)) {
      VocabString nodeName =
	(xWord == Vocab_None) ? "NULL" : vocab.getWord(xWord);
      dout() << "Lattice::removeAllXNodes: "
	     << "removing all " << nodeName << endl;
    }

    LHashIter<NodeIndex, LatticeNode> nodeIter(nodes);
    NodeIndex nodeIndex;
    while (LatticeNode *node = nodeIter.next(nodeIndex)) {

      if (debug(DebugPrintInnerLoop)) {
	dout() << "Lattice::removeAllXNodes: processing nodeIndex " 
	       << nodeIndex << "\n";
      }

      if (nodeIndex == final || nodeIndex == initial) {
	  continue; 
      }

      // testing 
      // VocabString nodeName =
      // (node->word == Vocab_None) ? "NULL" : vocab.getWord(node->word);
      // dout() << " Word name " << nodeName << "(" << node->word << ")\n";
      // end of testing
      
      if (node->word == xWord) {
	// this node is a Null node
	if (debug(DebugPrintInnerLoop)) {
	  dout() << "Lattice::removeAllXNodes: " 
		 << "remove node " << nodeIndex << "\n";
	}

	LogP loopweight = detectSelfLoop(nodeIndex); 

	// remove the current node, all the incoming and outgoing edges
	//    and create new edges
	// Notice that all the edges are recorded in two places: 
	//    inTransitions and outTransitions
	TRANSITER_T<NodeIndex,LatticeTransition>
	  inTransIter(node->inTransitions);
	NodeIndex fromNodeIndex;
	while (LatticeTransition *inTrans = inTransIter.next(fromNodeIndex)) {

	  if (debug(DebugPrintInnerLoop)) {
	    dout() << "Lattice::removeAllXNodes: " 
		   << "  fromNodeIndex " << fromNodeIndex << "\n";
	  }

	  LogP inWeight = inTrans->weight;

	  if (fromNodeIndex == nodeIndex) {
	    continue; 
	  }

	  TRANSITER_T<NodeIndex,LatticeTransition>
	    outTransIter(node->outTransitions);
	  NodeIndex toNodeIndex;
	  while (LatticeTransition *trans = outTransIter.next(toNodeIndex)) {

	    if (debug(DebugPrintInnerLoop)) {
	      dout() << "Lattice::removeAllXNodes: " 
		     << "     toNodeIndex " << toNodeIndex << "\n";
	    }

	    if (toNodeIndex == nodeIndex) {
	      continue; 
	    }

	    // loopweight is 1 in the prob domain and 
	    // loopweight is 0 in the log domain, if no loop 
	    //     for the current node
	    LogP weight = combWeights(inWeight, trans->weight); 
	    weight = combWeights(weight, loopweight); 

	    unsigned flag = 0;
	    // record where pause nodes were eliminated
	    if (xWord != Vocab_None && xWord == vocab.pauseIndex) {
		flag = pauseTFlag;
	    }

	    LatticeTransition t(weight, flag);
	    // new transition inherits properties from both parents
	    t.flags |= inTrans->flags | trans->flags; 
	    // ... except for "direct (non-pause) connection"
	    t.flags &= ~directTFlag;
	    // a non-pause connection is carried over if we are removing
	    // a null-node and each of the joined transitions was direct
	    if (xWord == Vocab_None &&
		inTrans->flags&directTFlag && trans->flags&directTFlag)
	    {
		t.flags |= directTFlag;
	    }

	    insertTrans(fromNodeIndex, toNodeIndex, t);
	  }
	} // end of inserting new edges

	// deleting xWord node
	removeNode(nodeIndex);

      } // end of processing xWord node
    }

    return true; 
}

Boolean
Lattice::recoverPause(NodeIndex nodeIndex, Boolean loop)
{
    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::recoverPause: " 
	     << "processing nodeIndex " << nodeIndex << "\n";
    }

    // this array is created to avoid inserting new elements into 
    //   temporary index, while iterating over it.
    TRANS_T<NodeIndex,LatticeTransition> newTransitions; 

    // going throught all the successive nodes of the current node (nodeIndex)
    LatticeNode *node = findNode(nodeIndex); 
    TRANSITER_T<NodeIndex,LatticeTransition> 
      outTransIter(node->outTransitions);
    NodeIndex toNodeIndex; 
    while (LatticeTransition *trans = outTransIter.next(toNodeIndex)) {
      // processing nodes at the next level 
      LatticeNode * toNode = findNode(toNodeIndex); 

      // if the current edge is a pause edge. insert a pause node
      // and its two edges.
      if (trans->getFlag(pauseTFlag)) {
	LatticeNode * toNode = findNode(toNodeIndex); 

	NodeIndex newNodeIndex = dupNode(vocab.pauseIndex, 0); 
	LatticeNode *newNode = findNode(newNodeIndex); 
	LatticeTransition *newTrans = newTransitions.insert(newNodeIndex); 
	newTrans->flags = 0; 
	newTrans->weight = trans->weight; 

	LatticeTransition t(unit(), 0);
	insertTrans(newNodeIndex, toNodeIndex, t);
	// add self-loop
	if (loop) { 
	  insertTrans(newNodeIndex, newNodeIndex, t);
	}
	
	if (!trans->getFlag(directTFlag)) {
	  removeTrans(nodeIndex, toNodeIndex); 
	}
      }
    } // end of outGoing edge loop

    TRANSITER_T<NodeIndex,LatticeTransition> 
      newTransIter(newTransitions);
    NodeIndex newNodeIndex;
    while (LatticeTransition *newTrans = newTransIter.next(newNodeIndex)) {
      LatticeTransition t(newTrans->weight, 0);
      insertTrans(nodeIndex, newNodeIndex, t);
    }

    return true;
}


/* ********************************************************
   recover the pauses that have been removed.
   ******************************************************** */
Boolean
Lattice::recoverPauses(Boolean loop)
{
    if (debug(DebugPrintFunctionality)) {
	dout() << "Lattice::recoverPauses: recovering pauses\n";
    }

    unsigned numNodes = getNumNodes();
    NodeIndex sortedNodes[numNodes];
    unsigned numReachable = sortNodes(sortedNodes);

    if (numReachable != numNodes) {
	dout() << "Lattice::recoverPauses: "
	       << "warning: there are " << (numNodes - numReachable)
	       << " unreachable nodes\n";
    }

    for (unsigned i = 0; i < numReachable; i ++) {
	recoverPause(sortedNodes[i], loop);
    }

    return true; 
}

Boolean
Lattice::recoverCompactPause(NodeIndex nodeIndex, Boolean loop)
{
    unsigned firstPauTrans = 1;

    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::recoverCompactPause: "
	     << "processing node: ("
	     << nodeIndex << ")\n"; 
    }

    NodeIndex newNodeIndex;

    // going through all the successive nodes of the current node (nodeIndex)
    LatticeNode *node = findNode(nodeIndex); 
    TRANSITER_T<NodeIndex,LatticeTransition> 
      outTransIter(node->outTransitions);
    NodeIndex toNodeIndex; 
    while (LatticeTransition *trans = outTransIter.next(toNodeIndex)) {
      // processing nodes at the next level 
      if (debug(DebugPrintInnerLoop)) {
	dout() << "Lattice::recoverCompactPause: "
	       << "  the toNode index " << toNodeIndex << "\n";
      }

      LatticeNode * toNode = findNode(toNodeIndex);
      LogP weight = trans->weight; 
      Boolean direct = trans->getFlag(directTFlag); 

      // if the current edge is a pause edge. insert a pause node
      // and its two edges.
      if (trans->getFlag(pauseTFlag)) {
	if (debug(DebugPrintInnerLoop)) {
	  dout() << "Lattice::recoverCompactPause: "
		 << "inserting pause node between ("
		 << nodeIndex << ", " << toNodeIndex << ")\n"; 
	}

	LatticeNode * toNode = findNode(toNodeIndex); 
	
	if (firstPauTrans) { 
	  firstPauTrans = 0; 
	  newNodeIndex = dupNode(vocab.pauseIndex, 0); 
	  if (debug(DebugPrintInnerLoop)) {
	    dout() << "Lattice::recoverCompactPause: "
		   << "new node index ("
		   << newNodeIndex << ")\n"; 
	  }
	}

	LatticeTransition t(weight, 0);
	insertTrans(newNodeIndex, toNodeIndex, t);
	
	if (!direct) {
	  removeTrans(nodeIndex, toNodeIndex); 
	}
      }
    } // end of outGoing edge loop

    // it means that there are outgoing edges with pauses.
    if (!firstPauTrans) { 
      LatticeTransition t(unit(), 0);
      insertTrans(nodeIndex, newNodeIndex, t);
      if (loop) {
	insertTrans(newNodeIndex, newNodeIndex, t);
      }
    }

    return true;
}

/* this method will create separate pauses for each trans that have 
   pause mark on.
   it works well.
   */ 

/* this is to recover compact pauses in lattice
 */
Boolean
Lattice::recoverCompactPauses(Boolean loop)
{
    if (debug(DebugPrintFunctionality)) {
      dout() << "Lattice::recoverCompactPauses: recovering Compact Pauses\n";
    }

    unsigned numNodes = getNumNodes();
    NodeIndex sortedNodes[numNodes];
    unsigned numReachable = sortNodes(sortedNodes);

    if (numReachable != numNodes) {
	dout() << "Lattice::recoverCompactPauses: "
	       << "warning: there are " << (numNodes - numReachable)
	       << " unreachable nodes\n";
    }

    for (unsigned i = 0; i < numReachable; i ++) {
	recoverCompactPause(sortedNodes[i], loop);
    }

    return true; 
}


/* **************************************************************
   minimization of lattices
   **************************************************************
   */

/*
 * Compare two transition lists for equality
 */
static Boolean
compareTransitions(const TRANS_T<NodeIndex,LatticeTransition> &transList1,
		   const TRANS_T<NodeIndex,LatticeTransition> &transList2)
{
    if (transList1.numEntries() != transList2.numEntries()) {
	return false;
    }

#ifdef USE_SARRAY
    // SArray already sorts indices internally
    TRANSITER_T<NodeIndex,LatticeTransition> transIter1(transList1);
    TRANSITER_T<NodeIndex,LatticeTransition> transIter2(transList2);

    NodeIndex node1, node2;
    while (transIter1.next(node1)) {
	if (!transIter2.next(node2) || node1 != node2) {
	    return false;
	}
    }
#else
    // assume random access is efficient
    TRANSITER_T<NodeIndex,LatticeTransition> transIter1(transList1);
    NodeIndex node1;
    while (transIter1.next(node1)) {
	if (!transList2.find(node1)) {
	    return false;
	}
    }
#endif

    return true;
}

/*
 * Merge two nodes 
 */
void
Lattice::mergeNodes(NodeIndex nodeIndex1, NodeIndex nodeIndex2,
			LatticeNode *node1, LatticeNode *node2, Boolean maxAdd)
{
    if (nodeIndex1 == nodeIndex2) {
	return;
    }

    if (node1 == 0) node1 = findNode(nodeIndex1);
    if (node2 == 0) node2 = findNode(nodeIndex2);

    assert(node1 != 0 && node2 != 0);

    if (debug(DebugPrintOutLoop)) {
      VocabString nodeName1 =
	(node1->word == Vocab_None) ? "NULL" : getWord(node1->word);
      VocabString nodeName2 =
	(node2->word == Vocab_None) ? "NULL" : getWord(node2->word);
      dout() << "Lattice::mergeNodes: "
	     << "      i.e., (" << nodeName1 << ", " << nodeName2 << ")\n";
    }

    // add the incoming trans of nodeIndex2 to nodeIndex1
    TRANSITER_T<NodeIndex,LatticeTransition> 
      inTransIter2(node2->inTransitions);
    NodeIndex fromNodeIndex;
    while (LatticeTransition *trans = inTransIter2.next(fromNodeIndex)) {
      insertTrans(fromNodeIndex, nodeIndex1, *trans, maxAdd);
    }

    // add the outgoing trans of nodeIndex2 to nodeIndex1
    TRANSITER_T<NodeIndex,LatticeTransition> 
      outTransIter2(node2->outTransitions);
    NodeIndex toNodeIndex;
    while (LatticeTransition *trans = outTransIter2.next(toNodeIndex)) {
      insertTrans(nodeIndex1, toNodeIndex, *trans, maxAdd);
    }

    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::mergeNodes: "
	     << "(" << nodeIndex2 << ") has been removed\n";
    }
    // delete this redudant node.
    removeNode(nodeIndex2); 
}

/*
 * Try merging successors of nodeIndex
 */
void
Lattice::packNodeF(NodeIndex nodeIndex, Boolean maxAdd)
{
    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::packNodeF: "
	     << "processing (" << nodeIndex << ") ***********\n"; 
    }

    LatticeNode *node = findNode(nodeIndex); 
    // skip nodes that have already beend deleted
    if (!node) {
      return;
    }

    if (debug(DebugPrintOutLoop)) {
      VocabString nodeName =
	(node->word == Vocab_None) ? "NULL" : getWord(node->word);
      dout() << "Lattice::packNodeF: "
	     << "      i.e., (" << nodeName << ")\n";
    }

    unsigned numTransitions = node->outTransitions.numEntries();
    NodeIndex nodeList[numTransitions]; 
    VocabIndex wordList[numTransitions];
    
    // going in a forward direction
    // collect all the out-nodes of nodeIndex, because we need to 
    // be able to delete transitions while iterating over them later
    // Also, store the words on each node to allow quick equality checks
    // without findNode() later.  Note we cannot save the findNode() result
    // itself since node objects may more around as a result of deletions.
    TRANSITER_T<NodeIndex,LatticeTransition> 
      outTransIter(node->outTransitions);
    unsigned position = 0; 
    NodeIndex toNodeIndex;
    while (outTransIter.next(toNodeIndex)) {
	wordList[position] = findNode(toNodeIndex)->word;
	nodeList[position] = toNodeIndex; 
	position ++;
    }

    // do a pair-wise comparison for all the successor nodes.
    for (unsigned i = 0; i < numTransitions; i ++) {
	// check if node has been merged
	if (Map_noKeyP(nodeList[i])) continue;

	for (unsigned j = i + 1; j < numTransitions; j ++) {
	    LatticeNode *nodeI, *nodeJ;

	    if (!Map_noKeyP(nodeList[j]) &&
		wordList[i] == wordList[j] &&
		(nodeI = findNode(nodeList[i]),
		 nodeJ = findNode(nodeList[j]),
		 compareTransitions(nodeI->inTransitions,
				    nodeJ->inTransitions)))
	    {
		mergeNodes(nodeList[i], nodeList[j], nodeI, nodeJ, maxAdd);

		// mark node j as merged for check above
		Map_noKey(nodeList[j]);
	    }
	}
    }
}

/*
 * Try merging predecessors of nodeIndex
 */
void
Lattice::packNodeB(NodeIndex nodeIndex, Boolean maxAdd)
{
    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::packNodeB: "
	     << "processing (" << nodeIndex << ") ***********\n"; 
    }

    LatticeNode *node = findNode(nodeIndex); 
    // skip nodes that have already beend deleted
    if (!node) {
      return;
    }

    if (debug(DebugPrintOutLoop)) {
      VocabString nodeName =
	(node->word == Vocab_None) ? "NULL" : getWord(node->word);
      dout() << "Lattice::packNodeB: "
	     << "      i.e., (" << nodeName << ")\n";
    }

    unsigned numTransitions = node->inTransitions.numEntries();
    NodeIndex nodeList[numTransitions]; 
    VocabIndex wordList[numTransitions];
    
    // going in a reverse direction
    // collect all the in-nodes of nodeIndex, because we need to 
    // be able to delete transitions while iterating over them
    // Also, store the words on each node to allow quick equality checks
    // without findNode() later.  Note we cannot save the findNode() result
    // itself since node objects may more around as a result of deletions.
    TRANSITER_T<NodeIndex,LatticeTransition> 
      inTransIter(node->inTransitions);
    unsigned position = 0; 
    NodeIndex fromNodeIndex;
    while (inTransIter.next(fromNodeIndex)) {
	wordList[position] = findNode(fromNodeIndex)->word;
	nodeList[position] = fromNodeIndex; 
	position ++;
    }

    // do a pair-wise comparison for all the predecessor nodes.
    for (unsigned i = 0; i < numTransitions; i ++) {
	// check if node has been merged
	if (Map_noKeyP(nodeList[i])) continue;

	for (unsigned j = i + 1; j < numTransitions; j ++) {
	    LatticeNode *nodeI, *nodeJ;

	    if (!Map_noKeyP(nodeList[j]) &&
	 	wordList[i] == wordList[j] &&
		(nodeI = findNode(nodeList[i]),
		 nodeJ = findNode(nodeList[j]),
		 compareTransitions(nodeI->outTransitions,
				    nodeJ->outTransitions)))
	    {
		mergeNodes(nodeList[i], nodeList[j], nodeI, nodeJ, maxAdd);

		// mark node j as merged for check above
		Map_noKey(nodeList[j]);
	    }
	}
    }
}

/* ******************************************************** 

   A straight forward implementation for packing bigram lattices.
   combine nodes when their incoming or outgoing node sets are the
   same.

   ******************************************************** */
Boolean 
Lattice::simplePackBigramLattice(unsigned iters, Boolean maxAdd)
{
    // keep track of number of node to know when to stop iteration
    unsigned numNodes = getNumNodes();

    Boolean onlyOne = false;

    if (iters == 0) {
	iters = 1;
	onlyOne = true; // do only one backward pass
    }
	
    if (debug(DebugPrintFunctionality)) {
      dout() << "Lattice::simplePackBigramLattice: "
	     << "starting packing...."
	     << " (" << numNodes  << " nodes)\n";
    }

    while (iters-- > 0) { 
      NodeIndex sortedNodes[numNodes];
      unsigned numReachable = sortNodes(sortedNodes, true);

      if (numReachable != numNodes) {
	dout() << "Lattice::simplePackBigramLattice: "
	       << "warning: there are " << (numNodes - numReachable)
	       << " unreachable nodes\n";
      }

      for (unsigned i = 0; i < numReachable; i ++) {
	packNodeB(sortedNodes[i], maxAdd);
      }

      unsigned newNumNodes = getNumNodes();
      
      // finish the Backward reduction
      if (debug(DebugPrintFunctionality)) {
	dout() << "Lattice::simplePackBigramLattice: "
	       << "done with B Reduction"
	       << " (" << newNumNodes  << " nodes)\n";
      }

      if (onlyOne) {
	break;
      }

      // now sort into forward topological order
      numReachable = sortNodes(sortedNodes, false);

      if (numReachable != newNumNodes) {
	dout() << "Lattice::simplePackBigramLattice: "
	       << "warning: there are " << (newNumNodes - numReachable)
	       << " unreachable nodes\n";
      }

      for (unsigned i = 0; i < numReachable; i ++) {
	packNodeF(sortedNodes[i], maxAdd);
      }

      newNumNodes = getNumNodes();

      if (debug(DebugPrintFunctionality)) {
	dout() << "Lattice::simplePackBigramLattice: "
	       << "done with F Reduction"
	       << " (" << newNumNodes  << " nodes)\n";
      }
      // finish one F-B iteration

      // check that lattices got smaller -- if not, stop
      if (newNumNodes == numNodes) {
	break;
      }
      numNodes = newNumNodes;
    }

    return true; 
}

/* ********************************************************
   this procedure tries to compare two nodes to see whether their
   outgoing node sets overlap more than x.
   ******************************************************** */

Boolean 
Lattice::approxMatchInTrans(NodeIndex nodeIndexI, NodeIndex nodeIndexJ,
			     int overlap)
{
    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::approxMatchInTrans: "
	     << "try to match (" << nodeIndexI
	     << ", " << nodeIndexJ << ") with overlap "
	     << overlap << "\n"; 
    }

    LatticeNode * nodeI = findNode(nodeIndexI); 
    LatticeNode * nodeJ = findNode(nodeIndexJ); 

    if (debug(DebugPrintOutLoop)) {
      VocabString nodeNameI =
	(nodeI->word == Vocab_None) ? "NULL" : getWord(nodeI->word);
      VocabString nodeNameJ =
	(nodeJ->word == Vocab_None) ? "NULL" : getWord(nodeJ->word);
      dout() << "Lattice::approxMatchInTrans: "
	     << "      i.e., (" << nodeNameI << ", " << nodeNameJ << ")\n";
    }

    unsigned numInTransI = nodeI->inTransitions.numEntries();
    unsigned numInTransJ = nodeJ->inTransitions.numEntries();
    unsigned minIJ = ( numInTransI > numInTransJ ) ? 
      numInTransJ : numInTransI;

    if (overlap > minIJ) return false;

    unsigned nonOverlap = minIJ-overlap; 

    NodeIndex toNodeIndex, fromNodeIndex;

    if (debug(DebugPrintInnerLoop)) {
      dout() << "Lattice::approxMatchInTrans: "
	     << "number of transitions (" 
	     << numInTransI << ", " << numInTransJ << ")\n";
    }

    // **********************************************************************
    // preventing self loop generation
    // **********************************************************************

    if (nodeI->inTransitions.find(nodeIndexJ)) {
      if (debug(DebugPrintInnerLoop)) {
	dout() << "Lattice::approxMatchInTrans: "
	       << " preventing potential selfloop (" << nodeIndexJ
	       << ", " << nodeIndexI << ")\n";
      }
      return false;
    }

    if (nodeI->outTransitions.find(nodeIndexJ)) {
      if (debug(DebugPrintInnerLoop)) {
	dout() << "Lattice::approxMatchInTrans: "
	       << " preventing potential selfloop (" << nodeIndexI
	       << ", " << nodeIndexJ << ")\n";
      }
      return false;
    }


    // **********************************************************************
    // compare the sink nodes of the incoming edges of the two given
    // nodes.
    // **********************************************************************

    // loop over J
    TRANSITER_T<NodeIndex,LatticeTransition> 
      inTransIterJ(nodeJ->inTransitions);
    while (inTransIterJ.next(fromNodeIndex)) {
      
      if (debug(DebugPrintInnerLoop)) {
	dout() << "Lattice::approxMatchInTrans: "
	       << "loop (" << fromNodeIndex << ")\n";
      }

      LatticeTransition * transI = nodeI->inTransitions.find(fromNodeIndex);
      if (!transI) {
	// one mismatch occurs;
	if (--nonOverlap < 0) { 
	  return false; }
      } else {
	if (--overlap == 0) { 
	  // already exceed minimum overlap required.
	  break;
	}
      }
    }

    // **********************************************************************
    // I and J are qualified for merging.
    // **********************************************************************

    // merging incoming node sets.
    inTransIterJ.init(); 
    while (LatticeTransition *trans = inTransIterJ.next(fromNodeIndex)) {
      insertTrans(fromNodeIndex, nodeIndexI, *trans);
    }

    // merging outgoing nodes
    TRANSITER_T<NodeIndex,LatticeTransition> 
      outTransIterJ(nodeJ->outTransitions);
    while (LatticeTransition *trans = outTransIterJ.next(toNodeIndex)) {
      insertTrans(nodeIndexI, toNodeIndex, *trans);
    }
    
    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::approxMatchInTrans: "
	     << "(" << nodeIndexJ << ") has been merged with "
	     << "(" << nodeIndexI << ") and has been removed\n";
    }

    // delete this redudant node.
    removeNode(nodeIndexJ); 
    
    return true;
}

/* ********************************************************
   this procedure is to pack two nodes if their OutNode sets 
   overlap beyong certain threshold
   
   input: 
   1) nodeIndex: the node to be processed;
   2) nodeQueue: the current queue.
   3) base: overlap base
   4) ratio: overlap ratio

   function:
       going through all the in-nodes of nodeIndex, and tries to merge
       as many as possible.
   ******************************************************** */
Boolean 
Lattice::approxRedNodeF(NodeIndex nodeIndex, NodeQueue &nodeQueue, 
			int base, double ratio)
{

    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::approxRedNodeF: "
	     << "processing (" << nodeIndex << ") ***********\n"; 
    }

    NodeIndex fromNodeIndex, toNodeIndex; 
    LatticeNode * node = findNode(nodeIndex); 
    if (!node) { // skip through this node, being merged.
      return true; 
    }

    if (debug(DebugPrintOutLoop)) {
      VocabString nodeName =
	(node->word == Vocab_None) ? "NULL" : getWord(node->word);
      dout() << "Lattice::approxRedNodeF: "
	     << "      i.e., (" << nodeName << ")\n";
    }

    // going in a forward direction
    TRANSITER_T<NodeIndex,LatticeTransition> 
      outTransIter(node->outTransitions);
    unsigned numTransitions = node->outTransitions.numEntries();
    NodeIndex list[numTransitions+1]; 
    unsigned position = 0; 
    
    // collect all the out-nodes of nodeIndex
    while (outTransIter.next(toNodeIndex)) {
      list[position++] = toNodeIndex; 
    }

    // do a pair-wise comparison for all the out-nodes.
    unsigned i, j; 
    for (i = 0; i< position; i++) {
        j = i+1; 

	if (j >= position) { 
	  break; }

	LatticeNode * nodeI = findNode(list[i]);

	// ***********************************
	// compare nodeI with nodeJ:
	//    Notice that numInTransI changes because
	//    merger occurs in the loop. this is different
	//    from exact match case.   
	// ***********************************
	unsigned merged = 0; 
	while (j < position) {
	  if (debug(DebugPrintInnerLoop)) {
	    dout() << "Lattice::approxRedNodeF: "
		   << "comparing (" << list[i] << ", "
		   << list[j] << ")\n"; 
	  }

          LatticeNode * nodeI = findNode(list[i]);
	  unsigned numInTransI = nodeI->inTransitions.numEntries();

	  LatticeNode * nodeJ = findNode(list[j]);
	  unsigned numInTransJ = nodeJ->inTransitions.numEntries();
	  
	  if (nodeI->word == nodeJ->word) {
	    unsigned overlap; 
	    if ((!base && (numInTransI < numInTransJ)) ||
		(base && (numInTransI > numInTransJ))) {
	      overlap = (unsigned ) floor(numInTransI*ratio + 0.5);
	    } else {
	      overlap = (unsigned ) floor(numInTransJ*ratio + 0.5);
	    }

	    overlap = (overlap > 0) ? overlap : 1;
	    if (approxMatchInTrans(list[i], list[j], overlap)) {
	      merged = 1; 
	      list[j] = list[--position];
	      continue;
	    } 
	  } 
	  j++; 
	}

	// clear marks on the inNodes, if nodeI matches some other nodes.
	if (merged) {
	  nodeI = findNode(list[i]);
	  TRANSITER_T<NodeIndex,LatticeTransition> 
	    inTransIter(nodeI->inTransitions);
	  while (inTransIter.next(fromNodeIndex)) {
	    LatticeNode * fromNode = findNode(fromNodeIndex); 
	    fromNode->unmarkNode(markedFlag); 
	  }
	}
    }

    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::approxRedNodeF: "
	     << "adding nodes ("; 
    }

    // **************************************************
    // preparing next level nodes for merging processing.
    // **************************************************
    node = findNode(nodeIndex); 
    TRANSITER_T<NodeIndex,LatticeTransition> 
      transIter(node->outTransitions);
    while (transIter.next(toNodeIndex)) {
      if (debug(DebugPrintInnerLoop)) {
	dout() << "Lattice::approxRedNodeF: "
	       << " " << toNodeIndex; 
      }
	  
      nodeQueue.addToNodeQueueNoSamePrev(toNodeIndex); 
      // nodeQueue.addToNodeQueue(toNodeIndex); 
    }

    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::approxRedNodeF: "
	     << ") to the queue\n"; 
    }

    // mark that the current node has been processed.
    node->markNode(markedFlag);
    
    return true;
}


/* ********************************************************
   this procedure tries to compare two nodes to see whether their
   outgoing node sets overlap more than x.
   ******************************************************** */

Boolean 
Lattice::approxMatchOutTrans(NodeIndex nodeIndexI, NodeIndex nodeIndexJ,
			     int overlap)
{
    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::approxMatchOutTrans: "
	     << "try to match (" << nodeIndexI
	     << ", " << nodeIndexJ << ") with overlap "
	     << overlap << "\n"; 
    }

    LatticeNode * nodeI = findNode(nodeIndexI); 
    LatticeNode * nodeJ = findNode(nodeIndexJ); 

    if (debug(DebugPrintOutLoop)) {
      VocabString nodeNameI =
	(nodeI->word == Vocab_None) ? "NULL" : getWord(nodeI->word);
      VocabString nodeNameJ =
	(nodeJ->word == Vocab_None) ? "NULL" : getWord(nodeJ->word);
      dout() << "Lattice::approxMatchOutTrans: "
	     << "      i.e., (" << nodeNameI << ", " << nodeNameJ << ")\n";
    }

    unsigned numOutTransI = nodeI->outTransitions.numEntries();
    unsigned numOutTransJ = nodeJ->outTransitions.numEntries();
    unsigned minIJ = ( numOutTransI > numOutTransJ ) ? 
      numOutTransJ : numOutTransI;

    if (overlap > minIJ) return false;

    unsigned nonOverlap = minIJ-overlap; 

    NodeIndex toNodeIndex, fromNodeIndex;

    if (debug(DebugPrintInnerLoop)) {
      dout() << "Lattice::approxMatchOutTrans: "
	     << "number of transitions (" 
	     << numOutTransI << ", " << numOutTransJ << ")\n";
    }

    // **********************************************************************
    // preventing self loop generation
    // **********************************************************************

    if (nodeI->inTransitions.find(nodeIndexJ)) {
      if (debug(DebugPrintInnerLoop)) {
        dout() << "Lattice::approxMatchInTrans: "
               << " preventing potential selfloop (" << nodeIndexJ
               << ", " << nodeIndexI << ")\n";
      }
      return false;
    }

    if (nodeI->outTransitions.find(nodeIndexJ)) {
      if (debug(DebugPrintInnerLoop)) {
        dout() << "Lattice::approxMatchInTrans: "
               << " preventing potential selfloop (" << nodeIndexI
               << ", " << nodeIndexJ << ")\n";
      }
      return false;
    }

    // **********************************************************************
    // compare the sink nodes of the outgoing edges of the two given
    // nodes.
    // **********************************************************************

    // loop over J
    TRANSITER_T<NodeIndex,LatticeTransition> 
      outTransIterJ(nodeJ->outTransitions);
    while (outTransIterJ.next(toNodeIndex)) {
      
      if (debug(DebugPrintInnerLoop)) {
	dout() << "Lattice::approxMatchOutTrans: "
	       << "loop (" << toNodeIndex << ")\n";
      }

      LatticeTransition * transI = nodeI->outTransitions.find(toNodeIndex);
      if (!transI) {
	// one mismatch occurs;
	if (--nonOverlap < 0) { 
	  return false; }
      } else {
	if (--overlap == 0) { 
	  // already exceed minimum overlap required.
	  break;
	}
      }
    }

    // **********************************************************************
    // I and J are qualified for merging.
    // **********************************************************************

    // merging outgoing node sets.
    outTransIterJ.init(); 
    while (LatticeTransition *trans = outTransIterJ.next(toNodeIndex)) {
      insertTrans(nodeIndexI, toNodeIndex, *trans);
    }

    // merging incoming nodes
    TRANSITER_T<NodeIndex,LatticeTransition> 
      inTransIterJ(nodeJ->inTransitions);
    while (LatticeTransition *trans = inTransIterJ.next(fromNodeIndex)) {
      insertTrans(fromNodeIndex, nodeIndexI, *trans);
    }
    
    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::approxMatchOutTrans: "
	     << "(" << nodeIndexJ << ") has been merged with "
	     << "(" << nodeIndexI << ") and has been removed\n";
    }

    // delete this redudant node.
    removeNode(nodeIndexJ); 
    
    return true;
}

/* ********************************************************
   this procedure is to pack two nodes if their OutNode sets 
   overlap beyong certain threshold
   
   input: 
   1) nodeIndex: the node to be processed;
   2) nodeQueue: the current queue.
   3) base: overlap base
   4) ratio: overlap ratio

   function:
       going through all the in-nodes of nodeIndex, and tries to merge
       as many as possible.
   ******************************************************** */
Boolean 
Lattice::approxRedNodeB(NodeIndex nodeIndex, NodeQueue &nodeQueue, 
			int base, double ratio)
{

    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::approxRedNodeB: "
	     << "processing (" << nodeIndex << ") ***********\n"; 
    }

    NodeIndex fromNodeIndex, toNodeIndex; 
    LatticeNode * node = findNode(nodeIndex); 
    if (!node) { // skip through this node, being merged.
      return true; 
    }

    if (debug(DebugPrintOutLoop)) {
      VocabString nodeName =
	(node->word == Vocab_None) ? "NULL" : getWord(node->word);
      dout() << "Lattice::approxRedNodeB: "
	     << "      i.e., (" << nodeName << ")\n";
    }

    // going in a backward direction
    TRANSITER_T<NodeIndex,LatticeTransition> 
      inTransIter(node->inTransitions);
    unsigned numTransitions = node->inTransitions.numEntries();
    NodeIndex list[numTransitions+1]; 
    unsigned position = 0; 
    
    // collect all the in-nodes of nodeIndex
    while (inTransIter.next(fromNodeIndex)) {
      list[position++] = fromNodeIndex; 
    }

    // do a pair-wise comparison for all the in-nodes.
    unsigned i, j; 
    for (i = 0; i< position; i++) {
        j = i+1; 

	if (j >= position) { 
	  break; }

	LatticeNode * nodeI = findNode(list[i]);
	// ***********************************
	// compare nodeI with nodeJ:
	//    Notice that numInTransI changes because
	//    merger occurs in the loop. this is different
	//    from exact match case.   
	// ***********************************
	unsigned merged = 0; 
	while (j < position) {
	  if (debug(DebugPrintInnerLoop)) {
	    dout() << "Lattice::approxRedNodeB: "
		   << "comparing (" << list[i] << ", "
		   << list[j] << ")\n"; 
	  }

          LatticeNode * nodeI = findNode(list[i]);
	  unsigned numOutTransI = nodeI->outTransitions.numEntries();

	  LatticeNode * nodeJ = findNode(list[j]);
	  unsigned numOutTransJ = nodeJ->outTransitions.numEntries();
	  if (nodeI->word == nodeJ->word) {
	    unsigned overlap; 
	    if ((!base && (numOutTransI < numOutTransJ)) ||
		(base && (numOutTransI > numOutTransJ))) {
	      overlap = (unsigned ) floor(numOutTransI*ratio + 0.5);
	    } else {
	      overlap = (unsigned ) floor(numOutTransJ*ratio + 0.5);
	    }

	    overlap = (overlap > 0) ? overlap : 1;
	    if (approxMatchOutTrans(list[i], list[j], overlap)) {
	      merged = 1; 
	      list[j] = list[--position];
	      continue;
	    } 
	  } 
	  j++; 
	}

	// clear marks on the outNodes, if nodeI matches some other nodes.
	if (merged) {
	  nodeI = findNode(list[i]);
	  TRANSITER_T<NodeIndex,LatticeTransition> 
	    outTransIter(nodeI->outTransitions);
	  while (outTransIter.next(toNodeIndex)) {
	    LatticeNode * toNode = findNode(toNodeIndex); 
	    toNode->unmarkNode(markedFlag); 
	  }
	}
    }

    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::approxRedNodeB: "
	     << "adding nodes ("; 
    }

    // **************************************************
    // preparing next level nodes for merging processing.
    // **************************************************
    node = findNode(nodeIndex); 

    if (!node) { 
      if (debug(DebugPrintFatalMessages)) {
	dout() << "warning: (Lattice::approxRedNodeB) "
	       <<  " this node " << nodeIndex << " get deleted!\n";
      }
      return false;
    }
	
    TRANSITER_T<NodeIndex,LatticeTransition> 
      transIter(node->inTransitions);
    while (transIter.next(fromNodeIndex)) {
      if (debug(DebugPrintInnerLoop)) {
	dout() << "Lattice::approxRedNodeB: "
	       << " " << fromNodeIndex; 
      }
	  
      nodeQueue.addToNodeQueueNoSamePrev(fromNodeIndex); 
      // nodeQueue.addToNodeQueue(fromNodeIndex); 
    }

    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::approxRedNodeB: "
	     << ") to the queue\n"; 
    }

    // mark that the current node has been processed.
    node->markNode(markedFlag);
    
    return true;
}



/* ******************************************************** 

   An approximating algorithm for reducing bigram lattices.
   combine nodes when their incoming or outgoing node sets overlap
   a significant amount (decided by base and ratio).

   ******************************************************** */
Boolean 
Lattice::approxRedBigramLattice(unsigned iters, int base, double ratio)
{
    // keep track of number of node to know when to stop iteration
    unsigned numNodes = getNumNodes();

    if (debug(DebugPrintFunctionality)) {
      dout() << "Lattice::approxRedBigramLattice: "
	     << "starting reducing...."
	     << " (" << numNodes  << " nodes)\n";
    }
    // if it is a fast mode (default), it returns.
    if (iters == 0) {
      clearMarkOnAllNodes(markedFlag);

      NodeQueue nodeQueue; 
      nodeQueue.addToNodeQueue(final); 

      // use width first approach to go through the whole lattice.
      // mark the first level nodes and put them in the queue.
      // going through the queue to process all the nodes in the lattice
      // this is in a backward direction.
      while (nodeQueue.is_empty() == false) {
	NodeIndex nodeIndex = nodeQueue.popNodeQueue();

	if (nodeIndex == initial) {
	  continue;
	}
	LatticeNode * node = findNode(nodeIndex);
	if (!node) { 
	  continue; 
	}
	if (node->getFlag(markedFlag)) {
	  continue;
	}
	approxRedNodeB(nodeIndex, nodeQueue, base, ratio); 
      }

      numNodes = getNumNodes();

      if (debug(DebugPrintFunctionality)) {
	dout() << "Lattice::approxRedBigramLattice: "
	       << "done with only one B Reduction"
	       << " (" << numNodes  << " nodes)\n";
      }
      return true;
    }

    while (iters-- > 0) { 
      clearMarkOnAllNodes(markedFlag);

      // the queue must be empty. Going Backward
      NodeQueue nodeQueue; 
      nodeQueue.addToNodeQueue(final); 

      // use width first approach to go through the whole lattice.
      // mark the first level nodes and put them in the queue.
      // going through the queue to process all the nodes in the lattice
      // this is in a backward direction.
      while (nodeQueue.is_empty() == false) {
	NodeIndex nodeIndex = nodeQueue.popNodeQueue();
	
	if (nodeIndex == initial) {
	  continue;
	}
	LatticeNode * node = findNode(nodeIndex);
	if (!node) { 
	  continue; 
	}
	if (node->getFlag(markedFlag)) {
	  continue;
	}

	approxRedNodeB(nodeIndex, nodeQueue, base, ratio); 
      }
      // finish the Backward reduction
      if (debug(DebugPrintFunctionality)) {
	dout() << "Lattice::approxRedBigramLattice: "
	       << "done with B Reduction\n"; 
      }
      clearMarkOnAllNodes(markedFlag);

      // the queue must be empty. Going Forward
      nodeQueue.addToNodeQueue(initial); 

      // use width first approach to go through the whole lattice.
      // mark the first level nodes and put them in the queue.
      // going through the queue to process all the nodes in the lattice
      // this is in a forward direction.
      while (nodeQueue.is_empty() == false) {
	NodeIndex nodeIndex = nodeQueue.popNodeQueue();

	if (nodeIndex == final) {
	  continue;
	}
	LatticeNode * node = findNode(nodeIndex);
	if (!node) { 
	  continue; 
	}
	if (node->getFlag(markedFlag)) {
	  continue;
	}
	
	approxRedNodeF(nodeIndex, nodeQueue, base, ratio); 
      }

      unsigned newNumNodes = getNumNodes();

      if (debug(DebugPrintFunctionality)) {
	dout() << "Lattice::approxRedBigramLattice: "
	       << "done with F Reduction"
	       << " (" << newNumNodes  << " nodes)\n";
      }
      // finish one F-B iteration

      // check that lattices got smaller -- if not, stop
      if (newNumNodes == numNodes) {
	break;
      }
      numNodes = newNumNodes;
    }
    if (debug(DebugPrintFunctionality)) {
      dout() << "Lattice::approxRedBigramLattice: "
	     << "done with multiple iteration(s)\n"; 
    }
    return true; 
}

/*
 * Compute outgoing transition prob on demand.  This saves LM computation
 * for transitions that are cached.
 */
static Boolean
computeOutWeight(PackInput &packInput)
{
  if (packInput.lm != 0) {
    VocabIndex context[3];
    context[0] = packInput.wordName;
    context[1] = packInput.fromWordName;
    context[2] = Vocab_None;

    packInput.outWeight = packInput.lm->wordProb(packInput.toWordName, context);
    packInput.lm = 0;
  }

  return true;
}

/* this function tries to pack together nodes in lattice
 * 1) for non-self loop case: only when trigram prob exists,
 *    the from nodes with the same wordName will be packed;
 * 2) for self loop case: 
 *    the from nodes with the same wordName will be packed, 
 *    regardless whether the trigram prob exists.
 *    But, the bigram and trigram will have separate nodes,
 *    which is reflected in two different out transitions from
 *    the mid node to the two different toNodes (bigram and trigram)
 */
Boolean 
PackedNodeList::packNodes(Lattice &lat, PackInput &packInput)
{
  PackedNode *packedNode = packedNodesByFromNode.find(packInput.fromWordName);

  if (!packedNode && lastPackedNode != 0 &&
      (packInput.toNodeIndex == lastPackedNode->toNode &&
       computeOutWeight(packInput) &&
       abs(LogPtoIntlog(packInput.outWeight) -
	   LogPtoIntlog(lastPackedNode->outWeight)) <= PACK_TOLERANCE))
  {
    packedNode = lastPackedNode;
    NodeIndex midNode = packedNode->midNodeIndex;

    // the fromNode could be different this time around, so we need to
    // re-cache the mid-node 
    packedNode = packedNodesByFromNode.insert(packInput.fromWordName); 

    packedNode->midNodeIndex = midNode; 
    packedNode->toNode = packInput.toNodeIndex; 
    packedNode->outWeight = packInput.outWeight; 

    if (packInput.toNodeId == 2) { 
      packedNode->toNodeId = 2; 
    } else if (packInput.toNodeId == 3) { 
      packedNode->toNodeId = 3; 
    } else {
      packedNode->toNodeId = 0; 
    }

    lastPackedNode = packedNode;
  }

  if (packedNode) {
    // only one transition is needed;
    LatticeTransition t(packInput.inWeight, packInput.inFlag);
    lat.insertTrans(packInput.fromNodeIndex, packedNode->midNodeIndex, t);

    if (!packInput.toNodeId) { 
      // this is for non-self-loop node, no additional outgoing trans
      // need to be added.

      LatticeNode *midNode = lat.findNode(packedNode->midNodeIndex);
      LatticeTransition * trans = 
        midNode->outTransitions.find(packInput.toNodeIndex);

      // if it is another toNode, we need to create a link to it.
      if (!trans) {
        // it indicates that there is another ngram node needed.
        computeOutWeight(packInput);
	LatticeTransition t(packInput.outWeight, packInput.outFlag);
        lat.insertTrans(packedNode->midNodeIndex, packInput.toNodeIndex, t);

	if (debug(DebugPrintInnerLoop)) {
	  dout() << "PackedNodeList::packNodes: \n"
		 << "insert (" << packInput.fromNodeIndex
		 << ", " << packedNode->midNodeIndex << ", " 
		 << packInput.toNodeIndex << ")\n";
        }
      }

      return true;
    } else {
	if (debug(DebugPrintInnerLoop)) {
	  dout() << "PackedNodeList::packNodes: \n"
		 << "reusing (" << packInput.fromNodeIndex
		 << ", " << packedNode->midNodeIndex << ", " 
		 << packInput.toNodeIndex << ")\n";
        }
    }

    // the following part is for selfLoop case
    // the toNode is for p(a | a, x) doesn't exist.
    if (packInput.toNodeId == 2) {
      if (!packedNode->toNode) {
        computeOutWeight(packInput);
	LatticeTransition t(packInput.outWeight, packInput.outFlag); 
	lat.insertTrans(packedNode->midNodeIndex, packInput.toNodeIndex, t);
	packedNode->toNode = packInput.toNodeIndex;
      }
      return true;
    }

    // the toNode is for p(a | a, x) exists.
    if (packInput.toNodeId == 3) {
      if (!packedNode->toNode) {
        computeOutWeight(packInput);
	LatticeTransition t(packInput.outWeight, packInput.outFlag);
	lat.insertTrans(packedNode->midNodeIndex, packInput.toNodeIndex, t);
	packedNode->toNode = packInput.toNodeIndex;
      }
      return true;
    }
  } else {
    // this is the first time to create triple.
    NodeIndex newNodeIndex = lat.dupNode(packInput.wordName, markedFlag);
    
    LatticeTransition t1(packInput.inWeight, packInput.inFlag); 
    lat.insertTrans(packInput.fromNodeIndex, newNodeIndex, t1);

    computeOutWeight(packInput);
    LatticeTransition t2(packInput.outWeight, packInput.outFlag);
    lat.insertTrans(newNodeIndex, packInput.toNodeIndex, t2);

    if (debug(DebugPrintInnerLoop)) {
      dout() << "PackedNodeList::packNodes: \n"
	     << "insert (" << packInput.fromNodeIndex
	     << ", " << newNodeIndex << ", " 
	     << packInput.toNodeIndex << ")\n";
    }

    packedNode = packedNodesByFromNode.insert(packInput.fromWordName); 

    packedNode->midNodeIndex = newNodeIndex; 
    packedNode->toNode = packInput.toNodeIndex; 
    packedNode->outWeight = packInput.outWeight; 

    if (packInput.toNodeId == 2) { 
      packedNode->toNodeId = 2; 
    } else if (packInput.toNodeId == 3) { 
      packedNode->toNodeId = 3; 
    } else {
      packedNode->toNodeId = 0; 
    }

    lastPackedNode = packedNode;
  }

  return true;
}

/*
 * NOTE: MAX_COST is small enough that we can add any of the *_COST constants
 * without overflow.
 */
const unsigned MAX_COST = (unsigned)(-10);      // an impossible path

/*
 * error type used in tracing back word/lattice alignmennts
 */
typedef enum {
      ERR_NONE, ERR_SUB, ERR_INS, ERR_DEL
} ErrorType;


template <class T>
void reverseArray(T *array, unsigned length)
{
    int i, j;	/* j can get negative ! */

    for (i = 0, j = length - 1; i < j; i++, j--) {
	T x = array[i];
	array[i] = array[j];
	array[j] = x;
    }
}

/*
 * sortNodes --
 *	Sort node indices topologically
 *
 * Result:
 *	The number of reachable nodes.
 *
 * Side effects:
 *	sortedNodes is filled with the sorted node indices.
 */
void
Lattice::sortNodesRecursive(NodeIndex nodeIndex, unsigned &numVisited,
			NodeIndex *sortedNodes, Boolean *visitedNodes)
{
    if (visitedNodes[nodeIndex]) {
	return;
    }
    visitedNodes[nodeIndex] = true;

    LatticeNode *node = findNode(nodeIndex); 
    if (!node) {
      if (debug(DebugPrintOutLoop)) {
	dout() << "Lattice::sortNodesRecursive: "
	       << "can't find an "
	       << "existing node (" << nodeIndex << ")\n";
      }
      return; 
    }

    TRANSITER_T<NodeIndex,LatticeTransition> 
      outTransIter(node->outTransitions);
    NodeIndex nextNodeIndex;
    while (outTransIter.next(nextNodeIndex)) {
        sortNodesRecursive(nextNodeIndex, numVisited, 
			   sortedNodes, visitedNodes);
    }

    sortedNodes[numVisited++] = nodeIndex; 
}

unsigned
Lattice::sortNodes(NodeIndex *sortedNodes, Boolean reversed)
{
    Boolean visitedNodes[maxIndex];
    for (NodeIndex j = 0; j < maxIndex; j ++) {
	visitedNodes[j] = false;
    }

    unsigned numVisited = 0;

    sortNodesRecursive(initial, numVisited, sortedNodes, visitedNodes);
    
    if (!reversed) {
	// reverse the node order from the way we generated it
	reverseArray(sortedNodes, numVisited);
    }

    return numVisited;
}

/*
 * latticeWER --
 *	compute minimal word error of path through lattice
 * using two dimensional chart:
 * node axis which indicates the current state in the lattice
 * word axis which indicates the current word in the ref anything before 
 *     which have been considered.
 */
unsigned
Lattice::latticeWER(const VocabIndex *words,
		   unsigned &sub, unsigned &ins, unsigned &del,
		   SubVocab &ignoreWords)
{
    if (debug(DebugPrintFunctionality)) {
      dout() << "Lattice::latticeWER: "
	     << "processing (" << words << ")\n";
    }

    unsigned numWords = Vocab::length(words);
    unsigned numNodes = getNumNodes(); 

    /*
     * The states indexing the DP chart correspond to lattice nodes.
     */
    const unsigned NO_PRED = (unsigned)(-1);	// default for pred link

    NodeIndex sortedNodes[numNodes];
    unsigned numReachable = sortNodes(sortedNodes);

    if (numReachable != numNodes) {
	dout() << "Lattice::latticeWER warning: called with unreachable nodes\n";
    }

    /*
     * Allocate the DP chart.
     * chartEntries are indexed by [word_position][lattice_node],
     * where word_position = 0 is the  left string margin,
     * word_position = numWords + 1 is the right string margin.
     */
    typedef struct {
	unsigned cost;		// minimal path cost to this state
	unsigned ins, del, sub; // error counts by type
	NodeIndex predNode;	// predecessor state used in getting there
	ErrorType errType;	// error type
    } ChartEntry;

    ChartEntry *chart[numWords + 2];

    unsigned i;
    for (i = 0; i <= numWords + 1; i ++) {
	chart[i] = new ChartEntry[maxIndex];
	assert(chart[i] != 0);
	for (unsigned j = 0; j < maxIndex; j ++) {
	    chart[i][j].cost = MAX_COST;
	    chart[i][j].sub = chart[i][j].ins = chart[i][j].del = 0;
	    chart[i][j].predNode = NO_PRED;
	    chart[i][j].errType = ERR_NONE;
	}
    }

    /*
     * Prime the chart by anchoring the alignment at the left edge
     */
    chart[0][initial].cost = 0;
    chart[0][initial].ins = chart[0][initial].del = chart[0][initial].sub = 0;
    chart[0][initial].predNode = initial;
    chart[0][initial].errType = ERR_NONE;

    /*
     * Check that lattice starts with <s>
     */
    LatticeNode *initialNode = findNode(initial);
    assert(initialNode != 0);
    if (initialNode->word != vocab.ssIndex && initialNode->word != Vocab_None) {
	dout() << "warning: initial node has non-null word; will be ignored\n";
    }

    // initialize the chart along the node axis
    /*
     * Insertions before the first word
     * NOTE: since we process nodes in topological order this
     * will allow chains of multiple insertions.
     */
    for (unsigned j = 0; j < numReachable; j ++) {
	NodeIndex curr = sortedNodes[j];
	LatticeNode *node = findNode(curr); 
	assert(node != 0);

	unsigned cost = chart[0][curr].cost;
	if (cost >= MAX_COST) continue;

	TRANSITER_T<NodeIndex,LatticeTransition> 
	  outTransIter(node->outTransitions);
	NodeIndex next;
	while (outTransIter.next(next)) {
	    LatticeNode *nextNode = findNode(next); 
	    assert(nextNode != 0);

	    unsigned haveIns = (nextNode->word != Vocab_None &&
				    !ignoreWords.getWord(nextNode->word));
	    unsigned insCost = cost + haveIns * INS_COST;

	    if (insCost < chart[0][next].cost) {
		chart[0][next].cost = insCost;
		chart[0][next].ins = chart[0][curr].ins + haveIns;
		chart[0][next].del = chart[0][curr].del;
		chart[0][next].sub = chart[0][curr].sub;
		chart[0][next].predNode = curr;
		chart[0][next].errType = ERR_INS;
	    }
	}
    }

    /*
     * For all word positions, compute minimal cost alignment for each
     * state.
     */
    for (i = 1; i <= numWords + 1; i ++) {
	/*
	 * Compute partial alignment cost for all lattice nodes
	 */
	unsigned j;
	for (j = 0; j < numReachable; j ++) {
 	    NodeIndex curr = sortedNodes[j];
	    LatticeNode *node = findNode(curr);
	    assert(node != 0);

	    unsigned cost = chart[i - 1][curr].cost;

	    if (cost >= MAX_COST) continue;

	    /*
	     * Deletion error: current word not matched by lattice
	     */
	    {
		unsigned haveDel = !ignoreWords.getWord(words[i - 1]);
		unsigned delCost = cost + haveDel * DEL_COST;

		if (delCost < chart[i][curr].cost) {
		    chart[i][curr].cost = delCost;
		    chart[i][curr].del = chart[i - 1][curr].del + haveDel;
		    chart[i][curr].ins = chart[i - 1][curr].ins;
		    chart[i][curr].sub = chart[i - 1][curr].sub;
		    chart[i][curr].predNode = curr;
		    chart[i][curr].errType = ERR_DEL;
		}
	    }

	    /*
	     * Substitution errors
	     */
	    
	    TRANSITER_T<NodeIndex,LatticeTransition> 
	      outTransIter(node->outTransitions);
	    NodeIndex next;
	    while (outTransIter.next(next)) {
		LatticeNode *nextNode = findNode(next); 
		assert(nextNode != 0);

		VocabIndex word = nextNode->word;

		/*
		 * </s> (on final node) matches Vocab_None in word string
		 */
		if (word == vocab.seIndex) {
		    word = Vocab_None;
		}

		unsigned haveSub = (word != words[i - 1]);
		unsigned subCost = cost + haveSub * SUB_COST;

		if (subCost < chart[i][next].cost) {
		    chart[i][next].cost = subCost;
		    chart[i][next].sub = chart[i - 1][curr].sub + haveSub;
		    chart[i][next].ins = chart[i - 1][curr].ins;
		    chart[i][next].del = chart[i - 1][curr].del;
		    chart[i][next].predNode = curr;
		    chart[i][next].errType = haveSub ? ERR_SUB : ERR_NONE;
		}
	    }
	}

	for (j = 0; j < numReachable; j ++) {
 	    NodeIndex curr = sortedNodes[j]; 
	    LatticeNode *node = findNode(curr);
	    assert(node != 0);

	    unsigned cost = chart[i][curr].cost;
	    if (cost >= MAX_COST) continue;

	    /*
	     * Insertion errors: lattice node not matched by word
	     * NOTE: since we process nodes in topological order this
	     * will allow chains of multiple insertions.
	     */

	    TRANSITER_T<NodeIndex,LatticeTransition> 
	      outTransIter(node->outTransitions);
	    NodeIndex next;
	    while (outTransIter.next(next)) {
		LatticeNode *nextNode = findNode(next); 
		assert(nextNode != 0);

		unsigned haveIns = (nextNode->word != Vocab_None &&
					!ignoreWords.getWord(nextNode->word));
	        unsigned insCost = cost + haveIns * INS_COST;

		if (insCost < chart[i][next].cost) {
		    chart[i][next].cost = insCost;
		    chart[i][next].ins = chart[i][curr].ins + haveIns;
		    chart[i][next].del = chart[i][curr].del;
		    chart[i][next].sub = chart[i][curr].sub;
		    chart[i][next].predNode = curr;
		    chart[i][next].errType = ERR_INS;
		}
	    }
	}
    }

    if (chart[numWords+1][final].predNode == NO_PRED) {
	dout() << "Lattice::latticeWER warning: called with unreachable final node\n";
        sub = ins = 0;
	del = numWords;
    } else {
	sub = chart[numWords+1][final].sub;
	ins = chart[numWords+1][final].ins;
	del = chart[numWords+1][final].del;
    }

    if (debug(DebugPrintInnerLoop)) {
      dout() << "Lattice::latticeWER: printing chart:\n";
    }
    for (i = 0; i <= numWords + 1; i ++) {
        unsigned j; 
        for (j = 0; j < numReachable; j++) {
	  if (debug(DebugPrintInnerLoop)) {
	    dout() << "chart[" << i << ", " << j << "] "
		   << chart[i][j].cost << " (" 
		   << chart[i][j].sub << ","
		   << chart[i][j].ins << ","
		   << chart[i][j].del << ")  ";
	  }
	}
	
	if (debug(DebugPrintInnerLoop)) {
	  dout() << "\n"; 
	}
	delete [] chart[i];
    }
    
    return sub + ins + del;
}

/*
 * Compute node forward and backward probabilities based on transition scores
 * 	Returns the max-min-posterior: the maximum over all lattice paths,
 *	of the minumum posterior of all nodes along the path.
 */
LogP2
Lattice::computeForwardBackward(LogP2 forwardProbs[], LogP2 backwardProbs[],
							double posteriorScale)
{
    /*
     * Algorithm: 
     * 0) sort nodes in topological order
     * 1) forward pass: compute forward probabilities
     * 2) backward pass: compute backward probabilities and posteriors
     */

    if (debug(DebugPrintFunctionality)) {
      dout() << "Lattice::computeForwardBackward: "
	     << "processing (posterior scale = " << posteriorScale << ")\n";
    }

    /*
     * topological sort
     */
    unsigned numNodes = getNumNodes(); 

    NodeIndex sortedNodes[numNodes];
    unsigned numReachable = sortNodes(sortedNodes);

    if (numReachable != numNodes) {
	dout() << "Lattice::computeForwardBackward: warning: called with unreachable nodes\n";
    }
    if (sortedNodes[0] != initial) {
	dout() << "Lattice::computeForwardBackward: initial node is not first\n";
        return LogP_Zero;
    }
    unsigned finalPosition = 0;
    for (finalPosition = 1; finalPosition < numReachable; finalPosition ++) {
	if (sortedNodes[finalPosition] == final) break;
    }
    if (finalPosition == numReachable) {
	dout() << "Lattice::computeForwardBackward: final node is not reachable\n";
        return LogP_Zero;
    }

    /*
     * compute forward probabilities
     */
    for (unsigned i = 0; i < maxIndex; i ++) {
	    forwardProbs[i] = LogP_Zero;
    }
    forwardProbs[initial] = LogP_One;

    for (int position = 1; position < numReachable; position ++) {
	LatticeNode *node = nodes.find(sortedNodes[position]);
        assert(node != 0);

	LogP2 prob = LogP_Zero;

        TRANSITER_T<NodeIndex,LatticeTransition>
					inTransIter(node->inTransitions);
	LatticeTransition *inTrans;
	NodeIndex fromNodeIndex; 
	while (inTrans = inTransIter.next(fromNodeIndex)) {
	    prob = AddLogP(prob,
			   forwardProbs[fromNodeIndex] +
				inTrans->weight/posteriorScale);
	}
	forwardProbs[sortedNodes[position]] = prob;
    }

    /*
     * compute backward probabilities
     */
    for (unsigned i = 0; i < maxIndex; i ++) {
	    backwardProbs[i] = LogP_Zero;
    }
    backwardProbs[final] = LogP_One;

    for (int position = finalPosition - 1; position >= 0; position --) {
	LatticeNode *node = nodes.find(sortedNodes[position]);
        assert(node != 0);

	LogP2 prob = LogP_Zero;

        TRANSITER_T<NodeIndex,LatticeTransition>
					outTransIter(node->outTransitions);
	LatticeTransition *outTrans;
	NodeIndex toNodeIndex; 
	while (outTrans = outTransIter.next(toNodeIndex)) {
	    prob = AddLogP(prob,
			   backwardProbs[toNodeIndex] +
				outTrans->weight/posteriorScale);
	}
	backwardProbs[sortedNodes[position]] = prob;
    }

    /*
     * set posteriors of unreachable nodes to zero
     */
    LHashIter<NodeIndex, LatticeNode> nodeIter(nodes);
    NodeIndex nodeIndex;

    while (LatticeNode *node = nodeIter.next(nodeIndex)) {
	node->posterior = LogP_Zero;
    }

    /*
     * array of partial min-max-posteriors
     */
    LogP2 maxMinPosteriors[maxIndex];
    maxMinPosteriors[initial] = LogP_One;

    /*
     * compute unnormalized log posteriors, as well as min-max-posteriors
     */
    for (int position = 0; position < numReachable; position ++) {
        NodeIndex nodeIndex = sortedNodes[position];
	LatticeNode *node = nodes.find(nodeIndex);
        assert(node != 0);

        node->posterior = forwardProbs[nodeIndex] + backwardProbs[nodeIndex];

	/*
	 * compute max-min-posteriors
	 */
	if (position > 0) {
	    LogP2 maxInPosterior = LogP_Zero;

	    TRANSITER_T<NodeIndex,LatticeTransition>
					    inTransIter(node->inTransitions);
	    NodeIndex fromNodeIndex; 
	    while (inTransIter.next(fromNodeIndex)) {
		 if (maxMinPosteriors[fromNodeIndex] > maxInPosterior) {
		    maxInPosterior = maxMinPosteriors[fromNodeIndex];
		 }
	    }

	    maxMinPosteriors[nodeIndex] =
			(node->posterior > maxInPosterior) ? maxInPosterior
							   : node->posterior;
	}
    }

    if (debug(DebugPrintFunctionality)) {
      dout() << "Lattice::computeForwardBackward: "
	     << "unnormalized posterior = " << forwardProbs[final] 
	     << " max-min path posterior = " << maxMinPosteriors[final] << endl;
    }

    return maxMinPosteriors[final];
}

/*
 * Compute node posterior probabilities based on transition scores
 * 	returns max-min path posterior
 */
LogP2
Lattice::computePosteriors(double posteriorScale)
{
    LogP2 forwardProbs[maxIndex];
    LogP2 backwardProbs[maxIndex];

    return computeForwardBackward(forwardProbs, backwardProbs, posteriorScale);
}

/*
 * Write node and transition posteriors in SRILM format:
 *
 *     Word lattices:
 *     version 2
 *     initial i
 *     final f
 *     node n w a p n1 p1 n2 p2 ...
 *     ...
 *
 *     (see wlat-format(5) man page)
 *
 */
Boolean
Lattice::writePosteriors(File &file, double posteriorScale)
{
    LogP2 forwardProbs[maxIndex];
    LogP2 backwardProbs[maxIndex];

    LogP2 maxMinPosterior =
	computeForwardBackward(forwardProbs, backwardProbs, posteriorScale);
    LogP2 totalPosterior = maxMinPosterior != LogP_Zero ? 
				nodes.find(initial)->posterior : LogP_Zero;

    fprintf(file, "version 2\n");
    fprintf(file, "initial %u\n", initial);
    fprintf(file, "final %u\n", final);

    
    for (unsigned i = 0; i < maxIndex; i ++) {
	LatticeNode *node = nodes.find(i);

        if (node) {
	    fprintf(file, "node %u %s -1 %lf", i, 
			node->word == Vocab_None ? "NULL" :
					vocab.getWord(node->word),
			(double)LogPtoProb(node->posterior - totalPosterior));

	    TRANSITER_T<NodeIndex,LatticeTransition>
					outTransIter(node->outTransitions);
	    LatticeTransition *outTrans;
	    NodeIndex toNodeIndex; 
	    while (outTrans = outTransIter.next(toNodeIndex)) {
		fprintf(file, " %u %lf", toNodeIndex,
			(double)LogPtoProb(forwardProbs[i] +
					    outTrans->weight/posteriorScale +
					    backwardProbs[toNodeIndex] -
					    totalPosterior));
	    }
	    fprintf(file, "\n");
        }
    }

    return true;
}

/*
 * Prune lattice nodes based on posterior probabilities
 *	all nodes (and attached transitions) with posteriors less than
 *	theshold of the total posterior are removed.
 */
Boolean
Lattice::prunePosteriors(Prob threshold, double posteriorScale)
{
    // keep track of number of node to know when to stop iteration
    unsigned numNodes = getNumNodes();
    
    if (debug(DebugPrintFunctionality)) {
	dout() << "Lattice::prunePosteriors: "
	       << "starting with " << numNodes << " nodes\n";
    }

    while (numNodes > 0) {
	LogP2 maxMinPosterior = computePosteriors(posteriorScale);

	if (maxMinPosterior == LogP_Zero) {
	    dout() << "Lattice::prunePosteriors: "
		   << "no paths left in lattice\n";
	    return false;
	}

	/*
	 * prune 'em
	 */
	LHashIter<NodeIndex, LatticeNode> nodeIter(nodes);
	NodeIndex nodeIndex;

	while (LatticeNode *node = nodeIter.next(nodeIndex)) {
	    if (LogPtoProb(node->posterior - maxMinPosterior) < threshold) {
		removeNode(nodeIndex);
	    }
	}

	unsigned newNumNodes = getNumNodes();

	if (newNumNodes == numNodes) {
	    break;
	}
	numNodes = newNumNodes;

	if (debug(DebugPrintFunctionality)) {
	    dout() << "Lattice::prunePosteriors: "
		   <<  "left with " << numNodes << " nodes\n";
	}
    }

    return true;
}

/* check connectivity of this lattice given two nodeIndices.
 * it can go either forward or backward directions.
 */
Boolean
Lattice::areConnected(NodeIndex fromNodeIndex, NodeIndex toNodeIndex, 
		      unsigned direction)
{
    if (debug(DebugPrintFunctionality)) {
      dout() << "Lattice::areConnected: "
	     << "processing (" << (fromNodeIndex, toNodeIndex) 
	     << ")\n";
    }

    if (fromNodeIndex == toNodeIndex) {
      if (debug(DebugPrintFunctionality)) {
	dout() << "Lattice::areConnected: "
	       << "(" << (fromNodeIndex, toNodeIndex) << ") are connected\n";
      }
      return true;
    }
     
    clearMarkOnAllNodes(markedFlag);

    int i = 0; 
    NodeQueue nodeQueue; 
    nodeQueue.addToNodeQueue(fromNodeIndex); 

    // use width first approach to go through the whole lattice.
    // mark the first level nodes and put them in the queue.
    // going through the queue to process all the nodes in the lattice
    while (nodeQueue.is_empty() == false) {
      NodeIndex nodeIndex = nodeQueue.popNodeQueue();

      // extend one more level in lattice and put the nodes 
      // there to the queue.
      LatticeNode * node = findNode(nodeIndex);
      if (!node) {
	if (debug(DebugPrintOutLoop)) {
	  dout() << "NonFatal Error in Lattice::areConnected: "
		 << "can't find an existing node (" << nodeIndex << ")\n";
	}
	continue;
      }

      // TRANSITER_T<NodeIndex,LatticeTransition> 
      // outTransIter(node->outTransitions);
      TRANSITER_T<NodeIndex,LatticeTransition> 
	outTransIter(!direction ? node->outTransitions : node->inTransitions);
      NodeIndex nextNodeIndex;
      while (outTransIter.next(nextNodeIndex)) {
	// processing nodes at the next level 

	if (nextNodeIndex == toNodeIndex) {

	  if (debug(DebugPrintFunctionality)) {
	    dout() << "Lattice::areConnected: "
		   << "(" << (fromNodeIndex, toNodeIndex) << ") are connected\n";
	  }
	  return true; 
	}

	LatticeNode *nextNode = findNode(nextNodeIndex);
	if (nextNode->getFlag(markedFlag)) {
	  continue;
	}

	nextNode->markNode(markedFlag); 
        nodeQueue.addToNodeQueue(nextNodeIndex); 
      }
    }

    if (debug(DebugPrintFunctionality)) {
      dout() << "Lattice::areConnected: "
	     << "(" << (fromNodeIndex, toNodeIndex) << ") are NOT connected\n";
    }
    return false; 
}

// higher level functions.

/* this method creates one pause for all the outgoing transitions of a 
   particular node that have pause marks.
   it works.
   */
/* this code is to replace weights on the links of a given lattice with
 * the LM weights.
 */
Boolean 
Lattice::replaceWeights(LM &lm)
{
    if (debug(DebugPrintFunctionality)) {
      dout() << "Lattice::replaceWeights: "
	     << "replacing weights with new LM\n";
    }

    LHashIter<NodeIndex, LatticeNode> nodeIter(nodes);
    NodeIndex nodeIndex;

    while (LatticeNode *node = nodeIter.next(nodeIndex)) {

      NodeIndex toNodeIndex;
      VocabIndex wordIndex;
      if (nodeIndex == initial) {
	wordIndex = vocab.ssIndex;
      } else {
	wordIndex = node->word; 
      }
      // need to check to see whether the word is in the vocab


      TRANSITER_T<NodeIndex,LatticeTransition> transIter(node->outTransitions);
      while (transIter.next(toNodeIndex)) {
	LatticeNode * toNode = nodes.find(toNodeIndex);

	VocabIndex toWordIndex;
        LogP weight;

	if (toNodeIndex == final) {
	  toWordIndex = vocab.seIndex; }
	else {
	  toWordIndex = toNode->word; }

	if (toWordIndex == Vocab_None || toWordIndex == lm.vocab.pauseIndex) {
	  /*
	   * NULL and pause nodes don't receive an language model weight
	   */
	  weight = LogP_One;
	} else {
	  VocabIndex context[2];
	  context[0] = wordIndex; 
	  context[1] = Vocab_None; 

	  weight = lm.wordProb(toWordIndex, context); 
	}

	setWeightTrans(nodeIndex, toNodeIndex, weight);
      }
    }

    return true; 
}

// *************************************************
// compact expansion to trigram
// *************************************************


// *************************************************
/*  Basic Algorithm: 
 *  Try to expand self loop to accomodate trigram
 *  the basic idea has two steps:
 *  1) ignore the loop edge and process other edge combinations
 *     just like in other cases, this is done in the main expandNodeToTrigram 
 *     program
 *  2) IN THIS PROGRAM: 
 *     a) duplicate the loop node (called postNode); 
 *     b) add an additional node (called preNode) between fromNode and the 
 *        loop node (postNode);
 *     c) create links between fromNode, preNode, postNode and toNode; and 
 *        create the loop edge on the loop node (postNode).
 */
void 
Lattice::initASelfLoopDB(SelfLoopDB &selfLoopDB, LM &lm, 
			NodeIndex nodeIndex, LatticeNode *node, 
			LatticeTransition *trans)
{
    selfLoopDB.preNodeIndex = selfLoopDB.postNodeIndex2 = 
      selfLoopDB.postNodeIndex3 = 0; 
    selfLoopDB.nodeIndex = nodeIndex; 

    selfLoopDB.selfTransFlags = trans->flags;

    selfLoopDB.wordName = node->word; 

    VocabIndex context[3];
    context[0] = selfLoopDB.wordName; 
    context[1] = selfLoopDB.wordName; 
    context[2] = Vocab_None; 

    selfLoopDB.loopProb = lm.wordProb(selfLoopDB.wordName, context); 
}

void 
Lattice::initBSelfLoopDB(SelfLoopDB &selfLoopDB, LM &lm, 
			NodeIndex fromNodeIndex, LatticeNode * fromNode, 
			LatticeTransition *fromTrans)
{
    // reinitialize the preNode
    selfLoopDB.preNodeIndex = 0; 

    // 
    selfLoopDB.fromNodeIndex = fromNodeIndex; 
    selfLoopDB.fromWordName = fromNode->word; 

    // 
    selfLoopDB.fromSelfTransFlags = fromTrans->flags; 

    // compute prob for the link between preNode and postNode
    VocabIndex context[3];
    context[0] = selfLoopDB.wordName; 
    context[1] = selfLoopDB.fromWordName; 
    context[2] = Vocab_None; 
    
    selfLoopDB.prePostProb =
			lm.wordProb(selfLoopDB.wordName, context); 

    // compute prob for fromPreProb; 
    context[0] = selfLoopDB.fromWordName; 
    context[1] = Vocab_None;

    selfLoopDB.fromPreProb = 
    			lm.wordProb(selfLoopDB.wordName, context); 
}

void 
Lattice::initCSelfLoopDB(SelfLoopDB &selfLoopDB, NodeIndex toNodeIndex, 
			LatticeTransition *toTrans)
{
    selfLoopDB.toNodeIndex = toNodeIndex;
    selfLoopDB.selfToTransFlags = toTrans->flags;
}

/* 
 * creating an expansion network for a self loop node.
 * the order in which the network is created is reverse:
 *  1) build the part of the network starting from postNode to toNode
 *  2) use PackedNodeList class function to build the part of 
 *     the network starting from fromNode to PostNode. 
 *
 */
Boolean
Lattice::expandSelfLoop(LM &lm, SelfLoopDB &selfLoopDB, 
			       PackedNodeList &packedSelfLoopNodeList)
{
    unsigned id = 0;
    NodeIndex postNodeIndex, toNodeIndex = selfLoopDB.toNodeIndex;
    LogP fromPreProb = selfLoopDB.fromPreProb; 
    LogP prePostProb = selfLoopDB.prePostProb; 

    VocabIndex wordName = selfLoopDB.wordName; 

    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::expandSelfLoop: "
	     << "nodeIndex (" << selfLoopDB.nodeIndex << ")\n";
    }

    // create the part of the network from postNode to toNode 
    //   if it doesn't exist.
    // first compute the probs of the links in that part.
    VocabIndex context[3];
    context[0] = wordName; 
    context[1] = wordName; 
    context[2] = Vocab_None; 

    LatticeNode *toNode = findNode(toNodeIndex); 
    VocabIndex toWordName = toNode->word; 

    LogP triProb = lm.wordProb(toWordName, context);

    unsigned usedContextLength;
    lm.contextID(context, usedContextLength);

    context[1] = Vocab_None;
    LogP biProb = lm.wordProb(toWordName, context);

    LogP postToProb; 

    if (usedContextLength > 1) {

      // get trigram prob for (post, to) edge: p(c|a, a)
      postToProb = triProb; 

      // create post node and loop if it doesn't exist;
      if (!selfLoopDB.postNodeIndex3) {
	selfLoopDB.postNodeIndex3 = 
	  postNodeIndex = dupNode(wordName, markedFlag);
	
	// create the loop, put trigram prob p(a|a,a) on the loop
	LatticeTransition t(selfLoopDB.loopProb, selfLoopDB.selfTransFlags);
	insertTrans(postNodeIndex, postNodeIndex, t); 
	// end of creating of loop
      }

      postNodeIndex = selfLoopDB.postNodeIndex3; 
      id = 3; 

    } else {
      
      // get an adjusted weight for the link between preNode to postNode
      LogP wordBOW = triProb - biProb;

      prePostProb = combWeights(prePostProb, wordBOW); 

      // get existing weight of (node, toNode) as the weight for (post, to).
      LatticeNode *node = findNode(selfLoopDB.nodeIndex); 
      if (!node) {
	if (debug(DebugPrintFatalMessages)) {
	  dout() << "Fatal Error in Lattice::expandSelfLoop: "
		 << "can't find node " << selfLoopDB.nodeIndex << "\n"; 
	}
	exit(-1);
      }

      // compute postToProb
      postToProb = biProb; 

      // create post node and loop if it doesn't exist;
      if (!selfLoopDB.postNodeIndex2) {
	selfLoopDB.postNodeIndex2 = 
	  postNodeIndex = dupNode(wordName, markedFlag);

	// create the loop, put trigram prob p(a|a,a) on the loop
	LatticeTransition t(selfLoopDB.loopProb, selfLoopDB.selfTransFlags);
	insertTrans(postNodeIndex, postNodeIndex, t); 
	// end of creating loop
      }
      postNodeIndex = selfLoopDB.postNodeIndex2; 
      id = 2; 
    }

    // create link from postNode to toNode if (postNode, toNode) doesn't exist;
    toNode = findNode(toNodeIndex); 
    LatticeTransition *postToTrans = toNode->inTransitions.find(postNodeIndex);
    if (!postToTrans) {
      // create link from postNode to toNode;
      LatticeTransition t(postToProb, selfLoopDB.selfToTransFlags); 
      insertTrans(postNodeIndex, toNodeIndex, t); 
    }
    // done with first part of the network. 

    // create the part of the network from fromNode to postNode.
    // create preNode and (from, pre) edge.
    NodeIndex preNodeIndex = selfLoopDB.preNodeIndex; 

    PackInput packSelfLoop;
    packSelfLoop.wordName = wordName; 
    packSelfLoop.fromWordName = selfLoopDB.fromWordName;
    packSelfLoop.toWordName = toNode->word; 
    packSelfLoop.fromNodeIndex = selfLoopDB.fromNodeIndex; 
    packSelfLoop.toNodeIndex = postNodeIndex; 
    packSelfLoop.inWeight = selfLoopDB.fromPreProb;
    packSelfLoop.inFlag = selfLoopDB.fromSelfTransFlags; 
    packSelfLoop.outWeight = prePostProb; 
    packSelfLoop.toNodeId = id; 
    packSelfLoop.lm = 0; 

    packedSelfLoopNodeList.packNodes(*this, packSelfLoop); 

    return true;
}

Boolean 
Lattice::expandNodeToTrigram(NodeIndex nodeIndex, LM &lm, unsigned maxNodes)
{
    SelfLoopDB selfLoopDB; 

    PackedNodeList packedNodeList, 
      packedSelfLoopNodeList; 

    LatticeTransition *outTrans;
    NodeIndex fromNodeIndex;
    NodeIndex toNodeIndex;
    LatticeTransition *inTrans;
    LatticeNode *fromNode; 
    VocabIndex context[3];
    LatticeNode *node = findNode(nodeIndex); 
    if (!node) {
	if (debug(DebugPrintFatalMessages)) {
            dout() << "Lattice::expandNodeToTrigram: "
		   << "Fatal Error: current node doesn't exist!\n";
	}
	exit(-1); 
    }

    LatticeTransition * selfLoop = node->inTransitions.find(nodeIndex); 
    Boolean selfLoopFlag; 
    if (selfLoop) { 
      selfLoopFlag = true; 
      initASelfLoopDB(selfLoopDB, lm, nodeIndex, node, selfLoop); 
    } else {
      selfLoopFlag = false; 
    }

    TRANSITER_T<NodeIndex,LatticeTransition> inTransIter(node->inTransitions);
    TRANSITER_T<NodeIndex,LatticeTransition> outTransIter(node->outTransitions);

    VocabIndex wordName = node->word; 

    VocabString nodeName =
      (wordName == Vocab_None) ? "NULL" : getWord(wordName);

    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::expandNodeToTrigram: "
	     << "processing word name: " << nodeName << ", Index: " 
	     << nodeIndex << "\n";
    }

    // going through all its incoming edges
    while (inTrans = inTransIter.next(fromNodeIndex)) {

      if (nodeIndex == fromNodeIndex) {

	if (debug(DebugPrintOutLoop)) {
	  dout() << "Lattice::expandNodeToTrigram: jump over self loop: " 
	         << fromNodeIndex << "\n"; 
	}

	continue; 
      }

      fromNode = findNode(fromNodeIndex); 
      if (!fromNode) {
	if (debug(DebugPrintFatalMessages)) {
	    dout() << "Lattice::expandNodeToTrigram: "
		   << "Fatal Error: fromNode " 
	           << fromNodeIndex << " doesn't exist!\n";
	}
	exit(-1); 
      }
      VocabIndex fromWordName = fromNode->word; 

      if (debug(DebugPrintOutLoop)) {
	VocabString fromNodeName =
	  (fromWordName == Vocab_None) ? "NULL" : getWord(fromWordName);
	dout() << "Lattice::expandNodeToTrigram: processing incoming edge: (" 
	       << fromNodeIndex << ", " << nodeIndex << ")\n" 
	       << "      (" << fromNodeName << ", " << nodeName << ")\n"; 
      }

      // compute in bigram prob
      LogP inWeight; 
      if (fromNodeIndex == getInitial()) {
	context[0] = fromWordName; 
	context[1] = Vocab_None; 
	inWeight = lm.wordProb(wordName, context);
      } else { 
	inWeight = inTrans->weight;
      }

      context[0] = wordName; 
      context[1] = fromWordName; 
      context[2] = Vocab_None; 

      unsigned inFlag = inTrans->flags; 

      // initialize it for self loop processing.
      if (selfLoopFlag) {
	initBSelfLoopDB(selfLoopDB, lm, fromNodeIndex, fromNode, inTrans);
      }

      // going through all the outgoing edges
      //       node = findNode(nodeIndex); 

      outTransIter.init(); 
      while (LatticeTransition * outTrans = outTransIter.next(toNodeIndex)) {
	
	if (nodeIndex == toNodeIndex) {
	  dout() << " In expandNodeToTrigram: self loop: " 
	         << toNodeIndex << "\n"; 
	  
	  continue; 
	}

	LatticeNode * toNode = findNode(toNodeIndex); 
	if (!toNode) {
	    if (debug(DebugPrintFatalMessages)) {
	        dout() << "Lattice::expandNodeToTrigram: "
		       << "Fatal Error: toNode " 
	               << toNode << " doesn't exist!\n";
	    }
	    exit(-1); 
	}

	if (debug(DebugPrintInnerLoop)) {
	  VocabString toNodeName =
	    (toNode->word == Vocab_None) ? "NULL" : getWord(toNode->word);
	  dout() << "Lattice::expandNodeToTrigram: the toNodeIndex (" 
	         << toNodeIndex << " has name " << toNodeName << ")\n"; 
	}

	// initialize selfLoopDB;
	if (selfLoopFlag) { 
	  initCSelfLoopDB(selfLoopDB, toNodeIndex, outTrans); 
	}

	// duplicate a node if the trigram exists.

	// computed on demand in packNodes(), saving work for cached transitions
	// LogP logProb = lm.wordProb(toNode->word, context);
	  
	if (debug(DebugPrintInnerLoop)) {
	  VocabString toNodeName =
	    (toNode->word == Vocab_None) ? "NULL" : getWord(toNode->word);
	  dout() << "Lattice::expandNodeToTrigram: tripleIndex (" 
	         << toNodeIndex << " | " << nodeIndex << ", "
	         << fromNodeIndex << ")\n"
	         << "      trigram prob: (" 
	         << toNodeName << " | " << context << ") found!!!!!!!!\n"; 
	}

	// create one node and two edges to place trigram prob
	// I need to do packing nodes here.

	PackInput packInput;
	packInput.fromWordName = fromWordName;
	packInput.wordName = wordName;
	packInput.toWordName = toNode->word;
	packInput.fromNodeIndex = fromNodeIndex;
	packInput.toNodeIndex = toNodeIndex; 
	packInput.inWeight = inWeight; 
	// computed on demand in packNodes()
	//packInput.outWeight = logProb; 
	packInput.lm = &lm;
	packInput.inFlag = inFlag; 
	packInput.outFlag = outTrans->flags; 
	packInput.nodeIndex = nodeIndex; 
	packInput.toNodeId = 0;
	
	if (debug(DebugPrintInnerLoop)) {
	  dout() << "Lattice::expandNodeToTrigram: "
	         << "outgoing edge for first incoming edge: (" 
	         << nodeIndex << ", " << toNodeIndex << ") is reused\n"; 
	}

	packedNodeList.packNodes(*this, packInput); 

        if (maxNodes > 0 && getNumNodes() > maxNodes) {
	  dout() << "Lattice::expandNodeToTrigram: "
	         << "aborting with number of nodes exceeding "
	         << maxNodes << endl;
	  return false;
	}
	
	// processing selfLoop
	if (selfLoopFlag) { 
	  expandSelfLoop(lm, selfLoopDB, packedSelfLoopNodeList); 
	}
      }	  // end of inter-loop
    } // end of out-loop

    // processing selfLoop case
    if (selfLoopFlag) { 
          node = findNode(nodeIndex);
	  node->inTransitions.remove(nodeIndex);
	  node = findNode(nodeIndex);
	  selfLoop = node->outTransitions.remove(nodeIndex);
	  if (!selfLoop) {
	    dout() << "Lattice::expandNodeToTrigram: "
	           << "nonFatal Error: non symetric setting\n";
	    exit(-1); 
	  }
    }

    // remove bigram transitions along with the old node
    removeNode(nodeIndex); 
    return true; 
}

Boolean 
Lattice::expandToTrigram(LM &lm, unsigned maxNodes)
{
    if (debug(DebugPrintFunctionality)) {
      dout() << "Lattice::expandToTrigram: "
	     << "starting expansion to conventional trigram lattice ...\n";
    }

    unsigned numNodes = getNumNodes(); 

    NodeIndex sortedNodes[numNodes];
    unsigned numReachable = sortNodes(sortedNodes);

    if (numReachable != numNodes) {
      if (debug(DebugPrintOutLoop)) {
	dout() << "Lattice::expandToTrigram: warning: called with unreachable nodes\n";
      }
    }

    for (unsigned i = 0; i < numReachable; i++) {
      NodeIndex nodeIndex = sortedNodes[i];

      if (nodeIndex == initial || nodeIndex == final) {
	continue;
      }
      if (!expandNodeToTrigram(nodeIndex, lm, maxNodes)) {
	return false;
      }
    }
    return true; 
}

/*
 * Expand bigram lattice to trigram, with bigram packing 
 *   (just like in nodearray.)
 *
 *   BASIC ALGORITHM: 
 *      1) foreach node u connecting with the initial NULL node, 
 *              let W be the set of nodes that have edge go into
 *              node u.
 *              a) get the set of outgoing edges e(u) of node u whose 
 *                      the other ends of nodes are not marked as processed.
 *
 *              b) for each edge e = (u, v) in e(u): 
 *                      for each node w in W do:
 *                        i)  if p(v | u, w) exists, 
 *                              duplicate u to get u' ( word name ), 
 *                              and edge (w, u') and (u', v)
 *                              with all the attributes.
 *                              place p(v | u, w) on edge (u', v)
 *
 *                       ii)  if p(v | u, w) does not exist,
 *                              add p(v | u) on edge (u, v)
 *                              multiply bo(w,u) to p(u | w) on (w, u)
 * 
 *  reservedFlag: to indicate that not all the outGoing nodes from the 
 *      current node have trigram probs and this bigram edge needs to be 
 *      preserved for bigram prob.
 */
Boolean 
Lattice::expandNodeToCompactTrigram(NodeIndex nodeIndex, Ngram &ngram,
							unsigned maxNodes)
{
    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::expandNodeToCompactTrigram: \n";
    }

    SelfLoopDB selfLoopDB; 
    PackedNodeList packedNodeList, 
      packedSelfLoopNodeList; 
    LatticeTransition *outTrans;
    NodeIndex fromNodeIndex, backoffNodeIndex;
    NodeIndex toNodeIndex;
    LatticeNode *fromNode; 
    VocabIndex * bowContext = 0; 
    int inBOW = 0; 
    VocabIndex context[3];
    LatticeNode *node = findNode(nodeIndex); 
    if (!node) {
        if (debug(DebugPrintFatalMessages)) {
	  dout() << "Fatal Error in Lattice::expandNodeToCompactTrigram: "
		 << "current node has lost!\n";
	}
	exit(-1); 
    }

    LatticeTransition * selfLoop = node->inTransitions.find(nodeIndex); 
    Boolean selfLoopFlag; 
    if (selfLoop) { 
      selfLoopFlag = true; 
      initASelfLoopDB(selfLoopDB, ngram, nodeIndex, node, selfLoop); 
    } else {
      selfLoopFlag = false; 
    }

    TRANSITER_T<NodeIndex,LatticeTransition> inTransIter(node->inTransitions);
    TRANSITER_T<NodeIndex,LatticeTransition> outTransIter(node->outTransitions);

    VocabIndex wordName = node->word; 

    VocabString nodeName =
      (wordName == Vocab_None) ? "NULL" : getWord(wordName);

    if (debug(DebugPrintOutLoop)) {
      dout() << "Lattice::expandNodeToCompactTrigram: "
	     << " processing word name: " << nodeName << ", Index: " 
	     << nodeIndex << "\n";
    }

    // going through all its incoming edges
    unsigned numInTrans = node->inTransitions.numEntries(); 

    while (LatticeTransition *inTrans = inTransIter.next(fromNodeIndex)) {

      if (nodeIndex == fromNodeIndex) {
	if (debug(DebugPrintInnerLoop)) {
	  dout() << "Lattice::expandNodeToCompactTrigram: "
		 << "jump over self loop: " 
		 << fromNodeIndex << "\n"; 
	}
	continue; 
      }

      fromNode = findNode(fromNodeIndex); 
      if (!fromNode) {
	if (debug(DebugPrintFatalMessages)) {
	  dout() << "Fatal Error in Lattice::expandNodeToCompactTrigram: "
		 << "fromNode " 
		 << fromNodeIndex << " doesn't exist!\n";
	}
	exit(-1); 
      }
      VocabIndex fromWordName = fromNode->word; 

      VocabString fromNodeName =
	(fromWordName == Vocab_None) ? "NULL" : getWord(fromWordName);

      if (debug(DebugPrintInnerLoop)) {
	dout() << "Lattice::expandNodeToCompactTrigram: "
	       << "processing incoming edge: (" 
	       << fromNodeIndex 
	       << ", " << nodeIndex << ")\n"
	       << "      (" << fromNodeName << ", " << nodeName << ")\n"; 
      }

      // compute in-coming bigram prob
      LogP inWeight; 
      if (fromNodeIndex == getInitial()) {
	context[0] = fromWordName; 
	context[1] = Vocab_None; 
	// this transition can have never been processed.
	inWeight = ngram.wordProb(wordName, context);
	inTrans->weight = inWeight; 

	if (debug(DebugPrintInnerLoop)) {
	  dout() << "Lattice::expandNodeToCompactTrigram: "
		 << "processing incoming edge: (" 
		 << fromNodeIndex 
		 << ", " << nodeIndex << ")\n"
		 << "      (" << fromNodeName << ", " << nodeName << ") = "
		 << "  " << inWeight << ";\n"; 
	}
      } else { 
	// the in-coming trans has been processed and 
	// we should preserve this value.
	inWeight = inTrans->weight;
      }

      context[0] = wordName; 
      context[1] = fromWordName; 
      context[2] = Vocab_None; 

      // LogP inWeight = ngram.wordProb(wordName, context);
      unsigned inFlag = inTrans->flags; 

      // initialize it for self loop processing.
      if (selfLoopFlag) {
	initBSelfLoopDB(selfLoopDB, ngram, fromNodeIndex, fromNode, inTrans); }

      // going through all the outgoing edges
      outTransIter.init(); 
      while (LatticeTransition *outTrans = outTransIter.next(toNodeIndex)) {
	
	if (nodeIndex == toNodeIndex) {
	  if (debug(DebugPrintInnerLoop)) {
	    dout() << "Lattice::expandNodeToCompactTrigram: "
		   << "self loop: " 
		   << toNodeIndex << "\n"; 
	  }
	  continue; 
	}

	LatticeNode * toNode = findNode(toNodeIndex); 
	if (!toNode) {
	  if (debug(DebugPrintFatalMessages)) {
	    dout() << "Fatal Error in Lattice::expandNodeToCompactTrigram: "
		   << "toNode " 
		   << toNode << " doesn't exist!\n";
	  }
	  exit(-1); 
	}

	if (debug(DebugPrintInnerLoop)) {
	  VocabString toNodeName =
	    (toNode->word == Vocab_None)?"NULL" : getWord(toNode->word);
	  dout() << "Lattice::expandNodeToCompactTrigram: "
		 << "the toNodeIndex (" 
		 << toNodeIndex << " has name " << toNodeName << ")\n"; 
	}

	// initialize selfLoopDB;
	if (selfLoopFlag) { 
	  initCSelfLoopDB(selfLoopDB, toNodeIndex, outTrans); 
	}

	// duplicate a node if the trigram exists.
	// see class Ngram in file /home/srilm/devel/lm/src/Ngram.h
	
	LogP * triProb; 

	if (triProb = ngram.findProb(toNode->word, context)) {
	  LogP logProb = *triProb; 
	  
	  if (debug(DebugPrintInnerLoop)) {
	    VocabString toNodeName =
	      (toNode->word == Vocab_None)?"NULL" : getWord(toNode->word);
	    dout() << "Lattice::expandNodeToCompactTrigram: "
		   << "tripleIndex (" 
		   << toNodeIndex << " | " << nodeIndex << ", "
		   << fromNodeIndex << ")\n"
		   << "      trigram prob: (" 
		   << toNodeName << " | " << context << ") found!!!!!!!!\n"; 
	  }

	  // create one node and two edges to place trigram prob
	  PackInput packInput;
	  packInput.fromWordName = fromWordName;
	  packInput.wordName = wordName;
	  packInput.toWordName = toNode->word;
	  packInput.fromNodeIndex = fromNodeIndex;
	  packInput.toNodeIndex = toNodeIndex; 
	  packInput.inWeight = inWeight; 
	  packInput.inFlag = inFlag; 
	  packInput.outWeight = logProb; 
	  packInput.outFlag = outTrans->flags; 
	  packInput.nodeIndex = nodeIndex; 
	  packInput.toNodeId = 0;
	  packInput.lm = 0;
	  
	  packedNodeList.packNodes(*this, packInput); 

	  // to remove the outGoing edge if all the outgoing nodes have
	  // trigram probs.
	  if (numInTrans == 1 && 
	      !(outTrans->getFlag(reservedTFlag))) {

	    if (debug(DebugPrintInnerLoop)) {
	      dout() << "Lattice::expandNodeToCompactTrigram: "
		     << "outgoing edge: (" 
		     << nodeIndex << ", " << toNodeIndex << ") is removed\n"; 
	    }
	    removeTrans(nodeIndex, toNodeIndex); 
	  }

	  if (maxNodes > 0 && getNumNodes() > maxNodes) {
	    dout() << "Lattice::expandNodeToCompactTrigram: "
		   << "aborting with number of nodes exceeding "
		   << maxNodes << endl;
	    return false;
	  }
	} else {
	  // there is no trigram prob for this context

	  if (debug(DebugPrintInnerLoop)) {
	    dout() << "Lattice::expandNodeToCompactTrigram: "
		   << "no trigram context (" 
		   << context << ") has been found -- keep " 
		   << fromNodeIndex << "\n"; 
	  }

	  // note down backoff context and in-coming node for 
	  // preservation, in case explicit trigram does not exist.
	  bowContext = context; 
	  outTrans->markTrans(reservedTFlag); 
	  backoffNodeIndex = fromNodeIndex;
	}
	
	// processing selfLoop
	if (selfLoopFlag) { 
	  expandSelfLoop(ngram, selfLoopDB, packedSelfLoopNodeList); 
	}
      }	  // end of inter-loop

      // processing incoming bigram cases.
      if (!bowContext) {
  	  // for this context, all the toNodes have trigram probs

	  if (debug(DebugPrintInnerLoop)) {
	    dout() << "Lattice::expandNodeToCompactTrigram: "
		   << "incoming edge ("
		   << fromNodeIndex << ", " << nodeIndex
		   << ") is removed\n"; 
	  }

  	  removeTrans(fromNodeIndex, nodeIndex); 
      } else {
	  if (debug(DebugPrintInnerLoop)) {
	      dout() << "Lattice::expandNodeToCompactTrigram: "
		     << "updating trigram backoffs on edge("
		     << fromNodeIndex << ", " << nodeIndex << ")\n"; 
	  }

 	  LogP * wordBOW = ngram.findBOW(bowContext);
	  if (!(wordBOW)) {
  	      if (debug(DebugPrintOutLoop)) {
		dout() << "nonFatal Error in Lattice::expandNodeToCompactTrigram: "
		       << "language model - BOW (" 
		       << bowContext << ") missing!\n";
	      }

	      static LogP zerobow = 0.0; 
	      wordBOW = &zerobow; 
	  }

	  LogP logProbW = *wordBOW; 
	  LogP weight = combWeights(inWeight, logProbW); 

	  setWeightTrans(backoffNodeIndex, nodeIndex, weight); 

	  bowContext = 0;
	  inBOW = 1; 
      }

      numInTrans--;
    } // end of out-loop

    // if trigram prob exist for all the tri-node paths
    if (!inBOW) {

        if (debug(DebugPrintInnerLoop)) {
	  dout() << "Lattice::expandNodeToCompactTrigram: "
		 << "node "
		 << nodeName << " (" << nodeIndex
		 << ") has trigram probs for all its contexts\n"
		 << " and its bigram lattice node is removed\n"; 
	}

        removeNode(nodeIndex); 
    } else {
        node = findNode(nodeIndex);
        if (selfLoopFlag) { 
	  node->inTransitions.remove(nodeIndex);
	  node = findNode(nodeIndex);
	  selfLoop = node->outTransitions.remove(nodeIndex);
	  if (!selfLoop) {

 	      if (debug(DebugPrintFatalMessages)) {
		dout() << "nonFatal Error in Lattice::expandNodeToCompactTrigram: "
		       << "non symetric setting \n";
	      }
	      exit(-1); 
	  }
	} 

	// process backoff to bigram weights. 
	TRANSITER_T<NodeIndex,LatticeTransition> 
	  outTransIter(node->outTransitions);
	while (LatticeTransition *outTrans = outTransIter.next(toNodeIndex)) {
	  
	  LatticeNode * toNode = findNode(toNodeIndex);
	  context[0] = wordName; 
	  context[1] = Vocab_None; 
	  LogP weight = ngram.wordProb(toNode->word, context); 

	  setWeightTrans(nodeIndex, toNodeIndex, weight); 
	}
    }

    return true; 
}

Boolean 
Lattice::expandToCompactTrigram(Ngram &ngram, unsigned maxNodes)
{
    if (debug(DebugPrintFunctionality)) {
      dout() << "Lattice::expandToCompactTrigram: "
	     << "starting expansion to compact trigram lattice ...\n";
    }

    unsigned numNodes = getNumNodes(); 

    NodeIndex sortedNodes[numNodes];
    unsigned numReachable = sortNodes(sortedNodes);

    if (numReachable != numNodes) {
      if (debug(DebugPrintOutLoop)) {
	dout() << "Lattice::expandToCompactTrigram: warning: called with unreachable nodes\n";
      }
    }

    for (unsigned i = 0; i < numReachable; i++) {
      NodeIndex nodeIndex = sortedNodes[i];

      if (nodeIndex == initial || nodeIndex == final) {
	continue;
      }
      if (!expandNodeToCompactTrigram(nodeIndex, ngram, maxNodes)) {
	return false;
      }
    }
    return true; 
}

/*
 * Expand lattice to implement general LMs
 * Algorithm: replace each node in lattice with copies that are 
 * associated with specific LM contexts. The mapping 
 *	(original node, context) -> new node
 * is constructed incrementally as the lattice is traversed in topological
 * order.
 *
 *	expandMap[startNode, <s>] := newStartNode;
 *	expandMap[endNode, </s>] := newEndNode;
 * 
 *	for oldNode in topological order
 *	    for expandMap[oldNode, c] = newNode
 *		for oldNode2 in successors(oldNode)
 *		    c2 = lmcontext(c + word(oldNode2));
 *		    find or create expandMap[oldNode2, c2] = newNode2;
 *		    word(newNode2) := word(oldNodes2);
 *		    prob(newNode->newNode2) := P(word(newNode2) | c);
 *	    delete oldNode;
 *	    delete expandMap[oldNode]; # to save space
 */
Boolean
Lattice::expandNodeToLM(VocabIndex oldIndex, LM &lm, unsigned maxNodes, 
		        Map2<NodeIndex, VocabContext, NodeIndex> &expandMap)
{
    Map2Iter2<NodeIndex, VocabContext, NodeIndex>
					    expandIter(expandMap, oldIndex);
    
    NodeIndex *newIndex;
    VocabContext context;

    while (newIndex = expandIter.next(context)) {

	// node structure might have been moved as a result of insertions
	LatticeNode *oldNode = findNode(oldIndex);
	assert(oldNode != 0);

	unsigned contextLength = Vocab::length(context);

	VocabIndex newContext[contextLength + 2];
	Vocab::copy(&newContext[1], context);

	TRANSITER_T<NodeIndex,LatticeTransition> 
			      transIter(oldNode->outTransitions);
	NodeIndex oldIndex2;
	while (LatticeTransition *oldTrans = transIter.next(oldIndex2)) {
	    LatticeNode *oldNode2 = findNode(oldIndex2);
	    assert(oldNode2 != 0);

	    VocabIndex word = oldNode2->word;

	    // determine context used by LM
	    // (use at least one word since that is always given at each node)
	    unsigned usedLength = 1;
	    newContext[0] = word;

	    // find all possible following words and determine maximal context
	    // needed for wordProb computation
	    TRANSITER_T<NodeIndex,LatticeTransition> 
			      transIter2(oldNode2->outTransitions);
	    NodeIndex oldIndex3;
	    while (transIter2.next(oldIndex3)) {
		LatticeNode *oldNode3 = findNode(oldIndex3);
		assert(oldNode3 != 0);
		
		unsigned usedLength2;
		lm.contextID(oldNode3->word, newContext, usedLength2);

		if (usedLength2 > usedLength) {
		    usedLength = usedLength2;
		}
	    }

	    // back-off weight to truncate the context
	    LogP bow = lm.contextBOW(newContext, usedLength);

	    // truncate context to what LM uses
	    VocabIndex saved = newContext[usedLength];
	    newContext[usedLength] = Vocab_None;

	    Boolean found;
	    NodeIndex *newIndex2 =
		expandMap.insert(oldIndex2, (VocabContext)newContext, found);

	    if (!found) {
		// create new node copy and store it in map
		*newIndex2 = dupNode(word, oldNode2->flags);

		if (maxNodes > 0 && getNumNodes() > maxNodes) {
		    dout() << "Lattice::expandNodeToLM: "
			   << "aborting with number of nodes exceeding "
			   << maxNodes << endl;
		    return false;
		}
	    }

	    LatticeTransition newTrans(lm.wordProb(word, context) + bow,
							   oldTrans->flags);
	    insertTrans(*newIndex, *newIndex2, newTrans, 0);

	    // restore full context
	    newContext[usedLength] = saved;
	} 
    }

    // old node (and transitions) is fully replaced by expanded nodes 
    // (and transitions)
    removeNode(oldIndex);

    // save space in expandMap by deleting entries that are no longer used
    expandMap.remove(oldIndex);

    return true;
}

Boolean 
Lattice::expandToLM(LM &lm, unsigned maxNodes)
{
    if (debug(DebugPrintFunctionality)) {
      dout() << "Lattice::expandToLM: "
	     << "starting expansion to general LM ...\n";
    }

    unsigned numNodes = getNumNodes(); 

    NodeIndex sortedNodes[numNodes];
    unsigned numReachable = sortNodes(sortedNodes);

    if (numReachable != numNodes) {
      if (debug(DebugPrintOutLoop)) {
	dout() << "Lattice::expandToLM: warning: called with unreachable nodes\n";
      }
    }

    Map2<NodeIndex, VocabContext, NodeIndex> expandMap;

    // prime expansion map with initial/final nodes
    LatticeNode *startNode = findNode(initial);
    assert(startNode != 0);
    VocabIndex newStartIndex = dupNode(startNode->word, startNode->flags);

    VocabIndex context[2];
    context[1] = Vocab_None;
    context[0] = vocab.ssIndex;
    *expandMap.insert(initial, context) = newStartIndex;

    LatticeNode *endNode = findNode(final);
    assert(endNode != 0);
    VocabIndex newEndIndex = dupNode(endNode->word, endNode->flags);

    context[0] = vocab.seIndex;
    *expandMap.insert(final, context) = newEndIndex;

    for (unsigned i = 0; i < numReachable; i++) {
      NodeIndex nodeIndex = sortedNodes[i];

      if (nodeIndex == final) {
	removeNode(final);
      } else {
        if (!expandNodeToLM(nodeIndex, lm, maxNodes, expandMap)) {
	  return false;
        }
      }
    }

    initial = newStartIndex;
    final = newEndIndex;

    return true; 
}

inline Boolean 
checkLimit(LogP limits[], unsigned level, LogP value, LogP pruning)
{

  LogP margin = pruning; 

  // initialize the limit value.
  if (limits[level] == 1) {
    limits[level+1] = 1; 
    limits[level] = value;
    return true; 
  }
    
  // if the current value is with the margin of the current limit, 
  // accept this node.
  if (limits[level] + margin <= value) {
    // if the current value is better than the limit, reset the limit
    // to the current value.
    if (limits[level] <= value) {
      limits[level] = value;
    }
    return true;
  } else {
    return false;
  }	  
}

Boolean
Lattice::computeNodeEntropy()
{

    NodeIndex nodeIndex;

    double fanInEntropy = 0.0, fanOutEntropy = 0.0;
    int total = 0;

    LHashIter<NodeIndex, LatticeNode> nodeIter(nodes);
    while (LatticeNode *node = nodeIter.next(nodeIndex)) {

      if (debug(DebugPrintInnerLoop)) {
	dout() << "Lattice::computeNodeEntropy: processing nodeIndex " 
	       << nodeIndex << "\n";
      }

      int fanOut = node->outTransitions.numEntries();
      int fanIn = node->inTransitions.numEntries();

      total += fanIn;

      if (nodeIndex == final) { 
	fanOut = 0; 
      }

      if (nodeIndex == initial) {
	fanIn = 0;
      }

      if (fanOut) {
	fanOutEntropy += (double) fanOut*log10(fanOut); 
      }

      if (fanIn) {
	fanInEntropy += (double) fanIn*log10(fanIn); 
      }
    }

    fanOutEntropy = log10(total) - fanOutEntropy/total; 
    fanInEntropy = log10(total) - fanInEntropy/total; 

    double uniform = log10(getNumNodes()); 

    printf("The fan-in/out/uniform entropies are ( %f %f %f )\n", 
	   fanInEntropy, fanOutEntropy, uniform); 

    return true;

}

/*
 * Reduce lattice by collapsing all nodes with the same word
 *	(except those in the exceptions sub-vocabulary)
 */
Boolean
Lattice::collapseSameWordNodes(SubVocab &exceptions)
{
    LHash<VocabIndex, NodeIndex> wordToNodeMap;

    if (debug(DebugPrintFunctionality)) {
      dout() << "Lattice::collapseSameWordNodes: "
	     << "starting with " << getNumNodes() << " nodes\n";
    }

    NodeIndex nodeIndex;
    LHashIter<NodeIndex, LatticeNode> nodeIter(nodes);
    while (LatticeNode *node = nodeIter.next(nodeIndex)) {

	if (node->word != Vocab_None && !exceptions.getWord(node->word)) {
	    Boolean foundP;

	    NodeIndex *oldNode = wordToNodeMap.insert(node->word, foundP);

	    if (!foundP) {
		// word is new, save its node index
		*oldNode = nodeIndex;
	    } else {
		// word has been found before -- merge nodes
		mergeNodes(*oldNode, nodeIndex);
	    }
	}
    }

    if (debug(DebugPrintFunctionality)) {
      dout() << "Lattice::collapseSameWordNodes: "
	     << "finished with " << getNumNodes() << " nodes\n";
    }

    return true;
}

