//############################################################
// KDindtree.h
// Kari Pulli
// 05/22/96
// A KD tree which stores only indices to points in a
// Pointcloud
//############################################################

#ifndef _KDINDTREE_H_
#define _KDINDTREE_H_
#include "Pnt3.h"
#include <vector.h>

// factory:
class KDindtree* CreateKDindtree (const Pnt3* pts,
				  const short* nrms,
				  int nPts);

// STL Update
class KDindtree* CreateKDindtree (const vector<Pnt3>::iterator pts,
				  const vector<short>::iterator nrms,
				  int nPts);


class KDindtree {
private:

  int      m_d;       // discriminator, 0, 1, or 2 (for x,y,z)
  float    m_p;       // partition
  Pnt3     min, max;
  int      Nhere;     // how many in this node
  KDindtree *child[2];
  int       *element;

  // the center of the cone that contains the normals of the
  // points in this subtree
  Pnt3    normal;
  float   theta;   // the opening angle of that cone
  float   cos_th_p_pi_over_4;

  int _search(const Pnt3 *pts, const short *nrms,
	      const Pnt3 &p, const Pnt3 &n,
		   int &ind, float &d) const;
  int _search(const Pnt3 *pts, const Pnt3 &p,
		   int &ind, float &d) const;

// STL Update
  int _search(const vector<Pnt3>::iterator pts, const vector<short>::iterator nrms,
	      const Pnt3 &p, const Pnt3 &n,
		   int &ind, float &d) const;
  int _search(const vector<Pnt3>::iterator pts, const Pnt3 &p,
		   int &ind, float &d) const;

public:

  // ind is a temporary array (of length n) that contains
  // indices to pts and nrms (it is assumed that same index
  // works for both arrays)
  // ind is modified,
  KDindtree(const Pnt3 *pts, const short *nrms,
	    int *ind, int n, int first = 1);
// STL Update
  KDindtree(const vector<Pnt3>::iterator pts, const vector<short>::iterator nrms,
	    int *ind, int n, int first = 1);
  ~KDindtree();

  // use normals
  int search(const Pnt3 *pts, const short *nrms,
	     const Pnt3 &p, const Pnt3 &n,
	     int &ind, float &d) const
    {
      float _d = d;
      _search(pts, nrms, p, n, ind, d);
      return (d!=_d);
    }

  // just find the closest point
  int search(const Pnt3 *pts, const Pnt3 &p,
	     int &ind, float &d) const
    {
      float _d = d;
      _search(pts, p, ind, d);
      return (d!=_d);
    }

// STL Update
  // use normals
  int search(const vector<Pnt3>::iterator pts, const vector<short>::iterator nrms,
	     const Pnt3 &p, const Pnt3 &n,
	     int &ind, float &d) const
    {
      float _d = d;
      _search(pts, nrms, p, n, ind, d);
      return (d!=_d);
    }

  // just find the closest point
  int search(const vector<Pnt3>::iterator pts, const Pnt3 &p,
	     int &ind, float &d) const
    {
      float _d = d;
      _search(pts, p, ind, d);
      return (d!=_d);
    }

};

#endif
