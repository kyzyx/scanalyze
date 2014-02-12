//############################################################
//
// VolCarve.h
//
// Matt Ginzton, Kari Pulli
// Mon Jul 13 11:53:22 PDT 1998
// Perform a volumetric space carving on RigidScans.
// Use octrees.
// For occlusion testing, treat octree nodes as spheres.
//############################################################

#ifndef _VOLCARVE_H_
#define _VOLCARVE_H_

#include <stack>
#include "RigidScan.h"
#include <math.h>


class CubeTreeBase {
 protected:
  OccSt           type;

  int             parIdx;  // which child we are of parent
  Pnt3            mCtr;    // center of node
  float           mSize;   // length of side (diameter, not radius)
  class CubeTree* parent;  // parent node
  friend class CubeTree;
  friend class CubeTreeLeaf;

  enum { defaultLeafDepth = 3 };
  static int   leafDepth;  // how many levels in a leaf block
  static float leafSize;   // size of the leaves in the tree
  static int   nVoxels;    // how many voxels in a leaf block

  // get list for boundary triangles
  virtual void extract_faces (vector<int> &quad_inds) = 0;
  virtual void keep_walls (bool x, bool y, bool z) = 0;

 public:
  CubeTreeBase(Pnt3 c, float s, CubeTree* parent, int iParIdx);
  virtual ~CubeTreeBase();

  int inside() const   { return type == INSIDE; }
  int outside() const  { return type == OUTSIDE; }
  int boundary() const { return type == BOUNDARY ||
			        type == SILHOUETTE; }
  int go_on() const    { return type == BOUNDARY ||
			        type == SILHOUETTE ||
                                type == INDETERMINATE; }

  // spatial info
  inline const Pnt3& ctr (void) const
    { return mCtr; }
  inline const float& size (void) const
    { return mSize; }

  // set the internal depth of leaf clusters
  void set_leaf_depth (int ld);

 protected:
  // find the cube (at the same level)
  // in the direction of face, give its status
  CubeTreeBase*         find_neighbor(int face);
  bool                  is_neighbor_cell (int face, int& iChild);
  virtual CubeTreeBase* recursive_find(int face, stack<int>& path,
				       bool bReachedTop) = 0;
};


class CubeTree: public CubeTreeBase {
  // data for pointer-referenced octree node
 private:
  CubeTreeBase * child[8];  // child nodes
  friend class CubeTreeBase;

  // helpers for pointer-referenced octree node
 private:

  void delete_children(void);

  // helper for find_neighbor
  CubeTreeBase* recursive_find(int face, stack<int>& path,
			       bool bReachedTop);
  void release_space(int i);
protected:
  // get list for boundary triangles
  virtual void extract_faces(vector<int> &tri_inds);
  virtual void keep_walls (bool x, bool y, bool z);

  void  carve_help(vector<RigidScan*> &views, int levels,
		   vector<int> &tri_inds,
		   float perc_min, float perc_max);
public:
  CubeTree(Pnt3 c, float s, CubeTree* parent = NULL,
	   int iParIdx = -1);
  ~CubeTree(void);

  void  carve(vector<RigidScan*> &views, int levels,
	      vector<Pnt3> &coords, vector<int> &tri_inds);
};


struct CubeTreeLeafData {

  float distance;
  float confidence;

  // edge intersection vertices (indices to vertex_list)
  int eixi, eiyi, eizi;

  CubeTreeLeafData(void) : eixi(-1), eiyi(-1), eizi(-1), distance(-1e33) {}
};


class CubeTreeLeaf: public CubeTreeBase {
 public:
  CubeTreeLeaf(const Pnt3 &c, float s,
	       CubeTree* parent, int iParIdx,
	       vector<RigidScan*>& views);
  virtual ~CubeTreeLeaf();

 protected:
  // get list for boundary triangles
  virtual void extract_faces (vector<int> &tri_inds);
  virtual void keep_walls (bool x, bool y, bool z);

  OccSt          neighbor_status  (int face, int child);
  bool           is_neighbor_cell (int face, int& child);
  CubeTreeBase*  recursive_find   (int face, stack<int>& path,
				   bool bReachedTop);
  CubeTreeLeaf*  find_neighbor    (int face);

  friend class CubeTreeBase;
  friend class CubeTree;

 private:

  CubeTreeLeafData* data;

  // stuff for accessing children...
  // could also be computed each time
  int   mask;
  int   nLeavesPerSide;
};


#endif
