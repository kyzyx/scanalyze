/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
/*  accpersp.c
 */

#ifdef __cplusplus
this is weird; but MSVC likes it; and wont link from the IDE without it;
#endif

#ifdef WIN32
#	include "winGLdecs.h"
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>

#define PI_ 3.14159265358979323846

/*	accFrustum()
 *  The first 6 arguments are identical to the glFrustum() call.
 *  
 *  pixdx and pixdy are anti-alias jitter in pixels. 
 *  Set both equal to 0.0 for no anti-alias jitter.
 *  eyedx and eyedy are depth-of field jitter in pixels. 
 *  Set both equal to 0.0 for no depth of field effects.
 *
 *  focus is distance from eye to plane in focus. 
 *  focus must be greater than, but not equal to 0.0.
 *
 *  Note that accFrustum() calls glTranslatef().  You will 
 *  probably want to insure that your ModelView matrix has been 
 *  initialized to identity before calling accFrustum().
 */

void accFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
    GLdouble znear, GLdouble zfar, GLdouble pixdx, GLdouble pixdy, 
    GLdouble eyedx, GLdouble eyedy, GLdouble focus)
{
    GLdouble xwsize, ywsize; 
    GLdouble dx, dy;
    GLint viewport[4];

    glGetIntegerv (GL_VIEWPORT, viewport);
	
    xwsize = right - left;
    ywsize = top - bottom;
	
    dx = -(pixdx*xwsize/(GLdouble) viewport[2] + eyedx*znear/focus);
    dy = -(pixdy*ywsize/(GLdouble) viewport[3] + eyedy*znear/focus);
	
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum (left + dx, right + dx, bottom + dy, top + dy, znear, zfar);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef (-eyedx, -eyedy, 0.0);
}

/*  accPerspective()
 * 
 *  The first 4 arguments are identical to the gluPerspective() call.
 *  pixdx and pixdy are anti-alias jitter in pixels. 
 *  Set both equal to 0.0 for no anti-alias jitter.
 *  eyedx and eyedy are depth-of field jitter in pixels. 
 *  Set both equal to 0.0 for no depth of field effects.
 *
 *  focus is distance from eye to plane in focus. 
 *  focus must be greater than, but not equal to 0.0.
 *
 *  Note that accPerspective() calls accFrustum().
 */
void accPerspective(GLdouble fovy, GLdouble aspect, 
    GLdouble znear, GLdouble zfar, GLdouble pixdx, GLdouble pixdy, 
    GLdouble eyedx, GLdouble eyedy, GLdouble focus)
{
    GLdouble fov2,left,right,bottom,top;

    fov2 = ((fovy*PI_) / 180.0) / 2.0;

    top = znear / (cos(fov2) / sin(fov2));
    bottom = -top;

    right = top * aspect;
    left = -right;

    accFrustum (left, right, bottom, top, znear, zfar,
	pixdx, pixdy, eyedx, eyedy, focus);
}

 
