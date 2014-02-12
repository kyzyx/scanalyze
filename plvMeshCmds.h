#ifndef _PLV_MESH_CMDS_
#define _PLV_MESH_CMDS_

#include <tk.h>

class DisplayableMesh;

#ifdef __cplusplus
extern "C" {
#endif

int PlvMeshResolutionCmd(ClientData clientData, Tcl_Interp *interp,
			 int argc, char *argv[]);
int PlvMeshDecimateCmd(ClientData clientData, Tcl_Interp *interp,
			 int argc, char *argv[]);
int PlvMeshResListCmd(ClientData clientData, Tcl_Interp *interp,
			 int argc, char *argv[]);
int PlvSetMeshResPreloadCmd(ClientData clientData, Tcl_Interp *interp,
			    int argc, char *argv[]);
int PlvCurrentResCmd(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[]);
int PlvMeshDeleteResCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[]);
int PlvMeshUnloadResCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[]);
int PlvMeshSetDeleteCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);
int PlvMeshInfoCmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[]);
int PlvIsRangeGridCmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[]);
int PlvTransMeshCmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[]);
int PlvMeshMaterialCmd(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[]);
int PlvMeshRemoveStepCmd(ClientData clientData, Tcl_Interp *interp,
			 int argc, char *argv[]);
int PlvScanColorCodeCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[]);
int PlvFlipMeshNormalsCmd(ClientData clientData, Tcl_Interp *interp,
			    int argc, char *argv[]);
int PlvBlendMeshCmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[]);
int PlvGroupScansCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);
int PlvHiliteScanCmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);
int SczXformScanCmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[]);
int SczGetScanXformCmd(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[]);
int PlvSmoothMesh(ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[]);
int PlvOrganizeSceneCmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[]);
int PlvRunExternalProgram(ClientData clientData, Tcl_Interp *interp,
			  int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif
