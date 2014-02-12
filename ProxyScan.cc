// ProxyScan.cc                placeholder for unloaded scans
// created 3/12/99             Matt Ginzton (magi@cs)


#include "ProxyScan.h"
#include "MeshTransport.h"
#include "TriMeshUtils.h"


ProxyScan::ProxyScan (const crope& proxyForFileName,
		      const Pnt3& min, const Pnt3& max)
{
  set_name (proxyForFileName);
  TbObj::readXform (get_basename());

  // set the bbox, but only if min<max; an uninitialized bbox
  // will come in as max<min.  But adding these two points to
  // the bbox will result in a valid bbox bounding everything.
  if (min[0] < max[0] && min[1] < max[1] && min[2] < max[2]) {
    bbox.add (min);
    bbox.add (max);
  }

  insert_resolution (0, proxyForFileName, false, false);
}


ProxyScan::~ProxyScan()
{
}


MeshTransport*
ProxyScan::mesh (bool perVertex, bool stripped,
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
      pushNormalAsShorts (*nrm, (bbox.corner(i) - bbox.center()).normalize());
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


crope
ProxyScan::getInfo (void)
{
  return crope ("Proxy scan, data not loaded.\n\n") + RigidScan::getInfo();
}
