#include <iostream.h>
#include <ctype.h>
#include "plvGlobals.h"
#include "RigidScan.h"
#include "togl.h"
#include "DisplayMesh.h"
#include "plvMeshCmds.h"
#include "VertexFilter.h"
#include "plvDraw.h"
#include "ToglCache.h"
#include "ScanFactory.h"
#include "GlobalReg.h"
#include "Progress.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "plvScene.h"


//
// Initialize the scene.
//
Scene::Scene (Tcl_Interp* _interp)
{
  bbox.clear();
  globalReg = NULL;
  slowPolyCount = 0;
  resOverride = resDefault;
  interp = _interp;
  globalReg = new GlobalReg;
}


Scene::~Scene()
{
  delete globalReg;
  freeMeshes();
}



//
// Add a mesh set to the scene
//

static int TclDictionaryCompare (const char* left, const char* right);

class dmNameSort: public binary_function<DisplayableMesh*,
		                          DisplayableMesh*,
		                          bool>
{
public:
  bool operator() (DisplayableMesh* d1, DisplayableMesh* d2);
};


DisplayableMesh*
Scene::addMeshSet(RigidScan* scan, bool bRecenterCameraOK, char* nameToUse)
{
  // Prepare bbox if necessary
  if (meshSets.size() == 0) {
    bbox.clear();
  }

  // Add the mesh set
  DisplayableMesh* dm = new DisplayableRealMesh (scan, nameToUse);
  addMeshInternal (dm);

  // Update the bbox
  computeBBox();

  // reset the view trackball according to where this mesh is.
  // don't do it if meshes are already loaded and autoCenter is off.
  if (bRecenterCameraOK) {
    char* pszAutoRecenter = Tcl_GetVar (interp, "autoCenterCamera",
					TCL_GLOBAL_ONLY);
    if (meshSets.size() == 1 || !strcmp (pszAutoRecenter, "1"))
      centerCamera();
  }

  return dm;
}


void
Scene::addMeshInternal (DisplayableMesh* dm)
{
// STL Update
  vector<DisplayableMesh*>::iterator pos = lower_bound (meshSets.begin(), meshSets.end(),
				       dm, dmNameSort() );
  meshSets.insert (pos, dm);
  AddMeshSetToHash (dm);
}


//static int MeshSetHashDelete(char *name);



vector<DisplayableMesh*>::iterator*
Scene::findSceneMesh (DisplayableMesh* mesh)
{
// STL Update
  for (vector<DisplayableMesh*>::iterator dm = meshSets.begin();
       dm < meshSets.end();
       dm++) {
    if (*dm == mesh) {
      return &dm;
    }
  }

  return NULL;
}

void
Scene::deleteMeshSet (DisplayableMesh* deadMesh)
{
  MeshSetHashDelete((char*)deadMesh->getName());
  cerr << "Deleting mesh " << deadMesh->getName() << endl;
// STL Update
  vector<DisplayableMesh*>::iterator* dm = findSceneMesh (deadMesh);
  assert (dm != NULL);

// STL Update
  vector<DisplayableMesh*>::iterator mydm = find (g_hilitedScans.begin(), g_hilitedScans.end(), **dm);
  if (mydm != g_hilitedScans.end()) g_hilitedScans.erase(mydm);

  RigidScan* rs = (**dm)->getMeshData();
  // remove the pointers from the globalreg data structure
  globalReg->deleteAllPairs(rs);
  delete **dm;
  delete rs;
// STL Update
  meshSets.erase (*dm);

  // Update the bbox
  computeBBox();
}


bool
Scene::renameMeshSet (DisplayableMesh* mesh, const char* nameToUse)
{
  // TODO
  return false;
}


void
Scene::freeMeshes()
{
  while (meshSets.size()) {
    RigidScan* rs = meshSets.back()->getMeshData();  // save...
    delete meshSets.back();    // while we delete the DisplayableMesh
    delete rs;                 // before deleting RigidScan it references
    meshSets.pop_back();
  }
}


//
// Compute the bounding box for the scene
//
static void ReallyUpdateBbox (ClientData data)
{
  Scene* scene = (Scene*)data;
  scene->computeBBox (Scene::sync);
}

static Tcl_TimerToken _nextBboxUpdate = 0;

void
Scene::computeBBox (bboxSyncMode synchronous)
{
  // avoid multiple repeated calls to recompute
  if (_nextBboxUpdate) {
    Tcl_DeleteTimerHandler (_nextBboxUpdate);
    _nextBboxUpdate = 0;
  } else if (synchronous == flush) {
    // nothing to do, this just forces existing callbacks to happen now
    return;
  }

  // if not explicitly forced to happen now, wait and see if
  // we can batch together multiple calls
  if (synchronous == async) {
    _nextBboxUpdate = Tcl_CreateTimerHandler (20, ReallyUpdateBbox,
					      (ClientData)this);
    return;
  }

  // Initialize bbox
  bbox.clear();

  // Loop through meshes
  for (int k = 0; k < meshSets.size(); k++) {

    DisplayableMesh *meshDisp = meshSets[k];

    // Don't include meshes that are not being drawn or have
    // no vertices (bounding sphere has radius zero)

    if (!meshDisp->getVisible()) continue;

    // Update scene bounding box with mesh set bounding box that
    // fits around mesh set bounding sphere - conservative

    bbox.add(meshDisp->getMeshData()->worldBbox());
  }

  tbView->setClippingPlanes(worldCenter(), sceneRadius());
  DisplayCache::InvalidateToglCache (toglCurrent);
}


// position camera so entire scene is visible in center of view
void
Scene::centerCamera (void)
{
  computeBBox (sync);

  Pnt3 camera(0,0,sceneRadius()*3);
  Pnt3 up(0,1,0);
  Pnt3 object (worldCenter());
  camera += object;

  tbView->setup(camera, object, up, sceneRadius());

#if 0
  printf ("Resetting camera trackball to obj=%g,%g,%g; "
	  "camera=%g,%g,%g; radius=%g\n",
	  object[0], object[1], object[2], camera[0], camera[1],
	  camera[2], theScene->sceneRadius());
#endif
}


void
Scene::flipNormals()
{
  for (int k = 0; k < meshSets.size(); k++) {
    meshSets[k]->getMeshData()->flipNormals();
  }
}


void
Scene::setMeshResolution (int res)
{
  // if tempHigh, tempLow, implement as override -- individual mesh
  // resolutions don't change.  Otherwise, change individual mesh
  // resolutions and leave override alone.

  int newOverride = resDefault;

  bool bInvisibleToo =
    atoi (Tcl_GetVar (interp,
		      "selectResIncludesInvisible",  TCL_GLOBAL_ONLY));

  switch (res) {
  case resTempLow:  // temp lo/hi -- just change override
  case resTempHigh:
    newOverride = res;
    break;

  default:
    {
// STL Update
      vector<DisplayableMesh*>::iterator dmi;
      Progress progress (meshSets.size(), "Change resolution", true);

      for (dmi = meshSets.begin(); dmi != meshSets.end(); dmi++) {
	if (bInvisibleToo || (*dmi)->getVisible()) {
	  switch (res) {
	  case resLow:
	    (*dmi)->getMeshData()->select_coarsest();
	    break;

	  case resHigh:
	    (*dmi)->getMeshData()->select_finest();
	    break;

	  case resNextLow:
	    (*dmi)->getMeshData()->select_coarser();
	    break;

	  case resNextHigh:
	    (*dmi)->getMeshData()->select_finer();
	    break;
	  }
	}

	if (!progress.updateInc())
	  break;
      }
    }
    break;
  }

  // then, if override actually changed, rebuild caches
  if (res != resOverride) {
    invalidateDisplayCaches();
  }

  if (newOverride != resOverride) {
    resOverride = newOverride;
  }

  Tcl_Eval (interp, "buildUI_ResizeAllResBars");
}


int
Scene::getMeshResolution (void)
{
  return resOverride;
}


void
Scene::invalidateDisplayCaches (void)
{
  for (int k = 0; k < meshSets.size(); k++) {
    meshSets[k]->invalidateCachedData();
  }
}


bool
Scene::wantMeshBBox (DisplayableMesh* mesh)
{
  return (theRenderParams->boundSelection && mesh == theActiveScan);
}


void
Scene::setHome (void)
{
  homePos = *tbView;
}


void
Scene::goHome (void)
{
  *tbView = homePos;
  computeBBox();
}


float
Scene::sceneRadius (void) const
{
  return bbox.diag() * .5;
}


Pnt3
Scene::worldCenter (void)
{
  return bbox.center();
}


void
Scene::flattenCameraXform (void)
{
  // applies the camera transformation to each mesh's transformation,
  // then resets the camera transformation to identity

  float q[4], t[3];
  tbView->getXform (q, t);
  Xform<float> viewer;
  viewer.fromQuaternion (q, t[0], t[1], t[2]);

  for (int i = 0; i < meshSets.size(); i++) {
    RigidScan* scan = meshSets[i]->getMeshData();

    scan->setXform (viewer * scan->getXform());
  }

  computeBBox();
  centerCamera();
}


bool
Scene::meshes_written_stripped (void)
{
  char* mode = Tcl_GetVar (interp, "meshWriteStrips", TCL_GLOBAL_ONLY);
  if (!strcmp (mode, "always"))
    return true;
  else if (!strcmp (mode, "never"))
    return false;
  else
    return theRenderParams->useTstrips;
}


int
Scene::getSlowPolyCount (void)
{
  return slowPolyCount;
}


void
Scene::setSlowPolyCount (int count)
{
  slowPolyCount = count;
}

static Tcl_HashTable meshSetHash;
static int mesh_set_hash_initted = 0;

int
MeshSetHashDelete(char *name)
{
  if(!mesh_set_hash_initted)
    return 1;

  Tcl_HashEntry *ent = Tcl_FindHashEntry(&meshSetHash, name);
  if (ent) Tcl_DeleteHashEntry(ent);
  return 1;
}


int
AddMeshSetToHash(DisplayableMesh* mesh)
{
  if(!mesh_set_hash_initted) {
    Tcl_InitHashTable(&meshSetHash, TCL_STRING_KEYS);
    mesh_set_hash_initted = 1;
  }
  int isnew;
  Tcl_HashEntry *ent = Tcl_CreateHashEntry(&meshSetHash,
					   mesh->getName(), &isnew);
  if(!isnew)
    return(mesh == (DisplayableMesh *)Tcl_GetHashValue(ent) ? 0 : 1);

  Tcl_SetHashValue(ent, (ClientData)mesh);

  return 1;
}


DisplayableMesh *
FindMeshDisplayInfo(const char *name)
{
  if(!mesh_set_hash_initted)
    return NULL;

  Tcl_HashEntry *ent = Tcl_FindHashEntry(&meshSetHash, name);
  if (ent == NULL)
    return NULL;

  return (DisplayableMesh *)Tcl_GetHashValue(ent);
}


DisplayableMesh *
FindMeshDisplayInfo(RigidScan* scan)
{
// STL Update
  for (vector<DisplayableMesh*>::iterator pdm = theScene->meshSets.begin();
       pdm < theScene->meshSets.end();
       pdm++) {
    if ((*pdm)->getMeshData() == scan)
      return *pdm;
  }

  return NULL;
}

// FindMeshDisplayInfo(RigidScan* scan) only searches the meshSets
// vector which only returns top level ("root") matches - if we need a
// match with leaves as well, then we need to look at the hashtable
// instead because it contains all the meshes in the scene, including
// non-roots.

DisplayableMesh *
GetMeshForRigidScan (RigidScan *scan)
{
  DisplayableMesh *dm;
  Tcl_HashSearch searchPtr;
  Tcl_HashEntry *entry = Tcl_FirstHashEntry(&meshSetHash, &searchPtr);

  while (entry) {
    dm = (DisplayableMesh *)Tcl_GetHashValue(entry);
    assert (dm);
    if (dm->getMeshData() == scan) {
      cerr << "Returning " << dm->getName() << endl;
      return dm;
    }
    entry = Tcl_NextHashEntry(&searchPtr);
  }
  return NULL;
}


const char *
GetTbObjName(TbObj *tb)
{
  RigidScan* obj = dynamic_cast<RigidScan*> (tb);
  if (!obj) return NULL;
  DisplayableMesh* dm = FindMeshDisplayInfo (obj);
  if (!dm) return NULL;
  return dm->getName();
}


bool dmNameSort::operator() (DisplayableMesh* d1, DisplayableMesh* d2)
{
    int res = TclDictionaryCompare (d1->getName(), d2->getName());
    return (res < 0);
}


static int TclDictionaryCompare (const char* left, const char* right)
{
    // copied from DictionaryCompare, Tcl 8.0 source, generic/tclCmdIL.c
    // so that we get the same behavior as the interface code that
    // displays mesh lists using [lsort -dictionary]

    int diff, zeros;
    int secondaryDiff = 0;

#define UCHAR(c) ((unsigned char) (c))
    while (1) {
        if (isdigit(UCHAR(*right)) && isdigit(UCHAR(*left))) {
            /*
             * There are decimal numbers embedded in the two
             * strings.  Compare them as numbers, rather than
             * strings.  If one number has more leading zeros than
             * the other, the number with more leading zeros sorts
             * later, but only as a secondary choice.
             */

	  /* Broken TCL code alert -- magi -- Tcl 8's DictionaryCompare
	     routine breaks on strings containing embedded 0's, because
	     it takes the whole string of zeros as "leading" and skips
	     over them as if they weren't there.  Instead, it should
	     skip over leading zeros, but if it comes to a non-digit,
	     sort using the final (or only) zero, instead of skipping
	     ALL zeros.  Fix incorporated here.
	  */

            zeros = 0;
#if 1       // Tcl's way
            while ((*right == '0') && (*(right + 1) != '\0')) {
                right++;
                zeros--;
            }
            while ((*left == '0') && (*(left + 1) != '\0')) {
                left++;
                zeros++;
            }
#else       // magi's way
            while ((right[0] == '0') && (isdigit(right[1]))) {
                right++;
                zeros--;
            }
            while ((left[0] == '0') && (isdigit(left[1]))) {
                left++;
                zeros++;
            }
#endif
            if (secondaryDiff == 0) {
                secondaryDiff = zeros;
            }

            /*
             * The code below compares the numbers in the two
             * strings without ever converting them to integers.  It
             * does this by first comparing the lengths of the
             * numbers and then comparing the digit values.
             */

            diff = 0;
            while (1) {
                if (diff == 0) {
                    diff = *left - *right;
                }
                right++;
                left++;
                if (!isdigit(UCHAR(*right))) {
                    if (isdigit(UCHAR(*left))) {
                        return 1;
                    } else {
                        /*
                         * The two numbers have the same length. See
                         * if their values are different.
                         */

                        if (diff != 0) {
                            return diff;
                        }
                        break;
                    }
                } else if (!isdigit(UCHAR(*left))) {
                    return -1;
                }
            }
            continue;
        }
        diff = *left - *right;
        if (diff) {
            if (isupper(UCHAR(*left)) && islower(UCHAR(*right))) {
                diff = tolower(*left) - *right;
                if (diff) {
                    return diff;
                } else if (secondaryDiff == 0) {
                    secondaryDiff = -1;
                }
            } else if (isupper(UCHAR(*right)) && islower(UCHAR(*left))) {
                diff = *left - tolower(UCHAR(*right));
                if (diff) {
                    return diff;
                } else if (secondaryDiff == 0) {
                    secondaryDiff = 1;
                }
            } else {
                return diff;
            }
        }
        if (*left == 0) {
            break;
        }
        left++;
        right++;
    }
    if (diff == 0) {
        diff = secondaryDiff;
    }
    return diff;
}


bool
Scene::readSessionFile (char* sessionName)
{
  ifstream file (sessionName);
  if (file.fail())
    return false;

  char name [PATH_MAX];
  Pnt3 min, max;

  while (!file.eof()) {
    file.get (name, PATH_MAX, ':');
    file.ignore (10, ':');
    file >> min;
    file.ignore (10, '*');
    file >> max;
    file >> ws;
    if (file.fail())
      return false;

    RigidScan* rs = CreateScanFromBbox (name, min, max);
    DisplayableMesh* dm = addMeshSet (rs);

    Tcl_VarEval (g_tclInterp, "addMeshToWindow ",
		 dm->getName(), " 0", NULL);
  }

  return true;
}


bool
Scene::writeSessionFile (char* sessionName)
{
  ofstream file (sessionName);
  if (file.fail())
    return false;

// STL Update
  for (vector<DisplayableMesh*>::iterator pdm = meshSets.begin();
       pdm < meshSets.end(); pdm++) {
    RigidScan* scan = (*pdm)->getMeshData();
    Bbox bbox = scan->localBbox();
    file << scan->get_name() << ": "
	 << bbox.min() << " * " << bbox.max()
	 << endl;
    if (file.fail())
      return false;
  }

  return true;
}


bool
Scene::setScanLoadedStatus (DisplayableMesh* dm, bool load)
{
  RigidScan* scan = NULL;
  RigidScan* oldScan = dm->getMeshData();

  const crope& name = oldScan->get_name();
  if (load) {
    scan = CreateScanFromFile (name);
  } else {
    Bbox bbox = oldScan->localBbox();
    scan = CreateScanFromBbox (name, bbox.min(), bbox.max());
  }
  if (!scan)
    return false;

  scan->setXform (oldScan->getXform());
  dm->resetMeshData (scan);
  delete oldScan;

  return true;
}


class sceneMeshSort: public binary_function<DisplayableMesh*,
		     DisplayableMesh*,
		     bool>
{
public:
  bool operator() (DisplayableMesh* d1, DisplayableMesh* d2);
};


static vector<Scene::sortCriteria>* s_pCriteria;
static bool s_bDictSort;

void
Scene::sortSceneMeshes (vector<DisplayableMesh*>& sortedList,
			vector<sortCriteria>& criteria,
			bool bDictionarySort,
			bool bListInvisible)
{
  if (bListInvisible) {
    sortedList = meshSets;
  } else {
    sortedList.reserve (meshSets.size());
    for (int i = 0; i < meshSets.size(); i++)
      if (meshSets[i]->getVisible())
	sortedList.push_back (meshSets[i]);
  }

  s_pCriteria = &criteria;
  s_bDictSort = bDictionarySort;
  sort (sortedList.begin(), sortedList.end(), sceneMeshSort());
}


bool
sceneMeshSort::operator() (DisplayableMesh* d1, DisplayableMesh* d2)
{
  int res;

  for (int i = 0; i < s_pCriteria->size(); i++) {
    switch (s_pCriteria->operator[](i)) {

    case Scene::name:
      if (s_bDictSort)
	res = TclDictionaryCompare (d1->getName(), d2->getName());
      else
	res = strcmp (d1->getName(), d2->getName());
      if (res == 0)
	break;

      //cerr << d1->getName() << " and " << d2->getName()
      //   << ": decided by name" << endl;
      return (res < 0);

    case Scene::date:
      {
	struct stat s1, s2;
	stat (d1->getMeshData()->get_name().c_str(), &s1);
	stat (d2->getMeshData()->get_name().c_str(), &s2);
#ifdef sgi
	if (s1.st_mtim.tv_sec == s2.st_mtim.tv_sec)
#else
	if (s1.st_mtime == s2.st_mtime)
#endif
	  break;

	//cerr << d1->getName() << " and " << d2->getName()
	//     << ": decided by date" << endl;
#ifdef sgi
	if (s1.st_mtim.tv_sec < s2.st_mtim.tv_sec)
#else
	if (s1.st_mtime < s2.st_mtime)
#endif
	  return true;
	return false;
      }

    case Scene::visibility:
      if (d1->getVisible() == d2->getVisible())
	break;

      //cerr << d1->getName() << " and " << d2->getName()
      //   << ": decided by vis" << endl;
      if (d1->getVisible())
	return true;  // d1 vis, d2 not, so d1 wins
      else
	return false;

    case Scene::polycount:
      {
	vector<ResolutionCtrl::res_info> ri;
	d1->getMeshData()->existing_resolutions (ri);
	int count1 = ri[0].abs_resolution;
	ri.clear();
	d2->getMeshData()->existing_resolutions (ri);
	int count2 = ri[0].abs_resolution;

	if (count1 == count2)
	  break;

	return (count1 < count2);
      }

    case Scene::loaded:
      {
	bool loaded1 = isScanLoaded (d1->getMeshData());
	bool loaded2 = isScanLoaded (d2->getMeshData());
	if (loaded1 == loaded2)
	  break;

	//cerr << d1->getName() << " and " << d2->getName()
	//     << ": decided by load" << endl;
	if (loaded1)
	  return true;
	else
	  return false;
      }
    }
  }

  return false;
}
