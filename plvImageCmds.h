#ifndef _PLV_IMAGE_CMDS_
#define _PLV_IMAGE_CMDS_

#include <tk.h>

int PlvWriteIrisCmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[]);
int PlvCountPixelsCmd (ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[]);

#endif // _PLV_IMAGE_CMDS_
