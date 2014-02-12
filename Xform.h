////////////////////////////////////////////////////////////
// Xform.h
//
// A class for doing 3D transformations using 4x4 matrices
////////////////////////////////////////////////////////////

#ifndef _xform_h_
#define _xform_h_

#include <assert.h>
#include <math.h>
#include <float.h>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include "Pnt3.h"


#ifdef __GNUC__
// G++ implements the standard for templated friend functions of template
// classes.  This means that the following forward declarations are necessary..

template <class T>
class Xform;

template <class T>
Xform<T> operator*(const Xform<T> &a, const Xform<T> &b);

template <class T>
Xform<T> delta(Xform<T> a, const Xform<T> &b);

template <class T>
Xform<T> linear_interp(const Xform<T> &a,
                       const Xform<T> &b,
                       double          t);

template <class T>
Xform<T> bilinear_interp(const Xform<T> &u0v0,
                         const Xform<T> &u1v0,
                         const Xform<T> &u0v1,
                         const Xform<T> &u1v1,
                         double u, double v);

template <class T>
Xform<T> relative_xform(const Xform<T> &a, Xform<T> b);

template <class T>
ostream& operator<<(ostream &out, const Xform<T> &xf);

template <class T>
istream& operator>>(istream & in, Xform<T> &xf);

#endif


// logically Xform is applied to a point: p' = M*p
// however, the transpose of the M is stored
// to keep it compatible with OpenGL

template <class T> // use only with float or double
class Xform {
private:
  T m[4][4];         // column major order

public:
  Xform(void);
  Xform(const double *a);  // from OpenGL matrix
  Xform(const float  *a);  // from OpenGL matrix
  Xform(const T r[3][3], const T t[3]);
  Xform<T> &operator=(const double *a);
  Xform<T> &operator=(const float *a);
  Xform<T> &operator=(const Xform<T> &xf);

  Xform<T> &fromQuaternion(const double q[4], // q == {w,x,y,z}
			   double tx = 0.0,
			   double ty = 0.0,
			   double tz = 0.0);
  Xform<T> &fromQuaternion(const float q[4],  // q == {w,x,y,z}
			   float tx = 0.0,
			   float ty = 0.0,
			   float tz = 0.0);
  void toQuaternion(double q[4]) const;
  void toQuaternion(float q[4]) const;
  Xform<T> &addQuaternion(T q0, T q1, T q2, T q3,
			  T tx = 0.0, T ty = 0.0, T tz = 0.0);
  Xform<T> &addQuaternion(const float  *q, int n = 4);  // n == 4,7
  Xform<T> &addQuaternion(const double *q, int n = 4);  // n == 4,7

  void fromEuler(const T ea[3], int order);
  void toEuler(T ea[3], int order);

  Xform<T> &fromFrame(const Pnt3 &x,
		      const Pnt3 &y,
		      const Pnt3 &z,
		      const Pnt3 &org);
  Xform<T> &fromRotTrans(const T r[3][3], const T t[3]);

  bool isIdentity (void) const;
  Xform<T> &identity(void);             // set to identity
  Xform<T> &fast_invert(void);          // assume rot + trans
  Xform<T> &invert(void);               // general inversion

  void translate(const double t[3]);    // M <- T*M
  void translate(const float  t[3]);    // M <- T*M
  void translate(T x, T y, T z);        // M <- T*M
  void getTranslation(double t[3]) const;
  void getTranslation(float  t[3]) const;

  void removeTranslation(void);
  void removeTranslation(double t[3]);
  void removeTranslation(float t[3]);

  void scale(const double t[3]);    // M <- S*M
  void scale(const float  t[3]);    // M <- S*M
  void scale(T x, T y, T z);        // M <- S*M

  void get_e(int i, T e[3]) const; // get a basis vector, i=0,1,2

  Xform<T> &rotX(T a);                  // M <- R*M
  Xform<T> &rotY(T a);                  // M <- R*M
  Xform<T> &rotZ(T a);                  // M <- R*M

  Xform<T> &rot(T angle, T ax, T ay, T az); // M <- R*M
  Xform<T> &rot(T angle, T ax, T ay, T az,
		T ox, T oy, T oz); // M <- R*M
  Xform<T> &rotQ(T q0, T q1, T q2, T q3);   // M <- R*M
  void      get_rot(T &angle, T &ax, T &ay, T &az);

  void apply(const double i[3], double o[3]) const; // o = M*i
  void apply(const float i[3],  float o[3]) const;  // o = M*i
  void apply_inv(const double i[3], double o[3]) const; // o = M^-1*i
  void apply_inv(const float i[3],  float o[3]) const;  // o = M^-1*i
  void operator()(double i[3]) const;   // i <- M*i
  void operator()(float  i[3]) const;   // i <- M*i

  // apply to normal vector (no translation, only rotation)
  void apply_nrm(const double i[3], double o[3]) const; // o = M*i
  void apply_nrm(const float i[3],  float o[3]) const;  // o = M*i

  T&   operator()(int i, int j);        // indexed access
  T    operator()(int i, int j) const;  // indexed access

  operator const T *(void) const { return m[0]; }  // array access
  operator       T *(void)       { return m[0]; }

  Pnt3 unproject(float u, float v, float z) const; // all input [-1,1]
  Pnt3 unproject_fast(float u, float v, float z) const;

  Xform<T> &operator*=(const Xform<T> &a); // (*this) = a * (*this)

  // test how close to a rigid transformation the current
  // transformation is, ideally returns 0
  float test_rigidity(void);
  // enforce the rigidity of the xform by setting the projective
  // part to 0 0 0 1 and making sure rotation part is just a rotation
  Xform<T> &enforce_rigidity(void);

#ifndef __GNUC__
  friend Xform<T> operator*(const Xform<T> &a,     // a*b
			    const Xform<T> &b);
  friend Xform<T> delta(Xform<T> a,                // d * a = b
			const Xform<T> &b);
  friend Xform<T> linear_interp(const Xform<T> &a, // r=t*a+(1-t)*b
				const Xform<T> &b,
				double          t);
  friend Xform<T> bilinear_interp(const Xform<T> &u0v0,
				  const Xform<T> &u1v0,
				  const Xform<T> &u0v1,
				  const Xform<T> &u1v1,
				  double u, double v);
  friend Xform<T> relative_xform(const Xform<T> &a,
				 Xform<T> b); // c = b^-1 * a

  friend ostream& operator<<(ostream &out, const Xform<T> &xf);
  friend istream& operator>>(istream & in,       Xform<T> &xf);
#else
  friend Xform<T> operator*<>(const Xform<T> &a,     // a*b
			      const Xform<T> &b);
  friend Xform<T> delta<>(Xform<T> a,                // d * a = b
			  const Xform<T> &b);
  friend Xform<T> linear_interp<>(const Xform<T> &a, // r=t*a+(1-t)*b
				  const Xform<T> &b,
				  double          t);
  friend Xform<T> bilinear_interp<>(const Xform<T> &u0v0,
				    const Xform<T> &u1v0,
				    const Xform<T> &u0v1,
				    const Xform<T> &u1v1,
				    double u, double v);
  friend Xform<T> relative_xform<>(const Xform<T> &a,
				   Xform<T> b); // c = b^-1 * a

  friend ostream& operator<< <> (ostream &out, const Xform<T> &xf);
  friend istream& operator>> <> (istream & in,       Xform<T> &xf);
#endif
};



template <class T> inline
Xform<T>::Xform(void)
{
  m[1][0] = m[2][0] = m[3][0] = m[0][1] = m[2][1] = m[3][1] = 0.0;
  m[0][2] = m[1][2] = m[3][2] = m[0][3] = m[1][3] = m[2][3] = 0.0;
  m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0;
}

template <class T> inline
Xform<T>::Xform(const double *a)
{
  m[0][0] = a[0]; m[0][1] = a[1]; m[0][2] = a[2]; m[0][3] = a[3];
  m[1][0] = a[4]; m[1][1] = a[5]; m[1][2] = a[6]; m[1][3] = a[7];
  m[2][0] = a[8]; m[2][1] = a[9]; m[2][2] = a[10];m[2][3] = a[11];
  m[3][0] = a[12];m[3][1] = a[13];m[3][2] = a[14];m[3][3] = a[15];
}

template <class T> inline
Xform<T>::Xform(const float *a)
{
  m[0][0] = a[0]; m[0][1] = a[1]; m[0][2] = a[2]; m[0][3] = a[3];
  m[1][0] = a[4]; m[1][1] = a[5]; m[1][2] = a[6]; m[1][3] = a[7];
  m[2][0] = a[8]; m[2][1] = a[9]; m[2][2] = a[10];m[2][3] = a[11];
  m[3][0] = a[12];m[3][1] = a[13];m[3][2] = a[14];m[3][3] = a[15];
}

template <class T> inline
Xform<T>::Xform(const T r[3][3], const T t[3])
{
  m[0][0] = r[0][0]; m[0][1]=r[1][0]; m[0][2]=r[2][0]; m[0][3]=0.0;
  m[1][0] = r[0][1]; m[1][1]=r[1][1]; m[1][2]=r[2][1]; m[1][3]=0.0;
  m[2][0] = r[0][2]; m[2][1]=r[1][2]; m[2][2]=r[2][2]; m[2][3]=0.0;
  m[3][0] = t[0]; m[3][1] = t[1]; m[3][2] = t[2]; m[3][3] = 1.0;
}


template <class T> inline Xform<T> &
Xform<T>::operator=(const double *a)
{
  m[0][0] = a[0]; m[0][1] = a[1]; m[0][2] = a[2]; m[0][3] = a[3];
  m[1][0] = a[4]; m[1][1] = a[5]; m[1][2] = a[6]; m[1][3] = a[7];
  m[2][0] = a[8]; m[2][1] = a[9]; m[2][2] = a[10];m[2][3] = a[11];
  m[3][0] = a[12];m[3][1] = a[13];m[3][2] = a[14];m[3][3] = a[15];
  return *this;
}

template <class T> inline Xform<T> &
Xform<T>::operator=(const float *a)
{
  m[0][0] = a[0]; m[0][1] = a[1]; m[0][2] = a[2]; m[0][3] = a[3];
  m[1][0] = a[4]; m[1][1] = a[5]; m[1][2] = a[6]; m[1][3] = a[7];
  m[2][0] = a[8]; m[2][1] = a[9]; m[2][2] = a[10];m[2][3] = a[11];
  m[3][0] = a[12];m[3][1] = a[13];m[3][2] = a[14];m[3][3] = a[15];
  return *this;
}

template <class T> inline Xform<T> &
Xform<T>::operator=(const Xform<T> &xf)
{
  return operator=((const T *) xf);
}

// see Graphics Gems III p. 465
// note quaternion order: w,x,y,z
template <class T> inline Xform<T> &
Xform<T>::fromQuaternion(const double q[4],
			 double tx, double ty, double tz)
{
  // q doesn't have to be unit
  T norm2 = q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3];
  T s = (norm2 > 0.0) ? 2.0/norm2 : 0.0;
  T xs = q[1]*s,   ys = q[2]*s,   zs = q[3]*s;
  T wx = q[0]*xs,  wy = q[0]*ys,  wz = q[0]*zs;
  T xx = q[1]*xs,  xy = q[1]*ys,  xz = q[1]*zs;
  T yy = q[2]*ys,  yz = q[2]*zs,  zz = q[3]*zs;

  m[0][0] = 1.0 - (yy+zz);  m[0][1] = xy + wz;  m[0][2] = xz - wy;
  m[1][0] = xy - wz;  m[1][1] = 1.0 - (xx+zz);  m[1][2] = yz + wx;
  m[2][0] = xz + wy;  m[2][1] = yz - wx;  m[2][2] = 1.0 - (xx+yy);

  m[3][0] = tx;  m[3][1] = ty;  m[3][2] = tz;
  m[0][3] = m[1][3] = m[2][3] = 0.0;
  m[3][3] = 1.0;

  return *this;
}

template <class T> inline Xform<T> &
Xform<T>::fromQuaternion(const float q[4],
			 float tx, float ty, float tz)
{
  // q doesn't have to be unit
  T norm2 = q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3];
  T s = (norm2 > 0.0) ? 2.0/norm2 : 0.0;
  T xs = q[1]*s,   ys = q[2]*s,   zs = q[3]*s;
  T wx = q[0]*xs,  wy = q[0]*ys,  wz = q[0]*zs;
  T xx = q[1]*xs,  xy = q[1]*ys,  xz = q[1]*zs;
  T yy = q[2]*ys,  yz = q[2]*zs,  zz = q[3]*zs;

  m[0][0] = 1.0 - (yy+zz);  m[0][1] = xy + wz;  m[0][2] = xz - wy;
  m[1][0] = xy - wz;  m[1][1] = 1.0 - (xx+zz);  m[1][2] = yz + wx;
  m[2][0] = xz + wy;  m[2][1] = yz - wx;  m[2][2] = 1.0 - (xx+yy);

  m[3][0] = tx;  m[3][1] = ty;  m[3][2] = tz;
  m[0][3] = m[1][3] = m[2][3] = 0.0;
  m[3][3] = 1.0;

  return *this;
}

// From Graphics Gems IV p. 213
// note transposed matrix and quaternion order w,x,y,z
// see also Graphics Gems II pp. 351 - 354
// Ignore translation and projective parts.
// This algorithm avoids near-zero divides by looking for a
// large component - first w, then x, y, or z.
// When the trace is greater than zero, |w| is greater than 1/2,
// which is as small as a largest component can be.
// Otherwise, the largest diagonal entry corresponds to the
// largest of |x|, |y|, or |z|, one of which must be larger
// than |w|, and at least 1/2.
template <class T> inline void
Xform<T>::toQuaternion(double q[4]) const
{
  T tr = m[0][0] + m[1][1]+ m[2][2];
  if (tr >= 0.0) {
    T s = sqrt(tr + m[3][3]);
    q[0] = s*0.5;
    s = 0.5 / s;
    q[1] = (m[1][2] - m[2][1]) * s;
    q[2] = (m[2][0] - m[0][2]) * s;
    q[3] = (m[0][1] - m[1][0]) * s;
  } else {
    int i = 0;
    if (m[1][1] > m[0][0]) i = 1;
    if (m[2][2] > m[i][i]) i = 2;
    int j = (i+1)%3;
    int k = (j+1)%3;
    T s = sqrt(m[3][3] + (m[i][i] - (m[j][j] + m[k][k])));
    q[i+1] = s * 0.5;
    s = 0.5 / s;
    q[j+1] = (m[i][j] + m[j][i]) * s;
    q[k+1] = (m[i][k] + m[k][i]) * s;
    q[0]   = (m[j][k] - m[k][j]) * s;
  }
  if (m[3][3] != 1.0) {
    T tmp = 1.0/sqrt(m[3][3]);
    q[0]*=tmp; q[1]*=tmp; q[2]*=tmp; q[3]*=tmp;
  }
}

template <class T> inline void
Xform<T>::toQuaternion(float q[4]) const
{
  T tr = m[0][0] + m[1][1]+ m[2][2];
  if (tr >= 0.0) {
    T s = sqrt(tr + m[3][3]);
    q[0] = s*0.5;
    s = 0.5 / s;
    q[1] = (m[1][2] - m[2][1]) * s;
    q[2] = (m[2][0] - m[0][2]) * s;
    q[3] = (m[0][1] - m[1][0]) * s;
  } else {
    int i = 0;
    if (m[1][1] > m[0][0]) i = 1;
    if (m[2][2] > m[i][i]) i = 2;
    int j = (i+1)%3;
    int k = (j+1)%3;
    T s = sqrt(m[3][3] + (m[i][i] - (m[j][j] + m[k][k])));
    q[i+1] = s * 0.5;
    s = 0.5 / s;
    q[j+1] = (m[i][j] + m[j][i]) * s;
    q[k+1] = (m[i][k] + m[k][i]) * s;
    q[0]   = (m[j][k] - m[k][j]) * s;
  }
  if (m[3][3] != 1.0) {
    T tmp = 1.0/sqrt(m[3][3]);
    q[0]*=tmp; q[1]*=tmp; q[2]*=tmp; q[3]*=tmp;
  }
}

template <class T> inline Xform<T> &
Xform<T>::addQuaternion(T q0, T q1, T q2, T q3, T tx, T ty, T tz)
{
  rotQ(q0,q1,q2,q3);
  translate(tx,ty,tz);
  return *this;
}

template <class T> inline Xform<T> &
Xform<T>::addQuaternion(const float *q, int n)
{
  rotQ(q[0],q[1],q[2],q[3]);
  if (n == 7)
    translate(q[4],q[5],q[6]);
  return *this;
}

template <class T> inline Xform<T> &
Xform<T>::addQuaternion(const double *q, int n)
{
  rotQ(q[0],q[1],q[2],q[3]);
  if (n == 7)
    translate(q[4],q[5],q[6]);
  return *this;
}

//
// Euler angle stuff from Graphics Gems IV pp. 222-229
//
/*** Order type constants, constructors, extractors ***/
    /* There are 24 possible conventions, designated by:    */
    /*	  o EulAxI = axis used initially		    */
    /*	  o EulPar = parity of axis permutation		    */
    /*	  o EulRep = repetition of initial axis as last	    */
    /*	  o EulFrm = frame from which axes are taken	    */
    /* Axes I,J,K will be a permutation of X,Y,Z.	    */
    /* Axis H will be either I or K, depending on EulRep.   */
    /* Frame S takes axes from initial static frame.	    */
    /* If ord = (AxI=X, Par=Even, Rep=No, Frm=S), then	    */
    /* {a,b,c,ord} means Rz(c)Ry(b)Rx(a), where Rz(c)v	    */
    /* rotates v around Z by c radians.			    */
#define EulFrmS	     0
#define EulFrmR	     1
#define EulFrm(ord)  ((unsigned)(ord)&1)
#define EulRepNo     0
#define EulRepYes    1
#define EulRep(ord)  (((unsigned)(ord)>>1)&1)
#define EulParEven   0
#define EulParOdd    1
#define EulPar(ord)  (((unsigned)(ord)>>2)&1)
#define EulSafe	     "\000\001\002\000"
#define EulNext	     "\001\002\000\001"
#define EulAxI(ord)  ((int)(EulSafe[(((unsigned)(ord)>>3)&3)]))
#define EulAxJ(ord)  ((int)(EulNext[EulAxI(ord)+(EulPar(ord)==EulParOdd)]))
#define EulAxK(ord)  ((int)(EulNext[EulAxI(ord)+(EulPar(ord)!=EulParOdd)]))
#define EulAxH(ord)  ((EulRep(ord)==EulRepNo)?EulAxK(ord):EulAxI(ord))
// EulGetOrd unpacks all useful information about order
// simultaneously.
#define EulGetOrd(ord,i,j,k,n,s,f) {\
  unsigned o=ord;f=o&1;o>>=1;s=o&1;o>>=1;\
  n=o&1;o>>=1;i=EulSafe[o&3];j=EulNext[i+n];\
  k=EulNext[i+1-n];}
// EulOrd creates an order value between 0 and 23 from 4-tuple
// choices.
#define EulOrd(i,p,r,f)	   (((((((i)<<1)+(p))<<1)+(r))<<1)+(f))
// Static axes
#define EulOrdXYZs    EulOrd(0,EulParEven,EulRepNo,EulFrmS)
#define EulOrdXYXs    EulOrd(0,EulParEven,EulRepYes,EulFrmS)
#define EulOrdXZYs    EulOrd(0,EulParOdd,EulRepNo,EulFrmS)
#define EulOrdXZXs    EulOrd(0,EulParOdd,EulRepYes,EulFrmS)
#define EulOrdYZXs    EulOrd(1,EulParEven,EulRepNo,EulFrmS)
#define EulOrdYZYs    EulOrd(1,EulParEven,EulRepYes,EulFrmS)
#define EulOrdYXZs    EulOrd(1,EulParOdd,EulRepNo,EulFrmS)
#define EulOrdYXYs    EulOrd(1,EulParOdd,EulRepYes,EulFrmS)
#define EulOrdZXYs    EulOrd(2,EulParEven,EulRepNo,EulFrmS)
#define EulOrdZXZs    EulOrd(2,EulParEven,EulRepYes,EulFrmS)
#define EulOrdZYXs    EulOrd(2,EulParOdd,EulRepNo,EulFrmS)
#define EulOrdZYZs    EulOrd(2,EulParOdd,EulRepYes,EulFrmS)
// Rotating axes
#define EulOrdZYXr    EulOrd(0,EulParEven,EulRepNo,EulFrmR)
#define EulOrdXYXr    EulOrd(0,EulParEven,EulRepYes,EulFrmR)
#define EulOrdYZXr    EulOrd(0,EulParOdd,EulRepNo,EulFrmR)
#define EulOrdXZXr    EulOrd(0,EulParOdd,EulRepYes,EulFrmR)
#define EulOrdXZYr    EulOrd(1,EulParEven,EulRepNo,EulFrmR)
#define EulOrdYZYr    EulOrd(1,EulParEven,EulRepYes,EulFrmR)
#define EulOrdZXYr    EulOrd(1,EulParOdd,EulRepNo,EulFrmR)
#define EulOrdYXYr    EulOrd(1,EulParOdd,EulRepYes,EulFrmR)
#define EulOrdYXZr    EulOrd(2,EulParEven,EulRepNo,EulFrmR)
#define EulOrdZXZr    EulOrd(2,EulParEven,EulRepYes,EulFrmR)
#define EulOrdXYZr    EulOrd(2,EulParOdd,EulRepNo,EulFrmR)
#define EulOrdZYZr    EulOrd(2,EulParOdd,EulRepYes,EulFrmR)

template <class T> inline void
Xform<T>::fromEuler(const T _ea[3], int order)
{
  T ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
  int i,j,k,n,s,f;
  EulGetOrd(order,i,j,k,n,s,f);
  T ea[3];
  if (f==EulFrmR) { ea[0]=_ea[2]; ea[1]=_ea[1]; ea[2]=_ea[0]; }
  else            { ea[0]=_ea[0]; ea[1]=_ea[1]; ea[2]=_ea[2]; }
  if (n==EulParOdd) {ea[0] = -ea[0]; ea[1] =-ea[1]; ea[2] =-ea[2];}
  ti = ea[0];   tj = ea[1];   th = ea[2];
  ci = cos(ti); cj = cos(tj); ch = cos(th);
  si = sin(ti); sj = sin(tj); sh = sin(th);
  cc = ci*ch; cs = ci*sh; sc = si*ch; ss = si*sh;
  if (s==EulRepYes) {
    m[i][i] = cj;     m[j][i] =  sj*si;    m[k][i] =  sj*ci;
    m[i][j] = sj*sh;  m[j][j] = -cj*ss+cc; m[k][j] = -cj*cs-sc;
    m[i][k] = -sj*ch; m[j][k] =  cj*sc+cs; m[k][k] =  cj*cc-ss;
  } else {
    m[i][i] = cj*ch; m[j][i] = sj*sc-cs; m[k][i] = sj*cc+ss;
    m[i][j] = cj*sh; m[j][j] = sj*ss+cc; m[k][j] = sj*cs-sc;
    m[i][k] = -sj;   m[j][k] = cj*si;    m[k][k] = cj*ci;
  }
  m[3][0]=m[3][1]=m[3][2]=m[0][3]=m[1][3]=m[2][3]=0.0; m[3][3]=1.0;
}

template <class T> inline void
Xform<T>::toEuler(T ea[3], int order)
{
  int i,j,k,n,s,f;
  EulGetOrd(order,i,j,k,n,s,f);
  if (s==EulRepYes) {
    T sy = sqrt(m[j][i]*m[j][i] + m[k][i]*m[k][i]);
    if (sy > 16*FLT_EPSILON) {
      ea[0] = atan2(m[j][i], m[i][k]);
      ea[1] = atan2(sy, m[i][i]);
      ea[2] = atan2(m[i][j], -m[i][k]);
    } else {
      ea[0] = atan2(-m[k][j], m[j][j]);
      ea[1] = atan2(sy, m[i][i]);
      ea[2] = 0;
    }
  } else {
    T cy = sqrt(m[i][i]*m[i][i] + m[i][j]*m[i][j]);
    if (cy > 16*FLT_EPSILON) {
      ea[0] = atan2(m[j][k], m[k][k]);
      ea[1] = atan2(-m[i][k], cy);
      ea[2] = atan2(m[i][j], m[i][i]);
    } else {
      ea[0] = atan2(-m[k][j], m[j][j]);
      ea[1] = atan2(-m[i][k], cy);
      ea[2] = 0;
    }
  }
  if (n==EulParOdd) {ea[0] = -ea[0]; ea[1] = -ea[1]; ea[2]=-ea[2];}
  if (f==EulFrmR)   {T t = ea[0]; ea[0] = ea[2]; ea[2] = t;}
}


template <class T> inline Xform<T> &
Xform<T>::fromFrame(const Pnt3 &x,
		    const Pnt3 &y,
		    const Pnt3 &z,
		    const Pnt3 &org)
{
  m[0][0] = x[0]; m[0][1] = x[1]; m[0][2] = x[2]; m[0][3] = 0.0;
  m[1][0] = y[0]; m[1][1] = y[1]; m[1][2] = y[2]; m[1][3] = 0.0;
  m[2][0] = z[0]; m[2][1] = z[1]; m[2][2] = z[2]; m[2][3] = 0.0;
  m[3][0] = org[0]; m[3][1] = org[1]; m[3][2] = org[2]; m[3][3] = 1.0;
  return *this;
}

template <class T> inline Xform<T> &
Xform<T>::fromRotTrans(const T r[3][3], const T t[3])
{
  m[0][0] = r[0][0]; m[0][1]=r[1][0]; m[0][2]=r[2][0]; m[0][3]=0.0;
  m[1][0] = r[0][1]; m[1][1]=r[1][1]; m[1][2]=r[2][1]; m[1][3]=0.0;
  m[2][0] = r[0][2]; m[2][1]=r[1][2]; m[2][2]=r[2][2]; m[2][3]=0.0;
  m[3][0] = t[0]; m[3][1] = t[1]; m[3][2] = t[2]; m[3][3] = 1.0;
  return *this;
}


template <class T> inline bool
Xform<T>::isIdentity (void) const
{
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      if (m[i][j] != (float)(i == j))
	return false;

  return true;
}


template <class T> inline Xform<T> &
Xform<T>::identity(void)
{
  m[1][0] = m[2][0] = m[3][0] = m[0][1] = m[2][1] = m[3][1] = 0.0;
  m[0][2] = m[1][2] = m[3][2] = m[0][3] = m[1][3] = m[2][3] = 0.0;
  m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0;
  return *this;
}


template <class T> inline Xform<T> &
Xform<T>::fast_invert(void)
{
  T tmp;
  // transpose the rotation part
  tmp = m[0][1]; m[0][1] = m[1][0]; m[1][0] = tmp;
  tmp = m[0][2]; m[0][2] = m[2][0]; m[2][0] = tmp;
  tmp = m[1][2]; m[1][2] = m[2][1]; m[2][1] = tmp;
  // translation (T' = - T * R^-1)
  T tx = -m[3][0];
  T ty = -m[3][1];
  T tz = -m[3][2];
  m[3][0] = tx * m[0][0] + ty * m[1][0] + tz * m[2][0];
  m[3][1] = tx * m[0][1] + ty * m[1][1] + tz * m[2][1];
  m[3][2] = tx * m[0][2] + ty * m[1][2] + tz * m[2][2];
  return *this;
}


template <class T> inline Xform<T> &
Xform<T>::invert(void)
{
   // Compute inverse matrix in place using Gauss-Jordan elimination
   // with full pivoting for better numerical stability.

   int swap_col[4], swap_row[4];
   int pivot_used[4] = {0, 0, 0, 0};
   int maxc, maxr, i, r, c;
   double max, temp, pivot_inv;

   for (int i=0; i<4; i++) {

      // Determine largest magnitude pivot element

      max = 0.0;
      for (r=0; r<4; r++) {
         if (pivot_used[r] != 1) {
	    for (c=0; c<4; c++) {
	       if (pivot_used[c] == 0) {
	          if (fabs(m[r][c]) >= max) {
	             max = fabs(m[r][c]);
	             maxr = r;
	             maxc = c;
	          }
	       }
               else if (pivot_used[c] > 1) {
	          cerr << "invert: Singular Matrix-1" << endl;
	          return *this;
	       }
            }
	 }
      }

      pivot_used[maxc] += 1;

      // Swap rows to get the pivot element on the diagonal

      if (maxr != maxc)
         for (c=0; c<4; c++)
            swap(m[maxr][c], m[maxc][c]);

      // We need to remember what we swapped, for reversing later...

      swap_row[i] = maxr;
      swap_col[i] = maxc;

      // If the pivot value is zero, matrix is singular

      if (m[maxc][maxc] == 0.0) {
         cerr << "invert: Singular Matrix-2" << endl;
         return *this;
      }

      // Divide the row by the pivot value

      pivot_inv = 1.0 / m[maxc][maxc];
      m[maxc][maxc] = 1.0;
      for (c=0; c<4; c++)
         m[maxc][c] *= pivot_inv;

      // Reduce the other, non-pivot rows

      for (r=0; r<4; r++)  {
         if (r != maxc) {
	    temp = m[r][maxc];
	    m[r][maxc] = 0.0;
	    for (c=0; c<4; c++)
               m[r][c] -= m[maxc][c] * temp;
         }
      }
   }

   // Fix things up by swapping columns back in reverse order

   for (int i=3; i>=0; i--) {
      if (swap_row[i] != swap_col[i])
         for (r=0; r<4; r++)
            swap(m[r][swap_row[i]], m[r][swap_col[i]]);
   }

   return *this;
}


template <class T> inline void
Xform<T>::translate(const double t[3]) // M <- T*M
{
  for (int i=0; i<4; i++) m[i][0] += t[0]*m[i][3];
  for (int i=0; i<4; i++)     m[i][1] += t[1]*m[i][3];
  for (int i=0; i<4; i++)     m[i][2] += t[2]*m[i][3];
}

template <class T> inline void
Xform<T>::translate(const float t[3]) // M <- T*M
{
  for (int i=0; i<4; i++) m[i][0] += t[0]*m[i][3];
  for (int i=0; i<4; i++)     m[i][1] += t[1]*m[i][3];
  for (int i=0; i<4; i++)     m[i][2] += t[2]*m[i][3];
}

template <class T> inline void
Xform<T>::translate(T x, T y, T z) // M <- T*M
{
  for (int i=0; i<4; i++) m[i][0] += x*m[i][3];
  for (int i=0; i<4; i++)     m[i][1] += y*m[i][3];
  for (int i=0; i<4; i++)     m[i][2] += z*m[i][3];
}

template <class T> inline void
Xform<T>::getTranslation(double t[3]) const
{
  t[0] = m[3][0];  t[1] = m[3][1];  t[2] = m[3][2];
}

template <class T> inline void
Xform<T>::getTranslation(float t[3]) const
{
  t[0] = m[3][0];  t[1] = m[3][1];  t[2] = m[3][2];
}

template <class T> inline void
Xform<T>::removeTranslation(void)
{
  m[3][0] = m[3][1] = m[3][2] = 0.0;
}

template <class T> inline void
Xform<T>::removeTranslation(double t[3])
{
  t[0] = m[3][0];  t[1] = m[3][1];  t[2] = m[3][2];
  m[3][0] = m[3][1] = m[3][2] = 0.0;
}

template <class T> inline void
Xform<T>::removeTranslation(float t[3])
{
  t[0] = m[3][0];  t[1] = m[3][1];  t[2] = m[3][2];
  m[3][0] = m[3][1] = m[3][2] = 0.0;
}

template <class T> inline void
Xform<T>::scale(const double s[3]) // M <- S*M
{
  m[0][0] *= s[0]; m[1][0] *= s[0]; m[2][0] *= s[0]; m[3][0] *= s[0];
  m[0][1] *= s[1]; m[1][1] *= s[1]; m[2][1] *= s[1]; m[3][1] *= s[1];
  m[0][2] *= s[2]; m[1][2] *= s[2]; m[2][2] *= s[2]; m[3][2] *= s[2];
}

template <class T> inline void
Xform<T>::scale(const float s[3]) // M <- S*M
{
  m[0][0] *= s[0]; m[1][0] *= s[0]; m[2][0] *= s[0]; m[3][0] *= s[0];
  m[0][1] *= s[1]; m[1][1] *= s[1]; m[2][1] *= s[1]; m[3][1] *= s[1];
  m[0][2] *= s[2]; m[1][2] *= s[2]; m[2][2] *= s[2]; m[3][2] *= s[2];
}

template <class T> inline void
Xform<T>::scale(T x, T y, T z) // M <- S*M
{
  m[0][0] *= x; m[1][0] *= x; m[2][0] *= x; m[3][0] *= x;
  m[0][1] *= y; m[1][1] *= y; m[2][1] *= y; m[3][1] *= y;
  m[0][2] *= z; m[1][2] *= z; m[2][2] *= z; m[3][2] *= z;
}

template <class T> inline void
Xform<T>::get_e(int i, T e[3]) const
{
  e[0] = m[i][0];  e[1] = m[i][1];  e[2] = m[i][2];
}

template <class T> inline Xform<T> &
Xform<T>::rotX(T a) // M <- R*M
{
  double sa = sin(a);
  double ca = cos(a);
  double row1[4];
  for (int i=0; i<4; i++) row1[i] = m[i][1]; // temp. copy
  for (int i=0; i<4; i++)     m[i][1] = ca*row1[i] - sa*m[i][2];
  for (int i=0; i<4; i++)     m[i][2] = sa*row1[i] + ca*m[i][2];
  return *this;
}

template <class T> inline Xform<T> &
Xform<T>::rotY(T a) // M <- R*M
{
  double sa = sin(a);
  double ca = cos(a);
  double row0[4];
  for (int i=0; i<4; i++) row0[i] =  m[i][0]; // temp. copy
  for (int i=0; i<4; i++)     m[i][0] =  ca*row0[i] + sa*m[i][2];
  for (int i=0; i<4; i++)     m[i][2] = -sa*row0[i] + ca*m[i][2];
  return *this;
}

template <class T> inline Xform<T> &
Xform<T>::rotZ(T a) // M <- R*M
{
  double sa = sin(a);
  double ca = cos(a);
  double row0[4];
  for (int i=0; i<4; i++) row0[i] = m[i][0]; // temp. copy
  for (int i=0; i<4; i++)     m[i][0] = ca*row0[i] - sa*m[i][1];
  for (int i=0; i<4; i++)     m[i][1] = sa*row0[i] + ca*m[i][1];
  return *this;
}

// rotate around (0,0,0)
template <class T> inline Xform<T> &
Xform<T>::rot(T angle, T ax, T ay, T az) // M <- R*M
{
  T q[4];
  q[0] = cos(angle/2.0);
  T fact = sin(angle/2.0) / sqrt(ax*ax+ay*ay+az*az);
  q[1] = ax*fact;
  q[2] = ay*fact;
  q[3] = az*fact;

  Xform<T> xf;
  xf.fromQuaternion(q);
  *this *= xf;
  return *this;
}

// rotate around (ox,oy,oz)
template <class T> inline Xform<T> &
Xform<T>::rot(T angle, T ax, T ay, T az,
	      T ox, T oy, T oz) // M <- R*M
{
  T q[4];
  q[0] = cos(angle/2.0);
  T fact = sin(angle/2.0) / sqrt(ax*ax+ay*ay+az*az);
  q[1] = ax*fact;
  q[2] = ay*fact;
  q[3] = az*fact;

  Xform<T> xf;
  xf.fromQuaternion(q);

  // add the part that makes the xf rotation to happen
  // around (ox,oy,oz)
  xf.m[3][0] += ox - (xf.m[0][0]*ox+xf.m[1][0]*oy+xf.m[2][0]*oz);
  xf.m[3][1] += oy - (xf.m[0][1]*ox+xf.m[1][1]*oy+xf.m[2][1]*oz);
  xf.m[3][2] += oz - (xf.m[0][2]*ox+xf.m[1][2]*oy+xf.m[2][2]*oz);

  *this *= xf;

  return *this;
}

template <class T> inline Xform<T> &
Xform<T>::rotQ(T q0, T q1, T q2, T q3)   // M <- R*M
{
  T q[4];
  q[0] = q0;  q[1] = q1;  q[2] = q2;  q[3] = q3;

  Xform<T> xf;
  xf.fromQuaternion(q);
  *this *= xf;
  return *this;
}


template <class T> inline void
Xform<T>::get_rot(T &angle, T &ax, T &ay, T &az)
{
  T q[4];
  toQuaternion(q);
  T len = sqrt(q[1]*q[1]+q[2]*q[2]+q[3]*q[3]);
  if (len == 0.0) {
    angle = ax = ay = 0.0; az = 1.0;
  } else {
    angle = acos(q[0]) * 2.0;
    T invlen = 1.0/len;
    if (sin(angle*.5) < 0.0) {
      ax = -q[1]*invlen;
      ay = -q[2]*invlen;
      az = -q[3]*invlen;
    } else {
      ax = q[1]*invlen;
      ay = q[2]*invlen;
      az = q[3]*invlen;
    }
  }
}

template <class T> inline void
Xform<T>::apply(const double i[3], double o[3]) const // o = M*i
{
  T invw =
    1.0 / (m[0][3]*i[0]+m[1][3]*i[1]+m[2][3]*i[2]+m[3][3]);
  o[0] = (m[0][0]*i[0]+m[1][0]*i[1]+m[2][0]*i[2]+m[3][0])*invw;
  o[1] = (m[0][1]*i[0]+m[1][1]*i[1]+m[2][1]*i[2]+m[3][1])*invw;
  o[2] = (m[0][2]*i[0]+m[1][2]*i[1]+m[2][2]*i[2]+m[3][2])*invw;
}

template <class T> inline void
Xform<T>::apply(const float i[3], float o[3]) const // o = M*i
{
  T invw =
    1.0 / (m[0][3]*i[0]+m[1][3]*i[1]+m[2][3]*i[2]+m[3][3]);
  o[0] = (m[0][0]*i[0]+m[1][0]*i[1]+m[2][0]*i[2]+m[3][0])*invw;
  o[1] = (m[0][1]*i[0]+m[1][1]*i[1]+m[2][1]*i[2]+m[3][1])*invw;
  o[2] = (m[0][2]*i[0]+m[1][2]*i[1]+m[2][2]*i[2]+m[3][2])*invw;
}

template <class T> inline void
Xform<T>::apply_inv(const double i[3], double o[3]) const // o = M^-1*i
{
  T tx = i[0] - m[3][0];
  T ty = i[1] - m[3][1];
  T tz = i[2] - m[3][2];
  o[0] = m[0][0]*tx + m[0][1]*ty + m[0][2]*tz;
  o[1] = m[1][0]*tx + m[1][1]*ty + m[1][2]*tz;
  o[2] = m[2][0]*tx + m[2][1]*ty + m[2][2]*tz;
}

template <class T> inline void
Xform<T>::apply_inv(const float i[3],  float o[3]) const // o = M^-1*i
{
  T tx = i[0] - m[3][0];
  T ty = i[1] - m[3][1];
  T tz = i[2] - m[3][2];
  o[0] = m[0][0]*tx + m[0][1]*ty + m[0][2]*tz;
  o[1] = m[1][0]*tx + m[1][1]*ty + m[1][2]*tz;
  o[2] = m[2][0]*tx + m[2][1]*ty + m[2][2]*tz;
}

template <class T> inline void
Xform<T>::operator()(double i[3]) const
{
  T o[3];
  o[0] = i[0]; o[1] = i[1]; o[2] = i[2];
  T invw =
    1.0 / (m[0][3]*o[0]+m[1][3]*o[1]+m[2][3]*o[2]+m[3][3]);
  i[0] = (m[0][0]*o[0]+m[1][0]*o[1]+m[2][0]*o[2]+m[3][0])*invw;
  i[1] = (m[0][1]*o[0]+m[1][1]*o[1]+m[2][1]*o[2]+m[3][1])*invw;
  i[2] = (m[0][2]*o[0]+m[1][2]*o[1]+m[2][2]*o[2]+m[3][2])*invw;
}

template <class T> inline void
Xform<T>::operator()(float i[3]) const
{
  T o[3];
  o[0] = i[0]; o[1] = i[1]; o[2] = i[2];
  T invw =
    1.0 / (m[0][3]*o[0]+m[1][3]*o[1]+m[2][3]*o[2]+m[3][3]);
  i[0] = (m[0][0]*o[0]+m[1][0]*o[1]+m[2][0]*o[2]+m[3][0])*invw;
  i[1] = (m[0][1]*o[0]+m[1][1]*o[1]+m[2][1]*o[2]+m[3][1])*invw;
  i[2] = (m[0][2]*o[0]+m[1][2]*o[1]+m[2][2]*o[2]+m[3][2])*invw;
}

template <class T> inline void
Xform<T>::apply_nrm(const double i[3], double o[3]) const // o = M*i
{
  double invw =
    1.0 / (m[0][3]*i[0]+m[1][3]*i[1]+m[2][3]*i[2]+m[3][3]);
  o[0] = (m[0][0]*i[0]+m[1][0]*i[1]+m[2][0]*i[2])*invw;
  o[1] = (m[0][1]*i[0]+m[1][1]*i[1]+m[2][1]*i[2])*invw;
  o[2] = (m[0][2]*i[0]+m[1][2]*i[1]+m[2][2]*i[2])*invw;
}

template <class T> inline void
Xform<T>::apply_nrm(const float i[3], float o[3]) const // o = M*i
{
  double invw =
    1.0 / (m[0][3]*i[0]+m[1][3]*i[1]+m[2][3]*i[2]+m[3][3]);
  o[0] = (m[0][0]*i[0]+m[1][0]*i[1]+m[2][0]*i[2])*invw;
  o[1] = (m[0][1]*i[0]+m[1][1]*i[1]+m[2][1]*i[2])*invw;
  o[2] = (m[0][2]*i[0]+m[1][2]*i[1]+m[2][2]*i[2])*invw;
}

template <class T> inline T&
Xform<T>::operator()(int i, int j)
{
  return m[j][i];
}

template <class T> inline T
Xform<T>::operator()(int i, int j) const
{
  return m[j][i];
}

template <class T> inline Pnt3
Xform<T>::unproject(float u, float v, float z) const
{
  float w =  1.0/(u*m[0][3]+v*m[1][3]+z*m[2][3]+m[3][3]);
  return Pnt3((u*m[0][0]+v*m[1][0]+z*m[2][0]+m[3][0])*w,
	      (u*m[0][1]+v*m[1][1]+z*m[2][1]+m[3][1])*w,
	      (u*m[0][2]+v*m[1][2]+z*m[2][2]+m[3][2])*w);
}

template <class T> inline Pnt3
Xform<T>::unproject_fast(float u, float v, float z) const
{
  float w =  1.0/(z*m[2][3]+m[3][3]);
  return Pnt3((u*m[0][0]+m[3][0])*w,
	      (u*m[1][1]+m[3][1])*w,
	      -w);
}

template <class T> inline Xform<T> &
Xform<T>::operator*=(const Xform<T> &a) // (*this) = a * (*this)
{
  Xform<T> b((T*)this);
  for (int i=0; i<4; i++) {
    for (int j=0; j<4; j++) {
      m[j][i] = a.m[0][i] * b.m[j][0] + a.m[1][i] * b.m[j][1] +
	        a.m[2][i] * b.m[j][2] + a.m[3][i] * b.m[j][3];
    }
  }
  return *this;
}


// test how close to a rigid transformation the current
// transformation is, ideally returns 0
template <class T> inline float
Xform<T>::test_rigidity(void)
{
  float ans = 0.0;
  // check the projective row
  ans = max(ans, fabsf(m[0][3]));
  ans = max(ans, fabsf(m[1][3]));
  ans = max(ans, fabsf(m[2][3]));
  ans = max(ans, fabsf(m[3][3]-1.0));
  // test the dots of rows with itself
  ans = max(ans, fabsf(dot(Pnt3(m[0]), Pnt3(m[0]))-1.0));
  ans = max(ans, fabsf(dot(Pnt3(m[1]), Pnt3(m[1]))-1.0));
  ans = max(ans, fabsf(dot(Pnt3(m[2]), Pnt3(m[2]))-1.0));
  // test the dots of rows with other rows
  ans = max(ans, fabsf(dot(Pnt3(m[0]), Pnt3(m[1]))));
  ans = max(ans, fabsf(dot(Pnt3(m[1]), Pnt3(m[0]))));
  ans = max(ans, fabsf(dot(Pnt3(m[2]), Pnt3(m[0]))));
  ans = max(ans, fabsf(dot(Pnt3(m[0]), Pnt3(m[2]))));
  ans = max(ans, fabsf(dot(Pnt3(m[1]), Pnt3(m[2]))));
  ans = max(ans, fabsf(dot(Pnt3(m[2]), Pnt3(m[1]))));
  return ans;
}


// force the rigidity of the xform by setting the projective
// part to 0 0 0 1 and making sure rotation part is just a rotation
template <class T> inline Xform<T> &
Xform<T>::enforce_rigidity(void)
{
  m[0][3] = m[1][3] = m[2][3] = 0.0;
  m[3][3] = 1.0;
  T q[4];
  toQuaternion(q);
  // fromQuaternion() normalizes q
  fromQuaternion(q, m[3][0], m[3][1], m[3][2]);
  return *this;
}


template <class T> inline Xform<T>
operator*(const Xform<T> &a, const Xform<T> &b)
{
  Xform<T> out;
  for (int i=0; i<4; i++) {
    for (int j=0; j<4; j++) {
      out.m[j][i] =
	a.m[0][i] * b.m[j][0] + a.m[1][i] * b.m[j][1] +
	a.m[2][i] * b.m[j][2] + a.m[3][i] * b.m[j][3];
    }
  }
  return out;
}


// delta * a = b
template <class T> inline Xform<T>
delta(Xform<T> a,
      const Xform<T> &b)
{
  return b * a.fast_invert();
}

// Assume the transform only has rotation and translation.
// Calculate a delta matrix that transforms a to b.
// For other parts than rotation, do just regular interpolation.
// For rotation, calculate the quaternion, rotation axis and
// angle, interpolate the angle, transform back.
template <class T> inline Xform<T>
linear_interp(const Xform<T> &a,
	      const Xform<T> &b,
	      double          t)
{
  Xform<T> d = delta(a,b);
  // make sure no projective or scaling part
  assert(d.m[0][3] == 0.0);
  assert(d.m[1][3] == 0.0);
  assert(d.m[2][3] == 0.0);
  assert(d.m[3][3] == 1.0);
  // interpolate translation
  T tx = d.m[3][0]*t;
  T ty = d.m[3][1]*t;
  T tz = d.m[3][2]*t;
  // interpolate rotation
  T angle, x,y,z;
  d.get_rot(angle, x,y,z);
  d.identity();
  d.rot(angle*t, x,y,z);

  d.m[3][0] = tx;
  d.m[3][1] = ty;
  d.m[3][2] = tz;

  return d*a;
}

template <class T> inline Xform<T>
bilinear_interp(const Xform<T> &u0v0,
		const Xform<T> &u1v0,
		const Xform<T> &u0v1,
		const Xform<T> &u1v1,
		double u, double v)
{
  return linear_interp(linear_interp(u0v0, u1v0, u),
		       linear_interp(u0v1, u1v1, u), v);
}

template <class T> inline Xform<T>
relative_xform(const Xform<T> &a, Xform<T> b)
{
  return b.fast_invert() * a;
}

template <class T> inline ostream&
operator<<(ostream &out, const Xform<T> &xf)
{
  for (int i=0; i<4; i++) {
    for (int j=0; j<4; j++) {
      //out << xf(i,j) << " ";
      out << xf.m[j][i] << " ";
    }
    out << endl;
  }
  return out;
}

template <class T> inline istream&
operator>>(istream &in, Xform<T> &xf)
{
  for (int i=0; i<4; i++) {
    for (int j=0; j<4; j++) {
      in >> ws >> xf.m[j][i];
    }
  }
  return in;
}





#endif

//
// testing part...
//
#if 0

#define SHOW(x) cout << #x " = " << x << endl
#define SHOW3(x) cout << #x " = " << x[0] << " " << x[1] << " " << x[2] << " " << endl

void
main(void)
{
  Xform<double> xf;
  double a[3] = { 1, 2, 3 };
  double b[3] = { 3,-1, 2 };
  double c[3];

  xf.translate(a);
  SHOW3(a);
  xf.getTranslation(c);
  SHOW3(c);
  xf.apply(a,c);
  SHOW3(c);
  xf.apply(b,c);
  SHOW3(b);
  SHOW3(c);

  cout << endl << "Test Euler angle stuff" << endl;
  cout << "Rotate vector around X by 45 deg" << endl;
  SHOW3(a);
  xf.identity();
  xf.rotX(M_PI/4.0);
  xf.apply(a,c);
  SHOW3(c);
  cout << "Rotate vector around Y by 45 deg" << endl;
  SHOW3(a);
  xf.identity();
  xf.rotY(M_PI/4.0);
  xf.apply(a,c);
  SHOW3(c);
  cout << "Rotate vector around Z by 45 deg" << endl;
  SHOW3(a);
  xf.identity();
  xf.rotZ(M_PI/4.0);
  xf.apply(a,c);
  SHOW3(c);
  cout << endl;
  double ea[3] = { M_PI/8.0, M_PI/7.0, M_PI/6.0 };
  SHOW3(ea);
  xf.fromEuler(ea, EulOrdXYZs);
  cout << "fromEuler()" << endl;
  SHOW(xf);
  xf.identity();
  xf.rotX(ea[0]);
  xf.rotY(ea[1]);
  xf.rotZ(ea[2]);
  SHOW(xf);
  xf.toEuler(ea, EulOrdXYZs);
  SHOW3(ea);

  cout << endl << "Test quaternions: " << endl;
  double q[4] = { .5, .5, .5, .5 };
  cout << q[0] << " "<< q[1] << " "<< q[2] << " "<< q[3] << endl;
  xf.fromQuaternion(q);
  cout << xf << endl;
  q[0] = q[1] = q[2] = q[3] = 0.0;
  xf.toQuaternion(q);
  cout << q[0] << " "<< q[1] << " "<< q[2] << " "<< q[3] << endl;
  float fq[4] = { .5, .5, .5, .5 };
  cout << fq[0] << " "<< fq[1] << " "<< fq[2] << " "<< fq[3] << endl;
  xf.fromQuaternion(fq);
  cout << xf << endl;
  fq[0] = fq[1] = fq[2] = fq[3] = 0.0;
  xf.toQuaternion(fq);
  cout << fq[0] << " "<< fq[1] << " "<< fq[2] << " "<< fq[3] << endl;

  cout << endl << "Test multiplication and inversion" << endl;
  Xform<double> xa,xb;
  xa.rotZ(M_PI/4.0);
  xa.translate(1,2,3);
  SHOW(xa);
  xb = xa;
  xb.fast_invert();
  SHOW(xb);
  SHOW(xa*xb);
  SHOW(xb*xa);
  cout << endl << "Test rotation" << endl;
  double angle, ax, ay, az;
  xa.identity();
  xa.rotX(M_PI/4.0);
  SHOW(xa);
  xa.get_rot(angle, ax, ay, az);
  SHOW(angle);
  SHOW(ax);
  SHOW(ay);
  SHOW(az);
  xb.identity();
  xb.rot(M_PI/4.0, 1, 0, 0);
  SHOW(xb);
  xa.identity();
  xa.rotY(M_PI/4.0);
  SHOW(xa);
  xa.get_rot(angle, ax, ay, az);
  SHOW(angle);
  SHOW(ax);
  SHOW(ay);
  SHOW(az);
  xb.identity();
  xb.rot(M_PI/4.0, 0, 1, 0);
  SHOW(xb);
  xa.identity();
  xa.rotZ(M_PI/4.0);
  SHOW(xa);
  xa.get_rot(angle, ax, ay, az);
  SHOW(angle);
  SHOW(ax);
  SHOW(ay);
  SHOW(az);
  xb.identity();
  xb.rot(M_PI/4.0, 0, 0, 1);
  SHOW(xb);
  xa.identity();
  SHOW(xa.rot(-.34, 1, -1, 1));
  xa.get_rot(angle, ax, ay, az);
  SHOW(angle);
  SHOW(ax);
  SHOW(ay);
  SHOW(az);

  cout << endl << "Test interpolation" << endl;
  xa.identity();
  xa.rotZ(M_PI/4.0);
  xa.translate(1,2,3);
  SHOW(xa);
  xb.identity();
  xb.rotY(-M_PI/4.0);
  xb.translate(-3,-2,-1);
  SHOW(xb);
  for (int i=0; i<=10; i++) {
    xf = linear_interp(xa,xb,i/10.0);
    SHOW(xf);
    a[0]=1;a[1]=1;a[2]=1;
    xf.apply(a,b);
    SHOW3(b);
  }
}
#endif











