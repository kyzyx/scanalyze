//############################################################
// GlobalReg.cc
// Kari Pulli
// Tue Jun 23 11:54:00 PDT 1998
//
// Global registration of several scans simultaneously
//############################################################

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <map>
#include <deque>
#include <iterator>
// G++ Update
#include <strstream>
#include <sys/stat.h>
#include "GlobalReg.h"
#include "absorient.h"
#include "TbObj.h"
#include "DisplayMesh.h"
#include "plvScene.h"
#include "ConnComp.h"
#include "Progress.h"
#include "BailDetector.h"
#include "DirEntries.h"
#include "FileNameUtils.h"
#include "ScanFactory.h"
#include "CyberScan.h"
#include "GroupScan.h"
#include "TclCmdUtils.h"
#include <math.h>

// TODO: make these sliders in globalreg window
#define VB_SIZE (100.0)
#define NOT_FOUND (-1)
#define THRESHOLD (700) // THIS SHOULD BE MUCH LARGER probably

#if defined(WIN32) || defined(i386)
#define LITTLE_ENDIAN
#endif

// setup file loading routines for a little endian machine
#ifdef LITTLE_ENDIAN

inline short swap_short(short x)
{
  return ((x & 0xff) << 8) | ((unsigned short)x >> 8);
}

/* unsigned is essential, otherwise bitshift will
 * sign extend and mess up the conversion.
 */
inline int swap_int(unsigned int x)
{
  return (x << 24) |
         ((x << 8) & 0x00ff0000) |
	 ((x >> 8) & 0x0000ff00) |
	 (x >> 24);
}

inline float swap_float(float x)
{
  unsigned int y = (*((unsigned int *)&x));
  y= (y << 24) |
     ((y << 8) & 0x00ff0000) |
     ((y >> 8) & 0x0000ff00) |
      (y >> 24);
  return (*((float *)&y));
}

inline short FixShort(short x)
{
  return (swap_short(x));
}

inline int FixInt(int x)
{
  return (swap_int(x));
}

inline float FixFloat(float x)
{
  return (swap_float(x));
}

// otherwise set up file loading routines for a big endian machine
#else

inline short FixShort(short x)
{
  return (x);
}

inline int FixInt(int x)
{
  return (x);
}

inline float FixFloat(float x)
{
  return (x);
}

#endif

// File writing routines
inline void WriteInt(int x, ofstream &out)
{
  int temp = FixInt(x);
  out.write((char*)&temp, 4);
}

inline void WriteFloat(float x, ofstream &out)
{
  float temp = FixFloat(x);
  out.write((char*)&temp, 4);
}

inline void WriteShort(short x, ofstream &out)
{
  short temp = FixShort(x);
  out.write((char*)&temp, 2);
}

/* There is no ReadChar since no special byte ordering is required for characters */

inline int ReadInt(ifstream &in)
{
  int temp;
  in.read((char*)&temp, 4);
  return FixInt(temp);
}

inline short ReadShort(ifstream &in)
{
  short temp;
  in.read((char*)&temp, 2);
  return FixShort(temp);
}

inline float ReadFloat(ifstream &in)
{
  float temp;
  in.read((char*)&temp, 4);
  return FixFloat(temp);
}

inline void WriteSDRawPoints(vector<sd_raw_pnt> &rp, int n, ofstream &out)
{
  for (int i=0; i<n; i++) {
    WriteInt(rp[i].config, out);

    WriteShort(rp[i].y, out);
    WriteShort(rp[i].z, out);

    WriteFloat(rp[i].nod, out);
    WriteFloat(rp[i].turn, out);
    WriteFloat(rp[i].tr_h, out);
  }
}

inline void WritePnt3(vector<Pnt3> &pnt, int n, ofstream &out)
{
  for (int i=0; i<n; i++) {
    Pnt3 p = pnt[i];

    WriteFloat(*(float *)p, out);
    WriteFloat(*((float *)p + 1), out);
    WriteFloat(*((float *)p + 2), out);
  }
}

inline void ReadPnt3(vector<Pnt3> &pnt, int n, ifstream &in)
{
  Pnt3 tempPnt;

  for (int i=0; i<n; i++) {
    float f1 = ReadFloat(in);
    float f2 = ReadFloat(in);
    float f3 = ReadFloat(in);

    tempPnt.set(f1, f2, f3);

    pnt[i] = tempPnt;
  }
}

#define GR_MESHNAMES_SEP "^^^"

#define ECHO(x) cout << #x << endl; x

#define FOR_MAP_ENTRIES(key, data) \
  for (ITT __m = hmm.begin(); __m != hmm.end(); __m++) {\
    TbObj *key     = (*__m).first;\
    mapEntry *data = (*__m).second;
#define END_FOR_MAP }

#define FOR_MATCHING_KEYS(key, data) \
  { pair<ITT, ITT> __p = hmm.equal_range(key);\
    for (ITT __i = __p.first; __i!=__p.second; __i++) {\
      mapEntry *data = (*__i).second;
#define END_FOR_KEYS }}

#define CONTAINS(vec, item) \
 (find(vec.begin(),vec.end(),item) != vec.end())

// this is ugly: it's linear time, which makes
// processing scans n^2 (though we have small n...)
inline int
scan_idx(const TbObj* a, const vector<TbObj*> &scan)
{
  for (int i=scan.size()-1; i>=0; i--)
    if (scan[i] == a) return i;
  assert(0);
  return 0;
}


inline float
dist2(const vector<Pnt3> &P, const vector<Pnt3> &Q)
{
  float d = 0.0;
  vector<Pnt3>::const_iterator ip = P.begin();
  vector<Pnt3>::const_iterator iq = Q.begin();
  while (ip != P.end()) {
    d += dist2(*ip, *iq);
    ip++; iq++;
  }
  assert(iq == Q.end());
  return d;
}


static RigidScan*
createProxyScanNamed (const crope& name)
{
  if (strcmp (Tcl_GetVar (g_tclInterp, "allowProxiesWithGR",
			  TCL_GLOBAL_ONLY), "0") == 0) {
    return NULL;
  }

  // make sure we can find the scan, else it's pointless to try to
  // globalreg it because we won't be able to use the results.
  Tcl_VarEval (g_tclInterp, "file_locateScan ", name.c_str(), NULL);
  char* filename = g_tclInterp->result;
  if (!filename[0]) {
    cerr << "Unable to locate scan for " << name << endl;
    return NULL;
  }

  // create invisible scan with 0 bbox
  RigidScan* rs = CreateScanFromBbox (filename, Pnt3(), Pnt3());
  DisplayableMesh* dm = theScene->addMeshSet (rs);

  //cerr << "creating proxy named " << name << " as "
  //   << dm->getName() << endl;

  Tcl_VarEval (g_tclInterp, "addMeshToWindow ",
	       dm->getName(), " 0", NULL);
  Tcl_VarEval (g_tclInterp, "changeVis ", dm->getName(), " 0", NULL);

  return rs;
}


static void
calcPairwiseRmsErr(const vector<Pnt3> &ptsa,
		   const vector<Pnt3> &nrma,
		   const vector<Pnt3> &ptsb,
		   const vector<Pnt3> &nrmb,
		   const Xform<float> &rel_xf,
		   float &pw_point_rmsErr,
		   float &pw_plane_rmsErr)
{
  assert(ptsa.size());

  // calculate the rms for point pair distances
  pw_point_rmsErr = pw_plane_rmsErr = 0.0;
  Pnt3  p,nrm;
  int   n = ptsa.size();
  bool  do_tangents = (nrma.size() != 0);
  float d;
  for (int i=0; i<n; i++) {
    rel_xf.apply(ptsa[i], p);
    pw_point_rmsErr += dist2(p, ptsb[i]);

    if (do_tangents) {
      rel_xf.apply_nrm(nrma[i], nrm);
      p -= ptsb[i];
      d = dot(nrm, p);
      pw_plane_rmsErr += d*d;
      d = dot(nrmb[i], p);
      pw_plane_rmsErr += d*d;
    }
  }
  pw_point_rmsErr = sqrtf(pw_point_rmsErr / n);
  pw_plane_rmsErr = sqrtf(pw_plane_rmsErr / (2*n));
}


void
GlobalReg::mapEntry::stats(void)
{
  Xform<float> xf_a = xfa->getXform();
  Xform<float> xf_b = xfb->getXform();

  int n = ptsa.size();
  Pnt3 first, second, nrm;
  maxErr = avgErr = rmsErr = 0.0;

  Xform<float> actual_rel_xf = xf_b;
  actual_rel_xf.fast_invert();
  actual_rel_xf = actual_rel_xf * xf_a;

  for (int j=0; j<n; j++) {
    // calculate the point wise distances
    first  = ptsa[j];
    second = ptsa[j];
    actual_rel_xf (first);
    rel_xf (second);

    float d = dist2(first, second);
    if (d > maxErr) maxErr = d;
    rmsErr += d;
    avgErr += sqrtf(d);
  }
  for (int j=0; j<n; j++) {
    // calculate the point wise distances
    first  = ptsb[j];
    second = ptsb[j];
    actual_rel_xf.apply_inv(first,first);
    rel_xf.apply_inv(second,second);

    float d = dist2(first, second);
    if (d > maxErr) maxErr = d;
    rmsErr += d;
    avgErr += sqrtf(d);
  }
  n *= 2;

  avgErr /= n;
  rmsErr = sqrtf(rmsErr/n);
}


static bool
write_points(ofstream &out, vector<Pnt3> &pts, TbObj *scan)
{
  CyberScan* cs = dynamic_cast<CyberScan*> (scan);

  if (cs && GetTclGlobalBool ("writeGRsRaw")) {

    //SHOW(cs);
    // magic number (CyberScan point data 1)
    out.write("cpd1", 4);
    // point count
    int n = pts.size();
    WriteInt(n, out);

    // the points
    vector<sd_raw_pnt> cw(n);
    for (int i=0; i<n; i++) {
      if (cs->get_raw_data(pts[i], cw[i]) == false) {
	cerr << "Problem in GlobalReg::write_points()"
	     << endl;
	return false;
      }
    }

    WriteSDRawPoints(cw, n, out);
  } else {

    // magic number (general point data 1)
    out.write("gpd1", 4);
    // point count
    int n = pts.size();
    WriteInt(n, out);

    // the points
    WritePnt3(pts, n, out);
  }

  return !out.fail();
}


static bool
read_points (ifstream &in,  vector<Pnt3> &pts)
{
  // magic number (general point data 1)
  char magic[4];
  in.read(magic, 4);

  if ( strncmp(magic, "cpd1", 4)==0 ) {

    cout << "CyberScan raw data" << endl;
    // CyberScan raw data
    int n;
    n = ReadInt(in);

    pts.resize(n);
    unsigned int config;
    short yz[2];
    float nt[2]; // nod, turn
    float tr_h;
    CyberXform xf;

    for (int i=0; i<n; i++) {
      config = ReadInt(in);
      yz[0] = ReadShort(in);
      yz[1] = ReadShort(in);
      nt[0] = ReadFloat(in);
      nt[1] = ReadFloat(in);
      tr_h = ReadFloat(in);

      int os = config&1; // other_screw

      xf.setup(config, tr_h, nt[os]);
      xf.set_screw(nt[!os], nt[!os]);
      pts[i] = xf.apply_xform(yz[0],yz[1]);
      if (! pts[i].isfinite()) {
	SHOW(pts[i]);
	SHOW(i);
	SHOW(config);
	SHOW(yz[0]);
	SHOW(yz[1]);
	SHOW(nt[0]);
	SHOW(nt[1]);
	SHOW(tr_h);
	SHOW(os);
      }
    }

  } else if (!strncmp (magic, "gpd1", 4)) {

    // good old regular 3d points
    // point count
    int n;

    n = ReadInt(in);

    pts.resize(n);

    // the points
    ReadPnt3(pts, n, in);
  } else {

    cerr << "Bad magic field in .gr file!" << endl;
    return false;

  }

  return !in.fail();
}


static struct {
  char* magic; int ver;
} GRVersionTable[] = {
  { "GR01", 1 },
  { "GR02", 2 },
  { "GR03", 3 },
  { "GR04", 4 }
};
#define cGRVersionTableSize (sizeof(GRVersionTable)/sizeof(GRVersionTable[0]))
#define GRVersionCurrent (GRVersionTable[cGRVersionTableSize-1].magic)

int GetGRVersion (char* magic)
{
  for (int i = 0; i < cGRVersionTableSize; i++) {
    if (!strncmp (GRVersionTable[i].magic, magic, 4))
      return GRVersionTable[i].ver;
  }

  return 0;
}


// try writing the mapEntry data into a single file
void
GlobalReg::mapEntry::export_to_file(const std::string &gr_dir)
{
  // figure out the filename
  const char *name1 = GetTbObjName(xfa);
  const char *name2 = GetTbObjName(xfb);
  if (!name1 || !name2) {
    cerr << "Couldn't get names, not saving global"
	 << " data" << endl;
    return;
  }

  bool flip = false;
  if (strcmp(name1, name2) == 1) {
    flip = true;
    const char *tmp = name2;
    name2 = name1; name1 = tmp;
  }

  std::string path = gr_dir;
  if (!manually_aligned) path += "/auto";
  path += std::string("/") + name1 + GR_MESHNAMES_SEP + name2 + ".gr";

  // just overwrite the file if it exists
#ifdef WIN32
  ofstream out(path.c_str(), ios::binary);
#else
  ofstream out(path.c_str());
#endif
  if (!out) {
    cerr << "Couldn't create file " << path.c_str() << endl;
    return;
  } else {
    SHOW(path.c_str());
  }

  // fill the file with useful stuff

  cout << "Writing correspondences between " << name1
       << " and " << name2 << " ... ";

  out.write(GRVersionCurrent, 4); // magic number

  //SHOW(name1);
  //SHOW(name2);
  //SHOW(dynamic_cast<CyberScan*> (xfa));
  //SHOW(dynamic_cast<GroupScan*> (xfa));
  if (flip) {
    write_points(out, ptsb, xfb);
    write_points(out, ptsa, xfa);
    Xform<float> tmp_xf = rel_xf;
    tmp_xf.fast_invert();
    for (int i=0; i<16; i++)
        WriteFloat(*((float*)tmp_xf + i), out);
  } else {
    write_points(out, ptsa, xfa);
    write_points(out, ptsb, xfb);
    for (int i=0; i<16; i++)
      WriteFloat(*((float*)rel_xf + i), out);
  }

  WriteFloat(pw_point_rmsErr, out);
  WriteFloat(pw_plane_rmsErr, out);
  WriteInt(quality_grade, out);
  out.write((char *)&manually_aligned, 1 /*sizeof(bool) */);

  if (out.fail()) return;
  cout << "success." << endl;

  // currently saving:
  // pairwiserms errors
  // point count
  // points
  // registration xform
  // whether manually aligned

  // should also write:
  // rmserror to tangent plane
  // sampling density?
  // or weight that would normalize based on area?
}


void
GlobalReg::mapEntry::export_cyber_raw(const char *fname)
{
  // only work if both scans are CyberScans
  CyberScan *csa = dynamic_cast<CyberScan*> (xfa);
  CyberScan *csb = dynamic_cast<CyberScan*> (xfb);

  if (!csa || !csb) return;

  ofstream out(fname);
  if (!out) {
    cerr << "Couldn't create file " << fname << endl;
    return;
  } else {
    SHOW(fname);
  }

  int cPnt = ptsa.size();
  out.write ((char*)&cPnt, sizeof(cPnt));
  //bool vert;
  sd_raw_pnt cw;
  for (int i=0; i<cPnt; i++) {
    if (csa->get_raw_data(ptsa[i], cw) == false) {
      cerr << "Problem in GlobalReg::mapEntry::export_cyber_raw()"
	   << endl;
      return;
    }
    int tmp = (cw.config & 0x00000001); // vert;
    out.write((char*) &tmp,    sizeof(int));
    out.write((char*) &cw.y,   2*sizeof(short));
    out.write((char*) &cw.nod, 2*sizeof(float));
  }
  for (int i=0; i<cPnt; i++) {
    if (csb->get_raw_data(ptsb[i], cw) == false) {
      cerr << "Problem in GlobalReg::mapEntry::export_cyber_raw()"
	   << endl;
      return;
    }
    int tmp = (cw.config & 0x00000001); // vert;
    out.write((char*) &tmp,    sizeof(int));
    out.write((char*) &cw.y,   2*sizeof(short));
    out.write((char*) &cw.nod, 2*sizeof(float));
  }
  out.write ((char*)(float*)rel_xf, 16 * sizeof(float));

  if (g_verbose) {
    if (out.fail()) cout << "failure." << endl;
    else            cout << "success." << endl;
  }
}

// if partner == NULL, this fn will return the points from ALL
// the scans P is partnered with
void
GlobalReg::get_pts(TbObj *x, vector<Pnt3> &P, vector<Pnt3> &Q,
		   TbObj* partner)
{
  P.clear(); Q.clear();

  // find entries with key x
  FOR_MATCHING_KEYS(x, data) {

    TbObj *datap, *dataq;
    XF_F  dataxf = data->rel_xf;
    vector<Pnt3> *pp;//, *qq;
    if (x == data->xfa) {
      datap = data->xfa; dataq = data->xfb;
      pp = &data->ptsa;//  qq = &data->ptsb;
    } else {
      datap = data->xfb; dataq = data->xfa;
      pp = &data->ptsb;//  qq = &data->ptsa;
      dataxf.fast_invert();
    }

    if (partner != NULL && dataq != partner)
      continue;

    int ind = P.size();
    P.insert(P.end(), pp->begin(), pp->end());
#if 1
    // Q becomes "where P's points should be in Q's coords"
    for (int i=0; i<pp->size(); i++) {
      //Pnt3 p = data->ptsa[i];
      Pnt3 p = (*pp)[i];
      dataxf(p);
      Q.push_back(p);
    }
#else
    Q.insert(Q.end(), qq->begin(), qq->end());
#endif
    // then convert to world coordinates
    for_each (P.begin() + ind, P.end(), datap->getXform());
    for_each (Q.begin() + ind, Q.end(), dataq->getXform());
  } END_FOR_KEYS
}


// used for global registration, and only Horn's version
void
GlobalReg::get_pts_within_group(TbObj *x,
				vector<Pnt3> &P, vector<Pnt3> &Q,
				vector<TbObj*> &group)
{
  P.clear(); Q.clear();

  // find entries with key x
  FOR_MATCHING_KEYS(x, data) {

    TbObj *datap, *dataq;
    XF_F  dataxf = data->rel_xf;
    vector<Pnt3> *pp;//, *qq;
    if (x == data->xfa) {
      datap = data->xfa; dataq = data->xfb;
      pp = &data->ptsa;//  qq = &data->ptsb;
    } else {
      datap = data->xfb; dataq = data->xfa;
      pp = &data->ptsb;//  qq = &data->ptsa;
      dataxf.fast_invert();
    }

    if (CONTAINS(group, dataq)) {
      // copy the points (in local coordinates)
      int ind = P.size();
      P.insert(P.end(), pp->begin(), pp->end());

#if 1
      // Q becomes "where P's points should be in Q's coords"
      for (int i=0; i<pp->size(); i++) {
	Pnt3 p = (*pp)[i];
	dataxf(p);
	Q.push_back(p);
      }
#else
      Q.insert(Q.end(), qq->begin(), qq->end());
#endif
      // then convert to world coordinates
      for_each (P.begin() + ind, P.end(), datap->getXform());
      for_each (Q.begin() + ind, Q.end(), dataq->getXform());
    }

  } END_FOR_KEYS;

}


void
GlobalReg::evaluate(const vector<TbObj*> &group)
{
  for (int i=0; i<group.size(); i++) {
    FOR_MATCHING_KEYS(group[i], data) {
      // TbObjs in a pair must be in the same group!
      assert(data->xfa == group[i] ||
	     CONTAINS(group, data->xfa));
      assert(data->xfb == group[i] ||
	     CONTAINS(group, data->xfb));

      if (data->xfb == group[i]) continue;

      data->stats();
    } END_FOR_KEYS;
  }
}


float
GlobalReg::evaluate(TbObj *one)
{
  // calculate the mean distance between every point pair
  int   cnt   = 0;
  float sumSq = 0.0;

  FOR_MATCHING_KEYS(one, data) {
     data->stats();
     int n     = data->ptsa.size();
     float rms = data->rmsErr;
     cnt   += n;
     sumSq += n*rms*rms;
  } END_FOR_KEYS;
  if (cnt == 0) {
    cerr << "GlobalReg::evaluate() didn't have entries to"
	 << " evaluate" << endl;
    return 0.0;
  }
  return sqrtf(sumSq/cnt);
}


static crope
nameFromTbObj (TbObj* tbo)
{
  if (tbo) {
    RigidScan* rs = dynamic_cast<RigidScan*> (tbo);
    if (rs) {
      DisplayableMesh* dm = FindMeshDisplayInfo (rs);
      if (dm) return dm->getName();
      else    return rs->get_name();
    }
    return crope("(unknown)");
  }
  return crope("(NULL)");
}


void
GlobalReg::dump(void)
{
  cout << endl;
  FOR_MAP_ENTRIES(key, data) {
    SHOW(key);
    SHOW(data);
    SHOW(data->xfa);
    SHOW(data->xfb);
    vector<Pnt3> &ptsa = data->ptsa;
    cout << "-" << endl;
    copy(ptsa.begin(), ptsa.end(),
	 ostream_iterator<Pnt3>(cout, "\n"));
    vector<Pnt3> &ptsb = data->ptsb;
    cout << "-" << endl;
    copy(ptsb.begin(), ptsb.end(),
	 ostream_iterator<Pnt3>(cout, "\n"));
    cout << "-" << endl;
    copy(all_scans.begin(), all_scans.end(),
	 ostream_iterator<TbObj*>(cout, "\n"));
  } END_FOR_MAP
  cout << endl;
}


void
GlobalReg::move_back(const vector<TbObj*> &group,
		     const vector<XF_F> &old_xf)
{
  cout << "Moving the scans back..." << flush;
  // first, create point pairs
  vector<Pnt3> P,Q;
  XF_F xfp, xfq;
  int i,j;
  for (i=0; i<group.size(); i++) {
    FOR_MATCHING_KEYS(group[i], data) {
      // each entry twice in the map
      if (group[i] == data->xfb) continue;
      // points in data ptsa
      int end = P.size();
      for (j=0; j<data->ptsa.size(); j+=20) {
	P.push_back(data->ptsa[j]);
	Q.push_back(data->ptsa[j]);
      }
      xfp = data->xfa->getXform();               // current xf
      xfq = old_xf[scan_idx(data->xfa, group)];  // old xf
      for_each(P.begin() + end, P.end(), xfp);
      for_each(Q.begin() + end, Q.end(), xfq);
      // points in data ptsb
      end = P.size();
      for (j=0; j<data->ptsb.size(); j+=20) {
	P.push_back(data->ptsb[j]);
	Q.push_back(data->ptsb[j]);
      }
      xfp = data->xfb->getXform();               // current xf
      xfq = old_xf[scan_idx(data->xfb, group)];  // old xf
      for_each(P.begin() + end, P.end(), xfp);
      for_each(Q.begin() + end, Q.end(), xfq);
    } END_FOR_KEYS;
  }

  // find the motion that brings the group back close to
  // where it was
  double q[7];
  horn_align(&P[0], &Q[0], P.size(), q);
  xfp.fromQuaternion(q, q[4], q[5], q[6]);

  // apply it
  for (i=0; i<group.size(); i++) {
    group[i]->setXform((xfp *
			group[i]->getXform()).enforce_rigidity());
  }
  cout << "done." << endl;
}


Xform<float>
GlobalReg::compute_xform(vector<Pnt3> &P, vector<Pnt3> &Q)
{
  Xform<float> xf;
  double q[7];
  horn_align (&P[0], &Q[0], P.size(), q);
  xf.fromQuaternion (q, q[4], q[5], q[6]);
  return xf;
}


void
GlobalReg::align_one_to_others (TbObj* one, TbObj* two)
{
  vector<Pnt3> P,Q;

  float sum = .5*FLT_MAX, oldsum;

  one->save_for_undo();
  do {
    oldsum = sum;

    get_pts (one, P, Q, two);  // two==NULL means use all partners
    if (P.size() == 0) continue;

    one->setXform(compute_xform(P, Q) * one->getXform(),
		  false);

    sum = evaluate(one);
    cout << "The average point-to-point distance is "
	 << sum << endl;

    if (BailDetector::bail()) {
      cerr << "global alignment cancelled." << endl;
      break;
    }
  } while (2.0*(oldsum-sum) > ftol*(sum+oldsum+1.e-10));
}


#define FOR_ACTIVE_NBORS(scan, nbor) \
  FOR_MATCHING_KEYS(scan, data) {\
    TbObj *nbor = (data->xfa == scan ? data->xfb : data->xfa);\
    if (active.end() != find(active.begin(),active.end(),nbor)){
#define END_FOR_NBORS }}}}

/* // get emacs identation working after above macro definition...
{
*/

// part covered by psuedo-code in the paper
void
GlobalReg::align_group(const vector<TbObj*> &group)
{
  vector<Pnt3> P,Q;
  int          i;

  assert (group.size() > 1);

  vector<XF_F> old_xf(group.size());
  for (i=0; i<group.size(); i++) {
    old_xf[i] = group[i]->getXform();
    group[i]->save_for_undo();
  }

  // new algorithm
  //
  // for each member of the group, calculate the number
  // of links, choose the one with the most links as the seed
  //
  // push the seed to a queue and to the active set
  //
  // process the queue until empty
  //     take the scan at the front
  //     align it with the scans in the active set
  //     if it improves more than the threshold
  //         push all the neighbors in the active set to queue
  //
  // add to the queue and the active set the scan with the
  // most links to scans in the active set

  vector<TbObj*> active, dormant, nbors;
  deque<TbObj*>  que;
  TbObj *seed;
  int    seed_links = 0;

  // set the dormant and active sets...
  if (dirty_scans.size()) {
    for (i=0; i<group.size(); i++) {
      if (dirty_scans.find(group[i]) != dirty_scans.end()) {
	dormant.push_back(group[i]);
      } else {
	active.push_back(group[i]);
      }
    }
  }

  if (active.size() == 0) {
    // active set is empty!
    // initialize, find the seed (one with most links)
    for (i=0; i<group.size(); i++) {
      int links = 0;
      FOR_MATCHING_KEYS(group[i], data) {
	links++;
      } END_FOR_KEYS;
      if (links > seed_links) {
	seed_links = links;
	seed = group[i];
      }
    }
    assert(seed_links);

    // initialize the active and dormant sets
    active.push_back(seed);
    dormant.clear();
    dormant.reserve(group.size()-1);
    for (i=0; i<group.size(); i++) {
      if (group[i] != seed) dormant.push_back(group[i]);
    }
  }

  Progress progress (dormant.size(), "Global alignment");

  // process all scans
  while (!dormant.empty()) {

    // find the dormant scan that has the most links to scans
    // in active set
    seed_links = 0;
    vector<TbObj*>::iterator it, pos;
    for (it = dormant.begin(); it != dormant.end(); it++) {
      int links = 0;
      FOR_ACTIVE_NBORS(*it, nbor) {
	links++;
      } END_FOR_NBORS;
      if (links > seed_links) {
	seed_links = links;
	pos        = it;
      }
    }
    assert(seed_links);

    // remove from dormant set
    seed = *pos;
    int dormant_size = dormant.size();
    dormant.erase(pos);
    assert(dormant_size - 1 == dormant.size());

    // push to active and queue
    active.push_back(seed);
    assert(que.empty());
    que.push_back(seed);

    cout << "\n";
    SHOW(active.size());
    SHOW(dormant.size());

    // process the queue
    int cnt = 0;
    while (!que.empty()) {

      seed = que.front();
      que.pop_front();

      // align front of the queue with the active set
      nbors.clear();
      FOR_ACTIVE_NBORS(seed, nbor) {
	nbors.push_back(nbor);
      } END_FOR_NBORS;

      SHOW(nbors.size());

      // here we already need to have determined the needed pts
      get_pts_within_group(seed, P, Q, nbors);
      assert(P.size());
      assert(P.size() == Q.size());

      float start_dist = dist2(P,Q);
      SHOW(start_dist);

      //Xform<float> xf;
      //double q[7];
      //horn_align (&P[0], &Q[0], P.size(), q);
      //xf.fromQuaternion (q, q[4], q[5], q[6]);

      // ... so when we get to here we can compute a less
      // biased xform
      Xform<float> xf = compute_xform(P, Q);

      for_each(P.begin(), P.end(), xf);
      float end_dist = dist2(P,Q);
      SHOW(end_dist);

      // if changed enough, push the neighbors into queue

      float diff = start_dist - end_dist;

      if (diff > 0.0)
	seed->setXform(xf * seed->getXform(), false);

      if (diff > ftol * end_dist &&
	  diff > 1.0e-6) {
	if (cnt++ < 200) {
	  for (it = nbors.begin(); it != nbors.end(); it++) {
	    if (!CONTAINS(que, *it)) {
	      // nbor not in queue already, push
	      que.push_back(*it);
	    }
	  }
	}
      }

      //SHOW(start_dist);
      //SHOW(end_dist);
      cout << que.size() << " left; last ";
      SHOW(diff/P.size());
      //SHOW(que.size());
      //SHOW(active.size());
      //SHOW(ftol);

    }

    if (!progress.updateInc())
      break;
    if (BailDetector::bail()) {
      cerr << "global alignment cancelled." << endl;
      break;
    }
  }

  // align the new points with the old points in order
  // to minimize the motion of the whole group

  move_back(group, old_xf);
  evaluate(group);
}


bool
GlobalReg::getPairError(TbObj *a, TbObj *b,
                        float &pointError,
                        float &planeError,
                        float &globalError,
                        bool  &manual)
{
  FOR_MATCHING_KEYS(a, data) {
    if ((data->xfa == a && data->xfb == b) ||
	(data->xfb == a && data->xfa == b)) {
      pointError  = data->pw_point_rmsErr;
      planeError  = data->pw_plane_rmsErr;
      globalError = data->rmsErr;
      manual      = data->manually_aligned;
      return true;
    }
  } END_FOR_KEYS;

  return false;
}


// later overwrites the first
// also if detect that both auto and manual versions
// exist, unlink the auto version
// or if two versions of the same link exist, delete
// the older one
GlobalReg::mapEntry*
GlobalReg::import(const std::string &fname, bool manual)
{
  cout << "import " << fname.c_str() << endl;

  // TODO: the below (I guess just the rfind) expects / as path separator,
  // and can't deal with \, so we need to either teach it or replace \ for /
  // to make this work on Windows.

  // get the mesh names
  std::string name[2];
  int tmp1 = fname.rfind("/");
  if (tmp1) tmp1++; // skip /
  int tmp2 = fname.find(GR_MESHNAMES_SEP);
  name[0].assign(fname, tmp1, tmp2-tmp1);
  tmp2 += strlen(GR_MESHNAMES_SEP); // skip ::: or whatever the separator is
  name[1].assign(fname, tmp2, fname.size()-tmp2-3);

  // get the TbObj pointers
  TbObj* tbobj[2];
  for (int is = 0; is < 2; is++) {
    DisplayableMesh *dm = FindMeshDisplayInfo(name[is].c_str());
    if (dm) {
      // the mesh is in memory
      tbobj[is] = dm->getMeshData();
    } else {
      // the mesh is not in memory; need to proxy
      tbobj[is] = createProxyScanNamed (name[is].c_str());
      if (!tbobj[is]) {
	//cerr << "  ignoring correspondence "
	//     << fname.c_str() << endl;
	return NULL;
      }
    }
  }

  FOR_MATCHING_KEYS(tbobj[0], data) {
    if ((data->xfa == tbobj[0] && data->xfb == tbobj[1]) ||
	(data->xfb == tbobj[0] && data->xfa == tbobj[1])) {
      // this mapentry exists!
      if (manual && data->manually_aligned == false) {
	// the previous entry was an autoICP entry, delete
	cerr << "Warning: there seem to be several entries "
	     << endl << "involving meshes " << name[0].c_str()
	     << " and " << name[1].c_str() << endl;
	cerr << "Deleting autoICP *.gr entry" << endl;
	unlink_gr_files(data->xfa, data->xfb, true);
	// continue to read over this entry
	break;
      } else {
	// don't read this entry
	return NULL;
      }
    }
  } END_FOR_KEYS;

#ifdef WIN32
  ifstream in(fname.c_str(), ios::binary);
#else
  ifstream in(fname.c_str());
#endif
  if (!in) {
    cerr << "Couldn't open file " << fname.c_str()
	 << " for reading" << endl;
    return NULL;
  }

  vector<Pnt3> ap, bp;
  float rel_xf[16];
  float pw_point_rmsErr, pw_plane_rmsErr;
  char version[4];
  in.read(version, 4);
  int cPnt;

  int iVersion = GetGRVersion (version);
  switch (iVersion) {
  case 1:
    // Oops, in version one saved this twice.
    // Ignore this first one.
    pw_point_rmsErr = ReadFloat(in);
    cPnt = ReadInt(in);
    ap.resize(cPnt); bp.resize(cPnt);
    ReadPnt3(ap, cPnt, in);
    ReadPnt3(bp, cPnt, in);

    /* haven't check this case, should work, but this was the code
     * that was here before the endian fix.
    in.read((char*)&pw_point_rmsErr, sizeof(float));
    in.read((char*)&cPnt, sizeof(int));
    ap.resize(cPnt); bp.resize(cPnt);
    in.read((char*)&ap[0], cPnt*sizeof(Pnt3));
    in.read((char*)&bp[0], cPnt*sizeof(Pnt3));
    */

    break;

  case 2:

    cPnt = ReadInt(in);
    ap.resize(cPnt); bp.resize(cPnt);
    ReadPnt3(ap, cPnt, in);
    ReadPnt3(bp, cPnt, in);

    /* haven't check this case, should work, but this was the code
     * that was here before the endian fix.
    in.read((char*)&cPnt, sizeof(int));
    ap.resize(cPnt); bp.resize(cPnt);
    in.read((char*)&ap[0], cPnt*sizeof(Pnt3));
    in.read((char*)&bp[0], cPnt*sizeof(Pnt3));
    */

    break;

  case 3:
  case 4:
    read_points(in, ap);
    read_points(in, bp);
    break;

  case 0:
  default:
    cerr << fname.c_str()
	 << " does not have the correct signature" << endl;
    cerr << "It may have been created with an out-of-date "
	 << "version of scanalyze" << endl;
    return NULL;
  }

  for (int i=0; i<16; i++)
    (*((float *)rel_xf + i)) = ReadFloat(in);
  pw_point_rmsErr = ReadFloat(in);
  pw_plane_rmsErr = ReadFloat(in);

  int quality = mapEntry::qual_Unknown;
  if (iVersion == 4) {
    // version 4 fields: quality, manually_aligned
    // yeah, manual is passed in to this function, but that seems a
    // broken idea to me, so let this override it

    quality = ReadInt(in);
    in.read((char*)&manual, 1 /*sizeof(bool)*/);

  }

  // create and insert mapEntry
  mapEntry* entry = addPair(tbobj[0], tbobj[1], ap, bp,
			    pw_point_rmsErr, pw_plane_rmsErr,
			    rel_xf, manual, 1000000, false, quality);

  // and grab its modification time from file
  struct stat fileinfo;
  stat (fname.c_str(), &fileinfo);
  entry->modifyDate = fileinfo.st_mtime;
  return entry;
}


inline void
unlink_gr_file(std::string path,
	       std::string n1,
	       std::string n2)
{
  path += "/";
  path += n1;
  path += GR_MESHNAMES_SEP;
  path += n2;
  path += ".gr";
  unlink_if_exists(path.c_str());
}


void
GlobalReg::unlink_gr_files(TbObj *a, TbObj *b, bool only_auto)
{
  std::string name1, name2;
  name1 = GetTbObjName(a);
  name2 = GetTbObjName(b);

  unlink_gr_file(gr_auto_dir, name1, name2);
  unlink_gr_file(gr_auto_dir, name2, name1);
  if (only_auto == false) {
    unlink_gr_file(gr_dir, name1, name2);
    unlink_gr_file(gr_dir, name2, name1);
  }
}


GlobalReg::GlobalReg(void)
  : initial_import_done(false)
{

  //
  // set up gr_dir and gr_auto_dir
  //
  char *tmp = getenv("SC_GR_DIR");
  if (tmp == NULL) {
    // use the current working directory
    char buf[256];
    getcwd(buf, 256);
    gr_dir.assign(buf);
    gr_dir.append("/globalregs");
  } else {
    gr_dir.assign(tmp);
    // remove trailing slash if needed
    std::string::iterator back = gr_dir.end()-1;
    if (*back == '/') gr_dir.erase(back);
  }

  if (!check_file_access(gr_dir.c_str(), 1,1,1,1,1)) {
    // need to have something; revert to current dir
    char current[PATH_MAX];
    getcwd (current, PATH_MAX);
    gr_dir.assign (current);
    cerr << "Reverting to current dir (" << gr_dir.c_str()
	 << ") for globalregs" << endl;
  }

  SHOW(gr_dir.c_str());

  cyber_raw_name = getenv("SC_RAW_DUMP");

}


GlobalReg::~GlobalReg(void)
{
  // delete the mapEntries
  // recall they are there twice, delete only once!
  FOR_MAP_ENTRIES(key, data) {
    if (key == data->xfa) delete data;
  } END_FOR_MAP
}


void
GlobalReg::initial_import(void)
{
  if (!initial_import_done) {

    gr_auto_dir.assign(gr_dir);
    gr_auto_dir.append("/auto");

    if (!check_file_access(gr_auto_dir.c_str(), 1,1,1,1,1)) {
      cerr << "Creating autoreg directory "
	   << gr_auto_dir.c_str() << endl;
      portable_mkdir (gr_auto_dir.c_str(), 00775);
    }

    perform_import();

    initial_import_done = true;
  }
}


void
GlobalReg::perform_import(void)
{
  cerr << "Importing *.gr... " << flush;

  int nTotal = 0, nYes = 0;
  int nTotalAuto = 0, nYesAuto = 0;
  for (DirEntries de_auto(gr_auto_dir, ".gr");
       !de_auto.done(); de_auto.next()) {
    ++nTotalAuto;
    if (import(de_auto.path()))
      ++nYesAuto;
  }

  int nQuality = mapEntry::qual_Good + 1;
  assert (nQuality == 4);
  int quality_bucket[4] = {0};
  mapEntry* entry;
  for (DirEntries de(gr_dir, ".gr"); !de.done(); de.next()) {
    ++nTotal;
    if (entry = import(de.path(),true)) {
      ++nYes;
      ++quality_bucket[entry->getQuality()];
    }
  }

  cerr << "imported " << nYesAuto << " / " << nTotalAuto << " auto; "
       << nYes << " / " << nTotal << " manual."
       << endl;
  cerr << "Quality distribution: ";
  for (int i = 0; i < nQuality; i++) {
    cerr << quality_bucket[i];
    if (i < nQuality - 1) cerr << " / ";
  }
  cerr << endl << "done." << endl;
}


GlobalReg::mapEntry*
GlobalReg::addPair(TbObj *a, TbObj *b,
		   const vector<Pnt3> &ap, const vector<Pnt3> &nrma,
		   const vector<Pnt3> &bp, const vector<Pnt3> &nrmb,
		   Xform<float> rel_xf,
		   bool manually_aligned,
		   int  max_pairs,
		   bool save, int saveQual)
{
  assert(ap.size() == bp.size());
  // delete previous instance
  deletePair(a,b, save);
  // create a mapEntry
  mapEntry *me = new mapEntry(a,b,ap,bp,rel_xf,max_pairs);
  me->manually_aligned = manually_aligned;
  me->quality_grade = saveQual;
  calcPairwiseRmsErr(ap,nrma,bp,nrmb,rel_xf,
		     me->pw_point_rmsErr,
		     me->pw_plane_rmsErr);
  // add it twice
  hmm.insert(HMM::value_type(a,me));
  hmm.insert(HMM::value_type(b,me));
  // add to set
  all_scans.insert(a);
  all_scans.insert(b);
  dirty_scans.insert(a);
  dirty_scans.insert(b);

  // Compute errors, results stored
  me->stats();

  if (save) me->export_to_file(gr_dir);
  if (cyber_raw_name) me->export_cyber_raw(cyber_raw_name);

  return me;
}


GlobalReg::mapEntry*
GlobalReg::addPair(TbObj *a, TbObj *b,
		   const vector<Pnt3> &ap,
		   const vector<Pnt3> &bp,
		   float pw_point_rmsErr,
		   float pw_plane_rmsErr,
		   Xform<float> rel_xf,
		   bool manually_aligned,
		   int  max_pairs,
		   bool save, int saveQual)
{
  assert(ap.size() == bp.size());
  // delete previous instance
  deletePair(a,b, save);
  // create a mapEntry
  mapEntry *me = new mapEntry(a,b,ap,bp,rel_xf,max_pairs);
  me->manually_aligned = manually_aligned;
  me->quality_grade = saveQual;
  me->pw_point_rmsErr = pw_point_rmsErr;
  me->pw_plane_rmsErr = pw_plane_rmsErr;
  // add it twice
  hmm.insert(HMM::value_type(a,me));
  hmm.insert(HMM::value_type(b,me));
  // add to set
  all_scans.insert(a);
  all_scans.insert(b);
  dirty_scans.insert(a);
  dirty_scans.insert(b);

  // Compute errors, results stored
  me->stats();

  if (save) me->export_to_file(gr_dir);
  if (cyber_raw_name) me->export_cyber_raw(cyber_raw_name);

  return me;
}


bool
GlobalReg::markPairQuality (TbObj *a, TbObj* b, int saveQual)
{
  FOR_MATCHING_KEYS(a, data) {
    if (data->xfa == b || data->xfb == b) {
      data->quality_grade = saveQual;
      data->export_to_file(gr_dir);
      return true;
    }
  } END_FOR_KEYS;

  return false;
}



void
GlobalReg::deletePair(TbObj *a, TbObj *b, bool deleteFile)
{
  // erase twice, delete once
  FOR_MATCHING_KEYS(a, data) {
    if (data->xfa == b || data->xfb == b) {
      hmm.erase(__i);
      break;
    }
  } END_FOR_KEYS;
  FOR_MATCHING_KEYS(b, data) {
    if (data->xfa == a || data->xfb == a) {
      hmm.erase(__i);
      delete data;
      break;
    }
  } END_FOR_KEYS;
  // if the transforms are not there, remove from set
  if (hmm.end() == hmm.find(a)) {
    all_scans.erase(a);
    dirty_scans.erase(a);
  } else {
    dirty_scans.insert(a);
  }
  if (hmm.end() == hmm.find(b)) {
    all_scans.erase(b);
    dirty_scans.erase(b);
  } else {
    dirty_scans.insert(b);
  }
  // delete the corresponding files
  if (deleteFile) unlink_gr_files(a,b);
}


void
GlobalReg::deleteAllPairs (TbObj *a, bool deleteFile)
{
  bool again;
  do {
    again = false;

    // deletion invalidates existing iterators, so don't go
    // through the
    // FOR_MATCHING_KEYS loop more than once; exit it and restart.
    FOR_MATCHING_KEYS (a, data) {
      // erase once
      hmm.erase (__i);

      // possible delete the *.gr file
      if (deleteFile) unlink_gr_files(data->xfa, data->xfb);

      // erase twice
      TbObj* other = data->xfa;
      if (other == a)
	other = data->xfb;
      bool foundit = false;
      FOR_MATCHING_KEYS (other, data2) {
	if (data2->xfa == a || data2->xfb == a) {
	  assert (data2 == data);
	  hmm.erase (__i);
	  foundit = true;
	  break;
	}
      } END_FOR_KEYS;
      assert (foundit);

      // delete once
      delete data;

      // then reset the iterator
      again = true;
      break;
    } END_FOR_KEYS;
  } while (again);

  assert (hmm.end() == hmm.find(a));
  all_scans.erase (a);

  cerr << "Nuked all pairs for " << nameFromTbObj (a) << endl;
}


void
GlobalReg::deleteAutoPairs (float thr, TbObj *a)
{
  bool again;
  int  cnt = 0;
  do {
    again = false;

    // deletion invalidates existing iterators, so don't go
    // through the
    // FOR_MAP_ENTRIES loop more than once; exit it and restart.
    FOR_MAP_ENTRIES (key, data) {

      if (a != NULL && data->xfa != a && data->xfb != a) continue;

      if (!data->manually_aligned &&
	  data->pw_plane_rmsErr > thr) {

	cnt++;
	deletePair(data->xfa, data->xfb, true);

	// then reset the iterator
	again = true;
	break;
      }
    } END_FOR_MAP;
  } while (again);

  cerr << "Nuked " << cnt
       << " auto pairs greater than " << thr << endl;
}


void
GlobalReg::showPointpairCount(TbObj *a, TbObj *b)
{
  assert(a);
  if (b == NULL) {
    cout << endl << "From "
	 << nameFromTbObj(a) << " to" << endl;
    cout << "count\trmsErr\tglobalErr\tscan" << endl;
  }
  bool found = false;
  FOR_MATCHING_KEYS(a, data) {
    if (b == NULL) {
      cout << data->ptsa.size() << "\t";
      cout << data->rmsErr << "\t";
      cout << data->maxErr << "\t";
      if (data->xfa == a) cout << nameFromTbObj(data->xfb);
      else                cout << nameFromTbObj(data->xfa);
      cout << endl;
      found = true;
    } else if (data->xfa == b || data->xfb == b) {
      cout << endl << data->ptsa.size() << "\t";
      cout << data->rmsErr << "\t";
      cout << data->maxErr << "\t";
      cout << nameFromTbObj(a) << "\t";
      cout << nameFromTbObj(b) << endl;
      found = true;
      break;
    }
  } END_FOR_KEYS;
  if (!found) {
    cout << "Couldn't find such pair!" << endl;
  }
}


bool
GlobalReg::pairRegistered (TbObj* a, TbObj* b,
			   bool &manual,
			   bool transitiveAllowed)
{
  FOR_MATCHING_KEYS(a, data) {
    if (data->xfa == a && data->xfb == b ||
	data->xfb == a && data->xfa == b) {
      manual = data->manually_aligned;
      return true;
    }
  } END_FOR_KEYS;

  manual = false;

  if (transitiveAllowed) {
    // create a vector of the TbObj pointers
    vector<TbObj*> scan;
    int inda = -1, indb = -1;
    for (ITS is = all_scans.begin(); is!=all_scans.end(); is++) {
      if (*is == a)
	inda = scan.size();
      if (*is == b)
	indb = scan.size();
      scan.push_back(*is);
    }
    if (inda == -1 || indb == -1) {
      // one of the scans has no partners at all
      return false;
    }

    // find the connected components
    ConnComp cc(scan.size());
    FOR_MAP_ENTRIES(key, data) {
      if (key == data->xfb) continue;
      cc.connect(scan_idx(data->xfa, scan),
		 scan_idx(data->xfb, scan));
    } END_FOR_MAP;

    // find connected component containing a
    vector<int> g;
    while (cc.get_next_group(g)) {
      // is a here?  if not, go to next group
      if (CONTAINS(g, inda))
	continue;

      // this is a's group... is b here?
      if (CONTAINS(g, indb))
	return true;  // b found in same group as a
      else
	return false; // a's here but b's not
    }
    // only get here if we never found a
    cerr << "Internal error in "
	 << "GlobalReg::pairRegistered(transitive)" << endl;
  }

  return false;
}


crope
GlobalReg::list_partners (TbObj* mesh, bool transitiveAllowed)
{
  crope response;

  if (!transitiveAllowed) {
    FOR_MATCHING_KEYS (mesh, data) {
      TbObj* other = data->xfa;
      if (other == mesh)
	other = data->xfb;
      if (!response.empty())
	response += crope (" ");
      response += nameFromTbObj(other);
    } END_FOR_KEYS;

  } else {

    // create a vector of the TbObj pointers
    vector<TbObj*> scan;
    int ind = -1;
    for (ITS is = all_scans.begin(); is!=all_scans.end(); is++) {
      if (*is == mesh)
	ind = scan.size();
      scan.push_back(*is);
    }
    if (ind != -1) {
      // find the connected components
      ConnComp cc(scan.size());
      FOR_MAP_ENTRIES(key, data) {
	if (key == data->xfb) continue;
	cc.connect(scan_idx(data->xfa, scan),
		   scan_idx(data->xfb, scan));
      } END_FOR_MAP;

      // find connected component containing a
      vector<int> g;
      while (cc.get_next_group(g)) {
	// is a here?  if not, go to next group
	if (CONTAINS(g, ind))
	  continue;

	// this is a's group... dump it
	for (int i = 0; i < g.size(); i++) {
	  if (!response.empty())
	    response += crope (" ");
	  response += nameFromTbObj(scan[g[i]]);
	}
	break;
      }
    }
  }

  return response;
}


int
GlobalReg::count_partners (TbObj* mesh)
{
  int partners = 0;
  FOR_MATCHING_KEYS (mesh, data) {
    ++partners;
  } END_FOR_KEYS;

  return partners;
}


void
GlobalReg::dump_connected_groups (void)
{
  // create a vector of the TbObj pointers
  vector<TbObj*> scan;
  for (ITS is = all_scans.begin(); is!=all_scans.end(); is++) {
    scan.push_back(*is);
  }

  // find the connected components
  ConnComp cc(scan.size());
  FOR_MAP_ENTRIES(key, data) {
    if (key == data->xfb) continue;
    cc.connect(scan_idx(data->xfa, scan),
	       scan_idx(data->xfb, scan));
  } END_FOR_MAP;

  // dump contents of each group
  vector<int> g;
  int nGroups = 0;
  while (cc.get_next_group(g)) {
    if (g.size()) {
      cout << "group (" << g.size() << "): ";
// STL Update
      for (vector<int>::iterator p = g.begin(); p != g.end(); p++)
	cout << nameFromTbObj (scan[*p]) << " ";
      cout << endl;
      ++nGroups;
    }
  }

  cout << "(" << nGroups << ") total" << endl;
}

// the old version of align (no area-normalization)
void
GlobalReg::align(float _ftol,
		 TbObj* scanToMove, TbObj* scanToMoveTo)
{
  ftol   = _ftol;

  if (scanToMove) {
    // just move one scan
    align_one_to_others (scanToMove, scanToMoveTo);

  } else {
    // align scans in connected groups

    // create a vector of the TbObj pointers
    vector<TbObj*> scan;
    for (ITS is = all_scans.begin(); is!=all_scans.end(); is++) {
      scan.push_back(*is);
    }

    // find the connected components
    ConnComp cc(scan.size());
    FOR_MAP_ENTRIES(key, data) {
      if (key == data->xfb) continue;
      cc.connect(scan_idx(data->xfa, scan),
		 scan_idx(data->xfb, scan));
    } END_FOR_MAP;
    // for each connected component, align
    vector<int> g;
    vector<TbObj*> group;
    while (cc.get_next_group(g)) {
      if (g.size() > 1) {
	group.clear();
	for (int i=0; i<g.size(); i++)
	  group.push_back(scan[g[i]]);
	align_group(group);
      }
      if (BailDetector::bail()) {
	cerr << "global alignment cancelled." << endl;
	break;
      }
    }
    dirty_scans.erase(dirty_scans.begin(), dirty_scans.end());
  }
}

// NEW VERSION of align
// this align function will area-normalize (whereas
// the above fn that does not take a worldBbox won't)

// TODO: use worldBbox instead of re-computing
void
GlobalReg::align(float _ftol, Bbox worldBbox,
		 TbObj* scanToMove, TbObj* scanToMoveTo)
{
  ftol   = _ftol;

  normalize_samples(worldBbox, scanToMoveTo);

  if (scanToMove) {
    // just move one scan
    align_one_to_others (scanToMove, scanToMoveTo);

  } else {
    // align scans in connected groups

    // create a vector of the TbObj pointers
    vector<TbObj*> scan;
    for (ITS is = all_scans.begin(); is!=all_scans.end(); is++) {
      scan.push_back(*is);
    }

    // find the connected components
    ConnComp cc(scan.size());
    FOR_MAP_ENTRIES(key, data) {
      if (key == data->xfb) continue;
      cc.connect(scan_idx(data->xfa, scan),
		 scan_idx(data->xfb, scan));
    } END_FOR_MAP;
    // for each connected component, align
    vector<int> g;
    vector<TbObj*> group;
    while (cc.get_next_group(g)) {
      if (g.size() > 1) {
	group.clear();
	for (int i=0; i<g.size(); i++)
	  group.push_back(scan[g[i]]);
	align_group(group);
      }
      if (BailDetector::bail()) {
	cerr << "global alignment cancelled." << endl;
	break;
      }
    }
    dirty_scans.erase(dirty_scans.begin(), dirty_scans.end());
  }

}

// normalize_samples goes through the map entries and throws
// out points based on a bucket structure that keeps track of
// how many points are already in the unit volume to which
// the new points belong.  only the set stored in scanToMoveTo
// in align(...) should be normalized in this way.
void GlobalReg::normalize_samples(Bbox worldBbox, TbObj *scanToMoveTo)
{
  VolumeBucket *vb;

  cerr << "initializing buckets...";
  int numBuckets = init_vol_buckets(&vb, worldBbox, scanToMoveTo,
				    VB_SIZE);
  cerr << "done" << endl;
  vector<TbObj*> scans; // vectorized version of scanToMoveTo

  // check to see if we're aligning with one scan or all scans
  if (scanToMoveTo) {
    // we only have one scan - throw it in our scan vector
    scans.push_back(scanToMoveTo);
  } else { // sTMT == NULL means use all
    for (ITS is = all_scans.begin(); is!=all_scans.end(); is++) {
      scans.push_back(*is);
    }
  }

  // iterating thru our scan vector, interate thru the map to
  // find matching keys to the current scan we're considering
  // for each point in ptsa (don't check ptsb because corresponds),
  // - find which bucket it belongs to
  // - check that bucket.
  // - if too full, delete points from a and b
  // - otherwise, throw the new points in the bucket

  int bucketInd;

  cerr << "iterating through points..." << scans.size() << " scans"
       << endl;
  for (int i = 0; i < scans.size(); i++) {
    // data is a mapEntry
    FOR_MATCHING_KEYS(scans[i], data) {
      vector<Pnt3>::iterator ip, ip2;
      ip = data->ptsa.begin();
      ip2 = data->ptsb.begin();
      for (int j = 0; j < data->ptsa.size(); j++) {
	assert(ip != data->ptsa.end());
	assert(ip2 != data->ptsb.end());
	// changed
	Pnt3 temp(data->ptsa[j]);

	scans[i]->xformPnt(temp);
	cerr << "pt: " << temp[0] << " " << temp[1] << " "
	     << temp[2];
	bucketInd = find_bucket(temp, vb, VB_SIZE, numBuckets);
	if (bucketInd == NOT_FOUND) {
	  temp.set((data->ptsb[j])[0],
		   (data->ptsb[j])[1], (data->ptsb[j])[2]);
	  scans[i]->xformPnt(temp);
	  bucketInd = find_bucket(temp, vb, VB_SIZE, numBuckets);
	  assert(bucketInd != NOT_FOUND);
	}
	//if (should_delete_point(data->ptsa[j], vb, bucketInd)) {
	if (vb[bucketInd].points.size() > THRESHOLD) {

	  data->ptsa.erase(ip);
	  data->ptsb.erase(ip2);
	  cerr << "...throwing out" << endl;

	} else {

	  vb[bucketInd].points.push_back(temp);
	  //vb[bucketInd].points.push_back(data->ptsa[j]);
	  //vb[bucketInd].points.push_back(data->ptsb[j]);
	  cerr << "...keeping" << endl;
	}

	ip++; ip2++;
      } // for j
    } END_FOR_KEYS;
  } // for i

  cerr << "done" << endl;
  free(vb);

}

// linear time vector delete
// int k = 0;
// 	  for (vector<Pnt3>::iterator ip = data->ptsa.begin();
// 	       ip != data->ptsa.end(); ip++) {
// 	    cerr << "looping a..., k = " << k << ", j = "
// 		 << j << endl;
// 	    if (k == j) {
// 	      data->ptsa.erase(ip); break;
// 	    }
// 	    k++;
// 	  }
// 	  k = 0;
// 	  for (vector<Pnt3>::iterator ip2 = data->ptsb.begin();
// 	       ip2 != data->ptsb.end(); ip2++) {
// 	    cerr << "looping b..., k = " << k << ", j = "
// 		 << j << endl;
// 	    if (k == j) {
// 	      data->ptsb.erase(ip2); break;
// 	    }
// 	    k++;
// 	  }

bool GlobalReg::should_delete_point(Pnt3 pt, VolumeBucket *vb,
				    int bucketInd)
{
  if (vb[bucketInd].points.size() > THRESHOLD) return (true);

  // put poisson disk jitter here - efficient way to do this?
  // if we go thru all the points in the bucket, it will be
  // very slow...

  return (false);
}

// figure out how large the space is
// divide to make some number of buckets
// store all the min and max vars for those buckets
int GlobalReg::init_vol_buckets(VolumeBucket **vb,
				Bbox worldBbox,
				TbObj *scanToMoveTo, float bucket_side)
{
  Pnt3 min;
  Bbox bbox; // bbox around all the scans in scanToMoveTo

  if (scanToMoveTo) {
    bbox.add(scanToMoveTo->worldBbox());
  } else { // sTMT == NULL means use all
    for (ITS is = all_scans.begin(); is != all_scans.end(); is++) {
      TbObj *key = *is;
      bbox.add(key->worldBbox());
    }
  }

  min.set(bbox.min()[0], bbox.min()[1], bbox.min()[2]);
  Pnt3 dim = bbox.max() - min;
  dim /= bucket_side;
  Pnt3 xincr(bucket_side, 0, 0);
  Pnt3 yincr(0, bucket_side, 0);
  Pnt3 zincr(0, 0, bucket_side);

  int numBuckets = ceil(dim[0]) * ceil(dim[1]) * ceil(dim[2]);
  cerr << "Initializing " << numBuckets << " buckets...";
  *vb = new VolumeBucket[numBuckets];
  assert(*vb);
  Pnt3 vol_size(bucket_side, bucket_side, bucket_side);
  Pnt3 max;
  int cnt = 0;

  for (int i = 0; i < ceil(dim[0]); i++) {

    for (int j = 0; j < ceil(dim[1]); j++) {

      for (int k = 0; k < ceil(dim[2]); k++) {

	max = min + vol_size;

	((*vb)[cnt]).volume = Bbox(min, max);
	min += zincr;
	cnt++;
      }
      min.set(min[0], min[1], bbox.min()[2]);
      min += yincr;
    }
    min.set(min[0], bbox.min()[1], min[2]);
    min += xincr;
  }
  assert(cnt == numBuckets);
  return (numBuckets);
}

// TODO: optimize so runs in constant time
// (currently linear time)
int GlobalReg::find_bucket(Pnt3 pt, VolumeBucket *vb,
			   float bucket_side, int numBuckets)
{
  for (int i = 0; i < numBuckets; i++) {
    if (!vb[i].volume.outside(pt)) {
      return (i);
    }
  }

  return (NOT_FOUND);
  //return (1);
}

std::string
GlobalReg::dump_meshpairs (int   choice,
			   float listThresh)
{
  vector<crope> names;
  vector<TbObj*> objs;

// STL Update
  for (vector<DisplayableMesh*>::iterator dm = theScene->meshSets.begin();
       dm < theScene->meshSets.end(); dm++) {
    if ((*dm)->getVisible()) {
      names.push_back ((*dm)->getName());
      objs.push_back ((*dm)->getMeshData());
    } else {
      cout << "Excluding invisible mesh "
	   << (*dm)->getName() << endl;
    }
  }

  int i, j;
  // write 2 digit numbers, horizontal row
  printf ("\n%35s", "");
  for (i = 0; i < names.size(); i++) {
    printf (" %d ", i / 10);
  }
  printf ("\n");
  printf ("%35s", "");
  for (i = 0; i < names.size(); i++) {
    printf (" %d ", i % 10);
  }
  printf ("\n\n");

  multimap<const float, int, greater<float> > sortedList;
  float pointError, planeError, globalError;
  bool  manual;
  for (i = 0; i < names.size(); i++) {
    printf ("%28s (%02d): ", names[i].c_str(), i);
    int nPartners = 0;
    for (j = 0; j < names.size(); j++) {
      char code[3];
      sprintf(code," -");
      bool paired = getPairError (objs[i], objs[j],
				  pointError,
				  planeError,
				  globalError,
				  manual);
      float error;
      if      (choice == 0) error = pointError;
      else if (choice == 1) error = planeError;
      else                  error = globalError;
      if (paired) {
	++nPartners;
	int i_err = int(error*10 + 0.5);
	if (i_err > 99) i_err = 99;
	sprintf(code,"%02d",i_err);
	if (i > j)
	  sortedList.insert(pair<const float, int>(error,
						   i*65000+j));
      }

      // if we're at/past i, it's diagonal or upper right --
      // duplicates, don't print
      if (j == i)
	sprintf(code," O");
      else if (j > i)
	sprintf(code,"  ");
      if (paired && manual) printf ("%s.", code);
      else                  printf ("%s ", code);
    }

    printf ("(%d partners)\n", nPartners);
  }

  printf ("\n");
  printf ("Meshes and their unsorted pairwise errors:\n");
  printf ("\n");

  for (i = 0; i < names.size(); i++) {
    printf ("%28s:", names[i].c_str());
    for (j = 0; j < names.size(); j++) {
      bool paired = getPairError (objs[i], objs[j],
				  pointError,
				  planeError,
				  globalError,
				  manual);
      float error;
      if      (choice == 0) error = pointError;
      else if (choice == 1) error = planeError;
      else                  error = globalError;
      if (paired) printf(" %1.2f", error);
    }
    printf ("\n");
  }

  std::string ans;

  if (listThresh < FLT_MAX) {
     cout << endl << "List of worst offenders:" << endl;
     multimap<const float, int, greater<float> >::iterator it;
     for (it = sortedList.begin(); it != sortedList.end(); it++) {
	float err = (*it).first;
	if (err > listThresh) {
	   unsigned int tricky_code = (*it).second;
	   unsigned int i = tricky_code / 65000;
	   unsigned int j = tricky_code % 65000;

	   bool paired = getPairError (objs[i], objs[j],
				       pointError,
				       planeError,
				       globalError,
				       manual);
	   ans += names[i].c_str(); ans += " ";
	   ans += names[j].c_str();
	   if (manual) {
	     printf("%s to %s: %g \t(manual %f)\n",
		    names[i].c_str(), names[j].c_str(),
		    err, planeError);
	     ans += " manual ";
	   } else {
	     printf("%s to %s: %g \t(auto %f)\n",
		    names[i].c_str(), names[j].c_str(),
		    err, planeError);
	     ans += " auto ";
	   }
	   char str[256];
	   sprintf(str, "%f ", err);
	   ans += str;
	} else {
	   break;
	}
     }
  }
  cout << endl;
  return ans;
}


static void
update_getPairingSummary_fields (GlobalReg::mapEntry* data,
				 GlobalReg::ERRMETRIC metric,
				 int count[3], float err[3], int qual[4])
{
  ++count[0];
  if (data->manually_aligned)
    ++count[1];
  else
    ++count[2];

  // choice of error metrics:
  // maxErr, avgErr, rmsErr, pw_point_rmsErr, pw_plane_rmsErr
  float errSrc;
  switch (metric) {
  case GlobalReg::errmetric_max: errSrc = data->maxErr;          break;
  case GlobalReg::errmetric_avg: errSrc = data->avgErr;          break;
  case GlobalReg::errmetric_rms: errSrc = data->rmsErr;          break;
  case GlobalReg::errmetric_pln: errSrc = data->pw_plane_rmsErr; break;
  default:  // this is the only one populated by a single icp
  case GlobalReg::errmetric_pnt: errSrc = data->pw_point_rmsErr; break;
  }

  err[0] = min (err[0], errSrc);
  err[1] += errSrc;
  err[2] = max (err[2], errSrc);

  ++qual[data->getQuality()];
}


bool
GlobalReg::getPairingSummary (TbObj* mesh, ERRMETRIC metric,
			      int count[3], float err[3], int qual[4])
{
  count[0] = count[1] = count[2] = 0;
  qual[0] = qual[1] = qual[2] = qual[3] = 0;
  err[0] = 1e6; err[1] = err[2] = 0;

  if (mesh) {

    FOR_MATCHING_KEYS(mesh, data) {
      update_getPairingSummary_fields (data, metric, count, err, qual);
    } END_FOR_KEYS;

  } else {

    FOR_MAP_ENTRIES (mesh, data) {
      update_getPairingSummary_fields (data, metric, count, err, qual);
    } END_FOR_MAP;

  }

  if (count[0])
    err[1] /= count[0];
  else
    err[0] = 0;

  if (!mesh) { // don't double-count
    for (int i = 0; i < 3; i++) count[i] /= 2;
    for (int i = 0; i < 4; i++) qual[i] /= 2;
  }

  return true;
}


bool
GlobalReg::getPairingStats (TbObj* from, TbObj* to,
			    bool& bManual, int& iQuality,
			    int& nPoints, time_t& date,
			    float err[5])
{
  FOR_MATCHING_KEYS(from, data) {
    if (data->xfa == to || data->xfb == to) {
      bManual = data->manually_aligned;
      iQuality = data->getQuality();
      nPoints = data->ptsa.size();
      date = data->modifyDate;
      err[0] = data->maxErr;
      err[1] = data->avgErr;
      err[2] = data->rmsErr;
      err[3] = data->pw_point_rmsErr;
      err[4] = data->pw_plane_rmsErr;
      return true;
    }
  } END_FOR_KEYS;

  return false;
}


#ifdef MAIN_GR

#include "Random.h"


void
main(void)
{
  GlobalReg gr;

  int nviews = 10;
  vector<Xform<float> > xf(nviews);
  Random rnd;

  int i,j,k;

  // random initial transforms
  for (i=0; i<nviews; i++) {
    float fact = .5;
    xf[i].rotX((rnd()-.5)*M_PI*fact);
    xf[i].rotY((rnd()-.5)*M_PI*fact);
    xf[i].rotZ((rnd()-.5)*M_PI*fact);
    xf[i].translate((rnd()-.5)*fact,
		     (rnd()-.5)*fact,
		     (rnd()-.5)*fact);
  }

  vector<int> views;
  int ti;
  for (i=0; i<nviews; i++) {
    // pair each view randomly with 3 other views
    views.clear();
    views.push_back(i);
    for (j=0; j<3; j++) {
      do {
	ti = nviews*rnd(); ti %= nviews;
      } while (CONTAINS(views,ti));
      views.push_back(ti);
    }
    // create 3000 random point pairs with each view pair
    vector<Pnt3> P(3000),Q(3000);
    for (j=1; j<4; j++) {
      for (k=0; k<3000; k++) {
	// get a random point
	Pnt3 p(rnd(), rnd(), rnd());
	// store it in world coordinates according the
	// current transformations
	xf[views[0]].apply(p,P[k]);
	xf[views[j]].apply(p,Q[k]);
	// add some noise to points
	P[k] += Pnt3(rnd()*.001-.0005,
		     rnd()*.001-.0005,
		     rnd()*.001-.0005);
	Q[k] += Pnt3(rnd()*.001-.0005,
		     rnd()*.001-.0005,
 		     rnd()*.001-.0005);

      }
      gr.addPair(&xf[i], &xf[views[j]], P, Q);
    }
  }

  float ftol = 1.e-3;

  copy(xf.begin(), xf.end(),
       ostream_iterator<Xform<float> >(cout, "\n"));

  gr.align(ftol, alignHorn);

  copy(xf.begin(), xf.end(),
       ostream_iterator<Xform<float> >(cout, "\n"));
}
#endif // MAIN_GR

