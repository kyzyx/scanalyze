//############################################################
//
// CyberScan.cc
//
// Kari Pulli
// Wed Jul  8 13:15:25 PDT 1998
//
// Store range scan information from a Cyberware custom made
// range scanner for the Digital Michelangelo project.
//
//############################################################

#include <stdio.h>
#include <string.h>
#include "DirEntries.h"
#include <fstream>
#ifdef WIN32
#	include <float.h>
#endif
#ifdef sgi
#	include <ieeefp.h>
#endif
#ifdef linux
#       define MAXFLOAT FLT_MAX
#endif
#include "CyberScan.h"
#include "plvGlobals.h"
#include "plvDraw.h"
#include "KDindtree.h"
#include "Random.h"
#include "TriMeshUtils.h"
#include "defines.h"
#include "FileNameUtils.h"
#include "Bbox.h"
#include "Progress.h"
#include "BailDetector.h"
#include "ColorUtils.h"
#include <algorithm>
#include "MeshTransport.h"
#include "VertexFilter.h"

#define SUBSAMPS 7

static ColorSet _cset;

KDindtree*
CyberScan::get_current_kdtree()
{
  int iTree = current_resolution_index();
  assert (iTree < kdtree.size());
  if (kdtree[iTree] != NULL)
    return kdtree[iTree];

  regLevelData* level = getCurrentRegLevel();
// STL Update
  kdtree[iTree] = CreateKDindtree(level->pnts->begin(),
				  level->nrms->begin(),
				  level->pnts->size());

  return kdtree[iTree];
}


void
CyberScan::read_postprocess(const char *filename)
{
  for (int i = 0; i < SUBSAMPS; i++) {
    char name[PATH_MAX];
    strcpy (name, filename);
    char* ext = strrchr (name, '.');
    if (ext == NULL)
      ext = name + strlen (name);
    sprintf (ext, "_samp%d.ply", 1<<i);

    int nFacesEstimated = 0;
    for (int j = 0; j < sweeps.size(); j++) {
      nFacesEstimated += sweeps[j]->resolutions[i].abs_resolution;
    }

    insert_resolution(nFacesEstimated, name, false, false);
    kdtree.push_back (NULL);
    reglevels.push_back (NULL);
  }

  // by default, start at lowest res

  for (int i=0; i<sweeps.size(); i++)
    sweeps[i]->select_coarsest();
  select_coarsest();

  // fill holes if requested

  if (!strcmp(Tcl_GetVar(g_tclInterp, "subsamplePreserveHoles",
	                 TCL_GLOBAL_ONLY), "filter")) {
	for (int i=0; i<sweeps.size(); i++) {
	  sweeps[i]->sd.fill_holes(2, 30);
	  sweeps[i]->sd.fill_holes(1, 20);
	}
  }
}


CyberScan::CyberScan()
{
  bDirty = false;
}


CyberScan::~CyberScan ()
{
  while (reglevels.size()) {
    delete reglevels.back();
    reglevels.pop_back();
  }

  while (sweeps.size()) {
    delete sweeps.back();
    sweeps.pop_back();
  }

  while (kdtree.size()) {
    delete kdtree.back();
    kdtree.pop_back();
  }
}


MeshTransport*
CyberScan::mesh(bool perVertex, bool stripped,
		ColorSource color, int colorSize)
{
  if (stripped && !perVertex) {
    cerr << "No t-strips without per-vertex properties";
    return NULL;
  }

  /* kberg - trying to get per face flat shading working
  if (!perVertex) {
    cerr << "No per face normals for CyberScan" << endl;
    return NULL;
  }
  */

  int i = current_resolution_index();
  load_resolution (i);

  MeshTransport *mt = new MeshTransport;
  for (int iTurn = 0; iTurn < sweeps.size(); iTurn++) {
    MeshTransport* turn = sweeps[iTurn]->mesh (perVertex, stripped, color,
					       colorSize);
    if (turn) {
      mt->appendMT (turn, sweeps[iTurn]->getXform());
      delete turn;
    }
  }

  return mt;
}


int
CyberScan::num_vertices(void)
{
  return getCurrentRegLevel()->pnts->size();
}


regLevelData*
CyberScan::getHighestRegLevel (void)
{
  if (!resolutions[0].in_memory)
    load_resolution (0);

  return getRegLevelFor (0);
}


regLevelData*
CyberScan::getCurrentRegLevel (void)
{
  return getRegLevelFor (current_resolution_index());
}


regLevelData*
CyberScan::getRegLevelFor (int iRes)
{
  assert (iRes < reglevels.size());
  load_resolution (iRes);
  assert (resolutions[iRes].in_memory);

  if (!reglevels[iRes]) {
    regLevelData* rl = reglevels[iRes] = new regLevelData;

    if (sweeps.size() == 1) {
      // don't actually need merge, so don't copy -- just share
      rl->pnts = &sweeps[0]->levels[iRes]->pnts;
      rl->nrms = &sweeps[0]->levels[iRes]->nrms;
      rl->bdry = &sweeps[0]->levels[iRes]->bdry;
      rl->bFree = false;
    } else {
      // need to create merged copy
      // preallocate vectors
      int nPts = 0;
      for (int i = 0; i < sweeps.size(); i++) {
	nPts += sweeps[i]->levels[iRes]->pnts.size();
      }
      cout << "Merging sweep data (" << nPts
	   << " pts) for KDtree... " << flush;
      rl->pnts = new vector<Pnt3>;  rl->pnts->reserve (nPts);
      rl->nrms = new vector<short>; rl->nrms->reserve (nPts);
      rl->bdry = new vector<char>;  rl->bdry->reserve (nPts);
      rl->bFree = true;

      for (int i = 0; i < sweeps.size(); i++) {
	levelData *ld = sweeps[i]->levels[iRes];
	long ps = rl->pnts->size();
	long ns = rl->nrms->size();

	rl->pnts->insert (rl->pnts->end(),
			  ld->pnts.begin(), ld->pnts.end());
	rl->nrms->insert (rl->nrms->end(),
			  ld->nrms.begin(), ld->nrms.end());
	rl->bdry->insert (rl->bdry->end(),
			  ld->bdry.begin(), ld->bdry.end());

	Xform<float> sxf = sweeps[i]->getXform();
	if (!sxf.isIdentity()) {
	  // need to apply sweep's transform to the new data
	  for_each (rl->pnts->begin() + ps, rl->pnts->end(), sxf);
	  sxf.removeTranslation();
// STL Update
	  for (vector<short>::iterator nb = rl->nrms->begin() + ns;
	       nb < rl->nrms->end(); nb += 3) {
	    Pnt3 n (nb[0], nb[1], nb[2]);
	    sxf (n);
	    nb[0] = n[0]; nb[1] = n[1]; nb[2] = n[2];
	  }
	}
      }
    }
  }

  return reglevels[iRes];
}


static Random rnd;

void
CyberScan::subsample_points(float rate, vector<Pnt3> &p,
			    vector<Pnt3> &n)
{
  regLevelData* level = getCurrentRegLevel();

  int end = level->pnts->size();
  p.clear(); p.reserve(end * rate * 1.1);
  n.clear(); n.reserve(end * rate * 1.1);
  for (int i = 0; i < end; i++) {
    if (rnd() <= rate) {
      p.push_back (level->pnts->operator[](i));    // save point
      pushNormalAsPnt3 (n, level->nrms->begin(), i);
    }
  }
}


RigidScan*
CyberScan::filtered_copy (const VertexFilter &filter)
{
  BailDetector bail;
  CyberScan *newScan = new CyberScan;
  newScan->bDirty = true;

  cout << "Clip " << sweeps.size() << " sweeps: " << flush;
  for (int i = 0; i < sweeps.size(); i++) {
    RigidScan* newSweep = sweeps[i]->filtered_copy(filter);
    if (newSweep) {
      CyberSweep *cs = dynamic_cast<CyberSweep*> (newSweep);
      assert(cs);
      newScan->sweeps.push_back(cs);
    }
    cout << "." << flush;

    if (bail()) {
      cerr << "Warning: clip interrupted; results are partial" << endl;
      break;
    }
  }
  cout << " " << newScan->sweeps.size() << " made it." << endl;

  if (newScan->sweeps.size()) {
    newScan->read_postprocess(get_name().c_str());
    newScan->setXform (getXform());
  }

  return newScan;
}


RigidScan*
CyberScan::get_piece (int sweepNumber, int frameStart, int frameFinish)
{
  if (sweepNumber >= sweeps.size()) {
    return NULL;
  }

  RigidScan* newSweep =
    sweeps[sweepNumber]->get_piece(frameStart, frameFinish);

  if (newSweep == NULL)
    return NULL;

  CyberSweep *cs = dynamic_cast<CyberSweep*> (newSweep);
  assert(cs);

  CyberScan *newScan = new CyberScan;
  newScan->sweeps.push_back(cs);
  newScan->read_postprocess(get_name().c_str());
  newScan->setXform (getXform());

  return newScan;
}


bool
CyberScan::filter_inplace (const VertexFilter &filter)
{
  return false;
}


bool
CyberScan::filter_vertices (const VertexFilter &filter,
			    vector<Pnt3>& p)
{
  for (int i=0; i<sweeps.size(); i++) {
    CyberSweep *cs = sweeps[i];
    int level = cs->current_resolution_index();
    vector<Pnt3> &pnts = cs->levels[level]->pnts;
    vector<Pnt3>::const_iterator it;
    for (it = pnts.begin(); it != pnts.end(); it++) {
      if (filter.accept(*it)) p.push_back(*it);
    }
  }
  return true;
}


bool
CyberScan::closest_point(const Pnt3 &p, const Pnt3 &n,
			 Pnt3 &cp, Pnt3 &cn,
			 float thr, bool bdry_ok)
{
  KDindtree* tree = get_current_kdtree();
  if (!tree) return false;

  int ind, ans;

  regLevelData* level = getCurrentRegLevel();
  ans = tree->search(level->pnts->begin(), level->nrms->begin(),
		     p, n, ind, thr);
  if (ans) {
    if (bdry_ok == 0) {
      // disallow closest points that are on the mesh boundary
      if (level->bdry->operator[](ind)) return 0;
    }
    cp = level->pnts->operator[](ind);
    ind *= 3;
    cn.set(level->nrms->operator[](ind  )/32767.0,
	   level->nrms->operator[](ind+1)/32767.0,
	   level->nrms->operator[](ind+2)/32767.0);
  }
  return ans;
}


void
CyberScan::computeBBox ()
{
  bbox.clear();

  for (int i = 0; i < sweeps.size(); i++) {
    sweeps[i]->computeBBox();
    if (sweeps[i]->bbox.valid())
      bbox.add (sweeps[i]->bbox.worldBox (sweeps[i]->getXform()));
  }

  if (bbox.valid()) {
    rot_ctr = bbox.center();
  } else {
    rot_ctr = Pnt3();
  }
}


void
CyberSweep::computeBBox ()
{
  bbox.clear();

  // find highest built resolution
  levelData* level = NULL;
  for (int i = 0; i < levels.size(); i++) {
    if (levels[i] != NULL && levels[i]->pnts.size() > 0) {
      level = levels[i];
      break;
    }
  }

  // and add its points to bbox
  if (level != NULL) {
// STL Update
    for (vector<Pnt3>::iterator p = level->pnts.begin(); p < level->pnts.end(); p++) {
      bbox.add(*p);
    }
  }
}


void
CyberScan::flipNormals (void)
{
  for (int i = 0; i < sweeps.size(); i++)
    sweeps[i]->flipNormals();
}


void
CyberSweep::flipNormals (void)
{
  for (int i = 0; i < levels.size(); i++) {
    if (!resolutions[i].in_memory)
      continue;

    for (int j = 0; j < levels[i]->nrms.size(); j++)
      levels[i]->nrms[j] = -levels[i]->nrms[j];
    flip_tris(levels[i]->tstrips, true);
  }
}


bool
CyberScan::read(const crope &fname)
{
  cout << "cs::read()" << endl;
  set_name (fname);
  const char* filename = fname.c_str();
  assert(has_ending(".sd") || has_ending(".sd.gz"));

  // is filename a directory containing lots of little .sd files?
  bool bReadDir = false;

  SHOW(filename);

  DirEntries de(filename, ".sd");

  if (de.isdir)  {    // If it's a directory

    bReadDir = true;

    {
      Progress progress (de.size(), "Read CyberSweeps");

      for (; !de.done(); de.next()) {
        CyberSweep *ct = new CyberSweep;
        if (ct->read(de.path().c_str()))
    	  sweeps.push_back(ct);
        else
	  delete ct;
        progress.updateInc();
      }
    }

    DirEntries gzde(filename, ".sd.gz");

    {
      Progress progress (gzde.size(), "Read CyberSweeps");

      for (; !gzde.done(); gzde.next()) {
        CyberSweep *ct = new CyberSweep;
        if (ct->read(gzde.path().c_str()))
    	  sweeps.push_back(ct);
        else
	  delete ct;
        progress.updateInc();
      }
    }

  } else {                           // If it's a single file
    CyberSweep *ct = new CyberSweep;
    if (ct->read(filename))
      sweeps.push_back(ct);
    else
      delete ct;
  }

  // If we didn't successfully read any .sd files, return error

  if (sweeps.size() == 0)
    return false;

  set_name(fname);

  // read registration xform?
  if (TbObj::readXform (get_basename())) {
    // if we read a single file instead of a directory,
    // sweep will have read the same .xf as we do;
    // don't want it doubly applied so nuke it from sweep.
    if (!bReadDir) {
      assert (sweeps.size() == 1);
      sweeps[0]->setXform (Xform<float>());
      clear_undo (sweeps[0]);
    }
  }

  clear_undo (this);

  read_postprocess(filename);
  return true;
}


bool
CyberScan::is_modified (void)
{
  return bDirty;
}


bool
CyberScan::write(const crope &fname)
{
   char sweepfn[PATH_MAX];

   if (fname.empty()) {
      // try to save to default name; quit if there isn't one
      if (name.empty()) return false;
   }
   else {
      if (name != fname)  {
         cout << "Saving to filename " << fname << endl;
         set_name(fname);
      }
   }

   if (portable_mkdir(name.c_str(), 0775) == 0)  {

      Progress progress (sweeps.size(), "Writing CyberScan");

      for (int i=0; i<sweeps.size(); i++)  {
         sprintf(sweepfn, "%s/%d.sd", name.c_str(), i);
	 if (!sweeps[i]->write(sweepfn))
             return false;
         progress.updateInc();
      }
   }
   else
      return false;

   bDirty = false;
   return true;
}


bool
CyberScan::load_resolution (int iRes)
{
  if (resolutions[iRes].in_memory)
    return true;

  int nSweeps = sweeps.size();
  if (g_verbose) cout << name << ": create mesh (~"
       << resolutions[iRes].abs_resolution << ") from "
       << nSweeps << " sweeps: " << flush;

  int newres = 0;
  BailDetector bail;
  for (int i = 0; i < nSweeps; i++) {
    //cout << (nSweeps - i) << " left: " << flush;
    sweeps[i]->load_resolution (iRes);
    newres += sweeps[i]->resolutions[iRes].abs_resolution;
    if (g_verbose) cout << "." << flush;

    if (bail()) {
      return false;
    }
  }

  if(g_verbose)  cout << " done." << endl;

  resolutions[iRes].abs_resolution = newres;
  resolutions[iRes].in_memory = true;
  computeBBox();
  return true;
}


int
CyberScan::create_resolution_absolute(int budget, Decimator dec)
{
  cerr << "Can't decimate CyberScan meshes for now" << endl;
  return 0;
}


bool
CyberScan::release_resolution(int nPolys)
{
  int iRes = findLevelForRes (nPolys);

  if (iRes == -1) {
    cerr << "Cyber scan resolution doesn't exist!" << endl;
    return false;
  }

  for (int i = 0; i < sweeps.size(); i++) {
    delete sweeps[i]->levels[iRes];
    sweeps[i]->levels[iRes] = new levelData;
    sweeps[i]->resolutions[iRes].in_memory = false;
  }

  resolutions[iRes].in_memory = false;
  return true;
}


/*
void
check(vector<int> &tstrips, int n)
{
  int ns = tstrips.size();
  for (int i=0; i<ns; i++) {
    assert(tstrips[i] >=-1);
    assert(tstrips[i] < n);
  }
}
*/



extern DrawObjects draw_other_things;


CyberSweep::CyberSweep(void)
{
  _cset.chooseNewColor(falseColor);
  DrawObj::disable();
  draw_other_things.add(this);
}

CyberSweep::~CyberSweep(void)
{
  while (kdtree.size()) {
    delete (kdtree.back());
    kdtree.pop_back();
  }

  while (levels.size()) {
    delete (levels.back());
    levels.pop_back();
  }

  draw_other_things.remove(this);
}


void
CyberSweep::init_leveldata(void)
{
  levels.clear(); levels.reserve(SUBSAMPS);
  kdtree.clear(); kdtree.reserve(SUBSAMPS);
  for (int i = 0; i < SUBSAMPS; i++) {
    levels.push_back (new levelData);
    kdtree.push_back (NULL);
  }
}


void
CyberSweep::insert_possible_resolutions(void)
{
  for (int i = 0; i < SUBSAMPS; i++) {
    int nFacesEstimated = 4 * sd.count_valid_pnts() / (1<<(2*i));

    if (i > 0) {
      insert_resolution(nFacesEstimated, "", false, false);
      int iPos = findLevelForRes(nFacesEstimated);
    } else {
      insert_resolution(nFacesEstimated / 2, "", false, false);
    }
  }
}


bool
CyberSweep::read(const crope &fname)
{
  if (!sd.read(fname))
    return false;

  set_name(fname);
  init_leveldata();

  insert_possible_resolutions();

  if (g_verbose) cout << "done." << endl;
  /*
  Bbox bb = sd.get_bbox();
  SHOW(bb);
#if 1
  int n = 8;
  Pnt3 diff = bb.max() - bb.min(), p, bp;
  for (int i=0; i<n; i++) {
    for (int j=0; j<n; j++) {
      for (int k=0; k<n; k++) {
	p = bb.min() + Pnt3(diff[0]*i, diff[1]*j, diff[2]*k)/n;
	if (sd.xf.back_project(p, bp)) {
	  // now go back to the data, find the closest
	  // data point
	  int row;
	  unsigned short y,z;
	  if (sd.find_data(bp[0], bp[1], row, y, z)) {
	    //SHOW(bp[1]);
	    //SHOW (y);
	    sd.set_xf(row);
	    //xf.set_screw(bp[0],bp[0]);
	    start.push_back(p);
	    end.push_back(sd.xf.apply_xform(y,z));
	    start.push_back(p);
	    //end.push_back(pa);
	    end.push_back(sd.xf.laser_ctr);
	    //start.push_back(.9*end.back()+.1*p);
	    //SHOW(start.back());
	    //SHOW(end.back());
	  } else {
	    cout << "find_data returns false" << endl;
	    //SHOW(bp);
	    //cout << row << " " << y << " " << z << endl;
	  }
	} else {
	  cout << "back_project returns false" << endl;
	}
      }
    }
  }
#else
  Pnt3 p;
  for (int i=0; i<sd.num_frames(); i+=40) {
    for (int j=40; j<480; j+=40) {
      sd.set_xf(i);
      if (sd.get_point(i,j,p)) {
	start.push_back(p);
	end.push_back(sd.xf.laser_ctr);
      }
    }
  }
#endif
  DrawObj::enable();
  */
  if (TbObj::readXform (get_basename()))
    cout << "Found .xf for sweep " << fname << endl;
  //else
  //cout << "No .xf for sweep " << fname << endl;

  return true;
}


bool
CyberSweep::write(const crope &fname)
{
  return sd.write(fname);
}


// called by CyberScan::load_resolution(),
// which again is inherited from ResolutionCtrl
bool
CyberSweep::load_resolution (int iRes)
{
  if (resolutions[iRes].in_memory)
    return true;

  //cout << "tstrip... " << flush;

  if (iRes == 0) {

    sd.get_pnts_and_intensities(levels[0]->pnts,
				levels[0]->intensity);

    // Changed the following line into a two step process, to avoid
    // Compiler complaint. -jed
    //levels[0]->bdry.assign (levels[0]->pnts.size(), 0);
    vector<char> tmp(levels[0]->pnts.size(), 0);
    levels[0]->bdry = tmp;


    sd.make_tstrip(levels[0]->tstrips, levels[0]->bdry);

  } else {

    levelData* level = levels[iRes];

    int step = 1 << iRes;

    sd.subsampled_tstrip(step,
			 Tcl_GetVar (g_tclInterp,
				     "subsamplePreserveHoles",
				     TCL_GLOBAL_ONLY),
			 level->tstrips,
			 level->bdry,
			 level->pnts,
			 level->intensity,
			 level->confidence,
			 level->map_sampled_to_unsampled);
  }

  // now fix step edges, normals
  levelData* level = levels[iRes];

  if (level->pnts.size()) {
    if (strcmp (Tcl_GetVar (g_tclInterp, "removeStepedges",
			    TCL_GLOBAL_ONLY), "0") != 0) {
      //cout << "stepedges... " << flush;
      remove_stepedges(level->pnts, level->tstrips, 4, 50, true);
    }

    //cout << "normals... " << flush;
    getVertexNormals(level->pnts, level->tstrips,
		     true, level->nrms, false);

    resolutions[iRes].abs_resolution =
      count_tris (levels[iRes]->tstrips);
  } else {
    resolutions[iRes].abs_resolution = 0;
  }

  resolutions[iRes].in_memory = true;
  return true;
}


void
CyberSweep::drawthis(void)
{
  glDisable(GL_LIGHTING);
  glBegin(GL_LINES);
  for (int i=0; i<start.size(); i++) {
    glColor3f(1,0,0);
    glVertex3fv(&start[i][0]);
    glColor3f(0,1,0);
    glVertex3fv(&end[i][0]);
  }
  glEnd();
}

/* kberg - 10 July 2001
 * adding per face normals - modeled partly after strips_to_tris
 * in TriMeshUtils.cc, except rather than storing the vertices, it
 * calculates a normal based upon those 3 vertices then adds it to
 * the faceNormals vector.
 */
void CyberSweep::simulateFaceNormals(vector<short> &faceNormals, int currentRes)
{
  if (!levels[currentRes]->tstrips.size())
    return;

  assert(levels[currentRes]->tstrips.back() == -1);

  faceNormals.clear();
  faceNormals.reserve(levels[currentRes]->tstrips.size() * 3); // estimate

  vector<int>::const_iterator vert;
  for (vert = levels[currentRes]->tstrips.begin(); vert != levels[currentRes]->tstrips.end(); vert++) {
    while (*vert == -1) { // handle 0-length strips
      ++vert;
      if (vert == levels[currentRes]->tstrips.end()) break;
    }

    if (vert == levels[currentRes]->tstrips.end()) break;

    vert += 2; // looking backwards at the triangles

    int dir = 0;
    while (*vert != -1) {
      Pnt3 &v0 = levels[currentRes]->pnts[vert[-2 + dir]];
      Pnt3 &v1 = levels[currentRes]->pnts[vert[-1 - dir]];
      Pnt3 &v2 = levels[currentRes]->pnts[vert[0]];

      //now calculate the normal based on these 3 points
      Pnt3 norm = normal(v0, v1, v2);
      pushNormalAsShorts(faceNormals, norm);

      vert++;
      dir ^= 1;
    }
  }
}

MeshTransport*
CyberSweep::mesh (bool perVertex, bool stripped,
		  ColorSource color, int colorSize)
{
  int i = current_resolution_index();
  assert (resolutions[i].in_memory);

  if (!levels[i]->pnts.size())
    return NULL;

  MeshTransport* mt = new MeshTransport;
  mt->setVtx(&levels[i]->pnts, MeshTransport::share);
  mt->setBbox(localBbox());

  /* kberg - 10 July 2001 - per face normals */
  if (perVertex)
    mt->setNrm(&levels[i]->nrms, MeshTransport::share);
  else {
    /* used GenericScan::mesh(...) as an example of how to simulate this.
       for some reason, by going into this mode, only part of a statue
       is displayed when intensity is selected.  It should have something to do
       with how DisplayMesh::renderMeshSingle(...) handles color.
    */
    vector<short> *faceNrm = new vector<short>;
    simulateFaceNormals(*faceNrm, i);
    mt->setNrm(faceNrm, MeshTransport::steal);
  }

  if (stripped) {
    mt->setTris(&levels[i]->tstrips, MeshTransport::share);
  } else {
    vector<int>* tris = new vector<int>;
    strips_to_tris (levels[i]->tstrips, *tris,
		    resolutions[i].abs_resolution);
    mt->setTris(tris, MeshTransport::steal);
  }

  switch (color) {
  case colorNone:
    break;

  case colorTrue:
    {
      vector<uchar>* colors = new vector<uchar>;
      pushColor (*colors, colorSize, falseColor);
      mt->setColor (colors, MeshTransport::steal);
    }
    break;

  case colorIntensity:
    {
      vector<uchar>* colors = new vector<uchar>;
      if (g_bNoIntensity) {
	// BUGBUG: what to do here?
      } else {
// STL Update
        vector<uchar>::iterator end = levels[i]->intensity.end();
	for (vector<uchar>::iterator c = levels[i]->intensity.begin(); c < end; c++)
	  pushColor (*colors, colorSize, *c);
      }
      mt->setColor (colors, MeshTransport::steal);
    }
    break;

  case colorConf:
    if (levels[i]->confidence.size())
    {
      vector<uchar>* colors = new vector<uchar>;
      colors->reserve (colorSize * levels[i]->confidence.size());
// STL Update
      vector<uchar>::iterator end = levels[i]->confidence.end();
      for (vector<uchar>::iterator c = levels[i]->confidence.begin(); c < end; c++)
	pushConf (*colors, colorSize, *c);
      mt->setColor (colors, MeshTransport::steal);
    }
    break;

  case colorBoundary:
    {
      vector<uchar>* colors = new vector<uchar>;
      colors->reserve (colorSize * levels[i]->bdry.size());
// STL Update
      vector<char>::iterator end = levels[i]->bdry.end();
      for (vector<char>::iterator c = levels[i]->bdry.begin(); c < end; c++)
	pushConf (*colors, colorSize, (uchar)(*c ? 0 : 255));
      mt->setColor (colors, MeshTransport::steal);
    }
    break;

  default:
    //cerr << "Color: TODO" << endl;
    break;
  }

  return mt;
}


vector<CyberSweep*>
CyberScan::get_sweep_list (void)
{
  return sweeps;
}


// double indexing: horizontal translation, turns
vector< vector<CyberSweep*> >
CyberScan::get_ordered_sweeps (void)
{
  vector< vector<CyberSweep*> > v;
  int iTrans, iSweep;

  // for each sweep
  for (int i=0; i < sweeps.size(); i++) {
    CyberSweep* sweep = sweeps[i];

    // insert a vector in v based on translation
    float t = sweep->sd.scanner_trans;
    for (iTrans = 0; iTrans < v.size(); iTrans++) {
      float tc = v[iTrans][0]->sd.scanner_trans;
      if (t <= tc) {
// STL Update
	if (t < tc) v.insert(v.begin() + iTrans, vector<CyberSweep*>());
	break;
      }
    }
    if (iTrans == v.size()) v.push_back( vector<CyberSweep*>() );
    // get a reference to the shell
    vector<CyberSweep*>& vShell = v[iTrans];

    // find a place to insert sweep (within shell)
    // based on other screw
    for (iSweep = 0; iSweep < vShell.size(); iSweep++) {
      if (sweep->sd.other_screw < vShell[iSweep]->sd.other_screw)
	break;
    }
    // insert into shell
// STL Update
    vShell.insert (vShell.begin() + iSweep, sweep);
  }

  return v;
}


// this command dumps into the file fname
// nPnts uniformly subsampled from all the sweeps
// such that there are nPnts output
// for each point, there are 5 floats: y,z,scanscrew,otherscrew,xform
// file is initialized with 1 for vertical scan, 0 for horizontal scan
void
CyberScan::dump_pts_laser_subsampled(std::string fname,
				     int nPnts)
{
  // open the fstream
  ofstream out(fname.c_str());
  assert(out);

  // vertical scan?
  int v = sweeps[0]->sd.xf.vertical_scan;
  out.write((char*)&v, sizeof(int));

  // for each sweep, calculate how many points it should
  // store, have it store them
  int n_valid_pts = 0;
  // first, how many valid points are there?
  for (int i=0; i < sweeps.size(); i++) {
    n_valid_pts += sweeps[i]->sd.valid_pts();
  }
  // now, iterate
  SHOW(nPnts);
  float fact = float(nPnts) / float(n_valid_pts);
  for (int i=0; i < sweeps.size(); i++) {
    int n = sweeps[i]->sd.valid_pts();
    n_valid_pts -= n;
    if (i+1 == sweeps.size()) n = nPnts;
    else                      n *= fact;
    nPnts -= sweeps[i]->sd.dump_pts_laser_subsampled(out, n);
    fact = float(nPnts) / float(n_valid_pts);
  }
  SHOW(nPnts);
}


// this function is intended to be used for getting the raw values
// for a given 3D point (given in local coordinates)
// used in conjunction with autocalibration
// assume the mesh has just been registered with ICP
bool
CyberScan::get_raw_data(const Pnt3 &p, sd_raw_pnt &data)
{
  // to simplify things, only works at finest resolution
  //int lvl = 0;
  int lvl = current_resolution_index();
  //if (current_resolution_index() != lvl) return false;
  // and the KDtree has to exist
  if (kdtree[lvl] == NULL) return false;
  // and the reglevelinfo
  if (reglevels[lvl] == NULL) return false;

  // ok. assume p is in the scan coordinates, by the way

  // find the index of the point closest to p in kdtree
  int ind;
  float thr = .1;
  if (!kdtree[lvl]->search(reglevels[lvl]->pnts->begin(),
			   p, ind, thr)) {
    cerr << "Couldn't find the point " << p << endl;
    return false;
  }

  // ok, found the point index
  // now, find which sweep it is and find the relative index there
  int i;
  for (i=0; i<sweeps.size(); i++) {
    int n = sweeps[i]->levels[lvl]->pnts.size();
    if (ind >= n) ind -= n;
    else          break;
  }
  if (i==sweeps.size()) {
    cerr << "Problem in CyberScan::get_raw_data()" << endl;
    return false;
  }

  // now we have the index to the sweep and an index to the
  // point there
  Pnt3  pp;
  if (lvl) {
    ind = sweeps[i]->levels[lvl]->map_sampled_to_unsampled[ind];
    pp = sweeps[i]->sd.raw_for_ith(ind, data);
  } else {
    pp = sweeps[i]->sd.raw_for_ith_valid(ind, data);
  }

  if (dist(pp,p) > .1) {
    SHOW(p);
    SHOW(pp);
    SHOW(dist(p,pp));
  }

  return true;
}


// returns confidence!
float
CyberScan::closest_point_on_mesh(const Pnt3 &p, Pnt3 &cl_pnt,
				 OccSt &status_p)
{
  // cheat...
  return closest_along_line_of_sight(p, cl_pnt, status_p);
}


// returns confidence!
float
CyberScan::closest_along_line_of_sight(const Pnt3 &p, Pnt3 &cp,
				       OccSt &status_p)
{
  // first need to move p into the local coordinates!!
  Pnt3 pp = p;
  xformInvPnt(pp);

  // simplified assumptions... recheck later
  // e.g., assume no per sweep xforms...
  status_p = INDETERMINATE;
  float mindist = 1e33;
  OccSt tmp;
  Pnt3  tmpcp;
  for (int i=0; i<sweeps.size(); i++) {
    sweeps[i]->closest_along_line_of_sight(pp,tmpcp,tmp);
    if (status_p == INDETERMINATE && tmp == NOT_IN_FRUSTUM) {
      status_p = tmp;
      continue;
    }
    // should make a weighted sum of distances, see VolCarve.cc
    float d = dist2(pp, tmpcp);
    if (d < mindist) {
      mindist  = d;
      status_p = tmp;
      cp       = tmpcp;
    }
  }

  // move closest point back into world coordinates!
  xformPnt(cp);

  // return confidence
  return (status_p == INSIDE || status_p == OUTSIDE ? 1.0 : 0.0);
}


OccSt
CyberScan::carve_cube  (const Pnt3 &ctr, float side)
{
  // first of all, we are going to carve a sphere s.t.
  // the ctr is reexpressed in local coordinates

  Pnt3 lctr(ctr);
  xformInvPnt(lctr);
  float r = 1.72 * .5 * side;

  // find a parent from the carve stack for this sphere
  while (carveStack.size() &&
	 !carveStack.back().can_be_child(ctr, side)) {
    carveStack.pop_back();
  }
  if (carveStack.size() == 0) {
    // initialize with all
    carveStack.push_back(carveStackEntry(lctr, r, sweeps));
  }

  // now the back() of the stack should have the sweeps that
  // we're interested in
  vector<CyberSweep*> &sw = carveStack.back().sweeps;

  // add a new entry for this cube
  carveStackEntry cse(lctr, r);

  // go through all the sweeps in the list
  OccSt res = INDETERMINATE;

  for (int i=0; i<sw.size(); i++) {
    OccSt tmp = sw[i]->carve_sphere(lctr, r);
    // add the current to the stack if needed
    if (tmp == BOUNDARY ||
	tmp == SILHOUETTE ||
	tmp == INDETERMINATE) {
      cse.sweeps.push_back(sw[i]);
    }
    if (int(tmp) < int(res)) res = tmp;
    if (res == OUTSIDE) return res;
  }
  carveStack.push_back(cse);
  return res;
}




bool
CyberScan::switchToResLevel (int iRes)
{
  if (!RigidScan::switchToResLevel (iRes))
    return false;

  // keep all sweeps synched to current res
  for (int i = 0; i < sweeps.size(); i++)
    sweeps[i]->switchToResLevel (iRes);

  return true;
}


crope
CyberSweep::get_description (void) const
{
  static char name[100];
  sprintf(name, "trans-%.2d-rot-%.2d",
	  (int)sd.scanner_trans,
	  (int)sd.other_screw);
  return crope (name);
}


KDindtree*
CyberSweep::get_current_kdtree()
{
  int iTree = current_resolution_index();
  assert (iTree < kdtree.size());
  if (kdtree[iTree] != NULL)
    return kdtree[iTree];

  levelData* level = levels[current_resolution_index()];
  kdtree[iTree] = CreateKDindtree(level->pnts.begin(),
				  level->nrms.begin(),
				  level->pnts.size());

  return kdtree[iTree];
}


void
CyberSweep::subsample_points(float rate, vector<Pnt3> &p,
			     vector<Pnt3> &n)
{
  levelData* level = levels[current_resolution_index()];

  int end = level->pnts.size();
  p.clear(); p.reserve(end * rate * 1.1);
  n.clear(); n.reserve(end * rate * 1.1);
  for (int i = 0; i < end; i++) {
    if (rnd() <= rate) {
      p.push_back(level->pnts[i]);    // save point
      pushNormalAsPnt3(n, level->nrms.begin(), i);
    }
  }
}


RigidScan *
CyberSweep::filtered_copy(const VertexFilter &filter)
{
  CyberSweep* newSweep = NULL;

  // need to multiply filter by this sweep's xform
  VertexFilter* sweepFilter = filter.transformedClone ((float*)getXform());
  if (!sweepFilter) {
    cerr << "CyberScan cannot clip because VertexFilter::transformedClone"
	 << " is not supported" << endl;
    return NULL;
  }

  if (sweepFilter->accept (bbox)) {
    newSweep = new CyberSweep;
    // copy the data, first shallow copy
    //*newSweep = *this;
    // except resolutions, which get rebuilt
    // newSweep->resolutions.clear();
    // then do a deeper, filtered copy
    newSweep->init_leveldata();
    sd.filtered_copy(*sweepFilter, newSweep->sd);
    if (sd.count_valid_pnts() == 0) {
      delete newSweep;
      newSweep = NULL;
    } else {
      newSweep->insert_possible_resolutions();
      newSweep->setXform (getXform());
    }
  }

  delete sweepFilter;
  return newSweep;
}


bool
CyberSweep::closest_point(const Pnt3 &p, const Pnt3 &n,
			  Pnt3 &cp, Pnt3 &cn,
			  float thr, bool bdry_ok)
{
  KDindtree* tree = get_current_kdtree();
  if (!tree) return false;

  int ind, ans;

  levelData* level = levels[current_resolution_index()];
  ans = tree->search(&level->pnts[0], &level->nrms[0],
		     p, n, ind, thr);
  if (ans) {
    if (bdry_ok == 0) {
      // disallow closest points that are on the mesh boundary
      if (level->bdry[ind]) return 0;
    }
    cp = level->pnts[ind];
    ind *= 3;
    cn.set(level->nrms[ind  ]/32767.0,
	   level->nrms[ind+1]/32767.0,
	   level->nrms[ind+2]/32767.0);
  }
  return ans;
}


crope
CyberScan::getInfo (void)
{
  long verts = 0;
  long tris = 0;
  for (int i = 0; i < sweeps.size(); i++) {
    verts += sweeps[i]->sd.count_valid_pnts();
    tris =+ sweeps[i]->resolutions[0].abs_resolution;
  }

  char infos[512];
  sprintf(infos, "CyberScan: verts %ld stripped tris %ld\n\n",
	  verts, tris);

  crope info (infos);

  if (sweeps.size() == 1) {
    info += crope ("Single sweep at ") + sweeps[0]->get_description();
    info += crope ("\nFrom file: ") + sweeps[0]->get_name();
  } else {
    float turnMin = FLT_MAX; float transMin = FLT_MAX;
    float turnMax = -FLT_MAX; float transMax = -FLT_MAX;
    int lastShell = -1000;
    int nShells = 0;

    for (int i = 0; i < sweeps.size(); i++) {
      turnMin = MIN (turnMin, sweeps[i]->sd.other_screw);
      turnMax = MAX (turnMin, sweeps[i]->sd.other_screw);
      transMin = MIN (transMin, sweeps[i]->sd.scanner_trans);
      transMax = MAX (transMin, sweeps[i]->sd.scanner_trans);

      if ((int)sweeps[i]->sd.scanner_trans != lastShell) {
	lastShell = sweeps[i]->sd.scanner_trans;
	++nShells;
      }
    }

    sprintf (infos, "%ld sweeps ranging:\n"
	     "Other-screw (nod/turn): %.2f to %.2f\n"
	     "Translation: %.2f to %.2f in %d shells",
	     sweeps.size(), turnMin, turnMax, transMin, transMax, nShells);
    info += crope (infos);
  }

  info += crope ("\n\n") + RigidScan::getInfo();

  return info;
}


unsigned int
CyberScan::get_scanner_config(void)
{
   return sweeps[0]->sd.scanner_config;
}


float
CyberScan::get_scanner_vertical(void)
{
   return sweeps[0]->sd.scanner_vert;
}


//
// This function converts a point from world coordinates (X,Y,Z) to
// sweep coordinates (screw length, y, and z for a specific sweep).
//
bool
CyberScan::worldCoordToSweepCoord(const Pnt3 &wc, int *sweepIndex, Pnt3 &sc)
{
   Pnt3 csc, swc, bpp, fpp;
   bool gotOneFlag = false;
   float theDist, minDist = MAXFLOAT;
   CyberXform cxf;

   // First transform the world coord to the CyberScan coordinate
   // system by multiplying by the inverse of the CyberScan's Xform.

   getXform().apply_inv(wc, csc);

   // Next, iterate over all the sweeps, backprojecting the
   // transformed world coordinate to the sweep's coordinates.
   // If there are any valid backprojected points, we will return
   // the one with the minimum distance from the original point.

   for (int j=0; j<sweeps.size(); ++j)
   {
      // Apply the inverse of the individual sweep xform
      (sweeps[j]->getXform()).apply_inv(csc, swc);

      cxf.setup(sweeps[j]->sd.scanner_config,  // Need to add vertical screw
		sweeps[j]->sd.scanner_trans,   // value to "setup" call when
		sweeps[j]->sd.other_screw);    // we start using it in SDfile.

      //puts ("checking sweep");

      if (cxf.back_project(swc, bpp, true)) {
	cout <<"back project success" << j << "  " << bpp << "\n" << flush;
	 sweepCoordToWorldCoord(j, bpp, fpp);
	 theDist = dist2(wc, fpp);
	 if (theDist < minDist) {
	    sc = bpp;
	    *sweepIndex = j;
	    minDist = theDist;
	    gotOneFlag = true;
         }
      }
   }

   return gotOneFlag;
}


//
// This function converts to world coordinates (X,Y,Z) from
// sweep coordinates (screw length, y, and z for a specific sweep).
//
void
CyberScan::sweepCoordToWorldCoord(int sweepIndex, const Pnt3 &sc, Pnt3 &wc)
{
   CyberXform cxf;

   //out << "In s2s"  << flush;

   // First, use the sweep configuration information and convert
   // to the sweep coordinate system.

   cxf.setup(sweeps[sweepIndex]->sd.scanner_config,  // Need to add vertical
             sweeps[sweepIndex]->sd.scanner_trans,   // screw value to "setup"
             sweeps[sweepIndex]->sd.other_screw);    // call at some point!!!
   cxf.set_screw(sc[0]);
   cxf.apply_xform((short) sc[1], (short) sc[2], wc);

   // Next, apply the transformation for the individual sweep.

   (sweeps[sweepIndex]->getXform())(wc);

   // Finally, apply the CyberScan's transformation.

   getXform()(wc);

   //out << "Out 2s"  << flush;
}


RigidScan *
CyberSweep::get_piece(int firstFrame, int lastFrame)
{
  // Does firstFrame have to be even???
  if (IS_ODD(firstFrame)) {
    cout << "CyberSweep::get_piece() Not tested with odd start frames!!" << endl;
  }

  if (firstFrame <  0 ||
      lastFrame  >= sd.num_frames() ||
      lastFrame  <  firstFrame) {
    return NULL;
  }

  CyberSweep *newSweep = new CyberSweep;
  newSweep->init_leveldata();

  sd.get_piece(firstFrame, lastFrame, newSweep->sd);

  if (newSweep->sd.count_valid_pnts() == 0) {
    delete newSweep;
    return NULL;
  }

  newSweep->insert_possible_resolutions();

  return newSweep;
}


// return confidence!
float
CyberSweep::closest_along_line_of_sight(const Pnt3 &p, Pnt3 &cp,
					OccSt &status_p)
{
  Pnt3 bp;
  if (!sd.xf.back_project(p, bp)) {
    status_p = NOT_IN_FRUSTUM;
    return 0.0;
  }
  int row;
  unsigned short y,z;
  if (!sd.find_data(bp[0], bp[1], row, y, z)) {
    status_p = NOT_IN_FRUSTUM;
    return 0.0;
  }
  sd.set_xf(row, false);
  sd.xf.apply_xform(y,z,cp);
  status_p = (z > bp[2]) ? INSIDE : OUTSIDE;
  return 1.0;
}


OccSt
CyberSweep::carve_sphere(const Pnt3 &ctr, float r)
{
  return OccSt(sd.sphere_status(ctr, r));
}

