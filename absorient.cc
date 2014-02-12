//############################################################
// absorient.cc
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

#include "tnt.h"
#include <cmath>
#include <limits.h>
#include <iostream>
#include "Pnt3.h"
#include "Xform.h"
#include "vec.h"
#include "cmat.h"
#include "trislv.h"
#include "transv.h"         /* transpose views */
#include "lu.h"
#include <float.h>

#ifdef WIN32
#define cbrt(r)  (pow((r), 1./3))
#endif


////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////              Horn's method starts              ////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////


// transform a quaternion to a rotation matrix
static void
quaternion2matrix(double *q, double m[3][3])
{
  double q00 = q[0]*q[0];
  double q11 = q[1]*q[1];
  double q22 = q[2]*q[2];
  double q33 = q[3]*q[3];
  double q03 = q[0]*q[3];
  double q13 = q[1]*q[3];
  double q23 = q[2]*q[3];
  double q02 = q[0]*q[2];
  double q12 = q[1]*q[2];
  double q01 = q[0]*q[1];
  m[0][0] = q00 + q11 - q22 - q33;
  m[1][1] = q00 - q11 + q22 - q33;
  m[2][2] = q00 - q11 - q22 + q33;
  m[0][1] = 2.0*(q12-q03);
  m[1][0] = 2.0*(q12+q03);
  m[0][2] = 2.0*(q13+q02);
  m[2][0] = 2.0*(q13-q02);
  m[1][2] = 2.0*(q23-q01);
  m[2][1] = 2.0*(q23+q01);
}


// find the coefficients of the characteristic eqn.
// l^4 + a l^3 + b l^2 + c l + d = 0
// for a symmetric 4x4 matrix
static void
characteristicPol(double Q[4][4], double c[4])
{
  // squares
  double q01_2 = Q[0][1] * Q[0][1];
  double q02_2 = Q[0][2] * Q[0][2];
  double q03_2 = Q[0][3] * Q[0][3];
  double q12_2 = Q[1][2] * Q[1][2];
  double q13_2 = Q[1][3] * Q[1][3];
  double q23_2 = Q[2][3] * Q[2][3];

  // other factors
  double q0011 = Q[0][0] * Q[1][1];
  double q0022 = Q[0][0] * Q[2][2];
  double q0033 = Q[0][0] * Q[3][3];
  double q0102 = Q[0][1] * Q[0][2];
  double q0103 = Q[0][1] * Q[0][3];
  double q0223 = Q[0][2] * Q[2][3];
  double q1122 = Q[1][1] * Q[2][2];
  double q1133 = Q[1][1] * Q[3][3];
  double q1223 = Q[1][2] * Q[2][3];
  double q2233 = Q[2][2] * Q[3][3];

  // a
  c[0] = -Q[0][0] - Q[1][1] - Q[2][2] - Q[3][3];

  // b
  c[1] = - q01_2 - q02_2 - q03_2 + q0011 - q12_2 -
    q13_2 + q0022 + q1122 - q23_2 + q0033 + q1133 +
    q2233;

  // c
  c[2] = (q02_2 + q03_2 + q23_2)*Q[1][1] - 2*q0102*Q[1][2] +
    (q12_2 + q13_2 + q23_2)*Q[0][0] +
    (q01_2 + q03_2 - q0011 + q13_2 - q1133)*Q[2][2] -
    2*Q[0][3]*q0223 - 2*(q0103 + q1223)*Q[1][3] +
    (q01_2 + q02_2 - q0011 + q12_2 - q0022)*Q[3][3];

  // d
  c[3] = 2*(-Q[0][2]*Q[0][3]*Q[1][2] + q0103*Q[2][2] -
	    Q[0][1]*q0223 + Q[0][0]*q1223)*Q[1][3] +
    q02_2*q13_2 - q03_2*q1122 - q13_2*q0022 +
    2*Q[0][3]*Q[1][1]*q0223 - 2*q0103*q1223 + q01_2*q23_2 -
    q0011*q23_2 - q02_2*q1133 + q03_2*q12_2 +
    2*q0102*Q[1][2]*Q[3][3] - q12_2*q0033 - q01_2*q2233 +
    q0011*q2233;
}



static int ferrari(double a, double b, double c, double d,
	  double rts[4]);

// calculate the maximum eigenvector of a symmetric
// 4x4 matrix
// from B. Horn 1987 Closed-form solution of absolute
// orientation using unit quaternions (J.Opt.Soc.Am.A)
void
maxEigenVector(double Q[4][4], double ev[4])
{
  static C_matrix<double> N(4,4);
  double rts[4];
  double c[4];
  // find the coeffs for the characteristic eqn.
  characteristicPol(Q, c);
  // find roots
  ferrari(c[0], c[1], c[2], c[3], rts);
  // find maximum root = maximum eigenvalue
  double l = rts[0];
  if (rts[1] > l) l = rts[1];
  if (rts[2] > l) l = rts[2];
  if (rts[3] > l) l = rts[3];

  /*
  SHOW(l);
  SHOW(rts[0]);
  SHOW(rts[1]);
  SHOW(rts[2]);
  SHOW(rts[3]);
  */

  // create the Q - l*I matrix
  N[0][0]=Q[0][0]-l;N[0][1]=Q[0][1] ;N[0][2]=Q[0][2]; N[0][3]=Q[0][3];
  N[1][0]=Q[1][0]; N[1][1]=Q[1][1]-l;N[1][2]=Q[1][2]; N[1][3]=Q[1][3];
  N[2][0]=Q[2][0]; N[2][1]=Q[2][1] ;N[2][2]=Q[2][2]-l;N[2][3]=Q[2][3];
  N[3][0]=Q[3][0]; N[3][1]=Q[3][1] ;N[3][2]=Q[3][2];N[3][3]=Q[3][3]-l;
  // the columns of the inverted matrix should be multiples of
  // the eigenvector, pick the largest
  static Vector<int> ipiv(4);
  static Vector<double> best(4),curr(4);
  //C_matrix<double> Ntmp = N;
  if (LU_factor(N, ipiv)) {
    //SHOW(Q[0][0]);
    cerr << "maxEigenVector():" << endl;
    cerr << "LU_factor failed!" << endl;
    cerr << "return identity quaternion" << endl;
    //cerr << N << endl;
    ev[0] = 1.0;
    ev[1] = ev[2] = ev[3] = 0.0;
    return;
  }
  best = 0; best[0] = 1;
  LU_solve(N, ipiv, best);
  double len =
    best[0]*best[0] + best[1]*best[1] +
    best[2]*best[2] + best[3]*best[3];
  for (int i=1; i<4; i++) {
    curr = 0; curr[i] = 1;
    LU_solve(N, ipiv, curr);
    double tlen =
      curr[0]*curr[0] + curr[1]*curr[1] +
      curr[2]*curr[2] + curr[3]*curr[3];
    if (tlen > len) { len = tlen; best = curr; }
  }
  // normalize the result
  len = 1.0/sqrt(len);
  ev[0] = best[0]*len;
  ev[1] = best[1]*len;
  ev[2] = best[2]*len;
  ev[3] = best[3]*len;
}

// find the transformation that aligns measurements to
// model
void
horn_align(Pnt3 *p,      // the model points  (source)
	   Pnt3 *x,      // the measured points (destination)
	   int n,        // how many pairs
	   double q[7])  // the reg. vector 0..3 rot, 4..6 trans
{
  if (n<3) {
    cerr << "horn_align() was given " << n << " pairs,"
	 << " while at least 3 are required" << endl;
    return;
  }

  Pnt3 p_mean, x_mean;
  double S[3][3];
  double Q[4][4];
  int i,j;

  // calculate the centers of mass
  for (i=0; i<n; i++) {
    p_mean += p[i];
    x_mean += x[i];
  }
  p_mean /= n;
  x_mean /= n;
  // calculate the cross covariance matrix
  for (i=0; i<3; i++)
    for (j=0; j<3; j++)
      S[i][j] = 0;
  for (i=0; i<n; i++) {
    S[0][0] += p[i][0]*x[i][0];
    S[0][1] += p[i][0]*x[i][1];
    S[0][2] += p[i][0]*x[i][2];
    S[1][0] += p[i][1]*x[i][0];
    S[1][1] += p[i][1]*x[i][1];
    S[1][2] += p[i][1]*x[i][2];
    S[2][0] += p[i][2]*x[i][0];
    S[2][1] += p[i][2]*x[i][1];
    S[2][2] += p[i][2]*x[i][2];
  }
  double fact = 1/double(n);
  for (i=0; i<3; i++)
    for (j=0; j<3; j++)
      S[i][j] *= fact;
  S[0][0] -= p_mean[0]*x_mean[0];
  S[0][1] -= p_mean[0]*x_mean[1];
  S[0][2] -= p_mean[0]*x_mean[2];
  S[1][0] -= p_mean[1]*x_mean[0];
  S[1][1] -= p_mean[1]*x_mean[1];
  S[1][2] -= p_mean[1]*x_mean[2];
  S[2][0] -= p_mean[2]*x_mean[0];
  S[2][1] -= p_mean[2]*x_mean[1];
  S[2][2] -= p_mean[2]*x_mean[2];
  // calculate the 4x4 symmetric matrix Q
  double trace = S[0][0] + S[1][1] + S[2][2];
  double A23 = S[1][2] - S[2][1];
  double A31 = S[2][0] - S[0][2];
  double A12 = S[0][1] - S[1][0];

  Q[0][0] = trace;
  Q[0][1] = Q[1][0] = A23;
  Q[0][2] = Q[2][0] = A31;
  Q[0][3] = Q[3][0] = A12;
  for (i=0; i<3; i++)
    for (j=0; j<3; j++)
      Q[i+1][j+1] = S[i][j]+S[j][i]-(i==j ? trace : 0);

  maxEigenVector(Q, q);

  // calculate the rotation matrix
  double m[3][3]; // rot matrix
  quaternion2matrix(q, m);
  // calculate the translation vector, put it into q[4..6]
  q[4] = x_mean[0] - m[0][0]*p_mean[0] -
    m[0][1]*p_mean[1] - m[0][2]*p_mean[2];
  q[5] = x_mean[1] - m[1][0]*p_mean[0] -
    m[1][1]*p_mean[1] - m[1][2]*p_mean[2];
  q[6] = x_mean[2] - m[2][0]*p_mean[0] -
    m[2][1]*p_mean[1] - m[2][2]*p_mean[2];
}


/**************************************************/
static int
qudrtc(double b, double c, double rts[4])
/*
     solve the quadratic equation -

         x**2+b*x+c = 0

*/
{
  int nquad;
  double rtdis;
  double dis = b*b-4.0*c;

  if (dis >= 0.0) {
    nquad = 2;
    rtdis = sqrt(dis);
    if (b > 0.0) rts[0] = ( -b - rtdis)*.5;
    else         rts[0] = ( -b + rtdis)*.5;
    if (rts[0] == 0.0) rts[1] = -b;
    else               rts[1] = c/rts[0];
  } else {
    nquad = 0;
    rts[0] = 0.0;
    rts[1] = 0.0;
  }
  return nquad;
} /* qudrtc */
/**************************************************/

static double
cubic(double p, double q, double r)
/*
     find the lowest real root of the cubic -
       x**3 + p*x**2 + q*x + r = 0

   input parameters -
     p,q,r - coeffs of cubic equation.

   output-
     cubic - a real root.

   method -
     see D.E. Littlewood, "A University Algebra" pp.173 - 6

     Charles Prineas   April 1981

*/
{
  //int nrts;
  double po3,po3sq,qo3;
  double uo3,u2o3,uo3sq4,uo3cu4;
  double v,vsq,wsq;
  double m,mcube,n;
  double muo3,s,scube,t,cosk,sinsqk;
  double root;

  double doubmax = sqrt(DBL_MAX);

  m = 0.0;
  //nrts =0;
  if        ((p > doubmax) || (p <  -doubmax)) {
    root = -p;
  } else if ((q > doubmax) || (q <  -doubmax)) {
    if (q > 0.0) root = -r/q;
    else         root = -sqrt(-q);
  } else if ((r > doubmax)|| (r <  -doubmax)) {
    root =  -cbrt(r);
  } else {
    po3 = p/3.0;
    po3sq = po3*po3;
    if (po3sq > doubmax) root =  -p;
    else {
      v = r + po3*(po3sq + po3sq - q);
      if ((v > doubmax) || (v < -doubmax))
	root = -p;
      else {
	vsq = v*v;
	qo3 = q/3.0;
	uo3 = qo3 - po3sq;
	u2o3 = uo3 + uo3;
	if ((u2o3 > doubmax) || (u2o3 < -doubmax)) {
	  if (p == 0.0) {
	    if (q > 0.0) root =  -r/q;
	    else         root =  -sqrt(-q);
	  } else         root =  -q/p;
	}
	uo3sq4 = u2o3*u2o3;
	if (uo3sq4 > doubmax) {
	  if (p == 0.0) {
	    if (q > 0.0) root = -r/q;
	    else         root = -sqrt(fabs(q));
	  } else         root = -q/p;
	}
	uo3cu4 = uo3sq4*uo3;
	wsq = uo3cu4 + vsq;
	if (wsq >= 0.0) {
	  //
	  // cubic has one real root
	  //
	  //nrts = 1;
	  if (v <= 0.0) mcube = ( -v + sqrt(wsq))*.5;
	  if (v  > 0.0) mcube = ( -v - sqrt(wsq))*.5;
	  m = cbrt(mcube);
	  if (m != 0.0) n = -uo3/m;
	  else          n = 0.0;
	  root = m + n - po3;
	} else {
	  //nrts = 3;
	  //
	  // cubic has three real roots
	  //
	  if (uo3 < 0.0) {
	    muo3 = -uo3;
	    s = sqrt(muo3);
	    scube = s*muo3;
	    t =  -v/(scube+scube);
	    cosk = cos(acos(t)/3.0);
	    if (po3 < 0.0)
	      root = (s+s)*cosk - po3;
	    else {
	      sinsqk = 1.0 - cosk*cosk;
	      if (sinsqk < 0.0) sinsqk = 0.0;
	      root = s*( -cosk - sqrt(3*sinsqk)) - po3;
	    }
	  } else
	    //
	    // cubic has multiple root -
	    //
	    root = cbrt(v) - po3;
	}
      }
    }
  }
  return root;
} /* cubic */
/***************************************/


static int
ferrari(double a, double b, double c, double d,	double rts[4])
/*
     solve the quartic equation -

   x**4 + a*x**3 + b*x**2 + c*x + d = 0

     input -
   a,b,c,e - coeffs of equation.

     output -
   nquar - number of real roots.
   rts - array of root values.

     method :  Ferrari - Lagrange
     Theory of Equations, H.W. Turnbull p. 140 (1947)

     calls  cubic, qudrtc
*/
{
  int nquar,n1,n2;
  double asq,ainv2;
  double v1[4],v2[4];
  double p,q,r;
  double y;
  double e,f,esq,fsq,ef;
  double g,gg,h,hh;

  asq = a*a;

  p = b;
  q = a*c-4.0*d;
  r = (asq - 4.0*b)*d + c*c;
  y = cubic(p,q,r);

  esq = .25*asq - b - y;
  if (esq < 0.0) return(0);
  else {
    fsq = .25*y*y - d;
    if (fsq < 0.0) return(0);
    else {
      ef = -(.25*a*y + .5*c);
      if ( ((a > 0.0)&&(y > 0.0)&&(c > 0.0))
	   || ((a > 0.0)&&(y < 0.0)&&(c < 0.0))
	   || ((a < 0.0)&&(y > 0.0)&&(c < 0.0))
	   || ((a < 0.0)&&(y < 0.0)&&(c > 0.0))
	   ||  (a == 0.0)||(y == 0.0)||(c == 0.0)
	   ) {
	/* use ef - */
	if ((b < 0.0)&&(y < 0.0)&&(esq > 0.0)) {
	  e = sqrt(esq);
	  f = ef/e;
	} else if ((d < 0.0) && (fsq > 0.0)) {
	  f = sqrt(fsq);
	  e = ef/f;
	} else {
	  e = sqrt(esq);
	  f = sqrt(fsq);
	  if (ef < 0.0) f = -f;
	}
      } else {
	e = sqrt(esq);
	f = sqrt(fsq);
	if (ef < 0.0) f = -f;
      }
      /* note that e >= 0.0 */
      ainv2 = a*.5;
      g = ainv2 - e;
      gg = ainv2 + e;
      if ( ((b > 0.0)&&(y > 0.0))
	   || ((b < 0.0)&&(y < 0.0)) ) {
	if (( a > 0.0) && (e != 0.0)) g = (b + y)/gg;
	else if (e != 0.0) gg = (b + y)/g;
      }
      if ((y == 0.0)&&(f == 0.0)) {
	h = 0.0;
	hh = 0.0;
      } else if ( ((f > 0.0)&&(y < 0.0))
		  || ((f < 0.0)&&(y > 0.0)) ) {
	hh = -.5*y + f;
	h = d/hh;
      } else {
	h = -.5*y - f;
	hh = d/h;
      }
      n1 = qudrtc(gg,hh,v1);
      n2 = qudrtc(g,h,v2);
      nquar = n1+n2;
      rts[0] = v1[0];
      rts[1] = v1[1];
      rts[n1+0] = v2[0];
      rts[n1+1] = v2[1];
      return nquar;
    }
  }
} /* ferrari */


////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////        Chen & Medioni's method starts          ////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

static void
get_transform(double correction[6],
	      double m[3][3], double t[3])
{
  // produces a premult matrix: p' = M . p + t
  double sa = sin(correction[0]);
  double ca = sqrt(1-sa*sa);
  double sb = sin(correction[1]);
  double cb = sqrt(1-sb*sb);
  double sr = sin(correction[2]);
  double cr = sqrt(1-sr*sr);

  t[0] = correction[3];
  t[1] = correction[4];
  t[2] = correction[5];

  m[0][0] = cb*cr;
  m[0][1] = -cb*sr;
  m[0][2] = sb;

  m[1][0] = sa*sb*cr + ca*sr;
  m[1][1] = -sa*sb*sr + ca*cr;
  m[1][2] = -sa*cb;

  m[2][0] = -ca*sb*cr + sa*sr;
  m[2][1] = ca*sb*sr + sa*cr;
  m[2][2] = ca*cb;
}

// Solve x from Ax=b using Cholesky decomposition.
// This function changes the contents of A in the process.
static bool
cholesky_solve(double A[6][6], double b[6], double x[6])
{
  int i, j, k;
  double sum;

  for (i=0; i<6; i++) {

    for (sum=A[i][i], k=0; k<i; k++)
      sum -= A[i][k]*A[i][k];

      if (sum < 0.0) {
        cerr << "Cholesky: matrix not pos.semidef." << endl;
        SHOW(sum);
        return false;
      } else if (sum < 1.0e-7) {
        cerr << "Cholesky: matrix not pos.def." << endl;
        return false;
      } else {
        A[i][i] = sqrt(sum);
    }

    for (j=i+1; j<6; j++) {

      for (sum=A[i][j], k=0; k<i; k++)
        sum -= A[i][k]*A[j][k];

      A[j][i] = sum / A[i][i];
    }
  }

  for (i=0; i<6; i++) {               // Forward elimination;
    for (sum=b[i], j=0; j<i; j++)     // solve Ly=b, store y in x
      sum -= A[i][j]*x[j];
    x[i] = sum / A[i][i];
  }

  for (i=5; i>=0; i--) {              // Backward elimination;
    for (sum=x[i], j=i+1; j<6; j++)   // solve L'x = y
      sum -= A[j][i]*x[j];
    x[i] = sum / A[i][i];
  }

  return true;
}

// Calculate the rotation and translation that moves points in
// array ctr toward their pairs.
void
chen_medioni(Pnt3 *ctr,     // control points (source)
	     Pnt3 *srf,     // points on the tangent plane (target)
	     Pnt3 *nrm,     // the normals at the pairs
	     int n,         // how many pairs
	     double q[7])   // registration quaternion
{
  // The least-squares solutions for the translation and
  // rotation are found independently.
  // Therefore it is much better to first move the control points
  // around origin, and fix the result in the end.
  // cm = Sum{ctr[i]}/n
  // R(p-cm) + t + cm == Rp + { t + cm - Rcm }

  Pnt3 cm;
  for (int i=0; i<n; i++) cm += ctr[i];
  cm /= float(n);

  double HtH[6][6];
  double HtP[6], Hi[6];

  for (i=0; i<6; i++) {
    HtH[i][0] =
    HtH[i][1] =
    HtH[i][2] =
    HtH[i][3] =
    HtH[i][4] =
    HtH[i][5] =
    HtP[i] = 0;
  }

  double Pi;
  Pnt3 PxS;
  double sum = 0;
  for (i=0; i<n; i++) {
    Pi = dot(srf[i]-ctr[i], nrm[i]);
    PxS = cross(ctr[i]-cm, nrm[i]);
    Hi[0] = PxS[0];
    Hi[1] = PxS[1];
    Hi[2] = PxS[2];
    Hi[3] = nrm[i][0];
    Hi[4] = nrm[i][1];
    Hi[5] = nrm[i][2];
    HtP[0] += Pi * Hi[0];
    HtP[1] += Pi * Hi[1];
    HtP[2] += Pi * Hi[2];
    HtP[3] += Pi * Hi[3];
    HtP[4] += Pi * Hi[4];
    HtP[5] += Pi * Hi[5];
    HtH[0][0] += Hi[0]*Hi[0];
    HtH[0][1] += Hi[0]*Hi[1];
    HtH[0][2] += Hi[0]*Hi[2];
    HtH[0][3] += Hi[0]*Hi[3];
    HtH[0][4] += Hi[0]*Hi[4];
    HtH[0][5] += Hi[0]*Hi[5];
    HtH[1][1] += Hi[1]*Hi[1];
    HtH[1][2] += Hi[1]*Hi[2];
    HtH[1][3] += Hi[1]*Hi[3];
    HtH[1][4] += Hi[1]*Hi[4];
    HtH[1][5] += Hi[1]*Hi[5];
    HtH[2][2] += Hi[2]*Hi[2];
    HtH[2][3] += Hi[2]*Hi[3];
    HtH[2][4] += Hi[2]*Hi[4];
    HtH[2][5] += Hi[2]*Hi[5];
    HtH[3][3] += Hi[3]*Hi[3];
    HtH[3][4] += Hi[3]*Hi[4];
    HtH[3][5] += Hi[3]*Hi[5];
    HtH[4][4] += Hi[4]*Hi[4];
    HtH[4][5] += Hi[4]*Hi[5];
    HtH[5][5] += Hi[5]*Hi[5];
    sum += Pi*Pi;
  }

  cout << "Sqrt of average squared error before transform "
       << sqrtf(sum/n) << endl;

  // solve Ax=b using Cholesky decomposition
  double d[6];
  Xform<double> xf;
  if( cholesky_solve(HtH,HtP,d) ) {
    double m[3][3];
    double t[3];
    get_transform(d, m, t);

    // fix the translation, see comments above
    float *c = &cm[0];
    t[0] += c[0] - (m[0][0]*c[0]+m[0][1]*c[1]+m[0][2]*c[2]);
    t[1] += c[1] - (m[1][0]*c[0]+m[1][1]*c[1]+m[1][2]*c[2]);
    t[2] += c[2] - (m[2][0]*c[0]+m[2][1]*c[1]+m[2][2]*c[2]);

    // output as quaternion
    xf.fromRotTrans (m, t);
  } else {
    cerr << "Warning: Cholesky failed" << endl;
  }

  xf.toQuaternion (q);
  xf.getTranslation (q+4);
}

#ifdef TEST_ABSORIENT

#include "Pnt3.h"
#include "Random.h"
#include "Xform.h"

void
main(void)
{
  Random rand;

  Pnt3 x[1000];
  Pnt3 p[1000];
  double qd[7];

  // random points
  for (int i=0; i<1000; i++) {
    x[i] = Pnt3(rand(), rand(), rand())*200.0 - Pnt3(100,100,100);
  }

  // random rotation
  double axis[3] = {1,1,1};
  axis[0] = rand();
  axis[1] = rand();
  axis[2] = rand();
  Xform<double> xf;
  xf.rot(rand(), axis[0], axis[1], axis[2]);

  float sum = 0.0;
  for (i=0; i<1000; i++) {
    p[i] = x[i];
    xf(p[i]);
    sum += dist2(p[i],x[i]);
  }
  SHOW(sum);

  horn_align(&p[0], &x[0], 1000, qd);
  xf.fromQuaternion(qd, qd[4], qd[5], qd[6]);

  sum = 0.0;
  for (i=0; i<1000; i++) {
    xf(p[i]);
    sum += dist2(p[i],x[i]);
  }
  SHOW(sum);
}
#endif
