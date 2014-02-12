//############################################################
//
// SyntheticScan.h
//
// Matt Ginzton
// Mon Jul 27 15:42:20 PDT 1998
//
// "Scan" objects created analytically rather than acquired.
//
//############################################################


#include "RigidScan.h"
#include "Bbox.h"


class SyntheticScan: public RigidScan
{
 public:
  SyntheticScan (float size = 1.0);

  // perVertex: colors and normals for every vertex (not every 3)
  // stripped:  triangle strips instead of triangles
  // color: one of the enum values from RigidScan.h
  // colorsize: # of bytes for color
  virtual MeshTransport* mesh(bool         perVertex = true,
			      bool         stripped  = true,
			      ColorSource  color = colorNone,
			      int          colorSize = 3);

  virtual OccSt carve_cube  (const Pnt3 &ctr, float side);

  virtual float
  closest_along_line_of_sight(const Pnt3 &p, Pnt3 &cp,
			      OccSt &status_p);
};
