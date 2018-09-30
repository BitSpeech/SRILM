/*
 * Trellis.h --
 *	Trellises for dynamic programming finite state models
 *
 * Copyright (c) 1995,1997 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/Trellis.h,v 1.11 1999/05/13 02:55:44 stolcke Exp $
 *
 */

#ifndef _Trellis_h_
#define _Trellis_h_

#include "Boolean.h"
#include "Prob.h"
#include "LHash.h"
#include "MemStats.h"

template <class StateT> class Trellis;		// forward declaration
template <class StateT> class TrellisIter;	// forward declaration

/*
 * A node in the trellis
 */
template <class StateT>
class TrellisNode
{
    friend class Trellis<StateT>;
    friend class TrellisIter<StateT>;

    LogP lprob;		/* total forward probability */
    LogP backlpr;	/* total backword probability */
    LogP max;		/* maximum forward probability */
    LogP backmax;	/* maximum backward probability */
    StateT prev;	/* Viterbi backpointer */
};

template <class StateT>
class Trellis
{
    friend class TrellisIter<StateT>;
public:
    Trellis(unsigned size);
    ~Trellis();

    unsigned where() { return currTime; };	/* current time index */

    void clear();				/* remove all states */
    void init(unsigned time = 0);	/* start DP for time index 0 */
    void step();	/* step and initialize next time index */

    void setProb(StateT state, LogP prob);
    Prob getProb(StateT state) { return getProb(state, currTime); };
    Prob getProb(StateT state, unsigned time)
      { return LogPtoProb(getLogP(state, time)); };
    LogP getLogP(StateT state) { return getLogP(state, currTime); };
    LogP getLogP(StateT state, unsigned time);

    LogP getMax(StateT state) { return getMax(state, currTime); };
    LogP getMax(StateT state, unsigned time)
      { LogP dummy; return getMax(state, time, dummy); };
    LogP getMax(StateT state, unsigned time, LogP &backmax);

    void update(StateT oldState, StateT newState, LogP trans);
			/* update DP with a transition */

    LogP sumLogP(unsigned time);	/* sum of all state probs */
    Prob sum(unsigned time) { return LogPtoProb(sumLogP(time)); };
    StateT max(unsigned time);		/* maximum prob state */

    unsigned viterbi(StateT *path, unsigned length);
    unsigned viterbi(StateT *path, unsigned length, StateT lastState);

    void setBackProb(StateT state, LogP prob);
    Prob getBackProb(StateT state)
      { return getBackProb(state, backTime); };
    Prob getBackProb(StateT state, unsigned time)
      { return LogPtoProb(getBackLogP(state, time)); };
    LogP getBackLogP(StateT state)
      { return getBackLogP(state, backTime); };
    LogP getBackLogP(StateT state, unsigned time);

    void initBack() { initBack(currTime); };	/* start backward DP */
    void initBack(unsigned time);		/* start backward DP */
    void stepBack();	/* step back in time */
    void updateBack(StateT oldState, StateT newState, LogP trans);
			/* update backward probs with a transition */

    void memStats(MemStats &stats);

private:
    LHash<StateT, TrellisNode<StateT> > *trellis;
    unsigned trellisSize;	/* maximum time index */

    unsigned currTime;		/* current time index */
    unsigned backTime;		/* current backward time index */

    void initSlice(LHash<StateT, TrellisNode<StateT> > &slice);
    LogP sumSlice(LHash<StateT, TrellisNode<StateT> > &slice);
    StateT maxSlice(LHash<StateT, TrellisNode<StateT> > &slice);
};

/*
 * Iteration over states in a trellis slice
 */
template <class StateT>
class TrellisIter
{
public:
    TrellisIter(Trellis<StateT> &trellis, unsigned time);

    void init();
    Boolean next(StateT &state, LogP &prob);

private:
    LHashIter<StateT, TrellisNode<StateT> > sliceIter;
};

#endif /* _Trellis_h_ */
