#ifndef _PLV_SCENE_
#define _PLV_SCENE_


#include "Bbox.h"
#include "Pnt3.h"
#include <vector>
#include "ColorUtils.h"
#include "Trackball.h"
#include "plvGlobals.h"
#include "DisplayMesh.h"


class GlobalReg;
class RigidScan;
class VertexFilter;


class Scene {
 private:  // private data

  Bbox      bbox;        // Bounding box around vertices in scene
  Trackball homePos;     // copy of orientation info for set/goHome
  int       resOverride; // master scene resolution, high/low/default

  int       slowPolyCount; // min. # of polys that deserve a status bar
                           // while rendering

// STL Update
  vector<DisplayableMesh*>::iterator* findSceneMesh (DisplayableMesh* mesh);

 public:   // public data

  vector<DisplayableMesh *> meshSets;   // ROOT mesh sets in scene only
  ColorSet meshColors;    // to obtain next color for false-color mode
  GlobalReg* globalReg;   // registration information
  Tcl_Interp* interp;

 public:   // public methods
  Scene (Tcl_Interp* _interp);
  ~Scene();

  DisplayableMesh* addMeshSet(RigidScan* scan,
			      bool bRecenterCamera = true,
			      char* nameToUse = NULL);
  void     addMeshInternal (DisplayableMesh* scan);
  void     deleteMeshSet (DisplayableMesh* deadMesh);
  bool     renameMeshSet (DisplayableMesh* mesh, const char* nameToUse);
  void     freeMeshes();

  enum   { resLow, resHigh, resNextLow, resNextHigh,
	   resTempLow, resTempHigh, resDefault };
  void     setMeshResolution(int res);
  int      getMeshResolution();

  bool     wantMeshBBox (DisplayableMesh* mesh = NULL);
  bool     meshes_written_stripped (void);

  typedef enum { async, sync, flush } bboxSyncMode;
  void     computeBBox (bboxSyncMode synchronous = async); // delay if possible
  void     centerCamera();

  typedef enum {
    none, name, date, visibility, loaded,
    polycount, dirname
  } sortCriteria;
  void     sortSceneMeshes (vector<DisplayableMesh*>& sortedList,
			    vector<sortCriteria>& criteria,
			    bool bDictionarySort = true,
			    bool bListInvisible = true);

  void     invalidateDisplayCaches();

  void     flipNormals (void);

  float    sceneRadius (void) const;
  Pnt3     worldCenter (void);
  const Bbox& worldBbox (void) const {return bbox;}

  void     flattenCameraXform (void);

  void     setHome (void);
  void     goHome  (void);

  int      getSlowPolyCount (void);
  void     setSlowPolyCount (int count);

  // file i/o
  bool     readSessionFile (char* name);
  bool     writeSessionFile (char* name);
  bool     setScanLoadedStatus (DisplayableMesh* dm, bool load);
};


// maybe these should be Scene members, but for now they're global
DisplayableMesh *FindMeshDisplayInfo(const char *name);// hashed -- fast

//The first one only looks at root scans for a match, the
//second one looks at the leaves as well. For most purposes the first
//one suffices, but there may be instances such in groups when you
//need to find non-roots.
DisplayableMesh *FindMeshDisplayInfo(RigidScan* scan); // linsrch -- slow
DisplayableMesh *GetMeshForRigidScan (RigidScan *scan); // walks over hashtable

int AddMeshSetToHash(DisplayableMesh *mesh);
const char *GetTbObjName(TbObj *tb);
int MeshSetHashDelete(char *name);

#endif   // _PLV_SCENE_
