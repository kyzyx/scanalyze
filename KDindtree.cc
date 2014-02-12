//############################################################
// KDindtree.cc
// Kari Pulli
// 04/13/96
//############################################################

#include <iostream>
#include <cassert>
#include "KDindtree.h"
#include "Bbox.h"
#include "defines.h"

#define MEDIAN_SPLIT 0

#if MEDIAN_SPLIT
#include "Median.h"
static Median<float> med;
#endif

// factory
KDindtree*
CreateKDindtree (const Pnt3* pts, const short* nrms, int nPts)
{
  cout << "Creating kdtree (" << nPts << " points)..." << flush;

  if (!nPts)
    return NULL;
  assert (pts);
  assert (nrms);

  // allocate and fill temp index array
  int *inds = new int[nPts];
  for (int i = 0; i < nPts; i++) inds[i] = i;

  // build tree
  KDindtree* kdtree = new KDindtree(pts, nrms, inds, nPts);

  // cleanup
  delete[] inds;

  cout << " done." << endl;
  return kdtree;
}


// STL Update
KDindtree*
CreateKDindtree (const vector<Pnt3>::iterator pts, const vector<short>::iterator nrms, int nPts)
{
  cout << "Creating kdtree (" << nPts << " points)..." << flush;

  if (!nPts)
    return NULL;

// WARNING: removing checks!
//  assert (pts);
//  assert (nrms);

  // allocate and fill temp index array
  int *inds = new int[nPts];
  for (int i = 0; i < nPts; i++) inds[i] = i;

  // build tree
  KDindtree* kdtree = new KDindtree(pts, nrms, inds, nPts);

  // cleanup
  delete[] inds;

  cout << " done." << endl;
  return kdtree;
}



static Pnt3
GetNormalAsPnt3 (const short* nrms, int ofs)
{
  ofs *= 3;
  Pnt3 n (nrms[ofs], nrms[ofs+1], nrms[ofs+2]);
  n /= 32767.0;
  return n;
}


static int
divisionsort(const Pnt3 *data, int *p, int n,
	     int dim, float med)
{
  // move values <= med to left, the rest to right
  int left = 0, right = n-1;
  while (1) {
    while (data[p[left]][dim] <= med && right > left) left++;
    while (data[p[right]][dim] > med && right > left) right--;
    int tmp = p[left];  // swap
    p[left] = p[right]; p[right] = tmp;
    if (left == right) {
      if (data[p[right]][dim] <= med) right++;
      break;
    }
    left++;
  }
  return right;
}

// STL Update
static int
divisionsort(const vector<Pnt3>::iterator data, int *p, int n,
	     int dim, float med)
{
  // move values <= med to left, the rest to right
  int left = 0, right = n-1;
  while (1) {
    while (data[p[left]][dim] <= med && right > left) left++;
    while (data[p[right]][dim] > med && right > left) right--;
    int tmp = p[left];  // swap
    p[left] = p[right]; p[right] = tmp;
    if (left == right) {
      if (data[p[right]][dim] <= med) right++;
      break;
    }
    left++;
  }
  return right;
}


void
merge_normal_cones(const Pnt3 &n1, float th1,
		   const Pnt3 &n2, float th2,
		   Pnt3 &n3, float &th3)
{
  // first, get the angle between the normals
  th3 = .5 * (th1 + th2) + acos(dot(n1,n2));

  // full sphere?
  if (th3 >= M_PI) {
    th3 = M_PI;
    n3.set(0,0,1);
    return;
  }

  // figure how much n1 should be rotated to become n3
  float half_ang = .25*(th3-th1);
  // create quaternion (real part and imaginary part)
  // for rotation
  float qr = cos(half_ang);
  Pnt3 qim = cross(n1,n2);
  qim *= sin(half_ang);
  // apply rotation
  (((n3 = n1) *= (qr*qr-qim.norm2()))
   += (cross(qim, n1) *= (2.0*qr)))
    += (qim *= (2.0*dot(n1,qim)));
  //n3 = (qr*qr-qim.norm2())*n1 + (2*dot(n1,qim))*qim
  //  + 2*qr*cross(qim,n1);
}


KDindtree::KDindtree(const Pnt3 *pts, const short *nrms,
		     int *ind, int n, int first)
: Nhere(0), element(NULL)
{
  int i;
  // find the dimension of maximum range
  min = max = pts[ind[0]];
  int *end = ind+n;
  for (int *ip=ind+1; ip<end; ip++) {
    const Pnt3 &p = pts[*ip];
    min.set_min(p);
    max.set_max(p);
  }
  float dist = max[0] - min[0];
  m_d = 0;
  float tmp;
  if ((tmp = max[1]-min[1]) > dist) {
    m_d = 1; dist = tmp;
  }
  if ((tmp = max[2]-min[2]) > dist) {
    m_d = 2; dist = tmp;
  }

  if (dist == 0.0) n = 1; // a single point several times

  if (n > 16) {
#if MEDIAN_SPLIT
#define DATA(i) (pts[ind[i]])[m_d]
    // find the median within that dimension
    med.clear();
    for (i=0; i<n; i++) med += DATA(i);
    m_p = med.find();
    int right = divisionsort(pts, ind, n, m_d, m_p);
    if (right == n) {
      // the median is also the largest, need new "median"
      // find the next largest item for that
      float nm = -9.e33;
      for (i=0; i<n; i++) {
	if (DATA(i) != m_p && DATA(i) > nm) nm = DATA(i);
      }
      right = divisionsort(pts, ind, n, m_d, (m_p=nm));
    }
    assert(right != 0 && right != n);
#undef DATA
#else
    m_p = .5*(max[m_d]+min[m_d]);
    int right = divisionsort(pts, ind, n, m_d, m_p);
    assert(right != 0 && right != n);
#endif
    // recurse
    child[0] = new KDindtree(pts, nrms, ind, right, 0);
    child[1] = new KDindtree(pts, nrms, &ind[right], n-right, 0);
  } else {
    // store data here
    Nhere = n;
    element = new int[n];
    for (i=0; i<n; i++) element[i] = ind[i];
    child[0] = child[1] = NULL;
  }

#if MEDIAN_SPLIT
  if (first) med.zap(); // release memory after the tree's done
#endif

  if (nrms == NULL) return;

  // now figure out bounds for the normals
  if (Nhere) {
    // a terminal node
    normal = GetNormalAsPnt3(nrms, element[0]);
    theta  = 0.0;
    for (i=1; i<Nhere; i++) {
      merge_normal_cones(normal, theta,
			 GetNormalAsPnt3(nrms, element[i]), 0,
			 normal, theta);
    }
  } else {
    // a non-terminal node
    merge_normal_cones(child[0]->normal, child[0]->theta,
		       child[1]->normal, child[1]->theta,
		       normal, theta);
  }
  tmp = theta + M_PI * .25;
  if (tmp > M_PI) cos_th_p_pi_over_4 = -1.0;
  else            cos_th_p_pi_over_4 = cos(tmp);
}

// STL Update
KDindtree::KDindtree(const vector<Pnt3>::iterator pts, const vector<short>::iterator nrms,
		     int *ind, int n, int first)
: Nhere(0), element(NULL)
{
  int i;
  // find the dimension of maximum range
  min = max = pts[ind[0]];
  int *end = ind+n;
  for (int *ip=ind+1; ip<end; ip++) {
    const Pnt3 &p = pts[*ip];
    min.set_min(p);
    max.set_max(p);
  }
  float dist = max[0] - min[0];
  m_d = 0;
  float tmp;
  if ((tmp = max[1]-min[1]) > dist) {
    m_d = 1; dist = tmp;
  }
  if ((tmp = max[2]-min[2]) > dist) {
    m_d = 2; dist = tmp;
  }

  if (dist == 0.0) n = 1; // a single point several times

  if (n > 16) {
#if MEDIAN_SPLIT
#define DATA(i) (pts[ind[i]])[m_d]
    // find the median within that dimension
    med.clear();
    for (i=0; i<n; i++) med += DATA(i);
    m_p = med.find();
    int right = divisionsort(pts, ind, n, m_d, m_p);
    if (right == n) {
      // the median is also the largest, need new "median"
      // find the next largest item for that
      float nm = -9.e33;
      for (i=0; i<n; i++) {
	if (DATA(i) != m_p && DATA(i) > nm) nm = DATA(i);
      }
      right = divisionsort(pts, ind, n, m_d, (m_p=nm));
    }
    assert(right != 0 && right != n);
#undef DATA
#else
    m_p = .5*(max[m_d]+min[m_d]);
    int right = divisionsort(pts, ind, n, m_d, m_p);
    assert(right != 0 && right != n);
#endif
    // recurse
    child[0] = new KDindtree(pts, nrms, ind, right, 0);
    child[1] = new KDindtree(pts, nrms, &ind[right], n-right, 0);
  } else {
    // store data here
    Nhere = n;
    element = new int[n];
    for (i=0; i<n; i++) element[i] = ind[i];
    child[0] = child[1] = NULL;
  }

#if MEDIAN_SPLIT
  if (first) med.zap(); // release memory after the tree's done
#endif

// STL Update
// WARNING: Not sure how to check for this..
//  if (nrms == NULL) return;

  // now figure out bounds for the normals
  if (Nhere) {
    // a terminal node
// STL Update
    normal = GetNormalAsPnt3(&*nrms, element[0]);
    theta  = 0.0;
    for (i=1; i<Nhere; i++) {
// STL Update
      merge_normal_cones(normal, theta,
			 GetNormalAsPnt3(&*nrms, element[i]), 0,
			 normal, theta);
    }
  } else {
    // a non-terminal node
    merge_normal_cones(child[0]->normal, child[0]->theta,
		       child[1]->normal, child[1]->theta,
		       normal, theta);
  }
  tmp = theta + M_PI * .25;
  if (tmp > M_PI) cos_th_p_pi_over_4 = -1.0;
  else            cos_th_p_pi_over_4 = cos(tmp);
}


KDindtree::~KDindtree(void)
{
  delete[] element;
  delete child[0];
  delete child[1];
}


int
KDindtree::_search(const Pnt3 *pts, const short *nrms,
		   const Pnt3 &p, const Pnt3 &n,
		   int &ind, float &d) const
{
  assert(this);

  if (dot(n, normal) < cos_th_p_pi_over_4)
    return 0;

  if (Nhere) { // terminal node
    float l, d2 = d*d;
    bool  need_sqrt = false;
    int *el  = element;
    int *end = el+Nhere;
    for (; el<end; el++) {
      l = dist2(pts[*el], p);
      if (l < d2) {
	const short *sp = &nrms[(*el)*3];
	// 32767/sqrt(2)==23169.77
	if (n[0]*sp[0] + n[1]*sp[1] + n[2]*sp[2] > 23169.77) {
	  // normals also within 45 deg
	  d2=l; ind = *el;
	  need_sqrt = true;
	}
      }
    }
    if (need_sqrt) d = sqrtf(d2);
    return ball_within_bounds(p,d,min,max);
  }

  if (p[m_d] <= m_p) { // the point is left from partition
    if (child[0]->_search(pts,nrms,p,n,ind,d))
      return 1;
    if (bounds_overlap_ball(p,d,child[1]->min,max)) {
      if (child[1]->_search(pts,nrms,p,n,ind,d))
	return 1;
    }
  } else {             // the point is right from partition
    if (child[1]->_search(pts,nrms,p,n,ind,d))
      return 1;
    if (bounds_overlap_ball(p,d,min,child[0]->max)) {
      if (child[0]->_search(pts,nrms,p,n,ind,d))
	return 1;
    }
  }

  return ball_within_bounds(p,d,min,max);
}


int
KDindtree::_search(const Pnt3 *pts, const Pnt3 &p,
		   int &ind, float &d) const
{
  assert(this);

  if (Nhere) { // terminal node
    float l, d2 = d*d;
    bool  need_sqrt = false;
    int *el  = element;
    int *end = el+Nhere;
    for (; el<end; el++) {
      l = dist2(pts[*el], p);
      if (l < d2) {
	d2=l; ind = *el;
	need_sqrt = true;
      }
    }
    if (need_sqrt) d = sqrtf(d2);
    return ball_within_bounds(p,d,min,max);
  }

  if (p[m_d] <= m_p) { // the point is left from partition
    if (child[0]->_search(pts,p,ind,d))
      return 1;
    if (bounds_overlap_ball(p,d,child[1]->min,max)) {
      if (child[1]->_search(pts,p,ind,d))
	return 1;
    }
  } else {             // the point is right from partition
    if (child[1]->_search(pts,p,ind,d))
      return 1;
    if (bounds_overlap_ball(p,d,min,child[0]->max)) {
      if (child[0]->_search(pts,p,ind,d))
	return 1;
    }
  }

  return ball_within_bounds(p,d,min,max);
}


// STL Update
int
KDindtree::_search(const vector<Pnt3>::iterator pts, const vector<short>::iterator nrms,
		   const Pnt3 &p, const Pnt3 &n,
		   int &ind, float &d) const
{
  assert(this);

  if (dot(n, normal) < cos_th_p_pi_over_4)
    return 0;

  if (Nhere) { // terminal node
    float l, d2 = d*d;
    bool  need_sqrt = false;
    int *el  = element;
    int *end = el+Nhere;
    for (; el<end; el++) {
      l = dist2(pts[*el], p);
      if (l < d2) {
	const short *sp = &nrms[(*el)*3];
	// 32767/sqrt(2)==23169.77
	if (n[0]*sp[0] + n[1]*sp[1] + n[2]*sp[2] > 23169.77) {
	  // normals also within 45 deg
	  d2=l; ind = *el;
	  need_sqrt = true;
	}
      }
    }
    if (need_sqrt) d = sqrtf(d2);
    return ball_within_bounds(p,d,min,max);
  }

  if (p[m_d] <= m_p) { // the point is left from partition
    if (child[0]->_search(pts,nrms,p,n,ind,d))
      return 1;
    if (bounds_overlap_ball(p,d,child[1]->min,max)) {
      if (child[1]->_search(pts,nrms,p,n,ind,d))
	return 1;
    }
  } else {             // the point is right from partition
    if (child[1]->_search(pts,nrms,p,n,ind,d))
      return 1;
    if (bounds_overlap_ball(p,d,min,child[0]->max)) {
      if (child[0]->_search(pts,nrms,p,n,ind,d))
	return 1;
    }
  }

  return ball_within_bounds(p,d,min,max);
}


int
KDindtree::_search(const vector<Pnt3>::iterator pts, const Pnt3 &p,
		   int &ind, float &d) const
{
  assert(this);

  if (Nhere) { // terminal node
    float l, d2 = d*d;
    bool  need_sqrt = false;
    int *el  = element;
    int *end = el+Nhere;
    for (; el<end; el++) {
      l = dist2(pts[*el], p);
      if (l < d2) {
	d2=l; ind = *el;
	need_sqrt = true;
      }
    }
    if (need_sqrt) d = sqrtf(d2);
    return ball_within_bounds(p,d,min,max);
  }

  if (p[m_d] <= m_p) { // the point is left from partition
    if (child[0]->_search(pts,p,ind,d))
      return 1;
    if (bounds_overlap_ball(p,d,child[1]->min,max)) {
      if (child[1]->_search(pts,p,ind,d))
	return 1;
    }
  } else {             // the point is right from partition
    if (child[1]->_search(pts,p,ind,d))
      return 1;
    if (bounds_overlap_ball(p,d,min,child[0]->max)) {
      if (child[0]->_search(pts,p,ind,d))
	return 1;
    }
  }

  return ball_within_bounds(p,d,min,max);
}
