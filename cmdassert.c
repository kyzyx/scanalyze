/*
// cmdassert.c
// magi@cs.stanford.edu
// 2/3/2000
//
// non-fatal equivalent of assert for use in Tcl command-processing
// functions, say when something doesn't like its arguments -- instead
// of calling abort(), we'll just return TCL_ERROR.
// The mechanics here are basically just stolen from assert.h.
*/

#include <stdio.h>

void __cmdassert(const char * expr, const char * file, int line)
{
  fprintf (stderr, "Benign assertion failed: %s, file %s, line %d\n",
	   expr, file, line);
}
