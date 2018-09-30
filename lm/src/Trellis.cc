/*
 * Trellis.cc --
 *	Finite-state trellis dynamic programming
 *
 */

#ifndef _Trellis_cc_
#define _Trellis_cc_

#ifndef lint
static char Trellis_Copyright[] = "Copyright (c) 1995,1997 SRI International.  All Rights Reserved.";
static char Trellis_RcsId[] = "@(#)$Header: /home/srilm/devel/lm/src/RCS/Trellis.cc,v 1.14 1999/05/13 02:55:44 stolcke Exp $";
#endif

#include <iostream.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "Trellis.h"

#include "LHash.cc"

#define INSTANTIATE_TRELLIS(StateT) \
    INSTANTIATE_LHASH(StateT,TrellisNode<StateT>); \
    template class Trellis<StateT>

template <class StateT>
Trellis<StateT>::Trellis(unsigned size)
    : trellisSize(size)
{
    assert(size > 0);

    trellis = new LHash<StateT, TrellisNode<StateT> >[size];
    assert(trellis != 0);

    init();
}

template <class StateT>
Trellis<StateT>::~Trellis()
{
    delete [] trellis;
}

template <class StateT>
void
Trellis<StateT>::clear()
{
    delete [] trellis;

    trellis = new LHash<StateT, TrellisNode<StateT> >[trellisSize];
    assert(trellis != 0);
    init();
}

template <class StateT>
void
Trellis<StateT>::init(unsigned time)
{
    assert(time < trellisSize);

    currTime = time;
    initSlice(trellis[currTime]);
}

template <class StateT>
void
Trellis<StateT>::step()
{
    currTime ++;
    assert(currTime < trellisSize);

    initSlice(trellis[currTime]);
}

template <class StateT>
void
Trellis<StateT>::setProb(StateT state, LogP prob)
{
    LHash<StateT, TrellisNode<StateT> > &currSlice = trellis[currTime];

    Boolean foundP;
    TrellisNode<StateT> *node = currSlice.insert(state, foundP);

    node->lprob = prob;

    if (!foundP) {
	node->max = prob;
	Map_noKey(node->prev);
	node->backlpr = LogP_Zero;
	node->backmax = LogP_Zero;
    } else {
	if (prob > node->max) {
	    node->max = prob;
	}
    }
}

template <class StateT>
LogP
Trellis<StateT>::getLogP(StateT state, unsigned time)
{
    assert(time <= currTime);
    LHash<StateT, TrellisNode<StateT> > &currSlice = trellis[time];

    TrellisNode<StateT> *node = currSlice.find(state);

    if (node) {
	return node->lprob;
    } else {
	return LogP_Zero;
    }
}

template <class StateT>
LogP
Trellis<StateT>::getMax(StateT state, unsigned time, LogP &backmax)
{
    assert(time <= currTime);
    LHash<StateT, TrellisNode<StateT> > &currSlice = trellis[time];

    TrellisNode<StateT> *node = currSlice.find(state);

    if (node) {
	backmax = node->backmax;
	return node->max;
    } else {
	backmax = LogP_Zero;
	return LogP_Zero;
    }
}

template <class StateT>
void
Trellis<StateT>::update(StateT oldState, StateT newState, LogP trans)
{
    assert(currTime > 0);
    LHash<StateT, TrellisNode<StateT> > &lastSlice = trellis[currTime - 1];
    LHash<StateT, TrellisNode<StateT> > &currSlice = trellis[currTime];

    TrellisNode<StateT> *oldNode = lastSlice.find(oldState);
    if (!oldNode) {
	/*
	 * If the predecessor state doesn't exist its probability is
	 * implicitly zero and we have nothing to do!
	 */
	return;
    } else {
	Boolean foundP;
	TrellisNode<StateT> *newNode = currSlice.insert(newState, foundP);

	/*
	 * Accumulate total forward prob
	 */
	LogP2 newProb = oldNode->lprob + trans;
	if (!foundP) {
	    newNode->lprob = newProb;
	    newNode->backlpr = LogP_Zero;
	    newNode->backmax = LogP_Zero;
	} else {
	    newNode->lprob = AddLogP(newNode->lprob, newProb);
	}

	/*
	 * Update maximal state prob and Viterbi links
	 */
	LogP totalProb = oldNode->max + trans;
	if (!foundP || totalProb > newNode->max) {
	    newNode->max = totalProb;
	    newNode->prev = oldState;
	}
    }
}

template <class StateT>
void
Trellis<StateT>::initSlice(LHash<StateT, TrellisNode<StateT> > &slice)
{
    LHashIter<StateT, TrellisNode<StateT> > iter(slice);
    TrellisNode<StateT> *node;
    StateT state;

    while (node = iter.next(state)) {
	node->lprob = LogP_Zero; 
	node->max = LogP_Zero;
	Map_noKey(node->prev);
	node->backlpr = LogP_Zero;
	node->backmax = LogP_Zero;
    }
}

template <class StateT>
LogP
Trellis<StateT>::sumLogP(unsigned time)
{
    assert(time <= currTime);

    return sumSlice(trellis[time]);
}

template <class StateT>
LogP
Trellis<StateT>::sumSlice(LHash<StateT, TrellisNode<StateT> > &slice)
{
    LHashIter<StateT, TrellisNode<StateT> > iter(slice);
    TrellisNode<StateT> *node;
    StateT state;

    LogP2 sum = LogP_Zero;
    while (node = iter.next(state)) {
	sum = AddLogP(sum, node->lprob);
    }

    return sum;
}

template <class StateT>
StateT
Trellis<StateT>::max(unsigned time)
{
    assert(time <= currTime);

    return maxSlice(trellis[time]);
}

template <class StateT>
StateT
Trellis<StateT>::maxSlice(LHash<StateT, TrellisNode<StateT> > &slice)
{
    LHashIter<StateT, TrellisNode<StateT> > iter(slice);
    TrellisNode<StateT> *node;
    StateT state;

    StateT maxState;
    Map_noKey(maxState);

    LogP maxProb = LogP_Zero;

    while (node = iter.next(state)) {
	if (Map_noKeyP(maxState) || node->max > maxProb) {
	    maxProb = node->max;
	    maxState = state;
	}
    }

    return maxState;
}

template <class StateT>
unsigned
Trellis<StateT>::viterbi(StateT *path, unsigned length)
{
    StateT lastState;
    Map_noKey(lastState);
    return viterbi(path, length, lastState);
}

template <class StateT>
unsigned
Trellis<StateT>::viterbi(StateT *path, unsigned length, StateT lastState)
{
    if (length > currTime + 1) {
	length = currTime + 1;
    }

    assert(length > 0);

    StateT currState;

    /*
     * Backtrace from the last state with maximum score, unless the caller
     * has given us a specific one to start with.
     */
    if (Map_noKeyP(lastState)) {
    	currState = maxSlice(trellis[length - 1]);
    } else {
	currState = lastState;
    }

    unsigned pos = length;
    while (!Map_noKeyP(currState)) {
	assert(pos > 0);
	pos --;
	path[pos] = currState;

	LHash<StateT, TrellisNode<StateT> > &currSlice = trellis[pos];
	TrellisNode<StateT> *currNode = currSlice.find(currState);
	assert(currNode != 0);
	
	currState = currNode->prev;
    }
    if (pos != 0) {
	/*
	 * Viterbi backtrace failed before reaching start of sentence,
	 */
	return 0;
    } else {
	return length;
    }
}

template <class StateT>
void
Trellis<StateT>::setBackProb(StateT state, LogP prob)
{
    LHash<StateT, TrellisNode<StateT> > &currSlice = trellis[backTime];

    Boolean foundP;
    TrellisNode<StateT> *node = currSlice.find(state, foundP);

    if (node) {
	node->backlpr = prob;
	if (prob > node->backmax) {
	    node->backmax = prob;
	}
    } else {
	cerr << "trying to set backward prob for nonexistent node " << state
	     << " at time " << backTime << endl;
    }
}

template <class StateT>
LogP
Trellis<StateT>::getBackLogP(StateT state, unsigned time)
{
    assert(time <= currTime);
    LHash<StateT, TrellisNode<StateT> > &currSlice = trellis[time];

    TrellisNode<StateT> *node = currSlice.find(state);

    if (node) {
	return node->backlpr;
    } else {
	return LogP_Zero; 
    }
}

template <class StateT>
void
Trellis<StateT>::initBack(unsigned time)
{
    assert(time <= currTime);

    backTime = time;
}

template <class StateT>
void
Trellis<StateT>::stepBack()
{
    assert(backTime > 0);
    backTime --;
}

template <class StateT>
void
Trellis<StateT>::updateBack(StateT oldState, StateT newState, LogP trans)
{
    assert(backTime != (unsigned)-1);	/* check to underflow */

    LHash<StateT, TrellisNode<StateT> > &nextSlice = trellis[backTime + 1];
    LHash<StateT, TrellisNode<StateT> > &currSlice = trellis[backTime];

    TrellisNode<StateT> *nextNode = nextSlice.find(newState);
    if (!nextNode) {
	/*
	 * If the successor state doesn't exist its probability is
	 * implicitly zero and we have nothing to do!
	 */
	return;
    } else {
	Boolean foundP;
	TrellisNode<StateT> *thisNode = currSlice.find(oldState, foundP);

        if (!thisNode) {
	    cerr << "trying to update backward prob for nonexistent node "
		 << oldState << " at time " << backTime << endl;
	    return;
	}

	/*
	 * Accumulate total backword prob
	 */
	LogP2 thisProb = nextNode->backlpr + trans;
	thisNode->backlpr = AddLogP(thisNode->backlpr, thisProb);

	LogP totalMax = nextNode->backmax + trans;
	if (totalMax > thisNode->backmax) {
	    thisNode->backmax = totalMax;
	}
    }
}

/*
 * Iteration over states in a trellis slice
 */

template <class StateT>
TrellisIter<StateT>::TrellisIter(Trellis<StateT> &trellis, unsigned time)
    : sliceIter(trellis.trellis[time])
{
    assert(time <= trellis.currTime);
}

template <class StateT>
void
TrellisIter<StateT>::init()
{
    sliceIter.init();
}

template <class StateT>
Boolean
TrellisIter<StateT>::next(StateT &state, LogP &prob)
{
    TrellisNode<StateT> *node = sliceIter.next(state);

    if (node) {
	prob = node->lprob;
	return true;
    } else {
	return false;
    }
}

#endif /* _Trellis_cc_ */
