//############################################################
//
// TriMeshUtils.h
//
// Kari Pulli
// Thu Jul  9 15:50:50 PDT 1998
//
// Utility functions for triangle meshes that are represented
// as list of vertices and list of indices that define the
// mesh connectivity.
//
//############################################################

#ifndef _TRIMESHUTILS_H_
#define _TRIMESHUTILS_H_

#include <vector.h>
#include "Pnt3.h"

// calculate vertex normals by averaging from triangle
// normals (obtained from cross products)
// possibly weighted with triangle areas
void
getVertexNormals(const vector<Pnt3> &vtx,
		 const vector<int>  &tri,
		 bool                strips,
		 vector<short>      &nrm,
		 int useArea = 0);

void
pushNormalAsShorts (vector<short>& nrms, Pnt3 n);

void
pushNormalAsPnt3 (vector<Pnt3>& nrms, short* n, int i);

// STL Update
void
pushNormalAsPnt3 (vector<Pnt3>& nrms, vector<short>::iterator n, int i);


// find the median (or percentile) edge length
float
median_edge_length(vector<Pnt3> &vtx,
		   vector<int>  &tri,
		   bool          strips = false,
		   int           percentile = 50);

// find the median edge length
// if a triangle has an edge that's longer than
// factor times the median (or percentile), remove the triangle
void
remove_stepedges(vector<Pnt3> &vtx,
		 vector<int>  &tri,
		 int           factor,
		 int           percentile = 50,
		 bool          strips = false);

// check whether some vertices in vtx are not being used
// in tri
// if so, remove them and also adjust the tri indices
void
remove_unused_vtxs(vector<Pnt3> &vtx,
		   vector<int>  &tri);

// count #tris in tstrip
int
count_tris(const vector<int> &strips);

void
tris_to_strips(int numvertices,
	       const vector<int>& tris,
	       vector<int>& tstripinds,
	       const char* name = NULL);

void
strips_to_tris(const vector<int> &tstripinds,
	       vector<int>& tris, int nTris = 0);

void
flip_tris(vector<int> &inds, bool stripped = 0);

// assume bdry has the right size and has been initialized with
// zeroes
void
mark_boundary_verts(vector<char> &bdry,
		    const vector<int> &tris);

void
distance_from_boundary(vector<float> &distances,
		       const vector<Pnt3> &pnts,
		       const vector<int> &tris);

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

typedef enum {
  PLACE_ENDPOINTS, PLACE_ENDORMID, PLACE_LINE, PLACE_OPTIMAL
} optLevelT;

// simplify a mesh using Michael Garlands qslim package
void
quadric_simplify(const vector<Pnt3> &vtx_in,
		 const vector<int>  &tri_in,
		 vector<Pnt3> &vtx_out,
		 vector<int>  &tri_out,
		 int           goal,
		 int           optLevel,
		 float         errLevel,
		 float         boundWeight);

typedef unsigned char uchar;


// write geometry/topology data, representing triangle mesh, as .ply file
// supports various combinations of per-vertex data besides position;
// others are possible if useful.
//
// Any form of write_ply_file that takes intensity can also take
// true-color data with 3 or 4 bytes per vertex, and the first 3 bytes per
// vertex will be written out as RGB components.  It's not currently
// supported to write both intensity and RGB color but it could be added
// in the future if useful.

void
write_ply_file(const char *fname,
	       const vector<Pnt3> &vtx,
	       const vector<int> &tris, bool strips);

void
write_ply_file(const char *fname,
	       const vector<Pnt3> &vtx,
	       const vector<int> &tris, bool strips,
	       const vector<float> &confidence);

void
write_ply_file(const char *fname,
	       const vector<Pnt3> &vtx,
	       const vector<int> &tris, bool strips,
	       const vector<uchar> &intensity);

void
write_ply_file(const char *fname,
	       const vector<Pnt3> &vtx,
	       const vector<int> &tris, bool strips,
	       const vector<Pnt3> &nrm);

void
write_ply_file(const char *fname,
	       const vector<Pnt3> &vtx,
	       const vector<int> &tris, bool strips,
	       const vector<Pnt3> &nrm,
	       const vector<uchar> &intensity);

void
write_ply_file(const char *fname,
	       const vector<Pnt3> &vtx,
	       const vector<int> &tris, bool strips,
	       const vector<Pnt3> &nrm,
	       const vector<float> &confidence);

void
write_ply_file(const char *fname,
	       const vector<Pnt3> &vtx,
	       const vector<int> &tris, bool strips,
	       const vector<uchar> &intensity,
	       const vector<float> &confidence);

void
write_ply_file(const char *fname,
	       const vector<Pnt3> &vtx,
	       const vector<int> &tris, bool strips,
	       const vector<Pnt3> &nrm,
	       const vector<uchar> &intensity,
	       const vector<float> &confidence);


#endif  // TRIMESHUTILS_H_
