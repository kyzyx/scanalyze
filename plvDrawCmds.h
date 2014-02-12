#ifndef _PLV_DRAW_CMDS_
#define _PLV_DRAW_CMDS_

#include <tk.h>

#ifdef __cplusplus
extern "C" {
#endif

int prepareDrawInWin(char *name);
void catchToglCreate (struct Togl* togl);
void drawInTogl (struct Togl* togl);
void drawInToglBuffer (struct Togl* togl, int buffer, bool bCacheable = false);
void redraw (bool bForceRender = false);
void drawOverlay (struct Togl *togl);
void drawOverlayAndSwap(struct Togl* togl);
long lastRenderTime(long milliseconds = -1);
unsigned long Get_Milliseconds(void);

int PlvDrawStyleCmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[]);
int PlvInvalidateToglCacheCmd(ClientData clientData, Tcl_Interp *interp,
			      int argc, char *argv[]);
int PlvMaterialCmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[]);
int PlvLoadProjectiveTexture(ClientData clientData, Tcl_Interp *interp,
			      int argc, char *argv[]);
int PlvFillPhotoCmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[]);
int PlvDrawCmd(ClientData, Tcl_Interp *interp, int argc, char *argv[]);
int PlvClearWinCmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[]);
int PlvSetSlowPolyCountCmd(ClientData clientData, Tcl_Interp *interp,
			   int argc, char *argv[]);
int PlvRenderThicknessCmd(ClientData clientData, Tcl_Interp *interp,
			  int argc, char *argv[]);
int PlvLastRenderTime(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[]);
#ifdef __cplusplus
}
#endif

#endif
