#ifndef _MESH_
#define _MESH_

#include <limits.h>
#include <vector>
#include "ResolutionCtrl.h"
#include "defines.h"
#include "Bbox.h"
#include <set>
#include <cassert>

typedef set<int, less<int> > TriList; // List of triangles attached to a single vtx
typedef set<int, less<int> >::iterator TriListI;

class Mesh {
private:

  vector<Pnt3> vtx;
  vector<short> nrm;
  vector<Pnt3> orig_vtx;    // Used by JED during fairing
                            // we want to constrain the surface
                            // to lie near the original noisy surface

  vector<TriList> vtxTris;  // The list of tris attached to each vtx

  vec3uc *vertMatDiff;
  vec2f *texture;
  float *vertIntensity;
  float *vertConfidence;
  int hasVertNormals;
  vector<char> bdry; // 1 for boundary vtx, used for registration

  char texFileName[PATH_MAX];

  vector<int> tris;
  /* store voxels each tri is from - for display voxel feature */
  vector<float> fromVoxels;
  bool hasVoxels;

  vector<int> tstrips;
  int numTris;
  vec3uc *triMatDiff;

  Bbox bbox;

  Pnt3  center;
  float radius;

  void subsample_points(float rate, vector<Pnt3> &pts);
  void subsample_points(float rate, vector<Pnt3> &pts,
			vector<Pnt3> &nrms);
  bool subsample_points(int n, vector<Pnt3> &pts);
  bool subsample_points(int n, vector<Pnt3> &pts,
			vector<Pnt3> &nrms);
  void remove_unused_vtxs(void);
  void init (void);

  void computeBBox(); // private; call updateScale() instead

public:

  Mesh();
  Mesh (const vector<Pnt3>& _vtx, const vector<int>& _tris);
  ~Mesh();

  void flipNormals();
  void initNormals(int useArea = FALSE);

  vector<int>& getTris();
  vector<int>& getTstrips();
  const Pnt3& Vtx (int v) { return vtx[v]; }

  void simulateFaceNormals (vector<short>& facen);

  void freeTris (void);
  void freeTStrips (void);

  void updateScale();
  void showBBox(void)
    { cout << bbox.min() << " " << bbox.max() << endl; }

  bool bNeedsSave;

  int  num_tris(void);
  int  num_verts(void)  { return vtx.size(); }

  void mark_boundary_verts(void);

  // routines for smoothing - JED
  void dequantizationSmoothing(double maxDisplacement);
  void restoreOrigVerts();
  void saveOrigVerts();
  void calcTriLists();
  // -----------

  Mesh *Decimate (int numFaces, int optLevel,
		  float errLevel, float boundWeight,
		  ResolutionCtrl::Decimator dec = ResolutionCtrl::decQslim);
  void remove_stepedges(int percentile = 50, int factor = 4);

  int  readPlyFile (const char *filename);
  int  writePlyFile (const char *filename, int useColorNotTexture,
		     int writeNormals);

  void addTri(int v1, int v2, int v3)
    {
      assert(tris.size()%3 == 0);
      int vs = vtx.size();
      if (v1 < 0 || v1 >= vs || v2 < 0 || v2 >= vs || v3 < 0 || v3 >= vs) {
	cerr << "\n\nRed alert: invalid triangle "
	     << v1 << " " << v2 << " "  << v3
	     << " (tri " << tris.size()/3
	     << ", valid range=0.." << vs-1 << ")\n" << endl;
      } else {
	tris.push_back(v1);        /* tris is a vector of ints */
	tris.push_back(v2);
	tris.push_back(v3);
      }
    }

  void addVoxelInfo(int v1, int v2, int v3,
		float fromVoxel1, float fromVoxel2, float fromVoxel3)
    {
      assert(tris.size()%3 == 0);
      int vs = vtx.size();
      if (v1 < 0 || v1 >= vs || v2 < 0 || v2 >= vs || v3 < 0 || v3 >= vs) {
	cerr << "\n\nRed alert: invalid triangle "
	     << v1 << " " << v2 << " "  << v3
	     << " (tri " << tris.size()/3
	     << ", valid range=0.." << vs-1 << ")\n" << endl;
      } else {
      	/* store voxels in fromVoxels - for display voxel feature */
	fromVoxels.push_back(fromVoxel1);
	fromVoxels.push_back(fromVoxel2);
	fromVoxels.push_back(fromVoxel3);
      }
    }

  void copyTriFrom (Mesh *mSrc, int src, int dst);
  void copyVertFrom (Mesh *mSrc, int src, int dst);

  friend class RangeGrid;
  friend class GenericScan;

  // added by wsh for tweaking violins
  void Warp();
};

/* typedef: optLevelT
 * -------
 * Used to select the level of optimization for Decimate().
 * Refers to the placement of points after pair contraction.
 * The options:
 *   - PLACE_ENDPOINTS: consider only the ends of the line segment
 *   - PLACE_ENDORMID:  consider ends or mids of the line segment
 *   - PLACE_LINE:      find the best position on the line segment
 *   - PLACE_OPTIMAL:   find the best position anywhere in space
 */

//typedef enum {
//  PLACE_ENDPOINTS, PLACE_ENDORMID, PLACE_LINE, PLACE_OPTIMAL
//} optLevelT;


#endif
