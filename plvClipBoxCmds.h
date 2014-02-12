#ifndef _PLV_CLIP_BOX_CMDS_
#define _PLV_CLIP_BOX_CMDS_

#include <tk.h>
#include <vector.h>
#include "VertexFilter.h"


class Selection
{
 public:
  // definitions
  enum Type { none, line, rect, shape };
  typedef ScreenPnt Pt;

  Selection();
  void clear();

  Pt& operator[] (int idx) { return pts[idx]; };
  const Pt& operator[] (int idx) const { return pts[idx]; };

// STL Update
  vector<Pt>::iterator iterator_at_index (int idx) { return pts.begin() + idx; };
  const vector<Pt>::iterator iterator_at_index (int idx) const { return pts.begin() + (idx); };

  // how to interpret the points?
  Type type;

  // selection data
  vector<Pt> pts;
};

extern Selection theSel;


int PlvPrintVoxelsCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);

int PlvClearSelectionCmd(ClientData clientData, Tcl_Interp *interp,
			 int argc, char *argv[]);

int PlvDrawBoxSelectionCmd(ClientData clientData, Tcl_Interp *interp,
			   int argc, char *argv[]);
int PlvDrawLineSelectionCmd(ClientData clientData, Tcl_Interp *interp,
			    int argc, char *argv[]);
int PlvDrawShapeSelectionCmd(ClientData clientData, Tcl_Interp *interp,
			     int argc, char *argv[]);

int PlvGetSelectionCursorCmd(ClientData clientData, Tcl_Interp *interp,
			     int argc, char *argv[]);

int PlvClipToSelectionCmd(ClientData clientData, Tcl_Interp *interp,
			  int argc, char *argv[]);

int PlvGetSelectionInfoCmd(ClientData clientData, Tcl_Interp *interp,
			  int argc, char *argv[]);
int PlvGetSelectedMeshesCmd(ClientData clientData, Tcl_Interp *interp,
			    int argc, char *argv[]);


unsigned char* filledPolyPixels (int& width, int& height,
				 const vector<ScreenPnt>& pts);


void drawSelection (struct Togl* togl);
void initSelection (struct Togl* togl);
void resizeSelectionToWindow (struct Togl* togl);


#endif // _PLV_CLIP_BOX_CMDS_
