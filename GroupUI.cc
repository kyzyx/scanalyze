// GroupUI.cc             manage interface to groupscan
// created 1/30/99        magi@cs


#include <vector.h>
#include "DisplayMesh.h"
#include "GroupScan.h"
#include "plvScene.h"
#include "ScanFactory.h"
#include "plvGlobals.h"

DisplayableMesh*
groupScans (vector<DisplayableMesh*>& scans, char* nameToUse, bool bDirty)
{
// STL Update
  for (vector<DisplayableMesh*>::iterator pdm = scans.begin(); pdm != scans.end(); pdm++) {
    (*pdm)->invalidateCachedData();    // memory won't be used for a while
    (*pdm)->setVisible (false);
  }

  RigidScan* group = CreateScanGroup (scans, nameToUse, bDirty);
  DisplayableMesh* dm = theScene->addMeshSet (group, false);

  // The code below removes each child of the group from the
  // vector theScene->meshSets, which is the place where all the
  // meshes in the current scene are stored. This is in accordance
  // with the strategy of storing only a list of all the "root"
  // meshes, and then calling get_children_for_display etc. to
  // get the remaining meshes if necessary.
  vector <DisplayableMesh *>children;
  crope names;

  GroupScan *g = dynamic_cast<GroupScan *>(group);
  if (group) {
    if (g->get_children_for_display (children)) {
      for (int i = 0; i < children.size(); i++) {
// STL Update
	for (vector<DisplayableMesh*>::iterator scan = theScene->meshSets.begin();
	     scan != theScene->meshSets.end(); scan++) {
	  if (!strcmp((*scan)->getName(), children[i]->getName())) {
	    names += crope (" ") + (*scan)->getName();
	    theScene->meshSets.erase(scan);
	    scan--;
	  }
	}
      }
    }
  }
  return dm;
}


vector<DisplayableMesh*>
ungroupScans (DisplayableMesh* group)
{
  assert (group);
  bool wasVis = group->getVisible();

  RigidScan* scanGroup = group->getMeshData();
  vector<DisplayableMesh*> scans = BreakScanGroup (scanGroup);

  if (scans.size()) {
// STL Update
    for (vector<DisplayableMesh*>::iterator scan = scans.begin(); scan != scans.end(); scan++) {
      (*scan)->setVisible (wasVis);
      // add children back
      theScene->meshSets.push_back(*scan);
    }
    theScene->deleteMeshSet (group);
  }

  return scans;
}


static int iGroups = 1;
char *
getNextUnusedGroupName ()
{
  char buf[256];

  sprintf(buf, "group%d", iGroups++);

  return (strdup(buf));
}


bool
addToGroup (DisplayableMesh* group, DisplayableMesh* scan)
{
  // TODO
  return false;
}


bool
removeFromGroup (DisplayableMesh* group, DisplayableMesh* scan)
{
  // TODO
  return false;
}
