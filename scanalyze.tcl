#!scanalyze


# import all helper functions
# order is only semi-important because mostly these files contain
# only procs that aren't executed until after all of them are sourced

catch {source $env(HOME)/.scanalyzehooks}
source $env(SCANALYZE_DIR)/tcl_util.tcl
source $env(SCANALYZE_DIR)/scanalyze_util.tcl
source $env(SCANALYZE_DIR)/wrappers.tcl
source $env(SCANALYZE_DIR)/preferences.tcl
source $env(SCANALYZE_DIR)/file.tcl
source $env(SCANALYZE_DIR)/registration.tcl
source $env(SCANALYZE_DIR)/globalreg.tcl
source $env(SCANALYZE_DIR)/res_ctrl.tcl
source $env(SCANALYZE_DIR)/analyze.tcl
source $env(SCANALYZE_DIR)/clip.tcl
source $env(SCANALYZE_DIR)/windows.tcl
source $env(SCANALYZE_DIR)/build_ui.tcl
source $env(SCANALYZE_DIR)/interactors.tcl
source $env(SCANALYZE_DIR)/modelmaker.tcl
source $env(SCANALYZE_DIR)/auto_a.tcl
source $env(SCANALYZE_DIR)/xform.tcl
source $env(SCANALYZE_DIR)/imagealign.tcl
source $env(SCANALYZE_DIR)/colorvis.tcl
source $env(SCANALYZE_DIR)/working_volume.tcl
source $env(SCANALYZE_DIR)/visgroups.tcl
source $env(SCANALYZE_DIR)/sweeps.tcl
source $env(SCANALYZE_DIR)/zoom.tcl
source $env(SCANALYZE_DIR)/help.tcl
source $env(SCANALYZE_DIR)/organize.tcl
source $env(SCANALYZE_DIR)/alignmentbrowser.tcl
source $env(SCANALYZE_DIR)/chooseDir.tcl
source $env(SCANALYZE_DIR)/vripUI.tcl
source $env(SCANALYZE_DIR)/group.tcl

# give initial values to variables that need to be declared
set theMesh ""
set uniqueInt 1
set rotCenterVisible 0
set rotCenterOnTop 0
set drawResolution "default"
set oldMover invalid
set oldMesh ""
set thePolygonMode fill
set theBackfaceMode lit
set theShadeModel smooth
set theLightingMode 1
set theShinyMode 1
set theCullMode 1
set theEmissiveMode 0
set theFlipNormalMode 0
set isDoubleBuffer 1
set isOrthographic 0
set theColorMode gray
set styleAntiAlias 0
set styleShadows 0
set styleDispList 0
set styleBbox 1
set styleAAsamps 8
set enabledWhenMeshSelected ""
set clipEnabled 0
set globalRegMethod horn
set dragRegMethod horn
set correspRegColorPoints 1
set meshCount 0
set polyCount 0
set visPolyCount 0
set noRedraw 0
set rotationConstraint none
set saveICPForGlobal 1
set transparentSelection 0
set _idlecallbacks() ""
set theCoR_save() auto
set highQualHideBbox 1
set pfcrData data_zbuffer


# set up bindings for variables
traceUIVariables
# create visible UI
buildScanalyzeUI


# also set preferences:
# persistant vars that have their own UI (ie, in menus)
prefs_AutoPersistVar manipRender(Points) 1
prefs_AutoPersistVar manipRender(TinyPoints) 0
prefs_AutoPersistVar manipRender(Unlit) 0
prefs_AutoPersistVar manipRender(Lores) 0
prefs_AutoPersistVar manipRender(SkipDisplayList) 1
prefs_AutoPersistVar manipRender(Thresh) 0
prefs_AutoPersistVar meshControlsSort(1) Name
prefs_AutoPersistVar meshControlsSort(2) Name
prefs_AutoPersistVar meshControlsSort(3) Name
prefs_AutoPersistVar meshControlsSort(4) Name
prefs_AutoPersistVar meshControlsSort(5) Name
prefs_AutoPersistVar meshControlsSort(mode) dictionary
prefs_AutoPersistVar styleTStrip 1
prefs_AutoPersistVar selectResIncludesInvisible 0
prefs_AutoPersistVar icpRegListsVisOnly 1


# things that go in Preferences dialog
prefs_AutoPrefsVar styleBbox "Show bounding box for manipulee" \
    bool 1 { plv_drawstyle -bbox [globalset styleBbox]; redraw 1 }
prefs_AutoPrefsVar styleAutoClearSel "Clear selection when trackball moves"\
    bool 1
prefs_AutoPrefsVar exitConfirmation "Confirm exit" \
    {{always Always} {dirty "If meshes have changed"} {never Never}} always
prefs_AutoPrefsVar exitSaveXforms "Save all scan xforms before exit" \
    bool 0
prefs_AutoPrefsVar theUnitScale "Mesh units are measured in" \
    {{1 Millimeters} {1000 Meters}} 1
prefs_AutoPrefsVar selectionFollowsVisibility \
    "Selected mesh follows visibility" \
    bool 1
prefs_AutoPrefsVar autoCenterCamera \
    "Reset viewer position when adding scan to scene" \
    bool 1
prefs_AutoPrefsVar warn64bit \
    "Warn when running 32bit version on 64bit machine" bool 1
prefs_AutoPrefsVar meshWriteStrips \
    "Write mesh triangles as strips" \
    {{always Always} {never Never} {asviewed "If view is stripped"}} \
    always
prefs_AutoPrefsVar stylePointSize \
    "Point size for point renderings" \
    {{1 1} {2 2} {3 3} {4 4} {5 5}} 3
prefs_AutoPrefsVar subsamplePreserveHoles \
    "Subsampling behavior" \
    {{holes "Holes always grow"} {conf "Calculate confidence"}
	{filter "Average"} {fast "Fast"}} fast
prefs_AutoPrefsVar spinningAllowed \
    "Allow spinning" \
    bool 1
prefs_AutoPrefsVar showScanHilites \
    "Hilite bounding box for" \
    {{always "All scans"} {visible "Visible scans"} {never "No scans"}} \
    visible
prefs_AutoPrefsVar regColorTransitive \
    "Registration-status color includes transitive connections" bool 0
prefs_AutoPrefsVar allowProxiesWithGR \
    "Allow proxy creation when loading *.gr files" bool 0
prefs_AutoPrefsVar writeGRsRaw \
    "Write CyberScan *.gr files as raw" bool 0
#prefs_AutoPrefsVar wantRedrawStatus \
#    "Show render status" \
#    {{0 "None"} {1 "A little"} {2 "A lot"}} 2
prefs_AutoPrefsVar removeStepedges \
    "Remove stepedges while triangulating meshes" bool 1

# more defaults
set theMover viewer
set slowPolyCount 1000000
hiliteRotationMode
selectScan ""
manipulateScan ""
plv_material -confscale 1
plv_drawstyle -cull $theCullMode
setShininess 1
set theUnitScale 1


# cd is a dangerous command, because things that were loaded from a
# relative path won't be able to save themselves
overrideCdCommand


# also dangerous is even running the 32-bit version these days, given the
# data sizes we're working with, so if this machine supports better, the
# user's probably making a mistake
if {$warn64bit == 1} checkOSversion


# done!  lift off...
