//############################################################
//
// TriMeshUtils.cc
//
// Utility functions for triangle meshes that are represented
// as list of vertices and list of indices that define the
// mesh connectivity.
//
//############################################################

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "TriMeshUtils.h"
#include "Median.h"
#include "ply++.h"
#include "plvGlobals.h"
#include "Progress.h"
#include "Timer.h"



// calculate vertex normals by averaging from triangle
// normals (obtained from cross products)
// possibly weighted with triangle areas
void
getVertexNormals(const vector<Pnt3> &vtx,
		 const vector<int>  &tri,
		 bool                strips,
		 vector<short>      &nrm_s,
		 int useArea)
{
  vector<Pnt3> nrm (vtx.size(), Pnt3());

  int ntris = tri.size();
  if (!useArea) {
    if (strips) {
      bool flip = false;
      vector<int>::const_iterator i;
      for (i = tri.begin(); i < tri.end(); i++) {
	if (*(i+2) == -1) {
	  // end of strip
	  i += 2;
	  flip = false;
	  continue;
	}

	int b, c;
	if (flip) {
	  b = *(i+2); c = *(i+1); flip = false;
	} else {
	  b = *(i+1); c = *(i+2); flip = true;
	}

	Pnt3 norm = normal(vtx[*i], vtx[b], vtx[c]);
	nrm[*i] += norm;
	nrm[b]  += norm;
	nrm[c]  += norm;
      }
    } else {
      for (int i = 0; i < ntris; i += 3) {
	Pnt3 norm = normal(vtx[tri[i+0]],
			   vtx[tri[i+1]],
			   vtx[tri[i+2]]);
	nrm[tri[i+0]] += norm;
	nrm[tri[i+1]] += norm;
	nrm[tri[i+2]] += norm;
      }
    }

  } else { // do use area norm

    if (strips) {
      bool flip = false;
      for (int i = 0; i < ntris; i++) {
	if (tri[i+2] == -1) {
	  // end of strip
	  i += 2;
	  flip = false;
	  continue;
	}

	int a, b, c;
	if (flip) {
	  a = tri[i]; b = tri[i+2]; c = tri[i+1]; flip = false;
	} else {
	  a = tri[i]; b = tri[i+1]; c = tri[i+2]; flip = true;
	}

	//float area = .5 * cross(vtx[a], vtx[b], vtx[c]).norm();
	//Pnt3 areaNorm = normal(vtx[a], vtx[b], vtx[c]);
	//areaNorm *= area;
	Pnt3 areaNorm = cross(vtx[a], vtx[b], vtx[c]);
	nrm[a] += areaNorm;
	nrm[b] += areaNorm;
	nrm[c] += areaNorm;
      }
    } else {
      for (int i = 0; i < ntris; i += 3) {

	/*
	float area = .5 * cross(vtx[tri[i+0]], vtx[tri[i+1]],
				vtx[tri[i+2]]).norm();
	Pnt3 areaNorm = normal(vtx[tri[i+0]],
			       vtx[tri[i+1]],
			       vtx[tri[i+2]]);
	areaNorm *= area;
	*/
	Pnt3 areaNorm = cross(vtx[tri[i+0]],
			      vtx[tri[i+1]],
			      vtx[tri[i+2]]);
	nrm[tri[i+0]] += areaNorm;
	nrm[tri[i+1]] += areaNorm;
	nrm[tri[i+2]] += areaNorm;
      }
    }
  }

  nrm_s.clear();
  nrm_s.reserve (3 * vtx.size());
// STL Update
  for (vector<Pnt3>::iterator n = nrm.begin(); n != nrm.end(); n++) {
    n->set_norm(32767.0);
    nrm_s.push_back (short(n->operator[](0)));
    nrm_s.push_back (short(n->operator[](1)));
    nrm_s.push_back (short(n->operator[](2)));
  }
}


float
median_edge_length(vector<Pnt3> &vtx,
		   vector<int>  &tri,
		   bool          strips,
		   int           percentile)
{
  int n = tri.size();
  if (n < 3) return 0.0;

  Median<float> med(percentile, n);

  if (strips) {
    int *end = &tri[n];
    // reuse the "inner edge" length in the strip
    float inner_edge = dist2(vtx[0], vtx[1]);
    for (int *i=&tri[2]; i<end; i++) {
      if (*i == -1) {
	// skip over the start part of the next strip
	int cnt = 0;
	while (cnt < 3) {
	  i++;
	  if (i >= end) goto done;
	  if (*i == -1) cnt = 0;
	  else          cnt++;
	}
	inner_edge = dist2(vtx[*(i-1)], vtx[*(i-2)]);
      }
      med += inner_edge;
      inner_edge = dist2(vtx[*(i-1)], vtx[*i]);
      med += inner_edge;
      med += dist2(vtx[*(i-2)], vtx[*i]);
    }
  } else {
    // edge lengths
    for (int i=0; i<n; i+=3) {
      med += dist2(vtx[tri[i+0]], vtx[tri[i+1]]);
      med += dist2(vtx[tri[i+2]], vtx[tri[i+1]]);
      med += dist2(vtx[tri[i+0]], vtx[tri[i+2]]);
    }
  }
 done:
  return sqrtf(med.find());
}

// find the median edge length
// if a triangle has an edge that's longer than
// factor times the median (or percentile), remove the triangle
void
remove_stepedges(vector<Pnt3> &vtx,
		 vector<int>  &tri,
		 int           factor,
		 int           percentile,
		 bool          strips)
{
  // calculate the threshold
  float thr = median_edge_length(vtx, tri, strips, percentile);
  thr *= thr * float(factor * factor);

  int n = tri.size();
  if (strips) {
    vector<int> ntri;
    ntri.reserve(n);
    bool strip_on = false;
    int cnt  = 0;
    for (int i=0; i<n; i++) {
      if (strip_on) {
	// strip is on
	bool end_found = (tri[i] == -1);
	bool end_here  = false;
	if (!end_found) {
	  // check for too long edges
	  if (cnt == 1)
	    end_here = (dist2(vtx[tri[i]], vtx[tri[i-1]]) > thr);
	  else
	    end_here = (dist2(vtx[tri[i]], vtx[tri[i-1]]) > thr ||
			dist2(vtx[tri[i]], vtx[tri[i-2]]) > thr);
	}
	if (end_found || end_here) {
	  // found an end, clean up
	  if (cnt < 3) {
	    ntri.pop_back();
	    if (cnt == 2) ntri.pop_back();
	    if (ntri.size()) assert(ntri.back() == -1);
	  } else {
	    ntri.push_back(-1);
	  }
	  strip_on = false;
	}
	if (end_here) {
	  if (cnt % 2) {
	    // cnt odd, check if can add a single triangle
	    if (i > n-4) break; // almost done, break...
	    if (tri[i+1] != -1 &&
		tri[i+2] != -1 &&
		dist2(vtx[tri[i  ]], vtx[tri[i+1]]) < thr &&
		dist2(vtx[tri[i  ]], vtx[tri[i+2]]) < thr &&
		dist2(vtx[tri[i+1]], vtx[tri[i+2]]) < thr) {
	      ntri.push_back(tri[i]);
	      ntri.push_back(tri[i+1]);
	      ntri.push_back(tri[i+2]);
	      ntri.push_back(-1);
	    }
	  } else {
	    // cnt even, restart a strip
	    strip_on = true;
	    cnt = 1;
	    ntri.push_back(tri[i]);
	  }
	}
	if (!end_found && !end_here) {
	  // continue this strip
	  ntri.push_back(tri[i]);
	  cnt++;
	}
      } else {
	// strip is not on
	if (tri[i] != -1) {
	  ntri.push_back(tri[i]);
	  strip_on = true;
	  cnt = 1;
	}
      }
      if (!strip_on) cnt = 0;
    }
    tri = ntri;

  } else {
    // could make faster by not recalculating edges!
    for (int i=n-3; i>=0; i-=3) {
      if (dist2(vtx[tri[i+0]], vtx[tri[i+1]]) > thr ||
	  dist2(vtx[tri[i+2]], vtx[tri[i+1]]) > thr ||
	  dist2(vtx[tri[i+0]], vtx[tri[i+2]]) > thr) {
	// remove the triangle
	tri[i+2] = tri.back(); tri.pop_back();
	tri[i+1] = tri.back(); tri.pop_back();
	tri[i+0] = tri.back(); tri.pop_back();
      }
    }
  }
}


// check whether some vertices in vtx are not being used in tri
// if so, remove them and also adjust the tri indices
void
remove_unused_vtxs(vector<Pnt3> &vtx,
		   vector<int>  &tri)
{
  // prepare vertex index map
  vector<int> vtx_ind(vtx.size(), -1);

  // march through the triangles
  // and mark the vertices that are actually used
  int n = tri.size();
  for (int i=0; i<n; i+=3) {
    vtx_ind[tri[i+0]] = tri[i+0];
    vtx_ind[tri[i+1]] = tri[i+1];
    vtx_ind[tri[i+2]] = tri[i+2];
  }

  // remove the vertices that were not marked,
  // also keep tab on how the indices change
  int cnt = 0;
  n = vtx.size();
  for (int i=0; i<n; i++) {
    if (vtx_ind[i] != -1) {
      vtx_ind[i] = cnt;
      vtx[cnt] = vtx[i];
      cnt++;
    }
  }
// STL Update
  vtx.erase(vtx.begin() +cnt, vtx.end());
  // march through triangles and correct the indices
  n = tri.size();
  for (int i=0; i<n; i+=3) {
    tri[i+0] = vtx_ind[tri[i+0]];
    tri[i+1] = vtx_ind[tri[i+1]];
    tri[i+2] = vtx_ind[tri[i+2]];
    assert(tri[i+0] >=0 && tri[i+0] < vtx.size());
    assert(tri[i+1] >=0 && tri[i+1] < vtx.size());
    assert(tri[i+2] >=0 && tri[i+2] < vtx.size());
  }
}


// count the number of triangles in a tstrip
int
count_tris(const vector<int> &strips)
{
  int cnt = 0;
  int i = 0;
  while (i < strips.size()) {
    // skip over the 2 first inds
    i+=2;
    while (strips[i++] != -1) {
      cnt++;
    }
  }
  return cnt;
}



struct _edge {
  int a,b;
  _edge(void) : a(0),b(0) {}
  _edge(int _a, int _b)
    {
      if (_a < _b) { a=_a; b=_b; }
      else         { b=_a; a=_b; }
    }
  void set(int _a, int _b)
    {
      if (_a < _b) { a=_a; b=_b; }
      else         { b=_a; a=_b; }
    }
  bool operator==(const _edge &e) const
    {
      return a == e.a && b == e.b;
    }
};

struct equal_edge
{
  bool operator()(const _edge &e1, const _edge &e2) const
    {
      return e1.a == e2.a && e1.b == e2.b;
    }
};

struct hash_edge
{
  size_t operator()(const _edge &e) const
    {
      return e.a * 1000000 + e.b;
    }
};


static void
remove_this_triangle(vector<int> &ts, int i)
{
  for (int j=0; j<3; j++) {
    int curr = ts[i+j]; // current neighbor
    if (curr >= 0) {
      if      (ts[curr]   == -2) continue; // nbor has been removed
      // mark the correct edge of the neighbor to
      // not have a neighbor any more
      if      (ts[curr]   == i) ts[curr]   = -1;
      else if (ts[curr+1] == i) ts[curr+1] = -1;
      else if (ts[curr+2] == i) ts[curr+2] = -1;
    }
    ts[i+j] = -2; // remove
  }
}


static int Verbose = 1;
static int Warnings = 1;

static void
old_tris_to_strips(const vector<int> &tris,
		   vector<int>& tstripinds,
		   const char* name)
{
  int nTris = tris.size();
  tstripinds.clear();
  tstripinds.reserve(1.1*nTris/3);

  // build a multimap: map edges to incident triangles
  typedef unordered_multimap<_edge,int,hash_edge,equal_edge> hmm;
  hmm edges;
  hmm::iterator edge_it;
  int i,j;
  _edge e;

  cout << "Creating triangle strips from raw triangle data..."
       << endl;
  if (Verbose)
    printf ("input: %d vertices (%d triangles)\n", nTris, nTris/3);

  // show progress graphically
  Progress progress (nTris * 3, "Create %s T-Strips", name);

  for (i = 0; i < nTris; i+=3) {  // for all tris
    if (i%300 == 0)
      progress.update (i);

    for (j = 0; j < 3; j++) {      // for all edges
      // create an edge
      e.set(tris[i+j], tris[i+(j+1)%3]);
      // store the edge and starting vertex of the triangle
      edges.insert(pair<const _edge,int>(e,i));
    }
  }


  // march through triangles
  // store for each edge the neighboring triangle
  //  -1 if neighbor doesn't exist,
  //  later -2 if the triangle is removed/processed
  vector<int> tri_nbors(nTris);
  for (i = 0; i < nTris; i+=3) {
    if (i%300 == 0)
      progress.update (i + nTris);

    for (j = 0; j < 3; j++) {
      // find all instances of current edge
      e.set(tris[i+j], tris[i+(j+1)%3]);
      pair<hmm::iterator, hmm::iterator> p = edges.equal_range(e);

      edge_it = p.first;
      // shouldn't have more than two triangles sharing this edge:
      p.first++;
      if (p.first != p.second) {
	p.first++;
	if (p.first != p.second) {
	  if (Warnings) {
	    fprintf(stderr, "More than two pointers to same "
		    "edge (%d,%d)\n", e.a, e.b);
	    while (Verbose && edge_it != p.second) {
	      int tri = (*edge_it).second;
	      fprintf(stderr,
		      "\t: offending tri: #%d (%d,%d,%d)\n",
		      tri, tris[tri+0],
		      tris[tri+1], tris[tri+2]);
	      edge_it++;
	    }
	  }
	  // non-manifold edge, no neighbors across this
	  tri_nbors[i+j] = -1;
	  continue;
	}
      }
      // there's only 1 or 2 triangles for this edge
      if ((*edge_it).second != i) {
	// if the first match is not this triangle,
	// the neighbor across this edge is the current triangle
	tri_nbors[i+j] = (*edge_it).second;
      } else {
	// the first of range is the current one
	edge_it++;
	if (edge_it == p.second) {
	  // that was the only edge, so this edge is not shared.
	  tri_nbors[i+j] = -1;
	} else {
	  // found another edge
	  // so the neighbor across this edge is now in .second
	  assert((*edge_it).first == e);
	  assert((*edge_it).second != i);
	  tri_nbors[i+j] = (*edge_it).second;
	}
      }
    }
  }

  int nStrips = 0; //for stat-keeping

  // start removing triangles from the tri_nbors
  // and build the t-strips
  int nNbors = tri_nbors.size();
  for (i = 0; i < nNbors; i += 3) {
    if (i%300 == 0)
      progress.update (2* nTris + i);

    if (tri_nbors[i] == -2) continue; // has been removed
    // try finding a neighbor
    int nbor = -1;
    for (j = 0; j < 3; j++) {
      if (tri_nbors[i+j] >= 0) {
	nbor = j;
	break;
      }
    }
    if (nbor == -1) {
      // single triangle
      tstripinds.push_back(tris[i+0]);
      tstripinds.push_back(tris[i+1]);
      tstripinds.push_back(tris[i+2]);
    } else {
      // initialize the tstrip
      tstripinds.push_back(tris[i+(nbor+2)%3]);
      tstripinds.push_back(tris[i+(nbor  )  ]);
      tstripinds.push_back(tris[i+(nbor+4)%3]);

      int prev = i;
      int dir = 1;
      int curr = tri_nbors[prev+nbor];
      while (curr != -1) {
	// find from which nbor we came to curr
	if      (tri_nbors[curr]   == prev) nbor = 0;
	else if (tri_nbors[curr+1] == prev) nbor = 1;
	else if (tri_nbors[curr+2] == prev) nbor = 2;
	else printf ("Warning: really lost!\n");

	remove_this_triangle(tri_nbors, prev);
	tstripinds.push_back(tris[curr+(nbor+2)%3]);
	// find next nbor
	nbor = (nbor+1+dir)%3;
	dir ^= 1;
	prev = curr;
	curr = tri_nbors[prev+nbor];
      }
      remove_this_triangle(tri_nbors, prev);
    }

    ++nStrips;   //just for stat-keeping
    tstripinds.push_back(-1);
  }

  if (Verbose)
    printf ("output: %ld vertices (%d strips); avg. length %.1f\n",
	  tstripinds.size()-nStrips, nStrips,
	  ((float)tstripinds.size()/nStrips) - 3);

  printf ("Done.\n");
}








typedef unsigned* adjacentfacelist;

adjacentfacelist*
TriMesh_FindAdjacentFaces (int numvertices,
			   const vector<int>& faces,
			   int*& numadjacentfaces)
{
  cout << "  Computing vtx->face mappings..." << flush;
  int numfaces = faces.size();
  assert (numvertices != 0);
  assert (numfaces != 0);

  // Step I - compute numadjacentfaces
  numadjacentfaces = new int[numvertices];
  memset(numadjacentfaces, 0, numvertices*sizeof(int));

  for (int i = 0; i < numfaces; i++) {
    numadjacentfaces[faces[i]]++;
  }

  // allocate one chunk of memory for all adjacent face lists
  // total number of adjacent faces is numfaces
  unsigned* adjacentfacedata = new unsigned [numfaces];
  // this pointer will be incremented as needed but adjacentfaces[0]
  // will always point to the beginning so it can later be freed

  // Step II - compute the actual vertex->tri lists...
  adjacentfacelist* adjacentfaces = new adjacentfacelist[numvertices];
  for (int i = 0; i < numvertices; i++) {
    adjacentfaces[i] = adjacentfacedata;
    adjacentfacedata += numadjacentfaces[i];

    //for (int j=0; j<numadjacentfaces[i]; j++)
    //  adjacentfaces[i][j] = numfaces;
  }

  assert (adjacentfacedata == adjacentfaces[0] + numfaces);
  for (unsigned* afdp = adjacentfaces[0]; afdp < adjacentfacedata; afdp++)
    *afdp = numfaces;

  for (int i = 0; i < numfaces; i++) {
    unsigned *p = adjacentfaces[faces[i]];
    while (*p != numfaces)
      p++;
    // snap to nearest multiple of 3, the start of tri data for that face
    *p = (i/3) * 3;
  }

  return adjacentfaces;
}




#define FOR_EACH_VERTEX_OF_FACE(i,j) \
  for (int jtmp = 0, j = faces[i]; \
       jtmp < 3; \
       jtmp++, j = faces[i + jtmp])

#define FOR_EACH_ADJACENT_FACE(i,j) \
  for (int jtmp=0, j = adjacentfaces[i][0]; \
       jtmp < numadjacentfaces[i]; \
       jtmp++, j = adjacentfaces[i][jtmp])

static bool* done = NULL;
static unsigned* stripsp = NULL;
static int* numadjacentfaces = NULL;
static adjacentfacelist* adjacentfaces = NULL;
static const int* faces = NULL;
static int nstrips = 0;
static int nEvilTriangles;



// Figure out the next triangle we're headed for...
static inline int
Tstrip_Next_Tri(unsigned tri, unsigned v1, unsigned v2)
{
  FOR_EACH_ADJACENT_FACE(v1, f1) {
    if ((f1 == tri) || done[f1/3])
      continue;
    FOR_EACH_ADJACENT_FACE(v2, f2) {
      if ((f2 == tri) || done[f2/3])
	continue;
      if (f1 == f2)
	return f1;
    }
  }

  return -1;
}

// Build a whole strip of triangles, as long as possible...
static void Tstrip_Crawl(unsigned v1, unsigned v2, unsigned v3,
			 unsigned next)
{
  // Insert the first tri...
  *stripsp++ = v1;
  *stripsp++ = v2;
  *stripsp++ = v3;

  unsigned vlast1 = v3;
  unsigned vlast2 = v2;

  bool shouldbeflipped = true;

  // Main loop...
  do {
    // Find the next vertex
    int vnext;
    FOR_EACH_VERTEX_OF_FACE(next,vnext_tmp) {
      if ((vnext_tmp == vlast1) || (vnext_tmp == vlast2))
	continue;
      vnext = vnext_tmp;
      break;
    }

    bool thisflipped = true;
    if ((faces[next+0] == vlast2) &&
	(faces[next+1] == vlast1) &&
	(faces[next+2] == vnext))
      thisflipped = false;
    if ((faces[next+2] == vlast2) &&
	(faces[next+0] == vlast1) &&
	(faces[next+1] == vnext))
      thisflipped = false;
    if ((faces[next+1] == vlast2) &&
	(faces[next+2] == vlast1) &&
	(faces[next+0] == vnext))
      thisflipped = false;

    if (thisflipped != shouldbeflipped) {
      if (nEvilTriangles-- > 0) {
	cerr << "Tstrip generation: inconsistent triangle orientation, "
	     << "tri " << next << endl;
      }
      goto bail;
    }


    // Record it

    *stripsp++ = vnext;
    vlast2 = vlast1;
    vlast1 = vnext;
    done[next/3] = true;
    shouldbeflipped = !shouldbeflipped;

    // Try to find the next tri
  } while ((next = Tstrip_Next_Tri(next, vlast1, vlast2)) != -1);

 bail:
  // OK, done.  Mark end of strip
  *stripsp++ = -1;
  ++nstrips;
}

// Begin a tstrip, starting with triangle tri
// tri is ordinal, not index (counts by 1)
static void Tstrip_Bootstrap(unsigned tri)
{
  done[tri] = true;

  // Find two vertices with which to start.
  // We do only a bit of lookahead, starting with vertices that will
  // let us form a strip of length at least 2...

  tri *= 3;
  unsigned vert1 = faces[tri];
  unsigned vert2 = faces[tri+1];
  unsigned vert3 = faces[tri+2];

  // Try vertices 1 and 2...
  int nextface = Tstrip_Next_Tri(tri, vert1, vert2);
  if (nextface != -1) {
    Tstrip_Crawl(vert3, vert1, vert2, nextface);
    return;
  }

  // Try vertices 2 and 3...
  nextface = Tstrip_Next_Tri(tri, vert2, vert3);
  if (nextface != -1) {
    Tstrip_Crawl(vert1, vert2, vert3, nextface);
    return;
  }

  // Try vertices 3 and 1...
  nextface = Tstrip_Next_Tri(tri, vert3, vert1);
  if (nextface != -1) {
    Tstrip_Crawl(vert2, vert3, vert1, nextface);
    return;
  }

  // OK, nothing we can do. Do a single-triangle-long tstrip.
  *stripsp++ = vert1;
  *stripsp++ = vert2;
  *stripsp++ = vert3;
  *stripsp++ = -1;
  ++nstrips;
}


static unsigned *Build_Tstrips(int numvertices,
			       const vector<int>& tris,
			       unsigned*& endstrips,
			       int& outNstrips)
{
  adjacentfaces = TriMesh_FindAdjacentFaces
    (numvertices, tris, numadjacentfaces);

  cout << " stripping... " << flush;
  int numfaces = tris.size() / 3;

  // Allocate more than enough memory
  unsigned* strips = new unsigned[4*numfaces+1];
  stripsp = strips;
  nEvilTriangles = 3;

  // Allocate array to record what triangles we've already done
  done = new bool[numfaces];
  memset(done, 0, numfaces*sizeof(bool));
  faces = &tris[0];
  nstrips = 0;

  // Build the tstrips
  for (int i = 0; i < numfaces; i++) {
    if (!done[i])
      Tstrip_Bootstrap (i);
  }
  endstrips = stripsp;
  outNstrips = nstrips;

  if (nEvilTriangles < 0) {
    cerr << "And there were " << -nEvilTriangles
	 << " more evil triangles for which no warnings were printed."
	 << endl << endl;
  }

  // cleanup global arrays
  delete [] done; done = NULL;
  delete [] numadjacentfaces; numadjacentfaces = NULL;
  delete [] adjacentfaces[0]; // ptr to one chunk of data for all
  delete [] adjacentfaces; adjacentfaces = NULL;
  stripsp = NULL;
  faces = NULL;

  cout << " done." << endl;
  return strips;
}


void
tris_to_strips(int numvertices,
	       const vector<int>& tris,
	       vector<int>& tstripinds,
	       const char* name)
{
#if 0
  old_tris_to_strips (tris, tstripinds, name);
#else
  cout << "Tstripping " << tris.size()/3 << " triangles ("
       << tris.size() << " vertices)..." << endl;

  tstripinds.clear();
  unsigned *strips, *end;
  int nStrips = 0;

  if (tris.size()) {
    strips = Build_Tstrips (numvertices, tris, end, nStrips);

    // this is a hair faster...
    //tstripinds.reserve (end-strips);
    //memcpy (&tstripinds[0], strips, 4*(end-strips));

    // but this is legal :)
    //#ifdef __STL_MEMBER_TEMPLATES
#if 0
    tstripinds.assign (strips, end);
#else
    // as long as your compiler supports member templates :(
    tstripinds.reserve (end - strips);
    while (strips < end)
      tstripinds.push_back (*strips++);
#endif
  }

  cout << "Tstrip results: " << nstrips << " strips ("
       << tstripinds.size() - nStrips << " vertices, avg. length "
       << ((float)tstripinds.size()/nStrips) - 3 << ")." << endl;
#endif
}





void
strips_to_tris(const vector<int> &tstrips,
	       vector<int>& tris, int nTris)
{
  if (!tstrips.size())
     return;

  assert(tstrips.back() == -1);
  tris.clear();
  if (!nTris)
    nTris = tstrips.size() * 3;  // estimate
  tris.reserve (nTris);

  vector<int>::const_iterator vert;
  for (vert = tstrips.begin(); vert != tstrips.end(); vert++) {
    while (*vert == -1) { // handle 0-length strips
      ++vert;
      if (vert == tstrips.end()) break;
    }
    if (vert == tstrips.end()) break;

    vert += 2;   //we'll look backwards at these

    int dir = 0;
    while (*vert != -1)  {
      tris.push_back(vert[-2 + dir]);
      tris.push_back(vert[-1 - dir]);
      tris.push_back(vert[0]);
      vert++;
      dir ^= 1;
    }
  }
}


void
flip_tris(vector<int> &inds, bool stripped)
{
  int n = inds.size();

  if (stripped) {
    int i=0;
    while (i<n) {
      i++; // skip the first one
      // after that, swap all complete pairs until -1
      while (1) {
	int tmp = inds[i++];
	// after break, inds[i-1] == -1
	if (tmp == -1) break;
	if (inds[i] == -1) { i++; break; }
	inds[i-1] = inds[i];
	inds[i++] = tmp;
      }
    }
  } else {
    for (int i = 0; i < n; i += 3)
      swap(inds[i], inds[i+1]);
  }
}



#define BEFORE(mod, var) \
  ((mod == 0) ? tris[var + 2] : tris[var - 1])

#define AFTER(mod, var) \
  ((mod == 2) ? tris[var - 2] : tris[var + 1])

// sort a ring of neighbors

static void
sort_nbors(int *nbors, int n, const  vector<int> &tris)
{
  for (int i=0; i<n; i++) {
    int vb = BEFORE(nbors[i]%3, nbors[i]);
    for (int j=i+2; j<n; j++) {
      if (AFTER(nbors[j]%3, nbors[j]) == vb) {
	// swap
	int tmp    = nbors[i+1];
	nbors[i+1] = nbors[j];
	nbors[j]   = tmp;
	break;
      }
    }
  }
}

static void
nbors_tris(int    n_vtx,
	   const  vector<int> &tris,
	   int*  &n_nbors, // number of neighbors for vtx i
	   int** &nbors,   // the actual neighbors
	   bool   sorted = false)
{
  // Step I - compute numadjacentfaces
  n_nbors = new int[n_vtx];
  memset(n_nbors, 0, n_vtx*sizeof(int));

  int n_tri_inds = tris.size();
  for (int i=0; i<n_tri_inds; i++) {
    n_nbors[tris[i]]++;
  }

  // allocate one chunk of memory for all neighbor triangle lists
  // total number of neighboring triangles is n_tri_inds
  int *data = new int[n_tri_inds];
  // this pointer will be incremented as needed but nbors[0]
  // will always point to the beginning so it can later be freed

  // Step II - compute the actual vertex->tri lists...
  nbors = new int*[n_vtx];
  for (int i=0; i<n_vtx; i++) {
    nbors[i] = data;
    data += n_nbors[i];
  }

  assert (data == nbors[0] + n_tri_inds);
  // mark as empty
  int *p;
  for (p = nbors[0]; p<data; p++) {
    *p = -1;
  }

  for (int i=0; i<n_tri_inds; i++) {
    int *p = nbors[tris[i]];
    while (*p != -1) p++;
    *p = i;
  }

  if (sorted) {
    for (int i=0; i<n_vtx; i++) {
      sort_nbors(nbors[i], n_nbors[i], tris);
    }
  }
}



// assume bdry has the right size and has been initialized with
// zeroes
void
mark_boundary_verts(vector<char> &bdry,
		    const vector<int> &tris)
{
  cout << "Marking boundary vertices ... " << flush;
  int nv = bdry.size();
  int *n_nbors, **nbors;
  nbors_tris(nv, tris, n_nbors, nbors, true);

  // for each vertex, try to find a full loop
  // around it, if can't its boundary
  int va, vb; // vertex after, vertex before
  for (int i=0; i<nv; i++) {
    if (n_nbors[i] <= 1) {
      bdry[i] = 1;
      continue;
    }
    int jend = n_nbors[i] - 1;
    for (int j=0; j<jend; j++) {
      // which vtx comes before this (corner j)?
      int k = nbors[i][j];
      vb = BEFORE(k % 3, k);
      // which vtx comes after this (corner j+1)?
      k = nbors[i][j+1];
      va = AFTER(k % 3, k);
      if (va != vb) {
	// not a continuous neighbor chain
	bdry[i] = 1;
	break;
      }
    }
    if (bdry[i] == 0) {
      // check the last possible link (from end to start)
      // which vtx comes before this (corner jend)?
      int k = nbors[i][jend];
      vb = BEFORE(k % 3, k);
      // which vtx comes after this (corner jstart)?
      k = nbors[i][0];
      va = AFTER(k % 3, k);
      if (va != vb) {
	// not a continuous neighbor chain
	bdry[i] = 1;
      }
    }
  }

  // cleanup
  delete[] n_nbors;
  delete[] nbors[0]; // ptr to one chunk of data for all
  delete[] nbors;
  cout << "done" << endl;
}


typedef vector< vector<int> > vecvec;
typedef vector<int>::const_iterator vicit;
static void
build_nbor_map(vecvec &nlist, const vector<int> &tris)
{
  // build an edge set
  typedef unordered_set<_edge,hash_edge,equal_edge> hs;
  hs edges;
  int n = tris.size();
  for (int i=0; i<n; i+=3) {
    edges.insert(_edge(tris[i+0], tris[i+1]));
    edges.insert(_edge(tris[i+1], tris[i+2]));
    edges.insert(_edge(tris[i+2], tris[i+0]));
  }

  // for every edge, create an entry to neighborhood map
  hs::const_iterator edge_it;
  for (edge_it = edges.begin(); edge_it != edges.end(); edge_it++) {
    nlist[(*edge_it).a].push_back((*edge_it).b);
    nlist[(*edge_it).b].push_back((*edge_it).a);
  }
}

#if 0

// assume bdry has the right size and has been initialized with
// zeroes
void
distance_from_boundary(vector<float> &distances,
		       const vector<Pnt3> &pnts,
		       const vector<int> &tris)
{
  TIMER(dist);

  int n = pnts.size();

  // find boundary verts
  vector<char> bdry(n, 0);
  mark_boundary_verts(bdry, tris);

  // initialize distance vector
  distances.clear();
  distances.insert(distances.begin(), n, 1e33);
  int nbdry = 0;
  for (int i=0; i<n; i++) {
    if (bdry[i]) { distances[i] = 0.0; nbdry++; }
  }

  // build neighborhood map
  vecvec nmap(n);
  build_nbor_map(nmap, tris);

  // find the neighbors of bdry vertices
  hash_set<int,hash<int>,equal_to<int> > prevset, workset, nextset;
  hash_set<int,hash<int>,equal_to<int> >::const_iterator hcit;
  for (i=0; i<n; i++) {
    if (bdry[i]) {
      vicit end = nmap[i].end();
      for (vicit it = nmap[i].begin(); it != end; it++) {
	if (!bdry[*it]) {
	  workset.insert(*it);
	}
      }
      prevset.insert(i);
    }
  }

  // first round: just go through the mesh in the
  // topological order in increasing distance from the
  // mesh boundary
  int cnt = nbdry;
  cout << pnts.size() << "..." << flush;
  while (!workset.empty()) {

    cout << pnts.size() - cnt << "..." << flush;
    cnt += workset.size();

    for (hcit = workset.begin(); hcit != workset.end(); hcit++) {
      i = *hcit;
      vicit end = nmap[i].end();
      for (vicit it = nmap[i].begin(); it != end; it++) {
	// calculate new distance
	float dij = dist(pnts[i], pnts[*it]);
	float d   = dij + distances[*it];
	if (d < distances[i]) distances[i] = d;
	// should neighbor go to the nextset?
	if (workset.find(*it) != workset.end()) continue;
	if (prevset.find(*it) != prevset.end()) continue;
	nextset.insert(*it);
	d = dij + distances[i];
	if (d < distances[*it]) distances[*it] = d;
      }
    }

    prevset = workset;
    workset = nextset;
    nextset.clear();
  }

  cout << endl << "Restart" << endl;

  // second round: put all the nonboundary vertices into
  // the workset, all the

  workset.clear();
  for (i=0; i<n; i++) {
    if (!bdry[i]) {
      workset.insert(i);
    }
  }

  while (!workset.empty()) {

    cout << workset.size() << "..." << flush;

    nextset.clear();
    // calculate distances
    for (hcit = workset.begin(); hcit != workset.end(); hcit++) {
      i = *hcit;
      vicit end = nmap[i].end();
      for (vicit it = nmap[i].begin(); it != end; it++) {
	float d = dist(pnts[i], pnts[*it]) + distances[*it];
	if (d < distances[i]) distances[i] = d;
      }
    }

    // see if neighbors should be in set
    for (hcit = workset.begin(); hcit != workset.end(); hcit++) {
      i = *hcit;
      vicit end = nmap[i].end();
      for (vicit it = nmap[i].begin(); it != end; it++) {
	float d = dist(pnts[i], pnts[*it]) + distances[i];
	if (d < distances[*it]) {
	  distances[*it] = d;
	  nextset.insert(*it);
	}
      }
    }
    workset = nextset;
  }
  cout << endl;
}
#else

// assume bdry has the right size and has been initialized with
// zeroes
void
distance_from_boundary(vector<float> &distances,
		       const vector<Pnt3> &pnts,
		       const vector<int> &tris)
{
  TIMER(dist);

  int n = pnts.size();
  int i;

  // find boundary verts
  vector<char> bdry(n, (char)0);

  int *n_nbors, **nbors;
  nbors_tris(n, tris, n_nbors, nbors, true);

  // for each vertex, try to find a full loop
  // around it, if can't its boundary
  int va, vb; // vertex after, vertex before
  for (i=0; i<n; i++) {
    if (n_nbors[i] <= 1) {
      bdry[i] = 1;
      continue;
    }
    int jend = n_nbors[i] - 1;
    for (int j=0; j<jend; j++) {
      // which vtx comes before this (corner j)?
      int k = nbors[i][j];
      vb = BEFORE(k % 3, k);
      // which vtx comes after this (corner j+1)?
      k = nbors[i][j+1];
      va = AFTER(k % 3, k);
      if (va != vb) {
	// not a continuous neighbor chain
	bdry[i] = 1;
	break;
      }
    }
    if (bdry[i] == 0) {
      // check the last possible link (from end to start)
      // which vtx comes before this (corner jend)?
      int k = nbors[i][jend];
      vb = BEFORE(k % 3, k);
      // which vtx comes after this (corner jstart)?
      k = nbors[i][0];
      va = AFTER(k % 3, k);
      if (va != vb) {
	// not a continuous neighbor chain
	bdry[i] = 1;
      }
    }
  }

  cout << "done boundary" << endl;

  // initialize distance vector
  distances.clear();
  distances.insert(distances.begin(), n, 1e33);
  int nbdry = 0;
  for (i=0; i<n; i++) {
    if (bdry[i]) { distances[i] = 0.0; nbdry++; }
  }

  // find the neighbors of bdry vertices
  unordered_set<int,hash<int>,equal_to<int> > prevset, workset, nextset;
  unordered_set<int,hash<int>,equal_to<int> >::const_iterator hcit;
  for (i=0; i<n; i++) {
    if (bdry[i]) {
      for (int j=0; j<n_nbors[i]; j++) {
	int k = AFTER(nbors[i][j]%3, nbors[i][j]);
	if (!bdry[k]) workset.insert(k);
	k = BEFORE(nbors[i][j]%3, nbors[i][j]);
	if (!bdry[k]) workset.insert(k);
      }
      prevset.insert(i);
    }
  }

  // first round: just go through the mesh in the
  // topological order in increasing distance from the
  // mesh boundary
  int cnt = nbdry;
  cout << pnts.size() << "..." << flush;
  while (!workset.empty()) {

    cout << pnts.size() - cnt << "..." << flush;
    cnt += workset.size();

    for (hcit = workset.begin(); hcit != workset.end(); hcit++) {
      i = *hcit;
      for (int j=0; j<n_nbors[i]; j++) {
	int k = AFTER(nbors[i][j]%3, nbors[i][j]);
	// calculate new distance
	float dij = dist(pnts[i], pnts[k]);
	float d   = dij + distances[k];
	if (d < distances[i]) distances[i] = d;
	// should neighbor go to the nextset?
	if (workset.find(k) != workset.end()) continue;
	if (prevset.find(k) != prevset.end()) continue;
	nextset.insert(k);
	d = dij + distances[i];
	if (d < distances[k]) distances[k] = d;
      }
    }

    prevset = workset;
    workset = nextset;
    nextset.clear();
  }

  cout << endl << "Restart" << endl;

  // second round: put all the nonboundary vertices into
  // the workset, all the

  workset.clear();
  for (i=0; i<n; i++) {
    if (!bdry[i]) {
      workset.insert(i);
    }
  }

  while (!workset.empty()) {

    cout << workset.size() << "..." << flush;

    nextset.clear();
    // calculate distances
    for (hcit = workset.begin(); hcit != workset.end(); hcit++) {
      i = *hcit;
      for (int j=0; j<n_nbors[i]; j++) {
	int k = AFTER(nbors[i][j]%3, nbors[i][j]);
	float d = dist(pnts[i], pnts[k]) + distances[k];
	if (d < distances[i]) distances[i] = d;
      }
    }

    // see if neighbors should be in set
    for (hcit = workset.begin(); hcit != workset.end(); hcit++) {
      i = *hcit;
      for (int j=0; j<n_nbors[i]; j++) {
	int k = AFTER(nbors[i][j]%3, nbors[i][j]);
	float d = dist(pnts[i], pnts[k]) + distances[i];
	if (d < distances[k]) {
	  distances[k] = d;
	  nextset.insert(k);
	}
      }
    }
    workset = nextset;
  }
  cout << endl;
  // cleanup
  delete[] n_nbors;
  delete[] nbors[0]; // ptr to one chunk of data for all
  delete[] nbors;
}
#endif


void
quadric_simplify(const vector<Pnt3> &vtx_in,
		 const vector<int>  &tri_in,
		 vector<Pnt3> &vtx_out,
		 vector<int>  &tri_out,
		 int           goal,
		 int           optLevel,
		 float         errLevel,
		 float         boundWeight)
{
   char inName[L_tmpnam], outName[L_tmpnam];
   FILE *f;
   int i;
   char ins[200];

   vtx_out.clear();
   tri_out.clear();

   // Write out the original mesh data to a temporary input file

   if (!tmpnam(inName))  {
      cerr << "ERROR: Unable to get temporary filename for QSlim input" << endl;
      return;
   }
   if (!(f = fopen(inName, "w")))  {
      cerr << "ERROR: Unable to open file " << inName << " for QSlim input"
           << endl;
      return;
   }
   for (i=0; i<vtx_in.size(); ++i)
      fprintf(f, "v %f %f %f\n", vtx_in[i][0], vtx_in[i][1], vtx_in[i][2]);
   for (i=0; i<tri_in.size(); i+=3)
      fprintf(f, "f %d %d %d\n", tri_in[i]+1, tri_in[i+1]+1, tri_in[i+2]+1);
   fclose(f);

   // Get a filename for the temporary output file

   if (!tmpnam(outName))  {
      cerr << "ERROR: Unable to get temporary filename for QSlim output"
           << endl;
      remove(inName);
      return;
   }

   // Run QSlim as an external program to simplify the mesh

   char qslimCmd[200];
   sprintf(qslimCmd, "qslim -t %d -B %f -o %s %s", goal, boundWeight,
                                                   outName, inName);
   if (system(qslimCmd))  {
      cerr << "\nERROR: Unable to execute call to external QSlim program:\n"
           << qslimCmd << "\nPlease verify that qslim is properly installed"
           << " and accessible on your system." << endl;
      remove(inName);
      remove(outName);
      return;
   }

   // Read in the simplified mesh from the resulting output file

   float v1, v2, v3;
   int f1, f2, f3;

   if (!(f = fopen(outName, "r"))) {
      cerr << "ERROR: Unable to open file " << outName << " from QSlim"
           << endl;
      remove(inName);
      return;
   }
   while (fscanf(f, "%s", ins) == 1)  {
      if (!strcmp(ins, "v")) {
         fscanf(f, "%f %f %f", &v1, &v2, &v3);
         vtx_out.push_back(Pnt3(v1, v2, v3));
      }
      else if (!strcmp(ins, "f")) {
         fscanf(f, "%d %d %d", &f1, &f2, &f3);
         tri_out.push_back(f1-1);
         tri_out.push_back(f2-1);
         tri_out.push_back(f3-1);
      }
   }
   fclose(f);

   remove(inName);
   remove(outName);
}



/////////////////////////////////////
// BEGIN write_ply_file() HELPER CODE

struct PlyVertex {
  float x, y, z;
  float nx, ny, nz;
  uchar diff_r, diff_g, diff_b;
  float tex_u, tex_v;
  float intensity;
  float std_dev;
  float confidence;
};

const int MAX_FACE_VERTS = 100;

struct PlyFace {
  uchar nverts;
  int verts[MAX_FACE_VERTS];
};


struct CoordConfVert {
   float v[3];
   float conf;
};

struct TriLocal {
   uchar nverts;
   int index[3];
};


static PlyProperty vert_prop_x =
   {"x", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_y =
  {"y", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_z =
  {"z", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_nx =
   {"nx", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_ny =
  {"ny", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_nz =
  {"nz", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_intens =
  {"intensity", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_conf =
  {"confidence", PLY_FLOAT, PLY_FLOAT, 0, 0, PLY_START_TYPE,
   PLY_START_TYPE, 0};


static PlyProperty vert_prop_diff_r =
  {"diffuse_red", PLY_UCHAR, PLY_UCHAR, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_diff_g =
  {"diffuse_green", PLY_UCHAR, PLY_UCHAR, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};
static PlyProperty vert_prop_diff_b =
  {"diffuse_blue", PLY_UCHAR, PLY_UCHAR, 0, 0, PLY_START_TYPE, PLY_START_TYPE, 0};

static PlyProperty face_props[] = {
  {"vertex_indices", PLY_INT, PLY_INT, 0, true, PLY_UCHAR, PLY_UCHAR, 0},
};

struct tstrip_info {
  int nverts;
// STL Update
  const vector<int>::const_iterator vertData;
};

static PlyProperty tstrips_props[] = {
  {"vertex_indices", PLY_INT, PLY_INT, 4, true, PLY_INT, PLY_INT, 0},
};

static void
set_offsets(void)
{
  static int first = 1;
  if (first) {
    first = 0;
    vert_prop_x.offset          = offsetof(PlyVertex,x);
    vert_prop_y.offset          = offsetof(PlyVertex,y);
    vert_prop_z.offset          = offsetof(PlyVertex,z);
    vert_prop_nx.offset         = offsetof(PlyVertex,nx);
    vert_prop_ny.offset         = offsetof(PlyVertex,ny);
    vert_prop_nz.offset         = offsetof(PlyVertex,nz);
    vert_prop_intens.offset     = offsetof(PlyVertex,intensity);
    vert_prop_conf.offset       = offsetof(PlyVertex,confidence);
    vert_prop_diff_r.offset     = offsetof(PlyVertex,diff_r);
    vert_prop_diff_g.offset     = offsetof(PlyVertex,diff_g);
    vert_prop_diff_b.offset     = offsetof(PlyVertex,diff_b);

    tstrips_props[0].offset     = offsetof(tstrip_info,vertData);
  }
}

////////////////////////////
// write_ply_file() : called by all exported wrappers to handle the different
// variants of ply files.

static void
write_ply_file(const char *fname,
	       const vector<Pnt3> &vtx,
	       const vector<int> &tris, bool strips,
	       const vector<Pnt3> &nrm,
	       const vector<uchar> &intensity,
	       const vector<float> &confidence,
	       bool hasNrm, bool hasIntensity, bool hasConfidence)
{
  int i;
  char *elem_names[] = {"vertex", strips ? "tristrips" : "face"};

  bool hasColor = false;
  bool hasAlpha = false;
  if (hasIntensity) {
    if (intensity.size() == 3 * vtx.size()) {
      hasColor = true;
      hasIntensity = false;
    } else if (intensity.size() == 4 * vtx.size()) {
      hasColor = true;
      hasAlpha = true;
      hasIntensity = false;
    }
  }

  PlyFile ply;
  if (!ply.open_for_writing(fname, 2, elem_names,PLY_BINARY_BE))
    return;

  vector<PlyProperty> vert_props;
  set_offsets();

  vert_props.push_back(vert_prop_x);
  vert_props.push_back(vert_prop_y);
  vert_props.push_back(vert_prop_z);

  if (hasNrm) {
    vert_props.push_back(vert_prop_nx);
    vert_props.push_back(vert_prop_ny);
    vert_props.push_back(vert_prop_nz);
  }

  if (hasIntensity) {
    vert_props.push_back(vert_prop_intens);
  }

  if (hasConfidence) {
    vert_props.push_back(vert_prop_conf);
  }

  if (hasColor) {
    vert_props.push_back(vert_prop_diff_r);
    vert_props.push_back(vert_prop_diff_g);
    vert_props.push_back(vert_prop_diff_b);
  }

  // count offset
  face_props[0].offset = offsetof(PlyFace, verts);
  face_props[0].count_offset = offsetof(PlyFace, nverts);

  ply.describe_element ("vertex", vtx.size(),
			vert_props.size(), &vert_props[0]);
  if (strips)
    ply.describe_element ("tristrips", 1, 1, tstrips_props);
  else
    ply.describe_element ("face", tris.size()/3, 1, face_props);
  ply.header_complete();

    // set up and write the vertex elements
  PlyVertex plyVert;

  ply.put_element_setup ("vertex");

  for (i = 0; i < vtx.size(); i++) {
    plyVert.x = vtx[i][0];
    plyVert.y = vtx[i][1];
    plyVert.z = vtx[i][2];
    if (hasIntensity)
      plyVert.intensity = intensity[i] / 255.0;

    if (hasColor) {
      int ci = (hasAlpha ? 4 : 3) * i;
      plyVert.diff_r = intensity[ci  ];
      plyVert.diff_g = intensity[ci+1];
      plyVert.diff_b = intensity[ci+2];
    }

    if (hasNrm) {
      plyVert.nx = nrm[i][0];
      plyVert.ny = nrm[i][1];
      plyVert.nz = nrm[i][2];
    }
    if (hasConfidence) {
      plyVert.confidence = confidence[i];
    }
    ply.put_element((void *) &plyVert);
  }

  if (strips) {
    ply.put_element_setup ("tristrips");

// STL Update
    tstrip_info ts_info = { tris.size(), tris.begin() };
    ply.put_element (&ts_info);

  } else {

    ply.put_element_setup ("face");

    PlyFace plyFace;
    for (i = 0; i < tris.size(); i+=3) {
      plyFace.nverts = 3;
      plyFace.verts[0] = tris[i+0];
      plyFace.verts[1] = tris[i+1];
      plyFace.verts[2] = tris[i+2];

      ply.put_element_static_strg((void *) &plyFace);
    }
  }
}

// exported wrappers for write_ply_file

void
write_ply_file(const char *fname,
	       const vector<Pnt3> &vtx,
	       const vector<int> &tris, bool strips,
	       const vector<Pnt3> &nrm,
	       const vector<uchar> &intensity,
	       const vector<float> &confidence)
{
  write_ply_file(fname, vtx, tris, strips, nrm, intensity, confidence,
		 true, true, true);
}

void
write_ply_file(const char *fname,
	       const vector<Pnt3> &vtx,
	       const vector<int> &tris, bool strips,
	       const vector<Pnt3> &nrm,
	       const vector<uchar> &intensity)
{
  vector<float> confidence;
  write_ply_file(fname, vtx, tris, strips, nrm, intensity, confidence,
		 true, true, false);
}

void
write_ply_file(const char *fname,
	       const vector<Pnt3> &vtx,
	       const vector<int> &tris, bool strips,
	       const vector<Pnt3> &nrm)
{
  vector<uchar> intensity;
  vector<float> confidence;
  write_ply_file(fname, vtx, tris, strips, nrm, intensity, confidence,
		 true, false, false);
}

void
write_ply_file(const char *fname,
	       const vector<Pnt3> &vtx,
	       const vector<int> &tris, bool strips)
{
  vector<Pnt3> nrm;
  vector<uchar> intensity;
  vector<float> confidence;
  write_ply_file(fname, vtx, tris, strips, nrm, intensity, confidence,
		 false, false, false);
}

void
write_ply_file(const char *fname,
	       const vector<Pnt3> &vtx,
	       const vector<int> &tris, bool strips,
	       const vector<uchar> &intensity)
{
  vector<Pnt3> nrm;
  vector<float> confidence;
  write_ply_file(fname, vtx, tris, strips, nrm, intensity, confidence,
		 false, true, false);
}

void
write_ply_file(const char *fname,
	       const vector<Pnt3> &vtx,
	       const vector<int> &tris, bool strips,
	       const vector<float> &confidence)
{
  vector<Pnt3> nrm;
  vector<uchar> intensity;
  write_ply_file(fname, vtx, tris, strips, nrm, intensity, confidence,
		 false, false, true);
}

void
write_ply_file(const char *fname,
	       const vector<Pnt3> &vtx,
	       const vector<int> &tris, bool strips,
	       const vector<uchar> &intensity,
	       const vector<float> &confidence)
{
  vector<Pnt3> nrm;
  write_ply_file(fname, vtx, tris, strips, nrm, intensity, confidence,
		 false, true, true);
}

void
write_ply_file(const char *fname,
	       const vector<Pnt3> &vtx,
	       const vector<int> &tris, bool strips,
	       const vector<Pnt3> &nrm,
	       const vector<float> &confidence)
{
  vector<uchar> intensity;
  write_ply_file(fname, vtx, tris, strips, nrm, intensity, confidence,
		 true, false, true);
}


void
pushNormalAsShorts (vector<short>& nrms, Pnt3 n)
{
  n *= 32767;
  nrms.push_back (n[0]);
  nrms.push_back (n[1]);
  nrms.push_back (n[2]);
}

void
pushNormalAsPnt3 (vector<Pnt3>& nrms, short* n, int i)
{
  i *= 3;
  Pnt3 nrm (n[i]/32767.0, n[i+1]/32767.0, n[i+2]/32767.0);
  nrms.push_back (nrm);
}

// STL Update
void
pushNormalAsPnt3 (vector<Pnt3>& nrms, vector<short>::iterator n, int i)
{
  i *= 3;
  Pnt3 nrm (n[i]/32767.0, n[i+1]/32767.0, n[i+2]/32767.0);
  nrms.push_back (nrm);
}


