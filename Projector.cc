// Projector.cc      Classes for easily simulating OpenGL
//                   projection/unprojection
// created 5/5/99    magi@cs.stanford.edu


#include "Projector.h"
#include "Trackball.h"
#include "TbObj.h"
#include "plvGlobals.h"


// these variables are declared and initialized in plvAnalyze.cc
extern unsigned int g_zbufferMaxUI;
extern float g_zbufferMaxF;


Projector::Projector (Trackball* tb, TbObj* tbobj)
{
  // Some naughty clients of this code try to initialize a Projector
  // before there's an OpenGL context (e.g. during static variable
  // initialization).  Some OpenGL implementations don't appreciate
  // our trying to run OpenGL commands at this time, so try to prevent this...
  if (!g_glVersion)
  	return;

  GLdouble model[16], proj[16];
  glGetDoublev(GL_PROJECTION_MATRIX, proj);    // current GL state

  int vp[4];
  glGetIntegerv(GL_VIEWPORT, vp);
  width = vp[2]; height = vp[3];

  int matrixMode;
  glGetIntegerv (GL_MATRIX_MODE, &matrixMode);

  glMatrixMode (GL_MODELVIEW);
  glPushMatrix();
  if (tb || tbobj) { // if both NULL, use current state
    // things that want true identity modelview have to set it that way
    // themselves, then call with both tb and tbobj NULL
    glLoadIdentity ();
    if (tb)
      tb->applyXform();
    if (tbobj)
      tbobj->gl_xform();
  }
  glGetDoublev(GL_MODELVIEW_MATRIX, model);    // applied from trackball
  glPopMatrix();

  glMatrixMode ((GLenum) matrixMode);

  xfForward = Xform<float>(proj) * Xform<float>(model);
}


Pnt3
Projector::operator() (const Pnt3& world) const
{
  Pnt3 screen = world;
  xfForward (screen);

  float winX = width * (screen[0] + 1) / 2;
  float winY = height * (screen[1] + 1) / 2;
  float winZ = (screen[2] + 1) / 2;

  return Pnt3 (winX, winY, winZ);
}


void
Projector::xformBy (const Xform<float>& xfRel)
{
  // BUGBUG suspect code!  needs testing.
  xfForward = xfForward * xfRel;
}


//////////////////////////////////////////////////////////////////////
//
// Unprojector
//
//////////////////////////////////////////////////////////////////////


Unprojector::Unprojector (Trackball* tb, TbObj* tbobj)
  : Projector (tb, tbobj)
{
  xfBack = xfForward;
  xfBack.invert();
}


Pnt3
Unprojector::operator() (int x, int y, unsigned int z) const
{
  float tx = (2. * x / width) - 1;
  float ty = (2. * y / height) - 1;
  // The line below used to just say (2. * z / (float) g_zbufferMaxUI),
  // but MSVC++ v.6 does the wrong thing without the extra parens! -- DRK
  float tz = (2. * z / ((float)g_zbufferMaxUI)) - 1;
  return xfBack.unproject (tx, ty, tz);
}


Pnt3
Unprojector::operator() (int x, int y, float z) const
{
  float tx = (2. * x / width) - 1;
  float ty = (2. * y / height) - 1;
  float tz = (2. * z / g_zbufferMaxF) - 1;
  return xfBack.unproject (tx, ty, tz);
}


Pnt3
Unprojector::forward (const Pnt3& world) const
{
  return Projector::operator() (world);
}


void
Unprojector::xformBy (const Xform<float>& xfRel)
{
  Projector::xformBy (xfRel);

  // BUGBUG suspect code!  needs testing.
  Xform<float> xfRelInv = xfRel; xfRelInv.invert();
  xfBack = xfRelInv * xfBack;
}
