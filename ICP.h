//############################################################
//
// ICP.h
//
// Kari Pulli
// Thu Jul  2 11:11:57 PDT 1998
//
// Iterative closest point method for registration
//
//############################################################

#ifndef _ICP_H_
#define _ICP_H_

#ifdef WIN32
#	include "winGLdecs.h"
#endif
#include <GL/gl.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include "Median.h"
#include "absorient.h"
#include "Xform.h"
#include "Pnt3.h"
#include "GlobalReg.h"

#include "DrawObj.h"
extern DrawObjects draw_other_things;

// The template argument is supposed to be a pointer

template <class T>
class ICP : public DrawObj {
public:
  bool allow_bdry;
private:
  T            P, Q;
  Xform<float> xfP, xfQ; // xfP is changed, xfQ stays constant

  vector<Pnt3> ssP, ssQ, ssPn, ssQn; // subsampled, local coords
  vector<Pnt3> pP, pQ, nP, nQ; // stored in world coords
  int firstend;

  Median<double> med;

  // return culling threshold
  float cull_pairs(int  p, // percentage to cull, [0,100]
		   bool cull = true)
    {
      if (p == 0) return 1.e33;

      med.set_percentage((100-p)/100.0); med.clear();
      int n = pP.size();
      vector<double> d(n);

      for (int i=0; i<n; i++) {
	med += d[i] = dist2(pP[i],pQ[i]);
      }
      double thr = med.find();

      if (cull) {
	i=0;
	int k = 0;
	for (int j=0; j<pP.size(); j++) {
	  if (d[j] <= thr) {
	    // keep
	    pP[i] = pP[j]; pQ[i] = pQ[j];
	    nP[i] = nP[j]; nQ[i] = nQ[j];
	    i++;
	  } else {
	    if (j < firstend) k++;
	  }
	}
// STL Update
	pP.erase(pP.begin()+i, pP.end());
	pQ.erase(pQ.begin()+i, pQ.end());
	nP.erase(nP.begin()+i, nP.end());
	nQ.erase(nQ.begin()+i, nQ.end());
	firstend -= k;
      }
      return sqrtf(thr);
    }



  float Horn_align(void)
    {
      // calculate the best rigid motion for alignment
      double q[7];
      horn_align(&pP[0], &pQ[0], pP.size(), q);

      // apply the motion to the current transformation
      xfP.addQuaternion(q, 7);

      Xform<double> _xf;
      _xf.addQuaternion(q, 7);
      for_each(pP.begin(), pP.end(), _xf);
      return RMS_point_to_point_error();
    }


  float CM_align(void)
    {
      // calculate the best rigid motion for alignment
      // we can do two loops of chen_medioni and the results
      // in general improve
      // third time doesn't improve much with the same pairs
      double q[7];
      for (int j=0; j<2; j++) {
	chen_medioni(&pP[0], &pQ[0], &nQ[0], pP.size(), q);

	// apply the motion to the current transformation
	xfP.addQuaternion(q, 7);

	Xform<float> _xf;
	_xf.addQuaternion (q, 7);
	for_each(pP.begin(), pP.end(), _xf);
      }
      return RMS_point_to_point_error();
    }


  void find_pairs(float thr)
    {
      pP.clear(); pQ.clear(); nP.clear(); nQ.clear();
      int n = ssP.size() + ssQ.size();

      cout << "Finding ~" << n << " points there... " << flush;

      pP.reserve(n); pQ.reserve(n); nP.reserve(n); nQ.reserve(n);
      // for each selected point, find the closest point
      Pnt3 wp,wn,lp,ln,cp,cn;
      n = ssP.size();
      // first, from P to Q
      Xform<float> xfi = xfQ; xfi.fast_invert();
      for (int i=0; i<n; i++) {
	// point to other mesh's coords
	xfP.apply(ssP[i], wp); xfP.apply_nrm(ssPn[i], wn); // world
	xfi.apply(wp, lp);     xfi.apply_nrm(wn, ln); // local in Q
	if (Q->closest_point(lp, ln, cp, cn, thr, allow_bdry)) {
	  // store world coords
	  // point
	  pP.push_back(wp);
	  xfQ(cp); pQ.push_back(cp);
	  // normal
	  nP.push_back(wn);
	  nQ.push_back(Pnt3()); xfQ.apply_nrm(cn, nQ.back());
	}
      }

      cout << "(" << pP.size() << "), and back... " << flush;

      firstend = pP.size();
      // then, from Q to P
      xfi = xfP; xfi.fast_invert();
      n = ssQ.size();
      for (i=0; i<n; i++) {
	// point to other mesh's coords
	xfQ.apply(ssQ[i], wp); xfQ.apply_nrm(ssQn[i], wn); // world
	xfi.apply(wp, lp);     xfi.apply_nrm(wn, ln); // local in P
	if (P->closest_point(lp, ln, cp, cn, thr, allow_bdry)) {
	  // store world coords
	  // point
	  pQ.push_back(wp);
	  xfP(cp); pP.push_back(cp);
	  // normal
	  nQ.push_back(wn);
	  nP.push_back(Pnt3()); xfP.apply_nrm(cn, nP.back());
	}
      }

      cout << "(" << pP.size() - firstend << "): done." << endl;
    }


public:

  ICP(void) : allow_bdry(0), firstend(0)
    {
      draw_other_things.add(this);
    }

  ~ICP(void)
    {
      draw_other_things.remove(this);
    }

  void set(T p, T q)
    {
      P = p; Q = q;
      xfP = P->getXform();
      xfQ = Q->getXform();
    }

  float RMS_point_to_point_error(void)
    {
      float len = 0;
      int n = pP.size();
      for (int i=0; i<n; i++) len += dist2(pP[i],pQ[i]);
      return sqrtf(len/n);
    }


  void sort_into_buckets(const vector<Pnt3> &n,
			 vector< vector<int> > &normbuckets)
  {
    const int Q = 4;
    const float Qsqrt1_2 = 2.8284f;
    normbuckets.resize(3*Q*Q);
    for (int i=0; i < n.size(); i++) {
      const float *N = &n[i][0];
      float ax = fabs(N[0]), ay = fabs(N[1]), az = fabs(N[2]);
      int A;
      float u, v;
      if (ax > ay) {
	if (ax > az) {
	  A = 0;  u = (N[0] > 0) ? N[1] : -N[1];  v = (N[0] > 0) ? N[2] : -N[2];
	} else {
	  A = 2;  u = (N[2] > 0) ? N[0] : -N[0];  v = (N[2] > 0) ? N[1] : -N[1];
	}
      } else {
	if (ay > az) {
	  A = 1;  u = (N[1] > 0) ? N[2] : -N[2];  v = (N[1] > 0) ? N[0] : -N[0];
	} else {
	  A = 2;  u = (N[2] > 0) ? N[0] : -N[0];  v = (N[2] > 0) ? N[1] : -N[1];
	}
      }
      int U = int(u * Qsqrt1_2) + (Q/2);
      int V = int(v * Qsqrt1_2) + (Q/2);
      normbuckets[((A * Q) + U) * Q + V].push_back(i);
    }
    for (int bucket=0; bucket < normbuckets.size(); bucket++)
      random_shuffle(normbuckets[bucket].begin(), normbuckets[bucket].end());
  }

  float align(float    sample_rate,
	      bool     normspace_sample,
	      int      n_iter,
	      int      culling_percentage,
	      int      method, // 0 plane (CM), 1 point (Horn)
	      int      thr_kind, // 0 relative, 1 absolute
	      float    thr_value)
    {
      float avgError;

      // Subsample both meshes
      if (normspace_sample) {
	// Sampling that tries to be somewhat uniform in the space of normals
	vector<Pnt3> tmpverts, tmpnorms;
	vector< vector<int> > normbuckets;
	tmpverts.reserve(max(P->num_vertices(), Q->num_vertices()));
	tmpnorms.reserve(max(P->num_vertices(), Q->num_vertices()));

	// Sample P
        P->subsample_points(1.0, tmpverts, tmpnorms);
	sort_into_buckets(tmpnorms, normbuckets);
	int ndesired = int(ceil(sample_rate * tmpverts.size()));
	ssP.clear();  ssPn.clear();
	while (ssP.size() < ndesired) {
	  for (int i=0; i < normbuckets.size(); i++) {
	    if (!normbuckets[i].empty()) {
		int ind = normbuckets[i].back();
		ssP.push_back(tmpverts[ind]);
		ssPn.push_back(tmpnorms[ind]);
		normbuckets[i].pop_back();
	    }
	  }
	}

	tmpverts.clear(); tmpnorms.clear(); normbuckets.clear();

	// Sample Q
        Q->subsample_points(1.0, tmpverts, tmpnorms);
	sort_into_buckets(tmpnorms, normbuckets);
	ndesired = int(ceil(sample_rate * tmpverts.size()));
	ssQ.clear();  ssQn.clear();
	while (ssQ.size() < ndesired) {
	  for (int i=0; i < normbuckets.size(); i++) {
	    if (!normbuckets[i].empty()) {
		int ind = normbuckets[i].back();
		ssQ.push_back(tmpverts[ind]);
		ssQn.push_back(tmpnorms[ind]);
		normbuckets[i].pop_back();
	    }
	  }
	}
      } else {
	// Good ol' random sampling
        P->subsample_points(sample_rate, ssP, ssPn);
        Q->subsample_points(sample_rate, ssQ, ssQn);
      }

      if (thr_kind == 0) {
	// relative threshold: change to absolute
	// find the smaller bounding box diagonal
	float diag = P->localBbox().diag();
	float tmp  = Q->localBbox().diag();
	if (tmp < diag) diag = tmp;
	thr_value *= diag;
      }

      if (n_iter == 0) {
	// just find pairs, mainly for debugging
	find_pairs(thr_value);
	cull_pairs(culling_percentage);
	return 0;
      }

      for (int i=0; i<n_iter; i++) {
	// find the point pairs going from set P to Q and
	// vice versa (all pts in world coords)
	find_pairs(thr_value);

	cull_pairs(culling_percentage);

	if (pP.size() == 0) {
	  cerr << "Couldn't find a single point pair!" << endl;
	  break;
	}

	if (method) avgError = Horn_align();
	else        avgError = CM_align();
	cout << avgError << endl;
      }
      P->setXform(xfP);

      return avgError;
    }

  bool too_few_pairs(void)
    {
      // if less than 10% of the points in the smaller
      // mesh didn't find pair, don't even try
      if (ssP.size() < ssQ.size()) {
	if (pP.size() <= .1 * ssP.size()) {
	  /*
	  cerr << "ICP::auto_align(): "
	       << "too few point pairs ("
	       << pP.size() << " / " << ssP.size() << ")"
	       << endl;
	  */
	  return true;
	}
      } else {
	if (pQ.size() <= .1 * ssQ.size()) {
	  /*
	  cerr << "ICP::auto_align(): "
	       << "too few point pairs ("
	       << pQ.size() << " / " << ssQ.size() << ")"
	       << endl;
	  */
	  return true;
	}
      }
      // Absolute minimum: 50 matches in each
      if ((pP.size() < 50) || (pQ.size() < 50)) {
	/*
	cerr << "ICP::auto_align(): "
	     << "too few point pairs"
	     << endl;
	*/
	return true;
      }
      return false;
    }

  // The idea of the auto_align() is to find good point pairs
  // for auto-calibration of the scanner or for global alignment
  // within a set of scans.
  // Return whether an alignment was really tried.
  bool auto_align(vector<Pnt3> &_pP,
		  vector<Pnt3> &_nP,
		  vector<Pnt3> &_pQ,
		  vector<Pnt3> &_nQ,
		  Xform<float> &rel_xf,
		  float         final_abs_thresh = FLT_MAX, // mm
		  float         max_motion       = FLT_MAX,
		  bool normspace_sample = false)
    {

      // Sample rate is set to 10% or to get around 500 subsampled points,
      // whichever is greater
      // We'll use 100% for the last round
      float Psample_rate = min(max(500.0f / P->num_vertices(), 0.1f), 1.0f);
      float Qsample_rate = min(max(500.0f / Q->num_vertices(), 0.1f), 1.0f);

      // first, test whether the bounding boxes overlap...
      if (!P->worldBbox().intersect(Q->worldBbox())) {
	/*
	cerr << "ICP::auto_align(): "
	     << "world bounding boxes don't overlap" << endl;
	*/
	return false;
      }

      allow_bdry = false;

      // Assume the initial alignment is pretty good.
      // To find out what kind of threshold we should
      // use, find the *minimum* dimension of the two
      // bounding boxes, take 20% of that as the initial
      // relative threshold, find point pairs.
      // Absolute threshold can override this.

      float thr_value = P->localBbox().minDim();
      float tmp       = Q->localBbox().minDim();
      if (tmp < thr_value) thr_value = tmp;
      thr_value *= .2;

      if (normspace_sample)
	cerr << endl << "Using normal-space sampling..." << endl;

      // now register 4 rounds with diminishing
      // cull percentage and threshold
      for (int i=0; i<4; i++) {

	if (normspace_sample) {
	  // Sampling that tries to be somewhat uniform in the space of normals
	  vector<Pnt3> tmpverts, tmpnorms;
	  vector< vector<int> > normbuckets;
	  tmpverts.reserve(max(P->num_vertices(), Q->num_vertices()));
	  tmpnorms.reserve(max(P->num_vertices(), Q->num_vertices()));

	  // Sample P
	  P->subsample_points(1.0, tmpverts, tmpnorms);
	  sort_into_buckets(tmpnorms, normbuckets);
	  int ndesired = int(ceil(Psample_rate * tmpverts.size()));
	  ssP.clear();  ssPn.clear();
	  while (ssP.size() < ndesired) {
	    for (int i=0; i < normbuckets.size(); i++) {
	      if (!normbuckets[i].empty()) {
		int ind = normbuckets[i].back();
		ssP.push_back(tmpverts[ind]);
		ssPn.push_back(tmpnorms[ind]);
		normbuckets[i].pop_back();
	      }
	    }
	  }

	  tmpverts.clear(); tmpnorms.clear(); normbuckets.clear();

	  // Sample Q
	  Q->subsample_points(1.0, tmpverts, tmpnorms);
	  sort_into_buckets(tmpnorms, normbuckets);
	  ndesired = int(ceil(Qsample_rate * tmpverts.size()));
	  ssQ.clear();  ssQn.clear();
	  while (ssQ.size() < ndesired) {
	    for (int i=0; i < normbuckets.size(); i++) {
	      if (!normbuckets[i].empty()) {
		int ind = normbuckets[i].back();
		ssQ.push_back(tmpverts[ind]);
		ssQn.push_back(tmpnorms[ind]);
		normbuckets[i].pop_back();
	      }
	    }
	  }

	} else {
	  // random sampling
	  P->subsample_points(Psample_rate, ssP, ssPn);
	  Q->subsample_points(Qsample_rate, ssQ, ssQn);
	}
	// the interpolation doesn't intentionally go all the way
	// (which would require a 5th round)
	find_pairs(((4-i)*thr_value+i*final_abs_thresh)/5.0);
	if (too_few_pairs()) return false;
	cull_pairs(((4-i)*20+i*1)/5.0);
	CM_align();
      }

      // Now do sets of three iterations as long as the
      // results seem to get better.

      float prev_error, curr_error = 1.e33;
      Xform<float> xfPg;
      do {
	// update the error
	prev_error = curr_error;
	// copy the last good point pairs ...
	//_pP = pP; _nP = nP; _pQ = pQ; _nQ = nQ;
	// ... and the last known good transformations
	xfPg = xfP;

	// subsample both meshes
	if (normspace_sample) {
	  // Sampling that tries to be somewhat uniform in the space of normals
	  vector<Pnt3> tmpverts, tmpnorms;
	  vector< vector<int> > normbuckets;
	  tmpverts.reserve(max(P->num_vertices(), Q->num_vertices()));
	  tmpnorms.reserve(max(P->num_vertices(), Q->num_vertices()));

	  // Sample P
	  P->subsample_points(1.0, tmpverts, tmpnorms);
	  sort_into_buckets(tmpnorms, normbuckets);
	  int ndesired = int(ceil(Psample_rate * tmpverts.size()));
	  ssP.clear();  ssPn.clear();
	  while (ssP.size() < ndesired) {
	    for (int i=0; i < normbuckets.size(); i++) {
	      if (!normbuckets[i].empty()) {
		int ind = normbuckets[i].back();
		ssP.push_back(tmpverts[ind]);
		ssPn.push_back(tmpnorms[ind]);
		normbuckets[i].pop_back();
	      }
	    }
	  }

	  tmpverts.clear(); tmpnorms.clear(); normbuckets.clear();

	  // Sample Q
	  Q->subsample_points(1.0, tmpverts, tmpnorms);
	  sort_into_buckets(tmpnorms, normbuckets);
	  ndesired = int(ceil(Qsample_rate * tmpverts.size()));
	  ssQ.clear();  ssQn.clear();
	  while (ssQ.size() < ndesired) {
	    for (int i=0; i < normbuckets.size(); i++) {
	      if (!normbuckets[i].empty()) {
		int ind = normbuckets[i].back();
		ssQ.push_back(tmpverts[ind]);
		ssQn.push_back(tmpnorms[ind]);
		normbuckets[i].pop_back();
	      }
	    }
	  }

	} else {
	  P->subsample_points(Psample_rate, ssP, ssPn);
	  Q->subsample_points(Qsample_rate, ssQ, ssQn);
	}
	// do two rounds, calculate errors
	curr_error = 0.0;
	for (int i=0; i<2; i++) {
	  find_pairs(final_abs_thresh);
	  cull_pairs(1);
	  if (too_few_pairs()) return false;
	  curr_error += CM_align();
	}

      } while (curr_error < .9999 * prev_error);

      if (curr_error > prev_error) {
	// restore the last known good points
	xfP = xfPg;
	//pP = _pP; nP = _nP; pQ = _pQ; nQ = _nQ;
      }

      // once more, with no subsampling
      cout << "last round...";
      P->subsample_points(1.0, ssP, ssPn);
      Q->subsample_points(1.0, ssQ, ssQn);
      find_pairs(final_abs_thresh);
      cull_pairs(1);
      CM_align();
      cout << "done" << endl;

      if (max_motion < FLT_MAX) {
	// see how much the subsampled points in P would move
	float d = 0.0;
	Xform<float> xf = P->getXform();
	xf = xf.fast_invert() * xfP;
	for (vector<Pnt3>::const_iterator it = ssP.begin();
	     it != ssP.end(); it++) {
	  Pnt3 p = *it; xf(p);
	  d += dist(*it, p);
	}
	d /= ssP.size();
	if (d > max_motion) return false;
      }

      get_pairs_for_global(_pP, _nP, _pQ, _nQ);

      rel_xf = xfQ;
      rel_xf.fast_invert();
      rel_xf = rel_xf * xfP;

      return true;
    }


  // get the same pairs as were used in the last iteration,
  // but in local coordinates
  void get_pairs_for_global(vector<Pnt3> &_pP,
			    vector<Pnt3> &_nP,
			    vector<Pnt3> &_pQ,
			    vector<Pnt3> &_nQ)
    {
      _pP = pP;
      _nP = nP;
      Xform<float> xfi = xfP;
      xfi.fast_invert();
      for_each(_pP.begin(), _pP.end(), xfi);
      xfi.removeTranslation();
      for_each(_nP.begin(), _nP.end(), xfi);
      _pQ = pQ;
      _nQ = nQ;
      xfi = xfQ;
      xfi.fast_invert();
      for_each(_pQ.begin(), _pQ.end(), xfi);
      xfi.removeTranslation();
      for_each(_nQ.begin(), _nQ.end(), xfi);
    }

  void show_lines(void) { enable(); }
  void hide_lines(void) { disable(); }

  void drawthis(void)
    {
      glDisable(GL_LIGHTING);

      glBlendFunc(GL_ONE, GL_ONE);
      glEnable(GL_BLEND);
      glDepthFunc(GL_LEQUAL);

      glBegin(GL_LINES);
      glColor3f(1,0,0);
      for (int i=0; i<firstend; i++) {
	glVertex3fv(pP[i]); glVertex3fv(pQ[i]);
      }
      glColor3f(0,1,0);
      for (; i<pQ.size(); i++) {
	glVertex3fv(pP[i]); glVertex3fv(pQ[i]);
      }
      glEnd();
    }
};


template <class T>
class AutoICP {

private:

  ICP<T>       icp;
  GlobalReg*   regall;
  bool         bRegallPrivate;
  vector<Pnt3> pP, nP, pQ, nQ;

  float        final_abs_threshold;
  float        max_motion;

public:

  // both parameters in the same units as data, usually mm
  AutoICP(GlobalReg* pGlobalReg = NULL,
	  float _abs_threshold = FLT_MAX,
	  float _max_motion = 20.0)
    : final_abs_threshold(_abs_threshold), max_motion (_max_motion)
    {
      if (!pGlobalReg) {
	regall = new GlobalReg;
	bRegallPrivate = true;
      } else {
	regall = pGlobalReg;
	bRegallPrivate = false;
      }
    }

  ~AutoICP()
    {
      if (bRegallPrivate)
	delete regall;
    }

  bool add_pair(T a, T b, int max_pairs = 0, bool nss = false)
    {
      icp.set (a, b);
      Xform<float> rel_xf;
      bool success = icp.auto_align(pP, nP, pQ, nQ,
				    rel_xf,
				    final_abs_threshold,
				    FLT_MAX, nss);

      if (success) {
	regall->addPair (a, b, pP, nP, pQ, nQ,
			 rel_xf, false, max_pairs);
      }

      return success;
    }

  void operator()(void)
    {
      regall->align (1.e-3);
    }
};


#endif

