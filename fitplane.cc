
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include "Pnt3.h"

void SVD(float **, int, int, float *, float **);

void
fitPnt3Plane (const vector<Pnt3>& pts, Pnt3& n, float& d, Pnt3& centroid)
{
    int i, j;
    centroid = Pnt3 (0, 0, 0);
    int numPts = pts.size();

    float **a = new float *[numPts];
    for (i=0; i<numPts; ++i)
       a[i] = new float[3];
    float w[3];
    float *v[3];
    for (i=0; i<3; ++i)
       v[i] = new float[3];

    for (i = 0; i < numPts; i++) {
      centroid += pts[i];
    }
    centroid /= numPts;

    for (i = 0; i < numPts; i++) {
      for (j = 0; j < 3; j++) {
	a[i][j] = pts[i][j] - centroid[j];
      }
    }

    SVD(a, numPts, 3, w, v);

    // Copy vector corresponding to lowest singular value into n.
    // We assume that the result from SVD is sorted.

    for (j = 0; j < 3; j++)
      n[j] = v[j][2];

    d = dot (n, centroid);

    for (i=0; i<numPts; ++i) delete[] a[i]; delete[] a;
    for (i=0; i<3; ++i) delete[] v[i];
}


static double at,bt,ct;
#define HYPOT(a,b) ((at=fabs(a)) > (bt=fabs(b)) ? \
(ct=bt/at,at*sqrt(1.0+ct*ct)) : (bt ? (ct=at/bt,bt*sqrt(1.0+ct*ct)): 0.0))

//
// SVD -- This function computes the singular value decomposition
// of a matrix, A=UWV'.  The m x n input matrix A will have its contents
// modified; the diagonal matrix of singular values W is returned in the
// n-vector s (sorted in decreasing value), and the n x n matrix V (not
// the transpose!) is also returned.  The function only works for m >= n.
//
// This function is adapted from the JAMA::SVD class in the
// Template Numerical Toolkit (TNT) distributed by NIST, v1.1.1.
//

void SVD(float **A, int m, int n, float *s, float **V)
{
   if (m < n)  {
      fprintf(stderr, "ERROR: SVD only valid for input matrices ");
      fprintf(stderr, "with m >= n.\n");
      return;
   }

   int i, j, k;
   float *e = new float[n];
   float *work = new float[m];

   // Reduce A to bidiagonal form, storing the diagonal elements
   // in s and the super-diagonal elements in e.

   int nct = min(m-1,n);
   int nrt = max(0,min(n-2,m));
   for (k = 0; k < max(nct,nrt); k++) {
      if (k < nct) {

         // Compute the transformation for the k-th column and
         // place the k-th diagonal in s[k].
         // Compute 2-norm of k-th column without under/overflow.
         s[k] = 0;
         for (i = k; i < m; i++) {
            s[k] = HYPOT(s[k],A[i][k]);
         }
         if (s[k] != 0.0) {
            if (A[k][k] < 0.0) {
               s[k] = -s[k];
            }
            for (i = k; i < m; i++) {
               A[i][k] /= s[k];
            }
            A[k][k] += 1.0;
         }
         s[k] = -s[k];
      }
      for (j = k+1; j < n; j++) {
         if ((k < nct) && (s[k] != 0.0))  {

         // Apply the transformation.

            double t = 0;
            for (i = k; i < m; i++) {
               t += A[i][k]*A[i][j];
            }
            t = -t/A[k][k];
            for (i = k; i < m; i++) {
               A[i][j] += t*A[i][k];
            }
         }

         // Place the k-th row of A into e for the
         // subsequent calculation of the row transformation.

         e[j] = A[k][j];
      }
      if (k < nrt) {

         // Compute the k-th row transformation and place the
         // k-th super-diagonal in e[k].
         // Compute 2-norm without under/overflow.
         e[k] = 0;
         for (i = k+1; i < n; i++) {
            e[k] = HYPOT(e[k],e[i]);
         }
         if (e[k] != 0.0) {
            if (e[k+1] < 0.0) {
               e[k] = -e[k];
            }
            for (i = k+1; i < n; i++) {
               e[i] /= e[k];
            }
            e[k+1] += 1.0;
         }
         e[k] = -e[k];
         if ((k+1 < m) & (e[k] != 0.0)) {

         // Apply the transformation.

            for (i = k+1; i < m; i++) {
               work[i] = 0.0;
            }
            for (j = k+1; j < n; j++) {
               for (i = k+1; i < m; i++) {
                  work[i] += e[j]*A[i][j];
               }
            }
            for (j = k+1; j < n; j++) {
               double t = -e[j]/e[k+1];
               for (i = k+1; i < m; i++) {
                  A[i][j] += t*work[i];
               }
            }
         }

         // Place the transformation in V for subsequent
         // back multiplication.

         for (i = k+1; i < n; i++) {
            V[i][k] = e[i];
         }
      }
   }

   // Set up the final bidiagonal matrix or order p.

   int p = min(n,m+1);
   if (nct < n) {
      s[nct] = A[nct][nct];
   }
   if (m < p) {
      s[p-1] = 0.0;
   }
   if (nrt+1 < p) {
      e[nrt] = A[nrt][p-1];
   }
   e[p-1] = 0.0;

   // generate V

   for (k = n-1; k >= 0; k--) {
      if ((k < nrt) & (e[k] != 0.0)) {
         for (j = k+1; j < n; j++) {
            double t = 0;
            for (i = k+1; i < n; i++) {
               t += V[i][k]*V[i][j];
            }
            t = -t/V[k+1][k];
            for (i = k+1; i < n; i++) {
               V[i][j] += t*V[i][k];
            }
         }
      }
      for (i = 0; i < n; i++) {
         V[i][k] = 0.0;
      }
      V[k][k] = 1.0;
   }

   // Main iteration loop for the singular values.

   int pp = p-1;
   int iter = 0;
   double eps = pow(2.0,-52.0);
   while (p > 0) {
      int kase=0;
      k=0;

      // Here is where a test for too many iterations would go.

      // This section of the program inspects for
      // negligible elements in the s and e arrays.  On
      // completion the variables kase and k are set as follows.

      // kase = 1     if s(p) and e[k-1] are negligible and k<p
      // kase = 2     if s(k) is negligible and k<p
      // kase = 3     if e[k-1] is negligible, k<p, and
      //              s(k), ..., s(p) are not negligible (qr step).
      // kase = 4     if e(p-1) is negligible (convergence).

      for (k = p-2; k >= -1; k--) {
         if (k == -1) {
            break;
         }
         if (fabs(e[k]) <= eps*(fabs(s[k]) + fabs(s[k+1]))) {
            e[k] = 0.0;
            break;
         }
      }
      if (k == p-2) {
         kase = 4;
      } else {
         int ks;
         for (ks = p-1; ks >= k; ks--) {
            if (ks == k) {
               break;
            }
            double t = (ks != p ? fabs(e[ks]) : 0.) +
                       (ks != k+1 ? fabs(e[ks-1]) : 0.);
            if (fabs(s[ks]) <= eps*t)  {
               s[ks] = 0.0;
               break;
            }
         }
         if (ks == k) {
            kase = 3;
         } else if (ks == p-1) {
            kase = 1;
         } else {
            kase = 2;
            k = ks;
         }
      }
      k++;

      // Perform the task indicated by kase.

      switch (kase) {

         // Deflate negligible s(p).

         case 1: {
            double f = e[p-2];
            e[p-2] = 0.0;
            for (j = p-2; j >= k; j--) {
               double t = HYPOT(s[j],f);
               double cs = s[j]/t;
               double sn = f/t;
               s[j] = t;
               if (j != k) {
                  f = -sn*e[j-1];
                  e[j-1] = cs*e[j-1];
               }
               for (i = 0; i < n; i++) {
                  t = cs*V[i][j] + sn*V[i][p-1];
                  V[i][p-1] = -sn*V[i][j] + cs*V[i][p-1];
                  V[i][j] = t;
               }
            }
         }
         break;

         // Split at negligible s(k).

         case 2: {
            double f = e[k-1];
            e[k-1] = 0.0;
            for (j = k; j < p; j++) {
               double t = HYPOT(s[j],f);
               double cs = s[j]/t;
               double sn = f/t;
               s[j] = t;
               f = -sn*e[j];
               e[j] = cs*e[j];
            }
         }
         break;

         // Perform one qr step.

         case 3: {

            // Calculate the shift.

            double scale = max(max(max(max(
                    fabs(s[p-1]),fabs(s[p-2])),fabs(e[p-2])),
                    fabs(s[k])),fabs(e[k]));
            double sp = s[p-1]/scale;
            double spm1 = s[p-2]/scale;
            double epm1 = e[p-2]/scale;
            double sk = s[k]/scale;
            double ek = e[k]/scale;
            double b = ((spm1 + sp)*(spm1 - sp) + epm1*epm1)/2.0;
            double c = (sp*epm1)*(sp*epm1);
            double shift = 0.0;
            if ((b != 0.0) | (c != 0.0)) {
               shift = sqrt(b*b + c);
               if (b < 0.0) {
                  shift = -shift;
               }
               shift = c/(b + shift);
            }
            double f = (sk + sp)*(sk - sp) + shift;
            double g = sk*ek;

            // Chase zeros.

            for (j = k; j < p-1; j++) {
               double t = HYPOT(f,g);
               double cs = f/t;
               double sn = g/t;
               if (j != k) {
                  e[j-1] = t;
               }
               f = cs*s[j] + sn*e[j];
               e[j] = cs*e[j] - sn*s[j];
               g = sn*s[j+1];
               s[j+1] = cs*s[j+1];
               for (i = 0; i < n; i++) {
                  t = cs*V[i][j] + sn*V[i][j+1];
                  V[i][j+1] = -sn*V[i][j] + cs*V[i][j+1];
                  V[i][j] = t;
               }
               t = HYPOT(f,g);
               cs = f/t;
               sn = g/t;
               s[j] = t;
               f = cs*e[j] + sn*s[j+1];
               s[j+1] = -sn*e[j] + cs*s[j+1];
               g = sn*e[j+1];
               e[j+1] = cs*e[j+1];
            }
            e[p-2] = f;
            iter = iter + 1;
         }
         break;

         // Convergence.

         case 4: {

            // Make the singular values positive.

            if (s[k] <= 0.0) {
               s[k] = (s[k] < 0.0 ? -s[k] : 0.0);
               for (i = 0; i <= pp; i++) {
                  V[i][k] = -V[i][k];
               }
            }

            // Order the singular values.

            while (k < pp) {
               if (s[k] >= s[k+1]) {
                  break;
               }
               double t = s[k];
               s[k] = s[k+1];
               s[k+1] = t;
               if (k < n-1) {
                  for (i = 0; i < n; i++) {
                     t = V[i][k+1]; V[i][k+1] = V[i][k]; V[i][k] = t;
                  }
               }
               k++;
            }
            iter = 0;
            p--;
         }
         break;
      }
   }

   // Free up allocated memory

   delete[] e;
   delete[] work;
}

#undef HYPOT


