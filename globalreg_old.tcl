# old global reg ui
proc globalRegistrationDialog {} {
    if {[window_Activate .globalReg]} { return }

    plv_globalreg init_import

    # create UI window
    set gr [toplevel .globalReg]
    wm title $gr "Global registration"
    window_Register $gr

    # TEMPORARY
    button $gr.new -text "Switch to new UI!" \
	-command "destroy $gr; New_globalRegistrationDialog"
    pack $gr.new -side top -fill x

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
	-command {globalreg_SelectPairsFor $theMesh}
    button $gr.opts.b.showgroup -text "Show only group" \
	-command {globalreg_SelectPairsFor $theMesh transitive}
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
	-command globalreg_listGroupStatus

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
    $f.entry insert end 200
    packchildren $f -side left

    checkbutton $gr.auto.preserve \
	-text "Preserve existing mesh pairs (don't recalculate)" \
	-variable preserveExistingInAutoAlign -anchor w

    checkbutton $gr.auto.normspace \
	-text "Normal-space sampling" \
	-variable normSpaceSample -anchor w

    frame $gr.auto.b
    button $gr.auto.b.go \
	-text "Go" \
	-command {
	    scz_auto_register $autoFrom $autoTo $theMesh \
		[.globalReg.auto.errorThresh.entry get] \
		[.globalReg.auto.nPairs.entry get] \
		$preserveExistingInAutoAlign \
                $normSpaceSample
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


# obsolete w/above
proc doGlobalRegister {args} {
    eval plv_globalreg register [.globalReg.reg.convTol.entry get] $args
    redraw 1
    update idletasks
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
