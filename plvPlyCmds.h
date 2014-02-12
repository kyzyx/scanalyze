#ifndef _PLV_PLY_CMDS_
#define _PLV_PLY_CMDS_

#include <tk.h>
#include "Xform.h"

int PlvReadFileCmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[]);

int PlvWriteScanCmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[]);
int PlvWriteMetaDataCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[]);
int PlvWriteResolutionMeshCmd(ClientData clientData, Tcl_Interp *interp,
			      int argc, char *argv[]);
int PlvIsScanModifiedCmd(ClientData clientData, Tcl_Interp *interp,
			 int argc, char *argv[]);
int PlvGetScanFilenameCmd(ClientData clientData, Tcl_Interp *interp,
			  int argc, char* argv[]);

int PlvSynthesizeObjectCmd(ClientData clientData, Tcl_Interp *interp,
			   int argc, char *argv[]);

bool matrixFromString (char* str, Xform<float>& xf);

int PlvWritePlyForVripCmd(ClientData clientData, Tcl_Interp *interp,
			  int argc, char *argv[]);
int PlvSaveCurrentGroup (ClientData clientData, Tcl_Interp *interp,
			 int argc, char *argv[]);
int PlvReadGroupMembersCmd (ClientData clientData, Tcl_Interp *interp,
			    int argc, char *argv[]);
int PlvGetNextGroupNameCmd (ClientData clientData, Tcl_Interp *interp,
			    int argc, char *argv[]);

#endif // _PLV_PLY_CMDS_
