#include <tcl.h>
#ifdef WIN32
#	include "winGLdecs.h"
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <algorithm>
#include <vector>
#include "plvDraw.h"
#include "plvScene.h"
#include "plvGlobals.h"

#ifdef no_overlay_support
#include "plvDrawCmds.h" // overlay planes work around
#endif

#include "accpersp.h"
#include "togl.h"
#include "RigidScan.h"
#include "DisplayMesh.h"
#include "DrawObj.h"
#include "plvClipBoxCmds.h"
#include "BailDetector.h"
#include "plvAnalyze.h"
#include "defines.h"
#include "TclCmdUtils.h"


static void drawCenterOfRotation();
static bool drawMeshes (bool bDrawAnnotations = true);
static bool drawShadowedMeshes();
static bool setupViewing (bool bFirstPass);
static void setupLighting();
static void setupShading();
static void initRenderParams();


static const int NUM_POWERS = 100;
static const int NUM_POWER_ARGS = 5000;
static float *thePowers;
static int hiliteColor = -1;


DrawObjects draw_other_things;


bool
initDrawing()
{
  float camera[3] = { 0, 0, 5 };
  float object[3] = { 0, 0, 0 };
  float up[3] = { 0, 1, 0 };
  tbView->setup (camera, object, up, 10);
  tbView->setOrthographic (false);

  theScene->centerCamera();

  initRenderParams();

  thePowers = new float[NUM_POWERS*NUM_POWER_ARGS];
  for (int i = 0; i < NUM_POWERS; i++) {
    for (int j = 0; j < NUM_POWER_ARGS; j++) {
      thePowers[j + i*NUM_POWER_ARGS] =
	pow(float(j)/(NUM_POWER_ARGS-1), i);
    }
  }

  return true;
}


bool
initDrawingPostCreation()
{
  if (toglCurrent != NULL) {
    initSelection (toglCurrent);

    hiliteColor = Togl_AllocColorOverlay (toglCurrent, 0, 1, 1);
    Togl_ShowOverlay (toglCurrent);
  }

  glLightModelfv (GL_LIGHT_MODEL_AMBIENT, theRenderParams->lightAmbient);
  return true;
}


static void
initRenderParams()
{
    theRenderParams = new RenderParams;
    theRenderParams->polyMode = GL_FILL;
    theRenderParams->hiddenLine = false;
    theRenderParams->shadeModel = perVertex;
    theRenderParams->cull = false;
    theRenderParams->useTstrips = false;
    theRenderParams->boundSelection = true;

    theRenderParams->light = true;
    theRenderParams->colorMode = grayColor;
    theRenderParams->useEmissive = false;
    theRenderParams->twoSidedLighting = false;

    // taken care of by two sided lighting
    // kberg - 6/5/01
    //theRenderParams->backFaceEmissive = false;

    theRenderParams->backfaceMode = lit;

    theRenderParams->confScale = 1;

    theRenderParams->pointSize = 1.0;
    theRenderParams->lineWidth = 1.0;

    theRenderParams->blend = 0;

    theRenderParams->shininess = 20;

    theRenderParams->specular[0] = 128;
    theRenderParams->specular[1] = 128;
    theRenderParams->specular[2] = 128;
    theRenderParams->specular[3] = 255;

    theRenderParams->diffuse[0] = 178; // .7 * 255
    theRenderParams->diffuse[1] = 178;
    theRenderParams->diffuse[2] = 178;
    theRenderParams->diffuse[3] = 255;

    /* added by kberg 6/5/01
       paramters used when two sided lighting is turne don
    */
    theRenderParams->backDiffuse[0] = 25;
    theRenderParams->backDiffuse[1] = 25;
    theRenderParams->backDiffuse[2] = 25;
    theRenderParams->backDiffuse[3] = 255;

    theRenderParams->backSpecular[0] = 18;
    theRenderParams->backSpecular[1] = 18;
    theRenderParams->backSpecular[2] = 18;
    theRenderParams->backSpecular[3] = 255;

    theRenderParams->backShininess = 20;

    theRenderParams->background[0] = 0;
    theRenderParams->background[1] = 0;
    theRenderParams->background[2] = 0;
    theRenderParams->background[3] = 255;

    theRenderParams->lightPosition[0] = 0.0;
    theRenderParams->lightPosition[1] = 0.0;
    theRenderParams->lightPosition[2] = 1.0;

    theRenderParams->lightAmbient[0] = 0.0;
    theRenderParams->lightAmbient[1] = 0.0;
    theRenderParams->lightAmbient[2] = 0.0;
    theRenderParams->lightAmbient[3] = 1.0;

    theRenderParams->lightDiffuse[0] = 1.0;
    theRenderParams->lightDiffuse[1] = 1.0;
    theRenderParams->lightDiffuse[2] = 1.0;
    theRenderParams->lightDiffuse[3] = 1.0;

    theRenderParams->lightSpecular[0] = 1.0;
    theRenderParams->lightSpecular[1] = 1.0;
    theRenderParams->lightSpecular[2] = 1.0;
    theRenderParams->lightSpecular[3] = 1.0;

    theRenderParams->shadows = false;
    theRenderParams->antiAlias = false;
    theRenderParams->numAntiAliasSamps = 8;
    theRenderParams->jitterX = 0;
    theRenderParams->jitterY = 0;
    theRenderParams->jitterArray = (jitter_point *)j8;
    theRenderParams->dofJitterX = 0;
    theRenderParams->dofJitterY = 0;
    theRenderParams->dofCenter = 0.5; // ratio between near and far
    theRenderParams->shadowLength = 0.05;
    theRenderParams->fromLightPOV = false;

    theRenderParams->flipnorm = false;
    theRenderParams->clearBeforeRender = true;
    theRenderParams->accelerateWithBbox = true;

    theRenderParams->bRenderManipsPoints = true;
    theRenderParams->bRenderManipsTinyPoints = false;
    theRenderParams->bRenderManipsUnlit = false;
    theRenderParams->bRenderManipsLores = false;
    theRenderParams->bRenderManipsSkipDlist = true;
    theRenderParams->iFastManipsThreshold = 0;

#ifdef no_overlay_support
    // RGBA being read back in
    theRenderParams->savedImage = new char[4 * theWidth * theHeight];

    theRenderParams->savedImageWidth = theWidth;
    theRenderParams->savedImageHeight = theHeight;
#endif
}


static bool
setupViewing (bool bFirstPass)
{
  struct RenderParams* gRP = theRenderParams; // for convenience

  // Viewport
  int width = theWidth;
  int height = theHeight;
  if (theResCap > 0) {
    width = min (theResCap, width);
    height = min (theResCap, height);
  }

  glViewport((theWidth - width) / 2, (theHeight - height) / 2, width, height);

  // Prepare projection
  tbView->applyProjection();

  // if antialiasing, have to jitter projection matrix
  if (gRP->antiAlias) {
    if (!tbView->getOrthographic()) {
      // perspective
      // rebuild projection matrix with jitter code
      float left, right, top, bottom, znear, zfar;
      tbView->getFrustum (left, right, top, bottom, znear, zfar);

      // zFocus = lerp (dofCenter, znear, zfar)
      float zFocus = gRP->dofCenter * zfar + (1 - gRP->dofCenter) * znear;
      // this doesn't actually work; I think to get true depth of field
      // you have to call accFrustum a bunch of times as depth of what you
      // draw changes?  I can get this to draw things blurry as the scene
      // approaches the camera, but a) not as they recede from the camera,
      // and b) it draws the whole scene with the same blurriness, regardless
      // of how far individual objects are from camera.

      accFrustum (left, right, top, bottom, znear, zfar,
		  gRP->jitterX, gRP->jitterY,
		  gRP->jitterX * gRP->dofJitterX,
		  gRP->jitterY * gRP->dofJitterY,
		  zFocus);
    } else {
      // orthographic
      // have to modify projection matrix with jitter code
      float heightAngle, aspectRatio, nearDistance, farDistance;
      tbView->getProjection(heightAngle,
			    aspectRatio,
			    nearDistance,
			    farDistance);

      float cameraHeight = tbView->getOrthoHeight();
      glTranslatef(gRP->jitterX * cameraHeight * aspectRatio / theWidth,
		   gRP->jitterY * cameraHeight / theHeight,
		   0.0);
    }
  }

  // Prepare viewing orientation/trans
  if (bFirstPass)    tbView->applyXform();   // apply spin()
  else               tbView->reapplyXform(); // no spin

  return true;
}



static void
setupLighting()
{
  if (theRenderParams->light) {
    // Enable lighting
    glEnable (GL_LIGHTING);
    glEnable (GL_LIGHT0);

    glMatrixMode (GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // if rendering with "flipped" normals, actually flip light dir
    if (theRenderParams->flipnorm)
      glRotatef (180, 0, 1, 0);

    glLightfv (GL_LIGHT0, GL_POSITION, theRenderParams->lightPosition);
    glLightfv (GL_LIGHT0, GL_AMBIENT, theRenderParams->lightAmbient);
    glLightfv (GL_LIGHT0, GL_DIFFUSE, theRenderParams->lightDiffuse);
    glLightfv (GL_LIGHT0, GL_SPECULAR, theRenderParams->lightSpecular);

    glPopMatrix();
  } else {
    glDisable (GL_LIGHTING);
  }
}



static void
setupShading()
{
  if (theRenderParams->shadeModel == perVertex)
    glShadeModel(GL_SMOOTH);
  else
    glShadeModel(GL_FLAT);

  //this is for the old pseudo-hidden line draw mode --
  //draw front faces outline and back faces solid black
  //glPolygonMode(GL_FRONT, theRenderParams->polyMode);

  // depending on whether normals are flipped, specify the orientation
  // of the faces - removed from if (...->cull) by kberg 7/5/01 so that
  // flipping normals did not cause nothing to be drawn when 2 sided lighting
  // was selected
  glFrontFace (theRenderParams->flipnorm ? GL_CW : GL_CCW);

  // don't cull back faces if two sided lighting is enabled
  if (theRenderParams->cull && !theRenderParams->twoSidedLighting) {
    glEnable (GL_CULL_FACE);
  } else {
    glDisable (GL_CULL_FACE);
  }

  //two sided lighting will take care of this
  //glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, theRenderParams->backFaceEmissive);
  glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, theRenderParams->twoSidedLighting);
}


bool
setupSceneDrawing()
{
  theScene->computeBBox (Scene::flush);

  // Prepare buffer
  glDepthFunc(GL_LESS);
  glEnable(GL_DEPTH_TEST);

  // Lighting
  setupLighting();
  setupShading();

  // Viewing
  setupViewing (true);

  // clear buffer
  if (theRenderParams->clearBeforeRender) {
    glClearColor(theRenderParams->background[0] / 255.,
		 theRenderParams->background[1] / 255.,
		 theRenderParams->background[2] / 255.,
		 theRenderParams->background[3] / 255.);
  }

  // bbox-clipping for this render pass?
  theRenderParams->accelerateWithBbox = GetTclGlobalBool ("noclip");

  if (theRenderParams->accelerateWithBbox) {
    // if the whole scene is onscreen, bbox clipping for individual
    // meshes is a waste of time.
    ScreenBox* filter = new ScreenBox (NULL,
			     0, Togl_Width(toglCurrent) - 1,
			     0, Togl_Height(toglCurrent) - 1);
    if (filter->acceptFully (theScene->worldBbox())) {
      //cout << "Whole scene onscreen; no need to clip test" << endl;
      theRenderParams->accelerateWithBbox = false;
    }
    delete filter;
  }

  return true;
}


bool
drawScene()
{
  if (!setupSceneDrawing())
    return false;

  if (theRenderParams->antiAlias) {

    glClear(GL_ACCUM_BUFFER_BIT);
    for (int i = 0; i < theRenderParams->numAntiAliasSamps; i++) {
      printf("\rRender pass %d of %d...", i+1,
	     theRenderParams->numAntiAliasSamps);
      fflush(stdout);

      theRenderParams->jitterX = theRenderParams->jitterArray[i].x;
      theRenderParams->jitterY = theRenderParams->jitterArray[i].y;

      // Viewing
      setupViewing (i == 0);
      drawShadowedMeshes();

      glAccum(GL_ACCUM, 1.0/theRenderParams->numAntiAliasSamps);
      if (GetTclGlobalBool ("aaswap"))
	Togl_SwapBuffers (toglCurrent);
    }

    glAccum(GL_RETURN, 1.0);
    printf("Antialiased render (%d passes) complete.\n", i);

  } else {

    drawShadowedMeshes();

  }

  if (!(   GetTclGlobalBool ("highQualSingle")
	&& GetTclGlobalBool ("highQualHideBbox"))) {

    if (theScene->wantMeshBBox()) {
      drawBoundingBox (theScene->worldBbox());
    }

    // bugnote, do this last b/c it hoses render modes
    drawCenterOfRotation();
  }

#ifdef no_overlay_support
  // only allocate new memory if the window has been resized
  if (theRenderParams->savedImageWidth != theWidth ||
      theRenderParams->savedImageHeight != theHeight) {
    delete [] theRenderParams->savedImage;

    theRenderParams->savedImageWidth = theWidth;
    theRenderParams->savedImageHeight = theHeight;

    theRenderParams->savedImage = new char[theWidth*theHeight*4];
  }

  glReadBuffer(GL_BACK);
  glReadPixels(0,0,theWidth, theHeight, GL_RGBA, GL_UNSIGNED_BYTE,
	       theRenderParams->savedImage);

  drawOverlay(toglCurrent);
#endif

  return true;
}


static void
initTextureParmsForShadowMap(void)
{
#ifdef GL_SGIX_shadow
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  glTexParameterf(GL_TEXTURE_2D, GL_SHADOW_AMBIENT_SGIX, 0.3);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_OPERATOR_SGIX,
		  GL_TEXTURE_LEQUAL_R_SGIX);

  // Enable texgen to get texture-coods (x, y, z, w) at every point.These
  // texture co-ordinates are then transformed by the texture matrix.
  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

  GLfloat p[4];
  p[0] = 1.0;
  p[1] = p[2] = p[3] = 0.0;
  glTexGenfv(GL_S, GL_OBJECT_PLANE, p);

  p[0] = 0.0;
  p[1] = 1.0;
  p[2] = p[3] = 0.0;
  glTexGenfv(GL_T, GL_OBJECT_PLANE, p);

  p[0] = p[1] = 0.0;
  p[2] = 1.0, p[3] = 0.0;
  glTexGenfv(GL_R, GL_OBJECT_PLANE, p);

  p[0] = p[1] = p[2] = 0.0;
  p[3] = 1.0;
  glTexGenfv(GL_Q, GL_OBJECT_PLANE, p);

  glEnable(GL_TEXTURE_GEN_S);
  glEnable(GL_TEXTURE_GEN_T);
  glEnable(GL_TEXTURE_GEN_R);
  glEnable(GL_TEXTURE_GEN_Q);
#endif
}

static float lightview_mat[16] = {0};
static float lightview_proj_mat[16] = {0};

static bool
drawMeshesFromLightView (void)
{
  // may need to render to back buffer to get info about where to position
  // "light-cam" and have relevant area onscreen
  bool bReadFromFront = true;
  if (theRenderParams->fromLightPOV
      || !GetTclGlobalBool ("shadowFastPass")) {
    drawMeshes();
    bReadFromFront = false;
  }

  int vp[4];
  glGetIntegerv(GL_VIEWPORT, vp);
  int curWidth = vp[2], curHeight = vp[3];

  // first, calculate desired light angle:
  Pnt3 current (0, 0, 1);
  Pnt3 wanted (theRenderParams->lightPosition);

  // if we're antialiasing, also jitter light for soft shadows
  if (theRenderParams->antiAlias) {
    Pnt3 jitter (theRenderParams->jitterX, theRenderParams->jitterY, 0);
    jitter *= theRenderParams->shadowLength;
    wanted += jitter;
  }

  // and rotation center:
  // first get camera parms
  float sceneCenter[3]; tbView->getRotationCenter (sceneCenter);
  float cameraCenter[3]; tbView->getCameraCenter (cameraCenter);
  float viewRot[4][4], viewTrans[3];
  tbView->getXform(viewRot,viewTrans);

  // try to get rotation center from center of screen instead of scene bbox
  Pnt3 objectCenter;
  if (!findZBufferNeighborExt (curWidth / 2, curHeight / 2, objectCenter,
			       toglCurrent, tbView, 100,
			       NULL, false, bReadFromFront)) {
    objectCenter = sceneCenter;
  }
  if (GetTclGlobalBool ("shadowHackCenter", "false"))
    objectCenter = sceneCenter;

  // get corners, sides while we're at it
  vector<Pnt3> cornerPos;
  for (int i = 0; i < 8; i++) {
    int x = (i & 1) ? 0 : curWidth - 1;
    int y = (i & 2) ? 0 : curHeight - 1;
    if (i >= 4) {
      if (x) {
	x /= 2;
      } else {
	if (y)
	  x = curWidth - 1;
	y = curHeight / 2;
      }
    }
    Pnt3 corner;
    if (findZBufferNeighborExt (x, y, corner, toglCurrent, tbView, 100,
				NULL, false, bReadFromFront)) {
      //cerr << "accept screen corner " << x << ", " << y
      //   << ": " << corner << endl;
      cornerPos.push_back (corner);
    }
  }
  // and toss in any corners of the scene bbox that fall onscreen
  ScreenBox screen (NULL, 0, curWidth-1, 0, curHeight-1);
  const Bbox& bb = theScene->worldBbox();
  for (i = 0; i < 8; i++) {
    if (screen.accept (bb.corner(i))) {
      cornerPos.push_back (bb.corner (i));
      //cerr << "accept bb corner " << i << endl;
    }
  }

  // now start setting matrices the way we need them
  glMatrixMode (GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  // rotate to light direction
  Pnt3 axis = cross (current, wanted);
  if (axis.norm2() > 0.0) {
    float angle = dot (current, wanted);
    angle = -180 / M_PI * acos (angle);
    if (!GetTclGlobalBool ("shadowHackRotate", false))
      glRotatef (angle, axis[0], axis[1], axis[2]);
  }

  // apply viewing xform (rotation only) so we're looking at correct face
  glMultMatrixf (&viewRot[0][0]);

  // translate back to object coords
  glTranslatef (-objectCenter[0], -objectCenter[1], -objectCenter[2]);

  // set up orthographic projection (since directional light)
  glMatrixMode (GL_PROJECTION);
  float oldProj[16];
  glGetFloatv (GL_PROJECTION_MATRIX, oldProj);

  float dummy, aspect, znear, zfar;
  tbView->getProjection(dummy, aspect, znear, zfar);
  float orthoHeight = tbView->getOrthoHeight();
  float halfScene = theScene->sceneRadius(); // conservative
  glLoadIdentity();
  glOrtho(-orthoHeight/2*aspect, orthoHeight/2*aspect,
	  -orthoHeight/2, orthoHeight/2,
     	  -halfScene, +halfScene);
  glGetFloatv (GL_PROJECTION_MATRIX, lightview_proj_mat);

  glMatrixMode (GL_MODELVIEW);
  // zoom scene (part rendered from camera view) so that it fits onscreen
  // and try to get it to fill screen, since shadow resolution is
  // screen-space dependent
  float shadScale = atof (GetTclGlobal ("shadowZoom", "1.0"));

  // and now we want to move the scene so the old viewport bbox is still
  // centered and fills the new viewport.
  Projector proj;
  int x1 = 1e6, x2 = -1e6, y1 = 1e6, y2 = -1e6;
  for (i = 0; i < cornerPos.size(); i++) {
    cornerPos[i] = proj (cornerPos[i]);
    x1 = MIN (x1, cornerPos[i][0]);
    x2 = MAX (x2, cornerPos[i][0]);
    y1 = MIN (y1, cornerPos[i][1]);
    y2 = MAX (y2, cornerPos[i][1]);
  }
  int dx = x2 - x1; int dy = y2 - y1;

  int tx = (curWidth - (x1 + x2)) / 2;
  int ty = (curHeight - (y1 + y2)) / 2;

  float scale = MIN ((float)curWidth / dx, (float)curHeight / dy);
  scale *= .9; // slightly conservative
  scale *= shadScale;
  cerr << "Using " << scale
       << " as shadow scale factor; set shadowZoom to change" << endl;

  // apply scale, translate on right
  glGetFloatv (GL_MODELVIEW_MATRIX, lightview_mat);
  glLoadIdentity();
  glScalef (scale, scale, 1.0); // z=1 b/c ortho, so doesn't matter, and
				// we don't want to hose clipping planes
  if (!GetTclGlobalBool ("shadowHackTranslate", false)) {
    glTranslatef (tx * scale, ty * scale, 0);
  }
  glMultMatrixf (lightview_mat);

  // save for retexturing
  glGetFloatv (GL_MODELVIEW_MATRIX, lightview_mat);

  bool ret = drawMeshes (false);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity();
  glMultMatrixf (oldProj);
  glMatrixMode (GL_MODELVIEW);
  glPopMatrix();

  return ret;
}


static void dumpGLerror (void)
{
  while (int a = glGetError()) {
    cerr << "GL error: " << a << endl << flush;
  }
}


static bool
generateShadowMap (void)
{
#ifdef GL_SGIX_shadow
  int x, y;
  GLfloat log2 = log(2.0);

  x = 1 << ((int) (log((float) theWidth) / log2));
  y = 1 << ((int) (log((float) theHeight) / log2));
  glViewport(0, 0, x, y);

  initTextureParmsForShadowMap();
  glEnable (GL_POLYGON_OFFSET_FILL);
  char* of1 = GetTclGlobal ("shadowOffset1", "3");
  char* of2 = GetTclGlobal ("shadowOffset2", "0");
  glPolygonOffset (atof (of1), atof (of2));
  bool ret = drawMeshesFromLightView();
  glDisable (GL_POLYGON_OFFSET_FILL);

  if (ret) {
    // Read in frame-buffer into a depth texture map
    // do NOT remove the glError calls (either one) or it hoses the pipe!
    // really the problem is some context switching/synchronization error
    // look at /usr/adm/crash/diag/gfx/IR/*.rslt for fun
    // but really make sure not to run gr_osview while running this.'
    // calling glGetError() causes it to occur much less frequently,
    // but doesn't stop it entirely.
    dumpGLerror();
    //cerr << "extracting texture:" << endl;
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16_SGIX,
		     0, 0, x, y, 0);
    dumpGLerror();
  }

  glViewport(0, 0, theWidth, theHeight);
  return ret;
#endif
}


bool
drawShadowedMeshes (void)
{
#ifdef GL_SGIX_shadow
  if (theRenderParams->fromLightPOV) {

    return drawMeshesFromLightView();

  } else if (theRenderParams->shadows) {

    // algorithm from Siggraph 99 course notes, OpenGL advanced
    // techniques, section 11.4.3: shadow maps
    // also Tom McReynolds' shadowmap.c sample
    if (!generateShadowMap())
      return false;

    // Now render the normal scene using projective textures to get the depth
    // value from the light's point of view into the r-cood of the texture.
    glEnable(GL_TEXTURE_2D);

    glMatrixMode (GL_TEXTURE);
    glLoadIdentity();
    glTranslatef (0.5, 0.5, 0.4994); // magic numbers?
    glScalef (0.5, 0.5, 0.5);
    glMultMatrixf (lightview_proj_mat);
    glMultMatrixf (lightview_mat);
    glMatrixMode (GL_MODELVIEW);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_SGIX, GL_TRUE);
    bool ret = drawMeshes();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_SGIX, GL_FALSE);
    glDisable (GL_TEXTURE_2D);
    return ret;

  } else {

    return drawMeshes();

  }
#else
    return drawMeshes();
#endif
}


bool
drawSceneIndexColored (int scale = 1)
{
  PushRenderParams();
  theRenderParams->polyMode = GL_FILL;
  theRenderParams->hiddenLine = false;
  theRenderParams->shadeModel = fakePerFace;
  theRenderParams->colorMode = noColor;
  theRenderParams->light = false;
  theRenderParams->useEmissive = false;
  theRenderParams->antiAlias = false;
  theRenderParams->boundSelection = false;
  theRenderParams->blend = false;
  theRenderParams->diffuse[0] = 0;
  theRenderParams->diffuse[1] = 0;
  theRenderParams->diffuse[2] = 0;
  theRenderParams->diffuse[3] = 255;
  theRenderParams->background[0] = 0;
  theRenderParams->background[1] = 0;
  theRenderParams->background[2] = 0;
  theRenderParams->background[3] = 255;

  setupSceneDrawing();
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  for (int i = 0; i < theScene->meshSets.size(); i++) {
    // encode mesh index as 3 floats, suitable for representation in
    // quantized 15-bit (or better) frame buffer.  Allows 32767 meshes.
    int is = (i + 1) * scale;
    uchar r = is % 32;
    uchar g = (is / 32) % 32;
    uchar b = is / 1024;

    theRenderParams->diffuse[0] = r * 8;
    theRenderParams->diffuse[1] = g * 8;
    theRenderParams->diffuse[2] = b * 8;

    // sanity check to make sure packing/unpacking works right
    int i2 = b * 1024 + g * 32 + r;
    i2 /= scale;
    assert (i2 == i + 1);

    theScene->meshSets[i]->drawSelf (false);
  }

  PopRenderParams();

  return true;
}


bool
drawSceneThicknessColored (void)
{
  // draw once normally...
  if (!setupSceneDrawing())
    return false;
  PushRenderParams();
  theRenderParams->boundSelection = false;
  drawMeshes();

  // save the near z-buffer
  vector<Pnt3> nearPts, farPts;
  Selection screen;
  screen.type = Selection::rect;
  screen.pts.push_back (Selection::Pt (0, 0));
  screen.pts.push_back (Selection::Pt (0, theHeight));
  screen.pts.push_back (Selection::Pt (theWidth, theHeight));
  screen.pts.push_back (Selection::Pt (theWidth, 0));
  if (!ReadZBufferArea (nearPts, screen, true, false)) {
    PopRenderParams();
    return false;
  }

  // then clear everything, and draw again with reversed depth test
  glClearDepth (0);
  setupSceneDrawing();
  glClearDepth (1);
  glDepthFunc (GL_GREATER);
  drawMeshes();

  // save the far z-buffer
  if (!ReadZBufferArea (farPts, screen, true, false, true)) {
    PopRenderParams();
    return false;
  }

  // create a nice visualization of this
  vector<unsigned int> depth;
  depth.reserve (theWidth * theHeight);
  for (int y = 0; y < theHeight; y++) {
    int yofs = y * theWidth;
    for (int x = 0; x < theWidth; x++) {
      float nz = nearPts [yofs + x] [2];
      float fz = farPts [yofs + x] [2];
      if (nz != 0 && fz != 0) {
	depth.push_back (((255 - (int)MIN (255, (nz-fz) * 10)) << 16)
			 + 65535 + (255<<24));
	//printf ("%f %f %x\n", nz, fz, depth.back());
      } else {
	depth.push_back (0);
      }
    }
  }

  // and blit it to screen.
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity();
  glDisable (GL_DEPTH_TEST);
  glRasterPos2f (-1, -1);
// STL Update
  glDrawPixels (theWidth, theHeight, GL_RGBA, GL_UNSIGNED_BYTE, &*depth.begin());
  while (int err = glGetError()) {
    cerr << "Error: " << err << endl;
  }

  PopRenderParams();
  return true;
}


bool
drawScanHilites (void)
{
  char* which = GetTclGlobal ("showScanHilites", "");
  if (!strcmp (which, "never"))
    return false;

  bool bInvis = (0 == strcmp (which, "always"));

  int nToHilite = g_hilitedScans.size();
  if (nToHilite) {
    setupViewing (false);
#ifndef no_overlay_support
    glIndexi (hiliteColor);
#else
    glColor3f(0,1,1);
#endif

    for (int i = 0; i < nToHilite; i++) {

      if (bInvis || g_hilitedScans[i]->getVisible()) {
	glMatrixMode (GL_MODELVIEW);
	glPushMatrix();
	g_hilitedScans[i]->getMeshData()->gl_xform();
	drawBoundingBox (g_hilitedScans[i]->getMeshData()->localBbox(),
			 false);
	glPopMatrix();
      }
    }
  }

  return true;
}


static void
drawCenterOfRotation()
{
  if (g_tclInterp != NULL) {
    if (GetTclGlobalBool ("rotCenterVisible")) {
      //show center of rotation
      float redcolor[3] = { .5, 0, 0 };
      Pnt3 center;
      glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, redcolor);

      if (theActiveScan == NULL) {
	tbView->getRotationCenter (center);
      } else {
	center = theActiveScan->getMeshData()->worldCenter();
      }

      glTranslatef(center[0],center[1], center[2]);
      if (GetTclGlobalBool ("rotCenterOnTop"))
	glDisable (GL_DEPTH_TEST);
      GLUquadric* quad;
      quad = gluNewQuadric();
      gluSphere (quad, theScene->sceneRadius() / 100, 10, 10);
      gluDeleteQuadric (quad);
    }
  }
}


static bool
drawMeshes (bool bDrawAnnotations)
{
  int k;
  int nMeshes = theScene->meshSets.size();
  BailDetector bail;

  // clear before rendering
  if (theRenderParams->clearBeforeRender)
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // ensure that bounding box is up-to-date
  theScene->computeBBox (Scene::flush);

  if (GetTclGlobalBool ("renderOnlyActiveMesh")) {
    theSelectedScan->drawSelf();
  } else {
    // two passes, draw opaque meshes then transparent ones
    for (k = 0; k < nMeshes; k++) {

      if (!theScene->meshSets[k]->transparent()) {
	theScene->meshSets[k]->drawSelf();
	//int g = 255 * ++i / nMeshes;
	//sprintf (cmd, "redrawStatus #%02x%02x00", 255 - g, g);
	//Tcl_Eval (g_tclInterp, cmd);

	if (bail())
	  break;
      }
    }

    for (k = 0; k < nMeshes; k++) {

      if (theScene->meshSets[k]->transparent()) {
	theScene->meshSets[k]->drawSelf();
	//int g = 255 * ++i / nMeshes;
	//sprintf (cmd, "redrawStatus #%02x%02x00", 255 - g, g);
	//Tcl_Eval (g_tclInterp, cmd);

	if (bail())
	  break;
      }
    }
  }

  if (bDrawAnnotations) {
    draw_other_things();
  }

  return true;
}


static void
glvtx(const Pnt3 &p)
{
  glVertex3fv(&p[0]);
}


void
drawBoundingBox (const Bbox& box, bool bUseDefaultColor)
{
  vector<Pnt3> bbox_edges(24);

  // don't draw the bounding box if the max is less than the min
  if (box.max()[0] < box.min()[0] &&
      box.max()[1] < box.min()[1] &&
      box.max()[2] < box.min()[2])
    return;

  if (bUseDefaultColor)
    glColor3f (1, 0, 0);

  glPushAttrib (GL_LIGHTING_BIT);
  glDisable (GL_LIGHTING);
  box.edgeList(bbox_edges);
  glBegin(GL_LINES);
  for_each(bbox_edges.begin(), bbox_edges.end(), glvtx);
  glEnd();
  glPopAttrib();
}


static vector<RenderParams> renderParamsStack;

void
PushRenderParams (void)
{
  renderParamsStack.push_back (*theRenderParams);
}


void
PopRenderParams (void)
{
  assert (renderParamsStack.size() > 0);
  *theRenderParams = renderParamsStack.back();
  renderParamsStack.pop_back();
}




#if 0  // texture code inactive

#ifdef WIN32
#undef PARALLEL //revisit this someday?  I just did it to get
                //this to compile... -- magi
#else
#include <ulocks.h>
#include <task.h>
#include <unistd.h>
#define PARALLEL
#endif

#include "Xform.h"

void texLightDiff(MeshSet *meshSet);
void texLightAll(MeshSet *meshSet);


void
texLightDiff(MeshSet *meshSet)
{
   int nx, ny, nz;
   int color;

#ifdef PARALLEL
   int myid = m_get_myid();
   int processes = m_get_numprocs();
#else
   int myid = 0;
   int processes = 1;
#endif

   int start = (meshSet->texXdim * meshSet->texYdim)/processes * myid;
   int stop = (meshSet->texXdim * meshSet->texYdim)/processes * (myid+1);

   float lx = theRenderParams->lightPosition[0];
   float ly = theRenderParams->lightPosition[1];
   float lz = theRenderParams->lightPosition[2];

   Xform<float> rotxf;
   float q[4], t[3];
   tbView->getXform(q,t);
   rotxf.rotQ(q[0], q[1], q[2], q[3]);
   rotxf = rotxf * meshSet->getXform();

   Pnt3 light(lx,ly,lz), newLight;
   rotxf.apply(light, newLight);

   lx = newLight[0]*theRenderParams->diffuse[0]*2;
   ly = newLight[1]*theRenderParams->diffuse[0]*2;
   lz = newLight[2]*theRenderParams->diffuse[0]*2;

   uchar *buf = meshSet->texture + start*3;
   uchar *newBuf = meshSet->newTexture + start;
   float diff;

   for (int i=start; i < stop; i++) {
      nx = int(*buf++ - 128);
      ny = int(*buf++ - 128);
      nz = int(*buf++ - 128);

      diff = nx*lx + ny*ly + nz*lz;
      if (diff > 0) {
	 color = uchar(diff);
	 if (color < 0)
	    *newBuf++ = 0;
	 else if (color > 255)
	    *newBuf++=  255;
	 else
	    *newBuf++ = color;
      }
      else {
	 *newBuf++ = 0;
      }
   }
}


void
texLightAll(MeshSet *meshSet)
{
  int nx, ny, nz;
  int color;

#ifdef PARALLEL
  int myid = m_get_myid();
  int processes = m_get_numprocs();
#else
  int myid = 0;
  int processes = 1;
#endif

  Xform<float> rotxf;
  float q[4], t[3];
  tbView->getXform(q,t);
  rotxf.rotQ(q[0], q[1], q[2], q[3]);
  rotxf = rotxf * meshSet->getXform();

  int start = (meshSet->texXdim * meshSet->texYdim)/processes * myid;
  int stop = (meshSet->texXdim * meshSet->texYdim)/processes * (myid+1);

  float lx = theRenderParams->lightPosition[0];
  float ly = theRenderParams->lightPosition[1];
  float lz = theRenderParams->lightPosition[2];

  Pnt3 light(lx,ly,lz), newLight;
  rotxf.apply(light, newLight);
  lx = newLight[0];
  ly = newLight[1];
  lz = newLight[2];

  Pnt3 viewer(0,0,1), newViewer;
  rotxf.apply(viewer, newViewer);

  float hx = lx + newViewer[0];
  float hy = ly + newViewer[1];
  float hz = lz + newViewer[2];

  float hnorm = (NUM_POWER_ARGS-1)/sqrt(hx*hx + hy*hy + hz*hz)/127;
  hx *= hnorm;
  hy *= hnorm;
  hz *= hnorm;

  uchar *buf = meshSet->texture + start*3;
  uchar *newBuf = meshSet->newTexture + start;
  float diff, spec, hn, diffFactor, specFactor;
  float *powerPtr;

  specFactor = theRenderParams->specular[0] * 350;
  diffFactor = 2 * theRenderParams->diffuse[0];

  if (theRenderParams->shininess < NUM_POWERS) {
    powerPtr = thePowers +
      int(theRenderParams->shininess) * NUM_POWER_ARGS;
  }
  else {
    powerPtr = thePowers + (NUM_POWER_ARGS-1)*NUM_POWERS;
  }

  lx *= diffFactor;
  ly *= diffFactor;
  lz *= diffFactor;

  for (int i=start; i < stop; i++) {
    nx = int(*buf++ - 128);
    ny = int(*buf++ - 128);
    nz = int(*buf++ - 128);

    diff = nx*lx + ny*ly + nz*lz;
    if (diff > 0) {
      hn = nx*hx + ny*hy + nz*hz;
      if (hn > 0) {
	//spec = pow(hn, theRenderParams->shininess) *
	//   specFactor;
	if (hn >= NUM_POWER_ARGS)
	  spec = specFactor;
	else {
	  spec = *(powerPtr + int(hn)) * specFactor;
	}
      } else {
	spec = 0;
      }

      color = uchar(diff + spec);
      if      (color < 0)	*newBuf++ = 0;
      else if (color > 255)	*newBuf++ = 255;
      else	                *newBuf++ = color;
    } else {
      *newBuf++ = 0;
    }
  }
}

#endif // texture code
