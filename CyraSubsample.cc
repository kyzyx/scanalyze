
//############################################################
//
// CyraSubsample.cc
//     (Extra functions for CyraScan.cc)
//
// Lucas Pereira
// Thu Jul 16 15:20:47 1998
//
// Compute subsampling on Cyra scans, allowing different
// filters, while still passing along confidence values.
//
//############################################################

#include "CyraScan.h"
#include "defines.h"
#include "TriMeshUtils.h"
#include "KDindtree.h"
#include "ColorUtils.h"
#include <stdio.h>
#include <iostream.h>
#include <fstream.h>
#include <stack.h>



// This function, given an original mesh, does mxn subsampling,
// using the given filter
bool
CyraResLevel::Subsample(CyraResLevel &original, int m, int n,
			CyraFilter filter)
{
  // Make width, height the ceilings of dividing by mxn
  width = (original.width + m-1) / m;
  height = (original.height +n-1) / n;

  if (width < 2 || height < 2) {
    // there will be no triangles
    return false;
  }

  numpoints = 0;
  numtris = 0;
  points.clear();
  tris.clear();
  origin = original.origin;


  cerr << "Subsample:  " << width << " x " << height <<endl;

  //////////// Subsample the points ////////////
  switch (filter) {
  case cfPOINT:
    // Point filter:  Just pick vertex at llc.
   if (!PointFilter(original, m, n)) {
      return false;
    }
    break;
    //  case cfBOX:
    //    break;
    //  case cfMEDIAN:
    //    break;
  case cfMEAN50:
    // The mean of the middle 50%.
   if (!Mean50Filter(original, m, n)) {
      return false;
    }
    break;
  default:
    cerr << "CyraSubsample: Filter not implemented." << endl;
    return false;
  }

  //////////// Recompute the triangulation ////////////
  // BUGBUG:  For now, this returns only TESS14 or TESS0
  CyraTess tess = TESS14;
  for (int xx=0; xx < width-1; xx++) {
    for (int yy=0; yy < height-1; yy++) {
      tess = TESS14;
      for (int mm=0; mm < m; mm++) {
	for (int nn=0; nn < n; nn++) {
	  int xind = MIN(xx*m+mm, original.width-2);
	  int yind = MIN(yy*n+nn, original.height-2);
	  if (original.tri(xind, yind) != TESS14) {
	    tess = TESS0;
	    break;
	  }
	}
      }
      tris.push_back(tess);
      if (tess == TESS14) numtris += 2;
    }
  }

  // Recompute the vertex normals
  CalcNormals();


  return true;
}

// This function does point sampling -- picks the vertex at the
// lower left corner.  Assumes setup and triangulation are handled
// by Subsample.
bool
CyraResLevel::PointFilter(CyraResLevel &original, int m, int n)
{
  CyraSample samp;
  for (int xx=0; xx < width; xx++) {
    for (int yy=0; yy < height; yy++) {
      // Grab the right sample from the original
      samp = original.point(xx*m, yy*n);
      // and stick it into this guy
      points.push_back(samp);
      if (samp.confidence) numpoints++;
    }
  }
  return true;
}


// This function first selects the median 50%, and then does a mean
// filter on those samples.
//
// Note:  For now, it will only take into account the first 64 samples.
// if max is larger than that, samples will be ignored...
#define MAX_VALID_SAMPS 64
#define OUTLIER_THRESH 50 // mm
bool
CyraResLevel::Mean50Filter(CyraResLevel &original, int m, int n)
{
  CyraSample samp;
  // pointer array to first 64 valid samples...
  CyraSample *validSamp[MAX_VALID_SAMPS];

  cerr << "Running Mean50 filter, " << m << " x " << n << endl;

  for (int xx=0; xx < width; xx++) {
    for (int yy=0; yy < height; yy++) {
      int origxx = xx * m;
      int origyy = yy * m;
      int origxend = MIN(origxx + m, original.width);
      int origyend = MIN(origyy + n, original.height);

      // First count the number of valid samples....
      int ox, oy;
      int nvalid=0;
      for (ox = origxx; ox < origxend; ox++) {
	for (oy = origyy; oy < origyend && nvalid < MAX_VALID_SAMPS; oy++) {
	  // if sample is valid, add it to the list of valid samps...
	  if (original.point(ox,oy).vtx[2] > 0) {
	    validSamp[nvalid++] = &(original.point(ox,oy));
	  }
	}
      }

      // Special case 0, 1, 2, 3, 4 valid samples....
      // Set samp, and then we'll push it on the list later...
      switch (nvalid) {
      case 0:
	// If no valid samples, set to empty. E.g., just copy
	// the first point, since it's empty, too...
	samp = original.point(origxx, origyy);
	break;
      case 1:
	// Just grab the first sample...
	samp = *(validSamp[0]);
	break;
      case 2:
	// Average the two samples together...
	samp.interp(*(validSamp[0]), *(validSamp[1]));
	break;
      case 3:
	{ // open scope for local vars...
	  // Compute the range of depth values in valid samples...
	  CyraSample *bigz = validSamp[0];
	  CyraSample *litz = validSamp[0];
	  for (int i = 1; i < 3; i++) {
	    if (validSamp[i]->vtx[2] > bigz->vtx[2]) {
	      bigz = validSamp[i];
	    } else if (validSamp[i]->vtx[2] < litz->vtx[2]) {
	      litz = validSamp[i];
	    }
	  }
	  // Set midz to be the middle sample...
	  CyraSample *midz = validSamp[0];
	  for (i=1; i < 3; i++) {
	    if (validSamp[i] != bigz && validSamp[i] != litz) {
	      midz = validSamp[i];
	    }
	  }
	  // If they differ by a lot, then just grab the
	  // middle one.  Otherwise, average them together...
	  if (bigz - litz > OUTLIER_THRESH) {
	    samp = *(midz);
	  } else {
	    samp.interp(*litz, *midz, *bigz);
	  }
	} // close localvar scope...
	break;
      case -2:
	{ // open scope for local vars...
	  // For four samples, take two middles...
	  // Compute the range of depth values in valid samples...
	  CyraSample *bigz = validSamp[0];
	  CyraSample *litz = validSamp[0];
	  int bign = 0;
	  int litn = 0;
	  for (int i = 1; i < 4; i++) {
	    if (validSamp[i]->vtx[2] > bigz->vtx[2]) {
	      bigz = validSamp[i];
	      bign = i;
	    } else if (validSamp[i]->vtx[2] < litz->vtx[2]) {
	      litz = validSamp[i];
	      litn = i;
	    }
	  }

	  // now find the other two...  Basically, since switch is
	  // fast, this computes, based on the values of the big/little
	  // z values, which two other samples to interpolate...
	  switch (bign*4 + litn) {
	  case 1:
	    // 4x 0 + 1 = 1
	    samp.interp(*(validSamp[2]), *(validSamp[3]));
	    break;
	  case 2:
	    // 4x 0 + 2 = 2
	    samp.interp(*(validSamp[1]), *(validSamp[3]));
	    break;
	  case 3:
	    // 4x 0 + 3 = 3
	    samp.interp(*(validSamp[1]), *(validSamp[2]));
	    break;
	  case 4:
	    // 4x 1 + 0 = 4
	    samp.interp(*(validSamp[2]), *(validSamp[3]));
	    break;
	  case 6:
	    // 4x 1 + 2 = 6
	    samp.interp(*(validSamp[0]), *(validSamp[3]));
	    break;
	  case 7:
	    // 4x 1 + 3 = 7
	    samp.interp(*(validSamp[0]), *(validSamp[2]));
	    break;
	  case 8:
	    // 4x 2 + 0 = 8
	    samp.interp(*(validSamp[1]), *(validSamp[3]));
	    break;
	  case 9:
	    // 4x 2 + 1 = 9
	    samp.interp(*(validSamp[0]), *(validSamp[3]));
	    break;
	  case 11:
	    // 4x 2 + 3 = 11
	    samp.interp(*(validSamp[1]), *(validSamp[2]));
	    break;
	  case 12:
	    // 4x 3 + 0 = 12
	    samp.interp(*(validSamp[1]), *(validSamp[2]));
	    break;
	  case 13:
	    // 4x 3 + 1 = 13
	    samp.interp(*(validSamp[0]), *(validSamp[2]));
	    break;
	  case 14:
	    // 4x 3 + 2 = 14
	    samp.interp(*(validSamp[0]), *(validSamp[1]));
	    break;
	  default:
	    // ok, weird, they weren't different...?
	    assert(bign==litn);
	    samp.interp(*(validSamp[0]), *(validSamp[1]));
	    break;
	  }
	  // now we've set samp, so break
	} // close localvar scope...
	break;
      default:
	{ // open scope for local vars...
	  // more than 3 samples....
	  // throw out biggest/smallest pairs until we're down to halfway
	  assert(nvalid > 3);
	  int nvalidleft = nvalid;
	  while ( nvalidleft > (nvalid+1) / 2) {
	    // Find biggest/smallest z elements...
	    int bign=0;
	    int litn=0;
	    CyraSample *bigz=validSamp[0];
	    CyraSample *litz=validSamp[0];
	    for (int i = 1; i < nvalidleft; i++) {
	      if (validSamp[i]->vtx[2] > bigz->vtx[2]) {
		bigz = validSamp[i];
		bign = i;
	      } else if (validSamp[i]->vtx[2] < litz->vtx[2]) {
		litz = validSamp[i];
		litn = i;
	      }
	    }
	    // swap them off the end of the list
	    // [only a pointer redirect]
	    validSamp[bign] = validSamp[nvalidleft-1];
	    validSamp[litn] = validSamp[nvalidleft-2];
	    nvalidleft -= 2;
	  }
	  // ok, at this point, the first nvalidleft elements of validSamp
	  // are the ones to interp...
	  samp.interp(nvalidleft, validSamp);
	} // close localvar scope...
	break;
      }

      // Add it to the list...
      points.push_back(samp);
      if (samp.confidence) numpoints++;
    }
  }

  return true;
}


