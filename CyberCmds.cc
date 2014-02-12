//############################################################
//
// CyberCmds.cc
//
// Matt Ginzton, Kari Pulli
// Mon Jan 25 16:47:50 CET 1999
//
// Implement interface to commands that operate only on
// data from the CyberWare custom range scanner for the
// Digital Michelangelo project.
//
//############################################################


#include <vector.h>
#include <fstream.h>

#include "Xform.h"
#include "Pnt3.h"
#include "CyberScan.h"
#include "CyberCmds.h"
#include "Progress.h"
#include "DisplayMesh.h"
#include "defines.h"
#include "plvGlobals.h"
#include "plvScene.h"
#include "ICP.h"
#include "GlobalReg.h"
#include "BailDetector.h"
#include "WorkingVolume.h"
#include "FileNameUtils.h"


int
PlvWriteSDForVripCmd(ClientData clientData,
		     Tcl_Interp *interp,
		     int argc, char* argv[])
{
   if (argc < 5) {
      interp->result =
	"Usage: PlvWriteSDForVripCmd <res level, 0=max> <dir> "
	"<sweeps|subsweeps> <delete-sweeps>";
      return TCL_ERROR;
   }

   int iRes = atoi (argv[1]);
   char* dir = argv[2];
   if (!check_file_access (dir, true, true, true, false, true)) {
     interp->result = "Cannot write to output directory";
     return TCL_ERROR;
   }

   bool useSubSweeps;
   if (!strcmp(argv[3], "sweeps")) {
      useSubSweeps = false;
   } else if (!strcmp(argv[3], "subsweeps")) {
      useSubSweeps = true;
   } else {
      cerr << "Invalid sweep/subsweep option!" << endl;
      return TCL_ERROR;
   }

   bool bDeleteSweeps = atoi (argv[4]);
   bool bConfOnly = (argc > 5) && atoi (argv[5]) && !useSubSweeps;

  // Code for version that used to write the whole scans.
  //Xform<float> xfRotate;
  //xfRotate.rotZ (M_PI);      // right-side-up just for viewing pleasure
  //xfRotate.rotX (-M_PI/2.0); // and for vrip, -90 degrees around X axis
  //Xform<float> xfIR = xfRotate; xfIR.fast_invert();

  char name[PATH_MAX];
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

  char confFN[PATH_MAX];
  sprintf (confFN, "%s/vrip.conf", dir);
  ofstream conffile (confFN, ios::app);

  int framesPerSweep = 256;
  // int overlap = MIN(64, pow(2,iRes+3)+0.5);
  int overlap = int(pow(2,iRes+2)+0.5);

// STL Update
  vector<DisplayableMesh*>::iterator dm = theScene->meshSets.begin();
  for (; dm < theScene->meshSets.end(); dm++) {
    if (!(*dm)->getVisible())
      continue;

    // JED - Check if mesh loaded, if not load

    RigidScan* sd = (*dm)->getMeshData();
    CyberScan* cs = dynamic_cast<CyberScan*> (sd);
    if (!cs)
      continue;

    vector<ResolutionCtrl::res_info> ri;
    int thisIRes;

    int sweepNum = 0;

    vector<CyberSweep*> sweeps = cs->get_sweep_list();
// STL Update
    for (vector<CyberSweep*>::iterator pSweep = sweeps.begin();
	 pSweep < sweeps.end(); pSweep++, sweepNum++) {

      CyberSweep* sweep = *pSweep;

      int numSubSweeps = useSubSweeps ?
	(sweep->num_frames()-1)/framesPerSweep + 1
	: 1;

      for (int k = 0; k < numSubSweeps; k++) {

	// check if file is already there, for incremental ease
	// so build the output filename now
	const char* sweepnum = sweep->get_basename().c_str();
	char* slash = strrchr (sweepnum, '/');
	if (slash)
	  sweepnum = slash + 1;
	sprintf(name, "%s/%s-sweep-%s-%s-sub-%d.ply", dir,
		sd->get_basename().c_str(), sweepnum,
		sweep->get_description().c_str(), k);
	for (slash = name + strlen(dir) + 1; *slash; slash++)
	  if (*slash == '/') *slash = '_';

	if (access (name, R_OK)) {
	  cout << "Writing " << name << " ... " << flush;
	} else if (bConfOnly) {
	  cout << "Reusing existing " << name << flush;
	} else {
	  cout << "Skipping " << name << " (already exists) ... " << endl;
	  prog->updateInc (1000 / sweeps.size());
	  continue;
	}

	int res;
	CyberSweep *newSweep = sweep;
	CyberScan *newScan;

	if (!bConfOnly) { // create mesh
	  if (useSubSweeps) {
	    int frameStart = k*framesPerSweep;
	    int frameFinish = MIN(sweep->num_frames()-1,
				  (k+1)*framesPerSweep+overlap+1);
	    newScan =
	      (CyberScan *)cs->get_piece(sweepNum, frameStart, frameFinish);

	    if (newScan == NULL)
	      continue;

	    newScan->existing_resolutions (ri);
	    thisIRes = MIN (iRes, ri.size() - 1);
	    newScan->load_resolution (thisIRes);

	    vector<CyberSweep*> newSweeps = newScan->get_sweep_list();
	    newSweep = newSweeps[0];

	    ri.clear();
	    newSweep->existing_resolutions(ri);
	    res = ri[thisIRes].abs_resolution;
	  } else {
	    // force resolution to exist, since we're writing one or
	    // more of its sweeps; harmless enough to call more than once
	    sd->existing_resolutions (ri);
	    thisIRes = MIN (iRes, ri.size() - 1);
	    cs->load_resolution (thisIRes);

	    newSweep = sweep;
	    newSweep->existing_resolutions(ri);
	    res = ri[thisIRes].abs_resolution;
	  }
	} else { // bConfOnly, ensure mesh exists
	  res = check_file_access (name, true, true);
	}

	if (res) {

	  Xform<float> xfRotate = newSweep->vrip_reorientation_frame();
	  xfRotate.invert();

	  if (!bConfOnly) {
	    // write plyfile
	    if (!newSweep->write_resolution_mesh (res, name, xfRotate)) {
	      cerr << "Scan " << sd->get_name()
		   << " failed to write self!"  << endl;
	      return TCL_ERROR;
	    }

	    /* Started to write something that would save bboxes for
	       pvrip, but realized that the world bboxes would not be
	       tight.  We can add tight world bboxes later.  For now,
	       pvrip computes its own just fine. */

	    cout << "done." << endl;
	  }

	  // write config string:
	  // bmesh filename x y z i j k r
	  char bmesh[PATH_MAX + 200];
	  float t[3];
	  float q[4];
	  Xform<float> xf = sd->getXform() * newSweep->getXform();
	  xfRotate.invert();
	  xf = xf * xfRotate;
	  xf.toQuaternion (q);
	  xf.getTranslation (t);
	  sprintf (bmesh, "bmesh %s %g %g %g %g %g %g %g",
		   1+strrchr(name,'/'),
		   t[0], t[1], t[2], -q[1], -q[2], -q[3], q[0]);

	  // write xf
	  strcpy (name + strlen(name) - 3, "xf");
	  cout << "Writing " << name << " ... " << flush;
	  ofstream xffile (name);
	  xffile << xf;
	  if (xffile.fail()) {
	    cerr << "Scan " << sd->get_name()
		 << " failed to write xform!" << endl;
	    success = false;
	    break;
	  }
	  cout << "done." << endl;

	  conffile << bmesh << endl;
	}

	if (useSubSweeps)
	  delete newScan;

	if (!g_bNoUI && !prog->updateInc (1000 / sweeps.size())) {
	  success = false;
	  cerr << endl << "CyberScan -> vrip output: cancelled!"
	       << endl << endl;
	  break;
	}
      }

      if (BailDetector::bail())
	break;
    }

    // free memory?
    if (!useSubSweeps && bDeleteSweeps) {
      sd->existing_resolutions (ri);
      int thisIRes = MIN (ri.size() - 1, iRes);
      cout << "now say bye-bye to res " << thisIRes << " of "
	   << cs->get_name() << endl;
      sd->release_resolution (ri[thisIRes].abs_resolution);
    }

    // JED - Unload mesh from memory if it started unloaded

    if (BailDetector::bail())
      break;
  }

  // cleanup
  delete prog;
  Tcl_SetVar (interp, "meshWriteStrips", oldStripVal,
	      TCL_GLOBAL_ONLY);

  if (!success)
    return TCL_ERROR;

  return TCL_OK;
}



int
PlvCyberScanSelfAlignCmd(ClientData clientData,
			 Tcl_Interp *interp,
			 int argc, char* argv[])
{
  // check input
  if (argc < 2) {
    interp->result =
      "No scan passed to PlvCyberScanSelfAlignCmd";
    return TCL_ERROR;
  }

  // get the actual CyberScan
  DisplayableMesh* dm = FindMeshDisplayInfo (argv[1]);
  if (!dm) {
    interp->result = "Missing scan in PlvCyberScanSelfAlignCmd";
    return TCL_ERROR;
  }

  CyberScan* scan = dynamic_cast<CyberScan*>(dm->getMeshData());
  if (!scan) {
    interp->result =
      "PlvCyberScanSelfAlignCmd: that's not a CyberScan!";
    return TCL_ERROR;
  }

  vector< vector<CyberSweep*> > shells =
    scan->get_ordered_sweeps();

  AutoICP<RigidScan*> self_align;

  int i,j;
  for (i = 0; i < shells.size(); i++) {
    vector<CyberSweep*>& shell = shells[i];

    for (j = 0; j < shell.size(); j++) {
      if (j > 0) {
	// register with left neighbor
	self_align.add_pair (shell[j-1], shell[j]);
      }

      if (i > 0) {
	// register with back neighbor(s)
	vector<CyberSweep*>& shellPrev = shells[i-1];
	// pick upper bound for pass to be odd... 3, 5, 7
	for (int pass = 0; pass < 5; pass++) {
	  // generate offset sequence, 0, 1, -1, 2, -2, ...
	  int ofs = (pass + 1) / 2;
	  if (pass % 2) ofs = -ofs;

	  int jo = ofs + j;
	  if (jo < 0 || jo >= shellPrev.size())
	    continue;

	  self_align.add_pair (shell[j], shellPrev[jo]);
	}
      }
    }
  }

  self_align();

  // write the xforms
  for (i = 0; i < shells.size(); i++) {
    vector<CyberSweep*>& shell = shells[i];
    for (j = 0; j < shell.size(); j++) {
      if (!shell[j]->write_metadata(RigidScan::md_xform)) {
	cerr << "Couldn't write the xform for "
	     << shell[j]->get_name() << endl;
      } else {
	cerr << "Saved xform for "
	     << shell[j]->get_name() << endl;
      }
    }
  }

  dm->invalidateCachedData(); // changed internal xforms

  return TCL_OK;
}


int
ScnDumpLaserPntsCmd(ClientData clientData,
		    Tcl_Interp *interp,
		    int argc, char* argv[])
{
  // check input
  if (argc < 4) {
    interp->result =
      "Usage: ScnDumpLaserPntsCmd scan_name filename nPts";
    return TCL_ERROR;
  }

  // get the actual CyberScan
  DisplayableMesh* dm = FindMeshDisplayInfo (argv[1]);
  if (!dm) {
    interp->result = "Missing scan in ScnDumpLaserPntsCmd";
    return TCL_ERROR;
  }

  CyberScan* scan = dynamic_cast<CyberScan*>(dm->getMeshData());
  if (!scan) {
    interp->result =
      "ScnDumpLaserPntsCmd: that's not a CyberScan!";
    return TCL_ERROR;
  }

  scan->dump_pts_laser_subsampled(argv[2], atoi(argv[3]));

  return TCL_OK;
}


int
PlvDiceCyberDataCmd(ClientData clientData,
		    Tcl_Interp *interp,
		    int argc, char* argv[])
{
  int numMeshes = theScene->meshSets.size();

  int framesPerSweep = 256;

  for (int i = 0; i < numMeshes; i++) {
    DisplayableMesh *displayMesh = theScene->meshSets[i];
    if (!displayMesh->getVisible())
      continue;

    RigidScan *sd = displayMesh->getMeshData();
    CyberScan* cs = dynamic_cast<CyberScan*> (sd);
    if (!cs)
      continue;

    vector<CyberSweep*> sweeps = cs->get_sweep_list();
    for (int j=0; j < sweeps.size(); j++) {
      CyberSweep *sweep = sweeps[j];
      int numSubSweeps = (sweep->num_frames()-1)/framesPerSweep + 1;
      for (int k = 0; k < numSubSweeps; k++) {
	int frameStart = k*framesPerSweep;
	int frameFinish = MIN(sweep->num_frames()-1,
			      (k+1)*framesPerSweep+33);

	// newmesh mode... makes a new mesh and adds it to Mesh Controls
	// filtered_copy will return NULL if not supported; will return
	// valid pointer to scan with no resolutions if the clip excludes
	// all data.

	cout << frameStart << "  "  << frameFinish << endl;

	DisplayableMesh *oldDisp = displayMesh;
	RigidScan *meshFrom = sd;
	RigidScan *meshTo = cs->get_piece(j, frameStart, frameFinish);

	if (meshTo) {
	  if (meshTo->num_resolutions() > 0) {
	    char tmpstr[PATH_MAX];
	    crope name;
	    sprintf(tmpstr, "%d", j);
	    name = meshFrom->get_basename() + "_" + tmpstr;
	    sprintf(tmpstr, "%d", k);
	    name = name + "_" + tmpstr + "." + meshFrom->get_nameending();
	    meshTo->set_name(name);

	    DisplayableMesh* newDisp = theScene->addMeshSet (meshTo, false);
	    Tcl_VarEval (interp, "clipMeshCreateHelper ",
			 oldDisp->getName(), " ", newDisp->getName(), NULL);
	  } else {
	    // scan supports clipping, but had no data
	    Tcl_VarEval (interp, "changeVis ",
			 oldDisp->getName(), " 0", NULL);
	  }
	}
      }
    }
  }
  return TCL_OK;
}


int
PlvWorkingVolumeCmd(ClientData clientData, Tcl_Interp *interp,
                    int argc, char *argv[])
{
  CyberWorkingVolume *cwv = GetCyberWorkingVolume();

  if        (argc==2 && !strcmp(argv[1], "off"))  {
    cwv->off();
  } else if (argc==2 && !strcmp(argv[1], "on"))  {
    cwv->on();
  } else if (argc==2 && !strcmp(argv[1], "solid"))  {
    cwv->drawMode(0);
  } else if (argc==2 && !strcmp(argv[1], "lines"))  {
    cwv->drawMode(1);
  } else if (argc==3 && !strcmp(argv[1], "use"))  {
    cwv->use((FindMeshDisplayInfo(argv[2]))->getMeshData());
  } else if (argc == 9)  {
    cwv->bounds(atof(argv[1]), atof(argv[2]), atof(argv[3]), atof(argv[4]),
		atof(argv[5]), atof(argv[6]), atof(argv[7]), atof(argv[8]));
  } else {
    interp->result = "Bad arguments to plv_working_volume";
    return TCL_ERROR;
  }

  return TCL_OK;
}

int
PlvWorldCoordToSweepCoord(ClientData clientData, Tcl_Interp *interp,
                    int argc, char *argv[])
{
  if (argc!=5) {
    // Usage
    Tcl_SetResult(interp,"Usage: plv_worldCoordToSweepCoord scanname x y z\n  returns four numbers that can be used to resontruct a new world coord \n  after callibration or realignment.\n",TCL_STATIC);
    return TCL_ERROR;
  }

  // Get args
  char *scanname = argv[1];
  Pnt3 worldPt(atof(argv[2]),atof(argv[3]),atof(argv[4]));
  Pnt3 scanPt;
  int sweepInd;
  Pnt3 newWpt;

  // Get scan reference
  RigidScan *rs = FindMeshDisplayInfo(scanname)->getMeshData();
  CyberScan* cs = dynamic_cast<CyberScan*> (rs);
  if (!cs) {
    Tcl_SetResult(interp,"Specified mesh is not a CyberScan",TCL_STATIC);
    return TCL_ERROR;
  }

  // Call dk's function
  bool res = cs->worldCoordToSweepCoord(worldPt,&sweepInd, scanPt);
  // cout << "World::\n" << worldPt << "\nScan:\n" << scanPt << "\n";

  // Return results
  if (res) {
    sprintf(interp->result,"%d %f %f %f",sweepInd, (float)scanPt[0],
	    (float)scanPt[1], (float)scanPt[2]);
  } else {
    Tcl_SetResult(interp,"Did not find a sweep containing the Pt.",TCL_STATIC);
  }

  cs->sweepCoordToWorldCoord(sweepInd, scanPt, newWpt);
  // cout << "World::\n" << newWpt << "\nScan:\n" << scanPt << "\n" << flush;

  return TCL_OK;

}

int
PlvSweepCoordToWorldCoord(ClientData clientData, Tcl_Interp *interp,
                    int argc, char *argv[])
{

  // Usage
  if (argc!=6) {
    Tcl_SetResult(interp,"Usage: plv_sweepCoordToWorldCoord scanname sweepindex val1 val2 val3",TCL_STATIC);
    return TCL_ERROR;
  }

  // Get args
  char *scanname = argv[1];
  int sweepInd = atoi(argv[2]);
  Pnt3 scanPt(atof(argv[3]),atof(argv[4]),atof(argv[5]));
  Pnt3 newWpt;

  // Get scan reference
  RigidScan *rs = FindMeshDisplayInfo(scanname)->getMeshData();
  CyberScan* cs = dynamic_cast<CyberScan*> (rs);
  if (!cs) {
    Tcl_SetResult(interp,"Specified mesh is not a CyberScan",TCL_STATIC);
    return TCL_ERROR;
  }

  // Call dk's function
  cs->sweepCoordToWorldCoord(sweepInd, scanPt, newWpt);
  // cout << "World::\n" << newWpt << "\nScan:\n" << scanPt << "\n" << flush;

  // Return results
  sprintf(interp->result,"%f %f %f",(float)newWpt[0],
	  (float)newWpt[1],(float)newWpt[2]);
  return TCL_OK;
}
