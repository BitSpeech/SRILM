/*
 * Discount.h --
 *	Discounting schemes
 */


#ifndef _Discount_h_
#define _Discount_h_

#include "Boolean.h"
#include "File.h"
#include "Array.h"
#include "Debug.h"

#include "NgramStats.h"

const Count GT_defaultMinCount = 1;
const Count GT_defaultMaxCount = 5;

/*
 * Discount --
 *	A method to manipulate counts for estimation purposes.
 *	Typically, a count > 0 is adjusted downwards to free up
 *	probability mass for unseen (count == 0) events.
 */
class Discount: public Debug
{
public:
    virtual ~Discount() { };

    virtual double discount(Count count, Count totalCount, Count observedVocab)
	{ return 1.0; };	    /* discount coefficient for count */

    virtual double discount(FloatCount count, FloatCount totalCount,
						    Count observedVocab)
	/*
	 * By default, we discount float counts by discounting the
	 * integer ceiling value.
	 */
	{ return discount((Count)ceil(count), (Count)ceil(totalCount),
							    observedVocab); };

    virtual Boolean nodiscount() { return false; };
				    /* check if discounting disabled */
    virtual void write(File &file) {};
				    /* save parameters to file */
    virtual Boolean read(File &file) { return false; };
				    /* read parameters from file */

    virtual Boolean estimate(NgramStats &counts, unsigned order)
	/*
	 * dummy estimator for when there is nothing to estimate
	 */
	{ return true; };
    virtual Boolean estimate(NgramCounts<FloatCount> &counts, unsigned order)
	/*
	 * by default, don't allow discount estimation from fractional counts
	 */
	{ return false; }
};


/*
 * GoodTuring --
 *	The standard discounting method based on count of counts
 */
class GoodTuring: public Discount
{
public:
    GoodTuring(unsigned mincount = GT_defaultMinCount,
	       unsigned maxcount = GT_defaultMaxCount);
    ~GoodTuring() {};		   /* works around g++ 2.7.2 bug */

    virtual double discount(Count count, Count totalCount, Count observedVocab);
    Boolean nodiscount();

    void write(File &file);
    Boolean read(File &file);

    Boolean estimate(NgramStats &counts, unsigned order);
    Boolean estimate(NgramCounts<FloatCount> &counts, unsigned order)
	{ return false; };

private:
    Count minCount;		    /* counts below this are set to 0 */
    Count maxCount;		    /* counts above this are unchanged */

    Array<double> discountCoeffs;   /* cached discount coefficients */
};

/*
 * ConstDiscount --
 *	Ney's method of subtracting a constant <= 1 from all counts
 */
class ConstDiscount: public Discount
{
public:
    ConstDiscount(double d, unsigned mincount = 0)
	: _discount(d < 0.0 ? 0.0 : d > 1.0 ? 1.0 : d),
	  _mincount(mincount) {};

    virtual double discount(Count count, Count totalCount, Count observedVocab)
      { return (count <= 0) ? 1.0 : (count < _mincount) ? 0.0 : 
					(count - _discount) / count; };

    Boolean nodiscount() { return _mincount <= 1.0 && _discount == 0.0; } ;

private:
    double _discount;		    /* the discounting constant */
    double _mincount;		    /* minimum count to retain */
};


/*
 * NaturalDiscount --
 *	Ristad's natural law of succession
 */
class NaturalDiscount: public Discount
{
public:
    NaturalDiscount(unsigned mincount = 0)
	: vocabSize(0), _mincount(mincount) {};

    double discount(Count count, Count totalCount, Count observedVocab);
    Boolean nodiscount() { return false; };

    Boolean estimate(NgramStats &counts, unsigned order);
    Boolean estimate(NgramCounts<FloatCount> &counts, unsigned order) 
       { return false; };

private:
    unsigned vocabSize;		    /* vocabulary size */
    double _mincount;		    /* minimum count to retain */
};

/*
 * WittenBell --
 *	Witten & Bell's method of estimating the probability of an
 *	unseen event by the total number of 'new' events overserved,
 *	i.e., counting each observed word type once.
 */
class WittenBell: public Discount
{
public:
    WittenBell(unsigned mincount = 0) : _mincount(mincount) {};

    virtual double discount(Count count, Count totalCount, Count observedVocab)
      { return (count <= 0) ? 1.0 : (count < _mincount) ? 0.0 : 
      			((double)totalCount / (totalCount + observedVocab)); };
    virtual double discount(FloatCount count, FloatCount totalCount,
							Count observedVocab)
      { return (count <= 0) ? 1.0 : (count < _mincount) ? 0.0 : 
      			((double)totalCount / (totalCount + observedVocab)); };

    Boolean nodiscount() { return false; };

    Boolean estimate(NgramStats &counts, unsigned order)
      { return true; } ;
    Boolean estimate(NgramCounts<FloatCount> &counts, unsigned order)
      { return true; } ;
	
private:
    double _mincount;		    /* minimum count to retain */
};


#endif /* _Discount_h_ */


