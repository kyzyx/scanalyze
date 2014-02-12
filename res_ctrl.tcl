proc deleteMeshRes {mesh meshResList res {mode unload}} {
    if {$mode == "permanent"} {
	plv_mesh_res_delete $mesh $res
    } else {
	plv_mesh_res_unload $mesh $res
    }

    load_resolutions $mesh $meshResList
    set res [plv_getcurrentres $mesh]
    hiliteResolution $meshResList $res
}


proc meshDecimateDialog {mesh} {
    # create dialog
    global uniqueInt
    set decimator [toplevel .meshDecimate$uniqueInt]
    incr uniqueInt
    wm title $decimator "$mesh resolutions"
    window_Register $decimator

    set tf [frame $decimator.topframe]
    pack $tf -side top
    set meshScaleFrame [frame $tf.meshScaleFrame]
    set meshResFrame [frame $tf.meshResFrame]

    set meshResList [Scrolled_Listbox $meshResFrame.listFrame \
			 -width 16 -height 6]
    set meshResListFrame $meshResFrame.listFrame
    set meshResButtonFrame [frame $meshResFrame.buttonFrame]

    set decimateButton [button $meshResButtonFrame.decButton \
			    -text "Decimate now" \
			    -command "render_decimated $mesh $meshResList\
                                        $meshScaleFrame.scaleMesh" \
			    -padx 0 -pady 0]
    set deleteBtn [button $meshResButtonFrame.delete \
		       -text "Delete" -padx 0 -pady 0 \
		       -command "deleteMeshRes $mesh $meshResList \
                                 [plv_getcurrentres $mesh] permanent"]

    pack $decimateButton $deleteBtn -side left
    pack $meshResButtonFrame $meshResListFrame -side top

    set scaleMesh [scale $meshScaleFrame.scaleMesh -from 1 -to 0 \
		       -variable $decimator -length 120 -relief groove \
		       -sliderlength 10 -activebackground "medium sea green"\
		       -tickinterval 1]

    pack $scaleMesh -side right -padx 5 -fill y -expand true
    $scaleMesh set 1

    pack $meshScaleFrame $meshResFrame -side left -fill both -expand true

    set method [frame $decimator.method]
    label $method.l -text Method:
    radiobutton $method.slim -text qslim \
	-variable globalDecimator -value qslim
    radiobutton $method.crunch -text plycrunch \
	-variable globalDecimator -value plycrunch
    pack $method.l $method.slim $method.crunch -side left
    pack $method -side top
    globalset globalDecimator qslim

    bind $meshResList <ButtonRelease-1> \
	"switchMeshResolutionFromList $mesh $meshResList $scaleMesh"
    bind $meshResList <ButtonRelease-3> \
	"meshResModify $mesh $meshResList %x %y"

    # fill resolution list
    set maxres [getPolyCount_maxres $mesh]
    load_resolutions $mesh $meshResList
        $scaleMesh configure -state normal -foreground "Black"
    hiliteResolution $meshResList [plv_getcurrentres $mesh]
    $scaleMesh configure -relief groove -troughcolor "Light Grey"
    $scaleMesh configure -from $maxres -tickinterval $maxres
    $scaleMesh set [getPolyCount_currentres $meshResList]
}


proc meshResModify {mesh list x y} {
    set select [$list nearest $y]
    set curpolys [getPolyCount_indexedres $list $select]

    if {$x < 18} {
	toggleResPreload $mesh $list $select
    } elseif {$x < 36} {
	set current [plv_getcurrentres $mesh]
	if {$current == $curpolys} {
	    puts "Error: cannot unload current res"
	} else {
	    deleteMeshRes $mesh $list $curpolys unload
	}
    } else {
	# do nothing
    }
}


proc switchMeshResolutionFromList {mesh meshResList scaleMesh} {
    #set level [lindex [$meshResList curselection] 0]
    set level [getPolyCount_currentres $meshResList]
    switchMeshResolution $mesh $level $scaleMesh $meshResList
}


proc hiliteResolution {meshResList res} {
    set resList [$meshResList get 0 end]

    $meshResList selection clear 0 end
    for {set i 0} {$i < [llength $resList]} {incr i} {
	if {$res == [extractPolyCount [lindex $resList $i]]} {
	    $meshResList selection set $i
	    $meshResList activate $i
	    return
	}
    }
}


proc load_resolutions {theMesh meshResList} {
    set level [$meshResList cursel]
    $meshResList delete 0 end;

    set resstring [plv_getreslist $theMesh]
    while {[string compare $resstring ""] != 0} {
	scan $resstring {%s} token
	set resstring [string trimleft $resstring $token]
	set resstring [string trimleft $resstring]
	$meshResList insert end $token
    }

    if {$level != ""} {
	$meshResList selection set $level
    }
}


proc extractPolyCount {resstring} {
    set polys 0
    scan $resstring {%[()AL-]:%d} modes polys
    return $polys
}

proc getPolyCount_maxres {theMesh} {
    set resstring [plv_getreslist $theMesh]
    return [extractPolyCount $resstring]
}


proc getPolyCount_indexedres {list index} {
    set resstring [lindex [$list get $index] 0]
    return [extractPolyCount $resstring]
}

proc getPolyCount_currentres {meshResList} {
    set level [$meshResList cursel]
    if {$level == ""} {set level 0}

    return [getPolyCount_indexedres $meshResList $level]
}


proc toggleResPreload {theMesh meshResList level} {
    set resstring [lindex [$meshResList get $level] 0]
    scan $resstring {%[()\-AL]:%d} modes polys

    if {[string index $modes 1] == "A"} {
	set opposite 0
    } else {
	set opposite 1
    }

    plv_setmeshpreload $theMesh $polys $opposite
    load_resolutions $theMesh $meshResList
}

proc switchMeshResolution {theMesh theMeshLevel scaleMesh meshResList} {
    global toglPane

    # update the meshset with current resolution
    setMeshResolution $theMesh $theMeshLevel

    # reset the position of the resolution slider
    $scaleMesh set [getPolyCount_currentres $meshResList]

    # rebuild the mesh list in case this forced us to fetch an unloaded mesh
    load_resolutions $theMesh $meshResList
}

proc render_decimated {theMesh meshResList resSlider} {
    set newpolys [plv_decimate $theMesh [$resSlider get]\
		      [globalset globalDecimator]]

    setMeshResolution $theMesh $newpolys
    load_resolutions $theMesh $meshResList
    hiliteResolution $meshResList $newpolys
}


proc setMeshResolution {mesh resolution} {
    cursor watch
    plv_resolution $mesh $resolution

    if {![globalset noui]} {
	redraw
	refreshCorrespRegWindows $mesh

	updatePolyCount
	global meshFrame
	buildUI_ResizeResBars $mesh $meshFrame($mesh)
    }

    cursor restore
}


proc buildMeshResList {mesh menu} {
    global toglPane
    global theMeshLevel

    # clear resolution list
    $menu delete 0 end

    # attempt to get current res; bail gracefully on failure
    # tcl8.0.5 appears to call this as the -postcommand even for
    # disabled menus, so just graying it out when it doesn't apply
    # isn't enough.
    if {[catch {set theMeshLevel [plv_getcurrentres $mesh]}]} {
        $menu add command -label "(No current mesh)" -state disabled
        return
    }

    # then add all resolutions available
    set resolutions [plv_getreslist $mesh]
    foreach res $resolutions {
	set polys [string range $res [expr [string first : $res] + 1] end]
	$menu add radiobutton -label $polys -command "
	    setMeshResolution $mesh $polys
	    $toglPane render
	" -value $polys -variable theMeshLevel
    }

    $menu add separator
    $menu add command -label "Highest" -accelerator "Ctrl+." \
	-command "setMeshResolution $mesh highest"
    $menu add command -label "Higher" -accelerator . \
	-command "setMeshResolution $mesh higher"
    $menu add command -label "Lower" -accelerator , \
	-command "setMeshResolution $mesh lower"
    $menu add command -label "Lowest" -accelerator "Ctrl+," \
	-command "setMeshResolution $mesh lowest"
    $menu add separator
    $menu add command -label "Other..." \
	-command "meshDecimateDialog $mesh"

    $menu config -tearoffcommand "setMenuTitle \"$mesh resolutions\""
}
