//############################################################
// Trackball.h
// Kari Pulli
// 10/22/95
//############################################################

#ifndef _Trackball_h_
#define _Trackball_h_

#ifdef WIN32
#	include "winGLdecs.h"
#endif
#include <GL/gl.h>
#include <GL/glu.h>

#include "TbObj.h"
#include "Xform.h"
#include "Pnt3.h"

typedef enum {
  CONSTRAIN_NONE, CONSTRAIN_X, CONSTRAIN_Y, CONSTRAIN_Z
} TbConstraint;

class Trackball {
private:
  float W; // window width
  float H; // window height
  float lastQuat[4], currQuat[4];
  int beginx;
  int beginy;
  int begint;
  int spinning;
  int panning;
  int zooming;
  int rotating;
  Pnt3 c,o,u,v,z,trans,up;
  float d;
  float znear, zfar, field_of_view, radius;
  float oblique_camera_offset_x, oblique_camera_offset_y;
  TbObj* lastTarget;
  TbConstraint constraint;
  bool  orthographic;
  float orthoHeight;
  bool  spinning_enabled;
  float transScale;
  Pnt3 transScaleReference;

  void getBounds (float& left, float& right, float& top, float& bottom);
  void setPerspXform(void);
  void setOrthoXform(void);

  void resetAxes(void);
  void enforceZlimits(void);
  void resetTransScale(void);

  //bool bUndoable;
  //Xform<float> undoPosition;

public:
  Trackball(void);

  void  setSize(int w, int h);
  void  setup(float c[3], float o[3], float up[3],
	      float r, float fov=45.0,
	      float oblique_x = 0, float oblique_y = 0);
  void  resetXform(void);
  void  newRotationCenter(float o[3], TbObj* target = NULL);
  void  changeFOV(float fov);
  float getFOV (void);
  void  setObliqueCamera(float x, float y);
  void  setClippingPlanes (Pnt3 bb_ctr, float bb_r);

  void  pressButton(int button, int up, int x, int y, int t,
		    TbObj* target = NULL);
  void  move(int x, int y, int t, TbObj* target = NULL);
  void  setTransScale (const Pnt3& clickPt = Pnt3(0,0,0));

  void  getXform(float m[4][4], float t[3]);
  void  getXform(float quat[4], float t[3]);
  void  applyXform(int bReinitMatrix = 1);
  void  reapplyXform(int bReinitMatrix = 1);

  void  getState (Pnt3& c, Pnt3& o, Pnt3& t, float q[4],
  		  float& fov, float &oblique_x, float &oblique_y);
  void  setState (Pnt3& t, float q[4]);

  void  enable_spinning(void);
  void  disable_spinning(void);
  void  spin(void);
  void  stop(void);
  bool  isSpinning (void);
  bool  isManipulating (void);

  bool  isRotating (void) const { return rotating; }
  bool  isPanning  (void) const { return panning; }
  bool  isZooming  (void) const { return zooming; }

  void  getProjection (float &pFov, float &pAsp_rat,
		       float &zNear, float &pFar);
  void  getFrustum (float& left, float& right,
		    float& top, float& bottom,
		    float& _znear, float& _zfar);
  void  applyProjection (void);
  void  setOrthographic (bool bOrtho);
  bool  getOrthographic (void);
  float getOrthoHeight (void);

  Pnt3  dir(void);
  void  getRotationCenter (float center[3]);
  void  getCameraCenter (float center[3]);

  void  constrainRotation (TbConstraint _constraint);
  void  rotateAroundAxis (const Pnt3& axis, float angle_rads,
			  TbObj* target = NULL);
  void  rotateQuat (float quat[4], TbObj* target = NULL);

  Xform<float> getUndoXform();           // only for use with setUndoXform
  void setUndoXform (const Xform<float>& xf);  // only for data from...

  void  translateInEyeCoords (Pnt3& trans, TbObj* target = NULL,
			      bool undoable = true);

  void  zoomToRect (int x1, int y1, int x2, int y2);
};



#endif //_Trackball_h_
