/*
 * tclmain.c --
 *	main() function for tcl clients
 *
 * $Header: /home/srilm/devel/misc/src/RCS/tclmain.cc,v 1.5 1999/08/01 18:07:12 stolcke Exp $
 */

#include <tcl.h>

/*
 * Tcl versions up to 7.3 defined main() in the libtcl.a
 */
#if (TCL_MAJOR_VERSION == 7 && TCL_MINOR_VERSION > 3) || (TCL_MAJOR_VERSION > 7)

extern "C" 
int
main(int argc, char **argv)
{
   Tcl_Main(argc, argv, Tcl_AppInit);
}

#endif

