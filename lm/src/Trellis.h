/*
 * Trellis.h --
 *	Trellises for dynamic programming finite state models
 *
 * Copyright (c) 1995-2006 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/Trellis.h,v 1.20 2010/09/15 21:41:10 stolcke Exp $
 *
 */

#ifndef _Trellis_h_
#define _Trellis_h_

#ifdef PRE_ISO_CXX
# include <iostream.h>
# include <iomanip.h>
#else
# include <iostream>
# include <iomanip>
using namespace std;
#endif

#include "LHash.h"

#include "Boolean.h"
#include "Prob.h"
#include "MemStats.h"

#ifdef max
#undef max	// avoid conflict with some system headers
#endif

/*
 * Forward declarations
 */
template <class StateT> class Trellis;
template <class StateT> class TrellisNode;
template <class StateT> class TrellisSlice;
template <class StateT> class TrellisNBestList;
template <class StateT> class TrellisIter;


/* ---------------Trellis node class------------------------------------------
 *
 * A node in the trellis.  Each node contains an nbestList that is a vector of
 * hypotheses.  See the next class for a description of the contents of the
 * hypothesis.
 */
template <class StateT> class TrellisNode;
template <class StateT> inline ostream& operator <<(ostream& os, const TrellisNode<StateT>& node);

template <class StateT>
class TrellisNode
{
    friend ostream& operator<< <StateT>(ostream& os,
					const TrellisNode<StateT>& node);
    friend class Trellis<StateT>;
    friend class TrellisSlice<StateT>;
    friend class TrellisIter<StateT>;

public:
    unsigned nbestSize() const { return nbest.size(); };
    void clear() { nbest.init(0); };
  
private:
    LogP lprob;				// total forward probability
    LogP backlpr;			// total backword probability
    LogP backmax;			// maximum backward probability

    TrellisNBestList<StateT> nbest;	// list of n-best paths
};

/*
 * At the moment, we only print out the node's nbest list when asked to
 * print out the node contents.
 */
template <class StateT>
inline ostream&
operator <<(ostream& os, const TrellisNode<StateT>& node) {
    return os << node.nbest;
}

/*---------------N-Best Hyp and List classes---------------------------------
 *
 * The (N-Best) Hypothesis class.  Each hyp consists of a score (usually
 * the log prob), a back pointer to the previous state and the index
 * of the particular nbest hypothesis at this previous state to which
 * the current one backpoints.
 */
template <class StateT>
struct Hyp {
    LogP score;				// nbest score
    StateT prev;			// Viterbi backpointer
    int whichbest;

    Hyp(): score(LogP_Zero), whichbest(0) { Map_noKey(prev); };
    Hyp(LogP s, StateT b, int w): score(s), prev(b), whichbest(w) {};
};

template <class StateT>
inline ostream&
operator<<(ostream& os, const Hyp<StateT>& h) {
    if (Map_noKeyP(h.prev)) {
	return os << "(" << std::setw(2) << std::setprecision(2) << h.score
		  << ", ," << h.whichbest << ")";
    } else {
	return os << "(" << std::setw(2) << std::setprecision(2) << h.score
		  << "," << h.prev << "," << h.whichbest << ")";
    }
}

/*
 * The nbest list class is just an array of hypotheses.  The size itself is
 * not set in the constructor.  So when a node in the trellis is
 * initialized, we must explicitly set the size of that state's nbest list
 * to the numNbest member of Trellis.
 */
template <class StateT> class TrellisNBestList;
template <class StateT> inline ostream& operator<< (ostream& os, const TrellisNBestList<StateT>& nbest);

template <class StateT>
class TrellisNBestList {
    friend ostream& operator<< <StateT>(ostream& os,
					const TrellisNBestList<StateT>& nbest);

public:
    TrellisNBestList(unsigned num = 0);
    ~TrellisNBestList();
  
    unsigned size() const { return numNbest; };
    void init(unsigned newSize = 1);
    Hyp<StateT>& operator[](unsigned n) { return nblist[n]; };
    const Hyp<StateT>& operator[](unsigned n) const { return nblist[n]; };
    unsigned findrank(const Hyp<StateT>& hyp) const;
    void insert(const Hyp<StateT>& h);

private:
    unsigned numNbest;
    Hyp<StateT> *nblist;
};

template <class StateT>
inline ostream&
operator<< (ostream& os, const TrellisNBestList<StateT>& nbest)
{
    for (unsigned n = 0; n < nbest.size(); n++) {
	os << nbest.nblist[n] << " ";
    }
    return os;
}

/*---------------Trellis time slice class------------------------------------
 *
 * The time-slice class.  This consists of a hash-map of nodes keyed by
 * states and a global N-best list at this time slice.  The global N-best lisT
 * is necessary for the following reason:  Suppose we required the nbest list
 * for a particular slice -- this is *NOT* the nbest list of the state that
 * ends the most likely path at this slice.  The next best path could end
 * at a different state.  We thus need to compute the n-best of the union
 * of all the nbest paths from this time slice.  That is what the global list
 * stores.
 */

template <class StateT> class TrellisSlice;
template <class StateT> ostream& operator<<(ostream& os, const TrellisSlice<StateT>& slice);

template <class StateT>
class TrellisSlice
{
    friend class Trellis<StateT>;
    friend class TrellisIter<StateT>;
    friend ostream& operator<< <StateT>(ostream& os,
					const TrellisSlice<StateT>& slice);

public:
    ~TrellisSlice();
    void init();
    unsigned size() const { return nodes.size(); };
    LogP sum();
    StateT max();
    TrellisNode<StateT>& operator[](StateT s) { return nodes[s]; };
    TrellisNode<StateT>& operator[](StateT s) const { return nodes[s]; };
    TrellisNBestList<StateT>& nbest(unsigned numNbest);

    TrellisNode<StateT> *insert(StateT s, Boolean& foundP)
      { return nodes.insert(s, foundP); };
    TrellisNode<StateT> *find(StateT s) { return nodes.find(s); };
    TrellisNode<StateT> *find(StateT s) const { return nodes.find(s); };

private:
    LHash<StateT, TrellisNode<StateT> > nodes;
    TrellisNBestList<StateT> globalNbest;
};

/*---------------Trellis Class-----------------------------------------------
 *
 * The Trellis class. This is implemented as an array of time slices
 * stored in the member trellis.
 */
template <class StateT> class Trellis;
template <class StateT> inline ostream& operator<<(ostream& os, const Trellis<StateT>& trellis);

template <class StateT>
class Trellis
{
    friend class TrellisIter<StateT>;
    friend ostream& operator<< <StateT> (ostream& os, const Trellis<StateT>& trellis);

public:
    Trellis(unsigned len, unsigned numNbest = 1);
    ~Trellis();

    unsigned where() { return currTime; };	/* current time index */

    unsigned size() const { return trellisSize; };
    void clear();				/* remove all states */
    void init(unsigned time = 0);		/* start DP for time index 0 */
    void step();	      /* step and initialize next time index */

    TrellisSlice<StateT>& operator[](unsigned t) { return trellis[t]; };
    const TrellisSlice<StateT>& operator[](unsigned t) const
	{ return trellis[t]; };

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

    unsigned viterbi(StateT *path, unsigned len);
    unsigned viterbi(StateT *path, unsigned len, StateT lastState)
      { LogP dummy; return nbest_viterbi(path, len, 0, dummy, lastState); };
    unsigned nbest_viterbi(StateT *path, unsigned len, unsigned nth)
      { LogP dummy; return nbest_viterbi(path, len, nth, dummy); };
    unsigned nbest_viterbi(StateT *path, unsigned len, unsigned nth,
						LogP &score);
    unsigned nbest_viterbi(StateT *path, unsigned len, unsigned nth,
						LogP &score, StateT lastState);

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
    void stepBack();	                        /* step back in time */
    void updateBack(StateT oldState, StateT newState, LogP trans);
      /* update backward probs with a transition */

private:
    TrellisSlice<StateT> *trellis;
    unsigned trellisSize;

    unsigned currTime;		/* current time index */
    unsigned backTime;		/* current backward time index */
    unsigned numNbest;	        /* number of nbest paths */

};

template <class StateT>
inline ostream&
operator<<(ostream& os, const Trellis<StateT>& trellis) {
    for (unsigned i = 0; i < trellis.size(); i++) {
	os << " Slice " << i << endl << trellis[i];
    }
    return os;
}

/*
 * Iteration over states in a trellis slice
 */
template <class StateT>
class TrellisIter
{
public:
    TrellisIter(Trellis<StateT> &trellis, unsigned t);

    void init();
    Boolean next(StateT &state, LogP &prob);

private:
    LHashIter<StateT, TrellisNode<StateT> > sliceIter;
};

#endif /* _Trellis_h_ */
