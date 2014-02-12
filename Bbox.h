//############################################################
// Bbox.h
// Kari Pulli
// 3/17/96
// A 3D bounding box for Pnt3
//############################################################

// we use min and max for our own purposes, so make sure the
// stdlib macros are not defined.  Do this outside of _Bbox_h
// test, so every invocation of bbox.h including the last one
// will clear the namespace, in case bbox.h is included before
// stdlib.h.
#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif


#ifndef _Bbox_h
#define _Bbox_h

#include <vector>
#include <iostream>
#include "Pnt3.h"


class Bbox {
private:
  Pnt3 lo,hi;
  Pnt3 c;      // center
  int  cdone;  // has the center been calculated

  void calcCenter(void)
    {
      if (!cdone) {
	cdone=1;
	c = lo; c += hi; c *= .5;
      }
    }

public:
  Bbox(void) { clear(); }
  Bbox(Pnt3 const &_lo, Pnt3 const &_hi)
    : lo(_lo), hi(_hi), cdone(0) {}
  ~Bbox(void) {}

  void clear(void)
    {
      hi.set(-1.e33,-1.e33,-1.e33);
      lo.set( 1.e33, 1.e33, 1.e33);
      cdone = 0;
    }

  void add(float x, float y, float z)
    {
      if (x > hi[0]) hi[0] = x;
      if (x < lo[0]) lo[0] = x;
      if (y > hi[1]) hi[1] = y;
      if (y < lo[1]) lo[1] = y;
      if (z > hi[2]) hi[2] = z;
      if (z < lo[2]) lo[2] = z;
      cdone = 0;
    }
  void add(const double *a)
    { add(a[0], a[1], a[2]); }
  void add(const float *a)
    { add(a[0], a[1], a[2]); }
  void add(const Bbox &box)
    { add(box.lo); add(box.hi); }

  const Pnt3 &max(void) const  { return hi; }
  const Pnt3 &min(void) const  { return lo; }
  Pnt3   center(void)          { calcCenter(); return c; }
  float  diag(void) const      { return dist(hi,lo); }
  float maxDim(void) const
    {
      float tmp, ans = hi[0] - lo[0];
      for (int i=1; i<3; i++)
	if ((tmp = hi[i] - lo[i]) > ans) ans = tmp;
      return ans;
    }
  float minDim(void) const
    {
      float tmp, ans = hi[0] - lo[0];
      for (int i=1; i<3; i++)
	if ((tmp = hi[i] - lo[i]) < ans) ans = tmp;
      return ans;
    }

  float size(void) const  // assume the box is a cube
    { return hi[0]-lo[0]; }

  bool valid (void) const // has anything been added?
    { return size() >= 0; }

  int outside(const Pnt3 &p, float d = 0.0) const
    {
      return (p[0] > hi[0]+d || p[0] < lo[0]-d ||
	      p[1] > hi[1]+d || p[1] < lo[1]-d ||
	      p[2] > hi[2]+d || p[2] < lo[2]-d);
    }

  void makeCube(float scale)
    {
      // around the center, expand all dimensions equal
      // to the largest one, then scale
      calcCenter();
      float   len = maxDim()*.5*scale;
      Pnt3 tmp(len,len,len);
      hi = c+tmp;
      lo = c-tmp;
    }

  Bbox octant(int i)
    {
      // i in [0,7], e.g. 6 == x hi, y hi, z lo
      calcCenter();
      Bbox ret(*this);
      if (i&4) ret.lo[0] = c[0];
      else     ret.hi[0] = c[0];
      if (i&2) ret.lo[1] = c[1];
      else     ret.hi[1] = c[1];
      if (i&1) ret.lo[2] = c[2];
      else     ret.hi[2] = c[2];
      ret.cdone = 0;
      return ret;
    }

  int inOctant(Pnt3 &p)
    {
      calcCenter();
      return 4*(p[0]>=c[0]) + 2*(p[1]>=c[1]) + (p[2]>=c[2]);
    }

  Pnt3 corner(int i) const
    {
      return Pnt3((i&4) ? hi[0] : lo[0],
		  (i&2) ? hi[1] : lo[1],
		  (i&1) ? hi[2] : lo[2]);
    }

  Bbox worldBox(float m[16]) const // transformation matrix, OpenGL order
    {
      Bbox ret;
      Pnt3 p;
      ret.add(p.set(hi[0],hi[1],hi[2]).xform(m));
      ret.add(p.set(hi[0],hi[1],lo[2]).xform(m));
      ret.add(p.set(hi[0],lo[1],hi[2]).xform(m));
      ret.add(p.set(hi[0],lo[1],lo[2]).xform(m));
      ret.add(p.set(lo[0],hi[1],hi[2]).xform(m));
      ret.add(p.set(lo[0],hi[1],lo[2]).xform(m));
      ret.add(p.set(lo[0],lo[1],hi[2]).xform(m));
      ret.add(p.set(lo[0],lo[1],lo[2]).xform(m));
      return ret;
    }

  // return pairs of points in e that define the edges
  // of this box
  void edgeList(vector<Pnt3> &e) const
    {
      e.resize(24);
      e[0].set(lo[0],lo[1],lo[2]);
      e[1].set(hi[0],lo[1],lo[2]);
      e[2].set(lo[0],lo[1],lo[2]);
      e[3].set(lo[0],hi[1],lo[2]);
      e[4].set(lo[0],lo[1],lo[2]);
      e[5].set(lo[0],lo[1],hi[2]);

      e[6].set(hi[0],hi[1],hi[2]);
      e[7].set(hi[0],hi[1],lo[2]);
      e[8].set(hi[0],hi[1],hi[2]);
      e[9].set(hi[0],lo[1],hi[2]);
      e[10].set(hi[0],hi[1],hi[2]);
      e[11].set(lo[0],hi[1],hi[2]);

      e[12].set(hi[0],hi[1],lo[2]);
      e[13].set(lo[0],hi[1],lo[2]);
      e[14].set(hi[0],hi[1],lo[2]);
      e[15].set(hi[0],lo[1],lo[2]);

      e[16].set(hi[0],lo[1],hi[2]);
      e[17].set(lo[0],lo[1],hi[2]);
      e[18].set(hi[0],lo[1],hi[2]);
      e[19].set(hi[0],lo[1],lo[2]);

      e[20].set(lo[0],hi[1],hi[2]);
      e[21].set(lo[0],lo[1],hi[2]);
      e[22].set(lo[0],hi[1],hi[2]);
      e[23].set(lo[0],hi[1],lo[2]);
    }

  float max_dist(const Pnt3 &p)
    {
      float d, md = 0.0;
      d = dist(corner(0), p); if (d > md) md = d;
      d = dist(corner(1), p); if (d > md) md = d;
      d = dist(corner(2), p); if (d > md) md = d;
      d = dist(corner(3), p); if (d > md) md = d;
      d = dist(corner(4), p); if (d > md) md = d;
      d = dist(corner(5), p); if (d > md) md = d;
      d = dist(corner(6), p); if (d > md) md = d;
      d = dist(corner(7), p); if (d > md) md = d;
      return md;
    }

  bool intersect(const Bbox &b)
    {
      // find whether any of the faces separate the boxes
      if (hi[0] < b.lo[0]) return false;
      if (hi[1] < b.lo[1]) return false;
      if (hi[2] < b.lo[2]) return false;
      if (b.hi[0] < lo[0]) return false;
      if (b.hi[1] < lo[1]) return false;
      if (b.hi[2] < lo[2]) return false;
      return true;
    }
};

inline ostream&
operator<<(ostream &out, const Bbox &a)
{
  return out << "Bbox: " << a.min() << " " << a.max();
}

#endif
