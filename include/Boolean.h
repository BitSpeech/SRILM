/*
 * Boolean Type
 *
 * Copyright (c) 1995, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/speech/stolcke/project/srilm/devel/misc/src/RCS/Boolean.h,v 1.2 1996/05/30 17:57:00 stolcke Exp $
 *
 */

#ifndef _BOOLEAN_H_
#define _BOOLEAN_H_

#ifdef __GNUG__
typedef bool Boolean;

#else /* ! __GNUG__ */

typedef int Boolean;
const Boolean false = 0;
const Boolean true = 1;
#endif /* __GNUG __*/

#endif /* _BOOLEAN_H_ */
