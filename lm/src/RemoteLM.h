/*
 * RemoteLM.h
 *	Protocol for network-based LM
 *
 * Copyright (c) 2007, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/lm/src/RCS/RemoteLM.h,v 1.1 2007/12/04 00:13:59 stolcke Exp $
 *
 */

#ifndef _RemoteLM_h_
#define _RemoteLM_h_

#define SRILM_DEFAULT_PORT      2525

#define REMOTELM_VERSION2	"_R_E_M_O_T_E_L_M_V=2"	// an unlikely word ...

/*
 * Procedure calls
 * 	
 */
#define REMOTELM_WORDPROB	"W"		// W context word
#define REMOTELM_CONTEXTID1	"C1"		// C1 context
#define REMOTELM_CONTEXTID2	"C2"		// C2 context word
#define REMOTELM_CONTEXTBOW	"B"		// B context word

/*
 * Return codes
 */
#define REMOTELM_OK		"OK"		// followed by return values
#define REMOTELM_ERROR		"ERROR"		// followed by error string

#define REMOTELM_MAXRESULTLEN	256

#endif /* _RemoteLM_h_ */
