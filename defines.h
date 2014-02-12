#ifndef _DEFINES_H_
#define _DEFINES_H_

#include <malloc.h>
#include <string.h>
#ifdef WIN32
#	include "winGLdecs.h"
#endif

#ifndef NULL
#define NULL    0
#endif

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef SWAP
#define SWAP(a, b, t) (t) = (a); (a) = (b); (b) = (t)
#endif

#ifndef SQUARE
#define SQUARE(x) ((x)*(x))
#endif

#ifndef ROUND_UCHAR
#define ROUND_UCHAR(x) (uchar)((x)+0.5)
#endif

#ifndef ROUND_INT
#define ROUND_INT(x) (int)((x)+0.5)
#endif

#ifndef ABS
#define ABS(x) ((x) > 0 ? (x) : -(x))
#endif

#ifndef SIGN
#define SIGN(x) ((x) > 0 ? 1 : -1)
#endif

#ifndef DEGTORAD
#define DEGTORAD(x) ((x)*M_PI/180)
#endif

#ifndef RADTODEG
#define RADTODEG(x) ((x)*180/M_PI)
#endif


#ifndef MALLOC
#define MALLOC(x, n) (x*)malloc((n)*sizeof(x))
#endif

#ifndef newmalloc
#define newmalloc(x, n) (x*)malloc((n)*sizeof(x))
#endif

#ifndef PI
#define PI 3.14159265358979323846264
#endif

#ifdef WIN32

/* This is stuff that the stnadard include files on Irix define somewhere
   or other, but that MSVC does not include.

   Note that defining PATH_MAX here causes extreme problems on Irix if the
   value is different than that in <limits.h>, depending on which value is
   used in a given situation.  And it's too hard to get at the one in
   <limits.h> from MSVC; so we define it here but only on Win32 and error
   if it's already been defined elsewhere.
*/

#  ifndef M_PI
#    define M_PI PI
#  endif

#  ifndef M_LN2
#    define M_LN2            0.69314718055994530942
#  endif

#  ifndef MAXFLOAT
#    define MAXFLOAT ((float)3.40282346638528860e+38)
#  endif

#  ifdef PATH_MAX
#    error "PATH_MAX is a pain"
#  else
#    define PATH_MAX 1024
#  endif

#endif

#ifndef EQSTR
#define EQSTR(x, y)  (strcmp((x),(y)) == 0)
#endif

#ifndef IS_ODD
#define IS_ODD(x)  ((x)%2 != 0)
#endif

#ifndef IS_EVEN
#define IS_EVEN(x)  ((x)%2 == 0)
#endif


/* Watch out for BSD incompatibility */

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef uchar vec3uc[3];
typedef float vec2f[2];


typedef uchar byte;

/* Stop using this? */
/* typedef int Bool; */

#endif
