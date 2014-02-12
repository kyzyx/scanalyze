//############################################################
//
// CyraResLevel.cc
//
// Lucas Pereira
// Thu Jul 16 15:20:47 1998
//
// Part of CyraScan.cc,
//    Store range scan information from a Cyra Cyrax Beta
//    time-of-flight laser range scanner.
//
//############################################################

#include "CyraResLevel.h"
#include <tcl.h>
#include <stdio.h>
#include <iostream.h>
#include <fstream.h>
#include <stack.h>
#include <stdlib.h>
#include "defines.h"
#include "TriMeshUtils.h"
#include "KDindtree.h"
#include "ColorUtils.h"
#include "plvScene.h"
#include "MeshTransport.h"
#include "VertexFilter.h"

#ifdef WIN32
#  define random rand
#endif

// The maximum depth distance across which it will tesselate.
// (Bigger than this, it assumes a noncontinuous surface)
#define DEFAULT_CYRA_TESS_DEPTH 80;
float CyraTessDepth = DEFAULT_CYRA_TESS_DEPTH;

// Default to filtering on...
#define DEFAULT_CYRA_FILTER_SPIKES TRUE;
bool CyraFilterSpikes = DEFAULT_CYRA_FILTER_SPIKES;

// Default to fill holes  on...
#define DEFAULT_CYRA_FILL_HOLES TRUE;
bool CyraFillHoles = DEFAULT_CYRA_FILL_HOLES;




//////////////////////////////////////////////////////////////////////
// CyraSample   (One single data point)
// Helper functions...
//////////////////////////////////////////////////////////////////////

void
CyraSample::zero(void) {
  vtx[0] = vtx[1] = vtx[2] = 0;
  nrm[0] = nrm[1] = nrm[2] = 0;
  intensity = 0;
  confidence = 0;
}

// interp 2 samples
void
CyraSample::interp(CyraSample &a, CyraSample &b) {
  for (int i = 0; i < 3; i++) {
    vtx[i] = 0.5 * (a.vtx[i] + b.vtx[i]);
    nrm[i] = 0.5 * (a.nrm[i] + b.nrm[i]);
  }
  intensity = 0.5 * (a.intensity + b.intensity);
  confidence = 0.5 * (a.confidence + b.confidence);
}

// interp 3 samples, weighted 1/4, 1/2, 1/4
void
CyraSample::interp(CyraSample &a, CyraSample &b, CyraSample &c) {
  for (int i = 0; i < 3; i++) {
    vtx[i] = 0.25 * (a.vtx[i] + b.vtx[i]+b.vtx[i] + c.vtx[i]);
    nrm[i] = 0.25 * (a.nrm[i] + b.nrm[i]+b.nrm[i] + c.nrm[i]);
  }
  intensity = 0.25 * (a.intensity + b.intensity+b.intensity + c.intensity);
  confidence = 0.25 * (a.confidence + b.confidence+b.confidence +
		       c.confidence);
}

// Interpolate arbitrary number of samples, weighted evenly...
void
CyraSample::interp(int nsamps, CyraSample *samps[]) {
  assert(nsamps > 0);
  this->zero();
  float wt = 1.0 / nsamps;
  // add up the values of every sample...
  for (int samnum = 0; samnum < nsamps; samnum++) {
    for (int i=0; i < 3; i++) {
      vtx[i] += samps[samnum]->vtx[i];
      nrm[i] += samps[samnum]->nrm[i];
    }
    intensity += samps[samnum]->intensity;
    confidence += samps[samnum]->confidence;
  }
  // divide by the appropriate weight...
  for (int i=0; i < 3; i++) {
    vtx[i] /= wt;
    nrm[i] /= wt;
  }
  intensity /= wt;
  confidence /= wt;
}



bool
CyraSample::isValid(void) {
  return((vtx[2] > 0) ? TRUE : FALSE);
}


//////////////////////////////////////////////////////////////////////
// CyraResLevel  (one resolution level for a cyra scan)
//////////////////////////////////////////////////////////////////////



CyraResLevel::CyraResLevel(void) {
  // Clear arrays
  width = 0;
  height = 0;
  numpoints = 0;
  numtris = 0;
  points.clear();
  tris.clear();
  origin.set(0,0,0);

  isDirty_cache = true;
  cachedPoints.clear();
  cachedNorms.clear();
  cachedBoundary.clear();
  kdtree = NULL;

}

CyraResLevel::~CyraResLevel(void) {
  // do nothing yet
  if (kdtree) delete kdtree;
}


// PUSH_TRI --  basically a macro used by the mesh-generation function
// to look up the 3 vertices in vert_inds, and do tstrips if necessary
inline void PUSH_TRI(int &ov2, int &ov3, vector<int> &tri_inds,
		      const bool stripped,
		      const vector<int> &vert_inds,
		      const int v1, const int v2, const int v3)
{
  // will next tri be odd or even? (even tris get flipped)
  static bool oddtri = true;

  if (stripped) {
    if (oddtri && (ov2 == v2 && ov3 == v1)) {
      // odd-numbered triangle
      tri_inds.push_back(vert_inds[v3]);
      oddtri = false;
    } else if (!oddtri && (ov2 == v1 && ov3 == v3)) {
      // even-numbered triangle -- listed backwards
      tri_inds.push_back(vert_inds[v2]);
      oddtri = true;
    } else {
      tri_inds.push_back(-1);
      tri_inds.push_back(vert_inds[v1]);
      tri_inds.push_back(vert_inds[v2]);
      tri_inds.push_back(vert_inds[v3]);
      oddtri = false;
    }
    ov2 = v2;
    ov3 = v3;
  } else {
    tri_inds.push_back(vert_inds[v1]);
    tri_inds.push_back(vert_inds[v2]);
    tri_inds.push_back(vert_inds[v3]);
  }
}





// Add the geometry for this reslevel to the list
MeshTransport*
CyraResLevel::mesh(bool perVertex, bool stripped,
		   ColorSource color, int colorSize)
{
  // Offset (index) for the next vertex;
  int vertoffset = 0;

  ////// Vertices /////
  // The valid vertices need contiguous index numbers.  So fill an
  // array vert_inds -- index numbers with a 1-1 correspondence
  // to the array of vertices.
  vector<int> vert_inds;
  vert_inds.reserve(points.size());
  // Set the vert_inds array
  for (int i = 0; i < points.size(); i++) {
    vert_inds.push_back((points[i].confidence > 0) ?
			vertoffset++ : -1);
  }

  vector<Pnt3>* vtx = new vector<Pnt3>;
  vector<short>* nrm = new vector<short>;
  vector<int>* tri_inds = new vector<int>;

  // Add each valid vertex to vtx, nrm(?)
  FOR_EACH_VERT(vtx->push_back(v->vtx));
  if (perVertex) {
    FOR_EACH_VERT(nrm->insert (nrm->end(), v->nrm, v->nrm + 3));
  } else {
    FOR_EACH_TRI(pushNormalAsShorts
		 (*nrm, ((Pnt3)cross(points[tv3].vtx-points[tv2].vtx,
				     points[tv1].vtx-points[tv2].vtx))
		  .normalize()));
  }

  ////// Triangles /////
  // Now fill in the triangle array
  // Initialize the "old v2, v3" values used in tstripping
  int ov2 = -1; int ov3 = -1;

  // call the FOR_EACH_TRI macro, which defines tv1, tv2, tv3.
  FOR_EACH_TRI(PUSH_TRI(ov2, ov3, *tri_inds, stripped,
			vert_inds, tv1, tv2, tv3));

  // Terminate the last tstrip
  if (stripped) {
    tri_inds->push_back(-1);
  }

  MeshTransport* mt = new MeshTransport;
  mt->setVtx (vtx, MeshTransport::steal);
  mt->setNrm (nrm, MeshTransport::steal);
  mt->setTris (tri_inds, MeshTransport::steal);

  if (color != RigidScan::colorNone) {
    vector<uchar>* colors = new vector<uchar>;
    int nColors = perVertex ? vtx->size() : num_tris();
    colors->reserve (colorSize * nColors);

    if (colorMesh (*colors, perVertex, color, colorSize))
      mt->setColor (colors, MeshTransport::steal);
    else
      delete colors;
  }

  return mt;
}


// Fill in the colors array.
bool
CyraResLevel::colorMesh(vector<uchar>   &colors,
			bool             perVertex,
			ColorSource      source,
			int              colorsize)
{
  switch (source) {
  case RigidScan::colorConf:
    // ========= Color by Confidence =========
    if (perVertex) {
      cerr << "adding per-vertex conf color..." << endl;
      // per-vertex confidence
      FOR_EACH_VERT(pushColor(colors, colorsize, (float)
			      (v->confidence /
			       CYRA_DEFAULT_CONFIDENCE)));
    } else {
      cerr << "adding per-face conf color..." << endl;
      // per-face confidence (take min of 3 confidences)
      FOR_EACH_TRI(pushColor(colors, colorsize, (float)
			     (MIN(points[tv1].confidence,
				       MIN(points[tv2].confidence,
					   points[tv3].confidence)) /
				   CYRA_DEFAULT_CONFIDENCE)));
    }
    break;

  case RigidScan::colorIntensity:
    // ========= Color by Intensity =========
    if (perVertex) {
      // per-vertex intensity
      FOR_EACH_VERT(pushColor(colors, colorsize, (float)
			      ((v->intensity+2048.0) / 4096)));
    } else {
      // per-face intensity (take min of 3 intensitys)
      FOR_EACH_TRI(pushColor(colors, colorsize, (float)
			     ((MIN(points[tv1].intensity,
				   MIN(points[tv2].intensity,
				       points[tv3].intensity)) + 2048.0) /
			      4096)));
    }

  case RigidScan::colorTrue:
    // ========== Color TrueColor ==========
    if (perVertex) {
      // per-vertex TrueColor
      // BUGBUG: For now, just color using intensity, because
      // I don't yet store colors for CyraScans.
      FOR_EACH_VERT(pushColor(colors, colorsize, (float)
			      ((v->intensity+2048.0) / 4096)));
    } else {
      // per-face TrueColor (take avg of 3 TrueColors)
      FOR_EACH_TRI(pushColor(colors, colorsize, (float)
			     ((MIN(points[tv1].intensity,
				   MIN(points[tv2].intensity,
				       points[tv3].intensity)) + 2048.0) /
			      4096)));
    }

  case RigidScan::colorBoundary:
    {
      create_kdtree();  // BUGBUG, this is a little heavyhanded approach
      colors.reserve (colorsize * cachedBoundary.size());
// STL Update
      vector<bool>::iterator end = cachedBoundary.end();
      for (vector<bool>::iterator c = cachedBoundary.begin(); c < end; c++)
	pushConf (colors, colorsize, (uchar)(*c ? 0 : 255));
    }
    break;

  default:
    cerr << "Unhandled Color Mode.  colorsource = " << source << endl;
    return false;
  }

  return true;
}


bool
CyraResLevel::ReadPts(const crope &inname)
{
  // open the input .pts file
  const char* filename = inname.c_str();
  FILE *inFile = fopen(filename, "r");
  if (inFile==NULL) return FALSE;
  bool isOldPtsFormat = false;		// old or new cyra .pts?

  // Read the 3-line .pts header
  fscanf(inFile, "%d\n", &width);
  fscanf(inFile, "%d\n", &height);
  float a,b,c;
  fscanf(inFile, "%f %f %f\n", &a, &b, &c);
  origin.set(a,b,c);

  cerr << "Reading " << width << "x" << height << " Cyra scan...";

  // Read in the points, since this is the only data actually in
  // a .pts file....
  CyraSample samp;

  // Figure out whether it's the old or new cyra format
  // old: X			new:	X
  //      Y      			Y
  //      0 0 0				0 0 0
  //	   x1 y1 z1 c1 nx1 ny1 nz1	1 0 0
  //	   x2 y2 z2 c2 nx2 ny2 nz2	0 1 0
  //	   x3 y3 z3 c3 nx3 ny3 nz3	0 0 1
  //	   ...                  	1 0 0 0
  //	                       		0 1 0 0
  //	                         	0 0 1 0
  //	                       		0 0 0 1
  //					x1 y1 z1 c1
  //					x2 y2 z2 c2
  //					...
  char buf[2000];
  int n1, n2, n3;
  // Read first line after 0 0 0
  fgets(buf, 2000, inFile);
  int nitems = sscanf(buf, "%g %g %g %d %d %d %d\n",
		      &(samp.vtx[0]), &(samp.vtx[1]), &(samp.vtx[2]),
		      &(samp.intensity),
		      &n1, &n2, &n3);
  if (nitems == 7) {
    // was the x1 y1 z1 c1 nx1 ny1 nz1 line... old format
    isOldPtsFormat = true;
    cerr << " (old format) ";
  } else if (nitems == 3) {
    // was the new format
    isOldPtsFormat = false;
    cerr << " (new format) ";
    // NOTE:  Skip (ignore) the 3+4 matrix lines
    for (int j=0; j < 7; j++) {
      fgets(buf, 2000,inFile);
    }
  }
  // Now, our assertion is that first line of data is read into
  // buf...


  // First reserve space, so we don't waste time doing multi mallocs...
  points.reserve(width * height);
  for (int x=0; x < width; x++) {
    for (int y=0; y < height; y++) {
      // Read position, intensity, normals
      Pnt3 filenrm;

      // Whether isOldPtsFormat or not, just ignore the normals that
      // are in the .pts file, for now...  Since the normals Cyra generates
      // are generally bogus anyways.
      int nread = sscanf(buf, "%f %f %f %d\n",
			 &(samp.vtx[0]), &(samp.vtx[1]), &(samp.vtx[2]),
			 &(samp.intensity));
      fgets(buf, 2000, inFile);
      if (nread != 4) {
	int lineno = ((isOldPtsFormat)?4:11) + x*height + y;
	cerr << "Error: Trouble reading Cyra input line " << lineno
	     << "." << endl;
	return false;
      }

      samp.nrm[0] = 0; samp.nrm[1] = 0; samp.nrm[2] = 32767;
      // Convert to millimeters
      samp.vtx *= 1000.0;

      // Compute confidence
      if (samp.vtx[0] == 0. && samp.vtx[1] == 0. &&
	  samp.vtx[2] == 0. && samp.intensity == 0) {
	// then point is missing data
	samp.confidence = 0.0;
      } else {
	samp.confidence = CYRA_DEFAULT_CONFIDENCE;
	// count number of valid points
	numpoints++;

	// unweight specular highlights -- linearly
	// make the weight falloff to 0 between intensities
	// -800 and 2048
	if (samp.intensity > -800) {
	  float unweightfactor = (2048.0 - samp.intensity) /
	    (2048.0 - (-800));
	  samp.confidence *= unweightfactor;
	}
	// also make the weight falloff between -1600 and -2048
	if (samp.intensity < -1600) {
	  float unweightfactor = (2048.0 + samp.intensity) /
	    (2048.0 - 1600.0);
	  samp.confidence *= unweightfactor;
	}
      }

      // Now put the vertex into the array
      points.push_back(samp);
    }
  }
  cerr << "done." << endl;

  // Now actually figure out the "correct" tesselation

  // first reserve space, so we don't waste time doing multi mallocs...
  tris.reserve((width-1) * (height-1));

  // Grab the CyraTessDepth from TCL...
  char* CTDstring = Tcl_GetVar (g_tclInterp, "CyraTessDepth", TCL_GLOBAL_ONLY);
  if (CTDstring == NULL) {
    CyraTessDepth = DEFAULT_CYRA_TESS_DEPTH;
  } else {
    // String defined, get it...?
    sscanf(CTDstring, "%f", &CyraTessDepth);
    // If 0, then set to default
    if (CyraTessDepth == 0) {
      CyraTessDepth = DEFAULT_CYRA_TESS_DEPTH;
    }
  }

  // Grab the CyraFilterSpikes from TCL...
  char* CFSstring = Tcl_GetVar (g_tclInterp, "CyraFilterSpikes", TCL_GLOBAL_ONLY);
  if (CFSstring == NULL) {
    CyraFilterSpikes = DEFAULT_CYRA_FILTER_SPIKES;
  } else {
    // String defined, get it...?
    int CFSval;
    sscanf(CFSstring, "%d", &CFSval);
    CyraFilterSpikes = (CFSval == 0) ? FALSE : TRUE;
  }

  // Grab the CyraFillHoles from TCL...
  char* CFHstring = Tcl_GetVar (g_tclInterp, "CyraFillHoles", TCL_GLOBAL_ONLY);
  if (CFHstring == NULL) {
    CyraFillHoles = DEFAULT_CYRA_FILTER_SPIKES;
  } else {
    // String defined, get it...?
    int CFHval;
    sscanf(CFHstring, "%d", &CFHval);
    CyraFillHoles = (CFHval == 0) ? FALSE : TRUE;
  }

  ////////////// Filter out spikes?  ///////////
  if (CyraFilterSpikes) {
    cerr << "Filtering out high-intensity spikes....";
    for (x=0; x < width-1; x++) {
      for (int y=0; y < height-1; y++) {
	CyraSample *v1 = &(point(x,y));

	// Look for random single spikes of noise...
	float myZ  = v1->vtx[2];
	int myI = v1->intensity;
	float neighborZ = 0;
	float neighborI = 0;
	float neighborWt = 0;
	int neighborsDeeper = 0;
	int neighborsCloser = 0;

      // over 3x3 neighborhood...
      // within 300mm depth...
	for (int nex = MAX(0,x-1); nex <= MIN(width-1, x+1); nex++) {
	  for (int ney = MAX(0,y-1); ney <= MIN(width-1, y+1); ney++) {
	    CyraSample *ne = &point(nex, ney);
	    if (ne->vtx[2] < myZ) neighborsDeeper++;
	    else neighborsCloser++;
	    if (ne->vtx[2] > myZ - 150 && ne->vtx[2] < myZ + 150 &&
		(nex != x || ney != y) && ne->intensity < myI) {
	      float neWt = myI - ne->intensity;
	      neighborZ += ne->vtx[2] * neWt;
	      neighborI += ne->intensity *neWt;
	      neighborWt += neWt;
	    }
	  }
	}

	if (neighborWt != 0) {
	  neighborZ /= neighborWt;
	  neighborI /= neighborWt;

	  if (neighborsDeeper <=2 && neighborsCloser >= 5 &&
		    myI > neighborI + 20) {
	    // This point seems to be farther away, and brighter, than
	    // most of it's neighbors... We think it's the funny artifact...

	    //cerr << "before dd: " << (myZ-neighborZ) << " " << (myI-neighborI)
	    //     << " depths: " << myZ << " , " << neighborZ << " ; intens: "
	    //     << myI << " , " << neighborI << endl;
	    float posScale = neighborZ / myZ;
	    v1->vtx[0] *= posScale;
	    v1->vtx[1] *= posScale;
	    v1->vtx[2] *= posScale;
	    v1->intensity = neighborI;
	  }
	}
      }  // end of filtering spikes...
    }
    cerr << "Done! (filtering)\n";
  }

  ////////////// Fill holes? ///////////
  if (CyraFillHoles) {
    cerr << "Filling Tiny Holes....";
    for (x=0; x < width-1; x++) {
      for (int y=0; y < height-1; y++) {
	CyraSample *v1 = &(point(x,y));

	float myZ  = v1->vtx[2];
	float neighborsZ = 0;
	float neighborsWt = 0;
	int xstart = MAX(0, x-1);
	int ystart = MAX(0, y-1);
	int xend = MIN(width-1, x+1);
	int yend = MIN(height-1, y+1);

	// over 3x3 neighborhood...
	// find the mean...
	for (int nex = xstart; nex <= xend; nex++) {
	  for (int ney = ystart; ney <= yend; ney++) {
	    CyraSample *ne = &point(nex, ney);
	    if (nex != x || ney != y) {
	      neighborsZ += ne->vtx[2];
	      neighborsWt += 1.0;
	    }
	  }
	}
	neighborsZ /= neighborsWt;

	// Find the neighbor closest to the mean...
	// over 3x3 neighborhood...
	float closestZ = point(xstart, ystart).vtx[2];
	float closestDZ2 = (neighborsZ-closestZ)*(neighborsZ-closestZ);
	for (nex = xstart; nex <= xend; nex++) {
	  for (int ney = ystart; ney <= yend; ney++) {
	    CyraSample *ne = &point(nex, ney);
	    float neDZ2 = (neighborsZ-ne->vtx[2])*(neighborsZ-ne->vtx[2]);
	    if (neDZ2 < closestDZ2) {
	      closestDZ2 = neDZ2;
	      closestZ = ne->vtx[2];
	    }
	  }
	}

	// Set neighborsZ to be closestZ
	// (this way, if we have one outlier, we'll still grab from the
	// z depth of the main cluster...)
	neighborsZ = closestZ;

	// Make sure the neighbors are clustered closely together...
	// say, within 150mm of each other...?
	int neighborsClose = 0;
	float neighborX = 0;
	float neighborY = 0;
	float neighborZ = 0;
	float neighborI = 0;
	float neighborConf = 0;

	for (nex = xstart; nex <= xend; nex++) {
	  for (int ney = ystart; ney <= yend; ney++) {
	    CyraSample *ne = &point(nex, ney);
	    if ((nex != x || ney != y) &&
		(ne->vtx[2] < neighborsZ + CyraTessDepth &&
		 ne->vtx[2] > neighborsZ - CyraTessDepth)) {
	      neighborsClose++;
	      neighborX += ne->vtx[0];
	      neighborY += ne->vtx[1];
	      neighborZ += ne->vtx[2];
	      neighborI += ne->intensity;
	      neighborConf += ne->confidence;
	    }
	  }
	}

	//cerr <<" "<<neighborsClose<< ", nz " <<neighborsZ<< " , myz " <<myZ<< endl;

	// Only fill holes of data off by more than 150mm...
	// e.g. make sure this point is far from the neighborhood,
	if (neighborsWt < 7 || neighborsClose < 7 ||
	    (neighborsZ < myZ + CyraTessDepth &&
	     neighborsZ > myZ - CyraTessDepth)) continue;

	// otherwise, modify the puppy....
	v1->vtx[0] = neighborX / neighborsClose;
	v1->vtx[1] = neighborY / neighborsClose;
	v1->vtx[2] = neighborZ / neighborsClose;
	v1->intensity = neighborI / neighborsClose;
	v1->confidence = neighborConf / neighborsClose;
      }  // end of filling holes...
    }
    cerr << "Done! (filling holes)\n";
  }


  ////////////// Generate the Tesselation ///////////
  cerr << "Generating tesselation...";
  for (x=0; x < width-1; x++) {
    for (int y=0; y < height-1; y++) {
      // get pointers to the four vertices surrounding this
      // square:
      // 2 4
      // 1 3
      CyraSample *v1 = &(point(x,y));
      CyraSample *v2 = &(point(x,y+1));
      CyraSample *v3 = &(point(x+1,y));
      CyraSample *v4 = &(point(x+1,y+1));

      CyraTess tess = TESS0;
      // Set mask bit to be true if a vertex:
      //   a) exists (has confidence)
      //   b) is not an occlusion edge
      unsigned int mask =
	((v4->confidence && !grazing(v3->vtx, v4->vtx, v2->vtx))? 8 : 0) +
	((v3->confidence && !grazing(v1->vtx, v3->vtx, v4->vtx))? 4 : 0) +
	((v2->confidence && !grazing(v4->vtx, v2->vtx, v1->vtx))? 2 : 0) +
	((v1->confidence && !grazing(v2->vtx, v1->vtx, v3->vtx))? 1 : 0);

      switch (mask) {
      case 15:
	// verts: 1 2 3 4
	tess = TESS14;
	numtris += 2;
	break;
      case 14:
	// verts: 2 3 4
	tess = TESS4;
	numtris++;
	break;
      case 13:
	// verts: 1 3 4
	tess = TESS3;
	numtris++;
	break;
      case 11:
	// verts 1 2 4
	tess = TESS2;
	numtris++;
	break;
      case 7:
	// verts 1 2 3
	tess = TESS1;
	numtris++;
	break;
      default:
	// two or less vertices
	tess = TESS0;
	break;
      }

      // Now put the tesselation into the array
      tris.push_back(tess);

    }
  }

  cerr << "Done! (tesselating)" << endl;
  CalcNormals();

  cerr << "Loaded " << numpoints << " vertices, " <<
    numtris << " triangles." << endl;
  return true;
}


bool
CyraResLevel::WritePts(const crope &outname)
{
  CyraSample samp;

  // open the output .pts file
  const char* filename = outname.c_str();
  FILE *outFile = fopen(filename, "w");
  if (outFile==NULL) {
    cerr << "Error: couldn't open output file " << filename << endl;
    return FALSE;
  }

  // compute bounds, so we can crop to the smallest rectangle
  // that contains all the data.
  int lox   = width-1;
  int hix   = 0;
  int loy   = height-1;
  int hiy   = 0;
  for (int x=0; x < width; x++) {
    for (int y=0; y < height; y++) {
      samp = point(x,y);
      if (samp.confidence != 0) {
 	lox = (x<lox) ? x : lox;
 	hix = (x>hix) ? x : hix;
	loy = (y<loy) ? y : loy;
	hiy = (y>hiy) ? y : hiy;
      }
    }
  }
  if (lox > hix || loy > hiy) {
    cerr << "Error:  no nonzero-points found.  Aborting..." << endl;
    return FALSE;
  }

  // Write the 3-line .pts header
  fprintf(outFile, "%d\n", hix - lox + 1);
  fprintf(outFile, "%d\n", hiy - loy + 1);
  fprintf(outFile, "%f %f %f\n", origin[0], origin[1], origin[2]);
  cerr << "Writing " << (hix-lox+1) << "x" << (hiy-loy+1) << " Cyra scan...";

  // Write out the points, since this is the only data actually in
  // a .pts file....
  for (x=lox; x <= hix; x++) {
    for (int y=loy; y <= hiy; y++) {
      samp = point(x,y);
      if (samp.confidence == 0) {
	fprintf(outFile, "0 0 0 0 0 0 0\n");
      } else {
	// divide by 1000, since .pts files are always in meters
	fprintf(outFile, "%.5f %.5f %.5f %d 0 0 1\n",
		samp.vtx[0]/1000.0, samp.vtx[1]/1000.0, samp.vtx[2]/1000.0,
		samp.intensity);
      }
    }
  }
  fclose(outFile);
  cerr << "Done." << endl;
  return TRUE;
}

// Recompute the normals
void
CyraResLevel::CalcNormals(void)
{
  // Compute Normals
  Pnt3 hedge, vedge, norm;
  Pnt3 v1, v2, v3, v4;
  CyraTess tess;

  for (int x=0; x < width; x++) {
    for (int y=0; y < height; y++) {
      if (point(x,y).confidence > 0) {
	hedge.set(0,0,0);
	vedge.set(0,0,0);
	// Lower left corner
	if (x >0 && y > 0) {
	  v1 = point(x-1, y-1).vtx;
	  v2 = point(x-1, y  ).vtx;
	  v3 = point(x  , y-1).vtx;
	  v4 = point(x  , y  ).vtx;

	  tess = tri(x-1, y-1);
	  switch (tess) {
	  case TESS2:
	    hedge += 0.5*(v4-v2);
	    vedge += 0.5*(v2-v1);
	    break;
	  case TESS3:
	    hedge += 0.5*(v3-v1);
	    vedge += 0.5*(v4-v3);
	    break;
	  case TESS4:
	  case TESS14:
	    hedge += 1.0*(v4-v2);
	    vedge += 1.0*(v4-v3);
	    break;
	  case TESS23:
	    hedge += 0.5*(v4-v2);
	    hedge += 0.5*(v3-v1);
	    vedge += 0.5*(v2-v1);
	    vedge += 0.5*(v4-v3);
	    break;
	  }
	}

	// Upper left corner
	if (x > 0 && y < height-1) {
	  v1 = point(x-1,y  ).vtx;
	  v2 = point(x-1,y+1).vtx;
	  v3 = point(x  ,y  ).vtx;
	  v4 = point(x  ,y+1).vtx;

	  tess = tri(x-1,y);
	  switch (tess) {
	  case TESS1:
	    hedge += 0.5*(v3-v1);
	    vedge += 0.5*(v2-v1);
	    break;
	  case TESS3:
	  case TESS23:
	    hedge += 1.0*(v3-v1);
	    vedge += 1.0*(v4-v3);
	    break;
	  case TESS4:
	    hedge += 0.5*(v4-v2);
	    vedge += 0.5*(v4-v3);
	    break;
	  case TESS14:
	    hedge += 0.5*(v4-v2);
	    hedge += 0.5*(v3-v1);
	    vedge += 0.5*(v2-v1);
	    vedge += 0.5*(v4-v3);
	    break;
	  }
	}

	// Lower right corner
	if (x < width-1 && y > 0) {
	  v1 = point(x  ,y-1).vtx;
	  v2 = point(x  ,y  ).vtx;
	  v3 = point(x+1,y-1).vtx;
	  v4 = point(x+1,y  ).vtx;

	  tess = tri(x,y-1);
	  switch (tess) {
	  case TESS1:
	    hedge += 0.5*(v3-v1);
	    vedge += 0.5*(v2-v1);
	    break;
	  case TESS2:
	  case TESS23:
	    hedge += 1.0*(v4-v2);
	    vedge += 1.0*(v2-v1);
	    break;
	  case TESS4:
	    hedge += 0.5*(v4-v2);
	    vedge += 0.5*(v4-v3);
	    break;
	  case TESS14:
	    hedge += 0.5*(v4-v2);
	    hedge += 0.5*(v3-v1);
	    vedge += 0.5*(v2-v1);
	    vedge += 0.5*(v4-v3);
	    break;
	  }
	}

	// Upper right corner
	if (x < width-1 && y < height-1) {
	  v1 = point(x  ,y  ).vtx;
	  v2 = point(x  ,y+1).vtx;
	  v3 = point(x+1,y  ).vtx;
	  v4 = point(x+1,y+1).vtx;

	  tess = tri(x,y);
	  switch (tess) {
	  case TESS1:
	  case TESS14:
	    hedge += 1.0*(v3-v1);
	    vedge += 1.0*(v2-v1);
	    break;
	  case TESS2:
	    hedge += 0.5*(v4-v2);
	    vedge += 0.5*(v2-v1);
	    break;
	  case TESS3:
	    hedge += 0.5*(v3-v1);
	    vedge += 0.5*(v4-v3);
	    break;
	  case TESS23:
	    hedge += 0.5*(v4-v2);
	    hedge += 0.5*(v3-v1);
	    vedge += 0.5*(v2-v1);
	    vedge += 0.5*(v4-v3);
	    break;
	  }
	}

	// Now compute cross product, save normal
	norm = cross(hedge, vedge);
	norm.normalize();
	norm *= 32767;
	point(x, y).nrm[0] = norm[0];
	point(x, y).nrm[1] = norm[1];
	point(x, y).nrm[2] = norm[2];
      }
    }
  }
}



// detects grazing tris
bool
CyraResLevel::grazing(Pnt3 v1, Pnt3 v2, Pnt3 v3)
{
#if 0
  // First compute (negative) norm, which should point pretty
  // much away from the origin.  Compare this to the vector
  // from the origin....
  Pnt3 edge1 = v3 - v2;
  Pnt3 edge2 = v2 - v1;
  Pnt3 norm = cross(edge1, edge2);
  norm.normalize();
  // Treat v2 as a vector from 0,0,0
  v2.normalize();
  float cosdev = dot(norm, v2);

  //if (cosdev < .259) {
    // More than 75-degrees away from perpendicular
  if (cosdev < .017) {
    // more than 89-degrees away from perpendicular
    return true;
  } else {
    return false;
  }
  // #else

  // #ifdef USE_Z_AS_DEPTH
  // Check to see if depth difference is greater than our
  // typical occlusion distance (CyraTessDepthmm)
  if (v1[2] > v2[2] + CyraTessDepth ||
      v1[2] > v3[2] + CyraTessDepth ||
      v2[2] > v1[2] + CyraTessDepth ||
      v2[2] > v3[2] + CyraTessDepth ||
      v3[2] > v1[2] + CyraTessDepth ||
      v3[2] > v2[2] + CyraTessDepth) {
    return true;
  } else {
    return false;
  }
#else
  // Don't use Z as depth... compute distance from the center...
  float v1dist = sqrtf(v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2]);
  float v2dist = sqrtf(v2[0]*v2[0] + v2[1]*v2[1] + v2[2]*v2[2]);
  float v3dist = sqrtf(v3[0]*v3[0] + v3[1]*v3[1] + v3[2]*v3[2]);

  if (v1dist > v2dist + CyraTessDepth ||
      v1dist > v3dist + CyraTessDepth ||
      v2dist > v1dist + CyraTessDepth ||
      v2dist > v3dist + CyraTessDepth ||
      v3dist > v1dist + CyraTessDepth ||
      v3dist > v2dist + CyraTessDepth) {
    return true;
  } else {
    return false;
  }




  //#endif  // USE_Z_AS_DEPTH
#endif  // 0
}



int
CyraResLevel::num_vertices(void)
{
  return numpoints;
}

int
CyraResLevel::num_tris(void)
{
  return numtris;
}

void
CyraResLevel::growBBox(Bbox &bbox)
{
  FOR_EACH_VERT(bbox.add(v->vtx));
}

void flip3shorts (short* p)
{
  p[0] *= -1;
  p[1] *= -1;
  p[2] *= -1;
}

void
CyraResLevel::flipNormals(void)
{
  // Ok, to flip the normals, we:
  //     1. Reverse the order of the vertical scanlines (in memory)
  //     2. Reverse the order of the Triangle tesselation flags (in mem)
  //     3. Reverse the sign of the per-vertex normals

  ////// 1. Reverse the order of the vertical scanlines (in memory)
  CyraSample samp;
  for (int xx=0; xx < width/2; xx++) {
    for (int yy=0; yy < height; yy++) {
      samp = point(xx,yy);
      point(xx,yy) = point(width-xx-1, yy);
      point(width-xx-1, yy) = samp;
    }
  }

  ////// 2. Reverse the order of the Triangle tesselation flags (in mem)
  // (This also requires changing TESS1 <-> TESS3, TESS2 <-> TESS4)
  CyraTess tess;
  // Note:  only goes to width-1, but want to do middle col, too, so
  // the width-1+1 cancel out...
  for (xx=0; xx < (width)/2; xx++) {
    for (int yy=0; yy < height-1; yy++) {
      tess = tri(xx,yy);
      // Set scanline xx to be mirror of width-xx-2
      switch(tri(width-xx-2, yy)) {
      case TESS0:
	tri(xx,yy) = TESS0; break;
      case TESS1:
	tri(xx,yy) = TESS3; break;
      case TESS2:
	tri(xx,yy) = TESS4; break;
      case TESS3:
	tri(xx,yy) = TESS1; break;
      case TESS4:
	tri(xx,yy) = TESS2; break;
      case TESS14:
      case TESS23:
	tri(xx,yy) = TESS14; break;
      }
      // Set scanline width-xx-2 to be mirror of xx
      switch(tess) {
      case TESS0:
	tri(width-xx-2,yy) = TESS0; break;
      case TESS1:
	tri(width-xx-2,yy) = TESS3; break;
      case TESS2:
	tri(width-xx-2,yy) = TESS4; break;
      case TESS3:
	tri(width-xx-2,yy) = TESS1; break;
      case TESS4:
	tri(width-xx-2,yy) = TESS2; break;
      case TESS14:
      case TESS23:
	tri(width-xx-2,yy) = TESS14; break;
      }
    }
  }

  ////// 3. Reverse the sign of the per-vertex normals
  FOR_EACH_VERT(flip3shorts (v->nrm));
}

bool
CyraResLevel::filtered_copy(CyraResLevel &original,
			    const VertexFilter& filter)
{
  width  = original.width;
  height = original.height;
  numpoints = 0;
  numtris   = 0;
  points.clear();
  tris.clear();
  origin = original.origin;

  CyraSample zero;
  zero.vtx.set(0,0,0);
  zero.nrm[0] = 0; zero.nrm[1] = 0; zero.nrm[2] = 32767;
  zero.confidence = 0;
  zero.intensity  = 0;

  // Copy the vertices
  for (int i=0; i < original.points.size(); i++) {
    if (filter.accept(original.points[i].vtx)) {
      points.push_back(original.points[i]);
      numpoints++;
    } else {
      points.push_back(zero);
    }
  }

  // Copy the triangles
  for (int xx=0; xx < width-1; xx++) {
    for (int yy=0; yy < height-1; yy++) {
      bool conf00 = (point(xx,yy).confidence > 0);
      bool conf01 = (point(xx,yy+1).confidence > 0);
      bool conf10 = (point(xx+1,yy).confidence > 0);
      bool conf11 = (point(xx+1,yy+1).confidence > 0);
      // Compute new tess, minus possible vertices
      CyraTess tess;
      switch (original.tri(xx, yy)) {
      case TESS0:
	tess = TESS0;
	break;
      case TESS1:
	if (conf00 && conf10 && conf01) tess = TESS1;
	else tess = TESS0;
	break;
      case TESS2:
	if (conf00 && conf01 && conf11) tess = TESS2;
	else tess = TESS0;
	break;
      case TESS3:
	if (conf00 && conf10 && conf11) tess = TESS3;
	else tess = TESS0;
	break;
      case TESS4:
	if (conf01 && conf10 && conf11) tess = TESS4;
	else tess = TESS0;
	break;
      case TESS14:
      case TESS23:
	if (conf00 && conf01 && conf10 && conf11) tess = TESS14;
	else if (conf00 && conf01 && conf10) tess = TESS1;
	else if (conf00 && conf01 && conf11) tess = TESS2;
	else if (conf00 && conf10 && conf11) tess = TESS3;
	else if (conf01 && conf10 && conf11) tess = TESS4;
	else tess = TESS0;
	break;
      default:
	cerr << "Unhandled tesselation flag " << original.tri(xx,yy)
	     << " in clipping..." << endl;
      }
      // Push Tri flag, increment numtris
      tris.push_back(tess);
      if (tess == TESS14 || tess == TESS23) numtris+=2;
      else if (tess == TESS0) numtris += 0;
      else numtris ++;
    }
  }

  return true;
}


bool
CyraResLevel::filter_inplace(const VertexFilter& filter)
{
  numpoints = 0;
  numtris   = 0;

  CyraSample zero;
  zero.vtx.set(0,0,0);
  zero.nrm[0] = zero.nrm[1] = 0; zero.nrm[2] = 32767;
  zero.confidence = 0;
  zero.intensity  = 0;

  // Filter the vertices
  FOR_EACH_VERT(*v = (filter.accept(v->vtx) && ++numpoints)? *v : zero);

  // Copy the triangles
  for (int xx=0; xx < width-1; xx++) {
    for (int yy=0; yy < height-1; yy++) {
      bool conf00 = (point(xx,yy).confidence > 0);
      bool conf01 = (point(xx,yy+1).confidence > 0);
      bool conf10 = (point(xx+1,yy).confidence > 0);
      bool conf11 = (point(xx+1,yy+1).confidence > 0);
      // Compute new tess, minus possible vertices
      CyraTess tess;
      switch (tri(xx, yy)) {
      case TESS0:
	tess = TESS0;
	break;
      case TESS1:
	if (conf00 && conf10 && conf01) tess = TESS1;
	else tess = TESS0;
	break;
      case TESS2:
	if (conf00 && conf01 && conf11) tess = TESS2;
	else tess = TESS0;
	break;
      case TESS3:
	if (conf00 && conf10 && conf11) tess = TESS3;
	else tess = TESS0;
	break;
      case TESS4:
	if (conf01 && conf10 && conf11) tess = TESS4;
	else tess = TESS0;
	break;
      case TESS14:
      case TESS23:
	if (conf00 && conf01 && conf10 && conf11) tess = TESS14;
	else if (conf00 && conf01 && conf10) tess = TESS1;
	else if (conf00 && conf01 && conf11) tess = TESS2;
	else if (conf00 && conf10 && conf11) tess = TESS3;
	else if (conf01 && conf10 && conf11) tess = TESS4;
	else tess = TESS0;
	break;
      default:
	cerr << "Unhandled tesselation flag " << tri(xx,yy)
	     << " in clipping..." << endl;
      }
      // Push Tri flag, increment numtris
      tri(xx,yy) = tess;
      if (tess == TESS14 || tess == TESS23) numtris+=2;
      else if (tess == TESS0) numtris += 0;
      else numtris ++;
    }
  }

  isDirty_cache = true;
  return true;
}


void
CyraResLevel::create_kdtree(void)
{
  // If everything built, no-op.
  if ((!isDirty_cache) && (kdtree)) return;

  // otherwise, rebuild
  if (kdtree) delete kdtree;
  cachedPoints.clear();
  cachedNorms.clear();
  cachedBoundary.clear();

  // re-assemble cachedPoints, norms, boundary
  cachedPoints.reserve(num_vertices());
  cachedNorms.reserve(num_vertices() * 3);
  cachedBoundary.reserve(num_vertices());
  FOR_EACH_VERT(cachedPoints.push_back(v->vtx));
  FOR_EACH_VERT(cachedNorms.insert (cachedNorms.end(), v->nrm, v->nrm + 3));
  FOR_EACH_VERT(cachedBoundary.push_back
		((bool) (vx == 0 || vx == width-1 ||
			 vy == 0 || vy == height-1 ||
			 (tri(vx-1,vy-1) != TESS14 &&
			  tri(vx-1,vy-1) != TESS23 &&
			  tri(vx-1,vy-1) != TESS4) ||
			 (tri(vx-1,vy) != TESS14 &&
			  tri(vx-1,vy) != TESS23 &&
			  tri(vx-1,vy) != TESS3) ||
			 (tri(vx,vy-1) != TESS14 &&
			  tri(vx,vy-1) != TESS23 &&
			  tri(vx,vy-1) != TESS2) ||
			 (tri(vx,vy) != TESS14 &&
			  tri(vx,vy) != TESS23 &&
			  tri(vx,vy) != TESS1))));

  kdtree = CreateKDindtree (cachedPoints.begin(), cachedNorms.begin(),
			    cachedPoints.size());

  isDirty_cache = false;
}

// for ICP...
void
CyraResLevel::subsample_points(float rate, vector<Pnt3> &p, vector<Pnt3> &n)
{
  int nv = num_vertices();
  int totalNum = (int)(rate * nv);

  if (totalNum > nv) return;

  p.clear(); p.reserve(totalNum);
  n.clear(); n.reserve(totalNum);

  int num = totalNum;
  int end = nv;
  for (int i = 0; i < end; i++) {
    if (random()%nv < num) {
      p.push_back(points[i].vtx);    // save point
      pushNormalAsPnt3 (n, points[i].nrm, 0);
      num--;
    }
    nv--;
  }
  assert(num == 0);
  if (p.size() != totalNum) {
    printf("Selected wrong number of points in the CyraResLevel subsample proc.\n");
  }
}

bool
CyraResLevel::closest_point(const Pnt3 &p, const Pnt3 &n,
			Pnt3 &cp, Pnt3 &cn,
			float thr, bool bdry_ok)
{
  create_kdtree();
  int ind, ans;
  ans = kdtree->search(&cachedPoints[0],
		       &cachedNorms[0], p, n, ind, thr);
  if (ans) {
    if (!bdry_ok) {
      // disallow closest points that are on the mesh boundary
      if (cachedBoundary[ind]) return 0;
    }
    cp = cachedPoints[ind];
    short *sp = &cachedNorms[ind*3];
    cn.set(sp[0]/32767.0,
	   sp[1]/32767.0,
	   sp[2]/32767.0);
  }
  return ans;
}

