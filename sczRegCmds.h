//############################################################
// plvRegUI.h
// Matt Ginzton, Kari Pulli
// Fri Jun 12 15:19:16 PDT 1998
//
// Interface between C and Tcl code for point-registration UI
//############################################################

#ifndef _SCZ_REG_UI_
#define _SCZ_REG_UI_

#include <vector>
class DisplayableMesh;
class Pnt3;


// for correspondence registration

int
PlvBindToglToAlignmentOverviewCmd(ClientData clientData, Tcl_Interp *interp,
				  int argc, char *argv[]);

int
PlvBindToglToAlignmentViewCmd(ClientData clientData, Tcl_Interp *interp,
			      int argc, char *argv[]);

int
PlvCorrespRegParmsCmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[]);

int
PlvRegUIMouseCmd (ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[]);

int
PlvAddPartialRegCorrespondenceCmd (ClientData clientData, Tcl_Interp *interp,
				   int argc, char *argv[]);

int
PlvConfirmRegCorrespondenceCmd (ClientData clientData, Tcl_Interp *interp,
				int argc, char *argv[]);

int
PlvDeleteRegCorrespondenceCmd (ClientData clientData, Tcl_Interp *interp,
			       int argc, char *argv[]);

int
PlvGetCorrespondenceInfoCmd (ClientData clientData, Tcl_Interp *interp,
			     int argc, char *argv[]);

int
PlvCorrespondenceRegistrationCmd (ClientData clientData, Tcl_Interp *interp,
				  int argc, char *argv[]);


// for drag registration

void
DrawAlignmentMeshToBack (struct Togl* togl);


int
PlvDragRegisterCmd (ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[]);


// for multiview registration from ICP data

int
PlvGlobalRegistrationCmd (ClientData clientData, Tcl_Interp *interp,
			  int argc, char *argv[]);


int
PlvRegIcpCmd(ClientData clientData, Tcl_Interp *interp,
	     int argc, char *argv[]);

int
PlvRegIcpMarkQualityCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[]);

int
PlvShowIcpLinesCmd(ClientData clientData, Tcl_Interp *interp,
	  int argc, char *argv[]);

int
SczAutoRegisterCmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[]);

#endif

