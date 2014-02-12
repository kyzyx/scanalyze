//###############################################################
// Pnt3.h
// Kari Pulli
// 11/08/1996
//###############################################################

#ifndef _pnt3_h
#define _pnt3_h

#include<math.h>
#include<iostream>
#ifdef WIN32
#include<float.h>
#endif
#ifdef sgi
#include<ieeefp.h>
#endif

using namespace std;

#ifdef sun
#define sqrtf(x) sqrt(x)
#endif

#define SHOW(X) cout << #X " = " << X << endl

class Pnt3 {
protected:
  float v[3];
public:
  Pnt3(float a=0.0, float b=0.0, float c=0.0)
    { v[0] = a, v[1] = b, v[2] = c;}
  Pnt3(const float *a)    { v[0] = a[0], v[1] = a[1], v[2] = a[2];}
  Pnt3(const double *a)   { v[0] = a[0], v[1] = a[1], v[2] = a[2];}

  Pnt3 &set(float a=0.0, float b=0.0, float c=0.0)
    { v[0] = a, v[1] = b, v[2] = c; return *this; }
  Pnt3 &set(const float *a)
    { v[0] = a[0], v[1] = a[1], v[2] = a[2]; return *this; }
  Pnt3 &set(const double *a)
    { v[0] = a[0], v[1] = a[1], v[2] = a[2]; return *this; }

  // linear interpolation
  Pnt3 &lerp(float t, const Pnt3 &a, const Pnt3 &b)
    {
      float u = 1.0 - t;
      v[0]=u*a[0]+t*b[0];
      v[1]=u*a[1]+t*b[1];
      v[2]=u*a[2]+t*b[2];
      return *this;
    }

  Pnt3 &set_max(const Pnt3 &p)
    {
      if (p.v[0] > v[0]) v[0] = p.v[0];
      if (p.v[1] > v[1]) v[1] = p.v[1];
      if (p.v[2] > v[2]) v[2] = p.v[2];
      return *this;
    }
  Pnt3 &set_min(const Pnt3 &p)
    {
      if (p.v[0] < v[0]) v[0] = p.v[0];
      if (p.v[1] < v[1]) v[1] = p.v[1];
      if (p.v[2] < v[2]) v[2] = p.v[2];
      return *this;
    }

#ifdef WIN32
  bool  isfinite(void) { return _finite(v[0]) && _finite(v[1]) && _finite(v[2]); }
#else
  bool  isfinite(void) { return finite(v[0]) && finite(v[1]) && finite(v[2]); }
#endif

  Pnt3  operator-() const       { return Pnt3(-v[0],-v[1],-v[2]); }
  Pnt3& operator+=(const Pnt3 &);
  Pnt3& operator-=(const Pnt3 &);
  Pnt3& operator*=(const float &);
  Pnt3& operator/=(const float &);
  int   operator==(const Pnt3 &);
  int   operator!=(const Pnt3 &);
  operator const float *(void) const   { return v; }
  operator       float *(void)         { return v; }
  operator const char  *(void) const   { return (char *)v; }
  operator       char  *(void)         { return (char *)v; }
  float& operator[](int i)             { return v[i]; }
  const float& operator[](int i) const { return v[i]; }
  friend ostream& operator<<(ostream &out, const Pnt3 &a);
  friend istream& operator>>(istream &in, Pnt3 &a);
  // operators that are not part of class: +,-,*,/

  float         norm(void) const;
  float         norm2(void) const;
  Pnt3 &        normalize(void);
  Pnt3 &        set_norm(float len);
  friend float  dist(const Pnt3 &, const Pnt3 &);
  friend float  dist2(const Pnt3 &, const Pnt3 &);
  friend float  dist_2d(const Pnt3 &, const Pnt3 &);
  friend float  dist2_2d(const Pnt3 &, const Pnt3 &);
  friend float  dist_manhattan(const Pnt3 &, const Pnt3 &);
  friend float  dist2_lineseg(const Pnt3 &, const Pnt3 &, const Pnt3 &);
  friend float  dist2_tri(const Pnt3 &,
			  const Pnt3 &, const Pnt3 &, const Pnt3 &);

  friend bool   closer_on_lineseg(const Pnt3 &, Pnt3 &,
				  const Pnt3 &,
				  const Pnt3 &, float &);
  friend bool   closer_on_tri(const Pnt3 &, Pnt3 &, const Pnt3 &,
			      const Pnt3 &, const Pnt3 &, float &);

  friend float  dot(const Pnt3 &a, const Pnt3 &b);
  friend Pnt3   cross(const Pnt3 &a, const Pnt3 &b);
  friend Pnt3   cross(const Pnt3 &a, const Pnt3 &b,
		      const Pnt3 &c);
  friend Pnt3   normal(const Pnt3 &, const Pnt3 &, const Pnt3 &);
  friend float  det(const Pnt3 &, const Pnt3 &, const Pnt3 &);
  friend void   line_plane_X(const Pnt3& p, const Pnt3& dir,
			     const Pnt3& t1, const Pnt3& t2,
			     const Pnt3& t3,
			     Pnt3 &x, float &dist);
  friend void   line_plane_X(const Pnt3& p, const Pnt3& dir,
			     const Pnt3& nrm, float d,
			     Pnt3 &x, float &dist);

  friend void   bary(const Pnt3& p,  const Pnt3& t1,
		     const Pnt3& t2, const Pnt3& t3,
		     float &b1, float &b2, float &b3);
  friend void   bary_fast(const Pnt3& p, const Pnt3& n,
			  const Pnt3 &t0, const Pnt3& v1, const Pnt3& v2,
			  float &b1, float &b2, float &b3);
  friend void   bary(const Pnt3& p,  const Pnt3& dir,
		     const Pnt3& t1, const Pnt3& t2,
		     const Pnt3& t3,
		     float &b1, float &b2, float &b3);
  friend int    above_plane(const Pnt3& p, const Pnt3& a,
			    const Pnt3& b, const Pnt3& c);

  float         smallest_circle(const Pnt3 &, const Pnt3 &, const Pnt3 &);

  // functions used for closest point searching in KDtrees and octrees
  friend bool   ball_within_bounds(const Pnt3 &, float,
				   const Pnt3 &, float);
  friend bool   ball_within_bounds(const Pnt3 &, float,
				   const Pnt3 &, const Pnt3 &);
  friend bool   bounds_overlap_ball(const Pnt3 &, float,
				    const Pnt3 &, float);
  friend bool   bounds_overlap_ball(const Pnt3 &, float,
				    const Pnt3 &, const Pnt3 &);
  friend bool   spheres_intersect(const Pnt3 &, const Pnt3 &,
				  float, float);
  friend bool   lines_intersect(const Pnt3 &p1,
				const Pnt3 &p2,
				const Pnt3 &p3,
				const Pnt3 &p4,
				Pnt3 &isect);

  // rigid transformations only (rot + trans)
  Pnt3 &        xform(float m[16]);// OpenGL matrix: p . M = p'
  Pnt3 &        xform(double m[16]);// OpenGL matrix: p . M = p'
  Pnt3 &        xform(float r[3][3], float  t[3]);
  Pnt3 &        xform(float r[3][3], double t[3]);
  Pnt3 &        invxform(float m[16]);// OpenGL matrix: p . M = p'
  Pnt3 &        invxform(double m[16]);// OpenGL matrix: p . M = p'
  Pnt3 &        invxform(float r[3][3], float  t[3]);
  Pnt3 &        invxform(float r[3][3], double t[3]);

  Pnt3 & setXformed(const Pnt3 &p, float r[3][3], float t[3]);
  Pnt3 & setRotated(const Pnt3 &p, float r[3][3]);
};


inline Pnt3&
Pnt3::operator+=(const Pnt3 &a)
{
  v[0] += a.v[0]; v[1] += a.v[1]; v[2] += a.v[2];
  return *this;
}

inline Pnt3&
Pnt3::operator-=(const Pnt3 &a)
{
  v[0] -= a.v[0]; v[1] -= a.v[1]; v[2] -= a.v[2];
  return *this;
}

inline Pnt3&
Pnt3::operator*=(const float &a)
{
  v[0] *= a; v[1] *= a; v[2] *= a;
  return *this;
}

inline Pnt3&
Pnt3::operator/=(const float &a)
{
  float tmp = 1.0f / a;
  v[0] *= tmp; v[1] *= tmp; v[2] *= tmp;
  return *this;
}

inline int
Pnt3::operator==(const Pnt3 &a)
{
  return (a.v[0]==v[0] && a.v[1]==v[1] && a.v[2]==v[2]);
}

inline int
Pnt3::operator!=(const Pnt3 &a)
{
  return (a.v[0]!=v[0] || a.v[1]!=v[1] || a.v[2]!=v[2]);
}

inline Pnt3
operator+(const Pnt3 &a, const Pnt3 &b)
{
  Pnt3 tmp = a;
  return (tmp += b);
}

inline Pnt3
operator-(const Pnt3 &a, const Pnt3 &b)
{
  Pnt3 tmp = a;
  return (tmp -= b);
}

inline Pnt3
operator*(const Pnt3 &a, const float &b)
{
  Pnt3 tmp = a;
  return (tmp *= b);
}

inline Pnt3
operator*(const float &a, const Pnt3 &b)
{
  Pnt3 tmp = b;
  return (tmp *= a);
}

inline Pnt3
operator/(const Pnt3 &a, const float &b)
{
  Pnt3 tmp = a;
  return (tmp /= b);
}

inline ostream&
operator<<(ostream &out, const Pnt3 &a)
{
  return out << "["<< a.v[0] <<" "<< a.v[1] <<" "<< a.v[2] << "]";
}

inline istream&
operator>>(istream &in, Pnt3 &a)
{
  char c = 0;
  in >> ws >> c;
  if (c == '[') {
    in >> a.v[0] >> a.v[1] >> a.v[2];
    in >> c;
    if (c != ']') in.clear(ios::badbit);
  } else {
    in.putback(c);
    in >> a.v[0] >> a.v[1] >> a.v[2];
  }
  return in;
}

inline float
Pnt3::norm() const
{
  return sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

inline float
Pnt3::norm2() const
{
  return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}

inline Pnt3 &
Pnt3::normalize(void)
{
  float a = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
  if (a != 0) {
    a = 1.0/sqrtf(a);
    v[0] *= a; v[1] *= a; v[2] *= a;
  }
  return *this;
}

inline Pnt3 &
Pnt3::set_norm(float len)
{
  float a = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
  if (a != 0) {
    a = len/sqrtf(a);
    v[0] *= a; v[1] *= a; v[2] *= a;
  }
  return *this;
}

inline float
dist2(const Pnt3 &a, const Pnt3 &b)
{
  float x = a.v[0]-b.v[0];
  float y = a.v[1]-b.v[1];
  float z = a.v[2]-b.v[2];
  return x*x + y*y + z*z;
}

inline float
dist(const Pnt3 &a, const Pnt3 &b)
{
  return sqrtf(dist2(a,b));
}

inline float
dist2_2d(const Pnt3 &a, const Pnt3 &b)
{
  float x = a.v[0]-b.v[0];
  float y = a.v[1]-b.v[1];
  return x*x + y*y;
}

inline float
dist_2d(const Pnt3 &a, const Pnt3 &b)
{
  return sqrtf(dist2_2d(a,b));
}

inline float
dist_manhattan(const Pnt3 &a, const Pnt3 &b)
{
  float d = fabs(a[0]-b[0]);
  float t = fabs(a[1]-b[1]);
  if (t > d) d = t;
  t = fabs(a[2]-b[2]);
  if (t > d) return t;
  return d;
}

// distance from x to linesegment from a to b
inline float
dist2_lineseg(const Pnt3 &x, const Pnt3 &a, const Pnt3 &b)
{
  Pnt3 ba = b; ba -= a;
  Pnt3 xa = x; xa -= a;

  float xa_ba = dot(xa,ba);
  // if the dot product is negative, the point is closest to a
  if (xa_ba < 0.0)   return dist2(x,a);

  // if the dot product is greater than squared segment length,
  // the point is closest to b
  float nba2 = ba.norm2();
  if (xa_ba >= nba2) return dist2(x,b);

  // take the squared dist x-a, squared dot of x-a to unit b-a,
  // use Pythagoras' rule
  return xa.norm2() - xa_ba*xa_ba/nba2;
}


inline float
dist2_tri(const Pnt3 &x,
	  const Pnt3 &t1, const Pnt3 &t2, const Pnt3 &t3)
{
  // calculate the normal and distance from the plane
  Pnt3 v1(t2.v[0]-t1.v[0], t2.v[1]-t1.v[1], t2.v[2]-t1.v[2]);
  Pnt3 v2(t3.v[0]-t1.v[0], t3.v[1]-t1.v[1], t3.v[2]-t1.v[2]);
  Pnt3 n = cross(v1,v2);
  float n_inv_mag2 = 1.0/n.norm2();
  float tmp  = (x.v[0]-t1.v[0])*n.v[0] +
               (x.v[1]-t1.v[1])*n.v[1] +
               (x.v[2]-t1.v[2])*n.v[2];
  float distp2 = tmp * tmp * n_inv_mag2;

  // calculate the barycentric coordinates of the point
  // (projected onto tri plane) with respect to v123
  float b1,b2,b3;
  float f = tmp*n_inv_mag2;
  Pnt3  pp(x[0]-f*n[0], x[1]-f*n[1], x[2]-f*n[2]);
  bary_fast(pp, n, t1,v1,v2, b1,b2,b3);

  // all non-negative, the point is within the triangle
  if (b1 >= 0.0 && b2 >= 0.0 && b3 >= 0.0) {
    return distp2;
  }

  // look at the signs of the barycentric coordinates
  // if there are two negative signs, the positive
  // one tells the vertex that's closest
  // if there's one negative sign, the opposite edge
  // (with endpoints) is closest

  if (b1 < 0.0) {
    if (b2 < 0.0) {
      return dist2(x,t3);
    } else if (b3 < 0.0) {
      return dist2(x,t2);
    } else return dist2_lineseg(x, t2,t3);
  } else if (b2 < 0.0) {
    if (b3 < 0.0) {
      return dist2(x,t1);
    } else return dist2_lineseg(x, t1,t3);
  } else   return dist2_lineseg(x, t1,t2);
}


inline bool
closer_on_lineseg(const Pnt3 &x, Pnt3 &cp, const Pnt3 &a,
		  const Pnt3 &b, float &d2)
{
  Pnt3 ba(b.v[0]-a.v[0], b.v[1]-a.v[1], b.v[2]-a.v[2]);
  Pnt3 xa(x.v[0]-a.v[0], x.v[1]-a.v[1], x.v[2]-a.v[2]);

  float xa_ba = dot(xa,ba);
  // if the dot product is negative, the point is closest to a
  if (xa_ba < 0.0) {
    float nd = dist2(x,a);
    if (nd < d2) { cp = a; d2 = nd; return true; }
    return false;
  }

  // if the dot product is greater than squared segment length,
  // the point is closest to b
  float fact = xa_ba/ba.norm2();
  if (fact >= 1.0) {
    float nd = dist2(x,b);
    if (nd < d2) { cp = b; d2 = nd; return true; }
    return false;
  }

  // take the squared dist x-a, squared dot of x-a to unit b-a,
  // use Pythagoras' rule
  float nd = xa.norm2() - xa_ba*fact;
  if (nd < d2) {
    d2 = nd;
    cp.v[0] = a.v[0] + fact * ba.v[0];
    cp.v[1] = a.v[1] + fact * ba.v[1];
    cp.v[2] = a.v[2] + fact * ba.v[2];
    return true;
  }
  return false;
}


inline bool
closer_on_tri(const Pnt3 &x, Pnt3 &cp, const Pnt3 &t1,
	      const Pnt3 &t2, const Pnt3 &t3, float &d2)
{
  // calculate the normal and distance from the plane
  Pnt3 v1(t2.v[0]-t1.v[0], t2.v[1]-t1.v[1], t2.v[2]-t1.v[2]);
  Pnt3 v2(t3.v[0]-t1.v[0], t3.v[1]-t1.v[1], t3.v[2]-t1.v[2]);
  Pnt3 n = cross(v1,v2);
  float n_inv_mag2 = 1.0/n.norm2();
  float tmp  = (x.v[0]-t1.v[0])*n.v[0] +
               (x.v[1]-t1.v[1])*n.v[1] +
               (x.v[2]-t1.v[2])*n.v[2];
  float distp2 = tmp * tmp * n_inv_mag2;
  if (distp2 >= d2) return false;

  // calculate the barycentric coordinates of the point
  // (projected onto tri plane) with respect to v123
  float b1,b2,b3;
  float f = tmp*n_inv_mag2;
  Pnt3  pp(x[0]-f*n[0], x[1]-f*n[1], x[2]-f*n[2]);
  bary_fast(pp, n, t1,v1,v2, b1,b2,b3);

  // all non-negative, the point is within the triangle
  if (b1 >= 0.0 && b2 >= 0.0 && b3 >= 0.0) {
    d2 = distp2;
    cp = pp;
    return true;
  }

  // look at the signs of the barycentric coordinates
  // if there are two negative signs, the positive
  // one tells the vertex that's closest
  // if there's one negative sign, the opposite edge
  // (with endpoints) is closest

  if (b1 < 0.0) {
    if (b2 < 0.0) {
      float nd = dist2(x,t3);
      if (nd < d2) { d2 = nd; cp = t3; return true; }
      else         { return false; }
    } else if (b3 < 0.0) {
      float nd = dist2(x,t2);
      if (nd < d2) { d2 = nd; cp = t2; return true; }
      else         { return false; }
    } else return closer_on_lineseg(x, cp, t2,t3, d2);
  } else if (b2 < 0.0) {
    if (b3 < 0.0) {
      float nd = dist2(x,t1);
      if (nd < d2) { d2 = nd; cp = t1; return true; }
      else         { return false; }
    } else return closer_on_lineseg(x, cp, t1,t3, d2);
  } else   return closer_on_lineseg(x, cp, t1,t2, d2);
}


inline float
dot(const Pnt3 &a, const Pnt3 &b)
{
  return (a.v[0]*b.v[0] + a.v[1]*b.v[1] + a.v[2]*b.v[2]);
}


inline Pnt3
cross(const Pnt3 &a, const Pnt3 &b)
{
  return Pnt3(a.v[1]*b.v[2] - a.v[2]*b.v[1],
	      a.v[2]*b.v[0] - a.v[0]*b.v[2],
	      a.v[0]*b.v[1] - a.v[1]*b.v[0]);
}


// two vectors, a and b, starting from c
inline Pnt3
cross(const Pnt3 &a, const Pnt3 &b, const Pnt3 &c)
{
  float a0 = a.v[0]-c.v[0];
  float a1 = a.v[1]-c.v[1];
  float a2 = a.v[2]-c.v[2];
  float b0 = b.v[0]-c.v[0];
  float b1 = b.v[1]-c.v[1];
  float b2 = b.v[2]-c.v[2];
  return Pnt3(a1*b2 - a2*b1, a2*b0 - a0*b2, a0*b1 - a1*b0);
}

// get a normal from triangle ABC, points given in ccw order
inline Pnt3
normal(const Pnt3 &a, const Pnt3 &b, const Pnt3 &c)
{
  float a0 = a.v[0]-c.v[0];
  float a1 = a.v[1]-c.v[1];
  float a2 = a.v[2]-c.v[2];
  float b0 = b.v[0]-c.v[0];
  float b1 = b.v[1]-c.v[1];
  float b2 = b.v[2]-c.v[2];
  float x = a1*b2 - a2*b1;
  float y = a2*b0 - a0*b2;
  float z = a0*b1 - a1*b0;
  float n = x*x+y*y+z*z;
  if (n == 0) {
    return Pnt3();
  } else {
    n = 1.0/sqrtf(n);
    return Pnt3(x*n, y*n, z*n);
  }
}

// determinant of 3 points
inline float
det(const Pnt3 &a, const Pnt3 &b, const Pnt3 &c)
{
  return a[0]*(b[1]*c[2]-b[2]*c[1])
    -    a[1]*(b[0]*c[2]-b[2]*c[0])
    +    a[2]*(b[0]*c[1]-b[1]*c[0]);
}

// calculate the intersection of a line going through p
// to direction dir with a plane spanned by t1,t2,t3
// (modified from Graphics Gems, p.299)
inline void
line_plane_X(const Pnt3& p, const Pnt3& dir,
	     const Pnt3& t1, const Pnt3& t2, const Pnt3& t3,
	     Pnt3 &x, float &dist)
{
  // note: normal doesn't need to be unit vector
  Pnt3  nrm = cross(t1,t2,t3);
  float tmp = dot(nrm,dir);
  if (tmp == 0.0) {
    cerr << "Cannot intersect plane with a parallel line" << endl;
    return;
  }
  // d  = -dot(nrm,t1)
  // t  = - (d + dot(p,nrm))/dot(dir,nrm)
  // is = p + dir * t
  x = dir;
  dist = (dot(nrm,t1)-dot(nrm,p))/tmp;
  x *= dist;
  x += p;
  if (dist < 0.0) dist = -dist;
}

inline void
line_plane_X(const Pnt3& p, const Pnt3& dir,
	    const Pnt3& nrm, float d, Pnt3 &x, float &dist)
{
  float tmp = dot(nrm,dir);
  if (tmp == 0.0) {
    cerr << "Cannot intersect plane with a parallel line" << endl;
    return;
  }
  x = dir;
  dist = -(d+dot(nrm,p))/tmp;
  x *= dist;
  x += p;
  if (dist < 0.0) dist = -dist;
}

// calculate barycentric coordinates of the point p
// on triangle t1 t2 t3
inline void
bary(const Pnt3& p,
     const Pnt3& t1, const Pnt3& t2, const Pnt3& t3,
     float &b1, float &b2, float &b3)
{
  // figure out the plane onto which to project the vertices
  // by calculating a cross product and finding its largest dimension
  // then use Cramer's rule to calculate two of the
  // barycentric coordinates
  // e.g., if the z coordinate is ignored, and v1 = t1-t3, v2 = t2-t3
  // b1 = det[x[0] v2[0]; x[1] v2[1]] / det[v1[0] v2[0]; v1[1] v2[1]]
  // b2 = det[v1[0] x[0]; v1[1] x[1]] / det[v1[0] v2[0]; v1[1] v2[1]]
  float v10 = t1.v[0]-t3.v[0];
  float v11 = t1.v[1]-t3.v[1];
  float v12 = t1.v[2]-t3.v[2];
  float v20 = t2.v[0]-t3.v[0];
  float v21 = t2.v[1]-t3.v[1];
  float v22 = t2.v[2]-t3.v[2];
  float c[2];
  c[0] = fabs(v11*v22 - v12*v21);
  c[1] = fabs(v12*v20 - v10*v22);
  int i = 0;
  if (c[1] > c[0]) i = 1;
  if (fabs(v10*v21 - v11*v20) > c[i]) {
    // ignore z
    float d = 1.0 / (v10*v21-v11*v20);
    float x0 = (p.v[0]-t3.v[0]);
    float x1 = (p.v[1]-t3.v[1]);
    b1 = (x0*v21 - x1*v20) * d;
    b2 = (v10*x1 - v11*x0) * d;
  } else if (i==0) {
    // ignore x
    float d = 1.0 / (v11*v22-v12*v21);
    float x0 = (p.v[1]-t3.v[1]);
    float x1 = (p.v[2]-t3.v[2]);
    b1 = (x0*v22 - x1*v21) * d;
    b2 = (v11*x1 - v12*x0) * d;
  } else {
    // ignore y
    float d = 1.0 / (v12*v20-v10*v22);
    float x0 = (p.v[2]-t3.v[2]);
    float x1 = (p.v[0]-t3.v[0]);
    b1 = (x0*v20 - x1*v22) * d;
    b2 = (v12*x1 - v10*x0) * d;
  }
  b3 = 1.0 - b1 - b2;
}


// calculate barycentric coordinates of the point p
// (already on the triangle plane) with normal vector n
// and two edge vectors v1 and v2,
// starting from a common vertex t0
inline void
bary_fast(const Pnt3& p, const Pnt3& n,
	  const Pnt3 &t0, const Pnt3& v1, const Pnt3& v2,
	  float &b1, float &b2, float &b3)
{
  // see bary above
  int i = 0;
  if (n.v[1] > n.v[0]) i = 1;
  if (n.v[2] > n.v[i]) {
    // ignore z
    float d = 1.0 / (v1.v[0]*v2.v[1]-v1.v[1]*v2.v[0]);
    float x0 = (p.v[0]-t0.v[0]);
    float x1 = (p.v[1]-t0.v[1]);
    b1 = (x0*v2.v[1] - x1*v2.v[0]) * d;
    b2 = (v1.v[0]*x1 - v1.v[1]*x0) * d;
  } else if (i==0) {
    // ignore x
    float d = 1.0 / (v1.v[1]*v2.v[2]-v1.v[2]*v2.v[1]);
    float x0 = (p.v[1]-t0.v[1]);
    float x1 = (p.v[2]-t0.v[2]);
    b1 = (x0*v2.v[2] - x1*v2.v[1]) * d;
    b2 = (v1.v[1]*x1 - v1.v[2]*x0) * d;
  } else {
    // ignore y
    float d = 1.0 / (v1.v[2]*v2.v[0]-v1.v[0]*v2.v[2]);
    float x0 = (p.v[2]-t0.v[2]);
    float x1 = (p.v[0]-t0.v[0]);
    b1 = (x0*v2.v[0] - x1*v2.v[2]) * d;
    b2 = (v1.v[2]*x1 - v1.v[0]*x0) * d;
  }
  b3 = 1.0 - b1 - b2;
}


// calculate barycentric coordinates for the intersection of
// a line starting from p, going to direction dir, and the plane
// of the triangle t1 t2 t3
inline void
bary(const Pnt3& p, const Pnt3& dir,
     const Pnt3& t1, const Pnt3& t2, const Pnt3& t3,
     float &b1, float &b2, float &b3)
{
  Pnt3 x; float d;
  line_plane_X(p, dir, t1, t2, t3, x, d);
  bary(x, t1,t2,t3, b1,b2,b3);
}

// is p above the plane spanned by triangle abc (ccw order)?
inline int
above_plane(const Pnt3& p, const Pnt3& a,
	    const Pnt3& b, const Pnt3& c)
{
  Pnt3 nrm = cross(a,b,c);
  return dot(p,nrm) > dot(a,nrm);
}



// find the smallest circle that covers a,b,c
// set self to the center, return radius
inline float
Pnt3::smallest_circle(const Pnt3 &A, const Pnt3 &B, const Pnt3 &C)
{
  Pnt3 a(B); a-=A;
  Pnt3 b(C); b-=B;
  Pnt3 c(A); c-=C;
  float da = a.norm2();
  float db = b.norm2();
  float dc = c.norm2();
  //const float *x;
  if (da > db && da > dc) {
    // da longest
    if (da >= db + dc) {
      // over 90 deg angle, solution is the center of the
      // longest edge
      v[0] = .5 * (A.v[0]+B.v[0]);
      v[1] = .5 * (A.v[1]+B.v[1]);
      v[2] = .5 * (A.v[2]+B.v[2]);
      return .5*sqrtf(da);
    }
  } else if (dc > db) {
    // dc longest
    if (dc >= db + da) {
      // over 90 deg angle, solution is the center of the
      // longest edge
      v[0] = .5 * (A.v[0]+C.v[0]);
      v[1] = .5 * (A.v[1]+C.v[1]);
      v[2] = .5 * (A.v[2]+C.v[2]);
      return .5*sqrtf(dc);
    }
  } else {
    // db longest
    if (db >= da + dc) {
      // over 90 deg angle, solution is the center of the
      // longest edge
      v[0] = .5 * (B.v[0]+C.v[0]);
      v[1] = .5 * (B.v[1]+C.v[1]);
      v[2] = .5 * (B.v[2]+C.v[2]);
      return .5*sqrtf(db);
    }
  }

  // solution is the circumcircle (see GGems 4 p.144)
  Pnt3 aperp = cross(a, cross(a,b));
  float fact = dot(b,c) / dot(aperp, c);
  v[0] = A.v[0] + .5 * (a.v[0] + fact * aperp.v[0]);
  v[1] = A.v[1] + .5 * (a.v[1] + fact * aperp.v[1]);
  v[2] = A.v[2] + .5 * (a.v[2] + fact * aperp.v[2]);
  return .5 * sqrtf(da*fact*fact+da);
}



// is the ball centered at b with radius r
// fully within the box centered at bc, with radius br?
inline bool
ball_within_bounds(const Pnt3 &b, float r,
		   const Pnt3 &bc, float br)
{
  r -= br;
  if ((b.v[0] - bc.v[0] <= r) ||
      (bc.v[0] - b.v[0] <= r) ||
      (b.v[1] - bc.v[1] <= r) ||
      (bc.v[1] - b.v[1] <= r) ||
      (b.v[2] - bc.v[2] <= r) ||
      (bc.v[2] - b.v[2] <= r)) return false;
  return true;
}


// is the ball centered at b with radius r
// fully within the box centered from min to max?
inline bool
ball_within_bounds(const Pnt3 &b,
		   float r,
		   const Pnt3 &min,
		   const Pnt3 &max)
{
  if ((b.v[0] - min.v[0] <= r) ||
      (max.v[0] - b.v[0] <= r) ||
      (b.v[1] - min.v[1] <= r) ||
      (max.v[1] - b.v[1] <= r) ||
      (b.v[2] - min.v[2] <= r) ||
      (max.v[2] - b.v[2] <= r)) return false;
  return true;
}


// does the ball centered at b, with radius r,
// intersect the box centered at bc, with radius br?
inline bool
bounds_overlap_ball(const Pnt3 &b, float r,
		    const Pnt3 &bc, float br)
{
  float sum = 0.0, tmp;
  if        ((tmp = bc.v[0]-br - b.v[0]) > 0.0) {
    if (tmp>r) return false; sum += tmp*tmp;
  } else if ((tmp = b.v[0] - (bc.v[0]+br)) > 0.0) {
    if (tmp>r) return false; sum += tmp*tmp;
  }
  if        ((tmp = bc.v[1]-br - b.v[1]) > 0.0) {
    if (tmp>r) return false; sum += tmp*tmp;
  } else if ((tmp = b.v[1] - (bc.v[1]+br)) > 0.0) {
    if (tmp>r) return false; sum += tmp*tmp;
  }
  if        ((tmp = bc.v[2]-br - b.v[2]) > 0.0) {
    if (tmp>r) return false; sum += tmp*tmp;
  } else if ((tmp = b.v[2] - (bc.v[2]+br)) > 0.0) {
    if (tmp>r) return false; sum += tmp*tmp;
  }
  return (sum < r*r);
}


inline bool
bounds_overlap_ball(const Pnt3 &b,
		    float r,
		    const Pnt3 &min,
		    const Pnt3 &max)
{
  float sum = 0.0, tmp;
  if        (b.v[0] < min.v[0]) {
    tmp = min.v[0] - b.v[0]; if (tmp>r) return false; sum+=tmp*tmp;
  } else if (b.v[0] > max.v[0]) {
    tmp = b.v[0] - max.v[0]; if (tmp>r) return false; sum+=tmp*tmp;
  }
  if        (b.v[1] < min.v[1]) {
    tmp = min.v[1] - b.v[1]; sum+=tmp*tmp;
  } else if (b.v[1] > max.v[1]) {
    tmp = b.v[1] - max.v[1]; sum+=tmp*tmp;
  }
  r *= r;
  if (sum > r) return false;
  if        (b.v[2] < min.v[2]) {
    tmp = min.v[2] - b.v[2]; sum+=tmp*tmp;
  } else if (b.v[2] > max.v[2]) {
    tmp = b.v[2] - max.v[2]; sum+=tmp*tmp;
  }
  return (sum < r);
}



// this function tests for the intersection
// of two spheres, for one of which we have the
// squared radius
inline bool
spheres_intersect(const Pnt3 &c1, const Pnt3 &c2,
		  float r1sqr, float r2)
{
  float x = c1.v[0]-c2.v[0];
  float y = c1.v[1]-c2.v[1];
  float z = c1.v[2]-c2.v[2];

  // try to avoid square root
  //float r2sqr = r2*r2;
  //float R2 = (x*x + y*y + z*z - r1sqr - r2sqr)*.5;
  float R2 = (x*x + y*y + z*z - r1sqr - r2*r2);
  // check against underestimate
  if (R2 < 0.0) return true;

  /*
  // check against overestimate
  //R2 *= .5;
  if (r1sqr > r2sqr) {
    if (R2 > r1sqr) return false;
  } else {
    if (R2 > r2sqr) return false;
  }
  */
  // had to take square root...
  return (R2 < 2.0*sqrtf(r1sqr)*r2);
}


//GGems II, p. 142
inline bool
lines_intersect(const Pnt3 &p1,
		const Pnt3 &p2,
		const Pnt3 &p3,
		const Pnt3 &p4,
		Pnt3 &isect)
{
  Pnt3 a = p2 - p1;
  Pnt3 b = p4 - p3;
  Pnt3 axb = cross(a,b);
  float d2 = axb.norm2();
  if (d2 < 1.e-7) return false;
  isect = p1 + a * dot(axb, cross((p3-p1),b)) / d2;
  return true;
}


inline Pnt3 &
Pnt3::xform(float m[16])
{
  float x = m[0]*v[0] + m[4]*v[1] + m[8] *v[2] + m[12];
  float y = m[1]*v[0] + m[5]*v[1] + m[9] *v[2] + m[13];
  v[2]    = m[2]*v[0] + m[6]*v[1] + m[10]*v[2] + m[14];
  v[0] = x; v[1] = y;
  return *this;
}


inline Pnt3 &
Pnt3::xform(double m[16])
{
  float x = m[0]*v[0] + m[4]*v[1] + m[8] *v[2] + m[12];
  float y = m[1]*v[0] + m[5]*v[1] + m[9] *v[2] + m[13];
  v[2]    = m[2]*v[0] + m[6]*v[1] + m[10]*v[2] + m[14];
  v[0] = x; v[1] = y;
  return *this;
}


inline Pnt3 &
Pnt3::xform(float r[3][3], float t[3])
{
  float x = t[0] + r[0][0]*v[0] + r[0][1]*v[1] + r[0][2]*v[2];
  float y = t[1] + r[1][0]*v[0] + r[1][1]*v[1] + r[1][2]*v[2];
  v[2]    = t[2] + r[2][0]*v[0] + r[2][1]*v[1] + r[2][2]*v[2];
  v[0] = x; v[1] = y;
  return *this;
}

inline Pnt3 &
Pnt3::xform(float r[3][3], double t[3])
{
  float x = t[0] + r[0][0]*v[0] + r[0][1]*v[1] + r[0][2]*v[2];
  float y = t[1] + r[1][0]*v[0] + r[1][1]*v[1] + r[1][2]*v[2];
  v[2]    = t[2] + r[2][0]*v[0] + r[2][1]*v[1] + r[2][2]*v[2];
  v[0] = x; v[1] = y;
  return *this;
}

inline Pnt3 &
Pnt3::invxform(float m[16])
{
  float tx = v[0] - m[12];
  float ty = v[1] - m[13];
  float tz = v[2] - m[14];
  v[0] = m[0]*tx + m[1]*ty + m[2]*tz;
  v[1] = m[4]*tx + m[5]*ty + m[6]*tz;
  v[2] = m[8]*tx + m[9]*ty + m[10]*tz;
  return *this;
}

inline Pnt3 &
Pnt3::invxform(double m[16])
{
  double tx = v[0] - m[12];
  double ty = v[1] - m[13];
  double tz = v[2] - m[14];
  v[0] = m[0]*tx + m[1]*ty + m[2]*tz;
  v[1] = m[4]*tx + m[5]*ty + m[6]*tz;
  v[2] = m[8]*tx + m[9]*ty + m[10]*tz;
  return *this;
}

inline Pnt3 &
Pnt3::invxform(float r[3][3], float t[3])
{
  float tx = v[0] - t[0];
  float ty = v[1] - t[1];
  float tz = v[2] - t[2];
  v[0] = r[0][0]*tx + r[1][0]*ty + r[2][0]*tz;
  v[1] = r[0][1]*tx + r[1][1]*ty + r[2][1]*tz;
  v[2] = r[0][2]*tx + r[1][2]*ty + r[2][2]*tz;
  return *this;
}

inline Pnt3 &
Pnt3::invxform(float r[3][3], double t[3])
{
  float tx = v[0] - t[0];
  float ty = v[1] - t[1];
  float tz = v[2] - t[2];
  v[0] = r[0][0]*tx + r[1][0]*ty + r[2][0]*tz;
  v[1] = r[0][1]*tx + r[1][1]*ty + r[2][1]*tz;
  v[2] = r[0][2]*tx + r[1][2]*ty + r[2][2]*tz;
  return *this;
}

inline Pnt3 &
Pnt3::setXformed(const Pnt3 &p, float r[3][3], float t[3])
{
  v[0] = r[0][0]*p.v[0] + r[0][1]*p.v[1] + r[0][2]*p.v[2] + t[0];
  v[1] = r[1][0]*p.v[0] + r[1][1]*p.v[1] + r[1][2]*p.v[2] + t[1];
  v[2] = r[2][0]*p.v[0] + r[2][1]*p.v[1] + r[2][2]*p.v[2] + t[2];
  return *this;
}

inline Pnt3 &
Pnt3::setRotated(const Pnt3 &p, float r[3][3])
{
  v[0] = r[0][0]*p.v[0] + r[0][1]*p.v[1] + r[0][2]*p.v[2];
  v[1] = r[1][0]*p.v[0] + r[1][1]*p.v[1] + r[1][2]*p.v[2];
  v[2] = r[2][0]*p.v[0] + r[2][1]*p.v[1] + r[2][2]*p.v[2];
  return *this;
}

class Vec3 : public Pnt3 {
public:
  Vec3(float a=0.0, float b=0.0, float c=0.0)
    { v[0] = a, v[1] = b, v[2] = c; normalize(); }
  Vec3(const Pnt3 &p)
    { v[0] = p[0]; v[1] = p[1]; v[2] = p[2]; normalize(); }
  Vec3 &xform(float m[16]); // OpenGL model matrix
  Vec3 &xform(float r[3][3]);
};

inline Vec3 &
Vec3::xform(float m[16])
{
  float x = m[0]*v[0] + m[4]*v[1] + m[8] *v[2];
  float y = m[1]*v[0] + m[5]*v[1] + m[9] *v[2];
  v[2]    = m[2]*v[0] + m[6]*v[1] + m[10]*v[2];
  v[0] = x; v[1] = y;
  return *this;
}

inline Vec3 &
Vec3::xform(float r[3][3])
{
  float x = r[0][0]*v[0] + r[0][1]*v[1] + r[0][2]*v[2];
  float y = r[1][0]*v[0] + r[1][1]*v[1] + r[1][2]*v[2];
  v[2]    = r[2][0]*v[0] + r[2][1]*v[1] + r[2][2]*v[2];
  v[0] = x; v[1] = y;
  return *this;
}

#endif




#ifdef PNT3_MAIN

#include <iostream>
#define SHOW(X) cout << #X " = " << X << endl

void
main(void)
{
  Pnt3 a;
  Pnt3 b(1,1,1);
  SHOW(a);
  SHOW(b);

  SHOW(dist2_lineseg(a, a, b));
  SHOW(dist2_lineseg(b, a, b));
  SHOW(dist2_lineseg(.5*(a+b), a, b));
  SHOW(dist2_lineseg(a+Pnt3(1,0,0), a, b));
  SHOW(dist2_lineseg(b+Pnt3(1,0,0), a, b));
  SHOW(dist2_lineseg(2*b, a, b));
  SHOW(dist2_lineseg( -b, a, b));

  Pnt3 c(1,0,0);
  SHOW(c);

  SHOW(dist2_tri(a, a,b,c));
  SHOW(dist2_tri(b, a,b,c));
  SHOW(dist2_tri(c, a,b,c));
  SHOW(dist2_tri((a+b+c)/3.0, a,b,c));
  SHOW(dist2_tri(Pnt3(0,-1,1), a,b,c));
  SHOW(dist2_tri((a+b+c)/3.0 + 2*Pnt3(0,-1,1), a,b,c));
}

#endif
