######################################################################
#
# Global registration UI
#
######################################################################


proc globalRegistrationDialog {} {
    if {[window_Activate .globalReg]} { return }

    plv_globalreg init_import

    # create UI window
    set gr [toplevel .globalReg]
    wm title $gr "Global registration"
    window_Register $gr

    frame $gr.file -relief groove -border 2
    label $gr.file.l -text "File options" -anchor w
    frame $gr.file.b
    button $gr.file.b.force_reload -text "Reload" \
	-command "plv_globalreg re_import"
    button $gr.file.b.delete -text "Delete..." \
	-command globalRegDeleteDialog
    packchildren $gr.file.b -side left -fill x -expand true
    packchildren $gr.file -side top -fill x -expand true

    frame $gr.opts -relief groove -border 2
    label $gr.opts.l -text "Render options" -anchor w
    frame $gr.opts.b
    button $gr.opts.b.showpartners -text "Show only partners" \
	-command {hideRegistrationNonPartners $theMesh}
    button $gr.opts.b.showgroup -text "Show only group" \
	-command {hideRegistrationNonPartners $theMesh transitive}
    packchildren $gr.opts.b -side left -fill x -expand true
    frame $gr.opts.c
    label $gr.opts.c.colorL -text "Color:"
    tk_optionMenu $gr.opts.c.color \
	theColorMode gray false registration
    packchildren $gr.opts.c -side left
    packchildren $gr.opts -side top -fill x -expand true

    frame $gr.show -relief groove -border 2
    label $gr.show.l -text "Dump registered pairs" -anchor w
    frame $gr.show.pairwise
    button $gr.show.pairwise.point_point \
	-text "Pairwise (point-point)" \
	-command {
	    badAlignDialog [plv_globalreg dumpallpairs 0 [.globalReg.show.thresh.entry get]]
	}
    button $gr.show.pairwise.point_plane \
	-text "Pairwise (point-plane)" \
	-command {
	    badAlignDialog [plv_globalreg dumpallpairs 1 [.globalReg.show.thresh.entry get]]
	}
    packchildren $gr.show.pairwise -side left -fill x -expand true
    frame $gr.show.dump
    button $gr.show.dump.globalreg -text "Globalreg Error" \
	-command {
	    badAlignDialog [plv_globalreg dumpallpairs 2 [.globalReg.show.thresh.entry get]]
	}
    button $gr.show.dump.partners -text "Partners" \
	-command {dumpRegistrationPartners $theMesh}
    packchildren $gr.show.dump -side left -fill x -expand true

#     frame $gr.show.select
#     button $gr.show.select.gr -text "Worst global to ICP" \
# 	-command { puts "Howdy" }
#     button $gr.show.select.pw -text "Worst pairwise to ICP" \
# 	-command {
# 	    puts "Doody"
# 	    ICPdialog
# 	}
#     packchildren $gr.show.select -side left -fill x -expand true

    frame $gr.show.thresh
    label $gr.show.thresh.l -text "Show offenders threshold (mm):"
    entry  $gr.show.thresh.entry \
	  -relief sunken -width 6
    $gr.show.thresh.entry insert end 5
    packchildren $gr.show.thresh -side left

    frame $gr.show.npairs
    label $gr.show.npairs.l -text "Pointpair counts"
    button $gr.show.npairs.curr -text "Current" \
	-command {plv_globalreg point_pair_count $theMesh *}
    button $gr.show.npairs.icp  -text "ICP dlg" \
	-command {plv_globalreg point_pair_count \
		      [globalset regICPFrom] [globalset regICPTo]}
    packchildren $gr.show.npairs -side left -fill x -expand true

    set p [frame $gr.show.part]
    button $p.list -text "List" -command "listFewPartners $p.val"
    label $p.l1 -text "meshes with fewer than"
    entry $p.val -width 3
    $p.val insert end 3
    label $p.l2 -text "partners"
    packchildren $p -side left

    button $gr.show.groups -text "List connected subgroups" \
	-command listGroups

    packchildren $gr.show -side top -fill x -expand true

    frame $gr.auto -relief groove -border 2

    set p [frame $gr.auto.choices]
    label $p.lf -text "Auto add pairs: from" -anchor w
    tk_optionMenu $gr.auto.choices.from autoFrom visible current
    label $p.lt -text "to" -anchor w
    tk_optionMenu $p.to autoTo visible all
    packchildren $p -side left -fill x -expand 1

    set f [frame $gr.auto.errorThresh]
    label $f.l -text "Error threshold (mm):"
    entry  $f.entry -relief sunken -width 6
    $f.entry insert end 5
    packchildren $f -side left -pady 2

    set f [frame $gr.auto.nPairs]
    label $f.l -text "Target number of pairs to calculate:"
    entry $f.entry -relief sunken -width 6
    $f.entry insert end 2000
    packchildren $f -side left
    checkbutton $gr.auto.preserve \
	-text "Preserve existing mesh pairs (don't recalculate)" \
	-variable preserveExistingInAutoAlign -anchor w

    frame $gr.auto.b
    button $gr.auto.b.go \
	-text "Go" \
	-command {
	    scz_auto_register $autoFrom $autoTo $theMesh \
		[.globalReg.auto.errorThresh.entry get] \
		[.globalReg.auto.nPairs.entry get] \
		$preserveExistingInAutoAlign
	}

    packchildren $gr.auto.b -side top -anchor c

    packchildren $gr.auto -side top -anchor w -fill x

    frame $gr.reg -relief groove -border 2

    label $gr.reg.l -text "Register:" -anchor w

    frame $gr.reg.b
    button $gr.reg.b.register -text "All<->All" \
	-command {doGlobalRegister}
    button $gr.reg.b.register1 -text "Selected->all" \
	-command {doGlobalRegister $theMesh}
    button $gr.reg.b.register12 -text "ICP 1->2" \
	-command {if {[winfo exists .regICP]} {
	    if {[_reg_check2meshes $regICPFrom $regICPTo]} {
		doGlobalRegister $regICPFrom $regICPTo
	    }
	} else {
	    tk_messageBox -title "scanalyze" -icon error \
		-parent .globalReg \
		-message "ICP dialog must be visible."
	}}
    packchildren $gr.reg.b -side left -fill x -expand 1

    frame $gr.reg.convTol
    label $gr.reg.convTol.l -text "Convergence Tolerance:"
    entry  $gr.reg.convTol.entry -relief sunken -width 6
    $gr.reg.convTol.entry insert end .01
    packchildren $gr.reg.convTol -side left -pady 2

    packchildren $gr.reg -side top -fill x

    packchildren $gr -side top -fill x -expand true \
	-pady 3 -padx 3
}


proc doGlobalRegister {args} {
    eval plv_globalreg register [.globalReg.reg.convTol.entry get] $args
    redraw 1
    update idletasks
}


proc hideRegistrationNonPartners {mesh {trans ""}} {
    if {$trans == ""} {
	set vis [plv_globalreg listpairsfor $mesh]
    } else {
	set vis [plv_globalreg listpairsfor $mesh transitive]
    }

    eval showOnlyMesh $vis $mesh
}


proc dumpRegistrationPartners {mesh} {
    set partner [plv_globalreg listpairsfor $mesh]
    set trans [plv_globalreg listpairsfor $mesh transitive]

    puts "$mesh is registered to:"
    puts "directly:     $partner"
    puts "transitively: $trans"
}


proc listFewPartners {widget} {
    set min [$widget get]

    foreach mesh [lsort -dictionary [getVisibleMeshes]] {
	set part [plv_globalreg getpaircount $mesh]
	if {$part < $min} {
	    puts "$mesh: $part partners"
	}
    }
}


proc listGroups {} {
    plv_globalreg groupstatus

    foreach mesh [lsort -dictionary [getVisibleMeshes]] {
	if {![plv_globalreg getpaircount $mesh]} {
	    lappend missing $mesh
	}
    }

    if {[info exists missing]} {
	puts "\nnot registered at all ([llength $missing]): $missing"
    }
}



######################################################################
#
# Drag registration UI
#
######################################################################


proc dragRegistrationDialog {} {
    global dragMeshesLeft
    set dragMeshesLeft [getVisibleMeshes]

    if {[llength $dragMeshesLeft] < 2} {
	tk_dialog .err "scanalyze" \
	    "You must have at least two meshes visible\
             to perform drag registration." "" 0 Ok
	return
    }

    set dr [toplevel .dragreg]
    wm title $dr "Drag registration"
    window_Register $dr

    global meshVisible
    foreach mesh $dragMeshesLeft {
	changeVis $mesh 0
    }

    frame $dr.cur
    label $dr.cur.curlab -text "Current mesh:"
    label $dr.cur.current -textvariable theMesh
    pack $dr.cur.curlab $dr.cur.current -side left
    pack $dr.cur -side top

    label $dr.instruct \
	-text "Drag mesh to correct position, then click Next."
    pack $dr.instruct -side top

    frame $dr.method
    radiobutton $dr.method.ptPt    -text "Point-to-point"\
	-variable dragRegMethod -value horn
    radiobutton $dr.method.ptPlane -text "Point-to-plane"\
	-variable dragRegMethod -value chen
    pack $dr.method.ptPt $dr.method.ptPlane -side left

    button $dr.next -text Next -command dragRegNext
    pack $dr.method $dr.next -side top -fill x -expand true

    frame $dr.left
    label $dr.left.leftlab -text "Remaining meshes:"
    label $dr.left.remain -textvariable dragMeshesLeft
    pack $dr.left.leftlab $dr.left.remain -side left
    pack $dr.left -side top

    bind $dr <Destroy> "+destroyDragRegUI %W"

    selectScan ""
    globalset theMover mesh
    dragRegNext
}


proc dragRegNext {} {
    global theMesh
    global dragMeshesLeft
    global meshVisible

    # read z-buffer to get correspondence points, invoke algorithm
    if {0 != [string compare $theMesh ""]} {
	if {[llength [getVisibleMeshes]] > 1} {
	    DragRegister $theMesh [globalset dragRegMethod]
	}
    }

    # on to next mesh:
    # move first element of dragMeshesLeft to theMesh
    selectScan [lindex $dragMeshesLeft 0]
    set dragMeshesLeft [lrange $dragMeshesLeft 1 end]

    # and if there IS a next mesh, make it visible
    if {0 == [string compare $theMesh ""]} {
	# done!
	destroy .dragreg
    } else {
	# make next mesh visible
	changeVis $theMesh 1
    }
}


proc destroyDragRegUI {window} {
    if {0 != [string compare $window .dragreg]} {
	return
    }

    globalset theMover viewer
    global dragMeshesLeft
    unset dragMeshesLeft
}


######################################################################
#
# Correspondence registration UI
#
######################################################################


proc renderParamsChanged {} {
    global correspRegWindows
    if {[array exists correspRegWindows]} {
	refreshCorrespRegWindows *
    }
}


proc correspRegistrationDialog {} {
    global meshVisible
    global uniqueInt
    global toglPane

    if {[array size meshVisible] < 2} {
	tk_dialog .err "scanalyze" \
	    "You must have at least two meshes loaded\
             to perform registration." "" 0 Ok
	return
    }

    toplevel .reg

    set which [frame .reg.which]
    set meshFrom [eval tk_optionMenu $which.mFrom regCorrespFrom bogus]
    set meshTo [eval tk_optionMenu $which.mTo regCorrespTo bogus]
    $which.mFrom config -pady 4
    $which.mTo config -pady 4

    set winid 1
    foreach meshMenu "$meshFrom $meshTo" {
	listRegMeshes $meshMenu
	bindCorrespRegMeshSelector $meshMenu $uniqueInt

	set winId$winid $uniqueInt
	incr winid
	incr uniqueInt

	bind $meshMenu <Shift-ButtonRelease> {
	    set chosenMesh [eval %W entrycget active -value]
	    createAlignmentView $uniqueInt $chosenMesh ""
	    incr uniqueInt
	    tkMenuUnpost {}
	    break
	}
    }

    label $which.lFrom -text Align
    button $which.swap -text "<->" -padx 3 \
	-command "swap regCorrespFrom regCorrespTo"
    button $which.sync -text "Sync" \
	-command {
	    refreshCorrespMeshList $meshFrom $winId1 $meshTo $winId2
	    syncRegMeshes $meshFrom $meshTo
	} -pady 1
    checkbutton $which.vis -text "Only visible" \
	-variable correspRegHideInvisible \
	-command "refreshCorrespMeshList $meshFrom $winId1 $meshTo $winId2"
    button $which.help -text "Help..." \
	-command correspRegHelp -pady 1

    pack $which.lFrom $which.mFrom $which.swap $which.mTo -side left
    pack $which.help $which.vis $which.sync -side right

    frame .reg.lines
    listbox .reg.lines.list -height 6 -borderwidth 2\
              -yscrollcommand ".reg.lines.scroll set"
    scrollbar .reg.lines.scroll -command ".reg.lines.list yview"
    pack .reg.lines.list -side left -fill both -expand true
    pack .reg.lines.scroll -side right -fill y

    # I want to just add some script to the existing Listbox binding,
    # but this doesn't seem to be possible given the Tcl difference
    # between substitutions inside "" and {} -- bind needs "" to work
    # with %W, but the rest of the script needs {} to work with the
    # temporary variables.  ARGH.

    bind .reg.lines.list <ButtonRelease-1> {
	set oldSel [extractMeshesInCorrespondence .reg.lines.list]
#	eval [bind Listbox <ButtonRelease-1>]
	tkCancelRepeat
	%W activate @%x,%y
	set newSel [extractMeshesInCorrespondence .reg.lines.list]

        refreshMeshesInCorrespondences [concat $oldSel $newSel]
    }

#    set listboxScript [bind Listbox <ButtonRelease-1>]
#    bind .reg.lines.list <ButtonRelease-1> "
#	set oldSel [extractMeshesInCorrespondence %W]
#        $listboxScript
#        set newSel [extractMeshesInCorrespondence %W]

#        refreshMeshesInCorrespondences [concat $oldSel $newSel]
#    "

    frame .reg.do
    button .reg.do.reg -text "Register meshes:" \
	-command {
	    plv_registerCorresp .regLayout.togl $correspRegMode \
		$regCorrespFrom $regCorrespTo
	    redraw 1
	    .regLayout.togl render
	} -pady 0
    globalset correspRegMode from2to
    radiobutton .reg.do.from2to -text "From to To" \
	-variable correspRegMode -value from2to
    radiobutton .reg.do.from2all -text "From to All" \
	-variable correspRegMode -value from2all
    radiobutton .reg.do.all2all -text "All to all" \
	-variable correspRegMode -value all2all

    pack .reg.do.reg .reg.do.from2to .reg.do.from2all .reg.do.all2all \
	-side left -fill x

    frame .reg.sel
    button .reg.sel.delete -text "Delete selected" \
	-command {
	    if {[string compare [.reg.lines.list cursel] ""] != 0} {
		DeleteRegCorrespondence .regLayout.togl \
		    [.reg.lines.list get active]
		refreshMeshesInCorrespondence \
		    [extractMeshesInCorrespondence .reg.lines.list]
		.reg.lines.list delete active
	    }
	} -pady 0

    button .reg.sel.deselect -text "Deselect" \
	-command { deselectSelectedCorrespondence } -pady 0
    checkbutton .reg.sel.colorPts -text "Color points" \
	-variable correspRegColorPoints \
	-command { plv_correspRegParms .regLayout.togl \
		       colorpoints $correspRegColorPoints
	    refreshCorrespRegWindows * }

    pack .reg.sel.delete .reg.sel.deselect .reg.sel.colorPts \
	-side left -anchor w -padx 4

    text .reg.halfDone -wrap none -height 1 -exportselection false \
	-foreground ForestGreen
    .reg.halfDone config -state disabled

    pack .reg.which -side top -fill x
    pack .reg.lines -side top -anchor w -fill both -expand true
    pack .reg.sel -side top -anchor w -fill x
    pack .reg.do -side top -anchor w -fill x
    pack .reg.halfDone -side right -anchor e

    wm title .reg "Interactive mesh registration -- correspondences"
    wm geometry .reg 500x300+0+0
    window_Register .reg

    createAlignmentOverview 500x300+524+0

    createAlignmentView $winId1 "" 500x450+0+372
    createAlignmentView $winId2 "" 500x450+524+372

    bind .reg <Unmap>    { showCorrespRegWindows 0 }
    bind .reg <Map>      { showCorrespRegWindows 1 }
    wm protocol .reg WM_DELETE_WINDOW destroyCorrespRegUI

    syncRegMeshes $meshFrom $meshTo 0 0
}


proc refreshCorrespMeshList {meshFrom winId1 meshTo winId2} {
    global correspRegHideInvisible
    listRegMeshes [list $meshFrom $meshTo] $correspRegHideInvisible
    bindCorrespRegMeshSelector $meshFrom $winId1
    bindCorrespRegMeshSelector $meshTo   $winId2
}


proc bindCorrespRegMeshSelector {meshlist windowid} {
    set len [$meshlist index end]
    for {set i 3} {$i <= $len} {incr i} {
	set mesh [$meshlist entrycget $i -label]
	$meshlist entryconfigure $i \
	    -command "resetAlignmentView $windowid $mesh"
    }
}


proc invokeActiveMesh {meshlist} {
    menuInvokeByName $meshlist [globalset theMesh]
}


proc activateMeshFromVariable {var} {
    globalset theMesh [globalset $var]
}


proc listRegMeshes {meshlistlist {onlyvis 0}} {
    set names [getSortedMeshList $onlyvis]

    foreach meshlist $meshlistlist {
	set var [lindex [$meshlist entryconfig 3 -variable] 4]
	$meshlist delete 0 end

	$meshlist add command -label "Select active mesh" \
	    -command "invokeActiveMesh $meshlist"
	$meshlist add command -label "Activate selected mesh" \
	    -command "activateMeshFromVariable $var"
	$meshlist add separator

	set count 3
	set max 35

	foreach name $names {
	    set color [GetMeshFalseColor $name]
	    if {[expr $count % $max] == 0} {
		set cb 1
	    } else {
		set cb 0
	    }
	    incr count

	    $meshlist add radiobutton -label $name \
		-variable $var -value $name \
		-foreground $color -activebackground $color \
		-columnbreak $cb
	}

	if {[llength $names] == 0} {
	    $meshlist add radiobutton -label "No eligible meshes!" \
		-variable $var -state disabled
	}
    }
}


proc syncRegMeshes {meshFrom meshTo {relist 0} {warn 1}} {
    if {$relist != 0} {
	# refresh mesh list
	listRegMeshes "$meshFrom $meshTo" [expr $relist - 1]
    }

    # now try to select the 2 visible meshes
    set vis [getVisibleMeshes]
    if {[llength $vis] != 2} {
	if {$warn} {
	    tk_messageBox -type ok -icon error \
		-message "Must have exactly 2 meshes visible to sync"\
		-parent $meshFrom
	} else {
	    # instead of failing, we're supposed to silently do our best.
	    # so select theMesh as first mesh, and anything else as second
	    set ind [menuInvokeByName $meshFrom [globalset theMesh]]

	    if {$ind == 3} {
		$meshTo invoke 4
	    } else {
		$meshTo invoke 3
	    }
	}
	return 0
    }

    # if either is the current mesh, make it the "from" mesh
    if {[globalset theMesh] == [lindex $vis 1]} {
	set vis [list [lindex $vis 1] [lindex $vis 0]]
    }

    set loop {{meshFrom 0} {meshTo 1}}
    foreach entry $loop {
	set meshMenu [set [lindex $entry 0]]
	set ind [lindex $entry 1]

	set name [lindex $vis $ind]
	menuInvokeByName $meshMenu $name
    }

    return 1
}


proc correspRegHelp {} {
    toplevel .help
    text .help.instruct -wrap word -exportselection false
    .help.instruct insert end "Choose two meshes from the dropdown menus.\
               If you want more than two mesh views, hold shift as you\
               select a mesh name.\n\n\
               Left-click a point in each mesh view to begin a correspondence;\
               middle button completes the correspondence and right button\
               cancels it.\n\n\
               Selecting a correspondence from the listbox will highlight it;\
               you can now delete it, or modify or delete individual points\
               by clicking in individual mesh views.\n\n
               To manipulate the meshes with a trackball, hold down the\
               Alt key."
    .help.instruct config -state disabled
    pack .help.instruct -fill both -expand true
}


proc extractMeshesInCorrespondence {widget {corresp ""}} {
    # if we're passed a widget and no correspondence, extract correspondence
    if {0 == [string compare $corresp ""]} {

	# if there is one
	if {0 == [string compare "" [$widget cursel]]} {
	    return ""
	}

	set corresp [$widget get active]
    }

    set meshes ""
    for {set i 1} {$i < [llength $corresp]} {set i [expr $i + 2]} {
	set mesh [lindex $corresp $i]
	set mesh [string range $mesh 0 [expr [string first : $mesh] - 1]]

	set meshes [concat $meshes $mesh]
    }

    return $meshes
}


proc refreshMeshesInCorrespondences {meshes} {
    foreach mesh $meshes {
	set meshNames($mesh) 1
    }

    # use names as keys in associative array, then extract names, to
    # discard duplicates
    foreach mesh [array names meshNames] {
	if {0 != [string compare $mesh ""]} {
	    refreshCorrespRegWindows $mesh
	}
    }
}


proc deselectSelectedCorrespondence {} {
    set meshes [getListboxSelection .reg.lines.list]
    .reg.lines.list selection clear 0 end
    if {$meshes != ""} {
	refreshMeshesInCorrespondences \
	    [extractMeshesInCorrespondence "" $meshes]
    }
}


proc rebuildSelectedCorrespondenceString {} {
    set index [.reg.lines.list cursel]
    if {$index == ""} {
	return
    }

    set sel [.reg.lines.list get $index]
    set id [getCorrespondenceIndex $sel]
    set sel [GetCorrespondenceInfo .regLayout.togl $id]

    .reg.lines.list insert $index $sel
    .reg.lines.list delete [expr $index + 1]
    .reg.lines.list selection clear 0 end
    .reg.lines.list selection set $index
}


proc refreshCorrespRegWindows {mesh} {
    global correspRegWindows

    foreach window [array names correspRegWindows] {
	set thisMesh $correspRegWindows($window)

	if {$mesh == $thisMesh || $mesh == "*"} {
	    $window.togl render
	}
    }

    if {$mesh == "*"} {
	.regLayout.togl render
    }
}


proc showCorrespRegWindows {bShow} {
    global correspRegWindows

    foreach window [concat .regLayout [array names correspRegWindows]] {
	if {[string compare $bShow 0] == 0} {
	    wm withdraw $window
	} else {
	    wm deiconify $window
	}
    }
}


proc destroyCorrespRegUI {} {

    # delete dependent windows

    global correspRegWindows
    foreach window [array names correspRegWindows] {
	destroy $window
    }
    unset correspRegWindows

    # then delete master windows
    wm protocol .reg WM_DELETE_WINDOW {}
    wm protocol .regLayout WM_DELETE_WINDOW {}
    destroy .reg
    destroy .regLayout

    # don't let user accidentally destroy alignment
    globalset theMover viewer
}


proc createAlignmentOverview {geometry} {
    toplevel .regLayout
    togl .regLayout.togl \
        -rgba true -double true -depth true -accum true -overlay false\
	-ident .regLayout.togl

    pack .regLayout.togl -fill both -expand 1
    wm title .regLayout "Interactive mesh registration -- layout"
    wm geometry .regLayout $geometry
    window_Register .regLayout

    bindToglToAlignmentOverview .regLayout.togl
    wm protocol .regLayout WM_DELETE_WINDOW destroyCorrespRegUI
}


proc createAlignmentView {viewName meshName geometry} {
    set view [toplevel .regMesh$viewName]
    togl $view.togl \
        -rgba true -double true -depth true -accum true \
	-ident $view.togl -time 30
    resetAlignmentView $viewName $meshName
    pack $view.togl -fill both -expand 1
    if {$geometry != ""} {
	wm geometry $view $geometry
    }

    bind $view.togl <Alt-ButtonPress-1> \
	"RegUIMouse $view.togl 1 %x %y %t start"
    bind $view.togl <Alt-B1-Motion> \
	"RegUIMouse $view.togl 0 %x %y %t"
    bind $view.togl <Alt-ButtonRelease-1> \
	"RegUIMouse $view.togl 1 %x %y %t stop"
    bind $view.togl <Alt-ButtonPress-2> \
	"RegUIMouse $view.togl 2 %x %y %t start"
    bind $view.togl <Alt-B2-Motion> \
	"RegUIMouse $view.togl 0 %x %y %t"
    bind $view.togl <Alt-ButtonRelease-2> \
	"RegUIMouse $view.togl 2 %x %y %t stop"

    global cursor
    $view config -cursor "diamond_cross$cursor(Fore)$cursor(Back)"
    bind $view <KeyPress-Alt_L>    \
	"%W.togl config -cursor \"$cursor(CurvedHand)$cursor(Fore)\""
    bind $view <KeyRelease-Alt_L>  \
	"%W.togl config -cursor \"diamond_cross$cursor(Fore)$cursor(Back)\""
    # enter bindings, in case you cross window boundary with Alt down
    bind $view <Enter> \
	"%W config -cursor \"diamond_cross$cursor(Fore)$cursor(Back)\""
    bind $view <Alt-Enter> \
	"%W config -cursor \"$cursor(CurvedHand)$cursor(Fore)\""

    bind $view.togl <ButtonPress-1> \
	"clickCorrespondence $view.togl .regLayout.togl %x %y"
    bind $view.togl <ButtonPress-2> \
	"completeCorrespondence .regLayout.togl"
    bind $view.togl <ButtonPress-3> \
	"completeCorrespondence .regLayout.togl delete"

    bind $view <Destroy> {+alignmentViewWentAway %W}
}


proc alignmentViewWentAway {widget} {
    # ignore duplicates -- only the toplevel counts
    if {0 != [string last . $widget]} { return }

    global correspRegWindows
    unset correspRegWindows($widget)
}


proc resetAlignmentView {viewName meshName} {
    global correspRegWindows

    set view ".regMesh$viewName"

    bindToglToAlignmentView $view.togl $meshName
    set correspRegWindows($view) $meshName
    wm title $view "$meshName -- registration (Alt to trackball)"

    bind $view <KeyPress-period> "setMeshResolution $meshName higher"
    bind $view <KeyPress-comma>  "setMeshResolution $meshName lower"
}


proc clickCorrespondence {togl toglOV x y} {
    set mesh [AddPartialRegCorrespondence $togl $toglOV $x $y]

    if {[string compare $mesh ""] == 0} {
	beep
	puts "clicked on nothing"
	return
    }

    .reg.halfDone config -state normal
    set meshPos [.reg.halfDone search $mesh 1.0]
    if {$meshPos == ""} {
	# otherwise, it's already in the list
	.reg.halfDone insert end "$mesh "
	.reg.halfDone xview moveto 1
    } else {
	set total [.reg.halfDone index "1.0 lineend"]
	set total [string range $total 2 end]
	set meshPos [string range $meshPos 2 end]
	.reg.halfDone xview moveto [expr $meshPos.0 / $total]
    }
    .reg.halfDone config -state disabled
}


proc getCorrespondenceIndex {corresp} {
    set index [lindex $corresp 0]
    set index [string range $index \
		   [expr [string first \[ $index] + 1] \
		   [expr [string last  \] $index] - 1]]

    return $index
}


proc completeCorrespondence {toglOV {delete ""}} {
    # deselect any selection
    deselectSelectedCorrespondence

    # update C++ data structures
    set corresp [ConfirmRegCorrespondence $toglOV $delete]

    set index [getCorrespondenceIndex $corresp]

    # if it wasn't cancelled
    if {$index != 0} {

	# and list it in listbox
    	.reg.lines.list insert end $corresp
    }

    # update the views that are affected
    update
    refreshMeshesInCorrespondences \
	[extractMeshesInCorrespondence "" $corresp]

    .reg.halfDone config -state normal
    .reg.halfDone delete 1.0 end
    .reg.halfDone config -state disabled
}


######################################################################
#
# ICP Degistration UI
#
######################################################################


proc _reg_check2meshes { mfrom mto } {

    if {$mfrom == $mto} {
	tk_messageBox -type ok -icon error -parent .regICP\
		-message "Please select two meshes"
	return 0
    }

    global meshVisible
    if {[lsearch [array names meshVisible] $mfrom] < 0 || \
            [lsearch [array names meshVisible] $mto] < 0 || \
            !$meshVisible($mfrom) || !$meshVisible($mto)} {
        set warn "Warning: attempting to register mesh that is not"
        set warn "$warn visible; sync first?"
        set res [tk_messageBox -message $warn -parent .regICP \
                -type yesnocancel -default yes -icon warning]
        if {$res == "cancel"} {
            return 0
        } elseif {$res == "yes"} {
            .regICP.which.sync invoke
            if {[lsearch [array names meshVisible] $mfrom] < 0 || \
                    [lsearch [array names meshVisible] $mto] < 0 || \
                    !$meshVisible($mfrom) || !$meshVisible($mto)} {
                return 0
            }
        }
    }

    return 1
}


proc doICP { samp normsamp n culling_percentage no_bdry opt_method \
		 mfrom mto thr_kind thr_val save_global gr_max_pairs qual} {

    if {![_reg_check2meshes $mfrom $mto]} {
	return
    }

    .regICP config -cursor watch
    cursor watch

    if {$thr_kind == "rel"} {
	set thr_val [expr $thr_val / 100.0]
    }

    set err [plv_icpregister $samp $normsamp $n $culling_percentage $no_bdry \
		 $opt_method $mfrom $mto $thr_kind $thr_val $save_global \
		 $gr_max_pairs $qual]

    redraw 1
    .regICP config -cursor ""
    .regICP.do.lastErr config -text $err
    .regICP.gr_frm.qual config -state active
    cursor restore
}

proc updateFromAndToMeshNames {} {

    if {![window_Activate .regICP]} { return }
    # do nothing if the ICP window is closed

    set names [getMeshList]
    if {[llength $names] == 0} { return }
    # i.e. do nothing if all the meshes have been deleted
    syncRegMeshes .regICP.which.mFrom.menu .regICP.which.mTo.menu 1 0
    # calls syncRegMeshes to refresh the menus so that the current
    # selections for the from and to mesh are valid choices


}

proc ICPdialog {} {

    global regICPFrom regICPTo

    set names [getMeshList]
    if {[expr [llength $names] < 2]} {
	puts "You need at least two meshes to register"
	return
    }

    if {[window_Activate .regICP]} { return }
    toplevel .regICP

    # Instruction message
    message .regICP.instruct -aspect 1000 \
	    -text "Choose two meshes in the menus"\
	    -justify left

    pack .regICP.instruct -side top -anchor w

    # option menu for mesh names
    set which [frame .regICP.which]
    set meshFrom [eval tk_optionMenu $which.mFrom regICPFrom  bogus]
    set meshTo [eval tk_optionMenu $which.mTo regICPTo bogus]
    syncRegMeshes $meshFrom $meshTo 1 0
    $meshFrom config -postcommand "refreshICPRegMeshes $meshFrom"
    $meshTo config -postcommand "refreshICPRegMeshes $meshTo"

    label $which.lFrom -text Align -padx 3
    button $which.swap -text "<->" -padx 3 \
	-command "swap regICPFrom regICPTo"
    button $which.sync -text "Sync" \
	-command "refreshICPRegMeshes {$meshFrom $meshTo} sync"
    checkbutton $which.vis -text "Only visible" \
	-variable icpRegListsVisOnly -onvalue 1 -offvalue 0

    pack $which.lFrom $which.mFrom $which.swap $which.mTo -side left
    pack $which.sync $which.vis -side right
    pack $which -side top -anchor w

    # scales
    set scl_frm [frame .regICP.scl_frm \
	    -borderwidth 4 -relief ridge]
    # scale for the sampling rate
    set lbl_wdth 13
    set samp [frame $scl_frm.samp]
    label  $samp.scl_label -text "Sampling rate" \
	    -width $lbl_wdth -anchor w -padx 3
    scale  $samp.scl \
	    -from .01 -to 1 -resolution .01 \
	    -variable regSample \
	    -orient horizontal \
	    -length 150
    $samp.scl set .10
    pack $samp -side top -expand true -fill x
    pack $samp.scl_label -side left -anchor sw
    pack $samp.scl -side right -expand true -fill x

    # scale for the number of iterations
    set iter [frame $scl_frm.iter]
    label  $iter.scl_label -text "Iterations" \
	    -width $lbl_wdth -anchor w -padx 3
    scale  $iter.scl \
	    -from 0 -to 20 \
	    -variable regIterations \
	    -orient horizontal
    $iter.scl set 3
    pack $iter -side top -expand true -fill x
    pack $iter.scl_label -side left -anchor sw
    pack $iter.scl -side right -expand true -fill x

    # scale for the max relative edge length
    set culling [frame $scl_frm.culling]
    label  $culling.scl_label -text "Cull Percentage" \
	    -width $lbl_wdth -anchor w -padx 3
    scale  $culling.scl \
	    -from 0 -to 99 \
	    -variable cullingPercentage \
	    -orient horizontal
    $culling.scl set 3
    pack $culling -side top -expand true -fill x
    pack $culling.scl_label -side left -anchor sw
    pack $culling.scl -side right -expand true -fill x

    # make a frame for thresholds
    set thr_frm [frame .regICP.scl_frm.thr_frm]
    # radio buttons
    label $thr_frm.l -text "Threshold:"
    set thr_radiob [frame $thr_frm.thr_radiob]
    radiobutton $thr_radiob.abs -variable thresh_kind \
	    -value "abs" \
	    -text "Absolute (mm)"
    radiobutton $thr_radiob.rel -variable thresh_kind \
	    -value "rel" \
	    -text "Relative (% of bbox)"
    $thr_radiob.abs select
    packchildren $thr_radiob -side bottom -anchor w
    # entry field
    entry  $thr_frm.entry \
	    -textvariable dist_threshold_val \
	    -relief sunken -width 6
    globalset dist_threshold_val 5.0
    packchildren $thr_frm -side left -expand true -fill x
    pack $thr_frm -side top -expand true -fill x

    pack $scl_frm -side top -expand true -fill x

    frame .regICP.opt
    # checkbutton for allowing mesh boundary targets
    checkbutton .regICP.opt.bdry -variable no_bndr_trgt \
	    -text "Avoid boundary"
    checkbutton .regICP.opt.normspace -variable norm_space_samp \
	    -text "Normal-space sampling"
    .regICP.opt.bdry select
    checkbutton .regICP.opt.lines -text "Show lines"\
	-variable regicpShowLines -command {showIcpLines $regicpShowLines}
    #.regICP.opt.lines select
    showIcpLines 0
    packchildren .regICP.opt -side left
    pack .regICP.opt -side top -anchor w

    # radiobuttons for choosing optimization method
    set rad_frm [frame .regICP.rad_frm \
	    -borderwidth 4 -relief ridge]
    radiobutton $rad_frm.point -variable opt_method \
	    -value "point" \
	    -text "Move points to points"
    radiobutton $rad_frm.plane -variable opt_method \
	    -value "plane" \
	    -text "Move points to planes"
    $rad_frm.plane select
    pack $rad_frm.point $rad_frm.plane -side top -anchor w
    pack $rad_frm -side top -expand true -fill x

    # global registration stuff
    set gr_frm [frame .regICP.gr_frm \
	    -borderwidth 4 -relief ridge]

    checkbutton $gr_frm.l1 \
	-text "Save data for globalreg, but no more than " \
	-variable saveICPForGlobal
    entry  $gr_frm.entry \
	    -textvariable gr_max_pairs \
	    -relief sunken -width 7
    globalset gr_max_pairs 200
    label  $gr_frm.l2 -text " pairs.   " \
	    -width 5 -anchor w -padx 3

    label $gr_frm.l3 -text "Quality:"
    set qualMenu [tk_optionMenu $gr_frm.qual regIcpQuality \
		      "0 - Unspecified" "1 - Bad" "2 - Fair" "3 - Good"]
    bindCommandToAllMenuItems $qualMenu {
	plv_icpreg_markquality $regICPFrom $regICPTo $regIcpQuality
    }
    globalset regIcpQuality 0

    packchildren $gr_frm -side left
    pack $gr_frm -side top -expand true -fill x

    # button for doing registration
    frame .regICP.do

    button .regICP.do.doIt -text "Register meshes" \
	-command {
	    doICP $regSample $norm_space_samp $regIterations $cullingPercentage \
		$no_bndr_trgt $opt_method $regICPFrom $regICPTo \
		$thresh_kind $dist_threshold_val $saveICPForGlobal \
		$gr_max_pairs $regIcpQuality
	}
    button .regICP.do.doIt100 -text "Register 1 round, 100%" \
	-command {
	    doICP 1 0 1 $cullingPercentage \
		$no_bndr_trgt $opt_method $regICPFrom $regICPTo \
		$thresh_kind $dist_threshold_val $saveICPForGlobal \
		$gr_max_pairs $regIcpQuality
	}
    button .regICP.do.histogram -text "Show histogram" \
	-command {
	    plv_error_histogram \
		$regSample $regIterations $cullingPercentage \
		$no_bndr_trgt $opt_method $regICPFrom $regICPTo \
		$thresh_kind $dist_threshold_val
	}

    label .regICP.do.lastErrl -text "Last err:"
    label .regICP.do.lastErr -text "(none)"

    packchildren .regICP.do -side left
    pack .regICP.do -side top -anchor w

    wm title .regICP "ICP registration"
    bind .regICP <Destroy> "+destroyICPDialog %W"
    bind .regICP <KeyPress-space> {.regICP.do.doIt invoke}
    window_Register .regICP

    trace variable regICPFrom w regIcpChangeMesh
    trace variable regICPTo   w regIcpChangeMesh
    regIcpChangeMesh ignore this arg
}


proc regIcpChangeMesh {var1 var2 op} {
    # NOTE: should we always set to 0(unknown) when they switch meshes, or
    # if those meshes are already aligned, should we set it to their old grade?
    globalset regIcpQuality 0
    .regICP.do.lastErr config -text "(none)"

    set reg [plv_globalreg pairstatus [globalset regICPFrom] \
		 [globalset regICPTo]]
    if {$reg} {
	.regICP.gr_frm.qual config -state active
    } else {
	.regICP.gr_frm.qual config -state disabled
    }
}


proc refreshICPRegMeshes {meshLists {sync 0}} {
    set invis [globalset icpRegListsVisOnly]

    if {$sync != "0"} {
	if {[llength $meshLists] != 2} {
	    tk_messageBox -message "internal error in arg count!"
	    return
	}
	eval syncRegMeshes $meshLists [expr 1 + $invis]
    } else {
	listRegMeshes $meshLists $invis
    }
}


proc destroyICPDialog {widget} {
    if {$widget != ".regICP"} {
	return
    }

    global regICPFrom
    global regICPTo
    trace vdelete regICPFrom w regIcpChangeMesh
    trace vdelete regICPTo   w regIcpChangeMesh

    showIcpLines 0
}


proc showIcpLines {show} {
    plv_showicplines $show
    redraw 1
}


proc globalRegDeleteDialog {} {
    set grd [toplevel .globalregdel]
    wm resizable $grd 0 0
    wm title $grd "Delete from globalreg"

    button $grd.all -text "Clear all registration pairs!" \
	-command "plv_globalreg reset"
    button $grd.current -text "Clear all pairs involving current mesh" \
	-command {plv_globalreg killpair [globalset theMesh] *}
    button $grd.these -text "Clear pair selected in ICP dialog" \
	-command {plv_globalreg killpair \
	    [globalset regICPFrom] [globalset regICPTo]}

    set f [frame $grd.f1]

    set fb [frame $f.fb]
    button $fb.b1 -text "Clear all" \
	-command { plv_globalreg deleteautopairs $auto_del_thrsh }
    button $fb.b2 \
	-text "Clear current mesh's" \
	-command {
	    plv_globalreg deleteautopairs $auto_del_thrsh $theMesh
	}
    packchildren $fb -side top -fill x -expand 1

    label $f.gt -text "auto pairs greater than"
    entry  $f.entry \
	    -textvariable auto_del_thrsh \
	    -relief sunken -width 6
    globalset auto_del_thrsh 1.0
    packchildren $f -side left -fill x -expand 1

    button $grd.cancel -text "Close" -command "destroy $grd"

    packchildren $grd -side top -fill x -expand 1
}

proc chooseICP {a b} {
    global regICPTo regICPFrom
    set regICPFrom $a
    set regICPTo $b
    ICPdialog
    showAllMeshes 0
    showMesh $a
    showMesh $b
}

proc badAlignDialog {str} {

    if {[winfo exists .badalign]} {
	destroy .badalign
    }

    set bad [toplevel .badalign]

    wm resizable $bad 0 0
    wm title $bad "Bad aligns"

    set l [split $str]
    for {set i 0} {$i < [llength $l]} {incr i 4} {
	if {[expr $i + 3] < [llength $l]} {
	    button $bad.$i \
		-text [join [lrange $l $i [expr $i + 3]]]\
		-command "chooseICP [lindex $l [expr $i + 0]] [lindex $l [expr $i + 1]]"
	}
    }
    packchildren $bad -side top -fill x -expand 1
}
