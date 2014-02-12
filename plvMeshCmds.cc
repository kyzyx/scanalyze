#include <stdlib.h>
#include "plvDrawCmds.h"
#include "plvGlobals.h"
#include "plvDraw.h"
#include "plvMeshCmds.h"
#include "plvScene.h"
#include "ply++.h"
#include "Mesh.h"
#include "RigidScan.h"
#include "togl.h"
#include "DisplayMesh.h"
#include "GroupUI.h"
#include "plvPlyCmds.h"
#include "GroupScan.h"
#include "GenericScan.h"
#include "ScanFactory.h"
#include "plvClipBoxCmds.h"

static RigidScan*
GetMeshFromCmd (Tcl_Interp* interp, int argc, char* argv[],
		int minArgC = 2, DisplayableMesh** pOutDisp = NULL,
		int iMeshName = 1)
{
  char buf [1000];

  if (argc < minArgC) {
    sprintf (buf, "%s: Wrong number of args", argv[0]);
    Tcl_SetResult (interp, buf, TCL_VOLATILE);
    return NULL;
  }

  DisplayableMesh* dispMesh = FindMeshDisplayInfo (argv[iMeshName]);
  if (pOutDisp != NULL)
    *pOutDisp = dispMesh;

  if (dispMesh == NULL) {
    sprintf (buf, "%s: Mesh '%s' does not exist", argv[0], argv[iMeshName]);
    Tcl_SetResult (interp, buf, TCL_VOLATILE);
    return NULL;
  }

  RigidScan* meshSet = dispMesh->getMeshData();
  assert (meshSet != NULL);

  return meshSet;
}


int
PlvMeshResolutionCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{
  DisplayableMesh* dm;
  RigidScan* scan = GetMeshFromCmd (interp, argc, argv, 3, &dm);
  if (!scan)
    return TCL_ERROR;

  bool ok = false;

  if (!strcmp (argv[2], "higher"))
    ok = scan->select_finer();
  else if (!strcmp (argv[2], "lower"))
    ok = scan->select_coarser();
  else if (!strcmp (argv[2], "highest"))
    ok = scan->select_finest();
  else if (!strcmp (argv[2], "lowest"))
    ok = scan->select_coarsest();
  else
    ok = scan->select_by_count (atoi (argv[2]));

  dm->invalidateCachedData();
  redraw (true);

  interp->result = ok ? "1" : "0";
  return TCL_OK;
}


int
PlvMeshDecimateCmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[])
{
  RigidScan* theSet = GetMeshFromCmd (interp, argc, argv, 3);
  if (!theSet)
    return TCL_ERROR;

  int targetSize = atoi(argv[2]);

  ResolutionCtrl::Decimator dec = ResolutionCtrl::decQslim;
  if (argc > 3) {
    if (!strcmp (argv[3], "qlsim"))
      dec = ResolutionCtrl::decQslim;
    else if (!strcmp (argv[3], "plycrunch"))
      dec = ResolutionCtrl::decPlycrunch;
  }

  int nPolys = theSet->create_resolution_absolute(targetSize, dec);

  char szPolys[20];
  sprintf (szPolys, "%d", nPolys);
  Tcl_SetResult (interp, szPolys, TCL_VOLATILE);

  if (nPolys)
    return TCL_OK;
  else
    return TCL_ERROR;
}

int
PlvMeshResListCmd(ClientData clientData, Tcl_Interp *interp,
	       int argc, char *argv[])
{
  char result[PATH_MAX], tempstr[PATH_MAX];

  RigidScan* theSet = GetMeshFromCmd (interp, argc, argv, 2);
  if (!theSet)
    return TCL_ERROR;

  bool bShort = false;
  if (argc > 2 && !strcmp (argv[2], "short"))
    bShort = true;

  vector<ResolutionCtrl::res_info> resList;
  theSet->existing_resolutions (resList);

  result[0] = '\0';
  for (int i = 0; i < resList.size(); i++) {
    if (bShort)
      sprintf (tempstr, "%d ", resList[i].abs_resolution);
    else
      sprintf (tempstr, "(%c%c):%d ",
	       resList[i].desired_in_mem ? 'A' : '-',
	       resList[i].in_memory ? 'L' : '-',
	       resList[i].abs_resolution);
    strcat(result, tempstr);
  }
  Tcl_SetResult(interp, result, TCL_VOLATILE);
  return TCL_OK;
}

int
PlvSetMeshResPreloadCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[])
{
  RigidScan* theSet = GetMeshFromCmd (interp, argc, argv, 4);
  if (!theSet)
    return TCL_ERROR;

  int nRes = atoi(argv[2]);
  bool bPreload = atoi(argv[3]);

  if (theSet->set_load_desired (nRes, bPreload))
    return TCL_OK;

  interp->result = "Specified resolution does not exist";
  return TCL_ERROR;
}


int
PlvCurrentResCmd(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[])
{
  char info[PATH_MAX];

  RigidScan* meshSet = GetMeshFromCmd (interp, argc, argv, 2);
  if (!meshSet)
  {
    interp->result = "Unable to find mesh in PlvCurrentResCmd";
    return TCL_ERROR;
  }

  if (!meshSet->num_resolutions())
  {
    interp->result = "Given mesh has no data in PlvCurrentResCmd";
    return TCL_ERROR;
  }

  ResolutionCtrl::res_info res = meshSet->current_resolution();
  sprintf(info, "%d", res.abs_resolution);
  Tcl_SetResult(interp, info, TCL_VOLATILE);
  return TCL_OK;
}


int
PlvMeshInfoCmd(ClientData clientData, Tcl_Interp *interp,
	       int argc, char *argv[])
{
  RigidScan* meshSet = GetMeshFromCmd (interp, argc, argv, 2);
  if (meshSet == NULL)
    return TCL_ERROR;

  if (argc > 2 && 0 == strcmp (argv[2], "graphical"))
    Tcl_SetResult(interp, (char *)meshSet->getInfo().c_str(),
		  TCL_VOLATILE);
  else
    cout << meshSet->getInfo() << endl;

  return TCL_OK;
}

int
PlvMeshDeleteResCmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[])
{
  RigidScan* scan = GetMeshFromCmd (interp, argc, argv, 3);
  if (scan == NULL)
    return TCL_ERROR;

  if (scan->num_resolutions() == 1) {
    interp->result = "Scan has only 1 resolution; can't be deleted";
    return TCL_ERROR;
  }
  int resLevel = atoi(argv[2]);
  if (!scan->delete_resolution (resLevel))
    return TCL_ERROR;

  //cout << "new #resolutions: " << scan->num_resolutions() << endl;
  return TCL_OK;
}


int
PlvMeshUnloadResCmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char* argv[])
{
  DisplayableMesh* dm;
  RigidScan* scan = GetMeshFromCmd (interp, argc, argv, 3, &dm);
  if (scan == NULL) {
    cerr << argv[0] << " does not exist!" << endl;
    return TCL_ERROR;
  }

  int resLevel = atoi(argv[2]);

  if (!scan->release_resolution (resLevel)) {
    cerr << "scan " << dm->getName()
	 << " failed to release resolution" << endl;
    return TCL_ERROR;
  }

  // scan released memory... which dm could have hold on
  dm->invalidateCachedData();

  return TCL_OK;
}

static void DeleteHelper (DisplayableMesh *mesh);

int
PlvMeshSetDeleteCmd(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[])
{
  DisplayableMesh* dispMesh;
  RigidScan* meshSet = GetMeshFromCmd (interp, argc, argv, 2, &dispMesh);
  if (meshSet == NULL)
    return TCL_ERROR;

  DeleteHelper(dispMesh);
  //theScene->deleteMeshSet (dispMesh);

  return TCL_OK;
}


static void
DeleteHelper (DisplayableMesh *mesh) {
  RigidScan *scan = mesh->getMeshData();
  GroupScan *group = dynamic_cast<GroupScan*>(scan);

  if (group) {
    vector<DisplayableMesh*> children = ungroupScans(mesh);

// STL Update
    for (vector<DisplayableMesh*>::iterator it = children.begin(); it < children.end(); it++) {
      DeleteHelper(*it);
    }

  } else {
    theScene->deleteMeshSet (mesh);
  }
}




int
PlvTransMeshCmd (ClientData clientData, Tcl_Interp *interp,
		 int argc, char* argv[])
{
  if (argc != 5) {
    printf ("%s: mesh x y z\n", argv[0]);
    return TCL_OK;
  }

  DisplayableMesh* dispMesh = FindMeshDisplayInfo (argv[1]);
  if (dispMesh == NULL) {
    printf ("Nonexistent mesh %s\n", argv[1]);
    return TCL_OK;
  }
  RigidScan* meshSet = dispMesh->getMeshData();
  assert (meshSet != NULL);

  meshSet->translate(atof(argv[2]), atof(argv[3]), atof(argv[4]));
  redraw (true);

  return TCL_OK;
}


int
PlvMeshRemoveStepCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{

#if 1
  printf ("Unimplemented!!!\n");
#endif

  return TCL_OK;
}

int
PlvScanColorCodeCmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[])
{
  DisplayableMesh* dispMesh;
  RigidScan* meshSet = GetMeshFromCmd (interp, argc, argv, 3, &dispMesh, 2);
  if (meshSet == NULL)
    return TCL_ERROR;

  if (!strcmp (argv[1], "set")) {
    unsigned int r, g, b;
    if (argc != 4 || 3 != sscanf (argv[3], "#%02x%02x%02x", &r, &g, &b)) {
      interp->result = "Bad color args.";
      return TCL_ERROR;
    }
    vec3uc color = {r, g, b};
    dispMesh->setFalseColor (color);

  } else {

    char color[20];
    const vec3uc& diffuse = dispMesh->getFalseColor();
    sprintf (color, "#%02x%02x%02x",
	     (unsigned)diffuse[0],
	     (unsigned)diffuse[1],
	     (unsigned)diffuse[2]);
    Tcl_SetResult (interp, color, TCL_VOLATILE);
  }

  return TCL_OK;
}


int
PlvFlipMeshNormalsCmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[])
{
  DisplayableMesh* mesh;
  RigidScan* scan = GetMeshFromCmd (interp, argc, argv, 2, &mesh);
  if (scan == NULL)
    return TCL_ERROR;

  scan->flipNormals();
  mesh->invalidateCachedData();

  redraw (true);
  return TCL_OK;
}


int
PlvBlendMeshCmd(ClientData clientData, Tcl_Interp *interp,
		int argc, char *argv[])
{
  DisplayableMesh* mesh;
  RigidScan* meshSet = GetMeshFromCmd (interp, argc, argv, 3, &mesh);
  if (meshSet == NULL)
    return TCL_ERROR;

  float alpha = atof (argv[2]);

  if (alpha > 0.0 && alpha < 1.0)
    mesh->setBlend (true, alpha);
  else
    mesh->setBlend (false, 1);

  redraw (true);
  return TCL_OK;
}


int
PlvGroupScansCmd(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[])
{
  if (argc < 3) {
    interp->result = "Bad args to PlvGroupScansCmd";
    return TCL_ERROR;
  }

  vector<DisplayableMesh*> members;

  if (!strcmp (argv[1], "create")) {

    for (int i = 3; i < argc-1; i++) {
      DisplayableMesh* dm = FindMeshDisplayInfo (argv[i]);
      if (!dm) {
	cerr << "No such mesh: " << argv[i] << endl;
	interp->result = "Missing scan in PlvGroupScansCmd";
	return TCL_ERROR;
      }
      members.push_back (dm);
    }
    bool dirty = (bool) atoi(argv[argc-1]);
    DisplayableMesh* group = groupScans (members, argv[2], dirty);

    if (!group) {
      interp->result = "The group could not be created.";
      return TCL_ERROR;
    }
    interp->result = (char*)group->getName();

  } else if (!strcmp (argv[1], "list")) {

    DisplayableMesh* mesh;
    if (!GetMeshFromCmd (interp, argc, argv, 3, &mesh, 2))
      return TCL_ERROR;

    GroupScan* group = dynamic_cast<GroupScan*> (mesh->getMeshData());

    crope names;
    if (group) {
      if (group->get_children_for_display (members)) {
	for (int i = 0; i < members.size(); i++) {
	  names += crope (" ") + members[i]->getName();
	}
      }
    }
    Tcl_SetResult (interp, (char*)names.c_str(), TCL_VOLATILE);

  } else if (!strcmp (argv[1], "break") || !strcmp (argv[1], "expand")) {

    DisplayableMesh* mesh;
    if (!GetMeshFromCmd (interp, argc, argv, 3, &mesh, 2))
      return TCL_ERROR;

    members = ungroupScans (mesh);
    if (!members.size()) {
      interp->result = "The scan could not be ungrouped -- perhaps "
	"it's not a group in the first place?";
      return TCL_ERROR;
    }

    crope names (members[0]->getName());
    for (int i = 1; i < members.size(); i++) {
      names += crope (" ") + members[i]->getName();
    }
    Tcl_SetResult (interp, (char*)names.c_str(), TCL_VOLATILE);

  } else if (!strcmp (argv[1], "isgroup")) {

      DisplayableMesh* dm = FindMeshDisplayInfo (argv[2]);
      if (!dm) {
	interp->result = "Missing scan in PlvGroupScansCmd";
	return TCL_ERROR;
      }
      RigidScan* scan = dm->getMeshData();
      vector<RigidScan*> children;
      if (scan->get_children (children)) {
	interp->result = "1";
      } else {
	interp->result= "0";
      }
      return TCL_OK;

  } else {
    interp->result = "Unrecognized option to PlvGroupScansCmd";
    return TCL_ERROR;
  }

  //cout << "Group: scene now has " << theScene->meshSets.size()
  //     << " meshes" << endl;

  return TCL_OK;
}


int
PlvHiliteScanCmd(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[])
{
  // unhilite previous selection in meshcontrols window
  for (int i = 0; i < g_hilitedScans.size(); i++)
    Tcl_VarEval (interp, "hiliteScanEntry ",
		 g_hilitedScans[i]->getName(),
		 " 0", NULL);

  // build new vector of currently hilited scans
  g_hilitedScans.clear();
  g_hilitedScans.reserve (argc - 1);

  for (int i = 1; i < argc; i++) {
    DisplayableMesh* dispMesh = FindMeshDisplayInfo (argv[i]);
    if (!dispMesh) {
      cerr << argv[0] << " can't find " << argv[i] << endl;
      interp->result = "Bad scan in PlvHiliteScanCmd";
      return TCL_ERROR;
    }

    g_hilitedScans.push_back (dispMesh);
  }

  // and hilite current selection in meshcontrols window
  for (int i = 0; i < g_hilitedScans.size(); i++)
    Tcl_VarEval (interp, "hiliteScanEntry ",
		 g_hilitedScans[i]->getName(),
		 " 1", NULL);

#ifndef no_overlay_support
  Togl_PostOverlayRedisplay (toglCurrent);
#else
  drawOverlayAndSwap(toglCurrent);
#endif

  return TCL_OK;
}


int
SczXformScanCmd (ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[])
{
  RigidScan* scan = GetMeshFromCmd (interp, argc, argv, 2);
  if (!scan)
    return TCL_ERROR;

  if (argc < 3) {
    interp->result = "Missing command in SczXformScanCmd";
    return TCL_ERROR;
  }

  if (!strcmp (argv[2], "reload")) {
    if (!scan->readXform (scan->get_basename())) {
      cerr << "Scan " << scan->get_name()
	   << " failed to read xform." << endl;
      interp->result = "Xform read failed!";
      return TCL_ERROR;
    }
  } else if (!strcmp (argv[2], "matrix")) {
    if (argc < 4) {
      interp->result = "Missing matrix!";
      return TCL_ERROR;
    }

    Xform<float> xfTo;
    if (!matrixFromString (argv[3], xfTo)) {
      interp->result = "Bad matrix!";
      return TCL_ERROR;
    }

    Xform<float> xfRelTo;
    if (argc > 4) {
      if (!strcmp (argv[4], "relative"))
	xfTo = xfTo * scan->getXform();
      else if(!matrixFromString (argv[4], xfRelTo)) {
	interp->result = "Bad matrix!";
	return TCL_ERROR;
      }
    }

    if (!xfRelTo.isIdentity()) {
      xfRelTo.invert();
      xfTo = xfTo * xfRelTo * scan->getXform();
    }

    scan->setXform (xfTo);
  }

  theScene->computeBBox();
  redraw();

  return TCL_OK;
}


int SczGetScanXformCmd(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[])
{
  RigidScan* scan = GetMeshFromCmd (interp, argc, argv, 2);
  if (!scan)
    return TCL_ERROR;

  if (argc < 3) {
    interp->result = "Missing command in SczXformScanCmd";
    return TCL_ERROR;
  }

  Xform<float> xf = scan->getXform();
  char buffer[1000] = "";

  if (!strcmp (argv[2], "matrix")) {
    char line[100];
    for (int i = 0; i < 4; i++) {
      sprintf (line, "%g %g %g %g\n",
	       xf(i,0), xf(i,1), xf(i,2), xf(i,3));
      strcat (buffer, line);
    }
  } else if (!strcmp (argv[2], "quat")) {
    float q[4], t[3];
    xf.toQuaternion (q);
    xf.getTranslation (t);
    sprintf (buffer, "%g %g %g %g\n%g %g %g\n",
	     q[0], q[1], q[2], q[3],
	     t[0], t[1], t[2]);
  } else if (!strcmp (argv[2], "axis")) {
    float phi, ax, ay, az, t[3];
    xf.get_rot (phi, ax, ay, az);
    xf.getTranslation (t);

    sprintf (buffer, "%g around %g %g %g\n%g %g %g\n",
	     phi * 180 / M_PI, ax, ay, az,
	     t[0], t[1], t[2]);
  } else {
    interp->result = "Bad command in SczGetScanXformCmd!";
    return TCL_ERROR;
  }

  Tcl_SetResult (interp, buffer, TCL_VOLATILE);
  return TCL_OK;
}

int
PlvSmoothMesh(ClientData clientData, Tcl_Interp *interp,
	      int argc, char *argv[])
{
  if (argc < 4) {
    interp->result = "plvSmoothMesh <meshName> <iterations> <maxMotion_mm>";
    return TCL_ERROR;
  }

  RigidScan* scan = GetMeshFromCmd (interp, argc, argv, 1);
  if (scan == NULL)
    {
      return TCL_ERROR;
    }

  int iterations = atoi(argv[2]);
  double maxMotion = atof(argv[3]);

  printf ("%s %d %f\n",argv[1],iterations,maxMotion);

  GenericScan* gscan=(GenericScan *)scan;
  gscan->dequantizationSmoothing(iterations,maxMotion);

  return TCL_OK;

}


#define MINARGCOUNT(n) \
  if (argc < n) { \
    _BadArgCount (argv[0], interp); \
    return TCL_ERROR; \
  }

void _BadArgCount (char* proc, Tcl_Interp* interp)
{
  char buf[200];

  sprintf (buf, "%s: not enough arguments\n", proc);
  Tcl_SetResult (interp, buf, TCL_VOLATILE);
}


int PolygonArea(vector<ScreenPnt> &pts)
{
  int i, j, area = 0;

  for (i=0; i<pts.size(); i++) {
    j = (i + 1) % pts.size();

    area += pts[i].x * pts[j].y;
    area -= pts[j].x * pts[i].y;
  }

  return area / 2;
}

//
// PlvRunExternalProgram
// ---------------------
// Tcl visible function that enables an external program to be run
// on a mesh.  Temporary files are used to transfer information to/from
// the external program.
//
int PlvRunExternalProgram(ClientData clientData, Tcl_Interp *interp,
			  int argc, char *argv[])
{
  char buf[256]; // temporary storage
  char clipBoxCoord[256];
  GLdouble mvmatrix[16], projmatrix[16];

  if (argc != 5) {
    interp->result = "plvRunExternalProgram <command> <name> "
                     "<output> <incBox>";
    return TCL_ERROR;
  }

  // easier access to command line parameters
  char *command = argv[1];
  char *inputMesh = argv[2];
  char *outputMesh = argv[3];
  int writeCBox = atoi(argv[4]);

  // parameter error checking
  if (!strlen(command) || !strlen(inputMesh) || !strlen(outputMesh)) {
    interp->result = "Please enter a command, inputMesh or outputMesh name";
    return TCL_ERROR;
  }

  if (writeCBox) {
    clipBoxCoord[0] = '\0';
    if (theSel.pts.size() < 3) {
      interp->result = "At least 3 points are needed for clipping information";
      return TCL_ERROR;
    }

    GLint viewport[4];
    vector<Pnt3> obj;
    vector<Pnt3> objNormForward;

    // information for unprojecting
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, mvmatrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projmatrix);

    for (int i=0; i<theSel.pts.size(); i++) {
      Unprojector unp;
      GLdouble objx, objy, objz, objFarX, objFarY, objFarZ;

      gluUnProject(theSel.pts[i].x, theSel.pts[i].y, 0,
		   mvmatrix, projmatrix, viewport, &objx, &objy, &objz);
      gluUnProject(theSel.pts[i].x, theSel.pts[i].y, 1,
		   mvmatrix, projmatrix, viewport, &objFarX, &objFarY, &objFarZ);

      Pnt3 v1(objFarX-objx, objFarY-objy, objFarZ-objz);
      v1.normalize();
      objNormForward.push_back(v1);

      Pnt3 t(objx, objy, objz);
      obj.push_back(t);
    }

    // print the list of information
    // get orientation of face vertices
    bool ccw = (PolygonArea(theSel.pts) > 0 ? true : false);

    int j;
    for (int i=0; i<theSel.pts.size(); i++) {
      char tempVertBuf[256];
      j = (i + 1) % theSel.pts.size();

      Pnt3 crs;
      Pnt3 up(obj[j][0] - obj[i][0], obj[j][1] - obj[i][1], obj[j][2] - obj[i][2]);

      if (!ccw)
	crs = cross(objNormForward[i], up);
      else
	crs = cross(up, objNormForward[i]);

      crs.normalize();

      // make the point list and concat it with the current clip region string
      sprintf(tempVertBuf, "%f %f %f %f %f %f ", (double)obj[i][0], (double)obj[i][1],
	      (double)obj[i][2], (double)crs[0], (double)crs[1], (double)crs[2]);

      strcat(clipBoxCoord, tempVertBuf);
    }
  }

  // get the mesh to send to output
  DisplayableMesh* dispMesh = FindMeshDisplayInfo (inputMesh);

  if (dispMesh == NULL) {
    sprintf (buf, "%s: Mesh '%s' does not exist", argv[0], inputMesh);
    Tcl_SetResult (interp, buf, TCL_VOLATILE);
    return TCL_ERROR;
  }

  // get the rigid scan from the mesh
  RigidScan* meshSet = dispMesh->getMeshData();
  if (meshSet == NULL) {
    sprintf (buf, "%s: Mesh '%s' does not exist", argv[0], inputMesh);
    Tcl_SetResult (interp, buf, TCL_VOLATILE);
    return TCL_ERROR;
  }

  // get a temporary name and convert it to a crope with extension ply
  crope inTName = tmpnam(NULL);
  inTName += ".ply";

  // write the highest resolution to the temporary file
  vector<ResolutionCtrl::res_info> resList;
  meshSet->existing_resolutions (resList);

  int finest = 0;
  for (int i=1; i<resList.size(); i++) {
    if (resList[finest].abs_resolution < resList[i].abs_resolution)
      finest = i;
  }

  // if we're clipping the object against some box, put the object in it's world space orientation
  // rather than just using it's default object space orientation.
  if (writeCBox)
    meshSet->write_resolution_mesh(resList[finest].abs_resolution, inTName, meshSet->getXform());
  else
    meshSet->write_resolution_mesh(resList[finest].abs_resolution, inTName);

  // get another temporary name
  crope outTName = tmpnam(NULL);
  outTName += ".ply";

  // execute the command
  char cmdBuffer[512];
  if (writeCBox)
    sprintf(cmdBuffer, "%s %s %s %s", command, inTName.c_str(), outTName.c_str(), clipBoxCoord);
  else
    sprintf(cmdBuffer, "%s %s %s ", command, inTName.c_str(), outTName.c_str());

  if (system (cmdBuffer)) {
    interp->result = "Command failed.";

    // delete the temporary files
    sprintf(cmdBuffer, "rm -f %s", inTName.c_str());
    system(cmdBuffer);
    sprintf(cmdBuffer, "rm -f %s", outTName.c_str());
    system(cmdBuffer);

    return TCL_ERROR;
  }

  // read in the output file
  RigidScan *newScan = CreateScanFromFile(outTName.c_str());
  if (newScan != NULL) {
    crope omName = outputMesh;
    omName += ".ply";

    newScan->set_name(omName);

    DisplayableMesh *dm = theScene->addMeshSet(newScan);

    sprintf(cmdBuffer, "clipMeshCreateHelper %s %s", inputMesh, outputMesh);
    Tcl_Eval(interp, cmdBuffer);
  } else {
    interp->result = "A filtered scan could not be created";

    // delete the temporary files
    sprintf(cmdBuffer, "rm -f %s", inTName.c_str());
    system(cmdBuffer);
    sprintf(cmdBuffer, "rm -f %s", outTName.c_str());
    system(cmdBuffer);

    return TCL_ERROR;
  }

  // delete the temporary files
  sprintf(cmdBuffer, "rm -f %s", inTName.c_str());
  system(cmdBuffer);
  sprintf(cmdBuffer, "rm -f %s", outTName.c_str());
  system(cmdBuffer);

  return TCL_OK;
}

