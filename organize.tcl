# organize.tcl
# 5/30/2000 magi@cs.stanford.edu


proc OrganizeScansDialog {} {
    if {[window_Activate .organizeScans]} { return }

    # create UI window
    set org [toplevel .organizeScans]
    wm title $org "Organize scan hierarchy"
    window_Register $org

    set f [frame $org.cmd]
    label $f.l -text "Commands:"
    set l [frame $f.line1]
    button $l.create -text "Create group:"
    entry $l.name -textvariable __org_name
    packchildren $l -side left
    button $f.delete -text "Delete group"
    packchildren $f -side top -fill x

    set f [frame $org.groups]
    # need to display hierarchy
    # for each line, need radiobutton for theMesh,
    # its name, and a --> button (for groups only)
    # that sets theMesh as a child of this group
    # Otherwise, very much like MeshControls window,
    # and should share code.

    # OR -- should we have this as a fold-out second column/pane
    # of MeshControls window itself?  (with separate scrollbar)

    packchildren $org -side top
}
