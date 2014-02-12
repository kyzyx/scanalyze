//############################################################
// plvMMCmds.cc
// Jeremy Ginsberg
// Fri Nov 13 18:08:16 CET 1998
//
// ModelMaker scan commands
//############################################################

#include <iostream.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include <fstream.h>

#include "Progress.h"
#include "plvMMCmds.h"
#include "DisplayMesh.h"
#include "MMScan.h"
#include "plvScene.h"
#include "RigidScan.h"
#include "plvDrawCmds.h"
#include "FileNameUtils.h"

int
MmsAbsorbXformCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{

  if ((argc != 3) && (argc != 2)) {
    printf("syntax: mms_absorbxform meshname [basexformname]\n");
    printf("        basexform name is an optional string used to\n");
    printf("        construct the names of each xform file. if none\n");
    printf("        is supplied, then the basename of the mesh is used.\n");
    return TCL_OK;
  }

  if (strlen(argv[1]) == 0) {
    printf("No mesh selected for mms_absorbxform.\n");
    return TCL_OK;
  }

  DisplayableMesh *dm = FindMeshDisplayInfo(argv[1]);
  assert (dm);

  RigidScan *theMesh = dm->getMeshData();
  assert (theMesh);

  MMScan *mm = dynamic_cast<MMScan *>(theMesh);
  if (!mm) {
    printf("Selected mesh is not a ModelMaker scan.\n");
    return TCL_OK;
  }

  // if we have an alternate basename, use it; otherwise use the mesh name
  if (argc == 3) mm->absorb_xforms(argv[2]);
    else mm->absorb_xforms(argv[1]);

  theScene->invalidateDisplayCaches();
  redraw(true);

  return TCL_OK;
}

int
MmsGetScanFalseColorCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[])
{

  if (argc != 3) {
    printf("need 3 args for mms_getscanfalsecolor.\n");
    return TCL_OK;
  }

  if (strlen(argv[1]) == 0) {
    printf("No mesh selected for mms_getscanfalsecolor.\n");
    return TCL_OK;
  }

  DisplayableMesh *dm = FindMeshDisplayInfo(argv[1]);
  assert (dm);

  RigidScan *theMesh = dm->getMeshData();
  assert (theMesh);

  MMScan *mm = dynamic_cast<MMScan *>(theMesh);

  char color[20];

  if (!mm) {
    printf("Selected mesh is not a ModelMaker scan.\n");
    sprintf(color, "#000000");
  }
  else {
    const vec3uc& diffuse = mm->getScanFalseColor(atoi(argv[2]));
    sprintf (color, "#%02x%02x%02x",
	   (unsigned)diffuse[0],
	   (unsigned)diffuse[1],
	   (unsigned)diffuse[2]);
  }
  Tcl_SetResult (interp, color, TCL_VOLATILE);
  return TCL_OK;
}


int
MmsNumScansCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[])
{

  if (argc != 2) {
    printf("need 2 args for mms_numscans.\n");
    return TCL_OK;
  }

  if (strlen(argv[1]) == 0) {
    printf("No mesh selected for mms_numscans.\n");
    return TCL_OK;
  }

  DisplayableMesh *dm = FindMeshDisplayInfo(argv[1]);
  assert (dm);

  RigidScan *theMesh = dm->getMeshData();
  assert (theMesh);

  MMScan *mm = dynamic_cast<MMScan *>(theMesh);

  char result[20];
  if (!mm) {
    printf("Selected mesh is not a ModelMaker scan.\n");
    return TCL_ERROR;
  }
  else {
    int numScans = mm->num_scans();
    sprintf (result, "%d", numScans);
  }
  Tcl_SetResult (interp, result, TCL_VOLATILE);
  return TCL_OK;
}

int
MmsVripOrientCmd(ClientData clientData, Tcl_Interp *interp,
	  int argc, char *argv[])
{
  if ((argc != 3) && (argc != 4)) {
    printf("usage: mms_vriporient meshname directoryname [-noxform]\n");
    return TCL_ERROR;
  }

  if (strlen(argv[1]) == 0) {
    printf("No mesh selected for VRIP alignment.\n");
    return TCL_OK;
  }

  DisplayableMesh *dm = FindMeshDisplayInfo(argv[1]);
  assert (dm);

  RigidScan *theMesh = dm->getMeshData();
  assert (theMesh);

  char* dir = argv[2];
  portable_mkdir (dir, 00775);

  MMScan *mm = dynamic_cast<MMScan *>(theMesh);
  if (!mm) {
    printf("Selected mesh is not a ModelMaker scan.\n");
    return TCL_OK;
  }

  crope confLines;
  int numFragsFound;

  printf("Auto-aligning meshes with Z-axis for VRIP...\n");
  if (argc == 4) {
    if (strcmp(argv[3], "-noxform") == 0) {
      printf("cancel that... no alignment with Z-axis, only saving pieces.\n");
      numFragsFound = mm->write_xform_ply(4, dir, confLines, false);
    }
  }
  else
     numFragsFound = mm->write_xform_ply(4, dir, confLines, true);

  cout << "Aligned " << numFragsFound << "modelmaker fragments for VRIP"
       << endl;

  printf("All done.\n");
  return TCL_OK;
}


int
PlvWriteMMForVripCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{
   if (argc < 3) {
      interp->result =
	 "Usage: plv_write_mm_for_vrip reslevel directory [-noxform]\n";
      return TCL_ERROR;
   }

   int iRes = atoi (argv[1]);
   char* dir = argv[2];
   portable_mkdir (dir, 00775);

   crope result;
   bool success = true;

   // vrip can't read tstrips
   char* oldStripVal = Tcl_GetVar (interp, "meshWriteStrips",
				   TCL_GLOBAL_ONLY);
   Tcl_SetVar (interp, "meshWriteStrips", "never",
	       TCL_GLOBAL_ONLY);

   Progress* prog = new Progress (theScene->meshSets.size() * 1000,
				  "Output plyfiles for vrip",
				  true);


// STL Update
   vector<DisplayableMesh*>::iterator dm = theScene->meshSets.begin();

   bool noXform = false;
   if ((argc > 3) && (strcmp(argv[3], "-noxform") == 0)) {
     printf("Not transforming scan fragments; simply writing ply files\n");
     noXform = true;
   }

   for (; dm < theScene->meshSets.end(); dm++) {
      if (!(*dm)->getVisible())
	 continue;

      RigidScan* rigidScan = (*dm)->getMeshData();
      MMScan* mmScan = dynamic_cast<MMScan*> (rigidScan);
      if (!mmScan)
	 continue;

      int numSweeps;

      // functionality to write out ply fragments without x-forms

      if (noXform)
	numSweeps = mmScan->write_xform_ply(iRes, dir, result, false);
      else
        numSweeps = mmScan->write_xform_ply(iRes, dir, result, true);

      if (!numSweeps) {
	 continue;
      }

      printf("Num meshes %d\n", numSweeps);

      /*

      for (int i = 0; i < numConfLines; i++) {
	 if (!result.empty())
	    result += " ";
	 //result += confLines[i];
	 cout << result << endl;
	 //delete confLines[i];
      }

      */

   }

   // cleanup
   delete prog;
   Tcl_SetVar (interp, "meshWriteStrips", oldStripVal,
	       TCL_GLOBAL_ONLY);

   if (!success)
      return TCL_ERROR;

   Tcl_SetResult (interp, (char*)result.c_str(), TCL_VOLATILE);
   return TCL_OK;
}


int
MmsIsScanVisibleCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{
  if (argc != 3) return TCL_ERROR;

  if (strlen(argv[1]) == 0) {
    printf("No mesh selected for MmsIsScanVisible()\n");
    return TCL_OK;
  }

  DisplayableMesh *dm = FindMeshDisplayInfo(argv[1]);
  assert (dm);

  RigidScan *theMesh = dm->getMeshData();
  assert (theMesh);

  MMScan *mm = dynamic_cast<MMScan *>(theMesh);
  if (!mm) {
    printf("Selected mesh is not a ModelMaker scan.\n");
    return TCL_OK;
  }

  int isVis = mm->isScanVisible(atoi(argv[2]));
  char result[20];
  sprintf(result, "%d", isVis);
  Tcl_SetResult (interp, result, TCL_VOLATILE);
  return TCL_OK;
}

int
MmsSetScanVisibleCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{
  if (argc != 4) return TCL_ERROR;

  if (strlen(argv[1]) == 0) {
    printf("No mesh selected for MmsSetScanVisible()\n");
    return TCL_OK;
  }

  DisplayableMesh *dm = FindMeshDisplayInfo(argv[1]);
  assert (dm);

  RigidScan *theMesh = dm->getMeshData();
  assert (theMesh);

  MMScan *mm = dynamic_cast<MMScan *>(theMesh);
  if (!mm) {
    printf("Selected mesh is not a ModelMaker scan.\n");
    return TCL_OK;
  }

  mm->setScanVisible(atoi(argv[2]), atoi(argv[3]));

  return TCL_OK;
}

int
MmsResetCacheCmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[])
{
  if (argc != 2) return TCL_ERROR;

  if (strlen(argv[1]) == 0) {
    printf("No mesh selected for MmsInvalidateCacheCmd\n");
    return TCL_OK;
  }

  DisplayableMesh *dm = FindMeshDisplayInfo(argv[1]);
  assert (dm);

  RigidScan *theMesh = dm->getMeshData();
  assert (theMesh);

  dm->invalidateCachedData();
  theMesh->computeBBox();
  return TCL_OK;
}


int
MmsDeleteScanCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{
  if (argc != 3) return TCL_ERROR;

  if (strlen(argv[1]) == 0) {
    printf("No mesh selected for MmsDeleteScan()\n");
    return TCL_OK;
  }

  DisplayableMesh *dm = FindMeshDisplayInfo(argv[1]);
  assert (dm);

  RigidScan *theMesh = dm->getMeshData();
  assert (theMesh);

  MMScan *mm = dynamic_cast<MMScan *>(theMesh);
  if (!mm) {
    printf("Selected mesh is not a ModelMaker scan.\n");
    return TCL_OK;
  }

  mm->deleteScan(atoi(argv[2]));
  dm->invalidateCachedData();

  return TCL_OK;
}

int
MmsFlipScanNormsCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{
  if (argc != 3) return TCL_ERROR;

  if (strlen(argv[1]) == 0) {
    printf("No mesh selected for MmsFlipScanNormsCmd()\n");
    return TCL_OK;
  }

  DisplayableMesh *dm = FindMeshDisplayInfo(argv[1]);
  assert (dm);

  RigidScan *theMesh = dm->getMeshData();
  assert (theMesh);

  MMScan *mm = dynamic_cast<MMScan *>(theMesh);
  if (!mm) {
    printf("Selected mesh is not a ModelMaker scan.\n");
    return TCL_OK;
  }

  mm->flipNormals(atoi(argv[2]));
  dm->invalidateCachedData();

  return TCL_OK;
}


/* THIS is the old version of vriporient that expected the user to align
 * the meshes by hand... not used anymore, but code may be useful for other
 * things still.

int
PlvVripOrientCmd(ClientData clientData, Tcl_Interp *interp,
	  int argc, char *argv[])
{
  if (argc != 2) return TCL_ERROR;

  if (strlen(argv[1]) == 0) {
    printf("No mesh selected.\n");
    return TCL_OK;
  }

  DisplayableMesh *dm = FindMeshDisplayInfo(argv[1]);
  assert (dm);

  RigidScan *theMesh = dm->getMeshData();
  assert (theMesh);

  if (!theMesh->has_ending(".cta")) return TCL_ERROR;

  FILE *vfile = fopen("vrip.conf", "a");
  if (!vfile) return TCL_ERROR;

  float q[4], t[3];
  tbView->getXform(q,t);
  Xform<float> viewer;
  viewer.fromQuaternion(q, t[0], t[1], t[2]);
  viewer.fast_invert();
  Xform<float> combined = theMesh->getXform();
  combined.removeTranslation();
  cout << "mesh xform is: " << endl << theMesh->getXform();

  //  combined = viewer * theMesh->getXform();
  // theMesh->setXform(combined);
  MMScan *mm = (MMScan *)theMesh;
  mm->write_xform_ply();

  char fname[PATH_MAX];
  strcpy(fname, theMesh->get_name().c_str());
  char *pExt = strrchr(fname, '.');
  if (pExt == NULL)
    pExt = fname + strlen(fname);
  strcpy(pExt, ".ply");
  combined.fast_invert();
  combined.toQuaternion(q);
  combined.getTranslation(t);
  fprintf(vfile, "bmesh %s %.5f %.5f %.5f %.5f %.5f %.5f %.5f\n",
	  fname, t[0], t[1], t[2], q[1], q[2], q[3], q[0]);
  fclose(vfile);
  strcpy(pExt, ".xf");
  mm->setXform(combined);
  mm->writeXform(fname);
  cout << "inverted xform is: " << endl << theMesh->getXform();
  return TCL_OK;
}

*/

