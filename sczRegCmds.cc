//////////////////////////////////////////////////////////////////////
//
// sczRegCmds.cc
// Matt Ginzton, Kari Pulli
// Fri Jun 12 15:19:16 PDT 1998
//
// Interface between C and Tcl code for various registration UIs
//   1.  Correspondence registration
//   2.  Drag registration
//   3.  Global registration
//   4.  ICP
//
//////////////////////////////////////////////////////////////////////


#include "plvGlobals.h"
#include "ToglHash.h"
#include "togl.h"
#include "Trackball.h"
#include "RigidScan.h"
#include "plvMeshCmds.h"
#include "plvDraw.h"
#include "sczRegCmds.h"
#include "plvAnalyze.h"
#include "ColorUtils.h"
#include "GlobalReg.h"
#include "plvScene.h"
#include "plvDrawCmds.h"
#include "DisplayMesh.h"
#include "absorient.h"
#include "Random.h"
#include "ToglCache.h"
#include "ICP.h"
#include "Progress.h"



//////////////////////////////////////////////////////////////////////
//
// 1.  Correspondence-registration UI
//
//////////////////////////////////////////////////////////////////////

static struct Togl* g_overviewTogl = NULL;
static uchar colorNew[3] = { 0, 0, 0 };            // black
static uchar colorOld[3] = { 255, 0, 0 };          // red
static uchar colorSel[3] = { 0, 255, 0 };          // green
static uchar colorHilite[3] = { 255, 255, 255 };   // white


#define A_LITTLE_OUT_OF_THE_SCREEN  1e-6
#define GLOBALREG_TOLERANCE         1.e-4


class AlignmentInfo
{
public:
  AlignmentInfo();
  virtual ~AlignmentInfo();

  Trackball* tb;
};


class AlignmentToglInfo: public AlignmentInfo
{
public:
  void SetTargetMesh (DisplayableMesh* _md);

  DisplayableMesh* meshDisplay;
  RigidScan* meshData;
};


class CorrespondencePt
{
public:
  DisplayableMesh* mesh;
  Pnt3 pt;
};


class CorrespondenceList
{
public:
  int AddPoint (DisplayableMesh* mesh, const Pnt3& pt);
// STL Update
  vector<CorrespondencePt>::iterator* FindPoint (DisplayableMesh* mesh);
  void DeletePoint (vector<CorrespondencePt>::iterator cp);
  void ToString (char* text);

  int id;
  uchar color[3];
  vector <CorrespondencePt> points;
};


class AlignmentOverviewInfo: public AlignmentInfo
{
public:
  AlignmentOverviewInfo (struct Togl* overviewTogl);
  ~AlignmentOverviewInfo();

  CorrespondenceList* AddCorrespondence (DisplayableMesh* mesh,
					 const Pnt3& pt);
  CorrespondenceList* CompleteCorrespondence (bool bKeep, char* text);

  vector<CorrespondenceList*> correspondences;
  int GetSelectedCorrespondenceId();
  CorrespondenceList* GetSelectedCorrespondence();
  CorrespondenceList* GetCorrespondenceById (int id);
  CorrespondenceList* GetCorrespondenceInProgress();

  bool                 bColorPoints;

private:
  int                  nNextId;
  CorrespondenceList*  correspInProgress;
  ColorSet             correspColors;
  struct Togl*         togl;
};


int
CorrespondenceList::AddPoint (DisplayableMesh* mesh, const Pnt3& pt)
{
// STL Update
  vector<CorrespondencePt>::iterator* cp = FindPoint (mesh);
  if (cp != NULL) {
    // already have one for this mesh -- adjust it
    (*cp)->pt = pt;
  } else {
    // create new correspondence for this mesh
    CorrespondencePt cp;
    cp.mesh = mesh;
    cp.pt = pt;
    points.push_back (cp);
  }

  return points.size();
}


// STL Update
vector<CorrespondencePt>::iterator*
CorrespondenceList::FindPoint (DisplayableMesh* mesh)
{
// STL Update
  for (vector<CorrespondencePt>::iterator cp = points.begin(); cp < points.end(); cp++) {
    if (cp->mesh == mesh)
      return &cp;
  }

  return NULL;
}


// STL Update
void
CorrespondenceList::DeletePoint (vector<CorrespondencePt>::iterator cp)
{
  points.erase (cp);
}


void
CorrespondenceList::ToString (char* text)
{
  sprintf (text, "[%d]", id);
// STL Update
  for (vector<CorrespondencePt>::iterator cp = points.begin();
       cp < points.end();
       cp++) {
    sprintf (text + strlen(text), " %s:(%.4g,%.4g,%.4g)",
	     cp->mesh->getName(),
	     cp->pt[0], cp->pt[1], cp->pt[2]);

// STL Update
    if (cp + 1 < points.end())
      strcat (text, " <=>");
  }
}


AlignmentInfo::AlignmentInfo()
{
  tb = NULL;
}


AlignmentInfo::~AlignmentInfo()
{
  if (tb != NULL)
    delete tb;
}


AlignmentOverviewInfo::AlignmentOverviewInfo (struct Togl* overviewTogl)
{
  nNextId = 0;
  correspInProgress = NULL;
  togl = overviewTogl;
  bColorPoints = true;
}


AlignmentOverviewInfo::~AlignmentOverviewInfo ()
{
  for (int i = 0; i < correspondences.size(); i++) {
    delete correspondences[i];
  }

  g_overviewTogl = NULL;
}


CorrespondenceList*
AlignmentOverviewInfo::AddCorrespondence (DisplayableMesh* mesh,
					  const Pnt3& pt)
{
  CorrespondenceList* clSelected = GetSelectedCorrespondence();
  if (clSelected != NULL) {
    clSelected->AddPoint (mesh, pt);
    return clSelected;
  }

  if (correspInProgress == NULL) {
    correspInProgress = new CorrespondenceList;
    correspInProgress->id = 0;
  }

  correspInProgress->AddPoint (mesh, pt);
  return correspInProgress;
}


CorrespondenceList*
AlignmentOverviewInfo::CompleteCorrespondence (bool bKeep,
					       char* text)
{
  if (correspInProgress == NULL) {
    strcpy (text, "[0]");
    return NULL;     // nothing to do
  }

  if (correspInProgress->points.size() < 2)  // nothing to keep!
    bKeep = false;

  if (bKeep) {
    correspInProgress->id = ++nNextId;
    correspInProgress->ToString (text);
    correspColors.chooseNewColor (correspInProgress->color);
    correspondences.push_back (correspInProgress);
    correspInProgress = NULL;
    return correspondences.back();
  } else {
    correspInProgress->ToString (text);
    delete correspInProgress;
    correspInProgress = NULL;
    return NULL;
  }
}


CorrespondenceList*
AlignmentOverviewInfo::GetSelectedCorrespondence (void)
{
  return GetCorrespondenceById (GetSelectedCorrespondenceId());
}


CorrespondenceList*
AlignmentOverviewInfo::GetCorrespondenceInProgress (void)
{
  return correspInProgress;
}


CorrespondenceList*
AlignmentOverviewInfo::GetCorrespondenceById (int id)
{
  if (id == 0)
    return NULL;

// STL Update
  vector<CorrespondenceList*>::iterator cl;
  for (cl = correspondences.begin(); cl < correspondences.end(); cl++) {
    if ((*cl)->id == id)
      return *cl;
  }

  printf ("Consistency error in correspondence data structures\n");
  return NULL;
}


int
AlignmentOverviewInfo::GetSelectedCorrespondenceId (void)
{
  Tcl_Interp* interp = Togl_Interp (togl);
  // figure out which correspondence is selected
  Tcl_Eval (interp, "getListboxSelection .reg.lines.list");
  int iCorresp = 0;
  if (strlen(interp->result) > 3)
    iCorresp = atoi (interp->result + 1);

  return iCorresp;
}


void
AlignmentToglInfo::SetTargetMesh (DisplayableMesh* _md)
{
  meshDisplay = _md;
  if (meshDisplay == NULL) {
    meshData = NULL;
    return;
  }
  meshData = meshDisplay->getMeshData();
  assert (meshData != NULL);

  // set up trackball, rotate by current view of object...
  Pnt3 up (0, 1, 0);
  Pnt3 obj = meshData->localCenter();
  float radius = meshData->radius();
  Pnt3 camera (0, 0, radius * 3);

  //position camera in front of object
  camera += obj;
  tb->setup (camera, obj, up, radius);

  // transform by rotations in effect: local, then viewer
  Xform<float> xfr = meshData->getXform();
  xfr.removeTranslation();
  float q[4], t[3];
  tbView->getXform (q, t);
  xfr.rotQ (q[0], q[1], q[2], q[3]);

  // and apply to trackball
  xfr.toQuaternion (q);
  tb->rotateQuat (q);
}


void
DrawAlignmentPoint (const Pnt3& pt, uchar* color, bool bSelected = false)
{
  glDisable (GL_LIGHTING);

  for (int iPass = 1; iPass >= 0; iPass--) {
    //draw twice, once partially transparent without depth test

    if (iPass == 0) {
      glEnable (GL_DEPTH_TEST);
      glDepthFunc (GL_LEQUAL);
      glDisable (GL_BLEND);
    } else {
      glDisable (GL_DEPTH_TEST);
      glEnable (GL_BLEND);
      glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    if (bSelected && iPass == 1) {
      glColor4ub (colorHilite[0], colorHilite[1], colorHilite[2], 255);
      glPointSize (7);
    } else {
      glColor4ub (color[0], color[1], color[2], 256*(1-iPass/1.3));
      if (bSelected)
	glPointSize (5);
      else
	glPointSize (4);
    }

    glBegin (GL_POINTS);
    glVertex3fv (pt);
    glEnd();
  }
}


void
DrawAlignmentPoints (struct Togl* togl)
{
  //warning: code that does not scale well with a huge number of
  //correspondences, but that probably won't matter.  If it does,
  //do something with better than linear performance here.

  AlignmentOverviewInfo* aoi = NULL;
  if (g_overviewTogl != NULL)
    aoi = (AlignmentOverviewInfo*)Togl_GetClientData (g_overviewTogl);
  if (aoi != NULL) {
    AlignmentToglInfo* ati = (AlignmentToglInfo*)Togl_GetClientData (togl);
    assert (ati != NULL);

    if (ati->meshData != NULL) {

      int iCorresp = aoi->GetSelectedCorrespondenceId();

      // now draw all correspondences in correct color
      for (int i = 0; i < aoi->correspondences.size(); i++) {
	CorrespondenceList* cl = aoi->correspondences[i];

	bool bSelected = (cl->id == iCorresp);

// STL Update
	for (vector<CorrespondencePt>::iterator c = cl->points.begin();
	     c < cl->points.end();
	     c++) {
	  if (c->mesh == ati->meshDisplay) {
	    uchar* color;
	    if (aoi->bColorPoints)
	      color = cl->color;
	    else
	      color = bSelected ? colorSel : colorOld;

	    DrawAlignmentPoint (c->pt, color, bSelected);
	  }
	}
      }
    }

    // now draw correspondence in progress
    CorrespondenceList* cl = aoi->GetCorrespondenceInProgress();
    if (cl != NULL) {
// STL Update
      for (vector<CorrespondencePt>::iterator c = cl->points.begin();
	   c < cl->points.end();
	   c++) {
	if (c->mesh == ati->meshDisplay)
	  DrawAlignmentPoint (c->pt, colorNew, true);
      }
    }
  }
}


void
DrawAlignmentMesh (struct Togl* togl)
{
  DrawAlignmentMeshToBack (togl);
  DrawAlignmentPoints (togl);

  Togl_SwapBuffers (togl);
}


void
DrawAlignmentMeshToBack (struct Togl* togl)
{
  AlignmentToglInfo* ati = (AlignmentToglInfo*)Togl_GetClientData (togl);
  assert (ati != NULL);

  if (ati->meshData == NULL)
    return;

  // initialize buffers and modes
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  ati->tb->setSize (Togl_Width (togl), Togl_Height (togl));
  glViewport(0, 0, Togl_Width (togl), Togl_Height (togl));
  glEnable (GL_DEPTH_TEST);
  glDepthFunc (GL_LESS);

  // Prepare projection
  ati->tb->applyProjection();

  // set up lighting
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity();
  float downZaxis[4] = { 0, 0, 1, 0 };
  glLightfv(GL_LIGHT0, GL_POSITION, downZaxis);

  // prepare modelview
  ati->tb->applyXform();

  // now render
  ati->meshDisplay->drawImmediate();
}


void
SpinToglTrackball (struct Togl* togl)
{
  AlignmentInfo* ai = (AlignmentInfo*)Togl_GetClientData (togl);
  assert (ai != NULL);

  if (ai->tb->isSpinning())
  {
    Togl_PostRedisplay (togl);
  }
}


void
FreeAlignmentInfo (struct Togl* togl)
{
  AlignmentInfo* ai = (AlignmentInfo*)Togl_GetClientData (togl);

  assert (ai != NULL);
  delete ai;

  Togl_SetClientData (togl, NULL);
}


int
PlvBindToglToAlignmentOverviewCmd(ClientData clientData, Tcl_Interp *interp,
				  int argc, char *argv[])
{
  if (argc < 2) {
    interp->result = "Bad arguments to PlvBindToglToAlignmentOverviewCmd";
    return TCL_ERROR;
  }

  struct Togl* togl = toglHash.FindTogl (argv[1]);
  if (togl == NULL) {
    interp->result = "Missing togl widget in PlvBindToglToAlignmentViewCmd";
    return TCL_ERROR;
  }
  assert (0 == strcmp (argv[1], Togl_Ident (togl)));

  AlignmentOverviewInfo* aoi = new AlignmentOverviewInfo (togl);
  aoi->tb = new Trackball;
  Togl_SetClientData (togl, aoi);
  Togl_SetDestroyFunc (togl, FreeAlignmentInfo);
  Togl_SetTimerFunc (togl, SpinToglTrackball);

  assert (g_overviewTogl == NULL);
  g_overviewTogl = togl;

  return TCL_OK;
}


int
PlvCorrespRegParmsCmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[])
{
  if (argc < 3) {
    interp->result = "Bad arguments to PlvCorrespRegParmsCmd";
    return TCL_ERROR;
  }

  struct Togl* togl = toglHash.FindTogl (argv[1]);
  if (togl == NULL) {
    interp->result = "Missing togl widget in PlvCorrespRegParmsCmd";
    return TCL_ERROR;
  }
  AlignmentOverviewInfo* aoi =
    (AlignmentOverviewInfo*)Togl_GetClientData (togl);

  if (!strcmp (argv[2], "colorpoints")) {
    if (argc > 2) {
      aoi->bColorPoints = atoi (argv[3]);
    } else {
      interp->result = aoi->bColorPoints ? "1" : "0";
    }
  }

  return TCL_OK;
}


int
PlvBindToglToAlignmentViewCmd(ClientData clientData, Tcl_Interp *interp,
			      int argc, char *argv[])
{
  if (argc < 3) {
    interp->result = "Bad arguments to PlvBindToglToAlignmentViewCmd";
    return TCL_ERROR;
  }

  struct Togl* togl = toglHash.FindTogl (argv[1]);
  if (togl == NULL) {
    interp->result = "Missing togl widget in PlvBindToglToAlignmentViewCmd";
    return TCL_ERROR;
  }
  assert (0 == strcmp (argv[1], Togl_Ident (togl)));

  DisplayableMesh* mesh = FindMeshDisplayInfo (argv[2]);
  if (mesh == NULL && strcmp (argv[2], "")) {
    interp->result = "Missing mesh in PlvBindToglToAlignmentViewCmd";
    return TCL_ERROR;
  }

  //set that togl widget up to display the given mesh
  AlignmentToglInfo* ati = (AlignmentToglInfo*)Togl_GetClientData (togl);
  if (ati == NULL) {
    ati = new AlignmentToglInfo;
    ati->tb = new Trackball;
    Togl_SetClientData (togl, ati);
    Togl_SetDisplayFunc (togl, DrawAlignmentMesh);
    Togl_SetDestroyFunc (togl, FreeAlignmentInfo);
    Togl_SetTimerFunc (togl, SpinToglTrackball);
  }

  ati->SetTargetMesh (mesh);

  Togl_PostRedisplay (togl);

  return TCL_OK;
}


int
PlvRegUIMouseCmd (ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[])
{
  // argument string: togl button x y time [start|stop]
  if (argc < 6) {
    interp->result = "Bad arguments to PlvRegUIMouseCmd";
    return TCL_ERROR;
  }

  struct Togl* togl = toglHash.FindTogl (argv[1]);
  if (togl == NULL) {
    interp->result = "Missing togl widget in PlvRegUIMouseCmd";
    return TCL_ERROR;
  }

  int button = atoi (argv[2]);
  int mouseX = atoi (argv[3]);
  int mouseY = atoi (argv[4]);
  int eventT = atoi (argv[5]);
  bool up = TRUE;
  if (argc > 6 && !strcmp (argv[6], "start"))
    up = FALSE;

  AlignmentToglInfo* ati = (AlignmentToglInfo*)Togl_GetClientData (togl);
  assert (ati != NULL);

  if (button == 0)
    ati->tb->move (mouseX, mouseY, eventT);
  else
    ati->tb->pressButton (button, up, mouseX, mouseY, eventT);

  Togl_PostRedisplay (togl);

  return TCL_OK;
}


int
PlvAddPartialRegCorrespondenceCmd (ClientData clientData, Tcl_Interp *interp,
				   int argc, char *argv[])
{
  //argument string: togl overviewTogl x y
  if (argc < 5) {
    interp->result = "Bad arguments to PlvAddPartialRegCorrespondenceCmd";
    return TCL_ERROR;
  }

  struct Togl* togl = toglHash.FindTogl (argv[1]);
  struct Togl* toglOV = toglHash.FindTogl (argv[2]);

  if (togl == NULL) {
    interp->result = "Missing togl widget in PlvCreateRegCorrespondenceCmd";
    return TCL_ERROR;
  }

  int x = atoi (argv[3]);
  int y = Togl_Height (togl) - atoi (argv[4]);

  AlignmentToglInfo* ati = (AlignmentToglInfo*)Togl_GetClientData (togl);
  assert (ati != NULL);
  AlignmentOverviewInfo* aoi =
    (AlignmentOverviewInfo*)Togl_GetClientData (toglOV);

  Pnt3 pt;

  if (!findZBufferNeighborExt (x, y, pt, togl, ati->tb, 300)) {
    // allow removal of one point from an existing correspondence,
    // if it has more than two points
    CorrespondenceList* clSelected = aoi->GetSelectedCorrespondence();
// STL Update
    vector<CorrespondencePt>::iterator* cpOld;
    if (clSelected != NULL) {
      if (clSelected->points.size() > 2)
// STL Update
        cpOld = clSelected->FindPoint (ati->meshDisplay);
      else
	printf ("Cannot delete last two points from correspondence\n");
    }

    if (cpOld != NULL) { // prompt to delete point
      Tcl_VarEval (interp, "tk_dialog .reg.confirm {Delete point?} "
		   "{Delete meshset ",
		   ati->meshDisplay->getName(),
		   " from correspondence?} {} 0 No Yes", NULL);
      if (atoi (interp->result) != 0) {
// STL Update
	clSelected->DeletePoint(*cpOld);
	Tcl_Eval (interp, "rebuildSelectedCorrespondenceString");
	DrawAlignmentMesh (togl);
      }
    }

    interp->result = "";
    return TCL_OK;
  }

  // add to temporary correspondence set
  CorrespondenceList* cl = aoi->AddCorrespondence (ati->meshDisplay, pt);
  // if modifying existing corresp, tell tcl
  if (cl->id != 0) {
    Tcl_Eval (interp, "rebuildSelectedCorrespondenceString");
  }

  // draw new point hilited
  DrawAlignmentMeshToBack (togl);
  DrawAlignmentPoints (togl);
  Togl_SwapBuffers (togl);

  interp->result = (char*)ati->meshDisplay->getName();
  return TCL_OK;
}


int
PlvDeleteRegCorrespondenceCmd (ClientData clientData, Tcl_Interp *interp,
			       int argc, char *argv[])
{
  if (argc != 3) {
    interp->result = "Bad arguments to PlvDeleteRegCorrespondenceCmd";
    return TCL_ERROR;
  }

  struct Togl* toglOV = toglHash.FindTogl (argv[1]);
  if (!toglOV) {
    interp->result = "Missing overview togl widget in DeleteRegCorrespondence";
    return TCL_ERROR;
  }
  AlignmentOverviewInfo* aoi =
    (AlignmentOverviewInfo*)Togl_GetClientData (toglOV);
  assert (aoi != NULL);

  int iCorresp = atoi (argv[2] + 1);  // in format [nnn] asdfasd

// STL Update
  for (vector<CorrespondenceList*>::iterator cl = aoi->correspondences.begin();
       cl < aoi->correspondences.end();
       cl++) {
    if ((*cl)->id == iCorresp) {
      aoi->correspondences.erase (cl);
      return TCL_OK;
    }
  }

  interp->result = "Index not found in PlvDeleteRegCorrespondenceCmd";
  return TCL_ERROR;
}


int
PlvConfirmRegCorrespondenceCmd (ClientData clientData, Tcl_Interp *interp,
				int argc, char *argv[])
{
  if (argc < 2) {
    interp->result = "Bad arguments to PlvConfirmRegCorrespondenceCmd";
    return TCL_ERROR;
  }

  struct Togl* toglOV = toglHash.FindTogl (argv[1]);
  if (!toglOV) {
    interp->result = "Missing overview togl widget in CreateRegCorrespondence";
    return TCL_ERROR;
  }
  AlignmentOverviewInfo* aoi =
    (AlignmentOverviewInfo*)Togl_GetClientData (toglOV);
  assert (aoi != NULL);

  bool bKeep = true;
  if (argc > 2 && 0 == strcmp (argv[2], "delete"))
    bKeep = false;

  char text[2000];
  CorrespondenceList* cl = aoi->CompleteCorrespondence (bKeep, text);

  Tcl_SetResult (interp, text, TCL_VOLATILE);
  return TCL_OK;
}


int
PlvGetCorrespondenceInfoCmd (ClientData clientData, Tcl_Interp *interp,
			     int argc, char *argv[])
{
  if (argc != 3) {
    interp->result = "Bad arguments to PlvGetCorrespondenceInfoCmd";
    return TCL_ERROR;
  }

  struct Togl* toglOV = toglHash.FindTogl (argv[1]);
  if (toglOV == NULL) {
    interp->result = "Missing overlay togl in PlvGetCorrespondenceInfoCmd";
    return TCL_ERROR;
  }

  int idCorresp = atoi (argv[2]);
  if (idCorresp == 0) {
    interp->result = "Bad correspondence id in PlvGetCorrespondenceInfoCmd";
    return TCL_ERROR;
  }

  AlignmentOverviewInfo* aoi =
    (AlignmentOverviewInfo*)Togl_GetClientData (toglOV);
  assert (aoi != NULL);
  CorrespondenceList* cl = aoi->GetCorrespondenceById (idCorresp);
  if (cl == NULL) {
    interp->result = "Missing correspondence in PlvGetCorrespondenceInfoCmd";
    return TCL_ERROR;
  }

  char text[2000];
  cl->ToString (text);
  Tcl_SetResult (interp, text, TCL_VOLATILE);

  return TCL_OK;
}



static int CorrespRegOneMesh (AlignmentOverviewInfo* aoi,
			      DisplayableMesh* meshFrom,
			      DisplayableMesh* meshTo);
static int CorrespRegAllToAll (AlignmentOverviewInfo* aoi);


int
PlvCorrespondenceRegistrationCmd (ClientData clientData, Tcl_Interp *interp,
				  int argc, char *argv[])
{
  // arguments are: overviewTogl mode from to
  // from and to are irrelevant for all2all mode, but must exist
  if (argc != 5) {
    interp->result = "Bad arguments to PlvCorrespondenceRegistrationCmd";
    return TCL_ERROR;
  }

  // get correspondence list
  struct Togl* toglOV = toglHash.FindTogl (argv[1]);
  if (toglOV == NULL) {
    interp->result = "Missing overview togl in PlvCorrespRegistrationCmd";
    return TCL_ERROR;
  }
  AlignmentOverviewInfo* aoi = (AlignmentOverviewInfo*)
				Togl_GetClientData (toglOV);
  assert (aoi != NULL);

  // get mode
  enum { from2to, from2all, all2all, error } mode = error;
  if (!strcmp (argv[2], "from2to")) {
    mode = from2to;
  } else if (!strcmp (argv[2], "from2all")) {
    mode = from2all;
  } else if (!strcmp (argv[2], "all2all")) {
    mode = all2all;
  }

  switch (mode) {
  case all2all:
    return CorrespRegAllToAll (aoi);

  case from2to:
  case from2all:
    {
      DisplayableMesh* meshFrom = FindMeshDisplayInfo (argv[3]);
      if (!meshFrom) {
	interp->result = "Missing 'from' mesh in PlvCorrespRegistrationCmd";
	return TCL_ERROR;
      }

      if (mode == from2all)
	return CorrespRegOneMesh (aoi, meshFrom, NULL);
      else {
	DisplayableMesh* meshTo = FindMeshDisplayInfo (argv[4]);
	if (!meshTo) {
	  interp->result = "Missing 'to' mesh in PlvCorrespRegistrationCmd";
	  return TCL_ERROR;
	}
	return CorrespRegOneMesh (aoi, meshFrom, meshTo);
      }
    }
  }

  interp->result = "Bad mode passed to PlvCorrespRegistrationCmd";
  return TCL_ERROR;
}


static int
CorrespRegOneMesh (AlignmentOverviewInfo* aoi, DisplayableMesh* meshFrom,
		   DisplayableMesh* meshTo)
{
  // pass meshTo as NULL to use all correspondences involving meshFrom
  // (from2all), or meshTo as a specific mesh to use only correspondences
  // between meshFrom and meshTo (from2to).

  RigidScan* msFrom = meshFrom->getMeshData();
  RigidScan* msTo = meshTo->getMeshData();

  // build lists of points from these meshes
  // transform points to world coordinates
  vector<Pnt3> srcPt, dstPt;
  for (int i = 0; i < aoi->correspondences.size(); i++) {
    CorrespondenceList* cl = aoi->correspondences[i];

// STL Update
    vector<CorrespondencePt>::iterator* cpThis = cl->FindPoint (meshFrom);
    if (cpThis != NULL) { // references this mesh
      for (vector<CorrespondencePt>::iterator cpOther = cl->points.begin();
	   cpOther < cl->points.end();
	   cpOther++) {
	// don't add us-us to list
	if (*cpThis == cpOther)
	  continue;
	// and if from2to mode, only add corresp. involving To mesh
	if (meshTo != NULL && cpOther->mesh != meshTo)
	  continue;

	srcPt.push_back ((*cpThis)->pt);
	(*cpThis)->mesh->getMeshData()->xformPnt (srcPt.back());
	dstPt.push_back (cpOther->pt);
	cpOther->mesh->getMeshData()->xformPnt (dstPt.back());
      }
    }
  }

  double q[7];
  horn_align (&srcPt[0], &dstPt[0], srcPt.size(), q);
  Xform<float> xf = msFrom->getXform();
  xf.addQuaternion (q, 7);
  msFrom->setXform (xf);
  theScene->computeBBox();

  return TCL_OK;
}


static int
CorrespRegAllToAll (AlignmentOverviewInfo* aoi)
{
  GlobalReg gr;
  assert (aoi != NULL);

  // for each pair of meshes, find all correspondences involving them
  // and add pairs to globalreg gr

  // so: iterate mesh pairs
  int numSets = theScene->meshSets.size();
  for (int iFrom = 0; iFrom < numSets; iFrom++) {
    for (int iTo = iFrom + 1; iTo < numSets; iTo++) {
      DisplayableMesh* mdFrom = theScene->meshSets[iFrom];
      DisplayableMesh* mdTo = theScene->meshSets[iTo];
      RigidScan* msFrom = mdFrom->getMeshData();
      RigidScan* msTo = mdTo->getMeshData();
      vector<Pnt3> pntFrom;
      vector<Pnt3> pntTo;

      // then iterate correspondences
      for (int i = 0; i < aoi->correspondences.size(); i++) {
	CorrespondenceList* cl = aoi->correspondences[i];

	// if correspondence mentions both meshes currently under consideration
// STL Update
	vector<CorrespondencePt>::iterator* cpFrom = cl->FindPoint (mdFrom);
	vector<CorrespondencePt>::iterator* cpTo = cl->FindPoint (mdTo);

	if (cpFrom != NULL && cpTo != NULL) {
	  // then add points (in local coords) to pair data
	  pntFrom.push_back ((*cpFrom)->pt);
	  pntTo.push_back ((*cpTo)->pt);
	}
      }

      // now, if there were any correspondences between these meshes,
      // add to globalreg
      if (pntFrom.size() != 0) {
	gr.addPair(msFrom, msTo,
		   pntFrom, vector<Pnt3>(),
		   pntTo,   vector<Pnt3>());
	cout << pntFrom.size();
      } else {
	cout << "no";
      }

      cout << " correspondences between "
	   << mdFrom->getName() << " and " << mdTo->getName()
	   << endl;
    }
  }

  gr.align (GLOBALREG_TOLERANCE, theScene->worldBbox());
  theScene->computeBBox();

  return TCL_OK;
}




//////////////////////////////////////////////////////////////////////
//
// 2nd method: drag-registration
// UI and implementation follows
//
//////////////////////////////////////////////////////////////////////


static void dragAlign (DisplayableMesh* mesh, AlignmentMethod method);

int
PlvDragRegisterCmd (ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[])
{
  // argument: name of mesh being registered (to all visible meshes)
  if (argc < 2) {
    interp->result = "Bad arguments to PlvDragRegisterCmd";
    return TCL_ERROR;
  }

  DisplayableMesh* ms = FindMeshDisplayInfo (argv[1]);
  if (ms == NULL) {
    interp->result = "Missing meshset in PlvDragRegisterCmd";
    return TCL_ERROR;
  }

  AlignmentMethod method = alignHorn;
  if (argc > 2) {
    if (       !strcmp (argv[2], "horn")) {
      method = alignHorn;
    } else if (!strcmp (argv[2], "chen")) {
      method = alignChenMedioni;
    }
  }

  dragAlign (ms, method);
  DisplayCache::InvalidateToglCache (toglCurrent);

  return TCL_OK;
}


#define ISBACKGROUND(x)  ((x)>=zBackground)

class SceneBuf
{
public:
  SceneBuf (int w, int h)
    {
      width = w;
      height = h;

      z = new unsigned int[width*height];
      zBackground = 0xffffffff;

      pt = new Pnt3*[w];
      for (int x = 0; x < w; x++) {
	pt[x] = new Pnt3[h];
      }
    }

  ~SceneBuf()
    {
      delete[] z;
      for (int x = 0; x < width; x++) {
	delete[] pt[x];
      }
      delete[] pt;
    }

  unsigned int* z;
  unsigned int zBackground;
  Pnt3** pt;
  int width, height;
};


// BUGBUG this next function is now very similar to ReadZBufferRect...
// should merge code
SceneBuf*
GetScenePoints (DisplayableMesh* meshWithout = NULL)
{
  // allocate buffer storage
  Togl_MakeCurrent (toglCurrent);
  int width = Togl_Width (toglCurrent);
  int height = Togl_Height (toglCurrent);

  SceneBuf* sceneBuf = new SceneBuf (width, height);

  // clear z-buffer to get background z-value
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  unsigned int zBackground;
  glReadPixels (0, 0, 1, 1, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,
		&zBackground);
  sceneBuf->zBackground = zBackground;

  // draw scene
  if (meshWithout)
    meshWithout->setVisible (false);
  drawInToglBuffer (toglCurrent, GL_BACK);
  if (meshWithout)
    meshWithout->setVisible (true);

  // and collect z-buffer
  glReadBuffer (GL_BACK);
  glReadPixels (0, 0, width, height, GL_DEPTH_COMPONENT,
		GL_UNSIGNED_INT, sceneBuf->z);

  glMatrixMode (GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity ();
  tbView->applyXform();
  GLdouble model[16], proj[16];
  glGetDoublev(GL_MODELVIEW_MATRIX, model);    // applied from trackball
  glGetDoublev(GL_PROJECTION_MATRIX, proj);    // current GL state
  glPopMatrix();

  Xform<float> projMat = Xform<float>(proj) * Xform<float>(model);
  projMat.invert();

  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      int ofs = y*width + x;
      if (!ISBACKGROUND (sceneBuf->z[ofs])) {
	float tx = (2. * x / width) - 1;
	float ty = (2. * y / height) - 1;
	float tz = (2. * sceneBuf->z[ofs] / (float)zBackground) - 1;
	sceneBuf->pt[x][y] = projMat.unproject (tx, ty, tz);
      }
      else
	sceneBuf->pt[x][y] = Pnt3(0.0);
    }
  }

  return sceneBuf;
}


void
GetMeshSceneOverlapPoints (DisplayableMesh* meshDisplay,
			   const SceneBuf& sceneBuf,
			   vector<Pnt3>& ptMesh,
			   vector<Pnt3>& ptScene,
			   vector<int>* ptSceneOfs)
{
  // already have rendering of scene without mesh
  // render mesh alone, to back buffer and collect z information

  // allocate buffer storage
  Togl_MakeCurrent (toglCurrent);
  int width = Togl_Width (toglCurrent);
  int height = Togl_Height (toglCurrent);

  unsigned int* bufMesh = new unsigned int[width*height];

  // now draw mesh alone
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // but first, while buffer is clear, get background z-value
  unsigned int zBackground;
  glReadPixels (0, 0, 1, 1, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,
		&zBackground);

  glMatrixMode (GL_MODELVIEW);
  glPushMatrix();
  meshDisplay->getMeshData()->gl_xform();
  meshDisplay->drawList();
  glFinish();
  glPopMatrix();
  // and collect z-buffer
  glReadPixels (0, 0, width, height, GL_DEPTH_COMPONENT,
		GL_UNSIGNED_INT, bufMesh);


  glMatrixMode (GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity ();
  tbView->applyXform();
  GLdouble model[16], proj[16];
  glGetDoublev(GL_MODELVIEW_MATRIX, model);    // applied from trackball
  glGetDoublev(GL_PROJECTION_MATRIX, proj);    // current GL state
  glPopMatrix();

  Xform<float> projMat = Xform<float>(proj) * Xform<float>(model);
  projMat.invert();

  // iterate points that overlap, untransform and add to point lists
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      int ofs = y*width + x;
      if (!ISBACKGROUND (sceneBuf.z[ofs]) &&
	  !ISBACKGROUND (bufMesh[ofs])) {
	// time to unproject
	float tx = (2. * x / width) - 1;
	float ty = (2. * y / height) - 1;
	float tz = (2. * bufMesh[ofs] / (float)zBackground) - 1;
	Pnt3 unproj = projMat.unproject (tx, ty, tz);

	ptScene.push_back (sceneBuf.pt[x][y]);
	if (ptSceneOfs != NULL)
	  ptSceneOfs->push_back (ofs);
	ptMesh.push_back (unproj);

#if 0
	Pnt3 fast = projMat.unproject_fast (tx, ty, tz);
	SHOW (fast - unproj);
#endif
#if 0
	SHOW(ofs);
	SHOW(bufScene[ofs]);
	SHOW(bufMesh[ ofs]);
	SHOW(Pnt3(scenePos));
	SHOW(Pnt3( meshPos));
#endif
      }
    }
  }

  printf ("Have %ld overlapping points\n", ptScene.size());

  delete[] bufMesh;
}


static void
calculateNormals (vector<Pnt3>& normals, SceneBuf* sceneBuf,
		  const vector<int>& sampPos)
{
  Pnt3 norm;

  for (int i = 0; i < normals.size(); i++) {
    // calculate and average cross product of all (possibly 8)
    // triangles sharing this point and its neighbors
    norm = 0.0;

    // first, calculate ray from this point to each of its
    // 8 neighbors
    vector<Pnt3> rays;
    rays.reserve (normals.size());
    int ofs = sampPos[i];
    int ptX = ofs % sceneBuf->width;
    int ptY = ofs / sceneBuf->width;
    for (int ix = -1; ix <= 1; ix++) {
      for (int iy = -1; iy <= 1; iy++) {
	if (ix == 0 && iy == 0) continue;
	int x = ptX + ix; int y = ptY + iy;
	int iofs = y * sceneBuf->width + x;
	if (sceneBuf->z[iofs] >= sceneBuf->zBackground)
	  continue; // no data at this point

	rays.push_back (sceneBuf->pt[x][y] -
			sceneBuf->pt[ptX][ptY]);
      }
    }

    // now we have up to 8 rays from chosen point to its existing
    // neighbors
    // calculate cross products and add to norm
    for (int ray = 0; ray < rays.size(); ray++) {
      Pnt3 ray1 = rays[ray];
      Pnt3 ray2 = rays[(ray+1)%rays.size()];

      Pnt3 tn = (cross (ray1, ray2)).normalize();
      cout << "SubNormal " << ray << ":";
      SHOW (tn);

      norm += (cross (ray1, ray2)).normalize();
    }

    normals[i] = norm.normalize();
    SHOW(normals[i]);
  }
}


static void
dragAlign (DisplayableMesh* mesh, AlignmentMethod method)
{
  const int nIterations = 20;
  const int nPtsPerIter = 8;

  RigidScan* scan = mesh->getMeshData();
  Xform<float> orig = scan->getXform();
  Xform<float> best = orig;
  float bestCost = -1;

  // restrict to small drawing area to speed up buffer reading
  int oldResCap = theResCap;
  theResCap = 100;
  // calculate minimum allowed mesh overlap, in pixels
  int minOverlap = (theResCap * theResCap) * 0.01; // 1% overlap minimum

  printf ("Starting drag alignment (%d iterations)\n",
	  nIterations);

  SceneBuf* sceneBuf = GetScenePoints (mesh);

  // establish initial correspondences
  vector<Pnt3> ptOrigMesh, ptOrigScene;
  vector<int> ptSceneOfs;
  GetMeshSceneOverlapPoints (mesh, *sceneBuf,
			     ptOrigMesh, ptOrigScene,
			     method == alignChenMedioni
			     ? &ptSceneOfs : NULL);

  for (int iter = 0; iter < nIterations; iter++) {
    vector<Pnt3> ptMesh, ptScene;

    // randomly choose 8 points from overlap region
    // and align using them
    vector<Pnt3> subPtMesh(nPtsPerIter), subPtScene(nPtsPerIter);
    vector<int> subSampPos(nPtsPerIter);
    for (int iPt = 0; iPt < nPtsPerIter; iPt++) {
      int iPos = drand48() * ptOrigMesh.size();
      subPtMesh[iPt] =  ptOrigMesh [iPos];
      subPtScene[iPt] = ptOrigScene[iPos];
      if (ptSceneOfs.size() > iPt)
	subSampPos[iPt] = ptSceneOfs[iPos];
    }

    double q[7];
    Xform<float> current (orig);
    switch (method) {
    case alignChenMedioni: {
      vector<Pnt3> nrmScene (nPtsPerIter);
      calculateNormals (nrmScene, sceneBuf, subSampPos);
      chen_medioni (&subPtMesh[0], &subPtScene[0], &nrmScene[0],
		    nPtsPerIter, q);
      }
      break;

    case alignHorn:
    default:
      horn_align (&subPtMesh[0], &subPtScene[0], nPtsPerIter, q);
    }
    current.addQuaternion (q, 7);

    // rerender using new transformation, to evaluate it
    scan->setXform (current);
    // this is unnecessary for algorithm, but looks neat:
    drawInToglBuffer (toglCurrent, GL_FRONT);
    GetMeshSceneOverlapPoints (mesh, *sceneBuf, ptMesh, ptScene,
			       NULL);
    scan->setXform (orig);

    // if overlap is too small, reject this guess outright
    if (ptMesh.size() < minOverlap) {
      printf ("\tOverlap region too small (%ld points)\n",
	      ptMesh.size());
      continue;
    }

    // calc cost: average distance between corresponding points
    float cost = 0;
    for (int i = 0; i < ptMesh.size(); i++) {
      cost += dist (ptMesh[i], ptScene[i]);
    }
    cost /= ptMesh.size();
printf ("\tCost for %ld points is %g\n", ptMesh.size(), cost);

    // update best if this is better
    if (cost < bestCost || bestCost < 0) {
      best = current;
      bestCost = cost;
    }
  }

  // apply best transformation
  printf ("Best cost is %g\n", bestCost);
  scan->setXform (best);

  theResCap = oldResCap;
  delete sceneBuf;
}




//////////////////////////////////////////////////////////////////
//
// 3rd method: global registration
// code and UI follows
//
//////////////////////////////////////////////////////////////////


int
PlvGlobalRegistrationCmd (ClientData clientData, Tcl_Interp *interp,
			  int argc, char *argv[])
{
  if (argc < 2) {
    interp->result = "Bad argument to PlvGlobalRegistrationCmd";
    return TCL_ERROR;
  }

  GlobalReg* gr = theScene->globalReg;
  assert (gr != NULL);

  if (       !strcmp (argv[1], "reset")) {
    delete gr;
    theScene->globalReg = new GlobalReg;
  } else if (!strcmp (argv[1], "init_import")) {
    gr->initial_import();
  } else if (!strcmp (argv[1], "re_import")) {
    gr->perform_import();
  } else if (!strcmp (argv[1], "register")) {
    if (argc < 3) {
      interp->result = "Bad argument to PlvGlobalRegistrationCmd";
      return TCL_ERROR;
    }
    TbObj* scanToMove = NULL;   // default to registering all scans
    TbObj* scanToMoveTo = NULL; // default to registering to all scans
    if (argc > 3) {
      DisplayableMesh* dm = FindMeshDisplayInfo (argv[3]);
      if (dm != NULL) {
	scanToMove = dm->getMeshData();
      }
      if (scanToMove == NULL) {
	cerr << "Scan " << argv[3] << " does not exist." << endl;
	interp->result =
	  "Bad mesh passed to GlobalReg 1->all align";
	return TCL_ERROR;
      }
    }

    if (argc > 4) {
      DisplayableMesh* dm = FindMeshDisplayInfo (argv[4]);
      if (dm != NULL) {
	scanToMoveTo = dm->getMeshData();
      }
      if (scanToMoveTo == NULL) {
	cerr << "Scan " << argv[4] << " does not exist." << endl;
	interp->result =
	  "Bad mesh passed to GlobalReg 1->2 align";
	return TCL_ERROR;
      }
    }

    // to put in code for normalizing, comment out this align call
    // and call the one below it
    gr->align (atof(argv[2]),
	       scanToMove, scanToMoveTo);
    //gr->align (atof(argv[2]), theScene->worldBbox(),
    //       scanToMove, scanToMoveTo);
    theScene->computeBBox();

  } else if (!strcmp (argv[1], "pairstatus")) {
    if (argc < 4) {
      interp->result = "Bad # args";
      return TCL_ERROR;
    }
    DisplayableMesh* dm1 = FindMeshDisplayInfo (argv[2]);
    DisplayableMesh* dm2 = FindMeshDisplayInfo (argv[3]);
    if (!dm1 || !dm2) {
      //interp->result = "Bad mesh specified - sczRegCmds.cc:PlvGlobalRegistrationCmd pairstatus";
      //      rihan 7/9/01
      interp->result = "0";
    } else {
      bool bTrans = false;
      if (argc > 4 && !strcmp (argv[4], "transitive"))
	bTrans = true;
      bool manual;
      bool reg = gr->pairRegistered (dm1->getMeshData(),
				     dm2->getMeshData(),
				     manual,
				     bTrans);
      interp->result = reg ? "1" : "0";
    }
  } else if (!strcmp (argv[1], "listpairsfor")) {
    if (argc < 3) {
      interp->result = "Bad # args";
      return TCL_ERROR;
    }
    DisplayableMesh* dm = FindMeshDisplayInfo (argv[2]);
    if (!dm) {
      interp->result = "Bad mesh specified - sczRegCmds.cc:PlvGlobalRegistrationCmd listpairsfor";
      return TCL_ERROR;
    }

    bool bTrans = false;
    if (argc > 3 && !strcmp (argv[3], "transitive"))
      bTrans = true;
    crope partners = gr->list_partners(dm->getMeshData(), bTrans);
    Tcl_SetResult (interp, (char*)partners.c_str(), TCL_VOLATILE);

  } else if (!strcmp (argv[1], "dumpallpairs")) {
    std::string tcl_cmd;
    if (argc > 3) {
      float showThresh = atof(argv[3]);
      tcl_cmd = gr->dump_meshpairs(atoi(argv[2]),
				    showThresh);
    } else {
      tcl_cmd = gr->dump_meshpairs(atoi(argv[2]));
    }
    Tcl_SetResult (interp, (char*) tcl_cmd.c_str(), TCL_VOLATILE);

  } else if (!strcmp (argv[1], "getpaircount")) {
    if (argc < 3) {
      interp->result = "bad # args";
      return TCL_ERROR;
    }
    DisplayableMesh* dm = FindMeshDisplayInfo (argv[2]);
    if (!dm) {
      interp->result = "Bad mesh specified - sczRegCmds.cc:PlvGlobalRegistrationCmd getpaircount";
      return TCL_ERROR;
    }

    int nPartners = gr->count_partners (dm->getMeshData());
    char buf[10];
    sprintf (buf, "%d", nPartners);
    Tcl_SetResult (interp, buf, TCL_VOLATILE);

  } else if (!strcmp (argv[1], "groupstatus")) {
    gr->dump_connected_groups();

  } else if (!strcmp (argv[1], "killpair")) {
    if (argc < 4) {
      interp->result =
	"Bad args to PlvGlobalRegistrationCmd killpair";
      return TCL_ERROR;
    }
    DisplayableMesh* dm1 = FindMeshDisplayInfo (argv[2]);
    DisplayableMesh* dm2 = FindMeshDisplayInfo (argv[3]);
    // "mesh *" is allowed to leave dm2 NULL and delete
    // all involving mesh
    if (!dm1 || (!dm2 && argv[3][0] != '*')) {
      interp->result =
	"Bad mesh in PlvGlobalRegistrationCmd killpair";
      return TCL_ERROR;
    }
    if (dm2)
      gr->deletePair (dm1->getMeshData(),
		      dm2->getMeshData(), true);
    else
      gr->deleteAllPairs (dm1->getMeshData(), true);

  } else if (!strcmp (argv[1], "point_pair_count")) {
    if (argc < 4) {
      interp->result =
	"Bad args to PlvGlobalRegistrationCmd point_pair_count";
      return TCL_ERROR;
    }
    DisplayableMesh* dm1 = FindMeshDisplayInfo (argv[2]);
    DisplayableMesh* dm2 = FindMeshDisplayInfo (argv[3]);
    // "mesh *" is allowed to leave dm2 NULL and deals with
    // all involving meshes
    if (!dm1 || (!dm2 && argv[3][0] != '*')) {
      interp->result =
	"Bad mesh in PlvGlobalRegistrationCmd point_pair_count";
      return TCL_ERROR;
    }
    if (dm2)  gr->showPointpairCount(dm1->getMeshData(),
				     dm2->getMeshData());
    else      gr->showPointpairCount(dm1->getMeshData());

  } else if (!strcmp (argv[1], "deleteautopairs")) {
    if (argc < 3 || argc > 4) {
      interp->result =
	"Bad args to PlvGlobalRegistrationCmd deleteautopairs";
      return TCL_ERROR;
    }
    TbObj* tb = NULL;
    if (argc == 4) {
      DisplayableMesh* dm = FindMeshDisplayInfo (argv[3]);
      if (!dm) {
	interp->result =
	  "Bad mesh in PlvGlobalRegistrationCmd deleteautopairs";
	return TCL_ERROR;
      }
      tb = dm->getMeshData();
    }
    gr->deleteAutoPairs(atof(argv[2]), tb);

  } else if (!strcmp (argv[1], "getstats")) {
    DisplayableMesh *dm1 = NULL, *dm2 = NULL;
    if (argc > 3) {
      dm1 = FindMeshDisplayInfo (argv[2]);
      dm2 = FindMeshDisplayInfo (argv[3]);
    }
    if (!dm1 || !dm2) {
      interp->result = "Bad mesh passed to getstats";
      return TCL_ERROR;
    }

    int iQuality, nPoints;
    time_t date;
    bool bManual;
    float errs[5];
    if (!gr->getPairingStats (dm1->getMeshData(), dm2->getMeshData(),
			      bManual, iQuality, nPoints, date, errs)) {
      interp->result = "Can't get stats for given mesh";
      return TCL_ERROR;
    }

    struct tm* pTime = localtime (&date);
    char szStats[200];
    sprintf (szStats, "%c %d %d %.3f %.3f %.3f %.3f %.3f %4d/%02d/%02d",
	     bManual ? 'M' : 'A',
	     iQuality, nPoints,
	     errs[0], errs[1], errs[2], errs[3], errs[4],
	     1900 + pTime->tm_year, 1 + pTime->tm_mon, pTime->tm_mday);
    Tcl_SetResult (interp, szStats, TCL_VOLATILE);

  } else if (!strcmp (argv[1], "getstatsummary")) {

    int qual[4];    // unknown/bad/fair/good
    float err[3];   // min/avg/max
    int count[3];   // total/man/auto

    DisplayableMesh* dm = NULL;
    if (argc > 2)
      dm = FindMeshDisplayInfo (argv[2]);
    if (!dm && 0 != strcmp (argv[2], "*")) {
      interp->result = "Bad mesh passed to getstatsummary";
      return TCL_ERROR;
    }

    TbObj* tb = dm ? dm->getMeshData() : NULL;

    // TODO
    GlobalReg::ERRMETRIC metric = GlobalReg::errmetric_pnt;

    if (!gr->getPairingSummary (tb, metric, count, err, qual)) {
      interp->result = "Can't get summary for given mesh";
      return TCL_ERROR;
    }

    char szStats[200];
    sprintf (szStats, "%d %d %d %.3f %.3f %.3f %d %d %d %d",
	     count[0], count[1], count[2],
	     err[0], err[1], err[2],
	     qual[0], qual[1], qual[2], qual[3]);
    Tcl_SetResult (interp, szStats, TCL_VOLATILE);

  } else {
    interp->result = "Bad argument to PlvGlobalRegistrationCmd";
    return TCL_ERROR;
  }

  return TCL_OK;
}

//////////////////////////////////////////////////////////////////
//
// 4th method: ICP
// code and UI follows
//
//////////////////////////////////////////////////////////////////


ICP<RigidScan*> icp;

int
PlvRegIcpCmd(ClientData clientData, Tcl_Interp *interp,
	  int argc, char *argv[])
{
  if (argc != 14) {
    interp->result = "Usage: plv_icpregister \n"
      "\t<sampling density [0,1]>\n"
      "\t<normal-space sampling {0|1}>\n"
      "\t<number of iterations [1,20]>\n"
      "\t<culling percentage [0,99]>\n"
      "\t<no boundary targets {0|1}>\n"
      "\t<optimization method {point|plane}>\n"
      "\t<source mesh>\n"
      "\t<target mesh>\n"
      "\t<edge threshold kind {abs|rel}>\n"
      "\t<edge threshold value>\n"
      "\t<save results for globalreg {0|1}>\n"
      "\t<save at most n pairs [0,a_big_number]>\n"
      "\t<quality rating [0..3]>\n";
    return TCL_ERROR;
  }

  DisplayableMesh *mdSrc = FindMeshDisplayInfo(argv[7]);
  DisplayableMesh *mdTrg = FindMeshDisplayInfo(argv[8]);
  assert(mdSrc && mdTrg);

  RigidScan* mSrc = mdSrc->getMeshData();
  RigidScan* mTrg = mdTrg->getMeshData();
  assert(mSrc && mTrg);

  // run the alignment
  icp.set(mSrc, mTrg);
  icp.allow_bdry = !atoi(argv[5]);
  float avgError = icp.align(atof(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]),
			     argv[6][1] == 'o', argv[9][0] == 'a',
			     atof(argv[10]));

  // if requested, add pairs to globalreg
  bool bSaveForGlobal = atoi (argv[11]);
  if (bSaveForGlobal) {
    assert (theScene->globalReg != NULL);
    vector<Pnt3> ptSrc, ptTrg, nrmSrc, nrmTrg;
    icp.get_pairs_for_global (ptSrc, nrmSrc, ptTrg, nrmTrg);

    if (ptSrc.size() || ptTrg.size()) {
      cout << "Aligned pairs saved for global registration: "
	   << mdSrc->getName() << " and "
	   << mdTrg->getName() << endl;

      int max_pairs = atoi(argv[12]);
      int quality = atoi(argv[13]);

      theScene->globalReg->addPair(mSrc, mTrg, ptSrc, nrmSrc,
				   ptTrg, nrmTrg,
				   mTrg->getXform().fast_invert() *
				   mSrc->getXform(),
				   true, max_pairs, true, quality);
    }
  }

  char szError[20];
  sprintf (szError, "%g", avgError);
  Tcl_SetResult (interp, szError, TCL_VOLATILE);

  return TCL_OK;
}


int
PlvRegIcpMarkQualityCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[])
{
  if (argc != 4) {
    interp->result = "Bad args to PlvRegIcpMarkQualityCmd";
    return TCL_ERROR;
  }

  DisplayableMesh *mdSrc = FindMeshDisplayInfo(argv[1]);
  DisplayableMesh *mdTrg = FindMeshDisplayInfo(argv[2]);
  assert(mdSrc && mdTrg);

  RigidScan* mSrc = mdSrc->getMeshData();
  RigidScan* mTrg = mdTrg->getMeshData();
  assert(mSrc && mTrg);

  int quality = atoi (argv[3]);

  if (quality < 0 || quality > 3) {
    interp->result = "Quality out of range in PlvRegIcpMarkQualityCmd";
    return TCL_ERROR;
  }

  if (!theScene->globalReg->markPairQuality (mSrc, mTrg, quality)) {
    interp->result = "General failure in PlvRegIcpMarkQualityCmd";
    return TCL_ERROR;
  }

  return TCL_OK;
}


int
PlvShowIcpLinesCmd(ClientData clientData, Tcl_Interp *interp,
	  int argc, char *argv[])
{
  if (argc < 2) {
    interp->result = "Bad arguments to PlvShowIcpLinesCmd";
    return TCL_ERROR;
  }

  bool show = atoi (argv[1]);
  if (show)
    icp.show_lines();
  else
    icp.hide_lines();

  return TCL_OK;
}


int
SczAutoRegisterCmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[])
{
  if (argc < 7) {
    interp->result = "Usage: scz_auto_register \n"
      "\t[visible | current] [visible | all] [name_of_curr_mesh] "
      "[error_thresh] [nTargetPairs] [bPreserveExisting] [bNormSSample]\n";
    return TCL_ERROR;
  }

  // pair with visible/all meshes
  bool bFromVisible = !strcmp("visible", argv[1]);
  bool bToVisible   = !strcmp("visible", argv[2]);

  // error threshold
  float final_error_thresh = atof(argv[4]);

  if (final_error_thresh <= 0) {
    interp->result = "Error threshold has to be positive!\n";
    return TCL_ERROR;
  }

  // target number of pairs to yield from alignment
  int nTargetPairs = atoi (argv[5]);
  if (nTargetPairs <= 0)
    nTargetPairs = 1000;

  // overwrite existing correspondences, or ignore meshes that
  // are already  paired?
  bool bOverwrite = !atoi (argv[6]);

  bool bNormSSample = atoi(argv[7]);

  // ctor
  AutoICP<RigidScan*> auto_align (theScene->globalReg,
				  final_error_thresh);

  vector<DisplayableMesh*>& scans = theScene->meshSets;

  cout << "Calculating pairs:" << endl << endl;

  DisplayableMesh *currMesh = FindMeshDisplayInfo (argv[3]);
  if (!currMesh) {
    interp->result =
      "Couldn't find the current mesh!";
    return TCL_ERROR;
  }

// STL Update
  vector<DisplayableMesh*>::iterator begin = scans.begin();
  vector<DisplayableMesh*>::iterator end = scans.end();
  vector<DisplayableMesh*>::iterator first, second;

  GlobalReg *gr = theScene->globalReg;

  int cnt = 0;
  for (first = begin; first != end; first++) {
    if (( bFromVisible && (*first)->getVisible()) ||
	(!bFromVisible && *first == currMesh)) {
      for (second = begin; second != end; second++) {
	if (first == second) continue;

	if (bToVisible && !(*second)->getVisible()) continue;

	if (bFromVisible && (*second)->getVisible() &&
	    second < first) continue;

	RigidScan* rs1 = (*first)->getMeshData();
	RigidScan* rs2 = (*second)->getMeshData();

	bool manual;
	if (gr->pairRegistered(rs1, rs2, manual)) {
	  if (manual)      continue;
	  if (!bOverwrite) continue;
	}

	if (rs1->num_vertices() == 0 ||
	    rs2->num_vertices() == 0) {
	  continue;
	}

	cnt++;
      }
    }
  }

  bool bBail = false;
  Progress progress (cnt, "Calculating pairs");

  for (first = begin; first != end; first++) {
    if (( bFromVisible && (*first)->getVisible()) ||
	(!bFromVisible && *first == currMesh)) {
      for (second = begin; second != end; second++) {
	if (first == second) continue;

	if (bToVisible && !(*second)->getVisible()) continue;

	if (bFromVisible && (*second)->getVisible() &&
	    second < first) continue;

	RigidScan* rs1 = (*first)->getMeshData();
	RigidScan* rs2 = (*second)->getMeshData();

	// are these meshes already registered?
	// never override a manually registered pair...
	bool manual;
	if (theScene->globalReg->pairRegistered(rs1, rs2,
						manual)) {
	  if (manual) {
	    cerr << "Don't override manual alignment for "
		 << (*first)->getName()
		 << " and " << (*second)->getName() << endl;
	    continue;
	  }
	  if (!bOverwrite) {
	    cerr << "Already paired: " << (*first)->getName()
		 << " and " << (*second)->getName() << endl;
	    continue;
	  }
	}

	//
	// ok, passes all tests... register
	//

	// TODO: set threshold based on mesh resolutions

	if (rs1->num_vertices() == 0 ||
	    rs2->num_vertices() == 0) {
	  cout << "Skipping empty scan" << endl;
	  continue;
	}

	if (!progress.updateInc()) {
	  cerr << "Warning: cancelled; "
	       << "not all scan pairs considered." << endl;
	  bBail = true;
	  break;
	}

	// ok, try to align
	cout << "Trying " << (*first)->getName() << " and "
	     << (*second)->getName() << endl;

	// normal-space sample: bNormSSample
	if (auto_align.add_pair (rs1, rs2, nTargetPairs, bNormSSample)) {
	  cout << "Paired " << (*first)->getName()
	       << " and " << (*second)->getName() << endl;
	}
      }
    }

    if (bBail) break;
  }

  return TCL_OK;
}
