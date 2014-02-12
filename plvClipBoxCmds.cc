#include <iostream>
#include <fstream>
#include <string>
#ifdef WIN32
#	include "winGLdecs.h"
#endif
#include <GL/gl.h>
#include <GL/glu.h>

#include <tcl.h>
#include "TclCmdUtils.h"

#include "togl.h"
#include "plvGlobals.h"
#include "plvClipBoxCmds.h"
#include "plvMeshCmds.h"
#include "plvDrawCmds.h"
#include "plvScene.h"
#include "plvDraw.h"
#include "DisplayMesh.h"
#include "RigidScan.h"
#include "VertexFilter.h"
#include "plvAnalyze.h"
#include "Progress.h"
#include "GroupScan.h"
#include "GenericScan.h"
#include "RangeGrid.h"


// public globals
Selection theSel;

// module globals
static const int kHandleSize = 3;
static bool bManipulatingSel = false;
static bool bMoveEntire = false;
static bool bChangeExisting = false;
static int iChangeExisting = 0;
static int iDelete = -1;
Selection::Pt ptRelativeTo;


static GLint olColor = -1;
static GLint handleColor = -1;
static GLint handleSelColor = -1;


Selection::Selection (void)
{
  clear();
}


void Selection::clear (void)
{
  type = none;
  pts.clear();
}


static void drawHandle (Selection::Pt pt, bool bSelected = false);
static bool ptOnHandle (Selection::Pt pt, Selection::Pt handle);
static bool ptOnLine (Selection::Pt pt,
		      Selection::Pt start, Selection::Pt end);
static int findSelectionHandle (Selection::Pt pt, const Selection& sel);
static int findSelectionLine (Selection::Pt pt, const Selection& sel);

// static DisplayableMesh *
// ClipGroupHelper (GroupScan *groupFrom, const VertexFilter &filter);
static DisplayableMesh* Renamer (DisplayableMesh *mesh);

int
PlvClipToSelectionCmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[])
{
  if (argc < 2) {
    interp->result = "Bad args to PlvClipToSelectionCmd";
    return TCL_ERROR;
  }

  DisplayableMesh* meshDisp = FindMeshDisplayInfo (argv[1]);

  bool bClipInvisible = false;
  bool bClipMulti = false;
  bool bKeepInside = true;
  bool bClipInPlace = false;
  for (int i = 2; i < argc; i++) {
    if (!strcmp (argv[i], "inside"))
      bKeepInside = true;
    else if (!strcmp (argv[i], "outside"))
      bKeepInside = false;
    else if (!strcmp (argv[i], "inplace"))
      bClipInPlace = true;
    else if (!strcmp (argv[i], "newmesh"))
      bClipInPlace = false;
    else if (!strcmp (argv[i], "sel"))
      bClipMulti = false;
    else if (!strcmp (argv[i], "vis")) {
      bClipMulti = true; bClipInvisible = false;
    } else if (!strcmp (argv[i], "all")) {
      bClipMulti = bClipInvisible = true;
    }
  }

  // build list of scans to clip
  vector<DisplayableMesh*> clippees;
  if (bClipMulti) {
    // push all meshes with sufficient visibility
// STL Update
    for (vector<DisplayableMesh*>::iterator pdm = theScene->meshSets.begin();
	 pdm != theScene->meshSets.end();
	 pdm++) {
      if (bClipInvisible || (*pdm)->getVisible())
	clippees.push_back (*pdm);
    }
  } else {
    // just use the given source mesh
    clippees.push_back (meshDisp);
  }

  // now iterate and clip each clippee
  bool bSuccess = true;
  char* error = NULL;
  Progress* progress = new Progress (clippees.size(), "Clipping scans", true);
  cout << "Clipping " << clippees.size() << " scans..." << endl;
// STL Update
  for (vector<DisplayableMesh*>::iterator clippee = clippees.begin();
       clippee != clippees.end();
       clippee++) {

    DisplayableMesh* oldDisp = *clippee;
    if (!oldDisp) {
      cerr << "Attempt to clip nonexistent mesh!" << endl;
      bSuccess = false;
      continue;
    }
    RigidScan* meshFrom = oldDisp->getMeshData();

    VertexFilter* filter = filterFromSelection (meshFrom, theSel);
    if (!filter) {
      error = "First you must select an area";
      bSuccess = false;
      break;
    }

    if (!bKeepInside) {
      filter->invert();
    }

    if (bClipInPlace) {
      // inplace mode -- no creating new mesh, just munge the existing one
      // Do it by saving the mesh's name and then making a
      // filtered copy of it.  Then delete the old one, rename
      // the new one and add it.
      cerr << "attempting a clip of mesh: " << meshFrom->get_name() << endl;
      RigidScan* meshTo = meshFrom->filtered_copy(*filter);
      GroupScan *groupResult = dynamic_cast<GroupScan*>(meshTo);
      if (meshTo) {

	if (meshTo->num_resolutions() > 0) {

	  char *name = strdup(meshFrom->get_name().c_str());
	  // get the old theMesh
	  char *theMeshOld = GetTclGlobal("theMesh");

	  char tclCmdBuff[256];
	  sprintf(tclCmdBuff, "confirmDeleteMesh %s", oldDisp->getName());
	  if (Tcl_Eval (interp, tclCmdBuff) != TCL_OK)
	    cerr << "confirmDelete failed " << endl;


	  DisplayableMesh* newDisp;
	  if(!groupResult) {
	    meshTo->set_name(name);
	    newDisp = theScene->addMeshSet (meshTo, false);
	  } else {
	    // clipping a group in place means that we need to
	    // rename the group and all its members using
	    // the Renamer routine.

	    newDisp = FindMeshDisplayInfo(meshTo);

	    newDisp = Renamer(newDisp);
	  }

	  sprintf(tclCmdBuff, "clipInplaceHelper %s %s", newDisp->getName(),
		  theMeshOld);
	  Tcl_Eval (interp, tclCmdBuff);

	  free(name);

	} else {
	  // scan supports clipping, but had no data

	  Tcl_VarEval (interp, "changeVis ", oldDisp->getName(), " 0", NULL);
	}
      } else {
	// scan doesn't know how to clip
	cerr << "Scan " << meshTo->get_name()
	     << " failed filtered_inplace" << endl;
	bSuccess = false;
      }

    } else {
      // newmesh mode... makes a new mesh and adds it to Mesh Controls
      // filtered_copy will return NULL if not supported; will return
      // valid pointer to scan with no resolutions if the clip excludes
      // all data.
      RigidScan* meshTo = meshFrom->filtered_copy(*filter);

      if (meshTo) {
	if (meshTo->num_resolutions() > 0) {
	  // set name, same as old with clip appended before extension
	  GroupScan *group = dynamic_cast<GroupScan*>(meshTo);
	  if (!group) {
	    crope name = meshFrom->get_basename()
	      + "Clip." + meshFrom->get_nameending();
	    meshTo->set_name(name);

	    DisplayableMesh* newDisp = theScene->addMeshSet (meshTo, false);
	    Tcl_VarEval (interp, "clipMeshCreateHelper ",
			 oldDisp->getName(), " ", newDisp->getName(), NULL);
	  } else {
	    // since it is a group, it has already been named + added to the scene
	    Tcl_VarEval (interp, "clipMeshCreateHelper ",
			 oldDisp->getName(), " ",
			 FindMeshDisplayInfo(meshTo)->getName(), NULL);
	  }
	} else {
	  // scan supports clipping, but had no data
	  Tcl_VarEval (interp, "changeVis ", oldDisp->getName(), " 0", NULL);
	}
      } else {
	// scan doesn't know how to clip
	cerr << "Scan " << meshFrom->get_name()
	     << " failed filtered_copy" << endl;
	bSuccess = false;
      }
    }

    delete filter;

    // update progress; check for cancel
    if (!progress->updateInc()) {
      cerr << "Clip operation cancelled ("
	   << (clippee - clippees.begin() + 1)
	   << " down; " << (clippees.end() - clippee - 1)
	   << " to go)" << endl;
      break;
    }
  }

  delete progress;

  theScene->computeBBox();
  if (bSuccess)
    return TCL_OK;

  if (error)
    interp->result = error;
  else
    interp->result = "At least one scan could not be clipped... "
      "Perhaps it does not support filtered_copy or filter_inplace?";

  return TCL_ERROR;
}

// This helper routine looks for the last instance
// of "Clip" in haystack.
// we are guaranteed that there is at least one "Clip"
// in the string haystack since we just created
// it as a filtered_copy.

static char*
my_clip_strrstr (char** haystack)
{
  char *str = *haystack;
  char *oldstr = str;

  //cerr << "str initted to " << str << endl;
  while ((str = strstr(oldstr+1, "Clip")) != NULL) {
    oldstr = str;
    //cerr << "Oldstr is " << oldstr << endl;
  }
  return oldstr;
}

// Clipping in place for groups works by creating a filtered
// copy of the group and then renaming each member recursively.
// We must delete it from the hashtable in theScene using its
// old name and then add it back with the new name.
// Also, theScene->meshSets (a vector) also maintains a list of
// displayable meshes - but by renaming the displayable
// mesh, we are ensuring that meshSets is in sync with the
// hashtable which is crucial.
// we have to rename both the displayable mesh and the rigid scan
// The renaming takes place by finding the last occurrence of
// "Clip" in the name and getting rid of it, so as to disguise
// the clip as the original mesh.
// (Due to complications with the hashtable/meshSets data
// structures, the renaming must take place after the
// entire group has been created, not while it is being
// built from its constituents).
static DisplayableMesh *
Renamer (DisplayableMesh *mesh) {
  vector<DisplayableMesh*>children;
  vector<DisplayableMesh*>vec;
  RigidScan *scan = mesh->getMeshData();

  char oldname [256];
  sprintf(oldname, "%s",  mesh->getMeshData()->get_basename().c_str());
  char *tmp = oldname; // tmp is needed to convince compiler
  *(my_clip_strrstr(&tmp)) = '\0'; // removes "Clip" from end of string

  //cerr << "oldname is now " << oldname << endl;
  crope newname;
  newname = crope(oldname) + "." + mesh->getMeshData()->get_nameending();
  char newbuf [PATH_MAX];
  sprintf (newbuf, "%s", newname.c_str());

  GroupScan *g = dynamic_cast<GroupScan*>(scan);
  if (g) {
    if (g->get_children_for_display(children)) {
      ungroupScans(mesh);
// STL Update
      for (vector<DisplayableMesh*>::iterator it = children.begin(); it < children.end(); it++)
	vec.push_back(Renamer (*it)); // rename the descendants of the group first
      mesh = groupScans(vec, newbuf, true);
      return mesh;
    }
    return NULL; //ok as long as group has to have children
  } else {
    // rename rigid scan
    mesh->getMeshData()->set_name(newname);

    // rename displayable mesh
    char buf[PATH_MAX];
    sprintf(buf, "%s",  mesh->getName());
    char *tmp2 = buf;
    *(my_clip_strrstr(&tmp2)) = '\0'; // removes "Clip" from end of string
    MeshSetHashDelete(strdup(mesh->getName()));
    mesh->setName(buf);
    AddMeshSetToHash(mesh);

    return mesh;
  }
}

// static DisplayableMesh *
// ClipGroupHelper (GroupScan *groupFrom, const VertexFilter &filter)
// {
//   vector<DisplayableMesh*>newkids;
//   char *oldGroupName = strdup(groupFrom->get_name().c_str());
//   cerr << "Old group name " << oldGroupName << endl;
//   VertexFilter *childfilter;
//   DisplayableMesh *groupMeshTo, *clippedGroup;

//   vector<DisplayableMesh*>kids =
//     ungroupScans(FindMeshDisplayInfo(oldGroupName));
//   for (DisplayableMesh** it = kids.begin(); it < kids.end(); it++) {
//     RigidScan *from = (*it)->getMeshData();
//     assert(from);
//     GroupScan* g = dynamic_cast<GroupScan*>(from);
//     childfilter = filter.transformedClone((float*)from->getXform());
//     if (g) {
//       clippedGroup = ClipGroupHelper (g, *childfilter);
//       newkids.push_back(clippedGroup);
//     } else {
//       RigidScan *to = from->filtered_copy(*childfilter, true);
//       char *name = strdup(from->get_name().c_str());
//       DisplayableMesh *disp = FindMeshDisplayInfo((*it)->getName());
//       theScene->deleteMeshSet(disp);
//       if (to) {
// 	if (to->num_resolutions() > 0) {

// 	  //	  char *theMeshOld = "back1"; //GetTclGlobal("theMesh");
// 	  to->set_name(name);

// 	  DisplayableMesh *newkid = theScene->addMeshSet(to,false);

// 	  newkids.push_back(newkid);
// 	}
//       }
//     }
//   }
//   groupMeshTo = groupScans (newkids, oldGroupName, true);
//   return groupMeshTo;
// }

/* This function is called from interactors.tcl when the user
   selects the Print Voxel Info command from the right-click
   menu after specifying a rectangular box region
   - for display voxel feature */
int PlvPrintVoxelsCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{
  printf("Getting voxel information...\n");

  if (argc != 2) {
    interp->result = "Bad args to PlvPrintVoxelsCmd";
    return TCL_ERROR;
  }

  DisplayableMesh* meshDisp = FindMeshDisplayInfo (argv[1]);
  if (meshDisp == NULL) {
    interp->result = "Attempting to print voxels for non-existant mesh";
    return (TCL_ERROR);
  }

  RigidScan* meshFrom = meshDisp->getMeshData();
  if (meshFrom == NULL) {
    interp->result = "PlvPrintVoxelsCmd: We don't have a mesh";
    return (TCL_ERROR);
  }
  //GenericScan* gscanFrom = (GenericScan *) meshFrom;
  GenericScan *gscanFrom = dynamic_cast<GenericScan*> (meshFrom);
  if (!gscanFrom) {
    interp->result = "Mesh is not a ply file";
    return (TCL_ERROR);
  }

  VertexFilter* filter = filterFromSelection (gscanFrom, theSel);
  // VertexFilter* filter = filterFromSelection (meshFrom, theSel);
  if (!filter) {
    interp->result = "First you must select an area";
    return (TCL_ERROR);
  }
  RigidScan* meshTo = gscanFrom->filtered_copy(*filter);
  //RigidScan* meshTo = meshFrom->filtered_copy(*filter);
  GenericScan* gscanTo = (GenericScan *) meshTo;
  if (meshTo->num_vertices() == 0) {
    interp->result = "No vertices selected";
    return (TCL_ERROR);
  } else gscanTo->PrintVoxelInfo();

  return (TCL_OK);
}

int
PlvClearSelectionCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{
  struct Togl* togl = toglHash.FindTogl (argv[1]);
  assert (togl);

  theSel.clear();
  Togl_PostOverlayRedisplay (togl);

  return TCL_OK;
}



int
PlvDrawLineSelectionCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[])
{
  // args are %x %y start
  if (argc < 4)
    return TCL_ERROR;

  Selection::Pt pt;
  pt.x = atoi(argv[1]);
  pt.y = theHeight - atoi(argv[2]);

  if (!strcmp (argv[3], "start")) {
    bChangeExisting = false;
    bManipulatingSel = true;
    bMoveEntire = false;

    if (theSel.type == Selection::line) {
      // attempt to find existing line
      int iHandle = findSelectionHandle (pt, theSel);
      if (iHandle >= 0) {
	// reuse pt that's not iHandle
	bChangeExisting = true;
	if (iHandle == 0)
	  theSel[0] = theSel[1];
	theSel[1] = pt;
      } else if (ptOnLine (pt, theSel[0], theSel[1])) {
	bChangeExisting = true;
	bMoveEntire = true;
	ptRelativeTo = pt;
      }
    }

    if (!bChangeExisting)
    {
      // start new line
      theSel.clear();
      theSel.type = Selection::line;
      theSel.pts.push_back (pt);
      theSel.pts.push_back (pt);
      ptRelativeTo.x = ptRelativeTo.y = 0;
    }
  } else if (!strcmp (argv[3], "stop")) {
    bManipulatingSel = false;
    if (theSel[0].x == theSel[1].x && theSel[0].y == theSel[1].y)
      Tcl_Eval (Togl_Interp (toglCurrent), "clearSelection");
  } else if (!strcmp (argv[3], "move")) {
    if (theSel.type != Selection::line) {
      interp->result = "Internal selection tracking error";
      return TCL_ERROR;
    }

    if (bMoveEntire) {
      theSel[0].x += pt.x - ptRelativeTo.x;
      theSel[0].y += pt.y - ptRelativeTo.y;
      theSel[1].x += pt.x - ptRelativeTo.x;
      theSel[1].y += pt.y - ptRelativeTo.y;
      ptRelativeTo = pt;
    } else {
      if (argc > 4 && !strcmp (argv[4], "constrain")) {
	int dx = abs (pt.x - theSel[0].x);
	int dy = abs (pt.y - theSel[0].y);

	if (dx > 2 * dy)   // strongly x-oriented -- x only
	  pt.y = theSel[0].y;
	else if (dy > 2 * dx) // strongly y-oriented -- y only
	  pt.x = theSel[0].x;
	else if (dx > dy)  // x dominates; 45 degree controlled by x
	  pt.y = theSel[0].y + dx * (pt.y > theSel[0].y ? 1 : -1);
	else  // 45 degree controlled by y
	  pt.x = theSel[0].x + dy * (pt.x > theSel[0].x ? 1 : -1);
      }

    theSel[1] = pt;
    }
  }

#ifndef no_overlay_support
  Togl_PostOverlayRedisplay (toglCurrent);
#else
  drawOverlayAndSwap(toglCurrent);
#endif

  return TCL_OK;
}


int
PlvDrawShapeSelectionCmd(ClientData clientData, Tcl_Interp *interp,
			 int argc, char *argv[])
{
  // args are %x %y start
  if (argc < 4)
    return TCL_ERROR;

  Selection::Pt pt;
  pt.x = atoi(argv[1]);
  pt.y = theHeight - atoi(argv[2]);

  if (!strcmp (argv[3], "start")) {
    bChangeExisting = false;
    bManipulatingSel = true;
    bMoveEntire = false;

    if (theSel.type == Selection::shape) {
      // attempt to find existing polyline
      int iHandle = findSelectionHandle (pt, theSel);
      if (iHandle >= 0) {
	// reuse pt i
	bChangeExisting = true;
	iChangeExisting = iHandle;
      } else {
	// not on a handle; look for a line
	if (findSelectionLine (pt, theSel) >= 0) {
	  bChangeExisting = true;
	  iChangeExisting = -1;
	  bMoveEntire = true;
	  ptRelativeTo = pt;
	}
      }
    }

    if (bChangeExisting) {
      if (iChangeExisting >= 0)
	theSel[iChangeExisting] = pt;
    } else {
      if (theSel.type != Selection::shape) {
	// start new polyline
	theSel.clear();
	theSel.type = Selection::shape;
	theSel.pts.push_back (pt);
      }

      bChangeExisting = true;
      iChangeExisting = theSel.pts.size();
      theSel.pts.push_back (pt);
      ptRelativeTo.x = ptRelativeTo.y = 0;
    }
  } else if (!strcmp (argv[3], "stop")) {
    bManipulatingSel = false;
  } else if (!strcmp (argv[3], "move")) {
    if (theSel.type != Selection::shape) {
      interp->result = "Internal selection tracking error";
      return TCL_ERROR;
    }
    iDelete = -1;

    if (bMoveEntire) {
      for (int i = 0; i < theSel.pts.size(); i++) {
	theSel[i].x += pt.x - ptRelativeTo.x;
	theSel[i].y += pt.y - ptRelativeTo.y;
      }
      ptRelativeTo = pt;
    } else if (bChangeExisting) {
      theSel.pts[iChangeExisting] = pt;
    }
  } else if (!strcmp (argv[3], "modify")) {
    // attempt to find existing polyline handle
    if (theSel.type != Selection::shape) {
      interp->result = "Internal selection tracking error";
      return TCL_ERROR;
    }
    int nPts = theSel.pts.size();
    bMoveEntire = false;
    bChangeExisting = false;

    // look for a point -- if found, start dragging it
    int iHandle = findSelectionHandle (pt, theSel);
    if (iHandle >= 0) {
      bChangeExisting = true;
      iChangeExisting = iDelete = iHandle;
    } else {
    // not on a point; look for a line to add a point
      int iLine = findSelectionLine (pt, theSel);
      if (iLine >= 0) {
	// insert new point between iLine and iLine+1
	iChangeExisting = iLine + 1;
	bChangeExisting = true;
// STL Update
	theSel.pts.insert (theSel.iterator_at_index(iChangeExisting), pt);
      }
    }
  } else if (!strcmp (argv[3], "remove")) {
    // if this was a click and not a drag, remove selected pt
    if (iDelete >= 0 && ptOnHandle (pt, theSel[iDelete])) {
// STL Update
      theSel.pts.erase (theSel.iterator_at_index(iDelete));
    }
  }

#ifndef no_overlay_support
  Togl_PostOverlayRedisplay (toglCurrent);
#else
  drawOverlayAndSwap(toglCurrent);
#endif

  return TCL_OK;
}


int
PlvDrawBoxSelectionCmd(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[])
{
  if (argc < 5)
    return TCL_ERROR;

  Selection::Pt pt;
  pt.x = atoi(argv[2]);
  pt.y = theHeight - atoi(argv[3]);

  struct Togl* togl = toglHash.FindTogl (argv[1]);
  assert (togl);

  // rect is stored as 4 points, corners, so that area selection
  // iterators actually work with it

  if (argc > 4 && !strcmp (argv[4], "start")) {
    bChangeExisting = false;
    bManipulatingSel = true;
    bMoveEntire = false;

    // attempt to move existing points
    if (theSel.type == Selection::rect) {
      int iHandle = findSelectionHandle (pt, theSel);
      if (iHandle >= 0) {
	// reuse pt iHandle
	bChangeExisting = true;
	theSel[0] = theSel[(iHandle+2)%4];
	theSel[2] = pt;
	theSel[1].x = theSel[2].x; theSel[1].y = theSel[0].y;
	theSel[3].x = theSel[0].x; theSel[3].y = theSel[2].y;
      } else {
	if (findSelectionLine (pt, theSel) >= 0) {
	  bChangeExisting = true;
	  bMoveEntire = true;
	  ptRelativeTo = pt;
	}
      }
    }

    if (!bChangeExisting) {
      theSel.clear();
      theSel.type = Selection::rect;
      theSel.pts.push_back (pt);
      theSel.pts.push_back (pt);
      theSel.pts.push_back (pt);
      theSel.pts.push_back (pt);
    }

    return TCL_OK;
  } else if (!strcmp (argv[4], "stop")) {
    bManipulatingSel = false;
    if (theSel[0].x == theSel[2].x && theSel[0].y == theSel[2].y)
      Tcl_Eval (Togl_Interp (togl), "clearSelection");
  } else {
    if (theSel.type != Selection::rect) {
      interp->result = "Internal selection tracking error";
      return TCL_ERROR;
    }

    if (bMoveEntire) {
      for (int i = 0; i < 4; i++) {
	theSel[i].x += pt.x - ptRelativeTo.x;
	theSel[i].y += pt.y - ptRelativeTo.y;
      }
      ptRelativeTo = pt;
    } else {
      // pt 0 doesn't change; pt 2 becomes new pont
      // pt 1 needs to change x, pt 3 needs to change y
      theSel[1].x = pt.x;
      theSel[3].y = pt.y;
      theSel[2] = pt;
    }
  }

#ifndef no_overlay_support
  Togl_PostOverlayRedisplay (togl);
#else
  drawOverlayAndSwap(togl);
#endif

  return TCL_OK;
}


int
PlvGetSelectionCursorCmd(ClientData clientData, Tcl_Interp *interp,
			 int argc, char *argv[])
{
  if (argc < 3)
    return TCL_ERROR;

  Selection::Pt pt;
  pt.x = atoi(argv[1]);
  pt.y = theHeight - atoi(argv[2]);

  // cursors: default = tcross, handle = dotbox, border = fleur
  // check handles
  if (findSelectionHandle (pt, theSel) >= 0)
    interp->result = "dotbox";
  else if (findSelectionLine (pt, theSel) >= 0)
    interp->result = "fleur";
  else
    interp->result = "tcross";

  return TCL_OK;
}

// ------------------------------------------------------------
// PlvGetSelectionInfoCmd
// Gets a string for the info of the current selection object
// ------------------------------------------------------------
int
PlvGetSelectionInfoCmd(ClientData clientData, Tcl_Interp *interp,
			  int argc, char *argv[])
{
  char buff[256];

  switch ( theSel.type ) {
  case Selection::none:
    interp->result = "PlvGetSelectionInfoCmd: error -- nothing selected";
    return TCL_ERROR;
    //    break;
  case Selection::line:
    sprintf(buff, "line %i %i %i %i", theSel[0].x, theSel[0].y,
	    theSel[1].x, theSel[1].y);
    break;
  case Selection::rect:
    /*
    sprintf(buff, "rect %i %i %i %i %i %i %i %i", theSel[0].x, theSel[0].y,
	    theSel[1].x, theSel[1].y, theSel[2].x, theSel[2].y,
	    theSel[3].x, theSel[3].y);
    */
    break;
  case Selection::shape:
    // CHEAP HACK WARNING!
    // Something weird happens when I try to use 0 1 2
    printf("PlvGetSelectionInfoCmd: getting only first 3 points\n");
    sprintf(buff, "shape %i %i %i %i %i %i", theSel[1].x, theSel[1].y,
	    theSel[2].x, theSel[2].y, theSel[3].x, theSel[3].y);
    break;
  default:
    interp->result = "PlvGetSelectionInfoCmd: error -- type not supported";
    return TCL_ERROR;
    //    break;
  }

  Tcl_SetResult(interp, buff, TCL_VOLATILE);
  return TCL_OK;
}


int
PlvMeshIntersectSelectionCmd(ClientData clientData, Tcl_Interp *interp,
			     int argc, char *argv[])
{
  if (argc < 2) {
    interp->result = "Missing mesh argument in PlvShowBySelectionCmd";
    return TCL_ERROR;
  }

  if (theSel.type != Selection::rect) {
    interp->result = "Only rectangle selections supported";
    return TCL_ERROR;
  }

  DisplayableMesh* dm = FindMeshDisplayInfo (argv[1]);
  if (!dm) {
    interp->result = "Bad mesh argument in PlvShowBySelectionCmd";
    return TCL_ERROR;
  }
  RigidScan* rs = dm->getMeshData();
  Bbox scanBox = rs->worldBbox();

  Bbox screenBounds;
  for (int i = 0; i < 8; i++) {
    Pnt3 pt = scanBox.corner (i);
    screenBounds.add (pt);
  }

  return TCL_OK;
}


void
initSelection (struct Togl* togl)
{
  if (togl) {
    olColor = Togl_AllocColorOverlay (togl, 1, 0, 0);
    handleColor = Togl_AllocColorOverlay (togl, 1, 1, 0);
    handleSelColor = Togl_AllocColorOverlay (togl, 0, 1, 0);
  }
}


void
drawSelection (struct Togl* togl)
{
  int width, height;

  width = Togl_Width (togl);
  height = Togl_Height (togl);

  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix(); // kberg - needed when overlay planes not available
  glLoadIdentity();
  gluOrtho2D(-0.5, width+0.5, -0.5, height+0.5);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix(); // kberg - needed for when overlay planes not available
  glLoadIdentity();

  int nPts = theSel.pts.size();
  if (nPts) {
    glPushAttrib (GL_LIGHTING_BIT);
    glDisable (GL_LIGHTING);

    // draw handles, if it's not currently being dragged
    if (!bManipulatingSel) {
      for (int i = 0; i < nPts; i++)
	drawHandle (theSel[i],
		    theSel.type == Selection::shape
		    && ((i==0) || (i==(nPts-1))));
    }

    // now draw the selection itself as a line loop
#ifdef no_overlay_support
    glColor3f(1,0,0);
#else
     glIndexi (olColor);
#endif
    glBegin (GL_LINE_LOOP);
    for (int i = 0; i <= nPts; i++)
      glVertex2i (theSel[i%nPts].x, theSel[i%nPts].y);
    glEnd();

    glPopAttrib();
  }

  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glFinish();
}


static void drawHandle (Selection::Pt pt, bool bSelected)
{
  glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);

#ifndef no_overlay_support
  glIndexi (bSelected ? handleSelColor : handleColor);

  glBegin (GL_QUADS);

#else
  // no clue on this one, but Linux/nVidia won't draw the quads but
  // will draw the line loop
  glBegin(GL_LINE_LOOP);
  bSelected ?  glColor3f(0,1,0) : glColor3f(1,1,0);
#endif
  glVertex2i (pt.x - kHandleSize, pt.y - kHandleSize);
  glVertex2i (pt.x - kHandleSize, pt.y + kHandleSize);
  glVertex2i (pt.x + kHandleSize, pt.y + kHandleSize);
  glVertex2i (pt.x + kHandleSize, pt.y - kHandleSize);
  glEnd();
}


static bool ptOnHandle (Selection::Pt pt, Selection::Pt handle)
{
  return (   abs (pt.x - handle.x) < kHandleSize
	  && abs (pt.y - handle.y) < kHandleSize);
}


static int findSelectionHandle (Selection::Pt pt, const Selection& sel)
{
  int nPts = sel.pts.size();
  for (int i = 0; i < nPts; i++)
    if (ptOnHandle (pt, sel[i]))
      return i;

  return -1;
}


static bool ptOnLine (Selection::Pt pt,
		      Selection::Pt start, Selection::Pt end)
{
  // handle vertical line
  if (start.x == end.x) {
    if (abs (pt.x - start.x) >= kHandleSize)
      return false;

    if (pt.y > start.y && pt.y > end.y)
      return false;
    if (pt.y < start.y && pt.y < end.y)
      return false;

    return true;
  }

  // ok, not vertical -- check horizontal
  if (pt.x < start.x && pt.x < end.x)
    return false;
  if (pt.x > start.x && pt.x > end.x)
    return false;

  // ok horizontally -- get slope
  float slope = (end.y - start.y) / (float)(end.x - start.x);
  float icpt = start.y - slope * start.x;
  int yofs = pt.y - (icpt + (slope * pt.x));

  return (abs (yofs) < kHandleSize);
}


static int findSelectionLine (Selection::Pt pt, const Selection& sel)
{
  int nPts = sel.pts.size();
  for (int i = 0; i < nPts; i++)
    if (ptOnLine (pt, sel[i], sel[(i+1)%nPts]))
      return i;

  return -1;
}


void splTessError (GLenum er)
{
  const GLubyte *str = gluErrorString (er);
  cout << "Tesselation error during polyline rasterization: "
       << str << endl;
}


vector<GLdouble*> hack_vert_alloc;

void splTessCombine (GLdouble coords[3],
		  GLdouble* vertex_data[4],
		  GLfloat weight[4],
		  GLdouble** dataOut)
{
  GLdouble* vertex = new GLdouble[3];

  vertex[0] = coords[0];
  vertex[1] = coords[1];
  vertex[2] = coords[2];

  hack_vert_alloc.push_back (vertex);  // mark to free later
  *dataOut = vertex;
}


unsigned char* filledPolyPixels (int& width, int& height,
				 const vector<ScreenPnt>& pts)
{
  // for word alignment: rounded up to nearest mult. of 4
  width = 4 * ((width + 3) / 4);

  // set up GL modes for rendering in screen coords
  int vp[4];
  glGetIntegerv(GL_VIEWPORT, vp);
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(-0.5, width+0.5, -0.5, height+0.5);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  // render filled polygon with tesselator
  glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
  glClear (GL_COLOR_BUFFER_BIT);
  glDisable (GL_DEPTH_TEST);
  glDisable (GL_LIGHTING);
  glDisable (GL_CULL_FACE);
  glColor3f (1, 1, 1);

#ifdef GLU_VERSION_1_2
  GLUtesselator* tess = gluNewTess();
  hack_vert_alloc.clear();

#ifdef WIN32
#  define DECORATE (GLvoid(__stdcall *) ()) &
#else
#  define DECORATE (GLvoid(*) ()) &
#endif
  gluTessCallback (tess, GLU_TESS_VERTEX, DECORATE glVertex3dv);
  gluTessCallback (tess, GLU_TESS_BEGIN,  DECORATE glBegin);
  gluTessCallback (tess, GLU_TESS_END,    DECORATE glEnd);
  gluTessCallback (tess, GLU_TESS_COMBINE,DECORATE splTessCombine);
  gluTessCallback (tess, GLU_TESS_ERROR,  DECORATE splTessError);
#undef DECORATE

  gluTessBeginPolygon (tess, NULL);
  gluTessBeginContour (tess);

  int nPts = pts.size();
  GLdouble* coords = new GLdouble [3 * (nPts + 1)];
  for (int i = 0; i <= nPts; i++) {
    coords[3*i  ] = pts[i%nPts].x;
    coords[3*i+1] = pts[i%nPts].y;
    coords[3*i+2] = 0;
    gluTessVertex (tess, coords + 3*i, coords + 3*i);
  }

  gluTessEndContour (tess);
  gluTessEndPolygon (tess);
  gluDeleteTess (tess);
  delete coords;

  // free memory used by combine-created vertices
  for (i = 0; i < hack_vert_alloc.size(); i++)
    delete hack_vert_alloc[i];
  hack_vert_alloc.clear();
#endif // GLU_VERSION_1_2

  // read the fruits of our labors
  unsigned char* pixels = new unsigned char [width*height];
  memset (pixels, 0, width*height);
  glReadBuffer (GL_BACK);
  glReadPixels (0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE,
		pixels);
  GLenum a = glGetError();
  if (a != GL_NO_ERROR) {
    cout << "raw error: " << a << endl;
    cout << "GL error: " << gluErrorString (a) << endl;
  }

  // and reset GL params
  glMatrixMode (GL_PROJECTION);
  glPopMatrix();
  glMatrixMode (GL_MODELVIEW);
  glPopMatrix();
  glViewport (vp[0], vp[1], vp[2], vp[3]);

#if 0
  int n = 0;
  for (i = 0; i < width*height; i++) {
    if (pixels[i] != 0)
      n++;
  }
  cout << n << " pixels are set" << endl;
#endif

  return pixels;
}



int
PlvGetSelectedMeshesCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[])
{
  bool bIncludeHidden = false;
  bool bInvert = false;
  bool bSilent = false;

  for (int i = 1; i < argc; i++) {
    if (!strcmp (argv[i], "hidden"))
      bIncludeHidden = true;
    else if (!strcmp (argv[i], "invert"))
      bInvert = true;
    else if (!strcmp (argv[i], "silent"))
      bSilent = true;
    else {
      cerr << argv[0] << " doesn't understand " << argv[i] << endl;
      interp->result = "Bad argument: look at stderr";
      return TCL_ERROR;
    }
  }

  if (theSel.type != Selection::rect) {
    if (bSilent && theSel.type == Selection::none)
      return TCL_OK;

    interp->result = "First you must select an area";
    return TCL_ERROR;
  }

  for (i = 0; i < theScene->meshSets.size(); i++) {
    DisplayableMesh *displayMesh = theScene->meshSets[i];
    if (!bIncludeHidden && !displayMesh->getVisible())
      continue;

    RigidScan *mesh = displayMesh->getMeshData();
    VertexFilter* filter = filterFromSelection (mesh, theSel);
    if (!filter) {
      interp->result = "First you must select an area";
      return TCL_ERROR;
    }

    if (bInvert)
      filter->invert();

    if (filter->accept(mesh->localBbox())) {
      Tcl_AppendElement (interp, (char*)displayMesh->getName());
    }
    delete filter;
  }

  return TCL_OK;
}


void resizeSelectionToWindow (struct Togl* togl)
{
  int nPts = theSel.pts.size();
  if (!nPts) return;

  float newhyp = sqrt (Togl_Width(togl)*Togl_Width(togl)
		       +Togl_Height(togl)*Togl_Height(togl));
  float oldhyp = sqrt (theWidth*theWidth + theHeight*theHeight);
  float scale = newhyp / oldhyp;

  int cx = 0, cy = 0;
  for (int i = 0; i < nPts; i++) {
    cx += theSel[i].x;
    cy += theSel[i].y;
  }
  cx /= i;
  cy /= i;

  int ncx = cx * Togl_Width (togl) / theWidth;
  int ncy = cy * Togl_Height (togl) / theHeight;

  for (i = 0; i < nPts; i++) {
    theSel[i].x = ncx + scale * (theSel[i].x - cx);
    theSel[i].y = ncy + scale * (theSel[i].y - cy);
  }

  Togl_PostOverlayRedisplay (togl);
}




