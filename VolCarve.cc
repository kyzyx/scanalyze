//############################################################
//
// VolCarve.cc
//
// Matt Ginzton, Kari Pulli
// Mon Jul 13 11:53:22 PDT 1998
//
// Perform a volumetric space carving on RigidScans.
// Use octrees.
// For occlusion testing, treat octree nodes as spheres.
//
//############################################################

#include <map>
#include <stack>
#include <deque>
#ifdef WIN32
#	include <float.h>
#	define isnanf _isnan
#endif
#ifdef sgi
#	include <ieeefp.h>
#endif
#include "VolCarve.h"
#include "MCEdgeTable.h"


#define VOLCARVE_MAIN 0

// face indices:
// 0, 2, 4 = dimension (x, y, z); +0 negative, +1 positive
// so 0 is left, 1 is right
//    2 is down, 3 is up
//    4 is in,   5 is out
static const int face_negX = 0;
//static const int face_posX = 1;
static const int face_negY = 2;
//static const int face_posY = 3;
static const int face_negZ = 4;
//static const int face_posZ = 5;



vector<Pnt3> vertex_list;

/*
#ifdef WIN32
  typedef __int64 CUBEKEY;
#else
  typedef long long CUBEKEY;
#endif
typedef map<CUBEKEY, int> VERTEXMAP;
VERTEXMAP g_vertexMap;

Pnt3 fblo, fbhi; // full box lower and upper corners
// use the two upper two bits to encode which edge of a cube corner
// the vertex belongs to
static const int encode_bits  = ((8 * sizeof(CUBEKEY))-2) / 3;
static const int encode_shift = (1 << encode_bits);
static float encode_scale;


static inline CUBEKEY
encode(const Pnt3 &p)
{
  CUBEKEY x = (p[0]-fblo[0]) * encode_scale;
  CUBEKEY y = (p[1]-fblo[1]) * encode_scale;
  CUBEKEY z = (p[2]-fblo[2]) * encode_scale;
  return (((x << encode_bits) + y) << encode_bits) + z;
}
*/


class CTL_memhandler {
private:
  int blockSize;
  stack<CubeTreeLeafData*> free;
  int count;

public:

  CTL_memhandler(void) : blockSize(0) {}
  ~CTL_memhandler(void)
    {
      //SHOW(free.size());
      //SHOW(count);
      while (!free.empty()) {
	delete[] free.top();
	free.pop();
      }
    }

  void set_blocksize(int bs)
    {
      if (bs != blockSize) {
	while (!free.empty()) {
	  delete[] free.top();
	  free.pop();
	}
	blockSize = bs;
	count     = 0;
      }
    }

  CubeTreeLeafData *operator()(void)
    {
      CubeTreeLeafData *ret;
      if (!free.empty()) {
	ret = free.top();
	free.pop();
	return ret;
      } else {
	count++;
	return new CubeTreeLeafData[blockSize];
      }
    }

  void store(CubeTreeLeafData *p)
    {
      free.push(p);
    }
};

CTL_memhandler mem_handler;


// first, implementation of methods used by all CubeTrees (CubeTreeBase),
// followed by specifics for octree nodes referenced by pointers
// (CubeTree, which doesn't need to have a complete set of children)
// and octree nodes that store all their children in a flat array
// (CubeTreeLeaf, which always has a complete set of children).


int   CubeTreeBase::leafDepth = CubeTree::defaultLeafDepth;
float CubeTreeBase::leafSize  = 0.0;
int   CubeTreeBase::nVoxels   = 0;

CubeTreeBase::CubeTreeBase (Pnt3 c, float s, CubeTree* _par, int _iParIdx)
  : mCtr(c), mSize(s), parent(_par), parIdx(_iParIdx), type(INDETERMINATE)
{ }


CubeTreeBase::~CubeTreeBase()
{ }

void
CubeTreeBase::set_leaf_depth (int ld)
{
  if (ld >= 0)  leafDepth = ld;
  else          leafDepth = defaultLeafDepth;
}


CubeTreeBase*
CubeTreeBase::find_neighbor (int face)
{
  if (!parent)
    return NULL;

  int iChild;
  bool sibling = is_neighbor_cell(face, iChild);
  if (sibling) {
    CubeTreeBase* theSibling = parent->child[iChild];
    if (!theSibling) return NULL;
    return theSibling;
  }

  // not a sibling: push child and go up
  stack<int> path;
  path.push (iChild);
  return parent->recursive_find (face, path, false);
}




bool
CubeTreeBase::is_neighbor_cell (int face, int& iChild)
{
  // given a direction, return the child index of the neighbor
  // that lies in that direction.  If that neighbor is a sibling
  // (iChild is valid for the same parent as us), return true.
  // Otherwise, return false.

  int axis = face / 2;
  int sign = face % 2;

  // neighbor will be in same position as we are, except
  // the bit representing the axis we're moving along will be
  // flipped
  iChild = parIdx ^ (1 << axis);

  // if we have the bit for that axis set and the direction of
  // movement is negative, it's a sibling, but if the direction
  // of movement is positive, it's not a sibling.
  // Vice-versa if our child index does
  // not have the bit for the movement axis set.
  int haveSign = (0 < (parIdx & (1 << axis)));
  return (sign != haveSign);
}


//
// Nonmember function to determine the type of a node with respect to
// the views, following the precedence order:
//   OUTSIDE
//   BOUNDARY
//   SILHOUETTE
//   INSIDE
//   NOT_IN_FRUSTUM
//   INDETERMINATE
//
OccSt
view_query_status (vector<RigidScan*> &views,
		   vector<RigidScan*> &remaining_views,
		   const Pnt3 &ctr, float size,
		   OccSt type = INDETERMINATE)
{
  vector<RigidScan*>::iterator vit;
  for (vit = views.begin(); vit != views.end(); vit++) {
    OccSt t = (*vit)->carve_cube(ctr, size);
    // OccSt types listed in descending precedence order
    if (int(type) > int(t))
      type = t;

    if (t != NOT_IN_FRUSTUM) {
      remaining_views.push_back(*vit);
    }

    if (t == OUTSIDE) {
      // can't get any better than this
      return t;
    }
  }

  return type;
}


float
view_query_distance (vector<RigidScan*> &views, const Pnt3& ctr,
		     float& distance)
{
  float totalConf = 0;
  OccSt occ;
  Pnt3 closest;
  distance = 0;

  //
  // HOW SHOULD THE DISTANCES REALLY BE COMBINED????
  //

  float thr  = 15.0;
  float ramp = 30.0;

  vector<RigidScan*>::iterator vit;
  for (vit = views.begin(); vit != views.end(); vit++) {
    float conf = (*vit)->closest_along_line_of_sight (ctr, closest, occ);
    //float conf = (*vit)->closest_point_on_mesh(ctr, closest, occ);

    float thisDist = dist(ctr, closest);

    if (occ == INSIDE) {
      if (thisDist > thr) {
	float d = thisDist - thr;
	if (d > ramp) conf = 0;
	else          conf *= (ramp - d) / ramp;
      }
    }
    totalConf += conf;
    if (occ == INSIDE)
      thisDist *= -1;
    distance += conf * thisDist;
  }

  if (totalConf) distance /= totalConf;
  else           distance = -1000;

  return totalConf;
}




///////////////////////////////////////////////////////////////////////////
//
// Pointerized octree nodes: CubeTree
//
///////////////////////////////////////////////////////////////////////////

void
CubeTree::delete_children (void)
{
  for (int i = 0; i < 8; i++) {
    delete child[i];
    child[i] = NULL;
  }
}


CubeTreeBase*
CubeTree::recursive_find(int face, stack<int>& path,
			 bool bReachedTop)
{
  if (path.empty()) {
    /*
    CubeTreeLeaf* leaf = dynamic_cast<CubeTreeLeaf*> (this);
    if (!leaf) {
      assert (0);
      return this;
    }
    return leaf;
    */
    return this;
  }

  int iChild;
  if (bReachedTop) {
    // then we want to descend
    iChild = path.top(); path.pop();
    CubeTreeBase* theChild = child[iChild];
    if (!theChild)
      return this; // child does not exist
    return theChild->recursive_find (face, path, true);
  } else {
    // we still need to traverse horizontally...
    if (!parent)
      return NULL;
    bool sibling = is_neighbor_cell (face, iChild);
    if (sibling) {
      // if we have a sibling, we can traverse
      // (and we've reached the top!)
      CubeTreeBase* sibling = parent->child[iChild];
      if (!sibling) return NULL;
      return sibling->recursive_find (face, path, true);
    } else {
      // otherwise we have to go farther up.
      path.push (iChild);
      return parent->recursive_find (face, path, false);
    }
  }
}

// find the neighbors of child i into negative directions
// in x, y, and z, and tell them that data along that
// face is not needed any more
void
CubeTree::release_space(int i)
{
  if (i&1) {
    child[i-1]->keep_walls(0,1,1);
  } else {
    CubeTreeBase *ctb = find_neighbor(face_negX);
    if (ctb) {
      ctb = (dynamic_cast<CubeTree*>(ctb))->child[i+1];
      if (ctb) ctb->keep_walls(0,1,1);
    }
  }
  if (i&2) {
    child[i-2]->keep_walls(1,0,1);
  } else {
    CubeTreeBase *ctb = find_neighbor(face_negY);
    if (ctb) {
      ctb = (dynamic_cast<CubeTree*>(ctb))->child[i+2];
      if (ctb) ctb->keep_walls(1,0,1);
    }
  }
  if (i&4) {
    child[i-4]->keep_walls(1,1,0);
  } else {
    CubeTreeBase *ctb = find_neighbor(face_negZ);
    if (ctb) {
      ctb = (dynamic_cast<CubeTree*>(ctb))->child[i+4];
      if (ctb) ctb->keep_walls(1,1,0);
    }
  }
}


CubeTree::CubeTree (Pnt3 c, float s, CubeTree* _par, int _i)
  : CubeTreeBase (c, s, _par, _i)
{
  for (int i = 0; i < 8; i++)
    child[i] = NULL;
}


CubeTree::~CubeTree (void)
{
  delete_children();
}


// recursively descend up to given level
// then if the neighboring cube is outside,
// create a face
void
CubeTree::extract_faces(vector<int> &tri_inds)
{
  // treat NOT_IN_FRUSTUM as OUTSIDE for now
  // for INSIDE should perhaps test that it is truly
  // all the way inside
  //if (!boundary())
  if (!go_on())
    return;

  // descend
  for (int i = 0; i < 8; i++) {
    assert(child[i]);
    child[i]->extract_faces(tri_inds);
  }
}


void
CubeTree::keep_walls(bool x, bool y, bool z)
{
  int l=0;
  for (int i=0; i<2; i++) {
    for (int j=0; j<2; j++) {
      for (int k=0; k<2; k++, l++) {
	if (child[l] == NULL) continue;
	if (k&&x || j&&y || i&&z) {
	  child[l]->keep_walls(x&&k, y&&j, z&&i);
	} else {
	  delete child[l];
	  child[l] = NULL;
	}
      }
    }
  }
}


void
CubeTree::carve_help(vector<RigidScan*> &views, int levels,
		     vector<int> &tri_inds,
		     float perc_min, float perc_max)
{
  // When checking status inflate size by the size of a leaf
  // so if the surface goes close to the  CubeTreeLeaf block boundary,
  // its neighbors exist in the tree.
  vector<RigidScan*> remaining_views;
  type = ::view_query_status (views, remaining_views,
			      mCtr, mSize+leafSize, type);

  SHOW(levels);

  //// only boundary nodes can have children
  //if (!boundary()) return;
  if (!go_on()) return;
  if (levels == 0) return;
  --levels;

  float childSize = mSize*.5;
  float stepSize  = mSize*.25;

  bool allChildrenOut = true;
  bool allChildrenIn  = true;

  Pnt3 childCtr;

  float perc_frac = (perc_max-perc_min) / 8.0;

  for (int i=0; i<8; i++) {
    childCtr.set(mCtr[0] + ((i&1) ? stepSize : -stepSize),
		 mCtr[1] + ((i&2) ? stepSize : -stepSize),
		 mCtr[2] + ((i&4) ? stepSize : -stepSize));

    if (levels > leafDepth) {
      CubeTree* ct = new CubeTree (childCtr, childSize, this, i);
      child[i] = ct;
      float _perc_min = perc_min + i*perc_frac;
      ct->carve_help(remaining_views, levels, tri_inds,
		     _perc_min, _perc_min + perc_frac);
      cout << "Carved " << _perc_min + perc_frac << " percent" << endl;
    } else {
      CubeTreeLeaf* ctl = new CubeTreeLeaf(childCtr, childSize,
					   this, i, views);
      child[i] = ctl;
      ctl->extract_faces(tri_inds);
    }

    /*
    // release unneeded parts of the tree
    if (i&1) child[i-1]->keep_walls(0,1,1);
    else {
      CubeTreeBase *ctb = find_neighbor(face_negX);
      if (ctb) {
	CubeTree *ct = dynamic_cast<CubeTree*>(ctb);
	if (ct->child[i+1]) ct->child[i+1]->keep_walls(0,1,1);
      }
    }
    if (i&2) child[i-2]->keep_walls(1,0,1);
    if (i&4) child[i-4]->keep_walls(1,1,0);
    */
    release_space(i);

    OccSt t = child[i]->type;
    if (t != OUTSIDE) allChildrenOut = false;
    if (t != INSIDE)  allChildrenIn  = false;
  }

  if (allChildrenOut) {
    delete_children();
    type = OUTSIDE;
  }
  if (allChildrenIn) {
    delete_children();
    type = INSIDE;
  }
}

//#define VIS_TRIOCTREE

#ifdef VIS_TRIOCTREE
#include "CyraScan.h"
#include "TriOctree.h"
#endif

// first split, then
// extract the boundary between outsides and the rest
void
CubeTree::carve(vector<RigidScan*> &views, int levels,
		vector<Pnt3> &coords, vector<int> &tri_inds)
{
#ifdef VIS_TRIOCTREE
  CyraScan *cs = dynamic_cast<CyraScan*> (views[0]);
  if (cs) cs->TriOctreeMesh(coords, tri_inds);
  return;
#endif

  // Figure out the size of the leaves.
  leafSize = mSize;
  for (int j = 0; j<levels; j++) leafSize *= .5;
  nVoxels  = 1<<3*leafDepth;
  mem_handler.set_blocksize(nVoxels);

  tri_inds.clear();
  vertex_list.clear();
  /*
  fbhi = fblo = mCtr;
  Pnt3 step(mSize*.5,mSize*.5,mSize*.5);
  fbhi += step;
  fblo -= step;
  encode_scale = .99999 * encode_shift / (fbhi[0] - fblo[0]);
  */

  carve_help(views, levels, tri_inds, 0, 100);

  delete_children();

  coords = vertex_list;
  SHOW(vertex_list.size());
  SHOW(tri_inds.size());
  vertex_list.clear();

  for (int i=0; i<coords.size(); i++) {
    if (isnanf(coords[i][0]) ||
	isnanf(coords[i][1]) ||
	isnanf(coords[i][2])) {
      SHOW(i); SHOW(coords[i]);
      break;
    }
  }
}

//#define CARVE_TEST

static int leaf_count = 0;

CubeTreeLeaf::CubeTreeLeaf (const Pnt3 &c, float s,
			    CubeTree* _par, int _i,
			    vector<RigidScan*> &views)
  : CubeTreeBase (c, s, _par, _i), data(NULL)
{
  // When checking status inflate size by the size of a leaf
  // so if the surface goes close to the  CubeTreeLeaf block boundary,
  // it's neighbors exist in the tree.
  vector<RigidScan*> remaining_views;
  type = ::view_query_status (views, remaining_views,
			      c, s+leafSize);
  //type = ::view_query_status (views, c, 1.25*leafSize);

  //if (!boundary())
  if (!go_on())
    return;

  leaf_count++;
  SHOW(leaf_count);

#ifdef CARVE_TEST
  return;
#endif

  nLeavesPerSide = 1 << leafDepth;
  mask = nLeavesPerSide - 1;

  //data = new CubeTreeLeafData[nVoxels];
  data = mem_handler();

  float tmp = .5 * (nLeavesPerSide - 1) * leafSize;
  Pnt3 base(mCtr[0]-tmp, mCtr[1]-tmp, mCtr[2]-tmp);

  Pnt3 lCtr;
  int depth2 = 2*leafDepth;
  int x, y, z;

  // evaluate the function at all the leaf nodes
  // also interpolate the edges here
  int ofsx = 1;
  int ofsy = 1<<leafDepth;
  int ofsz = 1<<depth2;
  // length of the step to go to the other end along this dimension
  int lenx = ofsy-ofsx;
  int leny = ofsz-ofsy;
  int lenz = (1<<leafDepth+depth2)-ofsz;
  CubeTreeLeafData *vp;

  CubeTreeLeaf* nborX = find_neighbor (face_negX);
  if (nborX && !nborX->data) nborX = NULL;
  CubeTreeLeaf* nborY = find_neighbor (face_negY);
  if (nborY && !nborY->data) nborY = NULL;
  CubeTreeLeaf* nborZ = find_neighbor (face_negZ);
  if (nborZ && !nborZ->data) nborZ = NULL;

  int i = 0;
  for (z=0; z<nLeavesPerSide; z++) {
    for (y=0; y<nLeavesPerSide; y++) {
      for (x=0; x<nLeavesPerSide; x++, i++) {

	float &dist = data[i].distance;

	lCtr.set(base[0] + leafSize*x,
		 base[1] + leafSize*y,
		 base[2] + leafSize*z);
	data[i].confidence = view_query_distance(views, lCtr,dist);

	// if zero confidence, assign arbitrarily dist = 1.0
	if (data[i].confidence < 1e-5) dist = 1.0;

	assert(dist > -1e22 && dist < 1e22);
	// interpolate edges
	if (x) {
	  vp = &data[i-ofsx];
	} else {
	  if (nborX)  vp = &nborX->data[i+lenx];
	  else        vp = NULL;
	}
	if (vp && dist * vp->distance <= 0.0) {
	  // signs differ, interpolate
	  vp->eixi = vertex_list.size();
	  float d = leafSize * dist / (dist - vp->distance);
	  assert(!isnanf(d));
	  vertex_list.push_back(Pnt3(lCtr[0]-d, lCtr[1], lCtr[2]));
	}

	if (y) {
	  vp = &data[i-ofsy];
	} else {
	  if (nborY)  vp = &nborY->data[i+leny];
	  else        vp = NULL;
	}
	if (vp && dist * vp->distance <= 0.0) {
	  // signs differ, interpolate
	  vp->eiyi = vertex_list.size();
	  float d = leafSize * dist / (dist - vp->distance);
	  assert(!isnanf(d));
	  vertex_list.push_back(Pnt3(lCtr[0], lCtr[1]-d, lCtr[2]));
	}

	if (z) {
	  vp = &data[i-ofsz];
	} else {
	  if (nborZ)  vp = &nborZ->data[i+lenz];
	  else        vp = NULL;
	}
	if (vp && dist * vp->distance <= 0.0) {
	  // signs differ, interpolate
	  vp->eizi = vertex_list.size();
	  float d = leafSize * dist / (dist - vp->distance);
	  assert(!isnanf(d));
	  vertex_list.push_back(Pnt3(lCtr[0], lCtr[1], lCtr[2]-d));
	}
      }
    }
  }
}


CubeTreeLeaf::~CubeTreeLeaf (void)
{
  //delete[] data;
  if (data) mem_handler.store(data);
}


static void
cube_bdry(const Pnt3 &ctr, float size,
	  vector<Pnt3> &p, vector<int> &i)
{
  float w = .5 * size;
  p.push_back(ctr + Pnt3(-w,-w,-w));
  p.push_back(ctr + Pnt3(-w,-w, w));
  p.push_back(ctr + Pnt3(-w, w,-w));
  p.push_back(ctr + Pnt3(-w, w, w));
  p.push_back(ctr + Pnt3( w,-w,-w));
  p.push_back(ctr + Pnt3( w,-w, w));
  p.push_back(ctr + Pnt3( w, w,-w));
  p.push_back(ctr + Pnt3( w, w, w));

  static int triinds[] = {
    0,1,3,2,0,3, 0,4,5,0,5,1, 2,6,4,2,4,0,
    3,5,7,3,1,5, 2,7,6,2,3,7, 7,4,6,7,5,4
  };

  int   s = vertex_list.size();
  for (int j=0; j<36; j++)
    i.push_back(s+triinds[j]);
}

static EdgeTable TheEdgeTable;

void
CubeTreeLeaf::extract_faces(vector<int> &tri_inds)
{
  //if (!boundary()) return;
  if (!go_on()) return;

#ifdef CARVE_TEST
  cube_bdry(mCtr, mSize, vertex_list, tri_inds);
  return;
#endif

  int depth2 = 2*leafDepth;
  int x, y, z;

  int ofsx = 1;
  int ofsy = 1<<leafDepth;
  int ofsz = 1<<depth2;
  // length of the step to go to the other end along this dimension
  int lenx = ofsy-ofsx;
  int leny = ofsz-ofsy;
  int lenz = nVoxels-ofsz;
  for (int i = 0; i < nVoxels; i++) {
    // set CubeTreeLeafData static center and size
    // nVoxels has depth bits each for x, y, z
    // find center of corresponding voxel i
    x = (i)              & mask;
    y = (i >> leafDepth) & mask;
    z = (i >> depth2)    & mask;

    //if (x&&y&&z) continue;

    // get the eight pointers to vertices
    // create also the index to the edge table
    CubeTreeLeafData *vp[8];
    int edgeTableIndex = 0;
    CubeTreeLeaf *nborX = NULL, *nborY = NULL, *nborZ = NULL;
    CubeTreeLeaf *nborXY = NULL, *nborXZ = NULL, *nborYZ = NULL;

    // these are the default vertices
    // the ones with a negative index to data will be replaced
    int j;
    vp[0] = ((j=i-ofsx-ofsy-ofsz) < 0 ? NULL : &data[j]);
    vp[1] = ((j=i     -ofsy-ofsz) < 0 ? NULL : &data[j]);
    vp[2] = ((j=i          -ofsz) < 0 ? NULL : &data[j]);
    vp[3] = ((j=i-ofsx     -ofsz) < 0 ? NULL : &data[j]);
    vp[4] = ((j=i-ofsx-ofsy     ) < 0 ? NULL : &data[j]);
    vp[5] = ((j=i     -ofsy     ) < 0 ? NULL : &data[j]);
    vp[6] = &data[i];
    vp[7] = ((j=i-ofsx          ) < 0 ? NULL : &data[j]);

    if (x == 0) {
      nborX = find_neighbor (face_negX);
      if (nborX && nborX->data) {
	vp[0] = ((j=i+lenx-ofsy-ofsz) < 0 ? NULL : &nborX->data[j]);
	vp[3] = ((j=i+lenx     -ofsz) < 0 ? NULL : &nborX->data[j]);
	vp[4] = ((j=i+lenx-ofsy     ) < 0 ? NULL : &nborX->data[j]);
	vp[7] = &nborX->data[i+lenx];
      } else {
	continue; // no neighbor, skip
      }
    }

    if (y == 0) {
      nborY = find_neighbor (face_negY);
      if (nborY && nborY->data) {
	vp[0] = ((j=i+leny-ofsx-ofsz) < 0 ? NULL : &nborY->data[j]);
	vp[1] = ((j=i+leny     -ofsz) < 0 ? NULL : &nborY->data[j]);
	vp[4] = &nborY->data[i+leny-ofsx];
	vp[5] = &nborY->data[i+leny];
      } else {
	continue; // no neighbor, skip
      }
    }

    if (z == 0) {
      nborZ = find_neighbor (face_negZ);
      if (nborZ && nborZ->data) {
	vp[0] = &nborZ->data[i+lenz-ofsx-ofsy];
	vp[1] = &nborZ->data[i+lenz     -ofsy];
	vp[2] = &nborZ->data[i+lenz          ];
	vp[3] = &nborZ->data[i+lenz-ofsx     ];
      } else {
	continue; // no neighbor, skip
      }
    }

    if (nborX && nborY) { // then out in both x and y
      nborXY = nborY->find_neighbor (face_negX);
      if (nborXY && nborXY->data) {
	vp[0] = ((j=i+lenx+leny-ofsz) < 0 ? NULL : &nborXY->data[j]);
	vp[4] = &nborXY->data[i+lenx+leny];
      } else {
	continue; // no neighbor, skip
      }
    }

    if (nborX && nborZ) {
      nborXZ = nborX->find_neighbor (face_negZ);
      if (nborXZ && nborXZ->data) {
	vp[0] = &nborXZ->data[i+lenx+lenz-ofsy];
	vp[3] = &nborXZ->data[i+lenx+lenz];
      } else {
	continue; // no neighbor, skip
      }
    }

    if (nborY && nborZ) {
      nborYZ = nborZ->find_neighbor (face_negY);
      if (nborYZ && nborYZ->data) {
	vp[0] = &nborYZ->data[i+leny+lenz-ofsx];
	vp[1] = &nborYZ->data[i+leny+lenz];
      } else {
	continue; // no neighbor, skip
      }
    }

    if (nborXY && nborZ) {
      CubeTreeLeaf* nborXYZ = nborXY->find_neighbor (face_negZ);
      if (nborXYZ && nborXYZ->data) {
	vp[0] = &nborXYZ->data[i+lenx+leny+lenz];
      } else {
	continue; // no neighbor, skip
      }
    }

    for (j=0; j<8; j++)
      assert(vp[j]->distance > -1e22 && vp[j]->distance < 1e22);

    if (vp[0]->distance > 0.0) edgeTableIndex |=   1;
    if (vp[1]->distance > 0.0) edgeTableIndex |=   2;
    if (vp[2]->distance > 0.0) edgeTableIndex |=   4;
    if (vp[3]->distance > 0.0) edgeTableIndex |=   8;
    if (vp[4]->distance > 0.0) edgeTableIndex |=  16;
    if (vp[5]->distance > 0.0) edgeTableIndex |=  32;
    if (vp[6]->distance > 0.0) edgeTableIndex |=  64;
    if (vp[7]->distance > 0.0) edgeTableIndex |= 128;

    // for all the 12 edges, if the table tells that there should
    // be an edge, copy a pointer to the corresponding precalculated vertex
    bool *edge = TheEdgeTable[edgeTableIndex].edge;
    int evtx[12];
    if (edge[0])  evtx[0]  = vp[0]->eixi;
    if (edge[1])  evtx[1]  = vp[1]->eiyi;
    if (edge[2])  evtx[2]  = vp[3]->eixi;
    if (edge[3])  evtx[3]  = vp[0]->eiyi;
    if (edge[4])  evtx[4]  = vp[4]->eixi;
    if (edge[5])  evtx[5]  = vp[5]->eiyi;
    if (edge[6])  evtx[6]  = vp[7]->eixi;
    if (edge[7])  evtx[7]  = vp[4]->eiyi;
    if (edge[8])  evtx[8]  = vp[0]->eizi;
    if (edge[9])  evtx[9]  = vp[1]->eizi;
    if (edge[10]) evtx[10] = vp[3]->eizi;
    if (edge[11]) evtx[11] = vp[2]->eizi;

    int nTrisCreated = TheEdgeTable[edgeTableIndex].Ntriangles;
    Triple *triList  = TheEdgeTable[edgeTableIndex].TriangleList;
    for (j=0; j<nTrisCreated; j++) {
#if 0
      SHOW (evtx[triList[j].A]);
      SHOW (evtx[triList[j].B]);
      SHOW (evtx[triList[j].C]);
#endif
      assert(evtx[triList[j].A]>-1);
      assert(evtx[triList[j].B]>-1);
      assert(evtx[triList[j].C]>-1);
      tri_inds.push_back(evtx[triList[j].A]);
      tri_inds.push_back(evtx[triList[j].B]);
      tri_inds.push_back(evtx[triList[j].C]);
    }
  }
}


void
CubeTreeLeaf::keep_walls(bool , bool , bool )
{
}


OccSt
CubeTreeLeaf::neighbor_status (int face, int child)
{
  // get address, and child status, of neighbor of given child
  bool sibling = is_neighbor_cell (face, child);
  if (sibling)
    return type;

  // not a child of this block; have to go up tree
  CubeTreeBase* ctb = CubeTreeBase::find_neighbor (face);
  CubeTreeLeaf* ctl = dynamic_cast<CubeTreeLeaf*> (ctb);
  if (!ctb)
    return OUTSIDE;

  return ctl->type;
}


CubeTreeBase*
CubeTreeLeaf::recursive_find(int face,
			     stack<int>& path, bool bReachedTop)
{
  return this;
}


CubeTreeLeaf*
CubeTreeLeaf::find_neighbor (int face)
{
  return dynamic_cast<CubeTreeLeaf*> (CubeTreeBase::find_neighbor (face));
}


bool
CubeTreeLeaf::is_neighbor_cell (int face, int& child)
{
  // child is both input and output parameter...
  bool contained = true;

  int axis = face / 2;
  int sign = face % 2;

  // need to extract relevant bits, munge them, and replace them into child:
  // extract
  int shift = axis * leafDepth;
  int coord = (child & (mask << shift)) >> shift;

  // munge
  coord += (sign ? +1 : -1);
  if (coord >= nLeavesPerSide) {
    contained = false;
    coord -= nLeavesPerSide;
  }
  else if (coord < 0) {
    contained = false;
    coord += nLeavesPerSide;
  }

  // replace
  child = (child & ~(mask << shift)) | (coord << shift);

  return contained;
}


