#ifndef _ACCPERSP_
#define _ACCPERSP_

#ifdef WIN32
#	include "winGLdecs.h"
#endif
#include <GL/gl.h>

#ifdef __cplusplus
extern "C" {
#endif


void accFrustum(GLdouble left, GLdouble right,
		GLdouble bottom, GLdouble top,
		GLdouble znear, GLdouble zfar,
		GLdouble pixdx, GLdouble pixdy,
		GLdouble eyedx, GLdouble eyedy,
		GLdouble focus);

void accPerspective(GLdouble fovy, GLdouble aspect,
		    GLdouble znear, GLdouble zfar,
		    GLdouble pixdx, GLdouble pixdy,
		    GLdouble eyedx, GLdouble eyedy,
		    GLdouble focus);

#ifdef __cplusplus
}
#endif

#endif

