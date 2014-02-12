#ifndef _PLV_VIEWER_CMDS_
#define _PLV_VIEWER_CMDS_

#include <tcl.h>


int PlvGetVisiblyRenderedScans(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[]);
int PlvSelectScanCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);
int PlvListScansCmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[]);
int PlvGetVisibleCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);
int PlvSetVisibleCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);
int PlvLightCmd(ClientData clientData, Tcl_Interp *interp,
		int argc, char *argv[]);
int PlvRotateLightCmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[]);
int PlvOrthographicCmd(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[]);
int PlvPerspectiveCmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[]);
int PlvViewAllCmd(ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[]);
int PlvResetXformCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);
int PlvRotateXYViewMouseCmd(ClientData clientData, Tcl_Interp *interp,
			    int argc, char *argv[]);
int PlvTransXYViewMouseCmd(ClientData clientData, Tcl_Interp *interp,
			   int argc, char *argv[]);
int PlvTranslateInPlaneCmd(ClientData clientData, Tcl_Interp *interp,
			   int argc, char *argv[]);
int PlvUndoXformCmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[]);
int PlvRedoXformCmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[]);
int PlvGetScreenToWorldCoords(ClientData clientData, Tcl_Interp *interp,
			      int argc, char *argv[]);
int PlvSetThisAsCenterOfRotation(ClientData clientData, Tcl_Interp *interp,
				 int argc, char *argv[]);
int PlvResetCenterOfRotation(ClientData clientData, Tcl_Interp *interp,
			     int argc, char *argv[]);
int PlvCameraInfoCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);
int PlvSetHomeCmd(ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[]);
int PlvGoHomeCmd(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[]);
int PlvSetOverallResCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[]);
int PlvFlattenCameraXformCmd(ClientData clientData, Tcl_Interp *interp,
			     int argc, char *argv[]);
int PlvSpaceCarveCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);

int PlvConstrainRotationCmd(ClientData clientData, Tcl_Interp *interp,
			    int argc, char *argv[]);
int PlvSetManipRenderModeCmd(ClientData clientData, Tcl_Interp *interp,
			     int argc, char *argv[]);

int PlvManualRotateCmd(ClientData clientData, Tcl_Interp *interp,
			     int argc, char *argv[]);
int PlvManualTranslateCmd(ClientData clientData, Tcl_Interp *interp,
			     int argc, char *argv[]);
int PlvPickScanFromPointCmd(ClientData clientData, Tcl_Interp *interp,
			    int argc, char *argv[]);

int PlvPositionCameraCmd(ClientData clientData, Tcl_Interp *interp,
			 int argc, char *argv[]);
int PlvZoomToRectCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);

int PlvZoomAngleCmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[]);
int PlvObliqueCameraCmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[]);

int PlvSortScanListCmd(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[]);

int PlvForceKeepOnscreenCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[]);
void SpinTrackballs (ClientData clientData);


// should the rendering window be drawn differently (high-speed) because
// the display is being interactively manipulated?
bool isManipulatingRender (void);
bool manipulatedRenderDifferent (void);


#endif
