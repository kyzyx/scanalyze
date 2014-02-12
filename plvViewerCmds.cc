#include <iostream>
#include <stdlib.h>
#include <limits.h>
#include "plvGlobals.h"
#include "plvDraw.h"
#include "plvViewerCmds.h"
#include "plvMeshCmds.h"
#include "plvDrawCmds.h"
#include "togl.h"
#include "RigidScan.h"
#include "DisplayMesh.h"
#include "plvAnalyze.h"
#include "VolCarve.h"
#include "Timer.h"
#include "ToglCache.h"
#include "ScanFactory.h"
#include "plvClipBoxCmds.h"
#include "VertexFilter.h"
#include "cmdassert.h"
#include "TclCmdUtils.h"
#include "plvScene.h"
#include "GroupScan.h"


static bool s_bManipulatingLocked = false;
static bool s_bForceOnscreen = false;
static crope s_autoResetScript;
static int  s_iAutoResetTime;
static void rescheduleAutoReset (void);


static void lastActionTrackball (void)
{
  if (toglCurrent)
    Tcl_Eval (Togl_Interp (toglCurrent), "autoClearSelection");
}


int
PlvSelectScanCmd(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[])
{
  if (argc < 2) {
    interp->result = "Wrong number of args";
    return TCL_ERROR;
  }

  // if argv[1] == "", the viewer is active
  DisplayableMesh* scan = FindMeshDisplayInfo (argv[1]);

  if (strlen(argv[1]) && scan == NULL) {
    interp->result = "Could not find mesh";
    return TCL_ERROR;
  }

  DisplayableMesh* oldActiveScan = theActiveScan;
  if (argc > 2 && !strcmp (argv[2], "manipulate")) {
    theActiveScan = scan;
  } else {
    theSelectedScan = scan;
  }

  if (!g_bNoUI) {
    if (theActiveScan != oldActiveScan
	|| theRenderParams->colorMode == registrationColor) {
      redraw (true);
    }
  }

  return TCL_OK;
}

int
PlvSetVisibleCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{
  if (argc < 3) {
    interp->result = "Wrong number of args";
    return TCL_ERROR;
  }

  DisplayableMesh *meshSet = FindMeshDisplayInfo (argv[1]);

  if (meshSet == NULL) {
    interp->result = "Could not find mesh";
    return TCL_ERROR;
  }

  bool bVisible;
  SetBoolFromArgIndex (2, bVisible);
  meshSet->setVisible (bVisible);

  theScene->computeBBox();
  redraw (true);

  return TCL_OK;
}

int
PlvGetVisibleCmd(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[])
{
  if (argc < 2) {
    interp->result = "Wrong number of args";
    return TCL_ERROR;
  }

  DisplayableMesh *meshSet = FindMeshDisplayInfo (argv[1]);

  if (meshSet == NULL) {
    interp->result = "Could not find mesh";
    return TCL_ERROR;
  }

  interp->result = meshSet->getVisible() ? "1" : "0";
  return TCL_OK;
}

static void
PlvListScansHelper (Tcl_Interp *interp, vector<DisplayableMesh*> meshes, bool bLeaf);
// NEW! IMPROVED!!
int
PlvListScansCmd(ClientData clientData, Tcl_Interp *interp,
		int argc, char *argv[])
{
  // options:
  // default,roots = only meshes with no parent
  // leaves = all meshes with no children
  // groups = all groups (i.e. "meshes" with children)

  bool bRoot = false;
  bool bLeaf = false;

  if (argc > 2) {
    interp->result = "Too many arguments to PlvListScansCmd";
    return TCL_ERROR;
  }

  if (argc == 1) bRoot = true; // i.e. default mode

  else {
    if (!strcmp (argv[1], "root"))
      bRoot = true;
    else if (!strcmp (argv[1], "leaves"))
      bLeaf = true;
    else if (!strcmp (argv[1], "groups"))
      bLeaf = false;
    else {
      interp->result = "Bad option to PlvListScansCmd";
      return TCL_ERROR;
    }
  }

  if (bRoot) {
// STL Update
    for (vector<DisplayableMesh*>::iterator pdm = theScene->meshSets.begin();
	 pdm < theScene->meshSets.end(); pdm++) {

      // Thus, since theScene->meshSets only stores the roots,
      // all we need to do by default is append
      Tcl_AppendElement (interp, (char*)(*pdm)->getName());
    }
  } else
    PlvListScansHelper (interp, theScene->meshSets, bLeaf);
  return TCL_OK;
}

static void
PlvListScansHelper (Tcl_Interp *interp, vector<DisplayableMesh*> meshes, bool bLeaf)
{
// STL Update
  for (vector<DisplayableMesh*>::iterator pdm = meshes.begin(); pdm < meshes.end(); pdm++) {
    vector<DisplayableMesh*>children;

    GroupScan *gp = dynamic_cast<GroupScan*>((*pdm)->getMeshData());
    if (gp) {
      bool bGroup = gp->get_children_for_display(children);
      assert (bGroup);
      if (!bLeaf) Tcl_AppendElement(interp, (char*) (*pdm)->getName());
      PlvListScansHelper(interp, children, bLeaf);
    } else {
      if (bLeaf) Tcl_AppendElement(interp, (char*) (*pdm)->getName());
      //      else PlvListScansHelper(interp, children, bLeaf);
    }
  }
}


int
PlvLightCmd(ClientData clientData, Tcl_Interp *interp,
	    int argc, char *argv[])
{
    char result[PATH_MAX];

    if (argc != 1 && argc != 4) {
	interp->result = "Wrong number of args";
	return TCL_ERROR;
    }

    if (argc == 4) {
	theRenderParams->lightPosition[0] = atof(argv[1]);
	theRenderParams->lightPosition[1] = atof(argv[2]);
	theRenderParams->lightPosition[2] = atof(argv[3]);
    } else {
	sprintf(result, "plv_light %f %f %f",
		theRenderParams->lightPosition[0],
		theRenderParams->lightPosition[1],
		theRenderParams->lightPosition[2]);
	Tcl_SetResult(interp, result, TCL_VOLATILE);
    }

    return TCL_OK;
}


int
PlvRotateLightCmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[])
{
  float x, y, z;

  // project rectangular coordinates onto hemisphere
  x = (2 * atoi(argv[2]) / (float)theWidth) - 1;
  y = (-2 *atoi(argv[3]) / (float)theHeight) + 1;

  if (x < -1) x = -1; else if (x > 1) x = 1;
  if (y < -1) y = -1; else if (y > 1) y = 1;

  z = (1 - x*x - y*y);
  if (z < 0)
    z = -sqrt (-z);
  else if (z != 0)    // avoid sqrt(0)
    z = sqrt (z);

  // set theRenderParams->lightPosition
  theRenderParams->lightPosition[0] = x;
  theRenderParams->lightPosition[1] = y;
  theRenderParams->lightPosition[2] = z;

  rescheduleAutoReset();
  redraw (true);

  return TCL_OK;
}


int
PlvOrthographicCmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[])
{
  tbView->setOrthographic (true);
  redraw (true);

  return TCL_OK;
}



int
PlvPerspectiveCmd(ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[])
{
  tbView->setOrthographic (false);
  redraw (true);

  return TCL_OK;
}


int
PlvZoomAngleCmd(ClientData clientData, Tcl_Interp *interp,
		int argc, char *argv[])
{
  tbView->changeFOV(atof(argv[1]));
  redraw (true);

  return TCL_OK;
}


int
PlvObliqueCameraCmd(ClientData clientData, Tcl_Interp *interp,
		int argc, char *argv[])
{
  if (argc > 2)
  	tbView->setObliqueCamera(atof(argv[1]), atof(argv[2]));
  redraw (true);

  return TCL_OK;
}



int
PlvViewAllCmd(ClientData clientData, Tcl_Interp *interp,
	      int argc, char *argv[])
{
  redraw (true);
  return TCL_OK;
}


int
PlvResetXformCmd(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[])
{
  DisplayableMesh* meshDisp = NULL;
  bool bMeshesOnly = false;
  bool bAllMeshes = false;

  if (argc > 1) {
    if (!strcmp (argv[1], "all")) {
      bAllMeshes = true;
    } else if (!strcmp (argv[1], "allmesh")) {
      bMeshesOnly = true; bAllMeshes = true;
    }

    if (bAllMeshes) {
      for (int i = 0; i < theScene->meshSets.size(); i++)
	theScene->meshSets[i]->getMeshData()->resetXform();
    } else {
      meshDisp = FindMeshDisplayInfo (argv[1]);
    }
  }

  if (meshDisp != NULL)
    meshDisp->getMeshData()->resetXform();
  else {
    if (!bMeshesOnly) {
      theScene->computeBBox();
      theScene->centerCamera();
    }
  }

  redraw (true);
  return TCL_OK;
}


// give me GL (lower-left origin) screen coordinates!

static void
resetTranslationScale (int x, int y)
{
  Pnt3 clickWorld, clickScreen;
  if (findZBufferNeighborExt (x, y, clickWorld, toglCurrent, tbView,
			      100, &clickScreen, false)) {
    tbView->setTransScale (clickScreen);
  } else {
    // do we want to fall back on old method (average-case guess for
    // transScale) or keep using the last intelligently computed
    // transScale?
    tbView->setTransScale();
    //cerr << "resetting scale" << endl;
  }
}


static Pnt3
screenRotationCenter (void)
{
  int x = theWidth / 2;
  int y = theHeight / 2;

  Pnt3 clickWorld, clickScreen;
  if (!findZBufferNeighborExt (x, y, clickWorld,
			       toglCurrent, tbView, 100)) {
    // force unproject of (x, y, z) where z is middle of scene depth
    Unprojector unproject (tbView);

    Pnt3 nearz = unproject.forward (theScene->worldBbox().min());
    Pnt3 farz = unproject.forward (theScene->worldBbox().max());
    clickWorld = unproject (x, y, (nearz[2] + farz[2]) / 2);
  }

  return clickWorld;
}


static bool
passesOnscreenTest (int oldSize = -1, bool bOnlyQuery = false)
{
  // scene bbox must have at least 1 corner within inner 80% of screen,
  // and area at least 1% of screen.

  // this test is definitely not perfect; you can get the whole model
  // offscreen while keeping one bbox corner on, and you can't zoom in
  // very far because you'll lose all the corners pretty quick.  But it
  // works pretty well.

  int xmargin = theWidth / 10;
  int ymargin = theHeight / 10;
  TbObj tbObjIdentity;
  ScreenBox sb (&tbObjIdentity,
		xmargin, theWidth - xmargin,
		ymargin, theHeight - ymargin);
  theScene->computeBBox (Scene::sync);

  bool onscreen = false;
  int minx = INT_MAX, miny = INT_MAX, maxx = ~INT_MAX, maxy = ~INT_MAX;

  Bbox bb = theScene->worldBbox();
  Pnt3 center = sb.getProjector() (bb.center());

  for (int i = 0; i < 8; i++) {
    Pnt3 corner = bb.corner (i);
    if (sb.accept (corner))
      onscreen = true;

    Pnt3 screen = sb.getProjector() (corner);
    minx = MIN (minx, screen[0]);
    maxx = MAX (maxx, screen[0]);
    miny = MIN (miny, screen[1]);
    maxy = MAX (maxy, screen[1]);
  }

  int area = (maxx - minx) * (maxy - miny);
  if (bOnlyQuery) {
    oldSize = area;

  } else {

    if (!onscreen) { // all 8 failed
      cerr << "onscreen test fails: All corners offscreen!" << endl;
      return false;
    }

    int minsize = theWidth * theHeight * .01;
    if (oldSize == -1)
      oldSize = minsize;

    if (tbView->isZooming() || !tbView->isManipulating()) {
      // don't let them zoom out too far.
      if (area < MIN (oldSize, minsize)) {
	cerr << "onscreen test fails: bbox covers only " << area
	     << " and it was " << oldSize << endl;
	return false;
      }
    }
  }

  return true;
}


static void
passMoveToTrackball (int x, int y, int t) {
  Xform<float> before;
  int oldSize = -1;
  if (s_bForceOnscreen) {
    before = tbView->getUndoXform();
    passesOnscreenTest (oldSize, true);
    rescheduleAutoReset();
  }

  tbView->move(x, y, t, MeshData(theActiveScan));
  theScene->computeBBox();

  if (s_bForceOnscreen && !passesOnscreenTest (oldSize)) {
    cerr << "annulling trackball action" << endl;
    tbView->setUndoXform (before);
  } else {
    redraw (true);
  }
}


int
PlvRotateXYViewMouseCmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[])
{
  if (!strcmp (argv[2], "stopspin")) {
    tbView->stop();
    return TCL_OK;
  }

  int x = atoi(argv[2]);
  int y = atoi(argv[3]);
  int t = atoi(argv[4]);

  if (argc > 5) {
    prepareDrawInWin(argv[1]);

    if (!strcmp (argv[5], "start")) {
      lastActionTrackball();
      tbView->pressButton (1, 0, x, y, t, MeshData(theActiveScan));
    } else {
      tbView->pressButton (1, 1, x, y, t, MeshData(theActiveScan));

      if (!tbView->isManipulating()) {
	if (manipulatedRenderDifferent())
	  redraw (true);
      }
    }

    return TCL_OK;
  }

  passMoveToTrackball (x, y, t);
  return TCL_OK;
}


int
PlvTransXYViewMouseCmd(ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[])
{
  //cout << "transxyview" << endl;
  int x, y, t;

  x = atoi(argv[2]);
  y = atoi(argv[3]);
  t = atoi(argv[4]);

  if (argc > 5) {
    prepareDrawInWin(argv[1]);

    if (!strcmp (argv[5], "start")) {
      lastActionTrackball();
      tbView->pressButton (2, 0, x, y, t, MeshData(theActiveScan));
      if (tbView->isPanning())
	resetTranslationScale (x, theHeight - 1 - y);
    } else {
      tbView->pressButton (2, 1, x, y, t, MeshData(theActiveScan));

      if (!tbView->isManipulating()) {
	if (manipulatedRenderDifferent())
	  redraw (true);
      }
    }

    return TCL_OK;
  }

  passMoveToTrackball (x, y, t);
  return TCL_OK;
}


int
PlvTranslateInPlaneCmd(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[])
{
  if (argc < 3) {
    interp->result = "Bad args to PlvTranslateInPlaneCmd";
    return TCL_ERROR;
  }

  float dx = atof (argv[1]);
  float dy = atof (argv[2]);

  Pnt3 dt (dx, dy, 0);
  TbObj* target = MeshData(theActiveScan);
  //tbView->saveUndoPos (target);
  tbView->translateInEyeCoords (dt, target);

  theScene->computeBBox();
  redraw (true);

  return TCL_OK;
}


int
PlvUndoXformCmd(ClientData clientData, Tcl_Interp *interp,
		int argc, char *argv[])
{
  //tbView->undo_last();
  TbObj::undo();
  theScene->computeBBox();
  redraw (true);

  return TCL_OK;
}


int
PlvRedoXformCmd(ClientData clientData, Tcl_Interp *interp,
		int argc, char *argv[])
{
  //tbView->undo_last();
  TbObj::redo();
  theScene->computeBBox();
  redraw (true);

  return TCL_OK;
}


int
PlvGetScreenToWorldCoords(ClientData clientData, Tcl_Interp *interp,
			     int argc, char *argv[])
{
  if (argc != 3) {
    interp->result = "Bad arguments to PlvGetScreenToWorldCoords";
    return TCL_ERROR;
  }

  int x = atoi(argv[1]);
  int y = theHeight - atoi(argv[2]);

  Pnt3 coords;
  char answer[200];
  if (ScreenToWorldCoordinates (x, y, coords)) {
    sprintf (answer, "%g %g %g", coords[0], coords[1], coords[2]);
  } else if (findZBufferNeighbor (x, y, coords)) {
    sprintf (answer, "Hit background; nearest point: %g %g %g",
	     coords[0], coords[1], coords[2]);
  } else {
    strcpy (answer, "Hit background");
  }


  Tcl_SetResult (interp, answer, TCL_VOLATILE);
  return TCL_OK;
}


int
PlvSetThisAsCenterOfRotation(ClientData clientData, Tcl_Interp *interp,
			     int argc, char *argv[])
{
  if (argc != 3) {
    interp->result = "Bad arguments to PlvSetThisAsCenterOfRotation";
    return TCL_ERROR;
  }

  int x = atoi(argv[1]);
  int y = theHeight - atoi(argv[2]);

  Pnt3 newcenter;
  if (findZBufferNeighbor (x, y, newcenter))
  {
    tbView->newRotationCenter(newcenter, MeshData(theActiveScan));
    return TCL_OK;
  }
  else
  {
    interp->result =
      "We hit a background pixel.  You must click on the mesh.\n";
    return TCL_ERROR;
  }
}


int
PlvResetCenterOfRotation(ClientData clientData, Tcl_Interp *interp,
			 int argc, char *argv[])
{
  Pnt3 center;

  if (!strcmp (argv[1], "screen")) {

    if (theActiveScan != NULL) {
      interp->result = "Don't use screen-centered rotation for moveMesh";
      return TCL_ERROR;
    }
    center = screenRotationCenter();

  } else if (!strcmp (argv[1], "object")) {

    if (theActiveScan == NULL) {
      theScene->computeBBox (Scene::sync);
      center = theScene->worldCenter();
    } else {
      theActiveScan->getMeshData()->computeBBox();
      center = theActiveScan->getMeshData()->worldCenter();
    }

  } else {

    interp->result = "Must specify object|screen to PlvResetCoR";
    return TCL_ERROR;

  }

  tbView->newRotationCenter (center, MeshData(theActiveScan));

  return TCL_OK;
}


int
PlvCameraInfoCmd(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[])
{
  char transform[100];

  float quat[4];
  float trans[3];
  float camera[3];

  tbView->getXform (quat, trans);
  tbView->getCameraCenter (camera);
  for (int i = 0; i < 3; i++)
    trans[i] += camera[i];

  sprintf (transform, "%g %g %g\n%g %g %g %g\n",
	   trans[0], trans[1], trans[2],
	   quat[0], quat[1], quat[2], quat[3]);

  if (argc > 1 && 0 == strcmp (argv[1], "graphical"))
    Tcl_SetResult (interp, transform, TCL_VOLATILE);
  else
    printf ("%s\n", transform);

  return TCL_OK;
}


int
PlvSetHomeCmd(ClientData clientData, Tcl_Interp *interp,
	      int argc, char *argv[])
{
  if (argc != 2) {
    interp->result = "Bad arguments to PlvSetHomeCmd";
    return TCL_ERROR;
  }

  DisplayableMesh* ms;
  if (0 == strcmp (argv[1], "")) {
    ms = NULL;
  } else {
    ms = FindMeshDisplayInfo (argv[1]);
    if (ms == NULL) {
      interp->result = "Missing meshset in PlvSetHomeCmd";
      return TCL_ERROR;
    }
  }

  if (ms == NULL) { // viewer
    theScene->setHome();
  } else {
    ms->setHome();
  }

  return TCL_OK;
}


int
PlvGoHomeCmd(ClientData clientData, Tcl_Interp *interp,
	      int argc, char *argv[])
{
  if (argc != 2) {
    interp->result = "Bad arguments to PlvGoHomeCmd";
    return TCL_ERROR;
  }

  DisplayableMesh* ms;
  if (0 == strcmp (argv[1], "")) {
    ms = NULL;
  } else {
    ms = FindMeshDisplayInfo (argv[1]);
    if (ms == NULL) {
      interp->result = "Missing meshset in PlvGoHomeCmd";
      return TCL_ERROR;
    }
  }

  if (ms == NULL) { // viewer
    theScene->goHome();
  } else {
    ms->goHome();
  }

  redraw (true);
  return TCL_OK;
}


int
PlvSetOverallResCmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[])
{
  if (argc != 2) {
    interp->result = "Bad argument to PlvSetOverallResCmd";
    return TCL_ERROR;
  }

  int res;
  if (strcmp (argv[1], "temphigh") == 0)
    res = Scene::resTempHigh;
  else if (strcmp (argv[1], "templow") == 0)
    res = Scene::resTempLow;
  else if (strcmp (argv[1], "high") == 0)
    res = Scene::resHigh;
  else if (strcmp (argv[1], "low") == 0)
    res = Scene::resLow;
  else if (strcmp (argv[1], "nexthigh") == 0)
    res = Scene::resNextHigh;
  else if (strcmp (argv[1], "nextlow") == 0)
    res = Scene::resNextLow;
  else
    res = Scene::resDefault;

  theScene->setMeshResolution (res);
  redraw (true);

  return TCL_OK;
}


int
PlvFlattenCameraXformCmd(ClientData clientData, Tcl_Interp *interp,
			 int argc, char *argv[])
{
  if (argc != 1) {
    interp->result = "Bad argument to PlvFlattenCameraXformCmd";
    return TCL_ERROR;
  }

  theScene->flattenCameraXform();
  redraw (true);

  return TCL_OK;
}


int
PlvSpaceCarveCmd(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[])
{
  // space-carve all visible scans and display the result as a mesh
  if (argc < 2) {
    interp->result = "Bad argument to PlvSpaceCarveCmd";
    return TCL_ERROR;
  }
  int levels = atoi (argv[1]);
  if (levels <= 0) {
    interp->result = "Bad levels in PlvSpaceCarveCmd";
    return TCL_ERROR;
  }

  // build list of visible scans
  vector<RigidScan*> scans;
  for (int i = 0; i < theScene->meshSets.size(); i++) {
    if (theScene->meshSets[i]->getVisible())
      scans.push_back (theScene->meshSets[i]->getMeshData());
  }

  // space-carve
  vector<Pnt3> pnts;
  vector<int>  tris;
  CubeTree* ct = new CubeTree (theScene->worldCenter(),
			       theScene->worldBbox().maxDim() * 1.05);
  //                           2.1 * theScene->sceneRadius());

  SHOW(theScene->sceneRadius());

  if (argc > 2)
    ct->set_leaf_depth (atoi (argv[2]));

  TIMER(carve);
  ct->carve (scans, levels, pnts, tris);
  TIMER_STOP(carve);
  delete ct;

  cout << "Polygonizer results:\n\t"
       << tris.size()/3 << " tris\n\t"
       << pnts.size() << " points"
       << endl;

#if 0  // if we don't trust the points output
  for (i=0; i<pnts.size(); i++) {
    if (pnts[i].norm() < 5.0) continue;
    SHOW(pnts[i]);
    pnts[i].set(0,0,0);
  }
#endif
#if 0  // if we don't trust the tris output
  for (i=0; i<tris.size(); i++) {
    if (tris[i] >= pnts.size()) {
      SHOW(tris[i]);
      int base = i/3;
      tris[3*base] = tris[3*base+1] = tris[3*base+2] = 0;
    }

    assert(tris[i]>=0);
    assert(tris[i]<pnts.size());
  }
#endif


  // now add this mesh to scene, and hide all others
  RigidScan* scan = CreateScanFromGeometry (pnts, tris, "polygonize");
  DisplayableMesh* dm = theScene->addMeshSet (scan);
  Tcl_Eval (interp, "showAllMeshes 0");
  Tcl_SetResult (interp, (char*)dm->getName(), TCL_VOLATILE);
  return TCL_OK;
}


int
PlvConstrainRotationCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[])
{
  TbConstraint constraint = CONSTRAIN_NONE;

  if (argc > 1) {
    if (!strcmp (argv[1], "x"))
      constraint = CONSTRAIN_X;
    else if (!strcmp (argv[1], "y"))
      constraint = CONSTRAIN_Y;
    else if (!strcmp (argv[1], "z"))
      constraint = CONSTRAIN_Z;
  }

  tbView->constrainRotation (constraint);

  return TCL_OK;
}


int
PlvSetManipRenderModeCmd(ClientData clientData, Tcl_Interp *interp,
			 int argc, char *argv[])
{
  if (argc == 2) { // better be lock/unlock
    if (!strcmp (argv[1], "lock")) {
      s_bManipulatingLocked = true;
      return TCL_OK;
    } else if (!strcmp (argv[1], "unlock")) {
      s_bManipulatingLocked = false;
      return TCL_OK;
    }
  }

  if (argc != 7) {
    interp->result = "Bad args to PlvSetManipRenderModeCmd";
    return TCL_ERROR;
  }

  theRenderParams->bRenderManipsPoints = atoi (argv[1]);
  theRenderParams->bRenderManipsTinyPoints = atoi (argv[2]);
  theRenderParams->bRenderManipsUnlit = atoi (argv[3]);
  theRenderParams->bRenderManipsLores = atoi (argv[4]);
  theRenderParams->bRenderManipsSkipDlist = atoi (argv[5]);
  theRenderParams->iFastManipsThreshold = atoi (argv[6]);

  return TCL_OK;
}

int
PlvManualRotateCmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[])
{
  Pnt3 axis;

  if (argc < 4) {
    interp->result = "Bad args to PlvManualRotateCmd";
    return TCL_ERROR;
  }

  //  tbView->saveUndoPos(MeshData(theActiveScan));

  axis[0] = 1; axis[1] = axis[2] = 0;
  tbView->rotateAroundAxis(axis, atof(argv[1]) * M_PI/180.0,
			   MeshData(theActiveScan));

  axis[0] = 0; axis[1] = 1;
  tbView->rotateAroundAxis(axis, atof(argv[2]) * M_PI/180.0,
			   MeshData(theActiveScan));

  axis[1] = 0; axis[2] = 1;

  tbView->rotateAroundAxis(axis, atof(argv[3]) * M_PI/180.0,
			   MeshData(theActiveScan));

  return TCL_OK;
}

int
PlvManualTranslateCmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[])
{
  Pnt3 tp;

  if (argc < 4) {
    interp->result = "Bad args to PlvManualTranslateCmd";
    return TCL_ERROR;
  }

  tp[0] = atof(argv[1]);
  tp[1] = atof(argv[2]);
  tp[2] = atof(argv[3]);

  //tbView->saveUndoPos(MeshData(theActiveScan));
  tbView->translateInEyeCoords(tp, MeshData(theActiveScan));


  return TCL_OK;
}


int
PlvPickScanFromPointCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[])
{
  if (argc < 2) {
    return TCL_ERROR;
  }

  static vector<DisplayableMesh*> ptMeshMap;
  static int w;
  static int h;

  if (!strcmp (argv[1], "init")) { // ok to call more than once w/o exit
    w = Togl_Width  (toglCurrent);
    h = Togl_Height (toglCurrent);

    // build pts->mesh map
    cerr << "Building map from screen points to meshes ... " << flush;
    GetPtMeshMap (w, h, ptMeshMap);
    cerr << "done." << endl;

  } else if (!strcmp (argv[1], "exit")) {

    // free pts->mesh map
    ptMeshMap.clear();

  } else if (!strcmp (argv[1], "get")) {

    if (argc < 4)
      return TCL_ERROR;

    int x = atoi (argv[2]);
    int y = h - atoi (argv[3]) - 1;

    cmdassert (x >= 0 && x < w);
    cmdassert (y >= 0 && y < h);

    DisplayableMesh* dm = ptMeshMap[y*w + x];
    if (dm)
      interp->result = (char*)dm->getName();
    else
      interp->result = "";
  } else
    return TCL_ERROR;

  return TCL_OK;
}

int
PlvGetVisiblyRenderedScans(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[])
{


  static vector<DisplayableMesh*> ptMeshMap;
   int w;
   int h;
   int startx, starty;
   int fullx, fully;

   fullx = Togl_Width  (toglCurrent);
   fully =  Togl_Height (toglCurrent);
   if (argc == 1) {
     // Check whole screen
     w = Togl_Width  (toglCurrent);
     h = Togl_Height (toglCurrent);
     startx=0;
     starty=0;
   } else if (argc == 2 && strcmp(argv[1],"selected")==0) {
     // Find Rectangle
      if (theSel.type == Selection::rect)
	{

	   startx = min (theSel[0].x, theSel[2].x);
	   w = abs (theSel[0].x - theSel[2].x);
	   starty = min (theSel[0].y, theSel[2].y);
	   h = abs (theSel[0].y - theSel[2].y);
	}
      else
	{
	  interp->result="Need to have a rectangle selected";
	  return TCL_ERROR;
	}
   } else if (argc == 5) {
     // Check in a x,y,w,h region
     startx = atoi(argv[1]);
     starty = atoi(argv[2]);
     w = atoi(argv[3]);
     h = atoi(argv[4]);
   } else {
     // Usage
     Tcl_SetResult(interp,"Usage: plv_getVisiblyRenderedScans [xstart ystart w h]\n       plv_getVisiblyRenderedScans [selected]\n",TCL_STATIC);
     return TCL_ERROR;
   }

    // build pts->mesh map
   // cerr << "Building map from screen points to meshes ... " << flush;
  GetPtMeshVector (startx,starty,w, h, fullx, fully, ptMeshMap);
  //cerr << "done." << endl;


  for (int i=0; i<theScene->meshSets.size(); i++) {
    DisplayableMesh* dm = ptMeshMap[i];
    if (dm) {
      Tcl_AppendResult(interp,(char*)dm->getName()," ", (char *) NULL);
    }
  }

  ptMeshMap.clear();
  return TCL_OK;
}


int
PlvPositionCameraCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{
  Pnt3 c, o, t;
  float q[4];
  float fov;
  float oblique_x, oblique_y;

#define PNT3Contents(p) p[0],p[1],p[2]
  if (argc == 1) {
    tbView->getState (c, o, t, q, fov, oblique_x, oblique_y);
    char buf[1000];
    sprintf (buf, "%g %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g\n",
	     PNT3Contents (c),
	     PNT3Contents (o),
	     PNT3Contents (t),
	     q[0], q[1], q[2], q[3], fov, oblique_x, oblique_y);
    Tcl_SetResult (interp, buf, TCL_VOLATILE);
  } else if (argc == 2) {
    // Load camera from a Tsai parameters file
    CameraParams *cam = CameraParams::Read(argv[1]);
    if (!cam) {
      interp->result = "Can't read camera file";
      return TCL_ERROR;
    }

    tbView->getState (c, o, t, q, fov, oblique_x, oblique_y);

    Xform<float> xf(cam->GLmodelmatrix());
    xf.rotX(M_PI);
    xf.toQuaternion(q);
    Pnt3 origin, campos, tmp;
    xf.apply_inv(origin, campos);
    campos[0] -= o[0];  campos[1] -= o[1];  campos[2] -= o[2];
    Xform<float> rot;
    rot.fromQuaternion(q);
    rot.apply(campos, tmp);
    tmp[0] += o[0];  tmp[1] += o[1];  tmp[2] += o[2];
    tmp[0] -= c[0];  tmp[1] -= c[1];  tmp[2] -= c[2];
    t[0] = -tmp[0];  t[1] = -tmp[1];  t[2] = -tmp[2];

    float w = cam->imgwidth, h = cam->imgheight;
    float xscale = (float)theWidth / w;
    float yscale = (float)theHeight / h;
    if (xscale < yscale) {
	h *= yscale / xscale;
    } else {
	w *= xscale / yscale;
    }
    float halffov = 0.5 * sqrt(w*w+h*h) / cam->pixels_per_radian;
    fov = atan(halffov) * 2.0 * 180.0 / M_PI;
    oblique_x = oblique_y = 0;

    Pnt3 up (0, 1, 0);
    tbView->setup (c, o, up, theScene->sceneRadius(), fov, oblique_x, oblique_y);
    tbView->setState (t, q);

    theScene->computeBBox();
    tbView->newRotationCenter (theScene->worldCenter(), MeshData(NULL));

    delete cam;
  } else if (argc >= 15) {
    int i;
    for (i = 1; i < 4; i++)
      c[i-1] = atof (argv[i]);
    for (; i < 7; i++)
      o[i-4] = atof (argv[i]);
    for (; i < 10; i++)
      t[i-7] = atof (argv[i]);
    for (; i < 14; i++)
      q[i-10] = atof (argv[i]);
    fov = atof(argv[i++]);
    if (argc == 17) {
	oblique_x = atof(argv[i++]);
	oblique_y = atof(argv[i++]);
    } else {
	oblique_x = oblique_y = 0;
    }
    Pnt3 up (0, 1, 0);
    tbView->setup (c, o, up, theScene->sceneRadius(), fov, oblique_x, oblique_y);
    tbView->setState (t, q);
    theScene->computeBBox (Scene::sync);
    redraw (true);
  } else {
    interp->result = "Bad # args";
    return TCL_ERROR;
  }

  return TCL_OK;
}


int
PlvSortScanListCmd (ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[])
{
  vector<DisplayableMesh*> sortedList;
  vector<Scene::sortCriteria> criteria;

  Scene::sortCriteria sc;
  bool bDictionary = true;
  bool bListInvisible = true;

  criteria.reserve (argc - 1);
  for (int i = 1; i < argc; i++) {
    sc = Scene::none;

    if (!strcmp (argv[i], "Name"))
      sc = Scene::name;
    else if (!strcmp (argv[i], "File date"))
      sc = Scene::date;
    else if (!strcmp (argv[i], "Loadedness"))
      sc = Scene::loaded;
    else if (!strcmp (argv[i], "Visibility"))
      sc = Scene::visibility;
    else if (!strcmp (argv[i], "PolygonCount"))
      sc = Scene::polycount;
    else if (!strcmp (argv[i], "ascii"))
      bDictionary = false;
    else if (!strcmp (argv[i], "dictionary"))
      bDictionary = true;
    else if (!strcmp (argv[i], "onlyvisible"))
      bListInvisible = false;
    else
      cerr << "Warning: unrecognized sort option " << argv[i] << endl;

    if (sc != Scene::none)
      criteria.push_back (sc);
  }

  theScene->sortSceneMeshes (sortedList, criteria,
			     bDictionary, bListInvisible);

  int size = 0;
  for (int i = 0; i < sortedList.size(); i++) {
    size += strlen (sortedList[i]->getName()) + 1;
  }

  if (size == 0) {
    Tcl_SetResult (interp, "", TCL_STATIC);
  } else {
    char* list = (char*)malloc (size);
    char* end = list;
    for (int i = 0; i < sortedList.size(); i++) {
      strcpy (end, sortedList[i]->getName());
      end += strlen (end);
      *end++ = ' ';
    }
    end[-1] = 0;  // then terminate the whole shebang

    Tcl_SetResult (interp, list, (Tcl_FreeProc*)free);
  }

  return TCL_OK;
}


int
PlvZoomToRectCmd(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[])
{
  int x1,x2,y1,y2;

  if (argc==5) {
    // We passed in the zoom rect values
    x1 = atoi(argv[1]);
    int ty1 = theHeight - atoi(argv[2]);
    x2 = atoi(argv[3]);
    int ty2 = theHeight - atoi(argv[4]);
    y1=MIN(ty1,ty2);
    y2=MAX(ty1,ty2);
  }
  else {
    // We want to get the values from the selection
    if (theSel.type != Selection::rect) {
      return TCL_ERROR;
    }
    // points 0 and 2 are the corners
    x1 = MIN (theSel[0].x, theSel[2].x);
    x2 = MAX (theSel[0].x, theSel[2].x);
    y1 = MIN (theSel[0].y, theSel[2].y);
    y2 = MAX (theSel[0].y, theSel[2].y);
  }

  printf("zoom %d %d : %d %d\n",x1,y1,x2,y2);

  // need to set translation scale before calling zoomToRect
  // BUGBUG this just looks for data near the center of selection rect;
  // normally people will probably use it this way, but we could actually
  // unproject everything in the selection, and average it.
	resetTranslationScale ((x1 + x2) / 2, (y1 + y2) / 2);
  tbView->zoomToRect (x1, y1, x2, y2);

  // attempt to update selection rectangle...
  int rectW = x2 - x1;
  int rectH = y2 - y1;
  float zoom = MIN ((float)theWidth / rectW, (float)theHeight / rectH);

  x1 = (theWidth / 2) - (rectW * zoom / 2);
  x2 = (theWidth / 2) + (rectW * zoom / 2);
  y1 = (theHeight / 2) - (rectH * zoom / 2);
  y2 = (theHeight / 2) + (rectH * zoom / 2);
	// update selection info only if we zoomed to a rect
	if (argc!=5) {
		theSel[0].x = x1; theSel[0].y = y1;
		theSel[2].x = x2; theSel[2].y = y2;
		theSel[1].x = x1; theSel[1].y = y2;
		theSel[3].x = x2; theSel[3].y = y1;
	}

  Togl_PostOverlayRedisplay (toglCurrent);
  redraw (true);

  return TCL_OK;
}


bool
isManipulatingRender (void)
{
  // return true if we render any differently (points, lores, unlit, etc.)
  // because the rendering is being trackballed

  if (s_bManipulatingLocked)
    return true;

  if (tbView->isManipulating())
      return manipulatedRenderDifferent();

  return false;
}


bool
manipulatedRenderDifferent (void)
{
  if (theRenderParams->iFastManipsThreshold < 0)
    return false;

  if (theRenderParams->bRenderManipsPoints
      || theRenderParams->bRenderManipsUnlit
      || theRenderParams->bRenderManipsLores) {

    char* pszNum = Tcl_GetVar (Togl_Interp (toglCurrent),
			       "visPolyCount",
			       TCL_GLOBAL_ONLY);
    int nPolys = atoi (pszNum);
    return (nPolys > theRenderParams->iFastManipsThreshold);
  }

  return false;
}


static Tk_TimerToken s_autoResetTimerToken = 0;


static void
autoResetProc (ClientData clientData = NULL)
{
  theScene->centerCamera();
  theRenderParams->lightPosition[0] = 0.0;
  theRenderParams->lightPosition[1] = 0.0;
  theRenderParams->lightPosition[2] = 1.0;
  redraw (true);

  if (!s_autoResetScript.empty()) {
    Tcl_Eval (g_tclInterp, (char*)s_autoResetScript.c_str());
  }

  rescheduleAutoReset();
}


static void
rescheduleAutoReset (void)
{
  if (s_autoResetTimerToken) {
    Tk_DeleteTimerHandler (s_autoResetTimerToken);
    s_autoResetTimerToken = 0;
  }

  if (s_iAutoResetTime) {
    s_autoResetTimerToken =
      Tk_CreateTimerHandler (s_iAutoResetTime, autoResetProc,
			     NULL);
  }
}


int
PlvForceKeepOnscreenCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[])
{
  if (argc < 2) {
    interp->result = "Wrong # args";
    return TCL_ERROR;
  }

  s_bForceOnscreen = atoi (argv[1]);
  if (s_bForceOnscreen && !passesOnscreenTest()) {
    s_bForceOnscreen = false;
    interp->result = "Please GET the model onscreen "
      "before trying to KEEP it there";
    return TCL_ERROR;
  }

  if (s_bForceOnscreen && argc > 2) {
    // 2nd arg is time until auto-reset-viewer.

    if (argc > 3) {
      // 3nd arg is script to run on auto-reset.
      s_autoResetScript = crope (argv[3]);
    }

    // force message to pop up right away
    s_iAutoResetTime = 50;
    rescheduleAutoReset();
    s_iAutoResetTime = 1000 * atoi (argv[2]);

  } else {
    s_iAutoResetTime = 0;
    s_autoResetScript = crope();
    rescheduleAutoReset();
  }

  return TCL_OK;
}


void
SpinTrackballs (ClientData clientData)
{
  if (tbView->isSpinning()) {

    Xform<float> before;
    if (s_bForceOnscreen)
      before = tbView->getUndoXform();

    redraw (true);

    if (s_bForceOnscreen && !passesOnscreenTest()) {
      cerr << "killing spin before it goes offscreen." << endl;
      tbView->stop();
      tbView->setUndoXform (before);
    }
  }

  Tk_CreateTimerHandler (30, SpinTrackballs, clientData);
}
