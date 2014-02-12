//############################################################
//
// TriOctree.h
//
// Kari Pulli
// Mon Nov 23 16:36:51 CET 1998
//
// Data structure that creates an octree containing all the
// triangles in a mesh.
// Can also used to quickly evaluate approximate distances
// to the surface.
//
//############################################################

#ifndef _TRIOCTREE_H_
#define _TRIOCTREE_H_

#include "Pnt3.h"
#include <vector.h>
#include <stack.h>

class TriOctree {

private:

  Pnt3        mCtr;
  float       mRadius;

  int         parIdx;
  TriOctree  *parent;
  TriOctree  *child[8];

  const Pnt3 *pts;
  vector<int> tind; // triangle indices, groups of three

  TriOctree*  recursive_find(int face, stack<int>& path,
			     bool bReachedTop);
  bool        is_neighbor_cell(int face, int &iChild);
  TriOctree*  find_neighbor (int face);

public:

  TriOctree(const Pnt3 &c, float r, const Pnt3 *p,
	    const vector<int> &tris, float minRadius,
	    int pidx = 0, TriOctree *parent = NULL);
  ~TriOctree(void) {};

  bool search(const Pnt3 &p, Pnt3 &cp, float &d);

  // creates a mesh that shows the structure of the octree
  void visualize(vector<Pnt3> &p, vector<int> &ind);
};

#endif
