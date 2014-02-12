#include <tcl.h>
#include "togl.h"
#include "plvGlobals.h"


bool             g_bNoUI = false;
bool             g_bNoIntensity = true;
int              Warn;
int              theWidth;
int              theHeight;
int              theResCap = 0;
Scene           *theScene;
DisplayableMesh *theActiveScan = NULL;
DisplayableMesh *theSelectedScan = NULL;
vector<DisplayableMesh *> g_hilitedScans;
RenderParams    *theRenderParams;
int              SubSampleBase = 2;
Trackball       *tbView;
struct Togl     *toglCurrent = NULL;
Tcl_Interp      *g_tclInterp = NULL;
float            g_glVersion = 0;
bool             g_verbose = true;

int NumProcs = 1;
int UseAreaWeightedNormals = 0;
