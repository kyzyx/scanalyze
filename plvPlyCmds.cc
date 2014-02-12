#include <stdlib.h>
#include <vector.h>
#include <ctype.h>
#include <fstream.h>

#include "plvImageCmds.h"
#include "plvGlobals.h"
#include "plvMeshCmds.h"
#include "plvPlyCmds.h"
#include "plvScene.h"
#include "ply++.h"
#include "RangeGrid.h"
#include "DisplayMesh.h"
#include "ScanFactory.h"
#include "GroupUI.h"
#include "GroupScan.h"

int
PlvIsRangeGridCmd(ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[])
{
  if (is_range_grid_file(argv[1]))
    interp->result = "1";
  else
    interp->result = "0";

  return TCL_OK;
}

int
PlvReadFileCmd(ClientData clientData, Tcl_Interp *interp,
	      int argc, char *argv[])
{
  if (argc < 2) {
    interp->result = "No filename specified in PlvReadFileCmd!";
    return TCL_ERROR;
  }

  RigidScan* scan = CreateScanFromFile (argv[1]);
  if (scan != NULL) {

    DisplayableMesh* dm = theScene->addMeshSet(scan);
    Tcl_SetResult(interp, (char*)dm->getName(), TCL_VOLATILE);
    return TCL_OK;

  } else {

    interp->result = "The given scan or set could not be loaded.";
    return TCL_ERROR;

  }
}

int
PlvReadGroupMembersCmd(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[])
{
  if (argc < 2) {
    interp->result = "No filename specified in PlvReadGroupMembersCmd!";
    return TCL_ERROR;
  }
  char buf[1000];
  ifstream fin (argv[1]);
  crope names = crope("");

  while (!fin.fail()) {
    fin.getline(buf, 1000);
    names += crope(buf) + crope(" ");
  }
  if (!strcmp(names.c_str(), "")) {
    interp->result = "Cannot load group";
    return TCL_ERROR;
  }
  interp->result = strdup(names.c_str());
  return TCL_OK;
}

int
PlvGetNextGroupNameCmd(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[])
{
  interp->result = getNextUnusedGroupName();
  return TCL_OK;
}

int
PlvSynthesizeObjectCmd(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[])
{
  if (argc < 2) {
    interp->result = "Bad arguments to PlvSynthesizeObjectCmd";
    return TCL_ERROR;
  }

  float size = atoi (argv[1]);
  if (size <= 0) {
    interp->result = "Bad size to PlvSynthesizeObjectCmd";
    return TCL_ERROR;
  }

  RigidScan* scan = CreateScanFromThinAir (size);
  DisplayableMesh* dm = theScene->addMeshSet (scan);
  Tcl_SetResult(interp, (char*)dm->getName(), TCL_VOLATILE);
  return TCL_OK;
}

int
PlvSaveCurrentGroup (ClientData clientData, Tcl_Interp *interp,
	       int argc, char *argv[])
{
  if (argc < 3) {
    interp->result = "Bad args to PlvSaveCurrentGroup";
    return TCL_ERROR;
  }

  // basename now contains name of the group
  DisplayableMesh* meshDisp = FindMeshDisplayInfo (argv[1]);
  RigidScan *scan = meshDisp->getMeshData();
  if (argc < 3) { // no name given
    if (scan->write())
      return TCL_OK;

  } else {
    //printf ("Attempting named write(%s)\n", argv[2]);
    if (scan->write (argv[2]))
      return TCL_OK;

    interp->result = "Unable to write Group.";
  }
  return TCL_ERROR;
}


int
PlvWriteScanCmd(ClientData clientData, Tcl_Interp *interp,
	       int argc, char *argv[])
{
  if (argc < 2) {
    interp->result = "Bad args to PlvWriteScanCmd";
    return TCL_ERROR;
  }

  DisplayableMesh* meshDisp = FindMeshDisplayInfo (argv[1]);
  if (!meshDisp) {
    interp->result = "Missing scan in PlvWriteScanCmd";
    return TCL_ERROR;
  }
  RigidScan* meshSet = meshDisp->getMeshData();

  if (argc < 3) { // no name given
    //printf ("Attempting write()\n");
    if (meshSet->write())
      return TCL_OK;

    interp->result = "unnamed";
  } else {
    //printf ("Attempting named write(%s)\n", argv[2]);
    if (meshSet->write (argv[2]))
      return TCL_OK;

    interp->result = "Unable to write scan.";
  }

  return TCL_ERROR;

#if 0
  int numMeshNames = atoi(argv[2]);

  int useColorNotTexture = EQSTR(argv[argc-1], "-tex_as_color") ||
    EQSTR(argv[argc-2], "-tex_as_color");

  int writeNormals = EQSTR(argv[argc-1], "-norm") ||
    EQSTR(argv[argc-2], "-norm");

  int numMeshes = MIN(numMeshNames, meshSet->num_resolutions());

  for (int i = 0; i < numMeshes; i++) {
    meshSet->getMesh(i)->writePlyFile(argv[i+3],
				      useColorNotTexture,
				      writeNormals);
  }
#endif
}


int PlvWriteMetaDataCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[])
{
  if (argc < 3) {
    interp->result = "Bad args to PlvWriteMetaDataCmd";
    return TCL_ERROR;
  }

  DisplayableMesh* meshDisp = FindMeshDisplayInfo (argv[1]);
  if (!meshDisp) {
    interp->result = "Missing scan in PlvWriteMetaDataCmd";
    return TCL_ERROR;
  }

  RigidScan::MetaData data;
  if (!strcmp (argv[2], "xform")) {
    data = RigidScan::md_xform;
  } else {
    interp->result = "Unrecognized metadata type";
    return TCL_ERROR;
  }

  RigidScan* scan = meshDisp->getMeshData();
  if (!scan->write_metadata (data)) {
    interp->result = "Scan was unable to write metadata";
    return TCL_ERROR;
  }

  return TCL_OK;
}


int PlvWriteResolutionMeshCmd(ClientData clientData, Tcl_Interp *interp,
			      int argc, char *argv[])
{
  if (argc < 4) {
    interp->result = "Bad args to PlvWriteResolutionMeshCmd";
    return TCL_ERROR;
  }

  DisplayableMesh* meshDisp = FindMeshDisplayInfo (argv[1]);
  if (!meshDisp) {
    interp->result = "Missing scan in PlvWriteResolutionMeshCmd";
    return TCL_ERROR;
  }
  RigidScan* scan = meshDisp->getMeshData();

  bool success = false;

  if (argc > 4 && !strcmp (argv[4], "flatten")) {
    success = scan->write_resolution_mesh (atoi (argv[2]), argv[3],
					   scan->getXform());
  } else if (argc > 4 && !strncmp (argv[4], "matrix", 6)) {
    Xform<float> mat;
    if (!matrixFromString (argv[4] + 7, mat)) {
      interp->result = "Bad matrix!";
      return TCL_ERROR;
    }

    success = scan->write_resolution_mesh (atoi (argv[2]), argv[3], mat);
  } else {
    success = scan->write_resolution_mesh (atoi (argv[2]), argv[3]);
  }

  if (!success) {
    interp->result = "Write failed";
    return TCL_ERROR;
  }

  return TCL_OK;
}


int PlvIsScanModifiedCmd(ClientData clientData, Tcl_Interp *interp,
			 int argc, char *argv[])
{
  if (argc != 2) {
    interp->result = "Bad args to PlvIsScanModifiedCmd";
    return TCL_ERROR;
  }

  DisplayableMesh* meshDisp = FindMeshDisplayInfo (argv[1]);
  if (!meshDisp) {
    interp->result = "Missing scan in PlvIsScanModifiedCmd";
    return TCL_ERROR;
  }
  RigidScan* scan = meshDisp->getMeshData();

  interp->result = scan->is_modified() ? "1" : "0";
  return TCL_OK;
}


int PlvGetScanFilenameCmd(ClientData clientData, Tcl_Interp *interp,
			  int argc, char* argv[])
{
  if (argc != 2) {
    interp->result = "Bad args to PlvGetScanFilenameCmd";
    return TCL_ERROR;
  }

  DisplayableMesh* meshDisp = FindMeshDisplayInfo (argv[1]);
  if (!meshDisp) {
    interp->result = "Missing scan in PlvGetScanFilenameCmd";
    return TCL_ERROR;
  }
  RigidScan* scan = meshDisp->getMeshData();

  interp->result = (char*)scan->get_name().c_str();
  return TCL_OK;
}


bool
matrixFromString (char* str, Xform<float>& xf)
{
  float mat[16];
  char* entry = str;
  for (int i = 0; i < 16; i++) {
    // seek to next digit
    while (entry && *entry && !isdigit(*entry)
	   && *entry != '-' && *entry != '.')
      ++entry;
    if (!entry || !*entry) {
      return false;
    }

    // extract value
    mat[i] = atof (entry);
    // and skip past
    entry = strchr (entry, ' ');
  }

  xf = Xform<float> (mat);

  return true;
}

