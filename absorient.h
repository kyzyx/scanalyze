//############################################################
// absorient.h
// Kari Pulli
// 06/06/97
//
// Two methods for solving the rotation and translation for
// registering two sets of 3D points.
//
// Horn's method takes as input two lists of corresponding
// 3D points. It calculates in one step the optimum motion
// to align the sets. The starting point does not have to
// be close to the solution.
//
// Chen & Medioni's method takes as input a list of points
// and a list of planes, one plane for each point. The
// algorithm tries to move the sets so that the points get
// close to their corresponding planes. It is assumed that
// only small rotations are needed, i.e., the sets are
// in approximate registration.
//############################################################


#ifndef _ABSORIENT_H_
#define _ABSORIENT_H_


typedef enum {
  alignHorn,
  alignChenMedioni
} AlignmentMethod;


//
// Alignment methods:
//   horn_align does point-to-point
//   chen_medioni does point-to-plane
// Both methods calculate the transformation necessary to align
// src to dst, as a quaternion rotation and a translation.
//
// Meanings of parameters:
//   src        Source points to align
//   dst        Target positioning
//   nrmDst     Normals at points in dst
//   n          Number of point pairs
//   q[7]       Registration quaternion; 0..3 rot, 4..6 trans
//


// Horn's method: calculates transformation from points to points

void
horn_align(Pnt3 *src,      // source points to align
	   Pnt3 *dst,      // target orientation
	   int   n,        // how many pairs
	   double q[7]);   // registration quaternion


// Chen-Medioni method: calculates transformation from points to planes

void
chen_medioni(Pnt3 *src,    // source points to align
	     Pnt3 *dst,    // target orientation (points on tangent planes)
	     Pnt3 *nrmDst, // and orientation of planes at target
	     int   n,      // how many pairs
	     double q[7]); // registration quaternion


#endif // _ABSORIENT_H_

