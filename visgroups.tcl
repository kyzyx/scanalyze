
# To add new things to the stored context:

# make up a new global var like vgRestoreRenderColor
# make up a new global var like vgRenderColor
# Add a global line to buildVisGroupsUI
# Add to the menu in buildVisGroupsUI for defauts pop up menu
# Add to vg_globalVars so defaults are set
# Add to vg_CreateView so defaults are copied to new context
# Add to menu in vg_addContextUI for context pop up menu
# Add to vg_SaveCurrentState
# Add to vg_RestoreState


#Build UI
proc buildVisGroupsUI {} {
    if [window_Activate .visgroups] return

    # Global vars # Set up default values if necessary
    vg_globalVars
    global vgRestoreCamera
    global vgRestoreMeshList
    global vgMeshMod
    global vg_viewName
    global vgAutoUpdate

    # Build window
    global vg
    set vg [toplevel .visgroups]
    wm title $vg "Context Manager"
    window_Register $vg

    # Entry Box
    frame $vg.eb
    button $vg.eb.addView -text "Create Context:" \
	-command vg_CreateView
    entry $vg.eb.viewName -relief sunken -textvariable vg_viewName
    pack $vg.eb.addView -fill x
    pack $vg.eb.viewName  -fill x
    bind $vg <Return> vg_CreateView

    # Defaults button
    menubutton $vg.menuB -relief raised \
	-text "Context Defaults ..." -menu $vg.menuB.defaultMenu
    bind $vg.menuB <Button-3> {
	tk_popup $vg.menuB.defaultMenu %X %Y
    }
    set defaultMenu [menu $vg.menuB.defaultMenu \
			 -tearoffcommand {setMenuTitle "Context Defaults"}]

    # Defaults menu

    $defaultMenu add checkbutton -label "Auto update context" \
	-variable vgAutoUpdate(default)

    $defaultMenu add separator
    $defaultMenu add checkbutton -label "Restore Camera" \
	-variable vgRestoreCamera(default)
    $defaultMenu add checkbutton -label "Restore Scan Set" \
	-variable vgRestoreMeshList(default)
    $defaultMenu add checkbutton -label "Restore Mesh Resolutions" \
	-variable vgRestoreMeshResolutions(default)

    $defaultMenu add separator
    $defaultMenu add radiobutton -label "Show ONLY scan set" \
	-variable vgMeshMod(default) -value exact
    $defaultMenu add radiobutton -label "Show scan set" \
	-variable vgMeshMod(default) -value show
    $defaultMenu add radiobutton -label "Hide scan set" \
	-variable vgMeshMod(default) -value hide
    $defaultMenu add radiobutton -label "Hide/Show toggle scan set" \
	-variable vgMeshMod(default) -value toggle

    $defaultMenu add separator
    $defaultMenu add command -label "Clear All Contexts" \
      -command {vg_clearAllContext}
    $defaultMenu add command -label "Save Context Set As..." \
      -command {vg_saveAsUI}
    $defaultMenu add command -label "Load Context Set..." \
      -command {vg_loadFileUI}
    $defaultMenu add command -label "Create Pseudo Group..." \
      -command {vg_savePseudoGroupUI}


    # List frame
    frame $vg.lb
    frame $vg.vMenus

    listbox $vg.lb.viewnames -yscrollcommand "$vg.lb.scroll set"
    bind $vg.lb.viewnames <ButtonRelease-1> {  }

    scrollbar $vg.lb.scroll -command "$vg.lb.viewnames yview"
    pack $vg.lb.scroll -side right -fill y
    pack $vg.lb.viewnames -fill x

    # Bindings for buttons in listbox
    bind $vg.lb.viewnames <Button-3>  {
	# Pop up menu
	set item [$vg.lb.viewnames get @%x,%y]
	if {$item != ""} {
	    tk_popup $vg.vMenus.x$item %X %Y
	}
    }
    bind $vg.lb.viewnames <Motion> {
	# Selection tracks mouse
	$vg.lb.viewnames selection clear 0 end
	$vg.lb.viewnames selection set @%x,%y
    }
    bind $vg.lb.viewnames <Leave> {
	# Leave clears slectioin
	$vg.lb.viewnames selection clear 0 end
	if {$vgCurrentContext != "" && \
		$vgAutoUpdate($vgCurrentContext)} {
	    vgSetListBox $vgCurrentContext
	}
    }
    bind $vg.lb.viewnames <Button-1> {
	# Restore state
	vg_RestoreState [$vg.lb.viewnames get @%x,%y]
    }

    # Reset the list of whats in the listbox
    vg_updateListbox

    $vg.lb.viewnames configure -height 25


    # Pack
    pack $vg.eb -fill x
    pack $vg.menuB -anchor w
    pack $vg.lb -fill x
}

proc vgSetListBox {name} {
    global vg

    for {set i 0} {$i < [$vg.lb.viewnames index end]} {incr i} {
	if {[$vg.lb.viewnames get $i] == "$name"} {
	    $vg.lb.viewnames selection set $i
	}
    }
}

proc vg_updateListbox {} {
    global vgRestoreCamera

    #Build new UI
    foreach view [array names vgRestoreCamera] {
	if {$view != "default"} {vg_addContextUI $view}
    }
}

proc vg_clearAllContext {} {
    global vgRestoreCamera
    foreach view [array names vgRestoreCamera] {
	if {$view != "default"} {vg_deleteContext $view}
    }
}
# Global Vars
proc vg_globalVars {} {

    global vgRestoreCamera
    global vgRestoreMeshList
    global vgCamera
    global vgMeshList
    global vgMeshMod
    global vgAutoUpdate
    global vgCurrentContext
    global vgRestoreMeshResolutions
    global vgMeshResolutions

    if {![info exists vgCurrentContext]} {
	set vgCurrentContext ""
    }

    if {![info exists vgAutoUpdate]} {
	set vgAutoUpdate(default) 0
    }

    if {![info exists vgRestoreCamera]} {
	set vgRestoreCamera(default) 1
    }

    if {![info exists vgRestoreMeshList]} {
	set vgRestoreMeshList(default) 1
    }

   if {![info exists vgRestoreMeshResolutions]} {
	set vgRestoreMeshResolutions(default) 1
    }

    if {![info exists vgMeshMod]} {
	set vgMeshMod(default) exact
    }

    uplevel {
	global vgRestoreCamera
	global vgRestoreMeshList
	global vgCamera
	global vgMeshList
	global vgMeshMod
	global vgAutoUpdate
	global vgCurrentContext
	global vgRestoreMeshResolutions
	global vgMeshResolutions

    }

}

# saves the current context set to a file
proc vg_saveContextSetToFile {filename} {
    vg_globalVars
    pref_saveVarsToFile $filename vgRestoreCamera vgRestoreMeshList vgCamera vgMeshList vgMeshMod vgAutoUpdate vgRestoreMeshResolutions vgMeshResolutions
}

proc vg_loadContextSetFromFile {filename} {
    vg_globalVars
    pref_loadVarsFromFile $filename
    vg_updateListbox
}

proc vg_saveAsUI {} {
    set types {
	{{Context file}       {.context}}
    }

    set name [tk_getSaveFile -defaultextension .context -filetypes $types\
		  -title "Save context set as"]

    if {"$name" != ""} {
	vg_saveContextSetToFile $name
    }
}

proc vg_loadFileUI {} {
    set types {
	{{Context file}       {.context}}
    }

    set name [tk_getOpenFile -defaultextension .context -filetypes $types\
		  -title "Load context set "]

    if {"$name" != ""} {
	vg_loadContextSetFromFile $name
    }
}


proc vg_savePseudoGroupUI {} {

    set name [tk_getSaveFile -title "Create a pseudo group"]

    if {"$name" != ""} {
	scz_pseudogroup $name [getVisibleMeshes]
    }
}


# CreateView
proc vg_CreateView {{viewArg ""}} {
    global vg_viewName
    global vgRestoreCamera
    global vgRestoreMeshList
    global vgMeshMod
    global vgCurrentContext
    global vgAutoUpdate

    # Make sure global vars exist
    vg_globalVars

    # If the user specifies use this, otherwise use the entrybox variable
    if {"$viewArg" != ""} {
	set vg_viewName $viewArg
    }

    # Check for empty view name
    if {"$vg_viewName" == "" } { return}

    # Check for name default
    if {"$vg_viewName" == "default" } { return}

    # Check for repeated view name
    foreach view [array names vgRestoreCamera] {
	if {$view == "$vg_viewName"} {
	    vg_SaveCurrentState $vg_viewName
	    puts "Saving current view to $vg_viewName."
	    vg_clearEntry
	    return
	}
    }

    # Set option values for new View
    set vgRestoreCamera($vg_viewName) $vgRestoreCamera(default)
    set vgRestoreMeshList($vg_viewName) $vgRestoreMeshList(default)
    set vgMeshMod($vg_viewName) $vgMeshMod(default)
    set vgAutoUpdate($vg_viewName) $vgAutoUpdate(default)
    set vgRestoreMeshResolutions($vg_viewName) $vgRestoreMeshResolutions(default)

    # Save Current State
    vg_SaveCurrentState $vg_viewName

    #Build new UI elements
    vg_addContextUI $vg_viewName

    # Auto update old context?
    if {$vgCurrentContext != ""} {
	if {$vgAutoUpdate($vgCurrentContext) == 1} {
	    vg_SaveCurrentState $vgCurrentContext
	}
    }

    # Set Current Context
    set vgCurrentContext $vg_viewName

    # Clear the entry box - last since we are using the value
    vg_clearEntry


}

proc vg_clearEntry {} {
    if [winfo exists .visgroups] {
	.visgroups.eb.viewName delete 0 end
    }
    globalset vg_viewName ""
}

proc vg_addContextUI {vg_viewName} {
    vg_globalVars

    # If the window isn't present return
    if {![winfo exists .visgroups]} return;

    # If the view is alrady in the list return
    if {[winfo exists .visgroups.vMenus.x$vg_viewName]} return;

    # Add View to list
    .visgroups.lb.viewnames insert end $vg_viewName

    # Create menu
    set m [menu .visgroups.vMenus.x$vg_viewName \
	       -tearoffcommand "setMenuTitle Context_$vg_viewName" ]

    $m add checkbutton -label "Auto update context" \
	-variable vgAutoUpdate($vg_viewName)

    $m add separator

    $m add checkbutton -label "Restore Camera" \
	-variable vgRestoreCamera($vg_viewName)

    $m add checkbutton -label "Restore Scan Set" \
	-variable vgRestoreMeshList($vg_viewName)
    $m add checkbutton -label "Restore Mesh Resolutions" \
	-variable vgRestoreMeshResolutions($vg_viewName)



    $m add separator
    $m add radiobutton -label "Show ONLY scan set" \
	-variable vgMeshMod($vg_viewName) -value exact
    $m add radiobutton -label "Show scan set" \
	-variable vgMeshMod($vg_viewName) -value show
    $m add radiobutton -label "Hide scan set" \
	-variable vgMeshMod($vg_viewName) -value hide
    $m add radiobutton -label "Hide/Show toggle scan set" \
	-variable vgMeshMod($vg_viewName) -value toggle

    $m add separator
    set meshMenu [createNamedCascade $m meshMenu \
		      "MeshList"  ]
    $meshMenu configure -tearoffcommand "setMenuTitle $vg_viewName"
    foreach mesh $vgMeshList($vg_viewName) {
	$meshMenu add command -label $mesh
    }
    $m add separator

    $m add command -label "Delete Context" \
	-command "vg_deleteContext $vg_viewName"
    $m add command -label "Update Context" \
	-command "vg_SaveCurrentState $vg_viewName"
}

proc vg_deleteContext {name} {
    global vgRestoreCamera
    global vgCurrentContext

    # Remove context from vgRestoreCamera (so we think its gone)
    unset vgRestoreCamera($name)

    if [winfo exists .visgroups] {
	# Delete menu
	destroy .visgroups.vMenus.x$name

	# Rebuild UI list - since I dont know how to delete this one
	.visgroups.lb.viewnames delete 0 end
	foreach view [array names vgRestoreCamera] {
	    if {$view != "default"} {
		.visgroups.lb.viewnames insert end $view
	    }
	}
    }
    # If this was the current context, set no current
    if {"$name" == "$vgCurrentContext" } {
	set vgCurrentContext ""
    }
}

proc vg_updateMeshListMenu {context} {
    vg_globalVars
    set m .visgroups.vMenus.x$context.meshMenu

   # puts "Checking existant"
    if {![winfo exists $m]} return;
   # puts "Finxing menu"

    # Clear old menu
    $m delete 0 end

    # Build new menu
    foreach mesh $vgMeshList($context) {
	$m add command -label $mesh
    }
}

proc vg_SaveCurrentState {name} {
    global vgRestoreCamera
    global vgRestoreMeshList
    global vgCamera
    global vgMeshList
    global vg_viewName
    vg_globalVars

    set vgCamera($name) [plv_positioncamera]
    set vgMeshList($name) [getVisibleMeshes]
    set vgMeshResolutions($name) [vg_buildMeshResList [getVisibleMeshes]]
   # puts $vgMeshResolutions($name)
    vg_updateMeshListMenu $name

}

proc vg_buildMeshResList {meshes} {

    set retstr ""
    puts $meshes
    foreach mesh $meshes {
	lappend retstr "$mesh [plv_getcurrentres $mesh]"
    }


    return $retstr
}

proc vg_setRestoreScanOnly {name} {
    globalset vgRestoreCamera($name) 0
    globalset vgRestoreMeshResolutions($name) 0
}

proc vg_RestoreState {name} {
    global vgRestoreCamera
    global vgRestoreMeshList
    global vgCamera
    global vgMeshList
    global vgMeshMod
    global vgrs_toggle
    global vgCurrentContext
    global vgAutoUpdate

    vg_globalVars

    # Check for empty
    if {$name == ""} return

    # Auto update old context?
    if {$vgCurrentContext != ""} {
	if {$vgAutoUpdate($vgCurrentContext) == 1} {
	    vg_SaveCurrentState $vgCurrentContext
	}
    }

    # Set new context
    set vgCurrentContext $name

    redraw block

    # Restore new context
    if {![info exist vgrs_toggle]} {set vgrs_toggle 0}

    # read camera pos into temp variable
    set camerapos $vgCamera($name)

    # because if we're switching to PREVIOUS, it will change when we
    # mess with the mesh list
    if {$vgRestoreMeshList($name) == 1} {
	if {$vgMeshMod($name) == "exact"} {
	    eval showOnlyMesh $vgMeshList($name)
	} elseif {$vgMeshMod($name) == "hide"} {
	    eval hideMesh $vgMeshList($name)
	} elseif {$vgMeshMod($name) == "show"} {
	    eval showMesh $vgMeshList($name)
	} elseif {$vgMeshMod($name) == "toggle"} {
	    if {$vgrs_toggle == 1} {
		eval showMesh $vgMeshList($name)
	    } else {
		eval hideMesh $vgMeshList($name)
	    }
	    set vgrs_toggle [expr 1 - $vgrs_toggle]
	}
    }
    if {$vgRestoreMeshResolutions($name) == 1} {

	foreach pair $vgMeshResolutions($name) {
	    eval "setMeshResolution $pair"
	}
    }

    if {$vgRestoreCamera($name) == 1} {
	eval plv_positioncamera $camerapos
    }

    # Redraw
    redraw flush
}

proc vg_pickScanContextHelper {mesh state} {
    global vgRestoreCamera
    global vgMeshList
    # If shift key isn't down just set the context to
    # be the selected scan
    # if shift key down append to the list
    # Mesh is the picked mesh
    # state is the modifier state from the event.
    # 1= shift key down

    # Make sure we have the desired context
    if {![info exists vgRestoreCamera(PICK_SCAN)]} {
	vg_CreateView PICK_SCAN
	puts "creating PICKSCAN"
    }

    # Add to context
    if {$state == 1} {
	# shift key down, append
	if {[lsearch -exact $vgMeshList(PICK_SCAN) $mesh]== -1} {
	lappend vgMeshList(PICK_SCAN) $mesh
	}
    } elseif {$state == 0} {
	# set scan
	globalset vgMeshList(PICK_SCAN) $mesh

    } else {
	puts "bad staet"
	return
    }

    # Update the meshList menu option
    vg_updateMeshListMenu PICK_SCAN

    # Make sure its set to only restore scan
    vg_setRestoreScanOnly PICK_SCAN
}
