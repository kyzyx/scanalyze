######################################################################
#
# build_ui.tcl
# created 10/29/98  magi
#
# create visible user interface -- main window interactors and menubar
#
######################################################################
#
# exports:
#   buildScanalyzeUI
#   addMeshToWindow
#   removeMeshFromWindow
#   buildUI_shutdown
#   hiliteScanEntry
#
######################################################################



############################################################################
#
# Create the complete visible UI
#
############################################################################

proc buildScanalyzeUI {} {

global errorInfo

    if {![globalset noui]} {
	detectCursorCapabilities
	buildUI_ScanalyzeMenus
	buildUI_ToglContextMenu
	buildUI_MainWindow
	buildUI_MeshControls
	buildUI_BindShortcuts
	buildUI_PositionWindows
	buildUI_BuildNudgeWindow
	buildUI_BuildVersionWindow

	bind all <Control-q> { confirmQuit}
    }

    globalset tcl_prompt1 {puts -nonewline "scanalyze> "}
    globalset tcl_prompt2 {puts -nonewline "> "}
}


proc buildUI_shutdown {} {
    # save any persistent settings before exit
    buildUI_SaveWindowPositions save
}


############################################################################
#
# add a mesh to the mesh controls window so it can be manipulated
#
############################################################################

proc addMeshToWindow {name {loaded 1}} {
    global meshCount
    global meshLoaded

    if {![globalset noui]} {
	global leftFrame
	global meshFrame

	set i [getUniqueInt]

	set fr $leftFrame.meshFrame$i
	if {[catch {set newFrame [buildUI_privAddMeshAsFrame \
				      $name $fr]} err]} {
	    puts "$err"
	    destroy $fr
	    return
	}
	set meshFrame($name) $newFrame
	pack $newFrame -side top -anchor e
    }

    incr meshCount
    set meshLoaded($name) $loaded
    updatePolyCount

    # always select the first scan loaded into scanalyze...
    if {$meshCount == 1} {
	selectScan $name
    }

    resizeMCscrollbar
}

############################################################################
# Changes the size of resolution bars in mesh controls window
############################################################################
proc buildUI_ResizeResBars {name inframe} {

    set curres [plv_getcurrentres $name]
    set maxres [getPolyCount_maxres $name]
    if {$maxres > 0} {
	if {$curres > 0} {
	    set curres [expr round(10 * (log10($curres) - 3))]
	}
	set maxres [expr round(10 * (log10($maxres) - 3))]
    }

    # anything less than 1000 gets set to 1 pix..
    if { $curres < 0 } {
	set curres 1
    }
    if { $maxres < 0 } {
	set maxres 1
    }

    # make them always visibly wide, for false color convenience
    incr curres 5
    incr maxres 5

    if {$curres == $maxres} {
	# hacks to make maxres bar invisible, since we can't make
	# it narrower than 1 pixel
	$inframe.title.maxBar config -width 1 \
	    -bg [lindex [$inframe.title.curBar config -bg] 4]
    } else {
	$inframe.title.maxBar config -width [expr $maxres - $curres] \
	    -bg "#000000"
    }
    $inframe.title.curBar config -width $curres

    return $maxres
}


proc buildUI_ResizeAllResBars {} {
    global meshFrame
    set meshes [getMeshList]

    foreach mesh $meshes {
	buildUI_ResizeResBars $mesh $meshFrame($mesh)
    }
}

proc toggleRangeFromActiveMeshTo {mesh} {
    global meshVisible
    set meshes [getSortedMeshControlsList]
    set start [lsearch -exact $meshes $mesh]
    set stop [lsearch -exact $meshes [globalset theMesh]]

    if {$stop < $start} {
	set start [expr $start - 1]
	swap start stop
    } else { incr start }
    set range [lrange $meshes $start $stop]

    without_redraw {
	foreach m $range {
	    changeVis $m [expr ! $meshVisible($m)]
	}
    } maskerrors
}

proc buildUI_privAddMeshAsFrame {name inframe} {
    global meshVisible
    global meshLoaded
    global menuView

    set titleFrame [frame $inframe]
    set meshFrame [frame $titleFrame.title]
    set meshLabel [label $meshFrame.label -text $name]
    set meshColor [frame $meshFrame.color -width 10 -height 10 \
		       -background [GetMeshFalseColor $name]]
    set radioMove [radiobutton $meshFrame.active -variable theMesh \
		       -value $name -command [list selectScan $name]]
    set checkVis [checkbutton $meshFrame.visible \
		      -variable meshVisible($name) \
		      -command [list changeVis $name]]
    set loaded [checkbutton $meshFrame.loaded \
		    -variable meshLoaded($name) \
		    -command [list changeLoaded $name]]

    set meshVisible($name) 1

    # resolution indicator bars
    set max [frame $meshFrame.maxBar\
		 -width 0 -height 10 -background black]
    set cur [frame $meshFrame.curBar\
		 -width 0 -height 10 -background [GetMeshFalseColor $name]]

    #cheap hack for spacing
    #assumes a max of 10^7 triangles (based on formula in resizeResBars)
    #the number 45 comes from 10 * (log10(10^7) - 3) + 5
    set maxpix [buildUI_ResizeResBars $name $inframe]
    set pad [frame $meshFrame.pad -width [expr 45 - $maxpix] \
		 -height 10 -background "#d9d9d9"]

    pack $loaded $pad $max $cur $radioMove $checkVis $meshLabel\
	-side right -anchor e

    if {[plv_groupscans isgroup $name]} {
 	set exp [label $meshFrame.expand -text "+" -anchor w]
	bind $exp <ButtonPress-1> "group_expandGroup $name"
	pack $exp -side left -anchor w
    }

    # don't need this now that res-indicator shows the false color
    #pack $meshColor -side right

    set meshMenu [menu $meshLabel.menu -tearoffcommand \
		      "setMenuTitle $name"]
    $meshMenu add cascade -label "Resolution" -menu $meshMenu.resMenu
    $meshMenu add separator
    $meshMenu add command -label "Flip normals" \
	-command "invertMeshNormals $name"
    $meshMenu add command -label "Reset transform" \
	-command "plv_resetxform $name"
    $meshMenu add command -label "Get info" \
	-command "showMeshInfo $name"
    $meshMenu add command -label "Change false color" \
	-command "chooseMeshFalseColor $name"
    $meshMenu add separator
    $meshMenu add command -label "Reload" -command "reloadMesh $name"
    $meshMenu add command -label "Delete" -command "confirmDeleteMesh $name"
    ss_addConvertSweepScanToMeshControlMenu $meshMenu $name

    menu $meshMenu.resMenu \
	-postcommand "buildMeshResList $name $meshMenu.resMenu"
    foreach control \
	"$meshLabel $meshColor $checkVis $radioMove $cur $max $loaded" {
	bind $control <ButtonPress-3> "tk_popup $meshMenu %X %Y"
    }
    bind $checkVis <Control-ButtonPress-1> "toggleRangeFromActiveMeshTo $name"

    bind $meshFrame <Enter> "plv_hilitescan $name"
    bind $meshFrame <Leave> "plv_hilitescan"

    pack $meshFrame -side top -fill both -expand 1
    return $titleFrame
}


############################################################################
#
# remove a mesh from the mesh controls window (presumably, after deletion)
#
############################################################################

proc removeMeshFromWindow {meshes} {
    global meshFrame
    global meshVisible
    global meshCount

    set needReactivate 0
    foreach mesh $meshes {
	destroy $meshFrame($mesh)
	unset meshFrame($mesh)
	unset meshVisible($mesh)
	set meshCount [expr $meshCount - 1]

	if {[globalset theMesh] == $mesh} {
	    set needReactivate 1
	}
    }

    if {$needReactivate} {
	globalset theMesh [lindex [array names meshVisible] 0]
    }

    updatePolyCount
    resizeMCscrollbar
}


proc progress {args} {
    # this got moved to C code
    eval plv_progress $args
}


# private functions follow


############################################################################
#
# build the render menu, used by the menubar and the togl context menu
#
############################################################################

proc buildUI_RenderMenu {name tearname} {
    set menuRender [menu $name \
			-tearoffcommand "setMenuTitle \"$tearname\""]

    $menuRender add radiobutton -label Polygons \
	-variable thePolygonMode -value fill
    $menuRender add radiobutton -label Wireframe \
	-variable thePolygonMode -value line
    $menuRender add radiobutton -label "Hidden line" \
	-variable thePolygonMode -value hiddenline
    $menuRender add radiobutton -label Points \
	-variable thePolygonMode -value point
    $menuRender add radiobutton -label "Hidden Points" \
	-variable thePolygonMode -value hiddenpoint
    $menuRender add separator
    $menuRender add radiobutton -label "Smooth per-vertex" \
	-variable theShadeModel -value smooth \
	-command {plv_drawstyle -shademodel $theShadeModel; redraw}
    $menuRender add radiobutton -label "Flat per-vertex" \
	-variable theShadeModel -value fakeflat \
	-command {plv_drawstyle -shademodel $theShadeModel; redraw}
    $menuRender add radiobutton -label "Flat per-face (slow)" \
	-variable theShadeModel -value realflat \
	-command {plv_drawstyle -shademodel $theShadeModel; redraw}
    $menuRender add separator
    set menuColor [createNamedCascade $menuRender menuColors \
		       "Change colors"]
    $menuColor add command -label Background -command "chooseColor background"
    $menuColor add separator
    $menuColor add command -label Materials: -state disabled
    $menuColor add command -label Specular -command "chooseColor specular"
    $menuColor add command -label Diffuse -command "chooseColor diffuse"
    $menuColor add separator
    $menuColor add command -label Back\ Materials: -state disabled
    $menuColor add command -label Specular -command "chooseColor backSpecular"
    $menuColor add command -label Diffuse -command "chooseColor backDiffuse"
    $menuColor add separator
    $menuColor add command -label Lighting: -state disabled
    $menuColor add command -label Ambient -command "chooseColor lightambient"
    $menuColor add command -label Diffuse -command "chooseColor lightdiffuse"
    $menuColor add command -label Specular -command "chooseColor lightspecular"
    $menuRender add separator
    $menuRender add checkbutton -label Lit \
	-variable theLightingMode \
	-command {plv_drawstyle -lighting $theLightingMode; redraw}
    $menuRender add checkbutton -label Shiny \
	-variable theShinyMode \
	-command {setShininess $theShinyMode; redraw}
    $menuRender add checkbutton -label Emissive \
	-variable theEmissiveMode \
	-command {plv_drawstyle -emissive $theEmissiveMode; redraw}
    $menuRender add checkbutton -label "Backface culling" \
	-variable theCullMode \
	-command {plv_drawstyle -cull $theCullMode; redraw}
# two sided lighting now implements this feature
#    $menuRender add checkbutton -label "Emissive backface like background" \
#	-variable theEmissiveBackfaceMode \
#	-command {plv_drawstyle -backfaceemissive $theEmissiveBackfaceMode;
#	    if {$theEmissiveBackfaceMode && $theCullMode} {
#		set theCullMode 0; plv_drawstyle -cull 0;
#	    }
#	    redraw}
    $menuRender add checkbutton -label "Two sided lighting" \
	-variable theTwoSidedLighting \
	-command {plv_drawstyle -twosidedlighting $theTwoSidedLighting; redraw}
    $menuRender add checkbutton -label "Flip normals" \
	-variable theFlipNormalMode \
	-command {plv_drawstyle -flipnorm $theFlipNormalMode; redraw}
    $menuRender add checkbutton -label "Selected mesh transparent" \
	-variable transparentSelection
    $menuRender add separator
    $menuRender add radiobutton -label "Lit backface" \
        -variable theBackfaceMode -value lit
    $menuRender add radiobutton -label "Emissive backface" \
	-variable theBackfaceMode -value emissive
    $menuRender add separator
    $menuRender add radiobutton -label "Gray color" \
	-variable theColorMode -value gray
    $menuRender add radiobutton -label "False color" \
	-variable theColorMode -value false
    $menuRender add radiobutton -label "Intensity" \
	-variable theColorMode -value intensity
    $menuRender add radiobutton -label "True color" \
	-variable theColorMode -value true
    $menuRender add radiobutton -label "Boundary" \
	-variable theColorMode -value boundary
    $menuRender add radiobutton -label "Confidence" \
	-variable theColorMode -value confidence
    $menuRender add radiobutton -label "Registration status" \
	-variable theColorMode -value registration
    $menuRender add radiobutton -label "Texture mapped" \
	-variable theColorMode -value texture

    return $menuRender
}


######################################################################
#
# clip menu treated slightly differently, but same effect -- you
# create it where you want it, then call this to fill it in, again
# decomposed so we can put one on the togl context menu
#
######################################################################

proc buildUI_populateClipMenu {clipMenu} {
    $clipMenu add command -label "Clip current mesh:" -state disabled
    $clipMenu add command -label "TO (discard all but selection)" \
	-command {doClipMesh $theMesh [list sel inplace inside]}
    $clipMenu add command -label "AWAY (discard selection)" \
	-command {doClipMesh $theMesh [list sel inplace outside]}
    $clipMenu add command -label "COPY (selection to new mesh)" \
	-command {doClipMesh $theMesh [list sel newmesh inside]}
    $clipMenu add separator
    $clipMenu add command -label "Split (remove selection to new mesh)" \
	-command { doClipSplitMesh $theMesh }
    $clipMenu add separator
    $clipMenu add command -label "Clip..." -command clipDialog
    $clipMenu add separator
    $clipMenu add command -label "Clear selection" -command clearSelection
}



######################################################################
#
# create main menubar
#
######################################################################

proc buildUI_ScanalyzeMenus {} {
    menu .menubar
    . config -menu .menubar

    set menuFile [createNamedCascade .menubar menuFile File "-underline 0"]
    $menuFile add command -label "Open scan..." \
	    -command { openScanFile }
    $menuFile add command -label "Open SD scan directory..." \
	    -command { openScanDir }
    $menuFile add command -label "Open session..." \
	    -command { fileSession open }
    $menuFile add command -label "Open group..." \
	-command { openGroupFile }
    $menuFile add separator
    $menuFile add command -label "Save current scan" \
	    -command { saveScanFile $theMesh }
    $menuFile add command -label "Save current scan as..." \
	    -command { saveScanFileAs $theMesh }
    $menuFile add command -label "Save all..." \
	    -command { confirmSaveMeshes }
    $menuFile add command -label "Save all visible to directory..." \
	    -command { saveVisibleScans }
    $menuFile add command -label "Save session..." \
	    -command { fileSession save }
    $menuFile add command -label "Save current group" \
	    -command { group_saveCurrentGroup  }
    $menuFile add command -label "Save current group to directory..." \
	    -command { group_saveCurrentGroupToDir  }
    $menuFile add separator
    $menuFile add command -label "Flatten all scans..." \
	-command {confirmFlattenMeshes }
    $menuFile add separator
    $menuFile add command -label "Export current scan as..." \
	-command {saveScanAsPlySet $theMesh }
    $menuFile add command -label "Export current mesh as..." \
	-command { saveScanResolutionMeshAs $theMesh }
    $menuFile add command -label "Export transformed mesh as..." \
	-command { saveScanResolutionMeshAs $theMesh extended }
    $menuFile add command -label "Export all scans as plysets" \
	-command saveAllScansAsPlySets
    $menuFile add command -label "Export rendered image as..." \
	-command saveScreenDump
    set menuRemoteDisplay [createNamedCascade .menubar.menuFile remoteDisplay \
			       "Export to remote display" ]

    $menuRemoteDisplay add command -label "Other..." \
	-command remoteDisplayUI
    $menuRemoteDisplay add separator
    $menuRemoteDisplay add command -label "nurb" \
	-command {sendImageTo nurb}
    $menuRemoteDisplay add command -label "cesello" \
	-command {sendImageTo cesello}
    $menuRemoteDisplay add command -label "brdf" \
	-command {sendImageTo brdf}
    $menuRemoteDisplay add command -label "enoriver" \
	-command {sendImageTo enoriver}


    $menuFile add separator
    $menuFile add command -label "Write scan xform" \
	-command { saveScanMetaData $theMesh xform }
    $menuFile add command -label "Write all xforms" \
	-command fileWriteAllScanXforms -accelerator ^X
    $menuFile add command -label "Write visible xforms" \
	-command fileWriteVisibleScanXforms
    $menuFile add separator
    $menuFile add command -label "Exit" -command confirmQuit -accelerator ^Q

    set menuView [createNamedCascade .menubar menuView View "-underline 0"]
    set menuViewRes [createNamedCascade $menuView res Resolution]
    $menuViewRes add radiobutton -label "View selected resolution" \
	-value default -variable drawResolution
    $menuViewRes add radiobutton -label "View highest resolution" \
	-value temphigh -variable drawResolution
    $menuViewRes add radiobutton -label "View lowest" \
	-value templow -variable drawResolution
    $menuViewRes add separator
    $menuViewRes add command -label "Select highest for all" \
	-command "set drawResolution high" -accelerator "Ctrl->"
    $menuViewRes add command -label "Next higher for all" \
	-command "set drawResolution nexthigh" -accelerator >
    $menuViewRes add command -label "Next lower for all" \
	-command "set drawResolution nextlow" -accelerator <
    $menuViewRes add command -label "Select lowest for all" \
	-command "set drawResolution low" -accelerator "Ctrl-<"
    $menuViewRes add separator
    $menuViewRes add command -label "Select Nth for all..." \
	-command "buildUI_selectNthResolution"
    $menuViewRes add command -label "Select above N for all..." \
	-command selectResAboveUI
    $menuViewRes add separator
    $menuViewRes add checkbutton -label "'All' includes invisible meshes" \
	-variable selectResIncludesInvisible

    set menuViewSpeed [createNamedCascade $menuView speed \
			   "Render speed while moving"]
    $menuViewSpeed add checkbutton -label "Draw points instead of tris" \
	-variable manipRender(Points)
    $menuViewSpeed add checkbutton -label "Use tiny points" \
	-variable manipRender(TinyPoints)
    $menuViewSpeed add checkbutton -label "Disable lighting" \
	-variable manipRender(Unlit)
    $menuViewSpeed add checkbutton -label "Use lowest resolution" \
	-variable manipRender(Lores)
    $menuViewSpeed add separator
    $menuViewSpeed add checkbutton -label "Prefer speedups to display list"\
	-variable manipRender(SkipDisplayList)
    $menuViewSpeed add separator
    $menuViewSpeed add command -state disabled \
	-label "Threshold for fast draw:"
    $menuViewSpeed add radiobutton -label "None (always draw fast)" \
	-variable manipRender(Thresh) -value 0
    $menuViewSpeed add radiobutton -label "50k polys" \
	-variable manipRender(Thresh) -value 50000
    $menuViewSpeed add radiobutton -label "100k polys" \
	-variable manipRender(Thresh) -value 100000
    $menuViewSpeed add radiobutton -label "200k polys" \
	-variable manipRender(Thresh) -value 200000
    $menuViewSpeed add radiobutton -label "500k polys" \
	-variable manipRender(Thresh) -value 500000
    $menuViewSpeed add radiobutton -label "1M polys" \
	-variable manipRender(Thresh) -value 1000000
    $menuViewSpeed add radiobutton -label "5M polys" \
	-variable manipRender(Thresh) -value 5000000
    $menuViewSpeed add radiobutton -label "10M polys" \
	-variable manipRender(Thresh) -value 10000000
    $menuViewSpeed add radiobutton -label "50M polys" \
	-variable manipRender(Thresh) -value 50000000
    $menuViewSpeed add radiobutton -label "100M polys" \
	-variable manipRender(Thresh) -value 100000000
    $menuViewSpeed add radiobutton -label "Never (disable fastdraw)" \
	-variable manipRender(Thresh) -value -1

    $menuView add separator
    $menuView add checkbutton -label "Use triangle strips" \
	-variable styleTStrip
    $menuView add checkbutton -label "Use display lists" \
	-variable styleDispList
    $menuView add checkbutton -label "Don't re-render static images" \
	-variable styleCacheRender
    $menuView add separator
    set menuViewHQ [createNamedCascade $menuView highquality \
			"Add realism to current rendering"]
    $menuViewHQ add command -label "Anti-aliasing" \
	-command { highQualityDraw 1 -1 }
    $menuViewHQ add command -label "Shadows" \
	-command { highQualityDraw -1 1 }
    $menuViewHQ add command -label "Soft shadows and anti-aliasing" \
	-command { highQualityDraw 1 1 }
    $menuViewHQ add separator
    $menuViewHQ add command -label "Soft shadow length..." \
	-command setSoftShadowLength
    $menuViewHQ add separator
    $menuViewHQ add checkbutton -label "Hide bounding box for HQ" \
	-variable highQualHideBbox
    $menuView add checkbutton -label "Always anti-alias (slow!)" \
	-command { plv_drawstyle -antialias $styleAntiAlias; \
		       $toglPane render } \
	-variable styleAntiAlias
    $menuView add checkbutton -label "Always draw shadows (slow!)" \
	-command { plv_drawstyle -shadows $styleShadows; \
		       $toglPane render } \
	-variable styleShadows
    createNamedCascade $menuView menuAAsamps "Anti-alias sample number"
    foreach x { 2 3 4 8 15 24 66 } {
	$menuView.menuAAsamps add radiobutton -label $x \
	    -variable styleAAsamps -value $x \
	    -command { plv_drawstyle -aasamps $styleAAsamps }
    }
    $menuView add separator
    $menuView add checkbutton -label "Double-buffer" -variable isDoubleBuffer \
	-command { $toglPane configure -double $isDoubleBuffer }
    $menuView add checkbutton -label "Orthographic" -variable isOrthographic \
	-command { if {$isOrthographic} {plv_ortho} else {plv_persp}; \
		       $toglPane render}
    $menuView add checkbutton -label "Hide render window" \
	-variable theHideRenderMode \
	-command {
	    if {$theHideRenderMode} {
		place forget $toglPane
	    } else {
		place $toglPane -x 0 -y 0 -relwidth 1.0 -relheight 1.0
	    }}
    $menuView add command -label "Kiosk mode" -command "kioskMode start"
    $menuView add separator

    $menuView add command -label "Context Manager..." \
	-command buildVisGroupsUI

#     set menuCamera [createNamedCascade $menuView menuCamera \
#     			"Camera positions"]
#     $menuCamera add command -label "Remember..." \
#     	-command "saveCameraPosition $menuCamera"
#     $menuCamera add separator
#     $menuCamera add command -label "(Reset)" -command plv_resetxform

    $menuView add command -label "Get camera info" -command showCameraInfo
    $menuView add command -label "Reset ALL transformations" \
	-command { plv_resetxform all }
    $menuView add command -label "Reset View transformations" \
	-command { plv_resetxform  }

    $menuView add command -label "Apply camera to mesh xform" \
	-command { plv_camera_xform_to_mesh; $toglPane render }
    $menuView add command -label "Load (tsai) camera viewpoint..." \
	-command {
	    set types { { {Tsai Files} {.tsai} } { {Camera Files} {.camera} } }
	    set tsaifile [tk_getOpenFile -defaultextension .tsai \
			      -filetypes $types]
	    if {$tsaifile != ""} {
		plv_resetxform
		plv_positioncamera $tsaifile ; redraw 1
	    }
	}
    $menuView add command -label "Resize render frame..." \
	-command windowResizeUI

    $menuView add separator
    $menuView add command -label "Preferences..." -command prefsDialog

    .menubar add cascade -label Render -underline 0 -menu .menubar.menuRender
    set menuRender [buildUI_RenderMenu .menubar.menuRender "Render options"]

    set menuMesh [createNamedCascade .menubar menuMesh Mesh "-underline 0"]
    $menuMesh add cascade -label Resolution -menu $menuMesh.resMenu
    menu $menuMesh.resMenu \
	-postcommand "buildCurrentMeshResList $menuMesh.resMenu"
    $menuMesh add separator
    $menuMesh add command -label "Flip normals" \
	-command { invertMeshNormals $theMesh }
    $menuMesh add separator
    $menuMesh add command -label "Get mesh info" \
	-command {showMeshInfo $theMesh}
    $menuMesh add command -label "Reset transformation" \
	-command { plv_resetxform $theMesh }


    set menuCommands [createNamedCascade .menubar menuCommands Commands \
			  "-underline 0"]
    $menuCommands add command -label "Correspondence registration" \
	-command correspRegistrationDialog
    $menuCommands add command -label "Drag registration walkthrough" \
	-command dragRegistrationDialog
    $menuCommands add command -label "Drag register current mesh" \
	-command {DragRegister $theMesh $dragRegMethod}
    $menuCommands add command -label "ICP registration" -accelerator "i" \
	-command ICPdialog
    $menuCommands add command -label "Global registration" -accelerator "^r" \
	-command globalRegistrationDialog
    $menuCommands add command -label "CyberScan self-alignment" \
	-command {plv_cyberscan_selfalign $theMesh; redraw 1}
    $menuCommands add separator

    set clipMenu [createNamedCascade $menuCommands clipMenu \
		      "Clip to selection"]
    buildUI_populateClipMenu $clipMenu

    $menuCommands add command -label "Fit plane to cliprect..." \
	-command planeFitClipRectUI
    $menuCommands add command -label "Analyze line for depth" \
	-command analyze_line_depth
    $menuCommands add command -label "Auto-Analyze lines..." \
	-command autoAnalyzeLineUI
    $menuCommands add command -label "Render thickness view" \
        -command "update; plv_render_thickness"
    $menuCommands add separator

    $menuCommands add command -label "Align image to mesh..." \
	-command imageAlignmentUI
    $menuCommands add command -label "Color image visualizer..." \
	-command colorVisUI
    $menuCommands add command -label "CyberScan Preview..." \
        -command cyberscanPreviewUI

    $menuCommands add separator
    set groupMenu [createNamedCascade $menuCommands groupMenu \
		       "Group/ungroup"]
    $groupMenu add command -label "Group visible meshes" \
	-command {group_createFromVis} -accelerator "^g"
    $groupMenu add command -label "Group visible meshes as..." \
	-command {group_createNamedFromVis} -accelerator "^G"
    $groupMenu add command -label "Ungroup active mesh" \
	-command {group_breakGroup $theMesh} -accelerator "^u"
    set xformMenu [createNamedCascade $menuCommands xformMenu \
		       "Transform mesh"]
    $xformMenu add command -label "Reload xform for current scan" \
	-command {scz_xform_scan $theMesh reload}
    $xformMenu add separator
    $xformMenu add command -label "Specify rotation/translation" \
	-command manualXFormUI
    $xformMenu add command -label "Specify matrix transform" \
	-command matrixXformUI
    set dumpXfMenu [createNamedCascade $menuCommands dumpXf \
			"Dump mesh transform"]
    $dumpXfMenu add command -label "As matrix" \
	-command "showCurrentScanXform matrix"
    $dumpXfMenu add command -label "As quaternion" \
	-command "showCurrentScanXform quat"
    $dumpXfMenu add command -label "As axial rotation" \
	-command "showCurrentScanXform axis"

    $menuCommands add separator
    $menuCommands add command -label "MM-VRIP Preparation" \
	-command {file_exportMMSForVripDialog}
    $menuCommands add command -label "CyberScan-VRIP Preparation..." \
	-command {file_exportSDForVripDialog}
    $menuCommands add command -label "Plyfile-VRIP Preparation..." \
	-command {file_exportPlyForVripDialog}
    $menuCommands add separator
    $menuCommands add command -label "Release all resolutions" \
	-command releaseAllMeshes
    $menuCommands add separator
    $menuCommands add command -label "Create cube" \
	-command { addMeshToWindow [plv_synthesize 1.0] }
    $menuCommands add command -label "Polygonize visible meshes" \
	-command { polygonizeDialog }
    $menuCommands add separator
    $menuCommands add command -label "Run external program" \
	-command { ExtProg }

    global menuWindow
    set menuWindow [createNamedCascade .menubar menuWindow Window \
			"-underline 0"]
    $menuWindow add checkbutton -label "Show toolbar" \
	-variable ui_showToolbar
    $menuWindow add checkbutton -label "Show status bar" \
	-variable ui_showStatusbar
    $menuWindow add separator
    bind $menuWindow <ButtonPress-3> "window_ActivateFromY %y"

    global menuHelp
    set menuHelp [createNamedCascade .menubar menuHelp Help \
		      "-underline 0"]
    $menuHelp add command -label "Show Help Window" \
	-command {help_showWindow}
    global help_onoff
    set help_onoff 1
    $menuHelp add checkbutton -label "Toggle Help On/Off" \
	-variable help_onoff -accelerator "^h"

    help_bindHelpToMenus
    help_bindHelpToButtons
    global env
    help_setHelpDirectory "$env(SCANALYZE_DIR)/help"
}


######################################################################
#
# create right-click menu for togl pane
#
######################################################################

proc buildUI_ToglContextMenu {} {
    global toglMenu
    global toglSubmenu

    set toglMenu [menu .toglMenu \
		      -tearoffcommand "setMenuTitle \"Quick options\""]
    # this will be populated later as tools are selected
    # but create submenus once, now, for later use

    set toglSubmenu(render) [buildUI_RenderMenu $toglMenu.renderMenu \
				 "Render options"]

    set toglSubmenu(reset) [menu $toglMenu.resetMenu \
				-tearoffcommand "setMenuTitle reset"]
    $toglSubmenu(reset) add command -label Viewer -command "plv_resetxform"
    $toglSubmenu(reset) add command -label "Current scan" -command \
	{if {$theMesh != ""} {plv_resetxform $theMesh}}
    $toglSubmenu(reset) add command -label "All scans" \
	-command "plv_resetxform allmesh"
    $toglSubmenu(reset) add command -label "All (scans and viewer)" \
	-command "plv_resetxform all"

    set toglSubmenu(gohome) [menu $toglMenu.goHomeMenu \
			    -tearoffcommand "setMenuTitle \"Go Home\""]
    $toglSubmenu(gohome) add command -label Viewer -command {goHome ""}
    $toglSubmenu(gohome) add command -label Mesh -command \
	{if {$theMesh != ""} {goHome $theMesh}}
    $toglSubmenu(gohome) add command -label All -command goHomeAll

    set toglSubmenu(CoR) [menu $toglMenu.corMenu \
			 -tearoffcommand "setMenuTitle \"Rotation center\""]
    $toglSubmenu(CoR) add checkbutton -label "Show" \
	-variable rotCenterVisible -command { $toglPane render }
    $toglSubmenu(CoR) add checkbutton -label "Always on top" \
	-variable rotCenterOnTop -command { $toglPane render }
    $toglSubmenu(CoR) add command -label "Flash" \
	-command flashCenterOfRotation

    set toglSubmenu(clip) [menu $toglMenu.clipMenu \
			       -tearoffcommand "setMenuTitle \"Clip\""]
    buildUI_populateClipMenu $toglSubmenu(clip)
}




############################################################################
#
# create/maintain mesh controls window
#
############################################################################


proc resizeMCscrollbar {} {
    if {[globalset noui]} return

    # batch multiple requests together
    idlecallback resizeMCscrollbar
}


proc _resizeMCscrollbar_idlecallback {} {
    global leftFrame
    global scrollFrame
    global meshFrame

    # want to re-sort everything
    #set slaves [lsort -dictionary [getMeshList]]
    set slaves [getSortedMeshList]

    set lastframe ""
    foreach slave $slaves {
	if {[array names meshFrame $slave]!=""} {
	    catch {
		set frame $meshFrame($slave)
		if {$lastframe == ""} {
		    pack $frame -side top
		} else {
		    pack $frame -side top -after $lastframe
		}
		set lastframe $frame
	    }
	}
    }

    # so that winfo works
    update idletasks

    set w [winfo reqwidth $leftFrame]
    set h [winfo reqheight $leftFrame]
    $scrollFrame config -scrollregion "0 0 $w $h" -width $w -height $h
}


proc buildUI_MeshControls {} {
    global meshControls
    global leftFrame
    global scrollFrame
    global meshSelScroll
    global meshVisible

    set meshControls [toplevel .meshControls]

    set allFrame [frame $meshControls.allFrame]

    button $allFrame.show      -text "ShowAll" -padx 0 -pady 0 \
	-command "showAllMeshes 1"
    button $allFrame.hideOther -text "HideOther" -padx 0 -pady 0 \
	-command { showOnlyMesh $theMesh }
    lappend enabledWhenMeshSelected $allFrame.hideOther
    button $allFrame.hide      -text "HideAll" -padx 0 -pady 0 \
	-command "showAllMeshes 0"
    button $allFrame.invert    -text "Inv" -padx 0 -pady 0 \
	-command "showAllMeshes invert"

    globalset invokeExtendedMeshControls \
	[menubutton $allFrame.menuB -text "..." \
	     -padx 3 -pady 1 -relief raised\
	     -menu $allFrame.menuB.hideShowMenu]
    set hideShowMenu [menu $allFrame.menuB.hideShowMenu \
		-tearoffcommand {setMenuTitle "Extended Mesh Controls"}]

    $hideShowMenu add command -label "Sort order..." \
	-command meshControlsSortDialog
    $hideShowMenu add separator

    $hideShowMenu add command -label "ShowAll" \
	-command {showAllMeshes 1} \
	-accelerator "^a"
    $hideShowMenu add command -label "HideOther" \
	-command {showOnlyMesh $theMesh}
    $hideShowMenu add command -label "HideAll" \
	-command {showAllMeshes 0}
    $hideShowMenu add command -label "Inv" \
	-command {showAllMeshes invert} \
	-accelerator "^i"
    $hideShowMenu add separator

    $hideShowMenu add command -label "Show only ICP meshes" \
	-command showOnlyIcpMeshes
    $hideShowMenu add separator

    $hideShowMenu add checkbutton -label "Render only active mesh" \
	-variable renderOnlyActiveMesh -accelerator "^1"
    $hideShowMenu add separator

    $hideShowMenu add command -label "Show only screen visible meshes" \
	-command {eval showOnlyMesh [plv_getVisiblyRenderedScans]} \
	-accelerator "^v"
    $hideShowMenu add command -label "Hide meshes outside" \
	-command {eval showOnlyMesh [plv_get_selected_meshes]}
    $hideShowMenu add command -label "Hide meshes inside" \
	-command {eval showOnlyMesh [plv_get_selected_meshes invert]}
    $hideShowMenu add command -label "Show only meshes inside" \
	-command {eval showOnlyMesh [plv_get_selected_meshes hidden]}
    $hideShowMenu add command -label "Show all meshes outside" \
	-command {eval showOnlyMesh [plv_get_selected_meshes hidden invert]}
    $hideShowMenu add separator

    $hideShowMenu add command -label "Group visible meshes" \
	-command {group_createFromVis} -accelerator "^g"
    $hideShowMenu add command -label "Group visible meshes as..." \
	-command {group_createNamedFromVis} -accelerator "^G"
    $hideShowMenu add command -label "Ungroup active mesh" \
	-command {group_breakGroup $theMesh} -accelerator "^u"
    $hideShowMenu add separator

    $hideShowMenu add command -label "Load visible meshes" \
	-command {eval loadMeshes 1 [getVisibleMeshes]}
    $hideShowMenu add command -label "Load meshes inside" \
	-command {eval loadMeshes 1 [plv_get_selected_meshes]}
    $hideShowMenu add command -label "Unload meshes inside" \
	-command {eval loadMeshes 0 [plv_get_selected_meshes]}
    $hideShowMenu add command -label "Unload meshes outside" \
	-command {eval loadMeshes 0 [plv_get_selected_meshes invert]}
    $hideShowMenu add separator

    $hideShowMenu add command -label "Delete Invisible Meshes" \
	-command deleteInvisibleMeshes
    $hideShowMenu add separator

    $hideShowMenu add command -label "Context Manager..." \
	-command "buildVisGroupsUI"
    bind $allFrame.menuB <Double-Button-1> {
	# double-click invokes the context manager:
	buildVisGroupsUI

	# need to unpost menu now; here are two possible ways:
	# 1) by hand.
	#$invokeExtendedMeshControls.hideShowMenu unpost
	#grab release [grab current]

	# 2) tkMenuUnpost is cleaner, since it releases the grab (with
	# error-checking) and cleans up various internal tk state.
	# but it cleans it up so well that the menu will see the 2nd
	# click of the double-click as a call to display again, so we
	# need to hack around that.
	tkMenuUnpost {}
	$invokeExtendedMeshControls config -state disabled
	after 100 "$invokeExtendedMeshControls config -state normal"
    }

    packchildren $allFrame -side left -fill x -expand true


#     set labelFrame [frame $meshControls.labelFrame]
#     label $labelFrame.meshLabel -text "Mesh"
#     label $labelFrame.visLabel -text "Vis."
#     label $labelFrame.activeLabel -text "Act."
#     pack $labelFrame.activeLabel $labelFrame.visLabel \
# 	-side right -anchor e
#     pack $labelFrame.meshLabel -side right -anchor e -fill x -expand 1

    set scrollerFrame [frame $meshControls.scrollFrame]
    set meshSelScroll [scrollbar $scrollerFrame.s \
			   -command "$scrollerFrame.leftFrame yview"]
    set scrollFrame [canvas $scrollerFrame.leftFrame -width 0 -height 0 \
			 -yscrollcommand "$meshSelScroll set"]
    set leftFrame [frame $scrollFrame.scrollee]
    $scrollFrame create window 0 0 -window $leftFrame -anchor nw
    pack $meshSelScroll -side right -fill y
    pack $scrollFrame -side left -fill both -expand 1

    packchildren $meshControls -side top -fill x -expand 1

    wm title $meshControls "Mesh controls"
    wm resizable $meshControls 0 1
    window_Register $meshControls undeletable
}




############################################################################
#
# add widgets to main window
#
############################################################################


proc buildUI_ShowToolbar {show} {
    if {![winfo exists .tools] || ![winfo exists .status]} { return }

    if {$show} {
	pack .tools -side left -fill y -anchor n -before .toglFrame
	pack forget .status.tools
    } else {
	pack forget .tools
	pack .status.tools -side left -before .status.info
    }

    # force redraw because togl was resized
    redraw 1
}


proc buildUI_ShowStatusbar {show} {
    if {![winfo exists .tools] || ![winfo exists .status]} { return }

    if {$show} {
	pack .status .statussep -side bottom -fill both
    } else {
	pack forget .status
    }

    # force redraw because togl was resized
    redraw 1
}


proc buildUI_MainWindow {} {

    global toglPane

    # main togl widget for rendering
    frame .toglFrame
    set toglPane .toglFrame.toglPane
    togl $toglPane \
	-width 10 -height 10 \
	-rgba true -double true -depth true -accum true -overlay true \
	-stencil true -ident $toglPane
    place $toglPane -x 0 -y 0 -relwidth 1.0 -relheight 1.0

    # tool palette
    frame .tools
    # move viewer, move selected mesh
    radiobutton .tools.moveMesh -text "moveMesh (m)" \
	-variable theMover -value mesh \
	-indicatoron false
    global enabledWhenMeshSelected
    lappend enabledWhenMeshSelected .tools.moveMesh
     radiobutton .tools.moveCamera -text "moveView (v)" \
	-variable theMover -value viewer \
	-indicatoron false
    radiobutton .tools.moveLight -text "moveLight (l)" \
	-variable theMover -value light \
	-indicatoron false
    # interactively select scans by selection rect
    radiobutton .tools.selectScan -text selectScan \
	-variable theMover -value selectScan \
	-indicatoron false
    # interactively select scans by selection rect
    radiobutton .tools.selectScreenScan -text selectVisScan \
	-variable theMover -value selectScreenScan \
	-indicatoron false
    # interactively choose one scan by pt click
    radiobutton .tools.pickScan -text "pickScan (p)"\
	-variable theMover -value pickScan \
	-indicatoron false
    # pick color
    radiobutton .tools.pickColor -text pickColor \
	-variable theMover -value pickColor \
	-indicatoron false
    # display coordinates under mouse
    radiobutton .tools.ptChooser -text "Xyz coord (x)" \
	-variable theMover -value ptChooser -indicatoron false
    # zoom tool
    radiobutton .tools.zoomer -text "Zoom (z)" \
	-variable theMover -value zoomer -indicatoron false

    # clipping tools
    frame .tools.clip
    global env
    radiobutton .tools.clip.line -bitmap @$env(SCANALYZE_DIR)/tool_line.xbm \
	-variable theMover -value clipLine \
	-indicatoron false
    radiobutton .tools.clip.rect -bitmap @$env(SCANALYZE_DIR)/tool_rect.xbm \
	-variable theMover -value clipRect \
	-indicatoron false
    radiobutton .tools.clip.shape -bitmap @$env(SCANALYZE_DIR)/tool_shape.xbm \
	-variable theMover -value clipShape \
	-indicatoron false
    pack .tools.clip.line .tools.clip.rect .tools.clip.shape \
 	-side left

    frame .tools.divide1 -height 3 -relief sunken -borderwidth 2

    # center of screen, center of obj, arbitrary
    radiobutton .tools.chooseCenter -text ChsCtr \
	-variable theMover -value rotCenter -indicatoron false
    button .tools.autoCenter -text AutoCtr \
	-command "set theCoR auto" -padx 0 -pady 0
    button .tools.screenCenter -text ScrnCtr \
	-command "set theCoR screen" -padx 0 -pady 0

    frame .tools.divide2 -height 3 -relief sunken -borderwidth 2

    # set and go home
    button .tools.setHome -text "SetHome (s)" -command setHome -padx 0 -pady 0
    button .tools.goHome -text "GoHome (g)" -command goHome -padx 0 -pady 0

    frame .tools.divide3 -height 3 -relief sunken -borderwidth 2

    # undo button
    button .tools.undo -text Undo -command plv_undo_xform -padx 0 -pady 0
    # redo button
    button .tools.redo -text Redo -command plv_redo_xform -padx 0 -pady 0

    # rotation constraints
    frame .tools.rc
    radiobutton .tools.rc.n -text F -variable rotationConstraint -value none \
	-indicator off
    radiobutton .tools.rc.x -text X -variable rotationConstraint -value x \
	-indicator off
    radiobutton .tools.rc.y -text Y -variable rotationConstraint -value y \
	-indicator off
    radiobutton .tools.rc.z -text Z -variable rotationConstraint -value z \
	-indicator off
    packchildren .tools.rc -side left -padx 0 -pady 0 -expand 1 -fill x

    # quick render options
    frame .tools.divide4 -height 3 -relief sunken -borderwidth 2
    frame .tools.ro
    verboseOptionMenu .tools.ro.fill "Fill" thePolygonMode \
	{fill Polygons} {line Wireframe} {hiddenline "Hidden line"} \
	{point Points} {hiddenpoint "Hidden Points"}
    verboseOptionMenu .tools.ro.color "Color" theColorMode \
	{gray "Gray color"} {false "False color"} {intensity Intensity}\
	{true "True color"} {boundary Boundary} {confidence Confidence}\
	{registration Registration} {texture Texture}
    button .tools.ro.redraw -text "Redraw (r)" -padx 0 -pady 0\
	-command "redraw safeflush"
    packchildren .tools.ro -side top -expand 1 -fill x -padx 0

    pack .tools.moveCamera .tools.moveMesh .tools.moveLight\
	.tools.selectScan .tools.selectScreenScan \
	.tools.pickScan .tools.pickColor .tools.ptChooser\
	.tools.zoomer \
	-side top -fill x
    pack .tools.clip \
	-side top -fill x
    pack .tools.divide1 -side top -fill x -pady 4
    pack .tools.chooseCenter .tools.autoCenter .tools.screenCenter \
	-side top -fill x
    pack .tools.divide2 -side top -fill x -pady 4
    pack .tools.setHome .tools.goHome \
	-side top -fill x
    pack .tools.divide3 -side top -fill x -pady 4
    pack .tools.undo .tools.redo .tools.rc -side top -fill x
    pack .tools.divide4 -side top -fill x -pady 4
    pack .tools.undo .tools.redo .tools.ro -side top -fill x

    fixButtonColors .tools.moveCamera .tools.moveMesh .tools.moveLight \
	.tools.ptChooser .tools.chooseCenter \
	.tools.pickScan .tools.pickColor .tools.selectScan
    fixButtonColors .tools.clip.line .tools.clip.rect .tools.clip.shape
    fixButtonColors .tools.rc.n .tools.rc.x .tools.rc.y .tools.rc.z

    button .tools.hide -text ">" -command "set ui_showToolbar 0"
    pack .tools.hide -side bottom -fill x

    # status bar
    frame .statussep -bg lightgray -height 2
    frame .status -relief sunken -borderwidth 2 -height 28
    frame .status.info
    label .status.info.mesh -textvariable meshCount
    label .status.info.l1 -text "scans  "
    label .status.info.tri -textvariable polyCount
    label .status.info.l2 -text "triangles (" -padx 0
    label .status.info.vis -textvariable visPolyCount -padx 0
    label .status.info.l3 -text " visible)"
    foreach widget {mesh l1 tri l2 vis l3} \
	{ pack .status.info.$widget -side left }
    label .status.info.mem -relief sunken
    autoSetText .status.info.mem getMemUsage
    label .status.info.tstrips -relief sunken -text tstrip -fg darkred
    label .status.info.displist -relief sunken -text dlist -fg darkred
    pack .status.info.displist .status.info.tstrips \
	.status.info.mem -side right
    bind .status.info.tstrips <Double-Button-1> {
	set styleTStrip [expr ! $styleTStrip]
    }
    bind .status.info.displist <Double-Button-1> {
	set styleDispList [expr ! $styleDispList]
    }
    pack .status.info -side left -fill x -expand true
    button .status.tools -text "<" -command "set ui_showToolbar 1"

    # arrange tool palette, display area and status bar in main window
    pack propagate .status 0
    pack .toglFrame -side top -fill both -expand 1 -anchor n
    globalset ui_showToolbar 1
    globalset ui_showStatusbar 1

    wm title   . scanalyze
    wm protocol . WM_DELETE_WINDOW confirmQuit

    # handle uncovering of window with cached redraw
    bind $toglPane <Visibility> "checkRedrawToglPane %s"

    # and now that toglPane exists, bind context menu
    bindToglPaneRclickToContextMenu
}


proc bindToglPaneRclickToContextMenu {} {
    global toglPane
    bind $toglPane <ButtonPress-3> {
	set toglContextClick "%x %y %s"
	tk_popup $toglMenu %X %Y
    }
    bind $toglPane <B3-Motion> ""
    bind $toglPane <ButtonRelease-3> ""
}


proc checkRedrawToglPane {state} {
    if {$state == "VisibilityUnobscured"} {
	#puts "forcing redraw due to vis change"

	# also inappropriate to redraw here because it happens after every
	# menu pull-down -- exactly what we're trying to avoid
	# What would be necessary here is to know what areas of the cache
	# are valid, and if an invalid area is exposed, then redraw.
	# It's not (currently) worth the trouble IMHO. -- magi

	#redraw 1
    }

    # not safe to redraw because we're obscured, and we'll pick up bad
    # stuff in the cache... is this the best we can do?
}


proc buildUI_PositionWindows {} {
    set scrX [winfo screenwidth .]
    set scrY [winfo screenheight .]

    set left   [expr int($scrX * .2)]
    set width  [expr int($scrX * .6)]
    set top    [expr int($scrY * .1)]
    set height [expr int($scrY * .7)]

    buildUI_SaveWindowPositions prepare

    set mainGeom [get_preference mainWindowGeometry \
		      ${width}x${height}+${left}+${top}]
    wm geometry . $mainGeom
    wm minsize . 450 400

    # allow window to grow bigger than screen, so you can
    # arrange a no-borders view
    wm maxsize . [expr int(1.6 * [winfo screenwidth .])] \
	[expr int(1.3 * [winfo screenheight .])]

    set meshGeom [get_preference meshWindowGeometry \
		      +[expr $left+$width+16]+${top}]
    wm geometry [globalset meshControls] $meshGeom
}


proc buildUI_SaveWindowPositions {mode} {
    if {$mode == "prepare"} {
	watchWindowPosition .
	watchWindowPosition [globalset meshControls]
    } elseif {$mode == "save"} {
	. config -menu ""
	update idletasks
	set_preference mainWindowGeometry [getWindowPosition .]
	. config -menu .menubar

	# for mesh controls, just part after the + sign
	set meshGeom [getWindowPosition [globalset meshControls]]
	set meshGeom [string range $meshGeom [string first + $meshGeom] end]
	set_preference meshWindowGeometry $meshGeom
    } else {
	puts "Bad parameter $mode to buildUI_SaveWindowPositions"
    }
}


proc watchWindowPosition {win} {
    bind $win <Configure> "+setWindowPosition $win %W %wx%h+%x+%y"
}


proc setWindowPosition {winTarget winMove pos} {
    if {$winTarget != $winMove} { return }

    global watch_win_pos
    set watch_win_pos($winMove) $pos

    #puts "Window $winMove moved to $pos"
}


proc getWindowPosition {win} {
    global watch_win_pos

    if {[info exists watch_win_pos($win)]} {
	return $watch_win_pos($win)
    } else {
	puts "Warning: getWindowPosition used on un-watched window"
	return [wm geometry $win]
    }
}



proc buildUI_BuildNudgeWindow {} {
    toplevel .nudge
    wm title .nudge "Nudge Controls"
    window_Register .nudge undeletable
    setWindowVisible .nudge 0

    label .nudge.help -text "Arrow keys move 1mm; shift=.1; control=10"
    pack .nudge.help -side top

    frame .nudge.up
    button .nudge.up.up1 -text .1 -command "translateinplane 0 .1"
    button .nudge.up.up10 -text 1 -command "translateinplane 0 1"
    button .nudge.up.up100 -text 10 -command "translateinplane 0 10"
    pack .nudge.up.up1 .nudge.up.up10 .nudge.up.up100 -side top

    frame .nudge.down
    button .nudge.down.down1 -text .1 -command "translateinplane 0 -.1"
    button .nudge.down.down10 -text 1 -command "translateinplane 0 -1"
    button .nudge.down.down100 -text 10 -command "translateinplane 0 -10"
    pack .nudge.down.down1 .nudge.down.down10 .nudge.down.down100 -side bottom

    frame .nudge.lr
    frame .nudge.lr.left
    button .nudge.lr.left.left1 -text .1 -command "translateinplane -.1 0"
    button .nudge.lr.left.left10 -text 1 -command "translateinplane -1 0"
    button .nudge.lr.left.left100 -text 10 -command "translateinplane -10 0"
    pack .nudge.lr.left.left1 .nudge.lr.left.left10 \
	.nudge.lr.left.left100 -side left

    frame .nudge.lr.right
    button .nudge.lr.right.right1 -text .1 -command "translateinplane .1 0"
    button .nudge.lr.right.right10 -text 1 -command "translateinplane 1 0"
    button .nudge.lr.right.right100 -text 10 -command "translateinplane 10 0"
    pack .nudge.lr.right.right1 .nudge.lr.right.right10 \
	.nudge.lr.right.right100 -side right

    pack .nudge.lr.left .nudge.lr.right -side left -fill x -expand 1
    pack .nudge.up .nudge.lr .nudge.down -side top -fill x -expand 1

    frame .nudge.units -borderwidth 2 -relief groove
    label .nudge.units.u -text "Mesh units:"
    radiobutton .nudge.units.m -text meters \
	-variable theUnitScale -value 1000
    radiobutton .nudge.units.mm -text millimeters \
	-variable theUnitScale -value 1
    pack .nudge.units.u .nudge.units.m .nudge.units.mm -side left
    pack .nudge.units -side top -fill x -expand 1 -pady 4 -padx 4

    frame .nudge.num -borderwidth 2 -relief groove
    frame .nudge.num.r1
    label .nudge.num.r1.l -text "Numeric translation (in mm):"
    frame .nudge.num.r2
    label .nudge.num.r2.lx -text X:
    entry .nudge.num.r2.x -textvariable nudgeX -width 5
    globalset nudgeX 0
    label .nudge.num.r2.ly -text Y:
    entry .nudge.num.r2.y -textvariable nudgeY -width 5
    globalset nudgeY 0
    pack .nudge.num.r1.l -side left
    set r2 .nudge.num.r2
    pack $r2.lx $r2.x $r2.ly $r2.y -side left
    button .nudge.num.go -text Nudge -command numericNudgeNow
    bind $r2.x <Return> numericNudgeNow
    bind $r2.y <Return> numericNudgeNow
    pack .nudge.num.go -side right -fill y -expand 1 -anchor e -pady 4 -padx 4
    pack .nudge.num.r1 .nudge.num.r2 -side top -fill x -anchor w -pady 2
    pack .nudge.num -side top -fill x -expand 1 -padx 4 -pady 4

    wm resizable .nudge 0 0
}


proc numericNudgeNow {} {
    translateinplane [globalset nudgeX] [globalset nudgeY]
}


proc buildUI_BuildVersionWindow {} {
    global env
    if [catch {set file [open "$env(SCANALYZE_DIR)/source-history" r]}] return
    set history [read $file]
    close $file

    set vi [toplevel .versioninfo]
    wm title $vi "Version information"
    window_Register $vi undeletable
    setWindowVisible $vi 0

    text $vi.t -wrap none -yscrollcommand "$vi.sy set"
    $vi.t insert end $history
    scrollbar $vi.sy -orient vert -command "$vi.t yview"

    pack $vi.sy -side right -fill y
    pack $vi.t -side top -fill both -expand true
}


proc buildUI_selectNthResolution {} {
    if {[window_Activate .resNselect]} return

    set rs [toplevel .resNselect]
    wm title $rs "Global resolution selector"
    window_Register $rs

    frame $rs.a -relief groove -border 2
    label $rs.a.l1 -text "Choose relative resolution for all scans"
    label $rs.a.l2 -text "(0 is highest res)"
    set f [frame $rs.a.f]
    entry $f.val -width 2
    scale $f.slide -orient horiz -from 0 -to 6 -showvalue 0 \
	-command {.resNselect.a.f.val delete 0 end;
	    .resNselect.a.f.val insert end}
    packchildren $f -side left
    packchildren $rs.a -side top -anchor w

    set p [frame $rs.preset]
    for {set i 0} {$i < 7} {incr i} {
	button $p.p$i -text $i -command "selectNthResolution $i"
    }
    packchildren $p -side left -padx 3

    frame $rs.b
    button $rs.b.ok -text "Select" \
	-command {selectNthResolution [.resNselect.a.val get]}
    button $rs.b.cl -text "Close" -command "destroy $rs"
    packchildren $rs.b -side left -fill x -expand 1

    packchildren $rs -side top -fill x -expand 1
}


######################################################################
#
# bind keyboard shortcuts to toolbar/menu commands
#
######################################################################


proc buildUI_BindShortcuts {} {
    # TODO: these switch theMover permanently; it's also useful to have keys
    # that switch the mode only while the key is down.  Maybe have lowercase
    # (unshifted) switch mode while key is down, or uppercase (shifted) keys
    # switch mode permanently?

    # bind Space to temporary theMover=viewer?
    bind . <KeyPress-v> "set theMover viewer"
    bind . <KeyPress-m> "set theMover mesh"
    bind . <KeyPress-l> "set theMover light"
    bind . <KeyPress-p> "set theMover pickScan"
    bind . <KeyPress-r> "set theMover clipRect"
    bind . <KeyPress-x> "set theMover ptChooser"
    bind . <KeyPress-z> "set theMover zoomer"
    bind . <KeyPress-s> "setHome"
    bind . <KeyPress-S> "setHomeAll"
    bind . <KeyPress-g> "goHome"
    bind . <KeyPress-G> "goHomeAll"
    bind . <KeyPress-r> "redraw 1"
    bind . <KeyPress-i> "ICPdialog"

    # X, Y, Z (must be uppercase) set rotation constraints
    foreach char {F X Y Z} {
	bind . <KeyPress-$char> "toggleRotationConstraint $char"
    }

    # left shift keeps us in fast-draw mode
    bind . <KeyPress-f> "plv_setManipRenderMode lock"
    bind . <KeyRelease-f> "plv_setManipRenderMode unlock; redraw"

    # some menu key shortcut
    bind .meshControls  <KeyPress-i> "ICPdialog"
    bind . <Control-KeyPress-r> "globalRegistrationDialog"
    bind .meshControls <Control-KeyPress-r> "globalRegistrationDialog"
    bind all <Control-KeyPress-h> "help_toggleOnOff"

    # resolution increase/decrease
    # basic idea is comma (<) down, period (>) up
    # shift applies to all meshes
    # control means go all the way
    # so you get the idea for ctrl+shift
    bind . <KeyPress-less>    {set drawResolution nextlow}
    bind . <KeyPress-greater> {set drawResolution nexthigh}
    bind . <KeyPress-comma>   {setMeshResolution $theMesh lower}
    bind . <KeyPress-period>  {setMeshResolution $theMesh higher}
    bind . <Control-KeyPress-comma>     {setMeshResolution $theMesh lowest}
    bind . <Control-KeyPress-period>    {setMeshResolution $theMesh highest}
    bind . <Control-KeyPress-less>      {set drawResolution low}
    bind . <Control-KeyPress-greater>   {set drawResolution high}

    # Mesh Visibility
    bind . <Control-KeyPress-a> {showAllMeshes 1}
    bind . <Control-KeyPress-o> {showOnlyMesh $theMesh}
    bind . <Control-KeyPress-v> {eval showOnlyMesh \
				     [plv_getVisiblyRenderedScans]}
    bind . <Control-KeyPress-i> {showAllMeshes invert}
    bind .meshControls <Control-KeyPress-a> {showAllMeshes 1}
    bind .meshControls <Control-Shift-KeyPress-a> {showAllMeshes 1}
    bind .meshControls <Control-KeyPress-v> {eval showOnlyMesh \
						 [plv_getVisiblyRenderedScans]}
    bind .meshControls <Control-KeyPress-i> {showAllMeshes invert}
    bind . <Control-KeyPress-k> {set renderOnlyActiveMesh \
				     [expr !$renderOnlyActiveMesh]}
    bind . <KeyPress-t> {changeVis $theMesh [expr ! $meshVisible($theMesh)]}
    bind .meshControls <KeyPress-t> \
	{set meshVisible($theMesh) [expr ! $meshVisible($theMesh)]}


    # Grouping
    bind . <Control-KeyPress-g> {group_createFromVis}
    bind .meshControls <Control-KeyPress-g> {group_createFromVis}
    bind . <Control-KeyPress-G> {group_createNamedFromVis}
    bind .meshControls <Control-KeyPress-G> {group_createNamedFromVis}
    bind . <Control-KeyPress-u> {group_breakGroup $theMesh}
    bind .meshControls <Control-KeyPress-u> {group_breakGroup $theMesh}

    # and number keys, to switch directly to given res
    foreach pair {{1 exclam} {2 at} {3 numbersign} {4 dollar} {5 percent}
	{6 asciicircum} {7 ampersand} {8 asterisk} {9 parenleft}
	{0 parenright}} {
	set num [lindex $pair 0]
	set sym [lindex $pair 1]
	bind . <KeyPress-$num> "selectNthResolutionForCurrentMesh $num"
	bind . <KeyPress-$sym> "selectNthResolution $num"
    }

    # nudge controls
    bind . <KeyPress-Left>  {translateinplane -1  0}
    bind . <KeyPress-Right> {translateinplane  1  0}
    bind . <KeyPress-Up>    {translateinplane  0  1}
    bind . <KeyPress-Down>  {translateinplane  0 -1}
    bind . <Shift-KeyPress-Left>  {translateinplane -.1  0}
    bind . <Shift-KeyPress-Right> {translateinplane  .1  0}
    bind . <Shift-KeyPress-Up>    {translateinplane  0  .1}
    bind . <Shift-KeyPress-Down>  {translateinplane  0 -.1}
    bind . <Control-KeyPress-Left>  {translateinplane -10  0}
    bind . <Control-KeyPress-Right> {translateinplane  10  0}
    bind . <Control-KeyPress-Up>    {translateinplane  0  10}
    bind . <Control-KeyPress-Down>  {translateinplane  0 -10}

    # rotational nudges
    # home/end = y, delete/pagedn = x, insert/pageup = z
    bind . <KeyPress-Delete> {plv_manrotate  0 -1  0; redraw 1}
    bind . <KeyPress-Next>   {plv_manrotate  0  1  0; redraw 1}
    bind . <KeyPress-Home>   {plv_manrotate -1  0  0; redraw 1}
    bind . <KeyPress-End>    {plv_manrotate  1  0  0; redraw 1}
    bind . <KeyPress-Insert> {plv_manrotate  0  0  1; redraw 1}
    bind . <KeyPress-Prior>  {plv_manrotate  0  0 -1; redraw 1}
    bind . <Control-KeyPress-Delete> {plv_manrotate   0 -10  0; redraw 1}
    bind . <Control-KeyPress-Next>   {plv_manrotate   0  10  0; redraw 1}
    bind . <Control-KeyPress-Home>   {plv_manrotate -10   0  0; redraw 1}
    bind . <Control-KeyPress-End>    {plv_manrotate  10   0  0; redraw 1}
    bind . <Control-KeyPress-Insert> {plv_manrotate   0  0  10; redraw 1}
    bind . <Control-KeyPress-Prior>  {plv_manrotate   0  0 -10; redraw 1}

    # select mesh
    bind . <KeyPress-bracketleft>    {selectMesh prev}
    bind . <KeyPress-bracketright>   {selectMesh next}
    bind .meshControls <KeyPress-bracketleft>    {selectMesh prev}
    bind .meshControls <KeyPress-bracketright>   {selectMesh next}
    bind . <KeyPress-braceleft>    {selectMeshFromAll prev}
    bind . <KeyPress-braceright>   {selectMeshFromAll next}
    bind .meshControls <KeyPress-braceleft>    {selectMeshFromAll prev}
    bind .meshControls <KeyPress-braceright>   {selectMeshFromAll next}

    # ICP dialog
    bind . <F1> {
	set regICPFrom $theMesh
	ICPdialog
    }
    bind . <F2> {
	set regICPTo $theMesh
	ICPdialog
    }

    # save registrations
    bind . <Control-KeyPress-x> {fileWriteAllScanXforms}


    # and some X event bindings that we need to process
    # minimize/maximize
    bind . <Unmap> {if {"%W"=="."} { window_Minimize save }}
    bind . <Map>   {if {"%W"=="."} { window_Minimize restore }}
    # resize
    bind . <Configure> {+if {"%W"=="."} { onMainResize }}
}

proc onMainResize {} {
    after idle {
	update
	if {[info exists onMainResizeProc]} {
	    eval $onMainResizeProc
	}
	redraw
    }
}

proc buildUI_privChangeMeshActive {mesh active {frame ""}} {
    global meshFrame
    global meshVisible

    if {$active} {
	set meshFrame($mesh) $frame
	set meshVisible($mesh) 1
    } else {
	unset meshFrame($mesh)
	unset meshVisible($mesh)
    }

}

proc buildUI_dummyPrivChangeMeshActive {mesh active {frame ""}} {
    global meshFrame
    global meshVisible

    if {$active} {
	set meshFrame($mesh) $frame
	set meshVisible($mesh) 1
    } else {
	destroy $meshFrame($mesh)
	unset meshFrame($mesh)
	unset meshVisible($mesh)
    }

}

proc hiliteScanEntry {mesh hilite} {
    global meshFrame
    set lab $meshFrame($mesh).title.label

    if {$hilite} {
	$lab config -bg "#00ffff"
    } else {
	resetBGcolor $lab
    }
}


proc redrawStatus {mode} {
    # early out for speed freaks
    if {![globalset wantRedrawStatus]} return

    set ud 0
    if {$mode == "start"} {
	set color Red
    } elseif {$mode == "cache"} {
	set color Purple
    } elseif {$mode == "end"} {
	set color Black
    } else {
	# medium out for medium speed freaks
	if {[globalset wantRedrawStatus] < 2} return
	set color $mode
    }

    .tools.ro.redraw config -fg $color
    #update idletasks
}

proc windowResizeUI {} {
    toplevel .resizeD
    wm title .resizeD "Resize render window..."
    wm resizable .resizeD 0 0

    label .resizeD.width -text "Width:"
    entry .resizeD.widthval -relief sunken -textvariable widthval
    label .resizeD.height -text "Height:"
    entry .resizeD.heightval -relief sunken -textvariable heightval

    button .resizeD.okb -text "OK" -command {
	if {    ( [scan $widthval "%d" foo] == 1 )
	     && ( [scan $heightval "%d" foo] == 1) } {
	    setMainWindowSize $widthval $heightval
	    destroy .resizeD
	}
    }

    button .resizeD.apply -text "Apply" -command {
	if {    ( [scan $widthval "%d" foo] == 1 )
	     && ( [scan $heightval "%d" foo] == 1) } {
	    setMainWindowSize $widthval $heightval
	}
    }

    button .resizeD.cancel -text "Cancel" -command "destroy .resizeD"

    pack .resizeD.width .resizeD.widthval \
	.resizeD.height .resizeD.heightval \
	.resizeD.okb .resizeD.apply .resizeD.cancel -side left
}

proc meshControlsSortDialog {} {
    set sd .meshControlsSortDialog
    if {[window_Activate $sd]} {return}
    toplevel $sd
    wm title $sd "Sort mesh list by:"
    window_Register $sd

    for {set n 1} {$n <= 5} {incr n} {
	set f [frame $sd.line$n]
	label $f.l -text "Criteria $n:" -anchor w
	set m [tk_optionMenu $f.sort meshControlsSort($n) \
		   "Name" "File date" "Loadedness" "Visibility" \
		   "PolygonCount"]
	packchildren $f -side left -fill x -expand 1
    }

    checkbutton $sd.dictionary -text "Use dictionary sort" \
	-variable meshControlsSort(mode) -onvalue dictionary -offvalue ascii

    button $sd.resort -text "Resort now" -command resizeMCscrollbar
    button $sd.close -text "Close" -command "destroy $sd"

    packchildren $sd -side top -fill x -expand 1
}


proc bindToglPaneRclickToMoveLight {} {
    global cursor
    global toglPane

    bind $toglPane <ButtonPress-3> \
	"plv_rotlight $toglPane %x %y;\
         $toglPane config -cursor \"circle$cursor(Fore)\""

    bind $toglPane <B3-Motion> \
	"plv_rotlight $toglPane %x %y"

    bind $toglPane <ButtonRelease-3> \
	"setTBCursor $toglPane 0 none"
}


proc kioskInstructions {} {
    set k [toplevel .kiosk]
    wm title $k "Instruzioni / Instructions"

    set width [expr [winfo screenwidth $k] / 5]

    set instr [frame $k.instructions]
    set i [frame $instr.italian]
    frame $instr.line -width 2 -borderwidth 2 -relief solid
    set e [frame $instr.english]
    frame $k.line -height 2 -borderwidth 2 -relief solid
    button $k.ok -text "Cominciare        OK               Begin" \
	-command "destroy $k"

    label $i.prova -text "Provatelo vi stessi!"
    label $i.rotare -text "Premere il pulsante sinistra (e trascinare il\
        mouse) per rotare la statua"
    label $i.muovere -text "Premere il pulsante a meta' (e trascinare il\
        mouse) per muovere la statua"
    label $i.zoomare -text "Premere simultaneamente i pulsanti sinistra e\
        a meta' (e trascinare il mouse) per zoomare"
    label $i.luce -text "Premere il pulsante destra (e trascinare il\
        mouse verticalmente) per muovere la luce"
    label $i.comincia -text "Cliccare su \"OK\" per cominciare"

    label $e.try -text "Try this yourself!"
    label $e.rotate -text "Push the left button (and drag the mouse) to\
        rotate the statue"
    label $e.move -text "Push the middle button (and drag the mouse) to\
        move the statue"
    label $e.zoom -text "Push the left and middle buttons at once (and\
        move the mouse vertically) to zoom"
    label $e.light -text "Push the right button (and move the mouse) to\
        move the light"
    label $e.begin -text "Click \"OK\" to begin"

    foreach child [concat [winfo children $i] [winfo children $e]] {
	$child config -wraplength $width -justify left -anchor w \
	    -font -*-times-medium-r-normal--20-*-*-*-p-96-iso8859-1
    }

    foreach child "$i.prova $e.try" {
	$child config -font -*-times-bold-r-normal--34-*-*-*-p-177-iso8859-1
    }

    packchildren $i -side top -fill x -pady 3
    packchildren $e -side top -fill x -pady 3

    packchildren $instr -side left -fill both -expand 1 -padx 10
    pack $k.instructions $k.line -side top -pady 5 -fill x
    pack $k.ok -side top -pady 5

    centerWindow $k
    grab set $k
    tkwait window $k
}


proc kioskMode {mode} {
    redraw block
    global preKioskGeometry

    if {$mode == "start"} {
	# go kiosk mode!
	# make sure model is onscreen; if this fails, die early
	if {[catch {plv_force_keep_onscreen 1 20 \
			"kioskInstructions"} msg]} {
	    bgerror $msg
	    redraw flush
	    return
	}

	# strip window of everything but rendering pane
	. config -menu ""
	buildUI_ShowToolbar 0
	buildUI_ShowStatusbar 0

	# set right button to move light
	bindToglPaneRclickToMoveLight
	globalset theMover viewer

	# fill the screen
	set preKioskGeometry [getWindowPosition .]
	set w [expr [winfo screenwidth .] + 80]
	set h [expr [winfo screenheight .] + 80]
	wm geometry . ${w}x${h}+-40+-40

	# and our exit key
	bind . <Escape> "kioskMode end"

    } else {
	# restore previous state

	# redo main window layout
	. config -menu .menubar
	buildUI_ShowToolbar [globalset ui_showToolbar]
	buildUI_ShowStatusbar [globalset ui_showStatusbar]

	# restore window position
	wm geometry . $preKioskGeometry
	unset preKioskGeometry

	# right button goes back to context menu
	bindToglPaneRclickToContextMenu
	plv_force_keep_onscreen 0

	# cleanup
	bind . <Escape> ""
    }

    redraw flush
}


proc toggleRotationConstraint {char} {
    global rotationConstraint

    set constraint [string tolower $char]
    if {$rotationConstraint == $constraint} {
	set rotationConstraint none
    } elseif {$constraint == "f"} {
	set rotationConstraint none
    } else {
	set rotationConstraint $constraint
    }
}
