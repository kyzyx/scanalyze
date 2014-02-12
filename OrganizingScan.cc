//////////////////////////////////////////////////////////////////////
//
// OrganizingScan.h
//
// Matt Ginzton
// Tue May 30 16:41:09 PDT 2000
//
// Dummy scan/helper class for DisplayableOrganizingMesh
//
//////////////////////////////////////////////////////////////////////


#include "OrganizingScan.h"
#include <iostream.h>



OrganizingScan::OrganizingScan()
{
}



MeshTransport*
OrganizingScan::mesh(bool         perVertex,
		     bool         stripped,
		     ColorSource  color,
		     int          colorSize)
{
  cerr << " OrganizingScan::mesh was never intended to be called" << endl;
  return NULL;
}


void
OrganizingScan::computeBBox (void)
{
  // TODO, BUGBUG
  // really need some form of bbox management
  // does that mean we'll have to know who our children are?
#if 0
  bbox.clear();
  rot_ctr = Pnt3();
  FOR_EACH_CHILD (it) {
    RigidScan* rs = (*it)->getMeshData();
    Bbox childBox = rs->worldBbox();
    if (childBox.valid()) {
      bbox.add (childBox);
      rot_ctr += rs->worldCenter();
    }
  }

  if (children.size())
    rot_ctr /= children.size();

  rot_ctr = bbox.center();
#endif
}


crope
OrganizingScan::getInfo (void)
{
  char info[1000];

  sprintf (info, "Organizing group containing ?? members.\n\n");
  return crope (info) + RigidScan::getInfo();
}
