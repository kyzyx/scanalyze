#ifndef _PLV_CYB_CMDS_
#define _PLV_CYB_CMDS_

#include <tk.h>

#ifdef __cplusplus
extern "C" {
#endif

int PlvReadCybCmd(ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[]);
int PlvBumpCybCmd(ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[]);
int PlvShadeCybCmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif

