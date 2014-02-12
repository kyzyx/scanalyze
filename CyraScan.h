//############################################################
//
// CyraScan.h
//
// Lucas Pereira
// Thu Jul 16 16:09:38 1998
//
// Store range scan information from a Cyra Cyrax Beta
// time-of-flight laser range scanner.
//
//############################################################

#ifndef _CyraSCAN_H_
#define _CyraSCAN_H_

#include "RigidScan.h"
#include "CyraResLevel.h"

class KDtritree;


//////////////////////////////////////////////////////////////////////
// CyraScan (all resolution levels for a cyra scan)
//////////////////////////////////////////////////////////////////////

class CyraScan : public RigidScan {

public:

  CyraScan(void);
  ~CyraScan(void);

  // perVertex: colors and normals for every vertex (not every 3)
  // stripped: triangle strips instead of triangles
  // color: one of the above enum values
  // colorsize: # of bytes for color: 1 = intensity,
  //              2 = intensity+alpha, 3 = rgb, 4 = rgba
  virtual MeshTransport* mesh(bool         perVertex = true,
			      bool         stripped  = true,
			      ColorSource  color = colorNone,
			      int          colorSize = 3);

  int num_vertices();
  int num_vertices(int resNum);
  int num_tris();
  int num_tris(int resNum);

  bool load_resolution (int iRes);

  void computeBBox (void);
  void flipNormals (void);

  bool read(const crope &fname);
  bool write(const crope &fname);
  bool ReadPts(const crope &fname);	// Read a .pts file
  bool WritePts(const crope &fname);	// Write a .pts file

  RigidScan* filtered_copy(const VertexFilter& filter);
  bool filter_inplace(const VertexFilter& filter);

  // for ICP...
  void subsample_points(float rate, vector<Pnt3> &p, vector<Pnt3> &n);
  bool closest_point(const Pnt3 &p, const Pnt3 &n,
		     Pnt3 &cp, Pnt3 &cn,
		     float thr = 1e33, bool bdry_ok = 0);

  // for volumetric processing
  virtual float
  closest_point_on_mesh(const Pnt3 &p, Pnt3 &cl_pnt, OccSt &status_p);
  virtual float
  closest_along_line_of_sight(const Pnt3 &p, Pnt3 &cp,
			      OccSt &status_p);
  virtual OccSt carve_cube  (const Pnt3 &ctr, float side);

  //void TriOctreeMesh(vector<Pnt3> &p, vector<int> &ind); // for debugging...

private:
  vector<CyraResLevel> levels;

  KDtritree   *triKD;
  MeshTransport* triKD_mesh;

  void build_vrip_accelerators();
  vector<Pnt3> pntdir;  // normalized point values for vripping
  vector<float> pntmag; // and the magnitudes that we divided out
  vector< vector< vector< int> > > bins;
  float minx, maxx, miny, maxy;
  float binw, binh;

  // PUSH_TRI:  Used by the mesh routines to push a triangle
  // onto the end of the tri_inds vector, doing triangle strips
  // if necessary.
  inline void PUSH_TRI(int &ov2, int &ov3, vector<int> &tri_inds,
		       const bool stripped,
		       const vector<int> &vert_inds,
		       const int v1, const int v2, const int v3);

};


#endif /* _CyraSCAN_H_ */
