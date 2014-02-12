######################################################################
#
# interactors.tcl
# created 10/29/98  magi
#
# manage responses to user actions without explicit -command bindings
#
######################################################################
#
# exports:
#   traceUIVariables
#   bindMouse
#
######################################################################



######################################################################
#
# setup variable traces
#
######################################################################

proc traceUIVariables {} {
    if {[globalset noui]} return

    globaltrace theMesh              w changeCurrentMesh
    globaltrace theMover             w changeTheMover
    globaltrace theColorMode         w changeColorMode
    globaltrace theBackfaceMode      w changeBackfaceMode
    globaltrace thePolygonMode       w changePolygonMode
    globaltrace drawResolution       w changeOverallResolution
    globaltrace transparentSelection w changeTransparentSelection
    globaltrace styleTStrip          w changeTStrip
    globaltrace styleDispList        w changeDispList
    globaltrace styleCacheRender     w changeCacheRender
    globaltrace slowPolyCount        w changeSlowPolyCount
    globaltrace rotationConstraint   w changeRotationConstraint
    globaltrace manipRender          w changeManipRenderMode
    globaltrace ui_showToolbar       w changeUIShowElement
    globaltrace ui_showStatusbar     w changeUIShowElement
    globaltrace stylePointSize       w changePointSize
    globaltrace meshControlsSort     w changeMeshControlsSort
    globaltrace renderOnlyActiveMesh w changeRenderOnlyActiveMesh
    globaltrace theCoR               w changeCenterOfRotation
}



######################################################################
#
# Mouse bindings: main entry point is bindMouse
# bindMouse calls unbindMouse, then, based on value of theMover,
# calls bindMouseToXXX to set up the appropriate bindings.
#
######################################################################


set prevMover ""
set oldMover "mesh"
set mouse_state_1 0
set mouse_state_2 0
set mouse_state_x 0
set mouse_state_y 0

proc bindMouse {widget} {
    unbindMouse $widget

    if {[globalset mouse_state_1]} {
	event generate $widget <ButtonRelease-1> \
	    -x [globalset mouse_state_x] -y [globalset mouse_state_y]
	puts "faking b1-up"
    }
    if {[globalset mouse_state_2]} {
	event generate $widget <ButtonRelease-2> \
	    -x [globalset mouse_state_x] -y [globalset mouse_state_y]
	puts "faking b2-up"
    }

    global theMover
    # theMover values: mesh, viewer, light, clipRect, rotCenter, ptChooser
    switch -exact -- $theMover {
	mesh        { bindMouseToTrackball $widget mesh }
	viewer      { bindMouseToTrackball $widget viewer }
	light       { bindMouseToMoveLight $widget }
	clipLine    { bindMouseToSelectLine  $widget }
	clipRect    { bindMouseToSelectRect  $widget }
	clipShape   { bindMouseToSelectShape $widget }
	selectScan  { bindMouseToSelectScans $widget }
	selectScreenScan { bindMouseToScreenSelectScans $widget }
	rotCenter   { bindMouseToChooseCenter $widget }
	ptChooser   { bindMouseToDisplayPoint $widget }
	pickScan    { bindMouseToPickScan  $widget }
	pickColor   { bindMouseToPickColor $widget }
	zoomer      { bindMouseToZoomer $widget }
    }

    globalset prevMover [globalset oldMover]
    globalset oldMover $theMover

    bind . <KeyPress-space>   {
	if {$theMover == "viewer"} {
	    if {$prevMover != "" && $prevMover != "viewer"} {
		set theMover $prevMover
	    } else {
		set theMover mesh
	    }
        } else {set theMover viewer}
    }

    if {[globalset mouse_state_1]} {
	event generate $widget <ButtonPress-1> \
	    -x [globalset mouse_state_x] -y [globalset mouse_state_y]
	puts "faking b1-dn"
    }
    if {[globalset mouse_state_2]} {
	event generate $widget <ButtonPress-2> \
	    -x [globalset mouse_state_x] -y [globalset mouse_state_y]
	puts "faking b2-dn"
    }

    # track mouse state since we can't query it
    prebind $widget <ButtonPress-1>   {set mouse_state_1 1}
    prebind $widget <ButtonRelease-1> {set mouse_state_1 0}
    prebind $widget <ButtonPress-2>   {set mouse_state_2 1}
    prebind $widget <ButtonRelease-2> {set mouse_state_2 0}
    prebind $widget <B1-Motion>       {set mouse_state_x %x;
	                               set mouse_state_y %y}
    prebind $widget <B2-Motion>       {set mouse_state_x %x;
	                               set mouse_state_y %y}
    prebind $widget    <Motion>       {set mouse_state_x %x;
	                               set mouse_state_y %y}
}


proc unbindMouse {widget} {
    bind $widget <ButtonPress-1> {}
    bind $widget <B1-Motion> {}
    bind $widget <Shift-B1-Motion> {}
    bind $widget <ButtonRelease-1> {}

    bind $widget <ButtonPress-2> {}
    bind $widget <B2-Motion> {}
    bind $widget <ButtonRelease-2> {}

    bind $widget <Motion> {}

    global unbindUnwindProc
    if {[info exists unbindUnwindProc]} {
	eval $unbindUnwindProc
    }

    resetToglMenu
}


proc resetToglMenu {} {
    global toglMenu
    global toglSubmenu

    $toglMenu delete 0 end

    $toglMenu add cascade -label "Render options" -menu $toglSubmenu(render)
    $toglMenu add separator
    $toglMenu add cascade -label "Reset transform" -menu $toglSubmenu(reset)
    $toglMenu add cascade -label "Go home" -menu $toglSubmenu(gohome)
    $toglMenu add separator
    $toglMenu add cascade -label "Rotation center" -menu $toglSubmenu(CoR)
    $toglMenu add separator
    $toglMenu add command -label Exit -command confirmQuit
}



proc bindMouseToTrackball {widget mode} {

    bind $widget <ButtonPress-1> \
	"setTBCursor $widget %s p1;\
         resetRotationCenter;\
         plv_rotxyviewmouse $widget %x %y %t start"
    bind $widget <B1-Motion> \
	"plv_rotxyviewmouse $widget %x %y %t"
    bind $widget <ButtonRelease-1> \
	"setTBCursor $widget %s r1;\
         plv_rotxyviewmouse $widget %x %y %t stop"

    bind $widget <ButtonPress-2> \
	"setTBCursor $widget %s p2;\
         plv_transxyviewmouse $widget %x %y %t start"
    bind $widget <B2-Motion> \
	"plv_transxyviewmouse $widget %x %y %t"
    bind $widget <ButtonRelease-2> \
	"setTBCursor $widget %s r2;\
         resetRotationCenter;\
         plv_transxyviewmouse $widget %x %y %t stop"

    # these bindings allow Ctrl and Alt to act as the shortcuts to
    # lighting and clipping controls like they used to, without
    # having to permanently bind Alt-B1-Motion and so on.
    bind $widget <Alt-Motion> \
	"bindMouseToTemporary $widget bindMouseToSelectRect"
    bind $widget <Control-Motion> \
	"bindMouseToTemporary $widget bindMouseToMoveLight"

    setTBCursor $widget 0 none

    globalset unbindUnwindProc "plv_rotxyviewmouse $widget stopspin"

    if {$mode == "mesh"} {
	# screenCenter rotation is illegal
	.tools.screenCenter config -state disabled
	globalset unbindUnwindProc "[globalset unbindUnwindProc]; \
                     .tools.screenCenter config -state normal"
    }

    # zoom in/out on context menu
    global toglMenu
    $toglMenu insert 1 command -label "Zoom in" \
	-command {eval zoomInClick $toglContextClick}
    $toglMenu insert 2 command -label "Zoom out" \
	-command {eval zoomOutClick $toglContextClick}
    $toglMenu insert 3 separator
}


proc bindMouseToMoveLight {widget} {
    bind $widget <ButtonPress-1> \
	"plv_rotlight $widget %x %y"
    bind $widget <B1-Motion> \
	"plv_rotlight $widget %x %y"

    global cursor
    $widget config -cursor "circle$cursor(Fore)"
}


proc bindMouseToTemporary {widget binding} {
    unbindMouse $widget
    $binding $widget
    bind $widget <Motion> "bindMouse $widget"
}

proc bindMouseToSelectRect {widget} {
    global theMesh

    bind $widget <ButtonPress-1> \
	"plv_drawboxselection $widget %x %y start"
    bind $widget <B1-Motion> \
	"set clipEnabled 1; \
	 plv_drawboxselection $widget %x %y move"
    bind $widget <ButtonRelease-1> \
	"plv_drawboxselection $widget %x %y stop"

    bind $widget <Motion> "setSelectionCursor $widget %x %y"

    global toglMenu toglSubmenu
    $toglMenu insert 1 command -label "Zoom to rect" \
	-command plv_zoom_to_rect
    if {[globalset oldMover] == "mesh"} {
	$toglMenu insert 2 command -label "Align mesh to plane" \
	    -command "alignToFitPlane mesh"
    } else {
	$toglMenu insert 2 command -label "Align viewer to plane" \
	    -command "alignToFitPlane camera"
    }
    $toglMenu insert 3 command -label "Clear selection" \
	-command clearSelection
    $toglMenu insert 4 cascade -label "Clip" -menu $toglSubmenu(clip)
    $toglMenu insert 5 separator
    # added menu option for printing voxels - for voxel display feature
    $toglMenu insert 6 command -label "Print voxel info - for ply file only" \
	-command "plv_print_voxels $theMesh"
    $toglMenu insert 7 separator
}


proc bindMouseToSelectLine {widget} {
    bind $widget <ButtonPress-1> \
	"plv_drawlineselection %x %y start"
    bind $widget <B1-Motion> \
	"set clipEnabled 1; plv_drawlineselection %x %y move"
    bind $widget <Shift-B1-Motion> \
	"set clipEnabled 1; plv_drawlineselection %x %y move constrain"
    bind $widget <ButtonRelease-1> \
	"plv_drawlineselection %x %y stop"

    bind $widget <Motion> "setSelectionCursor $widget %x %y"

    global toglMenu
    $toglMenu insert 1 command -label "Analyze line..." \
	-command analyze_line_depth
    $toglMenu insert 2 command -label "Clear selection" \
	-command clearSelection
    $toglMenu insert 3 separator
}


proc bindMouseToSelectShape {widget} {
    bind $widget <ButtonPress-1> \
	"plv_drawshapeselection %x %y start"
    bind $widget <B1-Motion> \
	"plv_drawshapeselection %x %y move"
    bind $widget <ButtonRelease-1> \
	"plv_drawshapeselection %x %y stop"

    bind $widget <ButtonPress-2> \
	"plv_drawshapeselection %x %y modify"
    bind $widget <B2-Motion> \
	"plv_drawshapeselection %x %y move"
    bind $widget <ButtonRelease-2> \
	"plv_drawshapeselection %x %y remove"

    bind $widget <Motion> "setSelectionCursor $widget %x %y"
    bind $widget <ButtonPress-2> "+setSelectionCursor $widget %x %y"
    bind $widget <ButtonRelease-2> "+setSelectionCursor $widget %x %y"

    global toglMenu toglSubmenu
    $toglMenu insert 1 cascade -label "Clip" -menu $toglSubmenu(clip)
    $toglMenu insert 2 command -label "Clear selection" \
	-command clearSelection
    $toglMenu insert 3 separator
}



proc _bmtss_helper {widget x y mode} {
    set clipEnabled 1
    plv_drawboxselection $widget $x $y $mode

    eval plv_hilitescan [plv_get_selected_meshes silent]
}

proc _bmtss_release_helper {widget} {
    plv_drawboxselection $widget 0 0 stop
    eval showOnlyMesh [plv_get_selected_meshes]
    clearSelection
    plv_hilitescan
}

proc bindMouseToSelectScans {widget} {
    bind $widget <ButtonPress-1>   "_bmtss_helper $widget %x %y start"
    bind $widget <B1-Motion>       "_bmtss_helper $widget %x %y move"
    bind $widget <ButtonRelease-1> "_bmtss_helper $widget %x %y stop"

    puts "\nB1-drag to select meshes; B2 to confirm"
    bind $widget <ButtonRelease-2> "_bmtss_release_helper $widget"

    bind $widget <Motion> "setSelectionCursor $widget %x %y"

    # remove any hilites
    globalset unbindUnwindProc "plv_hilitescan"
}

proc _bmtss_screen_helper {widget x y mode} {
    set clipEnabled 1
    plv_drawboxselection $widget $x $y $mode

    #eval plv_hilitescan [plv_get_selected_meshes silent]
}

proc _bmtss_screen_release_helper {widget} {
    plv_drawboxselection $widget 0 0 stop
    eval showOnlyMesh [plv_getVisiblyRenderedScans selected]
    clearSelection
    plv_hilitescan
}

proc bindMouseToScreenSelectScans {widget} {
    bind $widget <ButtonPress-1>   "_bmtss_screen_helper $widget %x %y start"
    bind $widget <B1-Motion>       "_bmtss_screen_helper $widget %x %y move"
    bind $widget <ButtonRelease-1> "_bmtss_screen_helper $widget %x %y stop"

    puts "\nB1-drag to select meshes; B2 to confirm"
    bind $widget <ButtonRelease-2> "_bmtss_screen_release_helper $widget"

    bind $widget <Motion> "setSelectionCursor $widget %x %y"

    # remove any hilites
    globalset unbindUnwindProc "plv_hilitescan"
}

proc _pick_scan_click_helper {x y b {s ""}} {
    set selected [plv_pickscan get $x $y]
    if {$selected == ""} {
	puts "Sorry, you missed."
    } else {
	if {$b == 1} {
	    # select mesh
	    globalset theMesh $selected
	    # Updates the context pickscan
	    vg_pickScanContextHelper $selected $s

	} elseif {$b == 2} {
	    # toggle visibility
	    global meshVisible
	    changeVis $selected [expr !$meshVisible($selected)]
	}
    }
}

proc _pick_scan_icp_helper {mode} {
    set selected [eval plv_pickscan get [globalset toglContextClick]]
    if {$selected == ""} {
	puts "Sorry, you missed."
    } else {
	globalset regICP$mode $selected
    }
}


proc _pick_scan_move_helper {x y} {
    set selected [plv_pickscan get $x $y]

    if {$selected == ""} {
	plv_hilitescan
	setWindowTitle pick
	set color [lindex [.tools.pickScan config -selectcolor] 3]

    } else {
	plv_hilitescan $selected
	setWindowTitle "pick $selected"
	set color [GetMeshFalseColor $selected]
    }

    .tools.pickScan config -selectcolor $color
}

proc _pick_color_click_helper {x y {s ""}} {

    set selected [plv_pickscan get $x $y]
    if {$selected == ""} {
	puts "Sorry, you missed."
    } else {

	# Update the context of pickscan
	vg_pickScanContextHelper $selected $s

	# Set mesh in colorvis
	color_vis_set_mesh $selected
	color_vis_proj_color $x $y

	# set the current mesh
	globalset theMesh $selected
    }

}

proc bindMouseToPickScan {widget} {
    cursor watch
    plv_pickscan init
    cursor restore
    puts "B1 to activate mesh; B2 to toggle visibility"
    puts "Shift-B1 to append to PICK_SCAN context mesh list."
    bind $widget <ButtonPress-1> \
	"_pick_scan_click_helper %x %y 1 %s"
    bind $widget <ButtonPress-2> \
	"_pick_scan_click_helper %x %y 2"
    bind $widget <Motion> \
	"_pick_scan_move_helper %x %y"

    global cursor
    $widget config -cursor "diamond_cross$cursor(Fore)$cursor(Back)"

    globalset onMainResizeProc { plv_pickscan init }
    globalset unbindUnwindProc {
	plv_pickscan exit
        plv_hilitescan
        setWindowTitle
        resetBGcolor .tools.pickScan
	globalset onMainResizeProc ""
    }

    global toglMenu
    $toglMenu insert 1 command -label "Pick as ICP from" \
	-command "_pick_scan_icp_helper From"
    $toglMenu insert 2 command -label "Pick as ICP to" \
	-command "_pick_scan_icp_helper To"
    $toglMenu insert 3 separator
}

proc bindMouseToPickColor {widget} {
    cursor watch
    plv_pickscan init
    cursor restore
    bind $widget <ButtonPress-1> \
	"_pick_color_click_helper %x %y %s"

    global cursor
    $widget config -cursor "diamond_cross$cursor(Fore)$cursor(Back)"

    globalset unbindUnwindProc "plv_pickscan exit; \

        resetBGcolor .tools.pickColor"
}

proc bindMouseToZoomer {widget} {
    bind $widget <ButtonPress-1> "zoomInClick %x %y %s"
    bind $widget <ButtonPress-2> "zoomOutClick %x %y %s"

    global cursor
    $widget config -cursor "diamond_cross$cursor(Fore)$cursor(Back)"
}


proc setSelectionCursor {widget x y} {
    global cursor

    set type [plv_getselectioncursor $x $y]
    $widget config -cursor "$type$cursor(Fore)"
}


proc bindMouseToChooseCenter {widget} {
    puts "Choose new rotation center: B1 to confirm, B2 to cancel"

    # set up bindings for choosing new center
    bind $widget <ButtonRelease-1> "setRotationCenter %x %y"
    bind $widget <ButtonRelease-2> "setRotationCenter none"

    global cursor
    $widget config -cursor "diamond_cross$cursor(Fore)$cursor(Back)"
}


proc bindMouseToDisplayPoint {widget} {
    bind $widget <ButtonPress-1> \
	{puts "(%x,%y): [plv_screentoworld %x %y]"
	    # addxyzpoint is for imageAlignment tool
	    addxyzpoint [plv_screentoworld %x %y] %x %y
	}

    global cursor
    $widget config -cursor "diamond_cross$cursor(Fore)$cursor(Back)"
}






######################################################################
#
# changeXXX functions -- variable trace event handlers
#
######################################################################


proc changeVis {mesh {val ""}} {
    global meshVisible

    # if val is empty, this means someone else already changed
    # meshVisible and wants to update the C code on that fact.
    if {$val != ""} {
	# but if val is non-empty and the same as the current state,
	# someone's just being daft and we don't actually have to
	# do anything.
	if {$val == $meshVisible($mesh)} {return}

	# ok, it's a real change
	set meshVisible($mesh) $val
    }

    if {[globalset selectionFollowsVisibility]} {
	# if the active mesh is invisible, need to do something about it
	if {!$meshVisible([globalset theMesh])} {
	    if {$meshVisible($mesh)} {
		# if this mesh just gained visibility, select it
		globalset theMesh $mesh
	    } else {
		# this mesh just hid... select the first mesh
		foreach maybe [array names meshVisible] {
		    if {$meshVisible($maybe)} {
			globalset theMesh $maybe
			break
		    }
		}
	    }
	}
    }

    plv_setvisible $mesh $meshVisible($mesh)
    updatePolyCount
    redraw 1
}


proc changeLoaded {mesh {val ""}} {
    global meshLoaded

    # if val is empty, this means someone else already changed
    # meshLoaded and wants to update the C code on that fact.
    if {$val != ""} {
	# but if val is non-empty and the same as the current state,
	# someone's just being daft and we don't actually have to
	# do anything.
	if {$val == $meshLoaded($mesh)} {return}

	# ok, it's a real change
	set meshLoaded($mesh) $val
    }

    scz_session activate $mesh $meshLoaded($mesh)

    updatePolyCount
    set f [globalset meshFrame($mesh)]
    set max [buildUI_ResizeResBars $mesh $f]
    $f.title.pad config -width [expr 45 - $max]

    redraw 1
}


proc changeColorMode {var dummy1 op} {
    plv_material -colormode [globalset theColorMode]
    redraw
}

proc changeBackfaceMode {var dummy1 op} {
    plv_material -backfacemode [globalset theBackfaceMode]
    redraw
}

proc changePolygonMode {var dummy1 op} {
    plv_drawstyle -polymode [globalset thePolygonMode]
    redraw
}


proc changeOverallResolution {var dummy1 op} {
    global drawResolution

    cursor watch
    plv_setoverallres $drawResolution
    redraw
    cursor restore

    # drawResolution is only allowed to be temphigh/templow/default
    # if any other, that was just to run the code above via the trace
    # so now that we've done it, go back to default
    if {$drawResolution != "reshigh" && $drawResolution != "reslow"} {
	# BUGBUG this doesn't help reset the radio button if the menu
	# is torn off, because other traces are disabled while this runs
	set drawResolution default
    }

    updatePolyCount
}


proc changeTransparentSelection {var dummy1 op} {
    # called when transparentSelection changes
    global transparentSelection
    global theMesh

    if {$transparentSelection} {
	plv_drawstyle -blend on
	set trans .5
    } else {
	plv_drawstyle -blend off
	set trans 0
    }

    if {[globalset theMover] == "mesh"} {
	if {$theMesh != ""} {
	    plv_blendmesh $theMesh $trans
	}
    }

    redraw
}


proc changeTStrip {var dummy1 op} {
    global styleTStrip

    plv_drawstyle -tstrip $styleTStrip
    redraw
    if {[winfo exists .status]} {
	if {$styleTStrip} {
	    .status.info.tstrips config -fg darkgreen
	} else {
	    .status.info.tstrips config -fg darkred
	}
    }
}

proc changeDispList {var dummy1 op} {
    global styleDispList

    plv_drawstyle -displaylist $styleDispList
    redraw
    if {$styleDispList} {
	.status.info.displist config -fg darkgreen
    } else {
	.status.info.displist config -fg darkred
    }
}


proc changeCacheRender {var dummy1 op} {
    plv_drawstyle -cachetogl [globalset styleCacheRender]
}


proc changeSlowPolyCount {var dummy1 op} {
    plv_setslowpolycount [globalset slowPolyCount]
}


proc changeRotationConstraint {var dummy1 op} {
    plv_constrain_rotation [globalset rotationConstraint]
}


proc changeManipRenderMode {var dummy1 op} {
    global manipRender
    plv_setManipRenderMode \
	$manipRender(Points) \
	$manipRender(TinyPoints) \
	$manipRender(Unlit) \
	$manipRender(Lores) \
	$manipRender(SkipDisplayList) \
	$manipRender(Thresh)
}


proc changeCurrentMesh {var dummy1 op} {
    # called when theMesh changes
    global theMesh
    global prevMesh
    global transparentSelection
    global theMover

    if {$transparentSelection} {
	if {$prevMesh != ""} {
	    plv_blendmesh $prevMesh 0
	}
	if {$theMesh != ""} {
	    if {$theMover == "mesh"} {
		plv_blendmesh $theMesh .5
	    }
	}
    }

    # this forces all necessary redraws
    plv_selectscan $theMesh
    if {$theMover == "mesh"} {
	plv_selectscan $theMesh manipulate
    }

    set prevMesh [globalset oldMesh]
    globalset oldMesh $theMesh
    setWindowTitle
    resetRotationMode
}


proc changeTheMover {var dummy1 op} {
    # called when theMover changes
    global transparentSelection
    global theMesh
    global theMover

    if {$theMover == [globalset oldMover]} {
	return
    }

    bindMouse [globalset toglPane]
    if {$transparentSelection != 0} {
	if {$theMesh != ""} {
	    if {$theMover == "mesh"} {
		plv_blendmesh $theMesh .5
	    } else {
		plv_blendmesh $theMesh 0
	    }
	}
    }

    if {$theMover == "mesh"} {
	if {$theMesh != ""} {
	    manipulateScan $theMesh
	}
    } elseif {$theMover == "viewer"} {
	manipulateScan ""
    }

    # and, since theMover changed...
    setWindowTitle
    resetRotationMode
    # any necessary redraws happened in manipulateScan
}


proc setTBCursor {widget state {change none}} {
    # stupid state from event gives what the state was before this event...

    set b1 [expr $state & 256]
    set b2 [expr $state & 512]

    switch -exact -- $change {
	p1 {set b1 1}
	r1 {set b1 0}
	p2 {set b2 1}
	r2 {set b2 0}
    }

    global cursor

    if {[globalset theMover] == "mesh"} {
	set fore AltFore
    } else {
	set fore Fore
    }

    if {$b2} {
	if {$b1} {
	    # zooming: pointing hand
	    $widget config -cursor "$cursor(PointingHand)$cursor($fore)"
	} else {
	    # panning: flat hand
	    $widget config -cursor "$cursor(FlatHand)$cursor($fore)"
	}
    } else {
	# nothing, or rotating: curved hand
	$widget config -cursor "$cursor(CurvedHand)$cursor($fore)"
    }
}


proc changeUIShowElement {var dummy1 op} {
    if {$var == "ui_showToolbar"} {
	buildUI_ShowToolbar [globalset ui_showToolbar]
    } elseif {$var == "ui_showStatusbar"} {
	buildUI_ShowStatusbar [globalset ui_showStatusbar]
    }
}


proc changePointSize {var dummy1 op} {
    plv_drawstyle -pointsize [globalset stylePointSize]
    redraw
}


proc changeMeshControlsSort {var dummy1 op} {
    resizeMCscrollbar
}


proc changeRenderOnlyActiveMesh {var dummy1 op} {
    redraw 1
}


proc changeCenterOfRotation {var dummy1 op} {
    hiliteRotationMode

    # BUGBUG -- magi -- this should call resetRotationCenter as below,
    # since it obviously needs an update.  Mouse rotations won't be hurt,
    # since they reset it themselves on lbuttondown anyway, but keyboard
    # rotation commands will use wrong center.  And maybe other things too.
    # Problem is: in certain cases (notably Invert), somehow this gets
    # called at a time when nothing is visible ?!? and in
    # PlvResetCenterOfRotation, (object, moveView), theScene->worldCenter
    # returns origin since the scene bbox will be empty.
    # Then lots of things get hosed.  This needs fixing but it doesn't
    # seem easy, offhand.
    #resetRotationCenter 1
}
