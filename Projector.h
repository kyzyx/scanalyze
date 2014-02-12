// Projector.h       Classes for easily simulating OpenGL
//                   projection/unprojection
// created 5/5/99    magi@cs.stanford.edu


#ifndef _PROJECTOR_H_
#define _PROJECTOR_H_


#include "Xform.h"
class Trackball;
class TbObj;


// class for projecting world coordinates into screen coordinates
class Projector
{
 public:
  Projector (Trackball* tb = NULL, TbObj* tbObj = NULL);

  Pnt3 operator() (const Pnt3& world) const;

  void xformBy (const Xform<float>& xfRel);

 protected:
  int width, height;
  Xform<float> xfForward;
};


// class for unprojecting screen coordinates into world coordinates
class Unprojector: public Projector
{
 public:
  Unprojector (Trackball* tb = NULL, TbObj* tbObj = NULL);

  Pnt3 operator() (int x, int y, unsigned int z) const;
  Pnt3 operator() (int x, int y, float z) const;
  Pnt3 forward (const Pnt3& world) const;

  void xformBy (const Xform<float>& xfRel);

 private:
  Xform<float> xfBack;
};


#endif // _PROJECTOR_H_
