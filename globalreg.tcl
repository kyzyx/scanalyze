######################################################################
#
# Global registration UI
#
######################################################################


# in-progress notes:
# does anything from globalreg_old still deserve to survive?
# (delete dialog is main candidate)


#TODO: fully replace and get rid of old stuff (oldglobalreg.tcl)
source $env(SCANALYZE_DIR)/globalreg_old.tcl

proc New_globalRegistrationDialog {} {
    if {[window_Activate .globalReg]} { return }

    plv_globalreg init_import

    # create UI window
    set gr [toplevel .globalReg]
    wm title $gr "Global registration manager"
    window_Register $gr

    # the Selection pane
    #TODO: tight, minerr
    #and the others don't appear to work right (don't leave scan itself)
    set pane [createTogglePane $gr selection "Show selection commands"]
    set f [createOptionsPaneWithDoButton $pane select]
    radiobutton $f.regDirect -text "Scans directly aligned" \
	-variable __gr_select -value direct
    radiobutton $f.regTrans -text "Scans transitively aligned" \
	-variable __gr_select -value trans
    radiobutton $f.regTight -text "Scans tightly aligned" \
	-variable __gr_select -value tight -state disabled
    radiobutton $f.regNot -text "Scans not at all aligned" \
	-variable __gr_select -value not
    set err [frame $f.regError]
    radiobutton $err.reg -text "Scans with error above" \
	-variable __gr_select -value minerr -state disabled
    entry $err.dist -width 3 -textvariable __gr_select_minerr
    label $err.l -text "mm"
    packchildren $err -side left
    packchildren $f -side top -anchor w

    # the Browse pane
    set pane [createTogglePane $gr browse "Show browse commands"]
    set f [createOptionsPaneWithDoButton $pane browse]
    radiobutton $f.browseAgg -text "Browse summary" \
	-variable __gr_browse -value summary
    radiobutton $f.browseAll -text "Browse all pairs" \
	-variable __gr_browse -value all
    radiobutton $f.browseVis -text "Browse partners of visible meshes" \
	-variable __gr_browse -value visible
    radiobutton $f.browseCur -text "Browse parterns of current mesh" \
	-variable __gr_browse -value current
    radiobutton $f.showGroups -text "Text dump of connected subgroups" \
	-variable __gr_browse -value subgroups
    packchildren $f -side top -anchor w

    # the AutoAlign pane
    set pane [createTogglePane $gr autoalign \
		  "Show auto-pair-creation commands"]
    set f [createOptionsPaneWithDoButton $pane autoalign]

    set p [frame $f.choices]
    label $p.lf -text "Auto add pairs: from" -anchor w
    tk_optionMenu $p.from __gr_auto_opts(autoFrom) visible current
    label $p.lt -text "to" -anchor w
    tk_optionMenu $p.to __gr_auto_opts(autoTo) visible all
    packchildren $p -side left -fill x -expand 1

    set p [frame $f.errorThresh]
    label $p.l -text "Error threshold (mm):"
    entry  $p.entry -relief sunken -width 6 \
	    -textvariable __gr_auto_opts(thresh)
    packchildren $p -side left -pady 2

    set p [frame $f.nPairs]
    label $p.l -text "Target number of pairs to calculate:"
    entry $p.entry -relief sunken -width 6 \
	    -textvariable __gr_auto_opts(numpairs)
    packchildren $p -side left

    set p [frame $f.cache]
    checkbutton $p.cb1 -text "Size limit for scan cache:" \
	-variable __gr_auto_opts(useScanCache)
    entry $p.entry -relief sunken -width 6 \
	    -textvariable __gr_auto_opts(cachelimit)
    label $p.l -text "KB"
    checkbutton $p.cb2 -text "including loaded scans" \
	-variable __gr_auto_opts(includeLoadedScans)
    packchildren $p -side left

    checkbutton $f.preserve \
	-text "Preserve existing mesh pairs (don't recalculate)" \
	-variable __gr_auto_opts(preserveExisting) -anchor w
    checkbutton $f.edit \
	-text "Edit pairing graph before registering" \
	-variable __gr_auto_opts(editGraph) -anchor w
    packchildren $f -side top -anchor w

    # the GlobalAlign pane
    #TODO: lots -- make old things work again, write new things
    #(or at least disable buttons that can't work)
    set pane [createTogglePane $gr globalreg "Show global alignment commands"]
    set f [createOptionsPaneWithDoButton $pane globalalign]
    set l [frame $f.buttonColumns]
    set c [frame $l.colFrom]
    label $c.l -text "Move:"
    radiobutton $c.all -text "All scans" -variable __gr_align1 -value all
    radiobutton $c.vis -text "Vis scans" -variable __gr_align1 -value vis
    radiobutton $c.cur -text "Cur scan"  -variable __gr_align1 -value cur
    radiobutton $c.icp -text "ICP 1"     -variable __gr_align1 -value icp
    packchildren $c -side top -anchor w
    set c [frame $l.colTo]
    label $c.l -text "Align to:"
    radiobutton $c.all -text "All scans" -variable __gr_align1 -value all
    radiobutton $c.vis -text "Vis scans" -variable __gr_align1 -value vis
    radiobutton $c.cur -text "Cur scan"  -variable __gr_align1 -value cur
    radiobutton $c.icp -text "ICP 2"     -variable __gr_align1 -value icp
    packchildren $c -side top -anchor w
    packchildren $l -side left
    set l [frame $f.conv]
    label $l.l -text "Convergence tolerance:"
    entry $l.val -width 4 -textvariable __gr_align_tolerance
    # label $l.l2 -text "mm"  # what ARE the units on this?
    packchildren $l -side left
    packchildren $f -side top -anchor w

    # make panes visible
    packchildren $gr -side top -anchor w -fill x

    # and then the ones we want initially enabled
    # (or their variables could be made autosave)
    foreach b {selection browse autoalign globalreg} {
	$gr.tp_${b}.toggle invoke
    }

    globalset __gr_select direct
    globalset __gr_select_minerr 1.5
    globalset __gr_browse summary
	globalset __gr_auto_opts(thresh) 5
	globalset __gr_auto_opts(numpairs) 200
	globalset __gr_auto_opts(cachelimit) 0
	globalset __gr_auto_opts(editGraph) 1
	globalset __gr_auto_opts(preserveExisting) 1
    globalset __gr_align1 all
    globalset __gr_align2 all
    globalset __gr_align_tolerance 0.01
}


# action button helpers for globalreg dialog
proc globalRegAction_select {} {
    global theMesh

    set invert 0

    switch -exact -- [globalset  __gr_select] {
	direct { set vis [plv_globalreg listpairsfor $theMesh] }
	trans  { set vis [plv_globalreg listpairsfor $theMesh trans] }
	tight  { set vis [plv_globalreg listpairsfor $theMesh tight] }
	not    { set vis [plv_globalreg listpairsfor $theMesh trans];
	         set invert 1 }
	minerr { set vis [plv_globalreg listpairsfor $theMesh \
			      errorabove $gr_select_minerr]}
    }

    if {$invert} {
	eval showOnlyMesh $vis $theMesh
    } else {
	eval hideOnlyMesh $vis $theMesh
    }
}


proc globalRegAction_browse {} {
    switch [globalset __gr_browse] {
	current   { newAlignmentBrowser [globalset theMesh] }
	visible   { newAlignmentBrowser [getVisibleMeshes] }
	all       { newAlignmentBrowser [getMeshList] }
	summary   { AlignmentSummaryDialog }
	subgroups { globalreg_listGroupStatus }
    }
}


proc globalRegAction_autoalign {} {
    global __gr_auto_opts

    scz_auto_register \
	    $__gr_auto_opts(autoFrom) $__gr_auto_opts(autoTo) \
	    [globalset theMesh] \
	    $__gr_auto_opts(thresh) $__gr_auto_opts(numpairs) \
	    $__gr_auto_opts(preserveExisting) $__gr_auto_opts(useScanCache) \
	    $__gr_auto_opts(cachelimit) \
	    $__gr_auto_opts(includeLoadedScans) $__gr_auto_opts(editGraph)
}


proc globalRegAction_align {} {
    foreach i {1 2} {
	set scan [globalset __gr_align$i]

	switch {$scan} {
	    cur { set scan [globalset theMesh] }
	    vis { set scan [getVisibleMeshes] }
	    all { set scan [getMeshList] }

	    icp {
		if {![winfo exists .regICP]} {
		    tk_messageBox -title "scanalyze" -icon error \
			-parent .globalReg \
			-message "ICP dialog must be visible."
		    return
		}

		if {$i == 1} {
		    set scan [globalset regICPFrom]
		} else {
		    set scan [globalset regICPTo]
		}
	    }
	}

	set s$i $scan
    }

    plv_globalreg register [globalset __gr_align_tolerance] $s1 $s2
    #TODO: make plv_globalreg speak this language

    redraw 1
}


# and action helper helpers
proc globalreg_SelectPairsFor {mesh {trans ""}} {
    if {$trans == ""} {
	set vis [plv_globalreg listpairsfor $mesh]
    } else {
	set vis [plv_globalreg listpairsfor $mesh transitive]
    }

    eval showOnlyMesh $vis $mesh
}


proc globalreg_listGroupStatus {} {
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


# window layout creation helpers for globalreg dialog
proc createOptionsPaneWithDoButton {pane actionProc} {
    set f [frame $pane.opt]
    vertbutton $pane.go -text "DO IT" \
	-command globalRegAction_$actionProc

    pack $pane.opt -side left -anchor w -padx 8
    pack $pane.go -side right -fill y -expand 1 -anchor e

    return $f
}


proc createTogglePane {parent name label} {
    set pane [frame $parent.tp_$name -relief groove -border 2]
    checkbutton $pane.toggle -text $label -anchor w \
	-variable togglePane_${parent}_${name} \
	-command "togglePaneSetVisible $pane"
    set inner [frame $pane.inner]

    pack $pane.toggle -side top -fill x -expand 1 -anchor w
    return $inner
}


proc togglePaneSetVisible {pane} {
    set var [$pane.toggle config -variable]
    set var [lindex $var 4]

    if {[globalset $var]} {
	pack $pane.inner -after $pane.toggle -side top -fill x -expand 1
    } else {
	pack forget $pane.inner
    }
}
