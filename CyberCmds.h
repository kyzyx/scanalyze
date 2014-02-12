#ifndef _SCANALYZE_CYBER_CMDS_
#define _SCANALYZE_CYBER_CMDS_

#include <tk.h>

int PlvWriteSDForVripCmd(ClientData clientData, Tcl_Interp *interp,
			 int argc, char* argv[]);
int PlvCyberScanSelfAlignCmd(ClientData clientData, Tcl_Interp *interp,
			     int argc, char* argv[]);
int ScnDumpLaserPntsCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char* argv[]);
int PlvDiceCyberDataCmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char* argv[]);
int PlvWorkingVolumeCmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char* argv[]);
int PlvWorldCoordToSweepCoord(ClientData clientData, Tcl_Interp *interp,
                    int argc, char *argv[]);
int PlvSweepCoordToWorldCoord(ClientData clientData, Tcl_Interp *interp,
                    int argc, char *argv[]);
#endif // _SCANALYZE_CYBER_CMDS_
