# To load up a saved group, for example, you need to be able to
# create by name.

proc group_createNamedGroup {name members {dirty 1}} {
    global theMesh

    set oldtheMesh $theMesh
    #subsequent commands change theMesh which messes up the foreach loop

    # check for any meshes that have already been loaded in
    # as part of another group - thus if any of the meshes
    # in $members are roots then they are legit i.e. there is
    # no problem - otherwise return without creating the group.
    set legitscans [plv_listscans root]
    set offenders ""
    set newmembers ""
    set i 0
     while { $i < [llength $members] } {
 	set member [lindex $members $i]
	set loaded [lsearch -exact $legitscans $member]
	if { $loaded == -1 } {
	    lappend offenders $member
	} else {
	    lappend newmembers $member
	}
	incr i
    }
    set members $newmembers

    if { $offenders != "" } {
	tk_messageBox -title Scanalyze -icon \
	    error -message "At least of the meshes you tried to group as \
                     [file tail $name] \
                     is already part of another group. The offenders were \
                     $offenders. [file tail $name] was not created." \
	    -type ok
	puts "Offenders are $offenders"
	return
    }

    removeMeshFromWindow $members
    set group [eval plv_groupscans create [append name ".gp"] $members $dirty]
    addMeshToWindow $group $members

    # fix selection
    foreach member $members {
	if {$member == $oldtheMesh} {
	    globalset theMesh $group
	}
    }
}


proc group_createFromVis {} {
    global theMesh

    set members [getVisibleMeshes]
    if {$members == ""} {
	tk_messageBox -title Scanalyze -icon \
	    error -message "Please make the meshes you wish to group visible." \
	    -type ok
	puts $members
	return
    }
    set oldtheMesh $theMesh
    #subsequent commands change theMesh which messes up the foreach loop

    removeMeshFromWindow $members
    set name [plv_getNextAvailGroupName]
    set group [eval plv_groupscans create [append name ".gp"]  $members 1]
    addMeshToWindow $group

    # fix selection

    foreach member $members {
	if {$member == $oldtheMesh} {
	    globalset theMesh $group
	}
    }
    redraw safeflush
}

proc group_createNamedFromVis {} {
    global theMesh

    set members [getVisibleMeshes]
    if {$members == ""} {
	tk_messageBox -title Scanalyze -icon \
	    error -message "Please make the meshes you wish to group visible." \
	    -type ok
	puts $members
	return
    }

    # setting up the dialog box that allows you to choose the group name
    set name [plv_getNextAvailGroupName]

    toplevel .g
    frame .g.group
    frame .g.but

    label .g.group.label -text "Enter group name: "
    entry .g.group.ent -width 36 -textvariable name
    .g.group.ent delete 0 end
    .g.group.ent insert 0 $name
    .g.group.ent selection range 0 end
    button .g.but.ok -text Ok \
	-command { trytomakegroup [.g.group.ent get] .g [getVisibleMeshes] }
    button .g.but.cancel -text Cancel -command "destroy .g"


    wm title .g "Create group..."
    wm transient .g .
    # the transient call registers the dialog box with the top level
    # window of the application
    pack .g.group.label .g.group.ent -side left
    pack .g.but.ok .g.but.cancel -side left
    pack .g.group .g.but -side top -padx 5 -pady 5

    bind .g.group.ent <Key-Return> {
	trytomakegroup $name .g [getVisibleMeshes]
    }
    grab set .g
    focus .g.group.ent
    tkwait window .g
}

# This procedure destroys dialog, and creates a group called name with
# members meshlist.
proc trytomakegroup { name dialog meshlist } {
    if {$name == ""} {
	tk_messageBox -title Scanalyze -icon error -type ok \
		-message "No name specified - try again."
    } else {
	destroy $dialog
	group_createNamedGroup $name $meshlist 1
	redraw flush
    }
}


proc group_breakGroup {group} {
    global theMesh
    global meshVisible

    set oldtheMesh $theMesh
    #subsequent commands change theMesh which messes up the foreach loop

    redraw block
    set vis $meshVisible($group)
    set members [plv_groupscans break $group]


    removeMeshFromWindow $group
    foreach member $members {
	addMeshToWindow $member
	changeVis $member $vis
    }

    # fix selection
    if {$oldtheMesh == $group} {
	globalset theMesh [lindex $members 0]
    }
    plv_pickscan init
    updateFromAndToMeshNames
    redraw flush
}

# assumes that theMesh is currently set to a group - otherwise pops up a dialog
proc group_saveCurrentGroup {} {
    global theMesh

    set members [plv_groupscans list $theMesh]
    if {$members == ""} {
	tk_messageBox -message "The group \"$theMesh\" does not \
        have any members - perhaps it's not a group after all?" -type ok
	return
    }

    group_recursiveSave $theMesh $members [pwd]
}

proc group_saveCurrentGroupToDir {} {
    global theMesh

    set members [plv_groupscans list $theMesh]
    if {$members == ""} {
	tk_messageBox -message "The group \"$theMesh\" does not \
        have any members - perhaps it's not a group after all?" -type ok
	return
    }

    set dir [tk_chooseDir -title "Choose directory to save group to"]

    if { $dir != "" } {
	if {![file exists $dir]} {
	    file mkdir $dir
	}
    }
    group_recursiveSave $theMesh $members $dir
}

proc group_recursiveSave {group meshes dir} {
    set name ""

    foreach mesh $meshes {
	set children [plv_groupscans list $mesh]
	if { $children != "" } {
	    group_recursiveSave $mesh $children $dir
	}
    }
    set head [file join $dir $group]
    plv_saveCurrentGroup $group [append head ".gp"]
}


proc group_expandGroup {group} {
    # BUGBUG -- expansion doesn't currently work with the canvas
    tk_messageBox -message "Group expansion will be working soon."
    # Print out contents of group  to see what is in the group,
    # though normal expansion doesn't work. -seander

    #set members [plv_groupscans list $group]
    #puts stderr "\nMesh $group: " nonewline
    #foreach member $members {
	#puts stderr "$member " nonewline
    #}
    #puts stderr ""
    return

    global meshFrame

    set groupFrame $meshFrame($group)
    set title [frame $groupFrame.title2]
    set widget [label $title.expand -text -]
    bind $widget <ButtonPress-1> "group_contractGroup $group $groupFrame"
    label $title.l -text $group
    pack $widget $title.l -side left
    pack $title -side top -fill x -anchor w
    pack forget $groupFrame.title


    set members [plv_groupscans expand $group]
    set i 0
    foreach member $members {
	set fr $groupFrame.g$i
	buildUI_privAddMeshAsFrame $member $fr
	buildUI_privChangeMeshActive $member 1 $fr
	pack $fr -side top -fill x -expand 1 -anchor e
	incr i
    }

    if {[globalset theMesh] == $group} {
	globalset theMesh [lindex $members 0]
    }

    buildUI_privChangeMeshActive $group 0
    resizeMCscrollbar
}


proc group_contractGroup {group groupFrame} {

    global meshFrame
    destroy $groupFrame.title2
    pack $groupFrame.title -fill both -expand 1

     set i 0
     set activateGroup 0
     while {[winfo exists $groupFrame.g$i]} {
    	set member [lindex [$groupFrame.g$i.title.label config -text] 4]
	incr i
     	lappend members $member

 	if {[globalset theMesh] == $member} {
 	    set activateGroup 1
 	}
    }


    #eval plv_groupscans contract $group $members
    if {$activateGroup} {
	globalset theMesh $group
    }

    foreach member $members {
	buildUI_dummyPrivChangeMeshActive $member 0
    }
    buildUI_privChangeMeshActive $group 1 $groupFrame
    puts "Members array is $members"

    resizeMCscrollbar
}
