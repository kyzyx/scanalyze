proc clipDialog {} {
    if [window_Activate .clipDialog] return

    set cd [toplevel .clipDialog]
    wm title $cd "Clip to Selection"
    window_Register $cd

    set src [frame $cd.src -border 2 -relief groove]
    label $src.l -text "Clip what:"
    radiobutton $src.sel -text "Current mesh" \
	-variable clipSrc -value sel
    radiobutton $src.vis -text "Visible meshes" \
	-variable clipSrc -value vis
    radiobutton $src.all -text "All meshes" \
	-variable clipSrc -value all
    packchildren $src -side top -anchor w

    set trg [frame $cd.trg -border 2 -relief groove]
    label $trg.l -text "Results:"
    radiobutton $trg.new -text "In new mesh" \
	-variable clipTrg -value newmesh
    radiobutton $trg.inp -text "In place of original" \
	-variable clipTrg -value inplace
    packchildren $trg -side top -anchor w

    set mode [frame $cd.mode -border 2 -relief groove]
    label $mode.l -text "While keeping:"
    radiobutton $mode.in -text "Inside" \
	-variable clipMode -value inside
    radiobutton $mode.out -text "Outside" \
	-variable clipMode -value outside
    packchildren $mode -side top -anchor w

    button $cd.clip -text "Clip" -command {
	doClipMesh $theMesh [list $clipSrc $clipTrg $clipMode]
    }

    globalset clipSrc sel
    globalset clipTrg newmesh
    globalset clipMode inside

    packchildren $cd -side top -anchor w -padx 3 -pady 3 -fill x
}


proc doClipMesh {mesh {mode "sel newmesh inside"}} {
    global theMesh

    if {$mesh == ""} {
	puts "No mesh selected."
	return
    }

    eval "plv_clip_to_selection $mesh $mode"

    #autoClearSelection - still want to see the box when the clip is done
    updatePolyCount
    redraw 1
}


proc doClipSplitMesh {mesh} {
    doClipMesh $mesh [list sel newmesh inside]
    changeVis $mesh 1
    doClipMesh $mesh [list sel inplace outside]
}

proc clipInplaceHelper {newMesh theMeshOld} {
    addMeshToWindow $newMesh

    changeVis $newMesh 1

    if {$theMeshOld == $newMesh} {
	globalset theMesh $newMesh
    }
}

proc clipMeshCreateHelper {oldMesh newMesh} {
    addMeshToWindow $newMesh

    redraw block
    changeVis $oldMesh 0
    changeVis $newMesh 1
    redraw flush

    if {[globalset theMesh] == $oldMesh} {
	globalset theMesh $newMesh
    }
}


proc autoClearSelection {} {
    if {[globalset styleAutoClearSel]} {
	clearSelection
    }
}


proc clearSelection {} {
    plv_clearselection [globalset toglPane]
}


proc extendedHideShow {} {
    if {[window_Activate .extHideShow]} { return }
    set ehs [toplevel .extHideShow]
    wm title $ehs "Visiblity Groups"
    window_Register $ehs



    set gr [frame $ehs.visgroups -border 2 -relief groove]
    frame $gr.f
    button $gr.f.create -text "Create visibility group named:" \
	-command "newVisGroup $gr"
    entry $gr.f.name -width 12
    packchildren $gr.f -side left -fill x -expand 1
    packchildren $gr -side top -fill both -expand 1

    packchildren $ehs -side top -fill x -expand 1 \
	-padx 3 -pady 3 -ipadx 3 -ipady 3
}


proc loadMeshes {load args} {
    redraw block
    plv_progress start [llength $args] "Load [llength $args] meshes"

    foreach mesh $args {
	changeLoaded $mesh $load
	if {![plv_progress updateinc]} {
	    break
	}
    }

    plv_progress done
    redraw flush
}


proc newVisGroup {gr} {
    global uniqueInt

    set f [frame $gr.vg[incr uniqueInt]]
    set name [$gr.f.name get]
    set list [getVisibleMeshes]
    set title "list Meshes in visibility group $name:"

    label $f.l -text $name
    button $f.s -text "Show" -command \
	"tk_messageBox -message [list $list] -title [list $title] -parent $gr"
    button $f.a -text "Activate" -command "showOnlyMesh $list"
    button $f.d -text "Delete" -command "destroy $f"
    pack $f.l -side left
    pack $f.d $f.a $f.s -side right
    pack $f -side top -fill x -expand 1
}
