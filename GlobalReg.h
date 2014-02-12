//############################################################
// GlobalReg.h
// Kari Pulli
// Tue Jun 23 11:54:00 PDT 1998
//
// Global registration of several scans simultaneously
//############################################################

#ifndef _GLOBALREG_H_
#define _GLOBALREG_H_

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <ext/rope>
#include <string>
#include <time.h>
#include "Random.h"
#include "Xform.h"
#include "Pnt3.h"
#include "absorient.h"
#include "TbObj.h"
#include "Bbox.h"

class GlobalReg {
private:

  typedef Xform<float> XF_F;

  float         ftol;

  std::string   gr_dir;
  std::string   gr_auto_dir;
  char          *cyber_raw_name;

  //
  // definitions for a hash_multimap
  // it stores pointers to view transformations
  // and points from those views (in current world coordinates)
  //
 public:
  class mapEntry {
  private:
    mapEntry(const mapEntry &me);
    mapEntry &operator=(const mapEntry &me);

  public:
    TbObj   *xfa, *xfb;         // current transforms
    vector<Pnt3> ptsa, ptsb;    // points in world coordinates
    Xform<float> rel_xf;
    float   maxErr;  // point-to-point errors after globalreg
    float   avgErr;
    float   rmsErr;
    float   pw_point_rmsErr;
    float   pw_plane_rmsErr;
    bool    manually_aligned;
    enum  { qual_Unknown, qual_Bad, qual_Fair, qual_Good };
    int     quality_grade; // 0..3 = unknown, bad, fair, good
    time_t  modifyDate;

    mapEntry(TbObj *xa, TbObj *xb,
	     const vector<Pnt3> &a, const vector<Pnt3> &b,
	     Xform<float> rxf,
	     int max_pairs = 0)
      : xfa(xa), xfb(xb), ptsa(a), ptsb(b),
      manually_aligned(false), rel_xf(rxf)
      {
	if (ptsa.size() > max_pairs) {
	  // too many pairs, remove some
	  int n_left  = ptsa.size();
	  int end     = n_left;
	  int allowed = max_pairs;
	  Random rnd;
	  while (n_left && allowed) {
	    if (rnd() < float(allowed) / float(n_left)) {
	      // keep
	      n_left--;  allowed--;
	    } else {
	      // don't keep
	      n_left--;  end--;
	      ptsa[n_left] = ptsa[end];
	      ptsb[n_left] = ptsb[end];
	    }
	  }
	  if (max_pairs) {
	    // clean the vectors
// STL Update
	    ptsa.erase(ptsa.begin() + (max_pairs), ptsa.end());
	    ptsb.erase(ptsb.begin() + (max_pairs), ptsb.end());
	  }
	}
	//cout << "constructing " << int(this) << endl;
	time (&modifyDate);
      }
    ~mapEntry(void)
      {
	//cout << "destructing " << int(this) << endl;
      }

    const Pnt3 *pts(TbObj *x)
      {
	if (x == xfa) return &ptsa[0];
	if (x == xfb) return &ptsb[0];
	assert(0);
	return NULL;
      }

    int getQuality (void)
      {
	if (quality_grade > qual_Good) return qual_Good;
	if (quality_grade < qual_Unknown) return qual_Unknown;
	return quality_grade;
      }

    // return max distant, count, and sum of distants
    void stats(void);

    // try writing the mapEntry data into a single file
    void export_to_file(const std::string &gr_dir);
    // try writing the mapEntry data into a single file
    // for CyberScans, raw data
    void export_cyber_raw(const char *fname);
  };

  struct equal_tbobj {
    bool operator()(const TbObj *a, const TbObj *b) const
      { return a==b; }
  };

  struct hash_tbobj {
    size_t operator()(const TbObj *a) const
      { return (unsigned long) a; }
  };
 private:

  //hash_multimap<Key, Data, HashFcn, EqualKey, Alloc>
  typedef
  unordered_multimap<TbObj *,mapEntry*, hash_tbobj,equal_tbobj> HMM;
  typedef unordered_set<TbObj *,hash_tbobj,equal_tbobj> HS;
  typedef HMM::iterator ITT;
  typedef HS::iterator  ITS;

  HMM hmm;
  HS  all_scans;
  // When a scan is changed in some way, it gets "dirty".
  // Globalreg only aligns dirty scans to their neighbors
  // (though the others may get aligned if their neighbors
  // move enough).
  // The set is cleaned after align_group().
  HS  dirty_scans;

  // a volume bucket is defines a space in the bbox of the
  // collection of scans which we'll throw points into
  // in order to area-normalize the number of points
  // considered in the alignment
  typedef struct {

    vector<Pnt3> points;
    Bbox volume;

  } VolumeBucket;

  // helpers to normalize_points
  bool should_delete_point(Pnt3 pt, VolumeBucket *vb,
			   int bucketInd);
  int init_vol_buckets(VolumeBucket **vb,
		       Bbox worldBbox,
		       TbObj *scanToMoveTo,
		       float bucket_side = 1.0);
  int find_bucket(Pnt3 pt, VolumeBucket *vb,
		  float bucket_side, int numBuckets);
  void  get_pts(TbObj *x, vector<Pnt3> &P, vector<Pnt3> &Q,
		TbObj* partner = NULL);
  void  get_pts_within_group(TbObj *x,
			     vector<Pnt3> &P, vector<Pnt3> &Q,
			     vector<TbObj*> &group);
  void  evaluate(const vector<TbObj*> &group);
  float evaluate(TbObj *);

  void  dump(void);

  void         move_back(const vector<TbObj*> &group,
			 const vector<XF_F> &old_xf);
  Xform<float> compute_xform(vector<Pnt3> &P, vector<Pnt3> &Q);
  void align_one_to_others(TbObj* one, TbObj* two = NULL);
  void align_group(const vector<TbObj*> &group);
  bool getPairError(TbObj* a, TbObj* b,
		    float &pointError,
		    float &planeError,
		    float &globalError,
		    bool  &manual);

  bool initial_import_done;
  mapEntry* import(const std::string &fname, bool manual = false);

  void unlink_gr_files(TbObj *a, TbObj *b, bool only_auto = false);

public:

  // constructor/destructor
  GlobalReg (void);
  ~GlobalReg(void);

  void initial_import(void);
  void perform_import(void);
  // manipulate pairs
  mapEntry* addPair(TbObj *a, TbObj *b,
		    const vector<Pnt3> &ap, const vector<Pnt3> &nrma,
		    const vector<Pnt3> &bp, const vector<Pnt3> &nrmb,
		    Xform<float> rel_xf = Xform<float>(),
		    bool manually_aligned = false,
		    int  max_pairs = 0,
		    bool save = true,
		    int saveQual = mapEntry::qual_Unknown);
  mapEntry* addPair(TbObj *a, TbObj *b,
		    const vector<Pnt3> &ap,
		    const vector<Pnt3> &bp,
		    float pw_point_rmsErr,
		    float pw_plane_rmsErr,
		    Xform<float> rel_xf = Xform<float>(),
		    bool manually_aligned = false,
		    int  max_pairs = 0,
		    bool save = true,
		    int saveQual = mapEntry::qual_Unknown);
  bool markPairQuality (TbObj *a, TbObj* b,
			int saveQual = mapEntry::qual_Unknown);
  int  getPairQuality (TbObj *a, TbObj* b);
  void deletePair(TbObj *a, TbObj *b, bool deleteFile = false);
  void deleteAllPairs(TbObj *a, bool deleteFile = false);
  void deleteAutoPairs(float thr, TbObj *a = NULL);

  void showPointpairCount(TbObj *a, TbObj *b = NULL);

  //void recalcPairwiseErrors(void);

  // align: if you pass scanToMove = NULL (default), all scans
  // are aligned to all others.
  // If you pass an object as scanToMove, only pairs
  // involving that scan are considered, and for each alignment
  // it will be moved toward the other scan, which should have
  // the effect of aligning it with a group of other
  // self-registered scans without moving any of them.
  // we'll need the worldBbox so we can compute buckets for area-
  // normalization (the second align fn)
  void align(float ftol,
	     TbObj* scanToMove = NULL,
	     TbObj* scanToMoveTo = NULL);
   void align(float ftol, Bbox worldBbox,
	     TbObj* scanToMove = NULL,
	     TbObj* scanToMoveTo = NULL);

   void normalize_samples(Bbox worldBbox, TbObj *scanToMoveTo);

  // status and debugging info
  bool pairRegistered(TbObj *a, TbObj *b,
		      bool &manual,
		      bool transitiveAllowed = false);
  int   count_partners(TbObj *mesh);
  crope list_partners (TbObj *mesh,
		       bool transitiveAllowed = false);

  std::string dump_meshpairs(int   choice,
			     float listThresh = FLT_MAX);
  void dump_connected_groups (void);

  enum ERRMETRIC { errmetric_max, errmetric_avg, errmetric_rms,
		   errmetric_pnt, errmetric_pln };

  bool getPairingSummary (TbObj* mesh,     // return info on this mesh's pairs:
			  ERRMETRIC metric,// using this field for err
			  int count[3],    // total/man/auto
			  float err[3],    // min/avg/max
			  int quality[4]); // unknown/bad/fair/good

  bool getPairingStats (TbObj* from, TbObj* to,  // between these two meshes
			bool& bManual,
			int& iQuality,
			int& nPoints,
			time_t& date,
			float err[5]);
};

#endif
