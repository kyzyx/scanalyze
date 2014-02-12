
//############################################################
// KDtritree.cc
// Kari Pulli
// 11/19/98
//############################################################

#include "KDtritree.h"
#include "Bbox.h"
#include "Median.h"

static Median<float> med;


static int
mediansort(KDsphere *data, int n,
	   int dim, float med)
{
  // move values <= med to left, the rest to right
  int left = 0, right = n-1;
  while (1) {
    while (data[left].ctr[dim] <= med && right > left) left++;
    while (data[right].ctr[dim] > med && right > left) right--;
    KDsphere tmp = data[left];  // swap
    data[left] = data[right]; data[right] = tmp;
    if (left == right) {
      if (data[right].ctr[dim] <= med) right++;
      break;
    }
    left++;
  }
  return right;
}


void
KDtritree::collapse_spheres(void)
{
  float cdist = dist(child[0]->ctr, child[1]->ctr);

  if (cdist + child[0]->radius < child[1]->radius) {
    ctr    = child[1]->ctr;
    radius = child[1]->radius;
  } else if (cdist + child[1]->radius < child[0]->radius) {
    ctr    = child[0]->ctr;
    radius = child[0]->radius;
  } else {
    // OK, find the new bounding sphere
    radius  = 0.5 * (cdist + child[0]->radius + child[1]->radius);
    float t = (radius - child[0]->radius) / cdist;
    ctr.lerp(t, child[0]->ctr, child[1]->ctr);
  }
}


KDtritree::KDtritree(const Pnt3 *pts, const int *triinds,
		     KDsphere *spheres, int n, int first)
{
  int i;

  if (n > 1) {
    // find the dimension of maximum range (of sphere centers)
    Bbox bbox;
    for (i=0; i<n; i++) bbox.add(spheres[i].ctr);
    Pnt3 diag = bbox.max(); diag -= bbox.min();
    if (diag[2] > diag[1] && diag[2] > diag[0]) ind = 2;
    else ind = diag[1] > diag[0];

    if (diag.norm2() == 0.0) {
      /*
      // all the spheres have same center?
      cerr << "KDtritree Warning: several triangles with the "
	   << "same center?!?" << endl;
      SHOW(diag);
      SHOW(n);
      for (i=0; i<n; i++) {
	SHOW(spheres[i].ctr);
	SHOW(spheres[i].radius);
      }
      */
      n = 1;
    }
  }

  if (n > 1) {
#define DATA(i) spheres[i].ctr[ind]
    // find the median within that dimension
    med.clear();
    for (i=0; i<n; i++) med += DATA(i);
    split = med.find();
    int right = mediansort(spheres, n, ind, split);
    if (right == n) {
      // the median is also the largest, need new "median"
      // find the next largest item for that
      float nm = -9.e33;
      for (i=0; i<n; i++) {
	if (DATA(i) != split && DATA(i) > nm) nm = DATA(i);
      }
      right = mediansort(spheres, n, ind, (split=nm));
    }
    assert(right != 0 && right != n);
#undef DATA

    // recurse
    child[0] = new KDtritree(pts, triinds, spheres, right, 0);
    child[1] = new KDtritree(pts, triinds, &spheres[right], n-right, 0);

    collapse_spheres();
  } else {
    // leaf, store data here
    ind      = spheres->ind;
    ctr      = spheres->ctr;
    radius   = spheres->radius;
    child[0] = child[1] = NULL;
  }

  if (first) med.zap(); // release memory after the tree's done
}

// STL Update
KDtritree::KDtritree(const vector<Pnt3>::const_iterator pts, const vector<int>::const_iterator triinds,
		     KDsphere *spheres, int n, int first)
{
  int i;

  if (n > 1) {
    // find the dimension of maximum range (of sphere centers)
    Bbox bbox;
    for (i=0; i<n; i++) bbox.add(spheres[i].ctr);
    Pnt3 diag = bbox.max(); diag -= bbox.min();
    if (diag[2] > diag[1] && diag[2] > diag[0]) ind = 2;
    else ind = diag[1] > diag[0];

    if (diag.norm2() == 0.0) {
      /*
      // all the spheres have same center?
      cerr << "KDtritree Warning: several triangles with the "
	   << "same center?!?" << endl;
      SHOW(diag);
      SHOW(n);
      for (i=0; i<n; i++) {
	SHOW(spheres[i].ctr);
	SHOW(spheres[i].radius);
      }
      */
      n = 1;
    }
  }

  if (n > 1) {
#define DATA(i) spheres[i].ctr[ind]
    // find the median within that dimension
    med.clear();
    for (i=0; i<n; i++) med += DATA(i);
    split = med.find();
    int right = mediansort(spheres, n, ind, split);
    if (right == n) {
      // the median is also the largest, need new "median"
      // find the next largest item for that
      float nm = -9.e33;
      for (i=0; i<n; i++) {
	if (DATA(i) != split && DATA(i) > nm) nm = DATA(i);
      }
      right = mediansort(spheres, n, ind, (split=nm));
    }
    assert(right != 0 && right != n);
#undef DATA

    // recurse
    child[0] = new KDtritree(pts, triinds, spheres, right, 0);
    child[1] = new KDtritree(pts, triinds, &spheres[right], n-right, 0);

    collapse_spheres();
  } else {
    // leaf, store data here
    ind      = spheres->ind;
    ctr      = spheres->ctr;
    radius   = spheres->radius;
    child[0] = child[1] = NULL;
  }

  if (first) med.zap(); // release memory after the tree's done
}

KDtritree::~KDtritree(void)
{
  delete child[0];
  delete child[1];
}



void
KDtritree::_search(const Pnt3 *pts, const int *inds,
		   const Pnt3 &p, Pnt3 &cp, float &d2) const
{
  if (child[0] == NULL) {
    // terminal node, check the triangle and return
    closer_on_tri(p, cp, pts[inds[ind]],
		  pts[inds[ind+1]],
		  pts[inds[ind+2]], d2);
    return;
  }

  assert(child[1]);

  if (p[ind] <= split) {
    // can the left subtree contain anything interesting?
    if (spheres_intersect(p, child[0]->ctr, d2, child[0]->radius))
      child[0]->_search(pts, inds, p, cp, d2);
    // can the right subtree contain anything interesting?
    if (spheres_intersect(p, child[1]->ctr, d2, child[1]->radius))
      child[1]->_search(pts, inds, p, cp, d2);
  } else {
    // can the right subtree contain anything interesting?
    if (spheres_intersect(p, child[1]->ctr, d2, child[1]->radius))
      child[1]->_search(pts, inds, p, cp, d2);
    // can the left subtree contain anything interesting?
    if (spheres_intersect(p, child[0]->ctr, d2, child[0]->radius))
      child[0]->_search(pts, inds, p, cp, d2);
  }
}

// STL Update
void
KDtritree::_search(const vector<Pnt3>::const_iterator pts, const vector<int>::const_iterator inds,
		   const Pnt3 &p, Pnt3 &cp, float &d2) const
{
  if (child[0] == NULL) {
    // terminal node, check the triangle and return
    closer_on_tri(p, cp, pts[inds[ind]],
		  pts[inds[ind+1]],
		  pts[inds[ind+2]], d2);
    return;
  }

  assert(child[1]);

  if (p[ind] <= split) {
    // can the left subtree contain anything interesting?
    if (spheres_intersect(p, child[0]->ctr, d2, child[0]->radius))
      child[0]->_search(pts, inds, p, cp, d2);
    // can the right subtree contain anything interesting?
    if (spheres_intersect(p, child[1]->ctr, d2, child[1]->radius))
      child[1]->_search(pts, inds, p, cp, d2);
  } else {
    // can the right subtree contain anything interesting?
    if (spheres_intersect(p, child[1]->ctr, d2, child[1]->radius))
      child[1]->_search(pts, inds, p, cp, d2);
    // can the left subtree contain anything interesting?
    if (spheres_intersect(p, child[0]->ctr, d2, child[0]->radius))
      child[0]->_search(pts, inds, p, cp, d2);
  }
}


KDtritree *
create_KDtritree(const Pnt3 *pts, const int *inds, int n)
{
  assert(n%3==0);
  // first, create as many spheres as there are triangles
  int ns = n/3;
  KDsphere *spheres = new KDsphere[ns];
  for (int i=0; i<ns; i++) {
    int ti = i*3;
    spheres[i].radius =
      spheres[i].ctr.smallest_circle(pts[inds[ti]],
				     pts[inds[ti+1]],
				     pts[inds[ti+2]]);
    spheres[i].ind = ti;
    //SHOW(spheres[i].radius);
    //SHOW(spheres[i].ctr);
  }
  KDtritree *ret = new KDtritree(pts, inds, spheres, ns, 1);
  delete[] spheres;
  return ret;
}

// STL Update
KDtritree *
create_KDtritree(const vector<Pnt3>::const_iterator pts, const vector<int>::const_iterator inds, int n)
{
  assert(n%3==0);
  // first, create as many spheres as there are triangles
  int ns = n/3;
  KDsphere *spheres = new KDsphere[ns];
  for (int i=0; i<ns; i++) {
    int ti = i*3;
    spheres[i].radius =
      spheres[i].ctr.smallest_circle(pts[inds[ti]],
				     pts[inds[ti+1]],
				     pts[inds[ti+2]]);
    spheres[i].ind = ti;
    //SHOW(spheres[i].radius);
    //SHOW(spheres[i].ctr);
  }
  KDtritree *ret = new KDtritree(pts, inds, spheres, ns, 1);
  delete[] spheres;
  return ret;
}
