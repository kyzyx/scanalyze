//############################################################
//
// CyraResLevel.h
//
// Lucas Pereira
// Thu Jul 16 16:09:38 1998
//
// Part of CyraScan.h:
//    Store range scan information from a Cyra Cyrax Beta
//    time-of-flight laser range scanner.
//
//############################################################

#ifndef _CyraResLevel_H_
#define _CyraResLevel_H_

#include <stdio.h>
#include "RigidScan.h"

class KDindtree;

// confidence  = 1 / (Std.Dev + 1)
// Assume Cyra has initial stddev of 20mm
#define CYRA_CONF_EPS 0.1
#define CYRA_DEFAULT_CONFIDENCE (CYRA_CONF_EPS / (20.0 + CYRA_CONF_EPS))

// Label the tesselation cases, based on the vertices that are
// the right-angle corner of the triangle...
//
//    0      1      2      3      4      14     23
//   ====   ====   ====   ====   ====   ====   ====
//   2  4   2\ 4   2--4   2 /4   2--4   2--4   2--4
//          | \    | /     / |    \ |   |\ |   | /|
//   1  3   1__3   1/ 3   1__3   1 \3   1_\3   1/_3

typedef enum {
  TESS0=0,
  TESS1,
  TESS2,
  TESS3,
  TESS4,
  TESS14,
  TESS23,
} CyraTess;

typedef enum {
  cfPOINT,
  cfBOX,
  cfMEDIAN,
  cfMEAN50
} CyraFilter;

// Store the data for each sample point
class CyraSample {
public:
  Pnt3  vtx;
  short nrm[3];
  int   intensity;
  float confidence;	// = 0 for missing data.  Otherwise,
  // consider stddev as (1/confidence - 1), or something like that


  // Useful functions
  CyraSample(void) {
    this->zero();
  }
  ~CyraSample(void) {
    // do nothing
  }

  // Zero the sample, setting default values for an invalid sample
  void zero(void);

  // Interpolate two other samples....
  void interp(CyraSample &a, CyraSample &b);

  // Interpolate three other samples, weighted .25, .5, .25...
  void interp(CyraSample &a, CyraSample &b, CyraSample &c);

  // Interpolate arbitrary number of samples, weighted evenly...
  void interp(int nsamps, CyraSample *samps[]);

  // Check if a sample is valid (e.g. nonzero)
  bool isValid(void);

};

//////////////////////////////////////////////////////////////////////
// CyraResLevel  (one resolution level for a cyra scan)
//////////////////////////////////////////////////////////////////////

class CyraResLevel {
  int width;		// Number of columns
  int height;		// number of rows
  int numpoints;	// Number of valid points
  int numtris;		// Number of valid tris
  vector<CyraSample> points;   // array size: width*height
  vector<CyraTess>   tris;     // array size: (width-1) * (height-1)
  Pnt3 origin;    	// The origin of the scanner's coordinate system.

  // KDtree stuff for mesh alignment
  bool               isDirty_cache;  // Have contents of memory changed?
  vector<Pnt3>       cachedPoints;    // Contiguous array of valid points
  vector<short>      cachedNorms;     // Contiguous array of valid norms
  vector<bool>       cachedBoundary;  // Contiguous array of valid boundary flags
  KDindtree          *kdtree;        // kdtree points into cachedPoints


public:
  CyraResLevel(void);
  ~CyraResLevel(void);

  typedef RigidScan::ColorSource ColorSource;

  // perVertex: colors and normals for every vertex (not every 3)
  // stripped: triangle strips instead of triangles
  // color: one of the above enum values
  // colorsize: # of bytes for color: 1 = intensity,
  //              2 = intensity+alpha, 3 = rgb, 4 = rgba
  virtual MeshTransport* mesh(bool         perVertex,
			      bool         stripped,
			      ColorSource  color,
			      int          colorSize);


  bool ReadPts(const crope &fname);	// Read a .pts file
  bool WritePts(const crope &fname);	// Write a .pts file
  bool grazing(Pnt3 v1, Pnt3 v2, Pnt3 v3);  // detects grazing tris

  int num_vertices(void);
  int num_tris(void);

  void growBBox(Bbox &bbox);
  void flipNormals (void);

  bool filtered_copy(CyraResLevel &original, const VertexFilter& filter);
  bool filter_inplace(const VertexFilter& filter);

  // for ICP...
  void create_kdtree(void);
  void subsample_points(float rate, vector<Pnt3> &p, vector<Pnt3> &n);
  bool closest_point(const Pnt3 &p, const Pnt3 &n,
		     Pnt3 &cp, Pnt3 &cn,
		     float thr = 1e33, bool bdry_ok = 0);

friend class CyraScan;

  bool Subsample(CyraResLevel &original, int m, int n, CyraFilter filter);
private:
  // Helper functions
  void CalcNormals(void);
  bool PointFilter(CyraResLevel &original, int m, int n);
  bool Mean50Filter(CyraResLevel &original, int m, int n);

  bool colorMesh (vector<uchar>& colors,
		  bool           perVertex,
		  ColorSource    source,
		  int            colorSize);

  // Access functions for the arrays
  inline CyraSample &point(int x, int y);
  inline CyraTess   &tri(  int x, int y);
  inline const CyraSample &point(int x, int y) const;
  inline const CyraTess   &tri(  int x, int y) const;
};

//////////////////////////////////////////////////////////////////////
// Access inline functions
// Automatically handle the indexing into the points and tris
// arrays, and do error checking and all that cool stuff
//////////////////////////////////////////////////////////////////////
inline CyraSample &CyraResLevel::point(int x, int y) {
  int index = x * height + y;
  assert(index < points.size());
  return points[index];
}

inline CyraTess &CyraResLevel::tri(int x, int y) {
  int index = x * (height-1) + y;
  assert(index < tris.size());
  return tris[index];
}

// rvalues
inline const CyraSample &CyraResLevel::point(int x, int y) const {
  int index = x * height + y;
  assert(index < points.size());
  return points[index];
}

inline const CyraTess &CyraResLevel::tri(int x, int y) const {
  int index = x * (height-1) + y;
  assert(index < tris.size());
  return tris[index];
}


/// useful access macros

// FOR_EACH_VERT
// Basically a meta-function for CyraResLevel
// Loops through valid vertices, and calls a user-defined
// function.  Inside it, each of the following variables is
// defined (with a local scope, so it won't overwrite if they
// already exist):
//      CyraSample *v  the vertex
//      int vx         x value (col) of the vert (0 to W-1)
//      int vy         y value (row) of the current vertex (0 to H-1)
//      int vindex     incrementing index (vx*H + vy)
#define FOR_EACH_VERT(userfun) \
{ \
  int vx=0, vy=0; \
  CyraSample *v; \
  int vindex=0; \
  for (vx=0; vx < this->width; vx++) { \
    for (vy=0; vy < this->height; vy++, vindex++) { \
      v = &(this->points[vindex]); \
      if (v->confidence > 0) { \
        (userfun); \
      } \
    } \
  } \
}


// FOR_EACH_TRI
// Basically a meta-function for CyraResLevel
// Identifies all the valid triangles, using the tesselation flags.
// Inside it, each of the following variables is defined (within a
// local scope, so it won't overwrite if they already exist):
//      int tv1    Triangle vertex 1 (index from 0 to (WxH-W-H-1)
//      int tv2    Triangle vertex 2 (index from 0 to (WxH-W-H-1)
//      int tv3    Triangle vertex 3 (index from 0 to (WxH-W-H-1)
//      int tx     x value of the current square (0 to width-2)
//      int ty     y value of the current square (0 to height-2)
//      int tindex incrementing index (tx*(H-1)+ty)
#define FOR_EACH_TRI(userfun) \
{ \
  int tv1=0, tv2=0, tv3=0; \
  int tx=0, ty=0; \
  int tindex=0; \
  int v1=0, v2=0, v3=0, v4=0; \
  for (tx=0; tx < this->width-1; tx++) { \
    v1 = tx*(this->height) - 1; \
    v2 = v1 + 1; \
    v3 = v1 + this->height; \
    v4 = v3+1; \
    for (ty=0; ty < this->height-1; ty++, tindex++) { \
      v1++; v2++; v3++; v4++; \
      switch(this->tris[tindex]) { \
      case TESS0: \
	break; \
      case TESS1: \
	tv1= v1; tv2= v3; tv3= v2; (userfun); break; \
      case TESS2: \
	tv1= v1; tv2= v4; tv3= v2; (userfun); break; \
      case TESS3: \
	tv1= v1; tv2= v3; tv3= v4; (userfun); break; \
      case TESS4: \
	tv1= v3; tv2= v4; tv3= v2; (userfun); break; \
      case TESS14: \
	tv1= v1; tv2= v3; tv3= v2; (userfun); \
	tv1= v3; tv2= v4; tv3= v2; (userfun); break; \
      case TESS23: \
	tv1= v1; tv2= v3; tv3= v4; (userfun); \
	tv1= v1; tv2= v4; tv3= v2; (userfun); break; \
      default: \
	cerr << "Error: Unrecognized triangle tesselation flag " \
             << this->tris[tindex] << " at grid " \
	     << tx << ", " << ty << "." << endl; \
	break; \
      } \
    } \
  } \
}


#endif // CyraResLevel.h
