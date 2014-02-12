/*
 * winGLdecs.h -- stupid stuff to make non-Windows programs that use GL compile,
 * because the way the compilers ship you can't include <GL\GL.h> without having
 * already included <windows.h>.
 *
 * 10/2/98  magi@cs.stanford.edu
 */


#ifndef _WINAPI_FOR_GL_
#define _WINAPI_FOR_GL_

/* from winnt.h */
#if (defined(_M_MRX000) || defined(_M_IX86) || defined(_M_ALPHA) || defined(_M_PPC)) && !defined(MIDL_PASS)
#define DECLSPEC_IMPORT __declspec(dllimport)
#else
#define DECLSPEC_IMPORT
#endif

/* from wingdi.h */
#if !defined(_GDI32_)
#define WINGDIAPI DECLSPEC_IMPORT
#else
#define WINGDIAPI
#endif

#ifndef WINAPI
/* from windef.h */
#if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#else
#define CALLBACK
#define WINAPI
#endif
#endif

#ifndef APIENTRY
#define APIENTRY    WINAPI
#endif

/* from stddef.h */
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif


#endif /* _WINAPI_FOR_GL_ */
