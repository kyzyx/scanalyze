proc redraw {{force 0}} {

    if {[globalset noui]} return
    if {![globalexists toglPane]} return

    if {$force == "block"} {
	globalset noRedraw [expr [globalset noRedraw] + 1]
	return
    } elseif {$force == "allow"} {
	set nr [expr [globalset noRedraw] - 1]
	if {$nr < 0} {
	    puts "Warning: noRedraw got set to $nr!"
	    set nr 0
	}
	globalset noRedraw $nr
	return
    } elseif {$force == "flush"} {
	update idletasks
	redraw allow
	redraw 1
	return
    } elseif {$force == "safeflush"} {
	if {[expr [globalset noRedraw] > 0]} {
	    redraw flush
	} else {
	    redraw 1
	}
	return
    }

    if {$force} {
	plv_invalidateToglCache
    }

    [globalset toglPane] render
}


proc setMenuTitle {caption menu window} {
    wm title $window $caption
    window_Register $window
}


proc setRotationCenter {{x none} {y ""}} {
    set success 0
    if {$x != "none"} {

	set result [catch \
			[plv_set_this_as_center_of_rotation $x $y] msg ]
	if { $result == 0 } {
	    globalset theCoR custom
	    set success 1
	} else {
	    tk_dialog .errMsg Error $msg "" 0 Ok
	}
	redraw 1

    } else {

	set success 1

    }

    global theMover
    if { $success == 1 & $theMover == "rotCenter"} {
	globalset theMover [globalset prevMover]
    }
}


proc resetRotationCenter {{force 0}} {
    # entry from choice of new rotation mode will use "force", and should
    # always succeed.  Otherwise, this is being called from trackball
    # bindings and should only continue if we're rotating (b1 and not b2).
    if {![expr ($force) || \
	 ([globalset mouse_state_1] && ![globalset mouse_state_2])]} {
	return
    }

    switch -exact -- [globalset theCoR] {
	auto    {plv_reset_rotation_center object}

	screen  {if {[globalset theMover] == "mesh"} {
	    plv_reset_rotation_center object
	} else {
	    plv_reset_rotation_center screen
	}
	}

	# custom gets left alone unless explicitly changed
    }
}


proc hiliteRotationMode {} {
    if {[globalset noui]} return

    set custom .tools.chooseCenter
    set auto   .tools.autoCenter
    set screen .tools.screenCenter

    if {![winfo exist $custom]} return
    global theCoR
    globalset theCoR_save([getActiveMesh]) $theCoR

    switch -exact -- $theCoR {
	custom {set active $custom }
	auto   {set active $auto }
	screen {set active $screen }
    }

    foreach control "$custom $auto $screen" {
	$control config -foreground Black
    }
    $active config -foreground ForestGreen
}


proc resetRotationMode {} {
    global theCoR

    if {[catch {set theCoR [globalset theCoR_save([getActiveMesh])]}]} {
	set theCoR auto
    }
}


proc setHome {} {
    global theMesh
    global theMover

    if {$theMover == "mesh"} {
	SetHome $theMesh
    } else {
	SetHome ""
    }
}


proc setHomeAll {} {
    foreach mesh [plv_listscans] {
	SetHome $mesh
    }
    SetHome ""
    redraw
}


proc goHome {{mesh " _"}} {
    global theMesh
    global theMover

    # use mesh if given ("" for viewer), otherwise selected mesh (theMesh)
    if {0 != [string compare $mesh " _"]} {
	GoHome $mesh
    } else {
	if {$theMover == "mesh"} {
	    GoHome $theMesh
	} else {
	    GoHome ""
	}
    }
    redraw
}


proc goHomeAll {} {
    foreach mesh [plv_listscans] {
	GoHome $mesh
    }
    GoHome ""
    redraw
}


proc setWindowTitle {{extra ""}} {
    global theMesh
    global theMover

    set title "scanalyze"

    if {$extra == ""} {
	# if no info specified, use current mesh name
	if {$theMesh != ""} {
	    set extra $theMesh
	    if {$theMover == "mesh"} {
		set extra "$extra*"
	    }
	}
    }

    wm title . "$title -- $extra"
}


proc detectCursorCapabilities {} {
    # handle tcl's cursor discrepancies -- basically, lots of cursor options
    # available on unix aren't implemented at all on Windows.

    global cursor
    global env

    set cursor(FlatHand) "@$env(SCANALYZE_DIR)/flat_hand.xbm"

    if [catch {. config -cursor "$cursor(FlatHand) red"} result] {
	puts $result
	# error... assume only basic (built-in, no color) cursor support
	set cursor(FlatHand) boat
	set cursor(CurvedHand) boat
	set cursor(PointingHand) boat
	set cursor(Fore) ""
	set cursor(AltFore) ""
	set cursor(Back) ""
    } else {
	# OK ... go ahead with other cursors and colors too
	set cursor(CurvedHand) "@$env(SCANALYZE_DIR)/curved_hand.xbm"
	set cursor(PointingHand) "@$env(SCANALYZE_DIR)/pointing_hand.xbm"
	set cursor(Fore) " red"
	set cursor(AltFore) " green"
	set cursor(Back) " white"
    }
    . config -cursor {}
}


proc showAllMeshes {mode} {
    vg_CreateView PREVIOUS

    if {$mode != "invert"} {
	# hiding/showing all, might spuriously change active mesh when
	# we hide it when something else is still visible, or show something
	# else when it's not visible.  This isn't appropriate because
	# on hide/show all, there's no use in trying to follow visibility.
	# so, remember which it was
	set active [globalset theMesh]
    }

    redraw block
    foreach mesh [plv_listscans root] {
	if {$mode == "invert"} {
	    set vis [expr ! [plv_getvisible $mesh]]
	} else {
	    set vis $mode
	}

	changeVis $mesh $vis
    }

    if {[info exists active]} {
	globalset theMesh $active
    }

    redraw flush
}


proc showMesh {args} {
    eval showHideAllMeshes 1 - $args
}

proc hideMesh {args} {
    eval showHideAllMeshes 0 - $args
}

proc showOnlyMesh {args} {
    global theMesh

    if {$theMesh == ""} { return }
    eval showHideAllMeshes 1 0 $args
}

proc hideOnlyMesh {args} {
    eval showHideAllMeshes 0 1 $args
}

proc showHideAllMeshes {ifmatch ifnot args} {
    redraw block
    set old [globalset theMesh]

    vg_CreateView PREVIOUS

    # build lists of all meshes, and those to show, in same order
    # then walk along "all" list, and if current entry is same as head
    # of "yes" list, show it, else hide it; then advance all list,
    # and yes list only if used
    set all [lsort -dictionary [getMeshList]]
    set yes [lsort -dictionary $args]
    set iYes 0

    foreach mesh $all {
	if {$mesh == [lindex $yes $iYes]} {
	    changeVis $mesh $ifmatch
	    incr iYes
	} else {
	    if {$ifnot != "-"} {
		changeVis $mesh $ifnot
	    }
	}
    }

    # preserve selection if possible
    if {[plv_getvisible $old]} {
	globalset theMesh $old
    }

    redraw flush
}


# getVisibleMeshes 1 - to get visible meshes
# getVisibleMeshes 0 - to get invisible meshes
proc getVisibleMeshes {{mode 1}} {
    set meshlist ""
    foreach mesh [plv_listscans] {
	if {[plv_getvisible $mesh] == $mode} {
	    lappend meshlist $mesh
	}

    }

    return $meshlist
}


# options - root (defaults), leaves, groups.
proc getMeshList {args} {
    return [eval plv_listscans $args]
}


proc writeXform {} {
    foreach mesh [plv_listscans] {
	mms_savexform $mesh
    }
}


proc idlecallback {proc} {
    global _idlecallbacks

    if {[array get _idlecallbacks $proc] != ""} {
	after cancel $_idlecallbacks($proc)
    }

    set _idlecallbacks($proc) [after 100 _${proc}_idlecallback]
}


proc updatePolyCount {} {
    # batch multiple requests together
    idlecallback updatePolyCount
}


proc _updatePolyCount_idlecallback {} {
    set pc 0
    set vpc 0
    foreach mesh [plv_listscans] {
	set poly [getPolyCount_maxres $mesh]
	set pc [expr $poly + $pc]

	if {[plv_getvisible $mesh]} {
	    set poly [plv_getcurrentres $mesh]
	    set vpc [expr $poly + $vpc]
	}
    }

    globalset polyCount $pc
    globalset visPolyCount $vpc
}


proc selectScan {name} {
    global theMesh
    global theMover
    global enabledWhenMeshSelected

    set theMesh $name

    if {![globalset noui]} {
	if {$theMesh == ""} {
	    set state disabled
	} else {
	    set state normal
	}

	foreach control $enabledWhenMeshSelected {
	    $control config -state $state
	}
	.menubar entryconfig Mesh -state $state
    }

    # if in manipulate-mesh mode, update selection
    if {[string compare $theMover "mesh"] == 0} {
	manipulateScan $name
    }
}


proc confirmQuit {} {
    if {![globalset noui]} {
	switch -exact -- [globalset exitConfirmation] {
	    always
	    {
		if {[confirmSaveMeshes 1] != "succeed"} return
	    }

	    dirty
	    {
		if {[confirmSaveMeshes 0] != "succeed"} return
	    }

	    never
	    {
	    }
	}

	if {[globalset exitSaveXforms]} {
	    if {![fileWriteAllScanXforms]} {
		tk_messageBox \
		    -title "Scanalyze: quitting will lose alignment." \
		    -message "Some or all xforms could not be saved.  If\
                              you want to quit anyway, disable 'save\
                              xforms on exit', or use Ctrl+C."
		return
	    }
	}

	# save settings
	buildUI_shutdown
	write_preferences
    }

    # data structure cleanup on C++ side
    plv_shutdown

    # and we're out
    puts ""
    exit
}

proc confirmFlattenMeshes {} {

    set meshes [getMeshList]
    set i 0

    set unsaved $meshes


    global confirmSaveMesh
    if {[info exists confirmSaveMesh]} {
	unset confirmSaveMesh
    }
    set confirmSaveMesh() ""

    set d [toplevel .confirmSaveMesh]
    wm title $d "Scanalyze flatten confirmation"
    label $d.l -text "Writes files incorporating mesh xform and current res.\n  Writes to <meshname>.ply, potentially overwriting exiting.\n Flatten them now?\n"

    set f [frame $d.caption]
    label $f.lm -text "Scan named:"
    label $f.lc -text "Flatten"
    pack $f.lm -side left
    pack $f.lc -side right

    foreach mesh $unsaved {
	incr i
	set f [frame $d.f$i]
	label $f.l -text $mesh
	checkbutton $f.s -variable confirmSaveMesh($mesh)
	set confirmSaveMesh($mesh) 1
	pack $f.l -side left
	pack $f.s -side right
    }

    set c [frame $d.control]
    button $c.ok -text OK \
	-command "set confirmSaveMesh() sure; destroy $d"
    button $c.cancel -text Cancel \
	-command "destroy $d"
    button $c.selall -text "Select all" \
	-command {
	    foreach mesh [array names confirmSaveMesh] {
		set confirmSaveMesh($mesh) 1
	    }
	    unset mesh
	}
    button $c.selnone -text "Deselect all" \
	-command {
	    foreach mesh [array names confirmSaveMesh] {
		set confirmSaveMesh($mesh) 0
	    }
	    unset mesh
	}
    packchildren $c -side left

    packchildren $d -side top -fill x -expand 1
    grab set $d
    tkwait window $d
    if {$confirmSaveMesh() != "sure"} {
	# means user clicked Cancel
	return fail
    }
    unset confirmSaveMesh()

    foreach mesh [array names confirmSaveMesh] {
	if {$confirmSaveMesh($mesh)} {
	    set res [plv_getcurrentres $mesh]
	    plv_write_resolutionmesh $mesh $res $mesh.ply flatten

	}
    }

    return succeed
}

proc confirmSaveMeshes {{confirmOnClean 0}} {
    set meshes [concat [getMeshList leaves] [getMeshList groups]]
    set i 0
    puts $meshes
    set unsaved ""
    foreach mesh $meshes {
	# is it modified?  if so, add to dialog
	if {[plv_is_scan_modified $mesh]} {
	    lappend unsaved $mesh
	}
    }

    if {[llength $unsaved] == 0} {
	if {$confirmOnClean} {
	    return [confirmCleanQuit]
	} else {
	    return succeed
	}
    }

    global confirmSaveMesh
    if {[info exists confirmSaveMesh]} {
	unset confirmSaveMesh
    }
    set confirmSaveMesh() ""

    set d [toplevel .confirmSaveMesh]
    wm title $d "Scanalyze save confirmation"
    wm transient $d .
    label $d.l -text "You have unsaved scans/groups.  Save them now? \n (Nested groups will be saved automatically if you save the parent)"

    set f [frame $d.caption]
    label $f.lm -text "Scan named:"
    label $f.lc -text "Save"
    pack $f.lm -side left
    pack $f.lc -side right

    foreach mesh $unsaved {
	incr i
	set f [frame $d.f$i]
	label $f.l -text $mesh
	checkbutton $f.s -variable confirmSaveMesh($mesh)
	set confirmSaveMesh($mesh) 1
	pack $f.l -side left
	pack $f.s -side right
    }

    set c [frame $d.control]
    button $c.ok -text OK \
	-command "set confirmSaveMesh() sure; destroy $d"
    button $c.cancel -text Cancel \
	-command "destroy $d"
    button $c.selall -text "Select all" \
	-command {
	    foreach mesh [array names confirmSaveMesh] {
		set confirmSaveMesh($mesh) 1
	    }
	    unset mesh
	}
    button $c.selnone -text "Deselect all" \
	-command {
	    foreach mesh [array names confirmSaveMesh] {
		set confirmSaveMesh($mesh) 0
	    }
	    unset mesh
	}
    packchildren $c -side left

    packchildren $d -side top -fill x -expand 1
    grab set $d

    tkwait window $d

    if {$confirmSaveMesh() != "sure"} {
	# means user clicked Cancel
	return fail
    }
    unset confirmSaveMesh()

    foreach mesh [array names confirmSaveMesh] {
	if {$confirmSaveMesh($mesh)} {
	    if { ![plv_groupscans isgroup $mesh] } {
		if {![saveScanFile $mesh]} {
		    return fail
		}
	    } else {
		group_recursiveSave $mesh [plv_groupscans list $mesh] [pwd]
	    }
	}
    }

    return succeed
}


proc confirmCleanQuit {} {
    if (![tk_dialog .confirmQuit "Confirm quit" \
	"Do you really want to quit scanalyze?" \
	     question 0 Yes No]) {
	return succeed
    }

    return fail
}


proc extractRes {r} {
    set col [string first ":" $r]
    return [string range $r [expr 1 + $col] end]
}


proc selectNthResolution {iRes} {
    if {[globalset noui]} {
	set meshes [plv_listscans]
    } else {
	if {[globalset selectResIncludesInvisible]} {
	    set meshes [getMeshList]
	} else {
	    set meshes [getVisibleMeshes]
	}
    }

    redraw block
    progress start [llength $meshes] "Switch to res $iRes"
    set num 0
    foreach mesh $meshes {
	if {[baildetect {selectNthResolutionForMesh $iRes $mesh} bailmsg]} {
	    puts "Res-change bailed: $bailmsg"
	    break
	}

	progress update [incr num]
    }

    progress done
    redraw flush
}


proc selectNthResolutionForMesh {iRes mesh} {
    set avail [plv_getreslist $mesh]
    set i $iRes
    if {$i >= [llength $avail]} {
	set i [expr [llength $avail] - 1]
    }

    set chosen [lindex $avail $i]
    set chosen [extractRes $chosen]

    setMeshResolution $mesh $chosen
}


proc selectNthResolutionForCurrentMesh {iRes} {
    selectNthResolutionForMesh $iRes [globalset theMesh]
}


# cd is a dangerous command, because things that were loaded from a
# relative path won't be able to save themselves
proc overrideCdCommand {} {
    rename cd _chdir

    proc cd {arg} {
	# we'll allow cd from scripts, assuming they know what they're
	# doing, but NOT from the command line.
	if {[info level] > 1} {
	    return [_chdir $arg]
	} else {
	    puts "cd is dangerous and unadvisable,\
                  because scans and transforms "
	    puts "already loaded may not be able to save\
                  themselves afterwards."
	    puts "If you understand the implications of\
                  this and want to do it"
	    puts "anyway, use chdir."
	}
    }

    proc chdir {arg} {
	_chdir $arg
	puts "Warning: working directory set to [pwd]"
	puts "If this causes problems, return to [globalset _initial_dir]"
    }

    globalset _initial_dir [pwd]
}


proc getMemUsage {} {
    # get memory info from ps
    # catch any return other than return statement (TCL_RETURN)

    if {1 == [catch {

	global unix
	if {![info exists unix]} {
		if {[catch  {set unix [exec uname]}]} {
			set unix IRIX32
		}
	}

	global _pageSize
	# need pageSize for IRIX
	if {![info exists _pageSize]} {
	    if {$unix == "IRIX64"} {
		set _pageSize 16
	    } else {
		set _pageSize 4
	    }
	}

	if {$unix == "Linux"} {
	    set virt [expr [exec ps -o vsz= -p [pid]] / 1024]
	    set res [expr [exec ps -o rss= -p [pid]] / 1024]
	}
	if {$unix == "IRIX" || $unix == "IRIX32" || $unix == "IRIX64"} {
	    # i.e. IRIX
	    set mem [exec ps -o vsz=,rss= -p [pid]]
	    set virt [expr [lindex $mem 0] / 1024]
	    set res [expr [lindex $mem 1] * $_pageSize / 1024]


	    # ps (irix 6.5) appears to have a bug, such that ps -o vsz=
	    # will return negative numbers for virtual sizes over 2gb even
	    # though the number it returns, because it's in K, is nowhere near
	    # 32 bits.  We can fix this between 2 and 4 gb though; processes
	    # bigger than 4gb will wrap over.
	    if {$virt < 0} {
					incr virt 4096
	    }
	} else {
	}
	set result "Res: ${res}M / Virt: ${virt}M"
    }
    ]} {

	set result "Mem: unavailable"
    }

    return $result
}


proc autoSetText {widget command} {
    $widget config -text [$command]
    after 5000 "autoSetText $widget $command"
}

proc releaseAllMeshes {} {
    if {[globalset selectResIncludesInvisible]} {
	set meshes [getMeshList]
    } else {
	set meshes [getVisibleMeshes]
    }

    redraw block
    set ok 1

    foreach mesh $meshes {
	set reses [plv_getreslist $mesh]
	# don't want to toss top one
	set reses [lrange $reses 1 end]
	foreach res $reses {
	    set res [extractRes $res]

	    if {[catch {plv_mesh_res_unload $mesh $res}]} {
		set ok 0
	    }

	}
    }
    redraw flush

    if {!$ok} {
	tk_messageBox -title "scanalyze" -icon error -type ok \
	    -message "Some scan(s) failed to release resolution(s)."
    }
}

proc chooseColor {usage} {
    if {$usage == "background"} {
	set command plv_drawstyle
    } else {
	set command plv_material
    }

    set old [$command -$usage]

    set color [tk_chooseColor -title "Select $usage color" -initialcolor $old]
    if {$color != ""} {
	set r [expr 0x[string range $color 1 2] / 255.0]
	set g [expr 0x[string range $color 3 4] / 255.0]
	set b [expr 0x[string range $color 5 6] / 255.0]

	$command -$usage $r $g $b
	update
    }
}


proc getUniqueInt {} {
    global uniqueInt
    return [incr uniqueInt]
}


proc selectMesh {which} {
    set list [lsort -dictionary [getVisibleMeshes]]
    set ind [lsearch -exact $list [globalset theMesh]]

    if {$ind != -1} {
	if {$which == "prev"} {
	    incr ind -1
	} else {
	    incr ind 1
	}

	set newMesh [lindex $list $ind]
	if {$newMesh != ""} {
	    globalset theMesh $newMesh
	}
    }
}

proc getSortedMeshControlsList {} {
    global meshFrame
    set inlist [getSortedMeshList]
    set outlist {}
    foreach el $inlist {
	if {[lsearch -exact [array names meshFrame] "$el"] >= 0} {
	    lappend outlist $el
	}
    }
    return $outlist
}

proc selectMeshFromAll {which} {
    set list [getSortedMeshControlsList]
    set ind [lsearch -exact $list [globalset theMesh]]

    if {$ind != -1} {
	if {$which == "prev"} {
	    incr ind -1
	} else {
	    incr ind 1
	}
	set newMesh [lindex $list $ind]

	if {$newMesh != ""} {
	    globalset theMesh $newMesh
	}
    }
}

proc checkOSversion {} {
    set word [plv_getwordsize]
    if {![catch {set os [exec uname]}]} {
	if {$os == "IRIX64" && $word != 8} {
	    set msg \
		"You are running the 32-bit version of scanalyze, even\
                 though you're lucky enough to be using a machine that can\
                 handle the 64-bit version.\n\nIt is recommended that\
                 instead of continuing, you use said 64-bit version, or\
                 scanalyze will crash if you load too many meshes."
	    if [globalset noui] {
		puts "$msg\n\nPress Ctrl+C now to quit, or return to continue."
		gets stdin
	    } else {
		tk_messageBox -message \
		    "$msg\n\nPress Ctrl+C now to quit, or OK to continue." \
		    -title "Scanalyze version warning"
	    }
	}
    }
}


proc askQuestion {q} {
    toplevel .ask
    wm title .ask "Scanalyze has a question for you"
    label .ask.l -text $q -anchor w
    entry .ask.e -width 32
    frame .ask.b
    button .ask.b.o -text OK \
	-command {set askQuestionAnswer [.ask.e get]; destroy .ask}
    button .ask.b.c -text Cancel \
	-command {set askQuestionAnswer ""; destroy .ask}
    packchildren .ask.b -side left
    packchildren .ask -side top -anchor w -fill x -padx 4 -pady 3
    grab set .ask
    tkwait window .ask

    global askQuestionAnswer
    set a [set askQuestionAnswer]
    unset askQuestionAnswer
    return $a
}


proc saveCameraPosition {menu} {
    set name [askQuestion "Enter name for camera position:"]
    if {$name != ""} {
	savecam $name
	$menu add command -label $name \
	    -command "restorecam [list $name]"
    }
}


proc savecam {name} {
    global cameraPositions

    set cameraPositions($name) [plv_positioncamera]
}


proc restorecam {{name ""}} {
    global cameraPositions

    if {$name == ""} {
	puts "Saved camera positions:"
	foreach pos [lsort -dictionary [array names cameraPositions]] {
	    puts "\t$pos"
	}
    } else {
	eval plv_positioncamera $cameraPositions($name)
    }
}


proc selectResAbove {above} {
    # BUGBUG this is identical to selectNthRes except for the
    # proc call in the middle
    if {[globalset noui]} {
	set meshes [plv_listscans]
    } else {
	if {[globalset selectResIncludesInvisible]} {
	    set meshes [getMeshList]
	} else {
	    set meshes [getVisibleMeshes]
	}
    }

    redraw block
    progress start [llength $meshes] "Switch to res above $above"
    set num 0
    foreach mesh $meshes {
	if {[baildetect {selectResAboveForMesh $above $mesh} bailmsg]} {
	    puts "Res-change bailed: $bailmsg"
	    break
	}

	progress update [incr num]
    }

    progress done
    redraw flush
}


proc selectResAboveForMesh {above mesh} {
    set list [plv_getreslist $mesh]
    for {set i 0} {$i < [llength $list]} {incr i} {
	set res [extractPolyCount [lindex $list $i]]

	if {$res < $above} {
	    if {$i > 0} {
		# use previous res, which was above (or 0 if they're
		# all under)
		incr i -1
	    }

	    setMeshResolution $mesh [extractPolyCount [lindex $list $i]]
	    return
	}
    }

    puts "Warning: selectResForMesh ($above,$mesh) failed"
}


proc selectResAboveUI {} {
    set above [askQuestion "Select resolution of at least how many polys?"]
    if {$above != ""} {
	selectResAbove $above
    }
}


proc ls {args} {
    eval shellcommand ls $args
}


proc mv {args} {
    eval shellcommand mv $args
}


proc cp {args} {
    eval shellcommand cp $args
}


proc shellcommand {cmd args} {
    set files ""
    foreach filespec $args {
	set glob [glob -nocomplain -- $filespec]
	if {$glob == ""} {
	    # treat as option, use verbatim
	    lappend files $filespec
	} else {
	    eval lappend files [lsort $glob]
	}
    }

    eval exec $cmd $files
}


proc deleteInvisibleMeshes {} {
    redraw block
    foreach mesh [getVisibleMeshes 0] {confirmDeleteMesh $mesh}
    redraw flush
}


proc chooseMeshFalseColor {mesh} {
    set oldcolor [GetMeshFalseColor $mesh]
    set color [tk_chooseColor -title "Select color for $mesh" \
		  -initialcolor $oldcolor]
    if {$color != ""} {
	SetMeshFalseColor $mesh $color
	redraw 1
    }
}


proc without_redraw {script {maskerrors 0}} {
    redraw block
    set err [catch {uplevel "$script"} errmsg]
    redraw flush

    if {$err} {
	if {$maskerrors == "maskerrors"} {
	    puts "[globalset errorInfo]"
	} else {
	    error $errmsg [globalset errorInfo]
	}
    }
}


proc setSoftShadowLength {} {
    set len [askQuestion "Soft shadow length (range .01 to .2 works\
                          well, default is .05)"]
    if {$len != ""} {
	plv_drawstyle -softshadowlength $len
    }
}

# kberg
# Code from running an external program on a mesh.
# Most code taken from the doICP widget
#
proc ExtProg {} {
    set names [getMeshList]
    if {[expr [llength $names] < 1]} {
	puts "You need at least one mesh to run an external program on"
	return
    }

    if {[window_Activate .extProg]} { return }
    toplevel .extProg

    message .extProg.instruct -aspect 750  \
	    -text "Choose a mesh, the name of the external program to run and the name of the resulting mesh"\
	    -justify left

    pack .extProg.instruct -side top -anchor w

    # list of all meshes
    set which [frame .extProg.which]
    set meshFrom [eval tk_optionMenu $which.mFrom extFrom bogus]
    $meshFrom config -postcommand "refreshICPRegMeshes $meshFrom"

    listRegMeshes "$meshFrom" 0
    set ind [menuInvokeByName $meshFrom [globalset theMesh]]

    label $which.lFrom -text "Input Mesh:" -padx 3

    pack $which.lFrom $which.mFrom
    pack $which -side top -anchor w

    # entry box
    frame .extProg.eb

    message .extProg.eb.instruct -aspect 1000  \
	    -text "Resulting mesh name:"
    entry .extProg.eb.resName -relief sunken -textvariable resMeshName
    message .extProg.eb.cmdInstruct -aspect 1000 \
	    -text "Command:"
    entry .extProg.eb.command -relief sunken -textvariable commandName

    pack .extProg.eb.instruct .extProg.eb.resName .extProg.eb.cmdInstruct .extProg.eb.command -padx 5 -fill x
    pack .extProg.eb -side top -anchor w -fill x

    #checkbox
    frame .extProg.cbutton
    checkbutton .extProg.cbutton.cb -text "Include selection information" \
	    -variable incSelInfo -onvalue 1 -offvalue 0
    pack .extProg.cbutton.cb
    pack .extProg.cbutton -side top -anchor w

    # buttons
    frame .extProg.do
    button .extProg.do.doIt -text "Filter" \
	-command { plv_extProg $commandName $extFrom $resMeshName $incSelInfo}

    packchildren .extProg.do -side top
    pack .extProg.do -side top -anchor w

    wm title .extProg "External Program"
    bind .extProg <KeyPress-Return> {.extProg.do.doIt invoke}
    window_Register .extProg
}


