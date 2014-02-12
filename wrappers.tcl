proc flashCenterOfRotation {} {
    global rotCenterVisible
    global rotCenterOnTop

    set cv $rotCenterVisible
    set cot $rotCenterOnTop

    set rotCenterOnTop 1
    set rotCenterVisible 1
    redraw 1
    set rotCenterVisible 0
    redraw 1
    set rotCenterVisible 1
    redraw 1

    set rotCenterVisible $cv
    set rotCenterOnTop $cot
}


alias bmset bmesh
alias rotmesh plv_rotmesh
alias rotview plv_rotview

proc rotview {axis angle} {
    plv_rotview $axis $angle
}



proc highQualityDraw { {aa -1} {shadows -1} } {
    # make any menus onscreen go away before, not after, we redraw
    update

    if {$aa == -1} {
	set aa [globalset styleAntiAlias]
    }
    if {$shadows == -1} {
	set shadows [globalset styleShadows]
    }

    plv_drawstyle -antialias $aa -shadows $shadows
    globalset highQualSingle 1
    redraw 1
    globalset highQualSingle 0
    plv_drawstyle \
	-antialias [globalset styleAntiAlias] \
	-shadows [globalset styleShadows]
}


proc invertMeshNormals {mesh} {
    FlipMeshNormals $mesh
    global toglPane
    $toglPane render
}


proc reloadMesh {mesh} {
    set file [plv_get_scan_filename $mesh]

    set answer [tk_messageBox -default yes -icon question \
		    -message "Save xform first?" \
		    -title "Scanalyze: reload $mesh" -type yesnocancel]

    if {$answer == "cancel"} {
	return
    }

    if {$answer == "yes"} {
	plv_write_metadata $mesh xform;
    }

    # preserve resolution, selection as much as possible
    set oldMesh [globalset theMesh]
    set oldColor [plv_scan_colorcode get $mesh]
    set rl [plv_getreslist $mesh short]
    set ri [lsearch -exact $rl [plv_getcurrentres $mesh]]

    redraw block
    catch {
	if {[confirmDeleteMesh $mesh]} {
	    readfile $file

	    # if it was selected, it won't be now; restore this
	    if {$oldMesh == $mesh} {
		globalset theMesh $mesh
	    }

	    # set old resolution index
	    set rl [plv_getreslist $mesh short]
	    plv_resolution $mesh [lindex $rl $ri]
	    SetMeshFalseColor $mesh $oldColor
	}
    }
    redraw flush
}



# TODO: confirmation
# if dirty, prompt save? yes/no/cancel
# if clean, prompt delete? ok/cancel
# or maybe this should happen automatically on the C++ side, in the
# destructor, so you get that behavior at exit time too

proc confirmDeleteMesh {mesh} {

    plv_meshsetdelete $mesh
    removeMeshFromWindow $mesh
    redraw 1
    plv_pickscan init

    updateFromAndToMeshNames

    return 1
}

proc cursor {name} {
    global toglPane
    global oldCursor

    if {[globalset noui]} return

    if {$name == "restore"} {
	$toglPane config -cursor $oldCursor
    } else {
	set oldCursor [lindex [$toglPane config -cursor] 4]
	$toglPane config -cursor $name
    }
    update idletasks
}


proc showCameraInfo {} {
    tk_messageBox -title "Camera info" -icon info \
	-message [plv_camerainfo graphical]
}


proc showMeshInfo {theMesh} {
    tk_messageBox -title "Mesh info for $theMesh" -icon info \
	-message [plv_meshinfo $theMesh graphical]
}


proc setShininess {shiny} {
    if {$shiny} {
        plv_material -shininess 90 -specular 0.25 0.25 0.25
    } else {
        plv_material -specular 0 0 0
    }
}


proc analyze_line_depth {} {
    global theMesh

    plv_analyze_line_depth $theMesh
}


alias quit confirmQuit


proc manipulateScan {scan} {
    plv_selectscan $scan manipulate
}


proc buildCurrentMeshResList {menu} {
    buildMeshResList [globalset theMesh] $menu
}


proc translateinplane {x y} {
    set x [expr $x * 1.0 / [globalset theUnitScale]]
    set y [expr $y * 1.0 / [globalset theUnitScale]]

    plv_translateinplane $x $y
}


proc getSortedMeshList {{onlyvis 0}} {
    global meshControlsSort

    if {$onlyvis} {
	set invis "onlyvisible"
    } else {
	set invis ""
    }

    return [eval plv_sort_scan_list \
		$meshControlsSort(1) \
		$meshControlsSort(2) \
		$meshControlsSort(3) \
		$meshControlsSort(4) \
		$meshControlsSort(5) \
		$meshControlsSort(mode) \
		$invis \
	   ]
}


proc showOnlyIcpMeshes {} {
    global regICPFrom
    global regICPTo

    showOnlyMesh $regICPFrom $regICPTo
}


proc getActiveMesh {} {
    if {[globalset theMover] == "mesh"} {
	return [globalset theMesh]
    } else {
	return ""
    }
}


proc alignToFitPlane {{mover camera}} {
    global pfcrData

    plv_clipBoxPlaneFit [globalset theMesh] align $pfcrData move$mover
}


proc GetMeshFalseColor {mesh} {
    return [plv_scan_colorcode get $mesh]
}


proc SetMeshFalseColor {mesh color} {
    plv_scan_colorcode set $mesh $color
    global meshFrame
    $meshFrame($mesh).title.color config -background $color
    $meshFrame($mesh).title.curBar config -background $color
}


proc guiError {message {parent .}} {
    tk_messageBox -title "Scanalyze error" -icon error \
	-parent $parent -message $message
}
