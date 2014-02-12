//############################################################
//
// SyntheticScan.cc
//
// Matt Ginzton
// Mon Jul 27 15:42:20 PDT 1998
//
// "Scan" objects created analytically rather than acquired.
//
//############################################################


#include "SyntheticScan.h"
#include "MeshTransport.h"
#include "TriMeshUtils.h"

#define SYNTH_SPHERE 1
//#define SYNTH_CUBE 1


SyntheticScan::SyntheticScan (float size)
{
  Pnt3 p (size, size, size);
  bbox = Bbox (-p, p);
  set_name ("cube");
  insert_resolution (12, get_name());
}


MeshTransport*
SyntheticScan::mesh(bool perVertex, bool stripped,
		    ColorSource color, int colorSize)
{
  MeshTransport* mt = new MeshTransport;

  vector<Pnt3>* vtx = new vector<Pnt3>;
  vtx->reserve (8);
  for (int i = 0; i < 8; i++) {
    vtx->push_back (bbox.corner(i));
  }

  vector<short>* nrm = new vector<short>;
  if (perVertex) {
    nrm->reserve(24);
    for (i = 0; i < 8; i++) {
      pushNormalAsShorts (*nrm, bbox.corner(i).normalize());
    }
  } else {
    nrm->reserve (36);
  }

  const int faceInd[12][3] = {
    { 0, 1, 2 },
    { 1, 3, 2 },
    { 2, 3, 6 },
    { 3, 7, 6 },
    { 6, 7, 4 },
    { 7, 5, 4 },
    { 0, 4, 1 },
    { 1, 4, 5 },
    { 0, 6, 4 },
    { 0, 2, 6 },
    { 1, 5, 7 },
    { 1, 7, 3 }
  };

  vector<int>* tri_inds = new vector<int>;
  for (i = 0; i < 12; i++) {
    int t1 = faceInd[i][0];
    int t2 = faceInd[i][1];
    int t3 = faceInd[i][2];

    tri_inds->push_back (t1);
    tri_inds->push_back (t2);
    tri_inds->push_back (t3);

    if (!perVertex) {
      Pnt3 normal = cross (((*vtx)[t1] - (*vtx)[t2]),
			   ((*vtx)[t1] - (*vtx)[t3]));
      normal.normalize();
      pushNormalAsShorts (*nrm, normal);
    }

    if (stripped)              // end strip
      tri_inds->push_back (-1);
  }

  mt->setVtx (vtx, MeshTransport::steal);
  mt->setNrm (nrm, MeshTransport::steal);
  mt->setTris (tri_inds, MeshTransport::steal);
  return mt;
}


OccSt
SyntheticScan::carve_cube  (const Pnt3 &ctr, float side)
{
  side /= 2;
  Pnt3 ofs (side, side, side);
  Bbox cube (ctr - ofs, ctr + ofs);

  float radius = bbox.size() / 2.0;
  Xform<float> xfInv = getXform();
  xfInv.fast_invert();

  bool bAllIn = true, bAllOut = true; // points of node relative to mesh

  for (int i = 0; i < 8; i++) {
    Pnt3 corner = cube.corner(i);
    xfInv (corner);
#if SYNTH_CUBE
    if (bbox.outside (corner))
      bAllIn = false;
    else
      bAllOut = false;

    // check whether this node contains current corner of entire cube --
    // if so, node can be neither entirely in front nor behind, so it's
    // a boundary
    corner = bbox.corner(i);
    xformPnt (corner);
    if (!cube.outside (corner))
      return BOUNDARY;

#else // SYNTH_SPHERE
    if (corner.norm() > radius)
      bAllIn = false;
    else
      bAllOut = false;
#endif
  }

#if SYNTH_SPHERE
  if (bAllOut) { // possible containment of sphere in voxel
    Pnt3 lo = cube.min();
    Pnt3 hi = cube.max();

    if (lo[0] < -radius && lo[1] < -radius && lo[2] < -radius
	&& hi[0] > radius && hi[1] > radius && hi[2] > radius)
      return BOUNDARY;
  }
#endif

  if (bAllIn)
    return INSIDE;
  else if (bAllOut)
    return OUTSIDE;
  else
    return BOUNDARY;
}


float
SyntheticScan::closest_along_line_of_sight(const Pnt3 &p, Pnt3 &cp,
					   OccSt &status_p)
{
#if SYNTH_CUBE

  Pnt3 _p;
  xf.apply_inv (p, _p);

  Pnt3 c = bbox.center();
  for (int i = 0; i < 3; i++)
    cp[i] = (_p[i] > c[i] ? bbox.max()[i] : bbox.min()[i]);

  if (bbox.outside (_p))
    status_p = OUTSIDE;
  else
    status_p = INSIDE;

  return 1.0;  // always confident

#else // SYNTH_SPHERE

  // ignore "line of sight" constraint
  // just return closest point
  float radius = bbox.size() / 2.0;
  float dist_to_ctr = p.norm();
  cp = p / dist_to_ctr * radius;

  if (dist_to_ctr < radius) {
    status_p = INSIDE;
  } else {
    status_p = OUTSIDE;
  }

  return 1.0; // always confident
#endif
}
