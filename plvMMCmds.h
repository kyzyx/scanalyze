//############################################################
// plvMMCmds.h
// Jeremy Ginsberg
// Fri Nov 13 18:08:16 CET 1998
//
// ModelMaker scan commands
//############################################################

#ifndef _PLV_MM_CMDS_
#define _PLV_MM_CMDS_

#include <tk.h>

int
MmsVripOrientCmd(ClientData clientData, Tcl_Interp *interp,
	  int argc, char *argv[]);

int
MmsGetScanFalseColorCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[]);

int
MmsAbsorbXformCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);

int
MmsNumScansCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);

int
MmsIsScanVisibleCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);

int
MmsSetScanVisibleCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);

int
MmsDeleteScanCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);

int
MmsFlipScanNormsCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);
int
MmsResetCacheCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);

int
PlvWriteMMForVripCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);


#endif
