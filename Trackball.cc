//############################################################
// Trackball.cc
// Kari Pulli
// 03/06/97
//############################################################

#include <iostream>
#include <math.h>
#include "Trackball.h"
#include "Xform.h"
#include "Projector.h"
#include "defines.h"

#include "plvGlobals.h" // for spinning preference...

/*
 * Trackball code:
 *
 * Implementation of a virtual trackball.
 * Implemented by Gavin Bell, lots of ideas from Thant Tessman and
 *   the August '88 issue of Siggraph's "Computer Graphics,"
 *   pp. 121-129.
 *
 * Vector manip code:
 *
 * Original code from:
 * David M. Ciemiewicz, Mark Grossman, Henry Moreton, and
 * Paul Haeberli
 *
 * Much mucking with by:
 * Gavin Bell
 * Kept most maths, changed just about everything else:
 * Kari Pulli
 */


/*
   This size should really be based on the distance from the
   center of rotation to the point on the object underneath the
   mouse.  That point would then track the mouse as closely as
   possible.  This is a simple example, though, so that is left
   as an Exercise for the Programmer.
*/
#define TRACKBALLSIZE  (0.8)


// Local function prototypes (not defined in trackball.h)
static float tb_project_to_sphere(float, float, float);
static void normalize_quat(float [4]);


//  Given an axis and angle, compute quaternion.
void
axis_to_quat(Pnt3 &a, float phi, float q[4])
{
  a.normalize();
  a *= sin(phi/2.0);
  q[1]=a[0]; q[2]=a[1]; q[3]=a[2];
  q[0] = cos(phi/2.0);
}

/*
   Ok, simulate a track-ball.  Project the points onto the virtual
   trackball, then figure out the axis of rotation, which is the
   cross product of P1 P2 and O P1 (O is the center of the ball,
   0,0,0)
   Note:  This is a deformed trackball-- is a trackball in the
   center, but is deformed into a hyperbolic sheet of rotation
   away from the center.  This particular function was chosen
   after trying out several variations.

   It is assumed that the arguments to this routine are in the
   range (-1.0 ... 1.0)
 */
void
trackball(float q[4], float p1x, float p1y, float p2x, float p2y,
	  TbConstraint constraint = CONSTRAIN_NONE)
{
  if (p1x == p2x && p1y == p2y) {
    // Zero rotation
    q[0] = 1.0;
    q[1] = q[2] = q[3] = 0.0;
    return;
  }

  // First, figure out z-coordinates for projection of P1 and P2
  // to deformed sphere
  Pnt3 p1(p1x,p1y,tb_project_to_sphere(TRACKBALLSIZE,p1x,p1y));
  Pnt3 p2(p2x,p2y,tb_project_to_sphere(TRACKBALLSIZE,p2x,p2y));

  // get axis of rotation
  Pnt3 a = cross(p1,p2);

  // Figure out how much to rotate around that axis.
  float t = dist(p1,p2) / (2.0*TRACKBALLSIZE);

  // Avoid problems with out-of-control values...
  if (t > 1.0) t = 1.0;
  if (t < -1.0) t = -1.0;
  float phi = 2.0 * asin(t); // how much to rotate about axis

  if (constraint == CONSTRAIN_X)
    a[1] = a[2] = 0;
  if (constraint == CONSTRAIN_Y)
    a[0] = a[2] = 0;
  if (constraint == CONSTRAIN_Z)
    a[0] = a[1] = 0;

  axis_to_quat(a,phi,q);
}

//
// Project an x,y pair onto a sphere of radius r OR a hyperbolic
// sheet if we are away from the center of the sphere.
//
static float
tb_project_to_sphere(float r, float x, float y)
{
  float d, t, z;

  d = sqrt(x*x + y*y);
  if (d < r * 0.70710678118654752440) {    // Inside sphere
    z = sqrt(r*r - d*d);
  } else {                                 // On hyperbola
    t = r / 1.41421356237309504880;
    z = t*t / d;
  }
  return z;
}

/*
   Given two rotations, e1 and e2, expressed as quaternion
   rotations, figure out the equivalent single rotation and stuff
   it into dest.
   This routine also normalizes the result every RENORMCOUNT
   times it is called, to keep error from creeping in.

   NOTE: This routine is written so that q1 or q2 may be the same
   as dest (or each other).
*/

#define RENORMCOUNT 97

void
add_quats(float q1[4], float q2[4], float dest[4])
{
  static int count=0;

  Pnt3 t1(&q1[1]);
  t1 *= q2[0];

  Pnt3 t2(&q2[1]);
  t2 *= q1[0];

  Pnt3 t3 = cross(&q1[1],&q2[1]);
  t3 += t1; t3 += t2;

  // watch out: q1 or q2 may be the same as dest!
  dest[0] = q1[0] * q2[0] - dot(&q1[1],&q2[1]);
  dest[1] = t3[0];
  dest[2] = t3[1];
  dest[3] = t3[2];

  if (++count > RENORMCOUNT) {
    count = 0;
    normalize_quat(dest);
  }
}

/*
   Quaternions always obey:  a^2 + b^2 + c^2 + d^2 = 1.0
   If they don't add up to 1.0, dividing by their magnitude will
   renormalize them.

   Note: See the following for more information on quaternions:

   - Shoemake, K., Animating rotation with quaternion curves,
     Computer Graphics 19, No 3 (Proc. SIGGRAPH'85), 245-254,1985.
     - Pletinckx, D., Quaternion calculus as a basic tool in
       computer graphics, The Visual Computer 5, 2-13, 1989.
*/
static void
normalize_quat(float q[4])
{
  float mag = (q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
  for (int i = 0; i < 4; i++) q[i] /= mag;
}

// Build a rotation matrix, given a quaternion rotation.
void
build_rotmatrix(float m[4][4], float q[4])
{
  m[0][0] = 1.0 - 2.0 * (q[2] * q[2] + q[3] * q[3]);
  m[1][0] = 2.0 * (q[1] * q[2] - q[3] * q[0]);
  m[2][0] = 2.0 * (q[3] * q[1] + q[2] * q[0]);
  m[3][0] = 0.0;

  m[0][1] = 2.0 * (q[1] * q[2] + q[3] * q[0]);
  m[1][1] = 1.0 - 2.0 * (q[3] * q[3] + q[1] * q[1]);
  m[2][1] = 2.0 * (q[2] * q[3] - q[1] * q[0]);
  m[3][1] = 0.0;

  m[0][2] = 2.0 * (q[3] * q[1] - q[2] * q[0]);
  m[1][2] = 2.0 * (q[2] * q[3] + q[1] * q[0]);
  m[2][2] = 1.0 - 2.0 * (q[2] * q[2] + q[1] * q[1]);
  m[3][2] = 0.0;

  m[0][3] = 0.0;
  m[1][3] = 0.0;
  m[2][3] = 0.0;
  m[3][3] = 1.0;
}


Trackball::Trackball(void)
  : spinning_enabled(true),
    rotating(0), spinning(0), panning(0), zooming(0),
    orthographic(false),
    lastTarget(NULL)
{
  trackball(currQuat, 0.0, 0.0, 0.0, 0.0);
  setTransScale();
}


void
Trackball::setSize(int w, int h)
{
  W=w; H=h;
}


// c  is camera center,
// o  is wherethe camera is looking at,
// up is the up vector of the camera
void
Trackball::setup(float _c[3], float _o[3], float _up[3],
		 float r, float fov, float oblique_x, float oblique_y)
{
  o = Pnt3(_o);
  c = Pnt3(_c);
  up = Pnt3(_up);

  radius = r;
  field_of_view = fov;
  oblique_camera_offset_x = oblique_x;
  oblique_camera_offset_y = oblique_y;

  resetXform();
  resetAxes();
}


void
Trackball::resetXform(void)
{
  trans = Pnt3();
  rotating = spinning = panning = zooming = 0;
  constraint = CONSTRAIN_NONE;
  //bUndoable = false;

  trackball(currQuat, 0.0, 0.0, 0.0, 0.0);
  trackball(lastQuat, 0.0, 0.0, 0.0, 0.0);
}


void
Trackball::newRotationCenter(float _o[3], TbObj* target)
{
  Pnt3 newCtr = Pnt3(_o);
  float distmoved = dist(newCtr, o);
  if (!distmoved)
    return;

  // radius used for clipping planes
  radius += distmoved;

  if (target) {
    target->new_rotation_center(newCtr);
    return;
  }

  Xform<float> newTrans;
  newTrans.translate (trans);
  newTrans.translate (o);
  newTrans.rotQ (-currQuat[0], currQuat[1], currQuat[2], currQuat[3]);
  newTrans.translate (-o);
  newTrans.translate (newCtr);
  newTrans.rotQ (currQuat[0], currQuat[1], currQuat[2], currQuat[3]);
  newTrans.translate (-newCtr);

  newTrans.getTranslation (trans);
  Pnt3 noBasisTrans;
  float tempQ[4];
  getXform (tempQ, noBasisTrans);

  o = newCtr;
  resetAxes();

  Xform<float> newBasis;
  newBasis.fromFrame (u, v, z, Pnt3());
  newBasis.apply (noBasisTrans, trans);
}


void
Trackball::changeFOV(float fov)
{
  field_of_view = fov;
}


float
Trackball::getFOV (void)
{
  return field_of_view;
}


void
Trackball::setObliqueCamera(float x, float y)
{
  oblique_camera_offset_x = x;
  oblique_camera_offset_y = y;
}

void
Trackball::resetAxes(void)
{
  // z is from the object to camera
  z = c - o;
  d = z.norm();
  z /= d;
  // u (x vector) is up X z
  u = cross(up,z);
  u.normalize();
  // v (y vector) is z X u
  v = cross(z,u);

  //clipping planes based on camera and object
  setClippingPlanes (o, radius);
}

void
Trackball::setClippingPlanes(Pnt3 bb_ctr, float bb_r)
{
  // calculate the radius around o
  radius = bb_r + dist(bb_ctr, o);

  // calculate the distance from the camera to center of rotation,
  // down the z axis -- d = distance from o to c along z, then subtract
  // out the translation (also projected onto z axis)
  float d_oc = d - dot (trans, z);
  zfar  = d_oc+radius;
  znear = zfar - 2*radius;

  enforceZlimits();
}


void
Trackball::pressButton(int button, int up, int x, int y, int t,
		       TbObj* target)
{
  spinning_enabled = atoi (Tcl_GetVar (g_tclInterp, "spinningAllowed",
				       TCL_GLOBAL_ONLY));
  lastTarget = target;
  y = (H-1)-y;

  if (!panning && !spinning && !zooming) {
    // new interaction... save state for undo
    //saveUndoPos (target);
    target->save_for_undo();
  }

  if (button == 1) {
    if (up == 0) {
      if (panning) {
	zooming = 1;
	panning = 0;
      } else {
	rotating = 1;
	spinning = 0;
      }
    } else {
      assert(up == 1);
      rotating = 0;
      if (zooming) {
	panning = 1;
	zooming = 0;

	// just stopped zooming.
	// clipping planes changed, so translation scale needs to change
	resetTransScale();
      }
    }
  } else {
    assert(button == 2);
    if (up == 0) {
      if (rotating) {
	zooming = 1;
	rotating  = 0;
      } else
	panning = 1;
    } else {
      assert(up == 1);
      panning = 0;
      spinning = 0;
      if (zooming) {
	rotating = 1;
	zooming = 0;
      }
    }
  }

  // check if mouse was moving as button was released --
  // compare timestamps
  // on this event and last event;
  // if there was any significant delay
  // between the release and the last mousemove, then don't spin.
  if (up) {
    float mousedist = (x-beginx)*(x-beginx) + (y-beginy)*(y-beginy);
    if (mousedist < .2*(t-begint)) {
      // mouse moving really slow: turn off spin on release
      //printf ("event age: %u\n", t - begint);
      //fflush (stdout);
      spinning = 0;      // unless they were still moving
    }
  }

  beginx = x;
  beginy = y;
  begint = t;

#if 0 //debug
  cout << "press button" << button << (up?" up":" down") << endl;
  SHOW(zooming);
  SHOW(panning);
  SHOW(rotating);
  SHOW(spinning);
  SHOW(beginx);
  SHOW(beginy);
#endif
}


void
Trackball::move(int x, int y, int t, TbObj *target)
{
  //if (target)  cout << "move obj" << endl;
  //else         cout << "move cam" << endl;

  y = (H-1)-y;

  if (rotating) {
    trackball(lastQuat,
	      2.0 * beginx / W - 1.0,
	      2.0 * beginy / H - 1.0,
	      2.0 * x / W - 1.0,
	      2.0 * y / H - 1.0,
	      constraint);
    /*
    float phi = 2*asin(lastQuat[0]);
    float mag = Pnt3(lastQuat+1).norm();
    cout << "lastQuat "
	 << 2*asin(lastQuat[0]) << " "
	 << lastQuat[1]/mag << " "
	 << lastQuat[2]/mag << " "
	 << lastQuat[3]/mag << endl;
    */
    // let spin() apply the rotation
    if (target) spin();
    spinning = 1;
  } else {
    Pnt3 dt;
    if (panning) {
      float factor = transScale ? transScale : ((d-trans[2])/W);
      dt[0] = factor*(x-beginx);
      dt[1] = factor*(y-beginy);
    }

    if (zooming) {
      float focalDistance = fabs(d - dot (trans, z));
      if (orthographic) {
	dt[2] = (beginy-y) * focalDistance / 400;
	orthoHeight -= dt[2];
	if (orthoHeight <= 1.e-6)
	  orthoHeight = 1.e-6;
      } else {
	float newFocalDist = focalDistance * powf(2.0,
						  (y-beginy)/80.0);
	dt[2] = (focalDistance - newFocalDist);
      }

      if (target == NULL) {
	// moving the camera
	zfar  -= dt[2];
	znear = zfar - 2*radius;
	enforceZlimits();
      }
    }

    translateInEyeCoords (dt, target, false);
  }

  beginx = x;
  beginy = y;
  begint = t;

#if 0  //debug
  cout << "move" << endl;
  SHOW(zooming);
  SHOW(panning);
  SHOW(rotating);
  SHOW(spinning);
  SHOW(beginx);
  SHOW(beginy);
#endif
}


void
Trackball::getXform (float quat[4], float t[3])
{
  for (int i = 0; i < 4; i++)
    quat[i] = currQuat[i];

  t[0] = dot (trans, u);
  t[1] = dot (trans, v);
  t[2] = dot (trans, z);
}

void
Trackball::getXform(float m[4][4], float t[3])
{
  float q[4];

  getXform (q, t);
  build_rotmatrix(m, q);
}

void
Trackball::applyXform(int bReinitMatrix)
{
  spin();
  reapplyXform (bReinitMatrix);
}


void
Trackball::reapplyXform (int bReinitMatrix)
{
  float m[4][4], t[3];
  getXform(m,t);
  if (bReinitMatrix) {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
  }
  glTranslatef(-c[0], -c[1], -c[2]); // move the camera
  glTranslatef(t[0], t[1], t[2]);    // pan and zoom
  glTranslatef(o[0], o[1], o[2]);    // rotate around origin
  glMultMatrixf(&m[0][0]);
  glTranslatef(-o[0], -o[1], -o[2]);

#if 0
  if (this != &tbView)
    //  return;
    cout << "mesh -- ";
  else
    cout << "view -- ";
  cout << "applyXform:\nCamera is at ";
  int i;
  for (i = 0; i < 3; i++) cout << c[i] << "\t";
  cout << endl;
  cout << "translated by ";
  for (i = 0; i < 3; i++) cout << t[i] << "\t";
  cout << endl;
  cout << "Object is at ";
  for (i = 0; i < 3; i++) cout << o[i] << "\t";
  cout << endl;
  cout << "rotated by" << endl;
  for (i = 0; i < 4; i++)
    {
      for (int j = 0; j < 4; j++)
	cout << m[i][j] << "\t";
      cout << endl;
    }
  cout << endl;
#endif
}


void
Trackball::enable_spinning(void)
{
  spinning_enabled = true;
}


void
Trackball::disable_spinning(void)
{
  spinning_enabled = false;
}


void
Trackball::spin(void)
{
  if (rotating || isSpinning()) {
    if (lastTarget == NULL) {
      add_quats(lastQuat, currQuat, currQuat);
    } else {
      // undo view transform, apply inverse rotation,
      // redo view transform
      float q[4],cq[4],lq[4];
      cq[0] = -currQuat[0]; cq[1] = currQuat[1];
      cq[2] = currQuat[2]; cq[3] = currQuat[3];
      lq[0] = lastQuat[0]; lq[1] = lastQuat[1];
      lq[2] = lastQuat[2]; lq[3] = lastQuat[3];
      add_quats(cq,lq,q);
      cq[0] = -cq[0];
      add_quats(q,cq,q);
      lastTarget->rotate(q[0],q[1],q[2],q[3], false);
    }
  }
}

/*
void
Trackball::undo_last (void)
{
  if (!bUndoable)
    return;

  spinning = false;

  Xform<float> redo;
  if (lastTarget) {
    redo = lastTarget->getXform();
    lastTarget->setXform (undoPosition);
  } else {
    redo = getUndoXform();
    this->setUndoXform (undoPosition);
  }
  undoPosition = redo;
}
*/

bool
Trackball::isSpinning (void)
{
  if (!spinning_enabled) return false;
  return spinning && !rotating && !zooming && !panning;
}


bool
Trackball::isManipulating (void)
{
  return (spinning && spinning_enabled) || rotating || zooming || panning;
}


void
Trackball::stop (void)
{
  spinning = false;
}


// return the viewing direction
Pnt3
Trackball::dir(void)
{
  float m[4][4];
  build_rotmatrix(m, currQuat);
  return Pnt3(m[0][2],m[1][2],m[2][2]);
}


void
Trackball::getRotationCenter (float center[3])
{
  for (int i = 0; i < 3; i++)
    center[i] = o[i];
}


void
Trackball::getCameraCenter (float center[3])
{
  for (int i = 0; i < 3; i++)
    center[i] = c[i];
}


// rotate angle_rads radians around axis: axis can be any vector,
// normalization is not necessary

void
Trackball::rotateAroundAxis (const Pnt3& axis, float angle_rads,
			     TbObj* target)
{
  float q[4];
  Pnt3 a = axis;   // mungeable copy
  axis_to_quat (a, angle_rads, q);
  rotateQuat (q, target);
}


void
Trackball::rotateQuat (float q[4], TbObj* target)
{
  if (target == NULL) { // rotate viewer
    target->save_for_undo();
    add_quats (q, currQuat, currQuat);
  } else {
    // undo view transform, apply inverse rotation,
    // redo view transform
    float *c = currQuat;
    target->rotate(-c[0], c[1], c[2], c[3]);
    target->rotate(q[0], q[1], q[2], q[3], false);
    target->rotate(c[0], c[1], c[2], c[3], false);
  }
}


void
Trackball::constrainRotation (TbConstraint _constraint)
{
  constraint = _constraint;
}


void
Trackball::getBounds (float& left, float& right, float& bottom, float& top)
{
  float halfdiag = tan(0.5 * M_PI/180.0 * field_of_view) * znear;
  top = halfdiag * H / sqrt(W*W+H*H);
  bottom = -top;
  right = W/H * top;
  left = -right;

  right -= oblique_camera_offset_x * znear;
  left -= oblique_camera_offset_x * znear;
  top -= oblique_camera_offset_y * znear;
  bottom -= oblique_camera_offset_y * znear;
}


void
Trackball::getFrustum (float& left, float& right,
		       float& bottom, float& top,
		       float& _znear, float& _zfar)
{
  getBounds (left, right, bottom, top);
  _znear = znear;
  _zfar = zfar;
}


void
Trackball::setPerspXform(void)
{
  float left, right, bottom, top;
  getBounds (left, right, bottom, top);

  glFrustum(left, right, bottom, top, znear, zfar);
}


float
Trackball::getOrthoHeight(void)
{
  // if not in orthographic mode, value won't be up-to-date
  if (orthographic)
    return orthoHeight;
  else
    return sqrt (znear * zfar);
}


void
Trackball::setOrthoXform(void)
{
  float aspect = (float)W/H;

  glOrtho(-orthoHeight/2*aspect, orthoHeight/2*aspect,
	  -orthoHeight/2, orthoHeight/2,
	  znear, zfar);
}


void
Trackball::applyProjection()
{
  int mm;
  glGetIntegerv (GL_MATRIX_MODE, &mm);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  if (orthographic)
    setOrthoXform();
  else
    setPerspXform();

  glMatrixMode ((GLenum) mm);
}


void
Trackball::getProjection (float &pFov, float &pAspect,
			  float &pNear, float &pFar)
{
  pFov = field_of_view;
  pAspect = (float)W / H;
  pNear = znear;
  pFar = zfar;
}


void
Trackball::setOrthographic (bool bOrtho)
{
  if (bOrtho == orthographic)
    return;  // nothing to do

  orthographic = bOrtho;

  // need to set new camera params to a reasonable facsimile of old ones
  if (orthographic) {
    // going orthographic
    orthoHeight = sqrt (znear * zfar);
  } else {
    // going perspective
    //znear = orthoHeight - radius;
    //zfar = orthoHeight + radius;
  }
  setClippingPlanes (o, radius);
  enforceZlimits();
}


bool
Trackball::getOrthographic (void)
{
  return orthographic;
}


void
Trackball::enforceZlimits (void)
{
  // znear needs both absolute and relative caps -- absolute because it
  // has to be >0, relative because it needs to be within 24 bits of zfar
  // (actually, notably less than 24 bits) to maintain z-buffer resolution
  if (znear < 1.e-6)  znear = 1.e-6;
  if (znear < zfar / 1.e3)
    znear = zfar / 1.e3;
  if (zfar < znear) zfar  = znear + 1.0;
}


Xform<float>
Trackball::getUndoXform (void)
{
  Xform<float> xf;
  xf.fromQuaternion (currQuat, trans[0], trans[1], trans[2]);

  // need to save zoom...  here's a hack.
  // in the xf, members m[0][3], m[1][3], m[2][3] are unused.
  xf(3,0) = znear;
  xf(3,1) = zfar;

  return xf;
}


void
Trackball::setUndoXform (const Xform<float>& xf)
{
  // hack -- see comments under getUndoXform
  znear = xf(3,0);
  zfar =  xf(3,1);

  xf.toQuaternion (currQuat);
  xf.getTranslation (trans);
}


void
Trackball::translateInEyeCoords (Pnt3& dt, TbObj* target, bool undoable)
{
  if (target) {
    // moving an object
    // translate the mesh instead of camera
    Pnt3 tr;
    tr[0] = dot (dt, u);
    tr[1] = dot (dt, v);
    tr[2] = dot (dt, z);
    Xform<float> xf;
    xf.rotQ(-currQuat[0], currQuat[1], currQuat[2], currQuat[3]);
    xf(tr);
    target->translate(tr[0], tr[1], tr[2], undoable);
    // note that this translate should check the scene and
    // set up the correct clipping planes by calling
    // Trackball::setClippingPlanes(Pnt3 ctr, float r)
  } else {
    // moving the camera
    if (undoable)
      target->save_for_undo();
    trans += dt;
  }
}

/*
void
Trackball::saveUndoPos (TbObj* target)
{
  if (target)
    undoPosition = target->getXform();
  else
    undoPosition = this->getUndoXform();

  lastTarget = target;
  bUndoable = true;
}
*/


void
Trackball::getState (Pnt3& _c, Pnt3& _o, Pnt3& _t, float q[4],
  		     float& _fov, float &_oblique_x, float &_oblique_y)
{
  _c = c;
  _o = o;

  _t = trans;
  for (int i = 0; i < 4; i++)
    q[i] = currQuat[i];
  _fov = field_of_view;
  _oblique_x = oblique_camera_offset_x;
  _oblique_y = oblique_camera_offset_y;
}


void
Trackball::setState (Pnt3& _t, float q[4])
{
  for (int i = 0; i < 4; i++)
    currQuat[i] = q[i];

  trans = _t;
}


void
Trackball::zoomToRect (int x1, int y1, int x2, int y2)
{
  // need to translate (at focal point) in screen plane, such that
  // given rect goes to center of screen, then translate perpendicular
  // to screen plane to zoom in by ratio of area of rect to screen

  int rectW = x2 - x1;
  int rectH = y2 - y1;

  int cx = x1 + rectW / 2;
  int cy = y1 + rectH / 2;

  int dx = W / 2 - cx;
  int dy = H / 2 - cy;

  float zoom = MIN ((float)W / rectW, (float)H / rectH);

  // now need to translate such that in screen coords, we go (dx, dy, dz)
  // where dz gives the desired zoom factor.

  // copied from ::move:
  Pnt3 dt;

  float factor = transScale ? transScale: ((d-trans[2])/W);
  dt[0] = factor*(dx);
  dt[1] = factor*(dy);

  dt[2] = (d - trans[2]) * (1 - 1/zoom);

  // and go there!
  translateInEyeCoords (dt, NULL, true);
  setClippingPlanes (o, radius);
}


void
Trackball::setTransScale (const Pnt3& clickPt)
{
  if (clickPt[2] != 0.) {
    float zeye = znear * zfar / (clickPt[2] * (zfar - znear) - zfar);
    //cerr << "setting near-click-far = " << znear << " " << zeye << " "
    // << zfar << " (" << clickPt[2] << ")" << endl;

    // copied from Trackball::setPerspXform
    float halfdiag = tan(0.5 * M_PI/180.0 * field_of_view) * zeye;
    float top = halfdiag / sqrt(W*W + H*H);
    transScale = (-2 * top);

    // save point that we're using as translation reference, in world
    // coords, in case we need to reproject it
    Unprojector unp (this);
    transScaleReference = unp (clickPt[0], clickPt[1], clickPt[2]);
  } else {
    transScale = 0.;
    transScaleReference = Pnt3 (0, 0, 0);
  }
}


void
Trackball::resetTransScale (void)
{
  // recomputes transScale based on previous point passed to
  // setTransScale -- useful if clipping planes change, ie after zoom.

 if (transScaleReference[2]) {
   // it doesn't suffice to recompute the transScale using the new
   // znear/zfar and old zscale value, because the zscale value (read from
   // depth buffer) will be different for the same pixel with old and new
   // clipping planes.  Need to reproject it.
   Projector proj (this);
   Pnt3 rep = proj (transScaleReference);
   setTransScale (rep);
 }
}
