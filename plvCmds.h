#ifndef _PLV_CMDS_
#define _PLV_CMDS_

#include <tcl.h>

#ifdef __cplusplus
extern "C" {
#endif

int PlvParamCmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[]);

int PlvUpdateWindowCmd(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[]);

int SczGetSystemTickCountCmd (ClientData clientData, Tcl_Interp *interp,
			      int argc, char *argv[]);

int SczSessionCmd (ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[]);

int SczPseudoGroupCmd (ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif

