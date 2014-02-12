/*
// cmdassert.h
// magi@cs.stanford.edu
// 2/3/2000
//
// non-fatal equivalent of assert for use in Tcl command-processing
// functions, say when something doesn't like its arguments -- instead
// of calling abort(), we'll just return TCL_ERROR.
// The mechanics here are basically just stolen from assert.h.
*/

#ifndef _CMDASSERT_H_
#define _CMDASSERT_H_

#ifdef __cplusplus
extern "C" {
#endif


#ifdef NDEBUG
#undef cmdassert
#define cmdassert(EX) ((void)0)

#else

extern void __cmdassert(const char *, const char *, int);
#ifdef __ANSI_CPP__
#define cmdassert(EX)  if (!(EX)) { \
         __cmdassert( # EX , __FILE__, __LINE__); \
         interp->result = "Non-fatal assertion error in command!"; \
         return TCL_ERROR; \
}

#else
#define cmdassert(EX)  if (!(EX)) { \
         __cmdassert("EX", __FILE__, __LINE__); \
         interp->result = "Non-fatal assertion error in command!"; \
         return TCL_ERROR; \
}
#endif
#endif /* NDEBUG */

#ifdef __cplusplus
}
#endif

#endif /* _CMDASSERT_H_ */
