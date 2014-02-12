# Microsoft Developer Studio Project File - Name="scanalyze" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=scanalyze - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "scanalyze.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "scanalyze.mak" CFG="scanalyze - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "scanalyze - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "scanalyze - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "scanalyze - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f scanalyze.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "scanalyze.exe"
# PROP BASE Bsc_Name "scanalyze.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "make opt"
# PROP Rebuild_Opt "clean"
# PROP Target_File "scanalyze.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "scanalyze - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f scanalyze.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "scanalyze.exe"
# PROP BASE Bsc_Name "scanalyze.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "make"
# PROP Rebuild_Opt "clean"
# PROP Target_File "scanalyze.debug32.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "scanalyze - Win32 Release"
# Name "scanalyze - Win32 Debug"

!IF  "$(CFG)" == "scanalyze - Win32 Release"

!ELSEIF  "$(CFG)" == "scanalyze - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cc"
# Begin Source File

SOURCE=.\absorient.cc
# End Source File
# Begin Source File

SOURCE=.\BailDetector.cc
# End Source File
# Begin Source File

SOURCE=.\BBox3f.cc
# End Source File
# Begin Source File

SOURCE=.\cameraparams.cc
# End Source File
# Begin Source File

SOURCE=.\colorquant.cc
# End Source File
# Begin Source File

SOURCE=.\ColorSet.cc
# End Source File
# Begin Source File

SOURCE=.\ColorUtils.cc
# End Source File
# Begin Source File

SOURCE=.\ConnComp.cc
# End Source File
# Begin Source File

SOURCE=.\ctatoply.cc
# End Source File
# Begin Source File

SOURCE=.\CyberCalib.cc
# End Source File
# Begin Source File

SOURCE=.\CyberCmds.cc
# End Source File
# Begin Source File

SOURCE=.\CyberScan.cc
# End Source File
# Begin Source File

SOURCE=.\CyberXform.cc
# End Source File
# Begin Source File

SOURCE=.\CyraResLevel.cc
# End Source File
# Begin Source File

SOURCE=.\CyraScan.cc
# End Source File
# Begin Source File

SOURCE=.\CyraSubsample.cc
# End Source File
# Begin Source File

SOURCE=.\DisplayMesh.cc
# End Source File
# Begin Source File

SOURCE=.\fitplane.cc
# End Source File
# Begin Source File

SOURCE=.\GenericScan.cc
# End Source File
# Begin Source File

SOURCE=.\GlobalReg.cc
# End Source File
# Begin Source File

SOURCE=.\GroupScan.cc
# End Source File
# Begin Source File

SOURCE=.\GroupUI.cc
# End Source File
# Begin Source File

SOURCE=.\histogram.cc
# End Source File
# Begin Source File

SOURCE=.\Image.cc
# End Source File
# Begin Source File

SOURCE=.\KDindtree.cc
# End Source File
# Begin Source File

SOURCE=.\KDtritree.cc
# End Source File
# Begin Source File

SOURCE=.\MCEdgeTable.cc
# End Source File
# Begin Source File

SOURCE=.\Mesh.cc
# End Source File
# Begin Source File

SOURCE=.\MeshTransport.cc
# End Source File
# Begin Source File

SOURCE=.\MMScan.cc
# End Source File
# Begin Source File

SOURCE=.\normquant.cc
# End Source File
# Begin Source File

SOURCE=.\plvAnalyze.cc
# End Source File
# Begin Source File

SOURCE=.\plvAux.cc
# End Source File
# Begin Source File

SOURCE=.\plvCamera.cc
# End Source File
# Begin Source File

SOURCE=.\plvClipBoxCmds.cc
# End Source File
# Begin Source File

SOURCE=.\plvCmds.cc
# End Source File
# Begin Source File

SOURCE=.\plvCybCmds.cc
# End Source File
# Begin Source File

SOURCE=.\plvDraw.cc
# End Source File
# Begin Source File

SOURCE=.\plvDrawCmds.cc
# End Source File
# Begin Source File

SOURCE=.\plvGlobals.cc
# End Source File
# Begin Source File

SOURCE=.\plvImageCmds.cc
# End Source File
# Begin Source File

SOURCE=.\plvInit.cc
# End Source File
# Begin Source File

SOURCE=.\plvMain.cc
# End Source File
# Begin Source File

SOURCE=.\plvMeshCmds.cc
# End Source File
# Begin Source File

SOURCE=.\plvMMCmds.cc
# End Source File
# Begin Source File

SOURCE=.\plvPlyCmds.cc
# End Source File
# Begin Source File

SOURCE=.\plvScene.cc
# End Source File
# Begin Source File

SOURCE=.\plvTiming.cc
# End Source File
# Begin Source File

SOURCE=.\plvViewerCmds.cc
# End Source File
# Begin Source File

SOURCE=.\plycrunch.cc
# End Source File
# Begin Source File

SOURCE=.\plyfile.cc
# End Source File
# Begin Source File

SOURCE=.\plyio.cc
# End Source File
# Begin Source File

SOURCE=.\Progress.cc
# End Source File
# Begin Source File

SOURCE=.\Projector.cc
# End Source File
# Begin Source File

SOURCE=.\ProxyScan.cc
# End Source File
# Begin Source File

SOURCE=.\qslim_embed.cc
# End Source File
# Begin Source File

SOURCE=.\qsplat_file.cc
# End Source File
# Begin Source File

SOURCE=.\QsplatScan.cc
# End Source File
# Begin Source File

SOURCE=.\rangeCyb.cc
# End Source File
# Begin Source File

SOURCE=.\RangeGrid.cc
# End Source File
# Begin Source File

SOURCE=.\rangePly.cc
# End Source File
# Begin Source File

SOURCE=.\RefCount.cc
# End Source File
# Begin Source File

SOURCE=.\RegGraph.cc
# End Source File
# Begin Source File

SOURCE=.\ResolutionCtrl.cc
# End Source File
# Begin Source File

SOURCE=.\RigidScan.cc
# End Source File
# Begin Source File

SOURCE=.\ScanFactory.cc
# End Source File
# Begin Source File

SOURCE=.\sczRegCmds.cc
# End Source File
# Begin Source File

SOURCE=.\SDfile.cc
# End Source File
# Begin Source File

SOURCE=.\spherequant.cc
# End Source File
# Begin Source File

SOURCE=.\SyntheticScan.cc
# End Source File
# Begin Source File

SOURCE=.\TbObj.cc
# End Source File
# Begin Source File

SOURCE=.\TextureObj.cc
# End Source File
# Begin Source File

SOURCE=.\ToglCache.cc
# End Source File
# Begin Source File

SOURCE=.\ToglHash.cc
# End Source File
# Begin Source File

SOURCE=.\ToglText.cc
# End Source File
# Begin Source File

SOURCE=.\Trackball.cc
# End Source File
# Begin Source File

SOURCE=.\TriangleCube.cc
# End Source File
# Begin Source File

SOURCE=.\TriMeshUtils.cc
# End Source File
# Begin Source File

SOURCE=.\TriOctree.cc
# End Source File
# Begin Source File

SOURCE=.\VertexFilter.cc
# End Source File
# Begin Source File

SOURCE=.\VolCarve.cc
# End Source File
# Begin Source File

SOURCE=.\WorkingVolume.cc
# End Source File
# Begin Source File

# Begin Group "Header Files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\absorient.h
# End Source File
# Begin Source File

SOURCE=.\accpersp.h
# End Source File
# Begin Source File

SOURCE=.\BailDetector.h
# End Source File
# Begin Source File

SOURCE=.\Bbox.h
# End Source File
# Begin Source File

SOURCE=.\cameraparams.h
# End Source File
# Begin Source File

SOURCE=.\cmdassert.h
# End Source File
# Begin Source File

SOURCE=.\colorquant.h
# End Source File
# Begin Source File

SOURCE=.\ColorSet.h
# End Source File
# Begin Source File

SOURCE=.\ColorUtils.h
# End Source File
# Begin Source File

SOURCE=.\ConnComp.h
# End Source File
# Begin Source File

SOURCE=.\ctafile.h
# End Source File
# Begin Source File

SOURCE=.\CyberCalib.h
# End Source File
# Begin Source File

SOURCE=.\CyberCmds.h
# End Source File
# Begin Source File

SOURCE=.\CyberScan.h
# End Source File
# Begin Source File

SOURCE=.\CyberXform.h
# End Source File
# Begin Source File

SOURCE=.\cyfile.h
# End Source File
# Begin Source File

SOURCE=.\CyraResLevel.h
# End Source File
# Begin Source File

SOURCE=.\CyraScan.h
# End Source File
# Begin Source File

SOURCE=.\datatypes.h
# End Source File
# Begin Source File

SOURCE=.\defines.h
# End Source File
# Begin Source File

SOURCE=.\DirEntries.h
# End Source File
# Begin Source File

SOURCE=.\DisplayMesh.h
# End Source File
# Begin Source File

SOURCE=.\DrawObj.h
# End Source File
# Begin Source File

SOURCE=.\FileNameUtils.h
# End Source File
# Begin Source File

SOURCE=.\GenericScan.h
# End Source File
# Begin Source File

SOURCE=.\GlobalReg.h
# End Source File
# Begin Source File

SOURCE=.\GroupScan.h
# End Source File
# Begin Source File

SOURCE=.\GroupUI.h
# End Source File
# Begin Source File

SOURCE=.\histogram.h
# End Source File
# Begin Source File

SOURCE=.\ICP.h
# End Source File
# Begin Source File

SOURCE=.\Image.h
# End Source File
# Begin Source File

SOURCE=.\jitter.h
# End Source File
# Begin Source File

SOURCE=.\KDindtree.h
# End Source File
# Begin Source File

SOURCE=.\KDtritree.h
# End Source File
# Begin Source File

SOURCE=.\lendian.h
# End Source File
# Begin Source File

SOURCE=.\MCEdgeTable.h
# End Source File
# Begin Source File

SOURCE=.\Median.h
# End Source File
# Begin Source File

SOURCE=.\Mesh.h
# End Source File
# Begin Source File

SOURCE=.\MeshTransport.h
# End Source File
# Begin Source File

SOURCE=.\MMScan.h
# End Source File
# Begin Source File

SOURCE=.\noDumbWinWarnings.h
# End Source File
# Begin Source File

SOURCE=.\normquant.h
# End Source File
# Begin Source File

SOURCE=.\plvAnalyze.h
# End Source File
# Begin Source File

SOURCE=.\plvAux.h
# End Source File
# Begin Source File

SOURCE=.\plvCamera.h
# End Source File
# Begin Source File

SOURCE=.\plvClipBoxCmds.h
# End Source File
# Begin Source File

SOURCE=.\plvCmds.h
# End Source File
# Begin Source File

SOURCE=.\plvConst.h
# End Source File
# Begin Source File

SOURCE=.\plvCybCmds.h
# End Source File
# Begin Source File

SOURCE=.\plvDraw.h
# End Source File
# Begin Source File

SOURCE=.\plvDrawCmds.h
# End Source File
# Begin Source File

SOURCE=.\plvGlobals.h
# End Source File
# Begin Source File

SOURCE=.\plvImageCmds.h
# End Source File
# Begin Source File

SOURCE=.\plvInit.h
# End Source File
# Begin Source File

SOURCE=.\plvMeshCmds.h
# End Source File
# Begin Source File

SOURCE=.\plvMMCmds.h
# End Source File
# Begin Source File

SOURCE=.\plvPlyCmds.h
# End Source File
# Begin Source File

SOURCE=.\plvScene.h
# End Source File
# Begin Source File

SOURCE=.\plvTiming.h
# End Source File
# Begin Source File

SOURCE=.\plvViewerCmds.h
# End Source File
# Begin Source File

SOURCE=".\ply++.h"
# End Source File
# Begin Source File

SOURCE=.\plyio.h
# End Source File
# Begin Source File

SOURCE=.\Pnt3.h
# End Source File
# Begin Source File

SOURCE=.\Progress.h
# End Source File
# Begin Source File

SOURCE=.\Projector.h
# End Source File
# Begin Source File

SOURCE=.\ProxyScan.h
# End Source File
# Begin Source File

SOURCE=.\q.h
# End Source File
# Begin Source File

SOURCE=.\qsplat_file.h
# End Source File
# Begin Source File

SOURCE=.\qsplat_frag.h
# End Source File
# Begin Source File

SOURCE=.\QsplatScan.h
# End Source File
# Begin Source File

SOURCE=.\Random.h
# End Source File
# Begin Source File

SOURCE=.\RangeGrid.h
# End Source File
# Begin Source File

SOURCE=.\rangePly.h
# End Source File
# Begin Source File

SOURCE=.\RefCount.h
# End Source File
# Begin Source File

SOURCE=.\RegGraph.h
# End Source File
# Begin Source File

SOURCE=.\RegistrationStatistics.h
# End Source File
# Begin Source File

SOURCE=.\ResolutionCtrl.h
# End Source File
# Begin Source File

SOURCE=.\RigidScan.h
# End Source File
# Begin Source File

SOURCE=.\ScanFactory.h
# End Source File
# Begin Source File

SOURCE=.\sczRegCmds.h
# End Source File
# Begin Source File

SOURCE=.\SDfile.h
# End Source File
# Begin Source File

SOURCE=.\spherequant.h
# End Source File
# Begin Source File

SOURCE=.\strings.h
# End Source File
# Begin Source File

SOURCE=.\SyntheticScan.h
# End Source File
# Begin Source File

SOURCE=.\TbObj.h
# End Source File
# Begin Source File

SOURCE=.\TextureObj.h
# End Source File
# Begin Source File

SOURCE=.\Timer.h
# End Source File
# Begin Source File

SOURCE=.\tkFont.h
# End Source File
# Begin Source File

SOURCE=.\tkInt.h
# End Source File
# Begin Source File

SOURCE=.\tkInt4.1.h
# End Source File
# Begin Source File

SOURCE=.\tkInt8.0.h
# End Source File
# Begin Source File

SOURCE=.\tkInt8.0p2.h
# End Source File
# Begin Source File

SOURCE=.\tkPort.h
# End Source File
# Begin Source File

SOURCE=.\tkWin.h
# End Source File
# Begin Source File

SOURCE=.\tkWinInt.h
# End Source File
# Begin Source File

SOURCE=.\tkWinPort.h
# End Source File
# Begin Source File

SOURCE=.\togl.h
# End Source File
# Begin Source File

SOURCE=.\ToglCache.h
# End Source File
# Begin Source File

SOURCE=.\ToglHash.h
# End Source File
# Begin Source File

SOURCE=.\ToglText.h
# End Source File
# Begin Source File

SOURCE=.\Trackball.h
# End Source File
# Begin Source File

SOURCE=.\TriMeshUtils.h
# End Source File
# Begin Source File

SOURCE=.\TriOctree.h
# End Source File
# Begin Source File

SOURCE=.\triutil.h
# End Source File
# Begin Source File

SOURCE=.\VertexFilter.h
# End Source File
# Begin Source File

SOURCE=.\VolCarve.h
# End Source File
# Begin Source File

SOURCE=.\winGLdecs.h
# End Source File
# Begin Source File

SOURCE=.\WorkingVolume.h
# End Source File
# Begin Source File

SOURCE=.\Xform.h
# End Source File
# Begin Source File

SOURCE=.\zlib_fgets.h
# End Source File
# End Group
# Begin Group "Sources (C)"

# PROP Default_Filter "c"
# Begin Source File

SOURCE=.\accpersp.c
# End Source File
# Begin Source File

SOURCE=.\cmdassert.c
# End Source File
# Begin Source File

SOURCE=.\cyfile.c
# End Source File
# Begin Source File

SOURCE=.\jitter.c
# End Source File
# Begin Source File

SOURCE=.\strings.c
# End Source File
# Begin Source File

SOURCE=.\tkConsole.c
# End Source File
# Begin Source File

SOURCE=.\togl.c
# End Source File
# Begin Source File

SOURCE=.\winMain.c
# End Source File
# End Group
# Begin Group "Tcl scripts"

# PROP Default_Filter "tcl"
# Begin Source File

SOURCE=.\analyze.tcl
# End Source File
# Begin Source File

SOURCE=.\auto_a.tcl
# End Source File
# Begin Source File

SOURCE=.\build_ui.tcl
# End Source File
# Begin Source File

SOURCE=.\clip.tcl
# End Source File
# Begin Source File

SOURCE=.\colorvis.tcl
# End Source File
# Begin Source File

SOURCE=.\file.tcl
# End Source File
# Begin Source File

SOURCE=.\help.tcl
# End Source File
# Begin Source File

SOURCE=.\histogram.tcl
# End Source File
# Begin Source File

SOURCE=.\imagealign.tcl
# End Source File
# Begin Source File

SOURCE=.\interactors.tcl
# End Source File
# Begin Source File

SOURCE=.\modelmaker.tcl
# End Source File
# Begin Source File

SOURCE=.\preferences.tcl
# End Source File
# Begin Source File

SOURCE=.\registration.tcl
# End Source File
# Begin Source File

SOURCE=.\res_ctrl.tcl
# End Source File
# Begin Source File

SOURCE=.\scanalyze.tcl
# End Source File
# Begin Source File

SOURCE=.\scanalyze_util.tcl
# End Source File
# Begin Source File

SOURCE=.\sweeps.tcl
# End Source File
# Begin Source File

SOURCE=.\tcl_util.tcl
# End Source File
# Begin Source File

SOURCE=.\visgroups.tcl
# End Source File
# Begin Source File

SOURCE=.\windows.tcl
# End Source File
# Begin Source File

SOURCE=.\working_volume.tcl
# End Source File
# Begin Source File

SOURCE=.\wrappers.tcl
# End Source File
# Begin Source File

SOURCE=.\xform.tcl
# End Source File
# Begin Source File

SOURCE=.\zoom.tcl
# End Source File
# End Group
# Begin Source File

SOURCE=.\Makedefs.win32
# End Source File
# Begin Source File

SOURCE=.\Makefile
# End Source File
# End Target
# End Project
