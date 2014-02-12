#ifndef _GLOBALS_H_
#define _GLOBALS_H_


#ifdef WIN32
#	include "winGLdecs.h"
#endif
#include "jitter.h"
#include "ToglHash.h"
#include <vector.h>

class Scene;
class DisplayableMesh;
class Trackball;
struct RenderParams;


extern bool               g_bNoUI;
extern bool               g_bNoIntensity;
extern int                Warn;
extern int                theWidth;
extern int                theHeight;
extern int                theResCap;
extern Scene             *theScene;
extern RenderParams      *theRenderParams;
extern int                SubSampleBase;
extern int                UseAreaWeightedNormals;
extern int                NumProcs;
extern Trackball         *tbView;
extern struct Togl       *toglCurrent;
extern ToglHash           toglHash;
extern struct Tcl_Interp *g_tclInterp;
extern float              g_glVersion;
extern bool               g_verbose;

// theActiveScan is the scan selected for trackball manipulation and will
// be NULL if "move viewer" is selected; theSelectedScan is the scan
// selected in the Selection window, which can still be non-NULL when the
// viewer is being manipulated.
extern DisplayableMesh   *theActiveScan;
extern DisplayableMesh   *theSelectedScan;
extern vector<DisplayableMesh*> g_hilitedScans;


#ifdef __cplusplus
extern "C" {
#endif

extern jitter_point j2[];
extern jitter_point j3[];
extern jitter_point j4[];
extern jitter_point j8[];
extern jitter_point j15[];
extern jitter_point j24[];
extern jitter_point j66[];

#ifdef __cplusplus
}
#endif


#endif // _GLOBALS_H_
