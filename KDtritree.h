//############################################################
// KDtritree.h
// Kari Pulli
// 11/19/98
// A KD tree for spheres, the leaf spheres contain triangles.
// When creating, use KD-tree partitioning, when returning
// from recursion, find bounding spheres.
//############################################################

#ifndef _KDTRITREE_H_
#define _KDTRITREE_H_
#include "Pnt3.h"
#include <vector>

struct KDsphere {
  int   ind;
  Pnt3  ctr;
  float radius;
};

class KDtritree {
private:

  Pnt3  ctr;
  float radius;
  // if leaf, use ind to tell where the indices for
  // the triangle are, else use to tell which dimension
  // was used in splitting
  int   ind;
  float split; // on which coordinate did the split happen

  KDtritree  *child[2];

  void collapse_spheres(void);

  void _search(const Pnt3 *pts, const int *inds,
	       const Pnt3 &p, Pnt3 &cp, float &d2) const;
// STL Update
  void _search(const vector<Pnt3>::const_iterator pts, const vector<int>::const_iterator inds,
	       const Pnt3 &p, Pnt3 &cp, float &d2) const;

public:

  KDtritree(const Pnt3 *pts, const int *triinds,
	    KDsphere *spheres, int n, int first = 1);
// STL Update
  KDtritree(const vector<Pnt3>::const_iterator pts, const vector<int>::const_iterator triinds,
	    KDsphere *spheres, int n, int first = 1);

  ~KDtritree();

  // just find the closest point
  bool search(const Pnt3 *pts, const int *inds, const Pnt3 &p,
	      Pnt3 &cp, float &d) const
    {
      float d2 = d*d;
      _search(pts, inds, p, cp, d2);
      if (d*d!=d2) {
	d = sqrtf(d2);
	return true;
      } else {
	return false;
      }
    }
// STL Update
  bool search(const vector<Pnt3>::const_iterator pts, const vector<int>::const_iterator inds, const Pnt3 &p,
	      Pnt3 &cp, float &d) const
    {
      float d2 = d*d;
      _search(pts, inds, p, cp, d2);
      if (d*d!=d2) {
	d = sqrtf(d2);
	return true;
      } else {
	return false;
      }
    }
};


KDtritree *
create_KDtritree(const Pnt3 *pts, const int *inds, int n);
// STL Update
KDtritree *
create_KDtritree(const vector<Pnt3>::const_iterator pts, const vector<int>::const_iterator inds, int n);

#endif
