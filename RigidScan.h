//############################################################
//
// RigidScan.h
//
// Kari Pulli
// Mon Jun 29 11:51:05 PDT 1998
//
// An abstract base class for holding all the scanner specific
// information of a complete scan.
//
// This class is meant to form an abstraction between the raw
// (or minimally processed) data from various scanners and
// the algorithms that are used to (at least)
//    o  display the data for view planning, registration, etc.,
//       possibly using a limited polygon budget;
//    o  determine a volumetric approximate model of the target;
//    o  fit an accurate surface description of the target using
//       the range data;
//    o  associate possible input color with scanned geometry.
//
// The logical unit is a single rigidly registered scan.
// This can mean different things for different scanners.
//    o  Cyrax: probably just a single scan.
//    o  Cyberware custom statue scanner: it may include
//       dozens of scan sweeps as long as the base of the
//       scanner was not moved.
//    o  Modelmaker the data from a single Faro position.
//    o  Generic scanner: Any other scanner from which we
//       don't know much more than range maps.
//
//############################################################

#ifndef _RIGID_SCAN_H_
#define _RIGID_SCAN_H_

#include <vector>
#include <ext/rope>

#include "ResolutionCtrl.h"
#include "TbObj.h"
#include "Xform.h"
#include "Pnt3.h"
class VertexFilter;
class MeshTransport;

typedef unsigned char uchar;

typedef enum {  // The queried entity is (in the precedence order)
  OUTSIDE,        //   in front of data
  BOUNDARY,       //   intersects data
  SILHOUETTE,     //   partially behind data, does not intersect
  INSIDE,         //   behind data
  NOT_IN_FRUSTUM, //   not in the view frustum of scanner
  INDETERMINATE   //   nothing is known (maybe missing data)
} OcclusionStatus, OccSt;


class RigidScan : public ResolutionCtrl, public TbObj {
private:

  // inherit from ResolutionCtrl:
  // vector<res_info> resolutions;
  // int              curr_res;
  // crope            name
  // crope            basename
  // crope            ending
  //
  // inherit from TbObj:
  // Xform<float>     xf;
  // Pnt3             rot_ctr;
  // Bbox             bbox;


public:

  RigidScan();
  virtual ~RigidScan() {}

  //////////////////////////////////////////////////////////////
  // Data access methods
  //////////////////////////////////////////////////////////////

  enum ColorSource { colorNone, colorIntensity, colorTrue,
		     colorConf, colorBoundary };
  // perVertex: colors and normals for every vertex (not every 3)
  // stripped: triangle strips instead of triangles
  // color: one of the above enum values
  // colorsize: # of bytes for color: 1 = intensity,
  //              2 = intensity+alpha, 3 = rgb, 4 = rgba
  virtual MeshTransport* mesh(bool         perVertex = true,
			      bool         stripped  = true,
			      ColorSource  color = colorNone,
			      int          colorSize = 3) = 0;

  // scans that don't want to return a MeshTransport can render themselves
  // any way they want.
  virtual bool render_self (ColorSource color = colorNone);

  virtual int  num_vertices(void);
  virtual void subsample_points(float rate, vector<Pnt3> &p,
				vector<Pnt3> &n);
  virtual RigidScan* filtered_copy(const VertexFilter &filter);
  virtual bool filter_inplace(const VertexFilter &filter);
  virtual bool filter_vertices (const VertexFilter& filter, vector<Pnt3>& p);
  virtual crope getInfo(void);
  virtual void flipNormals (void);
  virtual void computeBBox (void);
  virtual unsigned long byteSize() { return 0; }

  //////////////////////////////////////////////////////////////
  // Volume methods (space carving)
  // performed in world coordinates
  //////////////////////////////////////////////////////////////

  virtual OccSt carve_cube  (const Pnt3 &ctr, float side);
  virtual OccSt carve_sphere(const Pnt3 &ctr, float radius);

  //////////////////////////////////////////////////////////////
  // Point methods (registration, signed-distance function, ...)
  // a float return value is always confidence/weight [0,1]
  //////////////////////////////////////////////////////////////

  // for ICP registration:
  // returns
  // TRUE: closest point with normal withig 45 deg is within dThr
  // FALSE: isn't or that point is on boundary when not allowed
  virtual bool
    closest_point(const Pnt3 &p, const Pnt3 &n,
		  Pnt3 &cl_pnt, Pnt3 &cl_nrm,
		  float thr = 1e33, bool bdry_ok = 0);

#if 0   // unused, never overridden, causes compile warnings
  // for something else...
  virtual float
  closest_point(const Pnt3 &p, Pnt3 &cl_pnt);
  virtual float
  closest_point(const Pnt3 &p, Pnt3 &cl_pnt, Pnt3 &cl_nrm);
#endif

  // for volumetric processing
  virtual float
  closest_point_on_mesh(const Pnt3 &p, Pnt3 &cl_pnt, OccSt &status_p);
  virtual float
  closest_along_line_of_sight(const Pnt3 &p, Pnt3 &cp,
			      OccSt &status_p);

  virtual float
  closest_along_line(const Pnt3 &p, const Pnt3 &dir,
		     Pnt3 &cp, OccSt &status_p);

  virtual float
  color_along_line_of_sight(const Pnt3 &p, float rgb[3]);
  virtual float
  color_along_line_of_sight(const Pnt3 &p, uchar rgb[3]);

  //////////////////////////////////////////////////////////////
  // File I/O
  //////////////////////////////////////////////////////////////

  virtual bool read(const crope &fname);

  // is data worth saving?
  virtual bool is_modified (void);
  // save to given name: if default, save to existing name if there is
  // one, or return false if there's not
  virtual bool write(const crope& fname = crope());
  // for saving individual meshes
  virtual bool write_resolution_mesh (int npolys,
				      const crope& fname = crope(),
				      Xform<float> xfBy = Xform<float>());
  // for saving anything else
     // if you cut-and-paste these function delcarations to the .h file
     // for a subclass, don't copy the enum MetaData declaration
     // or the virtual function override will fail!
  enum MetaData { md_xform };
  virtual bool write_metadata (MetaData data);

  ////////////////////////////////////////////////////////////////
  // Aggregation
  ////////////////////////////////////////////////////////////////
  virtual bool get_children (vector<RigidScan*>& children) const;
};


#endif /* _RIGID_SCAN_H_ */
