#include <tk.h>
#include <stdlib.h>
#include <limits.h>

#include "togl.h"
#include "plvInit.h"
#include "plvGlobals.h"
#include "plvCmds.h"
#include "plvPlyCmds.h"
#include "plvViewerCmds.h"
#include "plvDrawCmds.h"
#include "plvCybCmds.h"
#include "plvImageCmds.h"
#include "plvClipBoxCmds.h"
#include "plvMeshCmds.h"
#include "plvScene.h"
#include "plvDraw.h"
#include "sczRegCmds.h"
#include "plvAnalyze.h"
#include "plvMMCmds.h"
#include "CyberCmds.h"
#include "Trackball.h"
#include "BailDetector.h"
#include "Progress.h"


int PlvDeinitCmd(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[]);
int PlvGetRendererStringCmd(ClientData clientData, Tcl_Interp *interp,
			    int argc, char *argv[]);
int PlvGetWordSizeCmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[]);
float getGLVersion (void);


#define PlvCreateCommand(name,func) \
    Tcl_CreateCommand (interp, (name), (func), (ClientData)main, \
		       (Tcl_CmdDeleteProc *)NULL)


int
Plv_Init(Tcl_Interp *interp)
{
  Tk_Window main;
  main = Tk_MainWindow(interp);

  g_tclInterp = interp;

  PlvCreateCommand("plv_saveCurrentGroup", PlvSaveCurrentGroup);

  PlvCreateCommand("plv_smoothMesh",PlvSmoothMesh );
  PlvCreateCommand("plv_lastRenderTime",PlvLastRenderTime );
  PlvCreateCommand("plv_sweepCoordToWorldCoord", PlvSweepCoordToWorldCoord);
  PlvCreateCommand("plv_worldCoordToSweepCoord", PlvWorldCoordToSweepCoord);
  PlvCreateCommand("plv_getVisiblyRenderedScans", PlvGetVisiblyRenderedScans);
  PlvCreateCommand("plv_getrendererstring", PlvGetRendererStringCmd);
  PlvCreateCommand("plv_getwordsize", PlvGetWordSizeCmd);

  PlvCreateCommand("plv_draw", PlvDrawCmd);
  PlvCreateCommand("plv_clearwin", PlvClearWinCmd);
  PlvCreateCommand("plv_writeiris", PlvWriteIrisCmd);
  PlvCreateCommand("plv_fillphoto", PlvFillPhotoCmd);
  PlvCreateCommand("plv_invalidateToglCache", PlvInvalidateToglCacheCmd);
  PlvCreateCommand("plv_countPixels", PlvCountPixelsCmd);

  PlvCreateCommand("plv_light", PlvLightCmd);
  PlvCreateCommand("plv_drawstyle", PlvDrawStyleCmd);
  PlvCreateCommand("plv_material", PlvMaterialCmd);
  PlvCreateCommand("plv_load_projective_texture", PlvLoadProjectiveTexture);
  PlvCreateCommand("plv_resolution", PlvMeshResolutionCmd);
  PlvCreateCommand("plv_setoverallres", PlvSetOverallResCmd);
  PlvCreateCommand("plv_scan_colorcode", PlvScanColorCodeCmd);
  PlvCreateCommand("plv_getvisible", PlvGetVisibleCmd);
  PlvCreateCommand("plv_setvisible", PlvSetVisibleCmd);

  PlvCreateCommand("plv_sort_scan_list", PlvSortScanListCmd);
  PlvCreateCommand("plv_listscans", PlvListScansCmd);
  PlvCreateCommand("plv_meshinfo", PlvMeshInfoCmd);
  PlvCreateCommand("plv_meshsetdelete", PlvMeshSetDeleteCmd);
  PlvCreateCommand("plv_camerainfo", PlvCameraInfoCmd);
  PlvCreateCommand("plv_positioncamera", PlvPositionCameraCmd);
  PlvCreateCommand("plv_zoom_to_rect", PlvZoomToRectCmd);
  PlvCreateCommand("plv_print_voxels", PlvPrintVoxelsCmd);
  /* added command to print voxel info for ply file
     - for display voxel feature */

  PlvCreateCommand("plv_param", PlvParamCmd);
  PlvCreateCommand("baildetect", PlvBailDetectCmd);
  PlvCreateCommand("plv_progress", PlvProgressCmd);
  PlvCreateCommand("plv_readcyb", PlvReadCybCmd);
  PlvCreateCommand("plv_bumpcyb", PlvBumpCybCmd);
  PlvCreateCommand("plv_shadecyb", PlvShadeCybCmd);
  PlvCreateCommand("plv_israngegrid", PlvIsRangeGridCmd);

  PlvCreateCommand("scz_session", SczSessionCmd);
  PlvCreateCommand("scz_pseudogroup", SczPseudoGroupCmd);
  PlvCreateCommand("plv_readfile", PlvReadFileCmd);
  PlvCreateCommand("plv_readgroupmembers", PlvReadGroupMembersCmd);
  PlvCreateCommand("plv_synthesize", PlvSynthesizeObjectCmd);
  PlvCreateCommand("plv_write_scan", PlvWriteScanCmd);
  PlvCreateCommand("plv_write_metadata", PlvWriteMetaDataCmd);
  PlvCreateCommand("plv_write_resolutionmesh", PlvWriteResolutionMeshCmd);
  PlvCreateCommand("plv_get_scan_filename", PlvGetScanFilenameCmd);
  PlvCreateCommand("plv_getNextAvailGroupName", PlvGetNextGroupNameCmd);
  PlvCreateCommand("plv_is_scan_modified", PlvIsScanModifiedCmd);
  PlvCreateCommand("plv_groupscans", PlvGroupScansCmd);

  PlvCreateCommand("plv_constrain_rotation", PlvConstrainRotationCmd);
  PlvCreateCommand("plv_force_keep_onscreen", PlvForceKeepOnscreenCmd);
  PlvCreateCommand("plv_ortho", PlvOrthographicCmd);
  PlvCreateCommand("plv_persp", PlvPerspectiveCmd);
  PlvCreateCommand("plv_zoomangle", PlvZoomAngleCmd);
  PlvCreateCommand("plv_oblique_camera", PlvObliqueCameraCmd);

  PlvCreateCommand("plv_viewall", PlvViewAllCmd);
  PlvCreateCommand("plv_resetxform", PlvResetXformCmd);
  PlvCreateCommand("plv_selectscan", PlvSelectScanCmd);
  PlvCreateCommand("plv_rotlight", PlvRotateLightCmd);
  PlvCreateCommand("plv_rotxyviewmouse", PlvRotateXYViewMouseCmd);
  PlvCreateCommand("plv_transxyviewmouse", PlvTransXYViewMouseCmd);
  PlvCreateCommand("plv_translateinplane", PlvTranslateInPlaneCmd);
  PlvCreateCommand("plv_undo_xform", PlvUndoXformCmd);
  PlvCreateCommand("plv_redo_xform", PlvRedoXformCmd);
  PlvCreateCommand("plv_screentoworld", PlvGetScreenToWorldCoords);
  PlvCreateCommand("plv_pickscan", PlvPickScanFromPointCmd);
  PlvCreateCommand("plv_set_this_as_center_of_rotation",
  		    PlvSetThisAsCenterOfRotation);
  PlvCreateCommand("plv_reset_rotation_center",
  		    PlvResetCenterOfRotation);
  PlvCreateCommand("SetHome", PlvSetHomeCmd);
  PlvCreateCommand("GoHome", PlvGoHomeCmd);
  PlvCreateCommand("plv_transmesh", PlvTransMeshCmd);
  PlvCreateCommand("plv_blendmesh", PlvBlendMeshCmd);
  PlvCreateCommand("plv_camera_xform_to_mesh", PlvFlattenCameraXformCmd);
  PlvCreateCommand("plv_setslowpolycount", PlvSetSlowPolyCountCmd);
  PlvCreateCommand("plv_setManipRenderMode", PlvSetManipRenderModeCmd);

  PlvCreateCommand("plv_manrotate", PlvManualRotateCmd);
  PlvCreateCommand("plv_mantranslate", PlvManualTranslateCmd);

  PlvCreateCommand("plv_clearselection", PlvClearSelectionCmd);
  PlvCreateCommand("plv_drawboxselection", PlvDrawBoxSelectionCmd);
  PlvCreateCommand("plv_drawlineselection", PlvDrawLineSelectionCmd);
  PlvCreateCommand("plv_drawshapeselection", PlvDrawShapeSelectionCmd);
  PlvCreateCommand("plv_getselectioncursor", PlvGetSelectionCursorCmd);
  PlvCreateCommand("plv_getselectioninfo", PlvGetSelectionInfoCmd);

  PlvCreateCommand("plv_clip_to_selection", PlvClipToSelectionCmd);
  PlvCreateCommand("plv_get_selected_meshes", PlvGetSelectedMeshesCmd);
  PlvCreateCommand("plv_clipBoxPlaneFit", PlvAlignToMeshBoxCmd);
  PlvCreateCommand("plv_analyze_line_depth", PlvAnalyzeClipLineDepth);
  PlvCreateCommand("plv_analyzeLineMode", PlvAnalyzeLineModeCmd);
  PlvCreateCommand("plv_export_graph_as_text", PlvExportGraphAsText);

  PlvCreateCommand("wsh_warp_mesh", wsh_WarpMesh);
  PlvCreateCommand("wsh_align_points_to_plane", wsh_AlignPointsToPlane);
  PlvCreateCommand("plv_draw_analyze_lines", PlvDrawAnalyzeLines);
  PlvCreateCommand("plv_clear_analyze_lines", PlvClearAnalyzeLines);

  PlvCreateCommand("plv_decimate", PlvMeshDecimateCmd);
  PlvCreateCommand("plv_getreslist", PlvMeshResListCmd);
  PlvCreateCommand("plv_getcurrentres", PlvCurrentResCmd);
  PlvCreateCommand("plv_setmeshpreload", PlvSetMeshResPreloadCmd);
  PlvCreateCommand("plv_mesh_res_delete", PlvMeshDeleteResCmd);
  PlvCreateCommand("plv_mesh_res_unload", PlvMeshUnloadResCmd);
  PlvCreateCommand("remove_step", PlvMeshRemoveStepCmd);
  PlvCreateCommand("FlipMeshNormals", PlvFlipMeshNormalsCmd);
  PlvCreateCommand("plv_hilitescan", PlvHiliteScanCmd);
  PlvCreateCommand("plv_render_thickness", PlvRenderThicknessCmd);

  PlvCreateCommand("plv_shutdown", PlvDeinitCmd);

  PlvCreateCommand("plv_icpregister", PlvRegIcpCmd);
  PlvCreateCommand("plv_icpreg_markquality", PlvRegIcpMarkQualityCmd);
  PlvCreateCommand("plv_showicplines", PlvShowIcpLinesCmd);
  PlvCreateCommand("bindToglToAlignmentView", PlvBindToglToAlignmentViewCmd);
  PlvCreateCommand("bindToglToAlignmentOverview",
		   PlvBindToglToAlignmentOverviewCmd);
  PlvCreateCommand("plv_correspRegParms", PlvCorrespRegParmsCmd);
  PlvCreateCommand("RegUIMouse", PlvRegUIMouseCmd);
  PlvCreateCommand("AddPartialRegCorrespondence",
		   PlvAddPartialRegCorrespondenceCmd);
  PlvCreateCommand("DeleteRegCorrespondence", PlvDeleteRegCorrespondenceCmd);
  PlvCreateCommand("ConfirmRegCorrespondence",
		   PlvConfirmRegCorrespondenceCmd);
  PlvCreateCommand("GetCorrespondenceInfo", PlvGetCorrespondenceInfoCmd);
  PlvCreateCommand("plv_registerCorresp", PlvCorrespondenceRegistrationCmd);
  PlvCreateCommand("DragRegister", PlvDragRegisterCmd);
  PlvCreateCommand("plv_globalreg", PlvGlobalRegistrationCmd);
  PlvCreateCommand("scz_auto_register", SczAutoRegisterCmd);
  PlvCreateCommand("scz_xform_scan", SczXformScanCmd);
  PlvCreateCommand("scz_get_scan_xform", SczGetScanXformCmd);

  PlvCreateCommand("mms_vriporient", MmsVripOrientCmd);
  PlvCreateCommand("mms_absorbxform", MmsAbsorbXformCmd);
  PlvCreateCommand("mms_getscanfalsecolor", MmsGetScanFalseColorCmd);
  PlvCreateCommand("mms_numscans", MmsNumScansCmd);
  PlvCreateCommand("mms_isscanvisible", MmsIsScanVisibleCmd);
  PlvCreateCommand("mms_setscanvisible", MmsSetScanVisibleCmd);
  PlvCreateCommand("mms_deletescan", MmsDeleteScanCmd);
  PlvCreateCommand("mms_flipscannorms", MmsFlipScanNormsCmd);
  PlvCreateCommand("mms_resetcache", MmsResetCacheCmd);
  PlvCreateCommand("plv_write_mm_for_vrip", PlvWriteMMForVripCmd);

  PlvCreateCommand("plv_spacecarve", PlvSpaceCarveCmd);
  PlvCreateCommand("plv_write_sd_for_vrip", PlvWriteSDForVripCmd);
  PlvCreateCommand("plv_dice_cyber_data", PlvDiceCyberDataCmd);
  PlvCreateCommand("plv_cyberscan_selfalign", PlvCyberScanSelfAlignCmd);
  PlvCreateCommand("scn_dumplaserpnts", ScnDumpLaserPntsCmd);
  PlvCreateCommand("plv_working_volume", PlvWorkingVolumeCmd);
  PlvCreateCommand("plv_saveworlddata", WriteWorldDataFromScreen);
  PlvCreateCommand("plv_savedepth", WriteOrthoDepth);

  PlvCreateCommand("plv_write_ply_for_vrip", PlvWritePlyForVripCmd);

  PlvCreateCommand("updatewindow", PlvUpdateWindowCmd);
  PlvCreateCommand("get_tick_count", SczGetSystemTickCountCmd);

  PlvCreateCommand("plv_extProg", PlvRunExternalProgram);

  tbView  = new Trackball;
  theScene = new Scene (interp);

  if (!g_bNoUI) {
    // initialize interactors
    Togl_DisplayFunc (drawInTogl);
    Togl_CreateFunc (catchToglCreate);
    Togl_OverlayDisplayFunc (drawOverlay);
    initDrawing();
  }

  char *plvDir = getenv("SCANALYZE_DIR");
  if (plvDir == NULL) {
    interp->result = "Need to set SCANALYZE_DIR environment variable.";
    return TCL_ERROR;
  }

  char plvPath[PATH_MAX];
  strcpy(plvPath, plvDir);
  strcat(plvPath, "/scanalyze.tcl");

  Tcl_SetVar (interp, "noui", g_bNoUI ? "1" : "0", TCL_GLOBAL_ONLY);
  int code = Tcl_EvalFile(interp, plvPath);
  if (code != TCL_OK) {
    interp->result = Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY);
    return TCL_ERROR;
  }

  // and, once togl widget exists (after scanalyze.tcl is sourced):
  // since these things need a GL context
  if (!g_bNoUI) {
    initDrawingPostCreation();
    g_glVersion = getGLVersion();
    Tk_CreateTimerHandler (30, SpinTrackballs, (ClientData)main);
  }

  // finally, source the user customizations -- ~/.scanalyzerc
  char* homeDir = getenv("HOME");
  if (homeDir == NULL) {
    fprintf(stderr, "Environment variable HOME not set - "
	    "will not execute $HOME/.scanalyzerc\n");
  } else {
    char rcPath[PATH_MAX];
    strcpy(rcPath, homeDir);
    strcat(rcPath, "/.scanalyzerc");
    Tcl_VarEval(interp, "file exists ", rcPath, (char *)NULL);
    if (atoi(interp->result)) {
      code = Tcl_EvalFile(interp, rcPath);
      if (code != TCL_OK) {
	char* errMsg = Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY);
	fprintf (stderr, "\nWarning: errors detected in ~/.scanalyzerc\n\n"
		 "%s\n\n"
		 "Scanalyze should be ok but your customizations probably "
		 " won't be.\n\n", errMsg);
      }
    }
  }

  return TCL_OK;
}



int
PlvDeinitCmd(ClientData clientData, Tcl_Interp *interp,
	     int argc, char *argv[])
{
#if 0
  delete theScene;
  theScene = NULL;

  //if this returns, Tk keeps processing events, and may call event
  //handlers -- in particular, it will try to redraw, which will crash
  //because there's no scene -- could also have drawMeshes() check for this.
  exit (0);
#else
  theScene->freeMeshes();
#endif

  return TCL_OK;
}


float
getGLVersion (void)
{
  const GLubyte* verStr = glGetString (GL_VERSION);
  const GLubyte* verRenderer = glGetString (GL_RENDERER);

  float ver = atof ((const char*)verStr);

  printf ("OpenGL version is %g is rendered by %s\n", ver, verRenderer);

  return ver;
}


int PlvGetRendererStringCmd(ClientData clientData, Tcl_Interp *interp,
			    int argc, char *argv[])
{
  const GLubyte* renderer = glGetString (GL_RENDERER);
  Tcl_SetResult (interp, (char*)renderer, TCL_VOLATILE);
  return TCL_OK;
}


int PlvGetWordSizeCmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[])
{
  char buf[8];
  sprintf (buf, "%ld", sizeof (long));
  Tcl_SetResult (interp, buf, TCL_VOLATILE);
  return TCL_OK;
}

