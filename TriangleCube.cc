//###############################################################
// TriangleCube.cc
// Kari Pulli
// Mon Nov 23 14:41:10 CET 1998
//
// Detect an intersection of a cube and a triangle
// Based on code from Graphics Gems III, Douglas Voorhies
//###############################################################

#include "Pnt3.h"

#define SIGN3( A ) (((A)[0]<0)?4:0 | ((A)[1]<0)?2:0 | ((A)[2]<0)?1:0)
#define MIN3(a,b,c) ((((a)<(b))&&((a)<(c))) ? (a) : (((b)<(c)) ? (b) : (c)))
#define MAX3(a,b,c) ((((a)>(b))&&((a)>(c))) ? (a) : (((b)>(c)) ? (b) : (c)))

/*___________________________________________________________________________*/

/* Which of the six face-plane(s) is point P outside of? */

static long
face_plane(const Pnt3 &p)
{
  long outcode = 0;
  if (p[0] >  .5) outcode |= 0x01;
  if (p[0] < -.5) outcode |= 0x02;
  if (p[1] >  .5) outcode |= 0x04;
  if (p[1] < -.5) outcode |= 0x08;
  if (p[2] >  .5) outcode |= 0x10;
  if (p[2] < -.5) outcode |= 0x20;
  return outcode;
}

/*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */

/* Which of the twelve edge plane(s) is point P outside of? */

static long
bevel_2d(const Pnt3 &p)
{
  long outcode = 0;
  if ( p[0] + p[1] > 1.0) outcode |= 0x001;
  if ( p[0] - p[1] > 1.0) outcode |= 0x002;
  if (-p[0] + p[1] > 1.0) outcode |= 0x004;
  if (-p[0] - p[1] > 1.0) outcode |= 0x008;
  if ( p[0] + p[2] > 1.0) outcode |= 0x010;
  if ( p[0] - p[2] > 1.0) outcode |= 0x020;
  if (-p[0] + p[2] > 1.0) outcode |= 0x040;
  if (-p[0] - p[2] > 1.0) outcode |= 0x080;
  if ( p[1] + p[2] > 1.0) outcode |= 0x100;
  if ( p[1] - p[2] > 1.0) outcode |= 0x200;
  if (-p[1] + p[2] > 1.0) outcode |= 0x400;
  if (-p[1] - p[2] > 1.0) outcode |= 0x800;
  return outcode;
}

/*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */

/* Which of the eight corner plane(s) is point P outside of? */

static long
bevel_3d(const Pnt3 &p)
{
  long outcode = 0;
  if (( p[0] + p[1] + p[2]) > 1.5) outcode |= 0x01;
  if (( p[0] + p[1] - p[2]) > 1.5) outcode |= 0x02;
  if (( p[0] - p[1] + p[2]) > 1.5) outcode |= 0x04;
  if (( p[0] - p[1] - p[2]) > 1.5) outcode |= 0x08;
  if ((-p[0] + p[1] + p[2]) > 1.5) outcode |= 0x10;
  if ((-p[0] + p[1] - p[2]) > 1.5) outcode |= 0x20;
  if ((-p[0] - p[1] + p[2]) > 1.5) outcode |= 0x40;
  if ((-p[0] - p[1] - p[2]) > 1.5) outcode |= 0x80;
  return outcode;
}

/*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */

/* Test the point "alpha" of the way from P1 to P2 */
/* See if it is on a face of the cube              */
/* Consider only faces in "mask"                   */

static bool
point_on_face(const Pnt3 &p1, const Pnt3 &p2, float alpha, long mask)
{
  Pnt3 plane_point;
  plane_point.lerp(alpha, p1, p2);
  return (face_plane(plane_point) & mask);
}

/*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */

/* Compute intersection of P1 --> P2 line segment with face planes */
/* Then test intersection point to see if it is on cube face       */
/* Consider only face planes in "outcode_diff"                     */
/* Note: Zero bits in "outcode_diff" means face line is outside of */

static bool
line_on_face(const Pnt3 &p1, const Pnt3 &p2, long outcode_diff)
{

  if (0x01 & outcode_diff)
    if (point_on_face(p1,p2,( .5-p1[0])/(p2[0]-p1[0]),0x3e)) return true;
  if (0x02 & outcode_diff)
    if (point_on_face(p1,p2,(-.5-p1[0])/(p2[0]-p1[0]),0x3d)) return true;
  if (0x04 & outcode_diff)
    if (point_on_face(p1,p2,( .5-p1[1])/(p2[1]-p1[1]),0x3b)) return true;
  if (0x08 & outcode_diff)
    if (point_on_face(p1,p2,(-.5-p1[1])/(p2[1]-p1[1]),0x37)) return true;
  if (0x10 & outcode_diff)
    if (point_on_face(p1,p2,( .5-p1[2])/(p2[2]-p1[2]),0x2f)) return true;
  if (0x20 & outcode_diff)
    if (point_on_face(p1,p2,(-.5-p1[2])/(p2[2]-p1[2]),0x1f)) return true;
  return false;
}

/*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */

/* Test if 3D point is inside 3D triangle */

static bool
point_triangle_intersection(const Pnt3 &p,
			    const Pnt3 &v1, const Pnt3 &v2, const Pnt3 &v3)
{
  /* First, a quick bounding-box test:                               */
  /* If P is outside triangle bbox, there cannot be an intersection. */

  if (p[0] > MAX3(v1[0], v2[0], v3[0])) return true;
  if (p[1] > MAX3(v1[1], v2[1], v3[1])) return true;
  if (p[2] > MAX3(v1[2], v2[2], v3[2])) return true;
  if (p[0] < MIN3(v1[0], v2[0], v3[0])) return true;
  if (p[1] < MIN3(v1[1], v2[1], v3[1])) return true;
  if (p[2] < MIN3(v1[2], v2[2], v3[2])) return true;

  /* For each triangle side, make a vector out of it by subtracting vertexes; */
  /* make another vector from one vertex to point P.                          */
  /* The crossproduct of these two vectors is orthogonal to both and the      */
  /* signs of its X,Y,Z components indicate whether P was to the inside or    */
  /* to the outside of this triangle side.                                    */

  long sign12 = SIGN3(cross(v2,p,v1)); // Extract X,Y,Z signs as 0...7 integer
  long sign23 = SIGN3(cross(v3,p,v2));
  long sign31 = SIGN3(cross(v1,p,v3));

  /* If all three crossproduct vectors agree in their component signs,  */
  /* then the point must be inside all three.                           */
  /* P cannot be OUTSIDE all three sides simultaneously.                */

  return ((sign12 != sign23) || (sign23 != sign31));
}

/*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */

//
// the main routine
// true if triangle t1,t2,t3 is outside a cube
// centered at c with length of a side s,
// false if the triangle intersects cube
//
bool
tri_outside_of_cube(const Pnt3 &c, float s,
		    const Pnt3 &t1, const Pnt3 &t2, const Pnt3 &t3)
{
  long v1_test,v2_test,v3_test;

  /* First compare all three vertexes with all six face-planes */
  /* If any vertex is inside the cube, return immediately!     */

  Pnt3 v1((t1[0]-c[0])/s, (t1[1]-c[1])/s, (t1[2]-c[2])/s);
  if (!(v1_test = face_plane(v1))) return false;
  Pnt3 v2((t2[0]-c[0])/s, (t2[1]-c[1])/s, (t2[2]-c[2])/s);
  if (!(v2_test = face_plane(v2))) return false;
  Pnt3 v3((t3[0]-c[0])/s, (t3[1]-c[1])/s, (t3[2]-c[2])/s);
  if (!(v3_test = face_plane(v3))) return false;

  /* If all three vertexes were outside of one or more face-planes, */
  /* return immediately with a trivial rejection!                   */

  if ((v1_test & v2_test & v3_test) != 0) return true;

  /* Now do the same trivial rejection test for the 12 edge planes */

  v1_test |= bevel_2d(v1) << 8;
  v2_test |= bevel_2d(v2) << 8;
  v3_test |= bevel_2d(v3) << 8;
  if ((v1_test & v2_test & v3_test) != 0) return true;

  /* Now do the same trivial rejection test for the 8 corner planes */

  v1_test |= bevel_3d(v1) << 24;
  v2_test |= bevel_3d(v2) << 24;
  v3_test |= bevel_3d(v3) << 24;
  if ((v1_test & v2_test & v3_test) != 0) return true;

  /* If vertex 1 and 2, as a pair, cannot be trivially rejected */
  /* by the above tests, then see if the v1-->v2 triangle edge  */
  /* intersects the cube.  Do the same for v1-->v3 and v2-->v3. */
  /* Pass to the intersection algorithm the "OR" of the outcode */
  /* bits, so that only those cube faces which are spanned by   */
  /* each triangle edge need be tested.                         */

  if ((v1_test & v2_test) == 0)
    if (line_on_face(v1,v2,v1_test|v2_test)) return false;
  if ((v1_test & v3_test) == 0)
    if (line_on_face(v1,v3,v1_test|v3_test)) return false;
  if ((v2_test & v3_test) == 0)
    if (line_on_face(v2,v3,v2_test|v3_test)) return false;

  /* By now, we know that the triangle is not off to any side,     */
  /* and that its sides do not penetrate the cube.  We must now    */
  /* test for the cube intersecting the interior of the triangle.  */
  /* We do this by looking for intersections between the cube      */
  /* diagonals and the triangle...first finding the intersection   */
  /* of the four diagonals with the plane of the triangle, and     */
  /* then if that intersection is inside the cube, pursuing        */
  /* whether the intersection point is inside the triangle itself. */

  /* To find plane of the triangle, first perform crossproduct on  */
  /* two triangle side vectors to compute the normal vector.       */

  Pnt3 norm = cross(v2,v3,v1);
  /* The normal vector "norm" X,Y,Z components are the coefficients */
  /* of the triangles AX + BY + CZ + D = 0 plane equation.  If we   */
  /* solve the plane equation for X=Y=Z (a diagonal), we get        */
  /* -D/(A+B+C) as a metric of the distance from cube center to the */
  /* diagonal/plane intersection.  If this is between -0.5 and 0.5, */
  /* the intersection is inside the cube.  If so, we continue by    */
  /* doing a point/triangle intersection.                           */
  /* Do this for all four diagonals.                                */

  float d = dot(norm, v1);

  Pnt3 hit;
  hit[0] = hit[1] = hit[2] = d / (norm[0] + norm[1] + norm[2]);
  if (fabsf(hit[0]) <= 0.5)
    if (point_triangle_intersection(hit,v1,v2,v3) == false)
      return false;
  hit[2] = -(hit[0] = hit[1] = d / (norm[0] + norm[1] - norm[2]));
  if (fabsf(hit[0]) <= 0.5)
    if (point_triangle_intersection(hit,v1,v2,v3) == false)
      return false;
  hit[1] = -(hit[0] = hit[2] = d / (norm[0] - norm[1] + norm[2]));
  if (fabsf(hit[0]) <= 0.5)
    if (point_triangle_intersection(hit,v1,v2,v3) == false)
      return false;
  hit[1] = hit[2] = -(hit[0] = d / (norm[0] - norm[1] - norm[2]));
  if (fabsf(hit[0]) <= 0.5)
    if (point_triangle_intersection(hit,v1,v2,v3) == false)
      return false;

  /* No edge touched the cube; no cube diagonal touched the triangle. */
  /* We're done...there was no intersection.                          */

  return true;
}


#ifdef TRICUBEMAIN

void
main(void)
{

  Pnt3 ctr(10,10,10);
  float s = 2;

  SHOW(ctr);
  SHOW(s);

  Pnt3 a(10.95, 10.95, 10.95);
  Pnt3 b;
  Pnt3 c(1,3,5);
  Pnt3 d(13.0001,10,10);
  Pnt3 e(10,13.0001,10);
  Pnt3 f(10,10,13.0001);
  Pnt3 g(10,10,12.99);

  SHOW(a);
  SHOW(b);
  SHOW(c);
  SHOW(d);
  SHOW(e);
  SHOW(f);
  SHOW(g);
  SHOW(tri_outside_of_cube(ctr,s,a,b,c));
  SHOW(tri_outside_of_cube(ctr,s,d,b,c));
  SHOW(tri_outside_of_cube(ctr,s,d,e,c));
  SHOW(tri_outside_of_cube(ctr,s,d,e,f));
  SHOW(tri_outside_of_cube(ctr,s,d,e,g));
  SHOW(tri_outside_of_cube(ctr,s,Pnt3(), Pnt3(4,5,6), Pnt3(6,7,8)));
}

#endif
