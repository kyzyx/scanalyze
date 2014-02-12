//###############################################################
// VertexFilter.h
// Matt Ginzton, Kari Pulli
// Thu Jul  2 18:29:04 PDT 1998
//
// Interface for creating a filtered copy of mesh geometry data
//###############################################################


#ifndef _VERTEXFILTER_H_
#define _VERTEXFILTER_H_

class Pnt3;
class TbObj;
#include <vector.h>
#include "Bbox.h"
#include "Xform.h"
#include "Projector.h"


// in general, use this to create a specific type of VertexFilter,
// but only access things at the VertexFilter level:
class VertexFilter* filterFromSelection (TbObj* ms,
					 const class Selection& sel);



struct ScreenPnt {
  ScreenPnt (int x, int y) { this->x = x; this->y = y; };
  ScreenPnt (const ScreenPnt& p) { this->x = p.x; this->y = p.y; };
  ScreenPnt () { x = y = 0; };
  int x; int y;
};


class VertexFilter {
 public:
  VertexFilter();
  virtual ~VertexFilter();

  virtual bool accept (const Pnt3& pt) const = 0;
  virtual bool accept (const Bbox &) const = 0;
  virtual void invert() = 0;

  virtual VertexFilter* transformedClone (const Xform<float>& xf) const
    { return NULL; };
};


class ScreenBox: public VertexFilter {
 protected:
  int    xmin, xmax, ymin, ymax;
  Projector projector;

  bool   inverted;

 public:
  ScreenBox (TbObj *ms, int x1, int x2, int y1, int y2);
  ScreenBox (const ScreenBox* source);

  bool accept (const Pnt3& crd) const;
  bool accept (const Bbox& box) const;
  bool acceptFully (const Bbox& box) const;

  void invert();
  virtual VertexFilter* transformedClone (const Xform<float>& xf) const;

  const Projector& getProjector (void) const { return projector; }
};


class ScreenPolyLine: public ScreenBox {
 private:
  unsigned char* pixels;
  int width, height;

 public:
  ScreenPolyLine (TbObj* ms, const vector<ScreenPnt>& pts);
  ScreenPolyLine (const ScreenPolyLine* source);
  ~ScreenPolyLine();

  bool accept (const Pnt3& crd) const;
  bool accept (const Bbox& box) const { return ScreenBox::accept (box); };

  virtual VertexFilter* transformedClone (const Xform<float>& xf) const;
};


#endif
