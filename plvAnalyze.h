//###############################################################
// plvAnalyze.cc
// Matt Ginzton
// Thu Jun 18 13:02:22 PDT 1998
//
// Interface for reading points or groups of points from z buffer
//###############################################################


#ifndef _PLV_ANALYZE_H_
#define _PLV_ANALYZE_H_

class Trackball;
class TbObj;
class Selection;
class DisplayableMesh;
#include <tcl.h>
#include "DrawObj.h"
#include "Pnt3.h"

using namespace std;

// Lines that represent each cross section of the auto analyze
class Auto_a_Line : public DrawObj {
private:
  struct Togl *togl;
  char axis;
  int x0, y0;
  int len, space, num;
public:
  Auto_a_Line(struct Togl *_togl, char _axis, int _x0, int _y0,
	      int _len, int _space, int _num);
  ~Auto_a_Line();
  void drawthis();
};


// for things that read Z buffer, and need to detect background
void storeMinMaxZBufferValues (void);

int ScreenToWorldCoordinates (int x, int y, Pnt3& ptWorld);

int ScreenToWorldCoordinatesExt (int x, int y, Pnt3& ptWorld,
				 struct Togl* togl, Trackball* tb,
				 float fudgeFactor);
int
WriteWorldDataFromScreen(ClientData clientData, Tcl_Interp *interp,
	       int argc, char *argv[]);
int
WriteOrthoDepth(ClientData clientData, Tcl_Interp *interp,
	       int argc, char *argv[]);

bool ReadZBufferArea (vector<Pnt3>& pts, const Selection& sel,
		      bool bIncludeBlanks = false, bool bRedraw = true,
		      bool bReverseBackground = false);

bool findZBufferNeighbor (int x, int y, Pnt3& neighbor, int kMaxTries = 100);
bool findZBufferNeighborExt (int x, int y, Pnt3& neighbor,
			     struct Togl* togl, Trackball* tb,
			     int kMaxTries = 100, Pnt3* rawPt = NULL,
			     bool bRedraw = true,
			     bool bReadExistingFromFront = true);


int PlvAnalyzeClipLineDepth(ClientData clientData, Tcl_Interp *interp,
			    int argc, char *argv[]);

int PlvAnalyzeLineModeCmd(ClientData clientData, Tcl_Interp *interp,
			  int argc, char *argv[]);

int PlvAlignToMeshBoxCmd(ClientData clientData, Tcl_Interp *interp,
			 int argc, char *argv[]);

int PlvExportGraphAsText(ClientData clientData, Tcl_Interp *interp,
			 int argc, char *argv[]);

void GetPtMeshMap (int w, int h, vector<DisplayableMesh*>& ptMeshMap);

int wsh_WarpMesh(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[]);

int wsh_AlignPointsToPlane(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[]);
int
PlvDrawAnalyzeLines(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[]);
int
PlvClearAnalyzeLines(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[]);

void
GetPtMeshVector (int xstart, int ystart, int w, int h,
		 int winwidth, int winheight,
		 vector<DisplayableMesh*>& ptMeshMap);


#endif  // _PLV_ANALYZE_H_
