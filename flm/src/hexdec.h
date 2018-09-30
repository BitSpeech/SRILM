/*
 * hexdec.h --
 *	portability for hex/dec printing.
 *
 * Jeff Bilmes <bilmes@ee.washington.edu>
 *
 * @(#)$Header: /home/srilm/devel/flm/src/RCS/hexdec.h,v 1.4 2005/09/24 14:26:54 stolcke Exp $
 *
 */

#ifndef _hexdec_h_
#define _hexdec_h_

#ifndef EXCLUDE_CONTRIB

// ok, now we're down to quick hacks, this one to get around 
// gcc 2.95.3 and gcc 3.x.x handling of hex/dec printing
// in c++'s lovely io lib. Define HEX to hex to get it working
// with 2.95.3, and define it to the other one to get it to
// work with gcc 3.x.x

#if defined(__GNUC__) && __GNUC__ <= 2

#define HEX hex
#define DEC dec

#define SHOWBASE(s)	/* don't know */

#else 

#include <ios>
#include <iomanip>

#define HEX std::setbase(16)
#define DEC std::setbase(10)

#define SHOWBASE(s)	((s).setf(std::ios_base::showbase))

#endif

#endif /* EXCLUDE_CONTRIB_END */

#endif /* _hexdec_h_ */
