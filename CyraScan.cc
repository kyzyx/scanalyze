//############################################################
//
// CyraScan.cc
//
// Lucas Pereira
// Thu Jul 16 15:20:47 1998
//
// Store range scan information from a Cyra Cyrax Beta
// time-of-flight laser range scanner.
//
//############################################################

#include "CyraScan.h"
#include <stdio.h>
#include <iostream.h>
#include <fstream.h>
#include <stack.h>
#include "defines.h"
#include "TriMeshUtils.h"
#include "KDindtree.h"
#include "ColorUtils.h"
#include "plvScene.h"
#include "KDtritree.h"
#include "Timer.h"
#include "MeshTransport.h"


//////////////////////////////////////////////////////////////////////
// CyraScan (all resolution levels for a cyra scan)
//////////////////////////////////////////////////////////////////////

CyraScan::CyraScan(void)
  : triKD(NULL), triKD_mesh(NULL)
{
  // Clear arrays
  levels.clear();
}


CyraScan::~CyraScan ()
{
  delete triKD;
  delete triKD_mesh;
  /*
    <Writing stuff deleted>
    delete kdtree;
  */
}

// perVertex: colors and normals for every vertex (not every 3)
// stripped: triangle strips instead of triangles
MeshTransport*
CyraScan::mesh(bool perVertex, bool stripped,
	       ColorSource color, int colorSize)
{
  if (stripped && !perVertex) {
    cerr << "No t-strips without per-vertex properties";
    return NULL;
  }

  int resNum = current_resolution_index();
  if (resNum >= levels.size())
    return NULL;

  return levels[resNum].mesh (perVertex, stripped, color, colorSize);
}


int
CyraScan::num_vertices()
{
  // Return num_vertices for current resolution
  return(num_vertices(current_resolution_index()));
}

int
CyraScan::num_tris()
{
  // Return num_tris for current resolution
  return(num_tris(current_resolution_index()));
}

int
CyraScan::num_vertices(int resNum)
{
  if (resNum < levels.size()) {
    return(levels[resNum].num_vertices());
  } else {
    return 0;
  }
}


int
CyraScan::num_tris(int resNum)
{
  if (resNum < levels.size()) {
    return(levels[resNum].num_tris());
  } else {
    return 0;
  }
}


void
CyraScan::computeBBox(void)
{

  bbox.clear();
  int resNum = current_resolution_index();

  // Add current level to the bbox
  // Also set the center of rotation
  if (resNum < levels.size()) {
    levels[resNum].growBBox(bbox);
    rot_ctr = bbox.center();
  } else {
    rot_ctr = Pnt3();
  }
}

void
CyraScan::flipNormals(void)
{
  // Flip all res levels
  for (int i=0; i < levels.size(); i++) {
    levels[i].flipNormals();
  }
}


bool
CyraScan::ReadPts(const crope &inname)
{
  // read it into a new level
  CyraResLevel *level = new CyraResLevel();
  if (level->ReadPts(inname)) {
    levels.push_back(*level);
  } else {
    return false;
  }

  // Tell scanalyze about the mesh...
  insert_resolution(level->num_tris(), inname, true, true);
  cerr << "Telling scanalyze about " << level->num_tris() << " tris." << endl;

  // Now subsample 5 lower-res versions.
  for (int i = 1; i < 6; i++) {
    CyraResLevel *sublevel = new CyraResLevel();
    levels.push_back(*sublevel);

    // Tell scanalyze about the mesh...
    int estimate = levels[0].num_tris() / (1 << (2*i));
    insert_resolution(estimate, inname, false, false);
    cerr << "Telling scanalyze about approx. " << estimate
	 << " tris." << endl;
  }

  // Recompute the bounding box
  computeBBox();

  // default to displaying coarsest res
  select_coarsest();
  return true;
}


bool
CyraScan::WritePts(const crope &inname)
{
  // write out the highest level to the .pts file
  return(levels[0].WritePts(inname));
}


bool
CyraScan::load_resolution (int iRes)
{
  if (resolutions[iRes].in_memory)
    return true;

  int subSamp = 1;
  for (int i = 0; i < iRes; i++)
    subSamp *= SubSampleBase;

  CyraResLevel *sublevel = new CyraResLevel();
  sublevel->Subsample(levels[0], subSamp, subSamp, cfMEAN50);

  //delete levels[iRes];
  levels[iRes] = *sublevel;
  resolutions[iRes].in_memory = true;
  resolutions[iRes].abs_resolution = sublevel->num_tris();
  computeBBox();

  return true;
}


bool
CyraScan::read(const crope &inname)
{
  set_name (inname);
  assert(has_ending(".pts"));

  // read it into the levels array
  if (!ReadPts(inname))
    return false;

  // read registration xform?
  TbObj::readXform (get_basename());

  return true;
}


bool
CyraScan::write(const crope &fname)
{
  cerr << "CyraScan::write was called.\n";

  if (fname.empty()) {
    // try to save to default name; quit if there isn't one
    if (name.empty()) return false;
  }
  else {
    if (name != fname) {
      cout << "Saving to filename " << fname << endl;
      set_name(fname);
    }
  }

  // write .pts, or fail....
  if (has_ending(".pts")) {
    return(WritePts(name));
  } else {
    cerr << "Error: CyraScan doesn't know how to write " <<
      "that kind of file." << endl;
    return false;
  }
}

bool
CyraScan::filter_inplace(const VertexFilter& filter)
{
  // Filter each level
  for (int i=0; i < levels.size(); i++) {
    CyraResLevel *level = &levels[i];
    level->filter_inplace(filter);
  }
  // BUGBUG update_res_ctrl();
  // Need to update the number of tris and stuff,
  computeBBox();
  theScene->invalidateDisplayCaches();

  return true;
}


RigidScan*
CyraScan::filtered_copy(const VertexFilter& filter)
{
  CyraScan *newScan = new CyraScan;
  assert (newScan != NULL);

  // Filter each level into the newScan
  for (int i=0; i < levels.size(); i++) {
    CyraResLevel *newLevel = new CyraResLevel;
    assert(newLevel != NULL);

    newLevel->filtered_copy(levels[i], filter);
    newScan->levels.push_back(*newLevel);
  }

  // Compute new boundingbox
  newScan->computeBBox();

  // Set the name of newScan
  crope clipName(get_basename());
  char info[50];
  sprintf(info, "clip.%d.cyra", newScan->num_vertices());
  clipName += info;

  // Tell scanalyze about the meshes...
  for (i=0; i < newScan->levels.size(); i++) {
    CyraResLevel *level = &newScan->levels[i];
    newScan->insert_resolution(level->num_tris(), clipName, true, true);
    cerr << "Telling scanalyze about " << level->num_tris()
	 << " tris." << endl;
  }

  return newScan;
}

// for ICP...
void
CyraScan::subsample_points(float rate, vector<Pnt3> &p, vector<Pnt3> &n)
{
  CyraResLevel& res = levels[curr_res];
  res.subsample_points(rate, p, n);
}

bool
CyraScan::closest_point(const Pnt3 &p, const Pnt3 &n,
			Pnt3 &cp, Pnt3 &cn,
			float thr, bool bdry_ok)
{
  CyraResLevel& res = levels[curr_res];
  return res.closest_point(p, n, cp, cn, thr, bdry_ok);
}




////////////////////////////////////////
// for volumetric processing
////////////////////////////////////////

float
CyraScan::closest_point_on_mesh (const Pnt3 &p, Pnt3 &cl_pnt,
				 OccSt &status_p)
{
  static Pnt3  prev_point(1e33, 1e33, 1e33);

  if (triKD == NULL) {
    // create the triangle spheretree for finding the distance to the mesh
    cout << "Obtaining geometry for KDtritree..." << flush;
    triKD_mesh = levels[0].mesh (true, false, RigidScan::colorNone, 0);
    cout << "building tree..." << flush;
    triKD = create_KDtritree(triKD_mesh->vtx[0]->begin(),
			     triKD_mesh->tri_inds[0]->begin(),
			     triKD_mesh->tri_inds[0]->size());
    cout << "done." << endl;
  }

  OccSt st;
  closest_along_line_of_sight(p, cl_pnt, st);
  float d   = dist2(p, cl_pnt);
  float dp2 = dist2(prev_point, p);
  if (d > dp2) {
    d = dp2;
    cl_pnt = prev_point;
  }
  d = sqrtf(d);
  triKD->search(triKD_mesh->vtx[0]->begin(),
		triKD_mesh->tri_inds[0]->begin(),
		p, cl_pnt, d);
  prev_point = cl_pnt;

  // just a simple, hacky test for occlusion status
  // assume that the camera center is at origin
  //if (p.norm2() < cl_pnt.norm2()) status_p = OUTSIDE;
  //else                            status_p = INSIDE;
  status_p = st;

  return 1.0;
}

//////////////////////////////////////////////////////////////////////
// magi's hack implementation for vrip member functions follows...
//////////////////////////////////////////////////////////////////////

float
CyraScan::closest_along_line_of_sight(const Pnt3 &p, Pnt3 &cp,
				      OccSt &status_p)
{
  build_vrip_accelerators();
  CyraResLevel& res = levels[0];

  status_p = INDETERMINATE;
  // lookup given los direction in ray bins
  Pnt3 dir = p; dir.normalize();
  if (dir[0] < minx || dir[0] > maxx) {
    //cout << "out of bounds x" << endl;
    return 0.0;
  }
  if (dir[1] < miny || dir[1] > maxy) {
    //cout << "out of bounds y" << endl;
    return 0.0;
  }

  int binx = (dir[0] - minx) * binw;
  int biny = (dir[1] - miny) * binh;

  if (!bins[biny][binx].size()) {
    if (binx > 1)
      binx--;
    else
      if (biny > 1)
	biny--;
      else
	return 0.0;

    if (!bins[biny][binx].size())
      return 0.0;
  }

  // have a valid bin
  int ofs = bins[biny][binx].back();
  if (!pntmag[ofs])
    return 0.0;

  int x = ofs / res.height;
  int y = ofs % res.height;
  Pnt3& pd = pntdir[ofs];
  int l, r, t, b;
  if (dir[0] < pd[0]) {
    l = x - 1; // needs help
    while (pntdir [l * res.height + y][0] > dir[0]) {
      if (--l < 0)
	return 0.0;
    }
    r = l + 1;
  } else {
    r = x + 1; // needs help
    while (pntdir [r * res.height + y][0] < dir[0]) {
      if (++r >= res.width)
	return 0.0;
    }
    l = r - 1;
  }
  if (dir[1] < pd[1]) {
    b = y - 1; // needs help
    while (pntdir [l * res.height + b][1] > dir[1]) {
      if (--b < 0)
	return 0.0;
    }
    t = b + 1;
  } else {
    t = y + 1; // needs help
    while (pntdir [l * res.height + t][1] < dir[1]) {
      if (++t >= res.height)
	return 0.0;
    }
    b = t - 1;
  }

  int ofss[4] = { t + l*res.height, t + r*res.height,
		  b + l*res.height, b + r*res.height };
  for (int ii = 0; ii < 4; ii++) {
    if (ofss[ii] < 0 || ofss[ii] >= pntdir.size() || !pntmag[ofss[ii]])
      return 0.0;
  }

  //SHOW (x); SHOW (y);
  //SHOW (l); SHOW (r); SHOW (t); SHOW (b);

  // bilerp across (l, b), (r, b), (l, t), (r, t)
  Pnt3& p00 = res.points[b + l*res.height].vtx;
  Pnt3& p01 = res.points[t + l*res.height].vtx;
  Pnt3& p10 = res.points[b + r*res.height].vtx;
  Pnt3& p11 = res.points[t + r*res.height].vtx;

  Pnt3& n00 = pntdir[b + l*res.height];
  Pnt3& n01 = pntdir[t + l*res.height];
  Pnt3& n10 = pntdir[b + r*res.height];
  Pnt3& n11 = pntdir[t + r*res.height];

  float ix = (n11[0] - n01[0]);
  ix = (dir[0] - n01[0]) / ix;

  float iyl = (n01[1] - n00[1]);
  iyl = (dir[1] - n00[1]) / iyl;

  float iyr = (n11[1] - n10[1]);
  iyr = (dir[1] - n10[1]) / iyr;

  Pnt3 L,R;
  L.lerp(iyl, p01, p00);
  R.lerp(iyr, p11, p10);
  cp.lerp(ix, R, L);

  //SHOW (ix); SHOW (iyl); SHOW (iyr);
  //SHOW (L); SHOW (R); SHOW (cp);

  float ind  = p.norm2();
  float outd = cp.norm2();
  if (ind < outd)
    status_p = OUTSIDE;
  else
    status_p = INSIDE;

  return 1.0;
}


static const float g_intersectTolerance = 1.e-5;
bool
OriginRaySphereIntersect (const Pnt3& rayD, const Pnt3& center,
			  float sphereRad, float& t1, float& t2)
{
  //from Heckbert, "Writing a Ray Tracer", in Glassner p. 280
  //int nroots;
  float b, disc;

  b = dot (center, rayD);
  disc = b*b - dot(center,center) + sphereRad*sphereRad;
  if (disc < 0.) return false;                    // doesn't hit
#ifdef WIN32
  disc = sqrt(disc);
#else
  disc = sqrtf(disc);
#endif
  t2 = b + disc;                              // 2nd root...
  if (t2 < g_intersectTolerance) return false;    // behind ray origin
  t1 = b - disc;                              // 1st root...

  return true;
}

#define HISTORY 1
#if HISTORY
struct carve_cache {
  Pnt3  ctr;
  float radius;
  vector<int> pinds;

  bool inside(const Pnt3 &p, float r)
    {
      return dist(p,ctr)+r <= radius;
    }

#if _MSC_VER == 1100
  // this compiler (MSVC5) won't compile without being able to compare carve_caches!
  bool operator< (const carve_cache& that) const
    {
      abort();
      return 0;
    }

  bool operator== (const carve_cache& that) const
    {
      abort();
      return 0;
    }
#endif
};

// carve history
static stack<carve_cache> hist;
#endif

static vector<int> dummy;

OccSt
CyraScan::carve_cube  (const Pnt3 &ctr, float side)
{
  build_vrip_accelerators();
  int numpoints = pntdir.size();

#if HISTORY
  while (!hist.empty() && !hist.top().inside(ctr, side)) {
    hist.pop();
  }

  carve_cache cc;
  cc.ctr = ctr;
  //cc.radius = (side/2.0) * sqrt(3.0); // cube side to sphere radius
  cc.radius = side * 0.866025404;
  vector<int> &oldinds = dummy;
  bool first = hist.empty();
  if (!first) oldinds = hist.top().pinds;
  hist.push(cc);
  if (first)  hist.top().pinds.reserve(numpoints);
  else        hist.top().pinds.reserve(oldinds.size());
#endif

  bool bBefore = false;
  bool bAfter = false;

  //cout << "CarveCube:" << endl;
  //SHOW (ctr);
  //SHOW (side);

  // figure out which rays it's worth intersecting with
  // take incoming bounding sphere, and figure out where it is on
  // the [0..1] scale that we've normalized point-rays to
  Pnt3 carvedir = ctr;
  float carvemag = carvedir.norm();
  carvedir /= carvemag;
  float carverad = cc.radius / carvemag;

  // now only keep rays that hit the square that bounds this circle in 2-d
  float l = carvedir[0] - carverad, r = carvedir[0] + carverad;
  float b = carvedir[1] - carverad, t = carvedir[1] + carverad;
  l = max (l, minx); r = min (r, maxx);
  b = max (b, miny); t = min (t, maxy);
  if (l > r || b > t) return NOT_IN_FRUSTUM;

  // look up the rays that fit in here
  int binl = (l - minx) * binw;
  int binr = (r - minx) * binw;
  int binb = (b - miny) * binh;
  int bint = (t - miny) * binh;

#if HISTORY
  if (first) {
    cout << numpoints << ": ";
#endif
    for (int binx = binl; binx <= binr; binx++) {
      for (int biny = binb; biny <= bint; biny++) {
	vector<int>& bin = bins[biny][binx];
	int binsize = bin.size();
	for (int inbin = 0; inbin < binsize; inbin++) {
	  int p = bin[inbin];

	  float mag = pntmag[p];
	  if (!mag) continue;
	  Pnt3& dir = pntdir[p];

	  float t1, t2;
	  if (OriginRaySphereIntersect (dir, ctr, side, t1, t2)) {
#if HISTORY
	    hist.top().pinds.push_back(p);
#endif
	    // if mag is between t1 and t2, we hit:
	    if (mag < t1)
	      bBefore = true;
	    else if (mag > t2)
	      bAfter = true;
	    else {
	      //SHOW (p); SHOW (t1); SHOW (mag); SHOW (t2);
	      cout << "BOUND for point " << p << " of " << numpoints << endl;
	      // leave the rest of the points into cache
#if HISTORY
	      for (; binx <= binr; binx++) {
		for (biny += 1; biny <= bint; biny++) {
		  vector<int>& bin = bins[biny][binx];
		  int binsize = bin.size();
		  for (int inbin = 0; inbin < binsize; inbin++) {
		    hist.top().pinds.push_back (bin[inbin]);
		  }
		}
	      }
#endif
	      return BOUNDARY;
	    }
	  }
	}
      }
    }
#if HISTORY
  } else {
    cout << oldinds.size() << ": ";
// STL Update
    for (vector<int>::iterator it = oldinds.begin(); it != oldinds.end(); it++) {
      float mag = pntmag[*it];
      if (!mag) continue;

      Pnt3& dir = pntdir[*it];

      float t1, t2;
      if (OriginRaySphereIntersect (dir, ctr, side, t1, t2)) {
	hist.top().pinds.push_back(*it);
	// if mag is between t1 and t2, we hit:
	if (mag < t1)
	  bBefore = true;
	else if (mag > t2)
	  bAfter = true;
	else {
	  //SHOW (p); SHOW (t1); SHOW (mag); SHOW (t2);
	  cout << "BOUND for point " << *it << " of " << numpoints << endl;
	  // leave the rest of the points into cache
	  hist.top().pinds.insert(hist.top().pinds.end(), ++it, oldinds.end());
	  return BOUNDARY;
	}
      }
    }
  }

  hist.push(cc);
#endif
  if (bAfter && bBefore) {
    cout << "SILH" << endl;
    return SILHOUETTE;
  } else if (bAfter) {
    cout << "IN" << endl;
    return INSIDE;
  } else if (bBefore) {
    cout << "OUT" << endl;
   return OUTSIDE;
  }

  cout << "NOT_IN_FRUSTUM" << endl;
  return NOT_IN_FRUSTUM;
}

/*
// for debugging...
void
CyraScan::TriOctreeMesh(vector<Pnt3> &p, vector<int> &ind)
{
  if (trioct == NULL) {
    // force creation
    Pnt3 p1, p2;
    OccSt occ;
    closest_point(p1, p2, occ);
  }
  p.clear();
  ind.clear();
  //trioct->visualize(p,ind);
}
*/


void
CyraScan::build_vrip_accelerators (void)
{
  CyraResLevel& res = levels[0];
  // if res.pntdir hasn't been initialized, do it now
  if (pntdir.size() == 0) {
    TIMER (Cyra_Vrip_Preprocess);
    int np = res.points.size();
    pntdir.reserve (np);
    pntmag.reserve (np);

    // separate point directions and magnitues, and find bounds
    minx = 1e33; miny = 1e33; maxx = -1e33; maxy = -1e33;
    Pnt3 p;
    float mag;
    for (int i = 0; i < np; i++) {
      p = res.points[i].vtx;
      mag = p.norm2();
      if (mag) {
	mag = sqrtf (mag);
	p /= mag;
      }

      pntdir.push_back (p);
      pntmag.push_back (mag);

      minx = min (minx, p[0]); maxx = max (maxx, p[0]);
      miny = min (miny, p[1]); maxy = max (maxy, p[1]);
    }

    // build bins
    float w = maxx - minx;
    float h = maxy - miny;

    binw = (res.width - 1) / w;
    binh = (res.height - 1) / h;

    vector<int> _bin1;
    // JED - changed the following to avoid compiler complaint
    //vector< vector< int> > _bins1;
    //_bins1.assign (res.width, _bin1);
    //bins.assign (res.height, _bins1);
    vector< vector< int> > _bins1(res.width, _bin1);
    vector< vector< vector <int> > > tmp(res.height, _bins1);
    bins = tmp;

    for (int ip = 0; ip < np; ip++) {
      if (pntmag[ip] == 0) continue;

      const Pnt3& p = pntdir[ip];
      int binx = (p[0] - minx) * binw;
      int biny = (p[1] - miny) * binh;

      assert (binx >= 0 && binx < res.width);
      assert (biny >= 0 && biny < res.height);

      bins[biny][binx].push_back (ip);
    }
  }
}
