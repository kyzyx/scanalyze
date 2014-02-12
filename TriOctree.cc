//############################################################
//
// TriOctree.cc
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

#include "TriOctree.h"


// face indices:
// 0, 2, 4 = dimension (x, y, z); +0 negative, +1 positive
// so 0 is left, 1 is right
//    2 is down, 3 is up
//    4 is in,   5 is out
//static const int face_negX = 0;
//static const int face_posX = 1;
//static const int face_negY = 2;
//static const int face_posY = 3;
//static const int face_negZ = 4;
//static const int face_posZ = 5;


TriOctree*
TriOctree::recursive_find(int face, stack<int>& path,
			  bool bReachedTop)
{
  if (path.empty()) return this;

  int iChild;
  if (bReachedTop) {
    // then we want to descend
    iChild = path.top(); path.pop();
    TriOctree* theChild = child[iChild];
    if (!theChild) return this; // child does not exist
    return theChild->recursive_find (face, path, true);
  } else {
    // we still need to traverse horizontally...
    if (!parent) return NULL;
    if (is_neighbor_cell (face, iChild)) {
      // if we have a sibling, we can traverse
      // (and we've reached the top!)
      TriOctree* sibling = parent->child[iChild];
      if (!sibling) return NULL;
      return sibling->recursive_find (face, path, true);
    } else {
      // otherwise we have to go farther up.
      path.push (iChild);
      return parent->recursive_find (face, path, false);
    }
  }
}


bool
TriOctree::is_neighbor_cell(int face, int &iChild)
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


TriOctree*
TriOctree::find_neighbor (int face)
{
  if (!parent)
    return NULL;

  int iChild;
  bool sibling = is_neighbor_cell(face, iChild);
  if (sibling) {
    TriOctree* theSibling = parent->child[iChild];
    if (!theSibling) return NULL;
    return theSibling;
  }

  // not a sibling: push child and go up
  stack<int> path;
  path.push (iChild);
  return parent->recursive_find (face, path, false);
}


bool
tri_outside_of_cube(const Pnt3 &c, float s,
		    const Pnt3 &t1, const Pnt3 &t2, const Pnt3 &t3);


static int leaftricount = 0;


TriOctree::TriOctree(const Pnt3 &c, float r, const Pnt3 *p,
		     const vector<int> &tris, float minRadius,
		     int pidx, TriOctree *parent)
  : mCtr(c), mRadius(r), parIdx(pidx)
{

  /*
  SHOW(mCtr);
  SHOW(mRadius);
  SHOW(tris.size()/3);
  SHOW(minRadius);
  */

  // check whether we need to subdivide more
  if (mRadius < 2*minRadius || tris.size() <= 18) {
    pts = p;
    tind = tris;
    leaftricount += tris.size();
    SHOW(leaftricount);
    child[0] = child[1] = child[2] = child[3] = NULL;
    child[4] = child[5] = child[6] = child[7] = NULL;
    return;
  }

  pts = NULL;

  // if so, try for triangle and each child whether they intersect
  float halfR = mRadius*.5;
  Pnt3 childCtr;

  vector<int> xp, xm; // x plus, x minus
  vector<int> yp, ym; // y plus, y minus
  vector<int> zp, zm; // z plus, z minus
  vector<int> uk;     // unknown (triangles going across splits)
  xp.reserve(tris.size());
  xm.reserve(tris.size());
  uk.reserve(tris.size());

  // split about x axis
  vector<int>::const_iterator it = tris.begin();
  int v1,v2,v3, cnt;
  while (it != tris.end()) {
    v1 = *it++; v2 = *it++; v3 = *it++;
    cnt = (p[v1][0] >= mCtr[0]) + (p[v2][0] >= mCtr[0]) +
      (p[v3][0] >= mCtr[0]);
    if (cnt == 3) {
      xp.push_back(v1); xp.push_back(v2); xp.push_back(v3);
    } else if (cnt == 0) {
      xm.push_back(v1); xm.push_back(v2); xm.push_back(v3);
    } else {
      uk.push_back(v1); uk.push_back(v2); uk.push_back(v3);
    }
  }

  // split about y axis
  vector<int> &xv = xm;
  vector<int> &yv = ym;
  for (int i=0; i<2; i++) {
    if (i) xv = xp;
    else   xv = xm;
    yp.clear(); yp.reserve(xv.size());
    ym.clear(); ym.reserve(xv.size());

    it = xv.begin();
    while (it != xv.end()) {
      v1 = *it++; v2 = *it++; v3 = *it++;
      cnt = (p[v1][1] >= mCtr[1]) + (p[v2][1] >= mCtr[1]) +
	(p[v3][1] >= mCtr[1]);
      if (cnt == 3) {
	yp.push_back(v1); yp.push_back(v2); yp.push_back(v3);
      } else if (cnt == 0) {
	ym.push_back(v1); ym.push_back(v2); ym.push_back(v3);
      } else {
	uk.push_back(v1); uk.push_back(v2); uk.push_back(v3);
      }
    }
    for (int j=0; j<2; j++) {
      if (j) yv = yp;
      else   yv = ym;
      zp.clear(); zp.reserve(yv.size());
      zm.clear(); zm.reserve(yv.size());

      it = yv.begin();
      while (it != yv.end()) {
	v1 = *it++; v2 = *it++; v3 = *it++;
	cnt = (p[v1][2] >= mCtr[2]) + (p[v2][2] >= mCtr[2]) +
	  (p[v3][2] >= mCtr[2]);
	if (cnt == 3) {
	  zp.push_back(v1); zp.push_back(v2); zp.push_back(v3);
	} else if (cnt == 0) {
	  zm.push_back(v1); zm.push_back(v2); zm.push_back(v3);
	} else {
	  uk.push_back(v1); uk.push_back(v2); uk.push_back(v3);
	}
      }

      // check the unknowns
      childCtr.set(mCtr[0] + (i ? halfR : -halfR),
		   mCtr[1] + (j ? halfR : -halfR),
		   mCtr[2] - halfR);
      it = uk.begin();
      while (it != uk.end()) {
	v1 = *it++; v2 = *it++; v3 = *it++;
	if (!tri_outside_of_cube(childCtr, mRadius, p[v1],
				 p[v2], p[v3])) {
	  zm.push_back(v1); zm.push_back(v2); zm.push_back(v3);
	}
      }

      // create the children, if needed
      int k = i+2*j;
      if (zm.size()) {
	child[k] = new TriOctree(childCtr, halfR, p, zm,
				 minRadius, k, this);
      } else {
	child[k] = NULL;
      }

      // check the unknowns
      childCtr.set(mCtr[0] + (i ? halfR : -halfR),
		   mCtr[1] + (j ? halfR : -halfR),
		   mCtr[2] + halfR);
      it = uk.begin();
      while (it != uk.end()) {
	v1 = *it++; v2 = *it++; v3 = *it++;
	if (!tri_outside_of_cube(childCtr, mRadius, p[v1],
				 p[v2], p[v3])) {
	  zp.push_back(v1); zp.push_back(v2); zp.push_back(v3);
	}
      }

      k += 4;
      if (zp.size()) {
	child[k] = new TriOctree(childCtr, halfR, p, zp,
				 minRadius, k, this);
      } else {
	child[k] = NULL;
      }
    }
  }
  /*
  vector<int> tmptris;
  tmptris.reserve(tris.size());
  for (int i=0; i<8; i++) {
    childCtr.set(mCtr[0] + ((i&1) ? halfR : -halfR),
		 mCtr[1] + ((i&2) ? halfR : -halfR),
		 mCtr[2] + ((i&4) ? halfR : -halfR));
    tmptris.clear();
    for (int j=0; j<tris.size(); j+=3) {
      if (!tri_outside_of_cube(childCtr, mRadius, p[tris[j]],
			       p[tris[j+1]], p[tris[j+2]])) {
	tmptris.push_back(tris[j]);
	tmptris.push_back(tris[j+1]);
	tmptris.push_back(tris[j+2]);
      }
    }
    if (tmptris.size()) {
      child[i] = new TriOctree(childCtr, halfR, p, tmptris,
			       minRadius, i, this);
    } else {
      child[i] = NULL;
    }
  }
*/
}


// the return value tells whether the closest distance (squared)
// has been found for sure
bool
TriOctree::search(const Pnt3 &p, Pnt3 &cp, float &d2)
{
  // is this a leaf node?
  if (pts) {
    // look for a new closest distance
    vector<int>::const_iterator   it  = tind.begin();
    while (it != tind.end()) {
      closer_on_tri(p, cp, pts[*(it++)], pts[*(it++)],
		    pts[*(it++)], d2);
    }
    return ball_within_bounds(p, sqrtf(d2), mCtr, mRadius);
  }

  // which child contains p?
  int iChild = 4*(mCtr[2]<p[2]) + 2*(mCtr[1]<p[1]) + (mCtr[0]<p[0]);

  // check that child first
  if (child[iChild] && child[iChild]->search(p, cp, d2)) return true;

  // now see if the other children need to be checked
  for (int i=0; i<8; i++) {
    if (i==iChild) continue;
    //if (child[i] && child[i]->bounds_overlap_ball(p,d)) {
    if (child[i] && bounds_overlap_ball(p,sqrtf(d2),child[i]->mCtr,
					child[i]->mRadius)) {
      if (child[i]->search(p, cp, d2)) return true;
    }
  }
  return ball_within_bounds(p, sqrtf(d2), mCtr, mRadius);
}


static void
cube_bdry(const Pnt3 &ctr, float r,
	  vector<Pnt3> &p, vector<int> &i)
{
  int s = p.size();
  p.push_back(ctr + Pnt3(-r,-r,-r));
  p.push_back(ctr + Pnt3( r,-r,-r));
  p.push_back(ctr + Pnt3(-r, r,-r));
  p.push_back(ctr + Pnt3( r, r,-r));
  p.push_back(ctr + Pnt3(-r,-r, r));
  p.push_back(ctr + Pnt3( r,-r, r));
  p.push_back(ctr + Pnt3(-r, r, r));
  p.push_back(ctr + Pnt3( r, r, r));

  i.push_back(s+0);
  i.push_back(s+3);
  i.push_back(s+1);
  i.push_back(s+2);
  i.push_back(s+3);
  i.push_back(s+0);

  i.push_back(s+0);
  i.push_back(s+5);
  i.push_back(s+4);
  i.push_back(s+0);
  i.push_back(s+1);
  i.push_back(s+5);

  i.push_back(s+2);
  i.push_back(s+4);
  i.push_back(s+6);
  i.push_back(s+2);
  i.push_back(s+0);
  i.push_back(s+4);

  i.push_back(s+3);
  i.push_back(s+7);
  i.push_back(s+5);
  i.push_back(s+3);
  i.push_back(s+5);
  i.push_back(s+1);

  i.push_back(s+2);
  i.push_back(s+6);
  i.push_back(s+7);
  i.push_back(s+2);
  i.push_back(s+7);
  i.push_back(s+3);

  i.push_back(s+7);
  i.push_back(s+6);
  i.push_back(s+4);
  i.push_back(s+7);
  i.push_back(s+4);
  i.push_back(s+5);
}


// creates a mesh that shows the structure of the octree
void
TriOctree::visualize(vector<Pnt3> &p, vector<int> &ind)
{
  cube_bdry(mCtr, mRadius, p, ind);
  for (int i=0; i<8; i++) {
    if (child[i])
      child[i]->visualize(p,ind);
  }
}


#ifdef TRIOCTREEMAIN

void
main(void)
{
  Pnt3 *p = NULL;
  vector<int> tris;
  TriOctree to(Pnt3(), 1, p, tris, .1);
}

#endif
