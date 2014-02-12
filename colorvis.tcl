proc colorVisUI {} {
    if [window_Activate .colorvis] return

    global cv
    set cv [toplevel .colorvis]
    wm title $cv "Color Visualization"
    window_Register $cv

    global currentSelectedMesh
    global cv_showimage
    if {![info exists cv_showimage]} {
	set cv_showimage 0
    }
    global cv_curmesh
    if {![info exists cv_curmesh]} {
	set cv_curmesh 1
    }
    global litOnly
    set litOnly 1

    # Current selected mesh
    frame $cv.csm -relief groove -borderwidth 4
    label $cv.csm.curmeshlab -text "Current Selected Mesh:"
    label $cv.csm.curmesh -relief sunken -textvariable currentSelectedMesh -anchor e
    bind $cv.csm.curmesh <Button-1> { csm_pick }
    pack $cv.csm.curmeshlab $cv.csm.curmesh -fill x

    # Options
    frame $cv.o -relief groove -borderwidth 4
    checkbutton $cv.o.showimage -text "Project Image" -variable cv_showimage \
	-command {showProjection [getListboxSelection .colorvis.il.filenames]}

    checkbutton $cv.o.curmesh -text "Track Current Mesh" -variable cv_curmesh
    checkbutton $cv.o.litonly -text "Lit Images Only" -variable litOnly -command makeImageList

    button $cv.o.allimages -text "Show all images" -command makeImageList
    button $cv.o.findimages -text "Show images on rect" \
	-command findImagesOnRect

    button $cv.o.displayimage -text "Display raw image" \
	-command displayRawImage



    pack $cv.o.showimage $cv.o.curmesh $cv.o.litonly -anchor w

    pack $cv.o.allimages $cv.o.findimages $cv.o.displayimage -fill x

    # $cv.o.allimages -anchor w -fill x

    # Image List
    frame $cv.il
    listbox $cv.il.filenames -yscrollcommand "$cv.il.scroll set"
    bind $cv.il.filenames <ButtonRelease-1> {
	showProjection [getListboxSelection .colorvis.il.filenames] }

    scrollbar $cv.il.scroll -command "$cv.il.filenames yview"
    pack $cv.il.filenames $cv.il.scroll -side left  -fill y -expand false
    $cv.il.filenames configure -height 15

    # Pack All
    pack $cv.csm $cv.o -fill x
    pack $cv.il

    # Start Idle Routine
    #  $cv.il.filenames configure -height 100
    # set currentSelectedMesh ""
    colorvis_idlerefresh
    after 100 scrollboxhack
    updateCVBox

}

proc csm_pick {} {
    global cv_curmesh
    global currentSelectedMesh

    if { ! $cv_curmesh } {
	# Print a usage info thing
	puts ""
	puts "You cant pick a directory, so to pick a scan: Pick any sweep in "
	puts "  the scan directory."
	# Pick a new scan
	set types {
	    {{sd Files}       {.sd}}}


	set filename [tk_getOpenFile -defaultextension .sd -filetypes $types]
	if {$filename != ""} {
	    set currentSelectedMesh [ file dirname $filename ]
	    makeImageList
	}
    }
}

# make the list window expandable
# check geometry before scroll size chagne and reset afterwards
proc scrollboxhack {} {
    global cv
    set geom [wm geometry $cv]
    $cv.il.filenames configure -height 300
    wm geometry $cv $geom
}

# Show projection of image on meshes
proc showProjection {imageName} {
    global currentSelectedMesh
    global cv_showimage

    if  $cv_showimage  {
	plv_load_projective_texture $currentSelectedMesh/color/$imageName nomipmap
    } else {
	plv_load_projective_texture $currentSelectedMesh/color/$imageName green nomipmap
    }


    globalset theColorMode texture
    redraw 1

}

# Refresh the notion of current active mesh occasionally
proc colorvis_idlerefresh {} {
    global currentSelectedMesh
    global theMesh
    global imageList
    global cv
    global cv_curmesh

    if { $cv_curmesh && [winfo exists .colorvis] && $theMesh != "" } {

	# Check if we need to update the list
	set presentmesh [plv_get_scan_filename $theMesh]
	set dirname [file dirname $presentmesh]
	set ext [file extension $dirname]

	if {$ext == ".sd"} {set presentmesh $dirname}

	if [expr  [string compare $currentSelectedMesh $presentmesh] != 0 ]  {
	    # Change the current mesh name
	    set currentSelectedMesh $presentmesh
	    puts changing
	    # Make a new list
	    makeImageList

	}

	#$cv.il.filenames configure -height 30
    }
	# Call again after an interval
	after 300 colorvis_idlerefresh

}

proc makeImageList {} {
    global cv
    global currentSelectedMesh
    global litOnly
    global cv_ImageList

    # Make a new list
    set cv_ImageList ""
    if { $litOnly == 1 } {
	if { [file exists $currentSelectedMesh/images.paste] } {
	    set imageList [exec grep process $currentSelectedMesh/images.paste | sed s/^.*color.\\(.*\\)\\..*/\\1/]
	} else {
	    puts "Failed to open file: "
	    puts  $currentSelectedMesh/images.paste
	    set imageList ""
	}
	set ext ""
	foreach filename $imageList {
	    if {$ext == ""} {
		if [file exists $currentSelectedMesh/color/$filename.png] {
		    set ext "png"
		} else {
		    set ext "rgb"
		}
	    }

	    lappend cv_ImageList  $filename.$ext

	}

    } else  {
	set imageList [glob $currentSelectedMesh/color/*.png $currentSelectedMesh/color/*.rgb]
	foreach filename $imageList {
	    lappend cv_ImageList $filename
	}
    }

    # Actually set it in the box
    updateCVBox
}

proc updateCVBox {} {
    global cv_ImageList
    global cv

    if [winfo exists .colorvis] {
	$cv.il.filenames delete 0 [$cv.il.filenames size]
	foreach filename $cv_ImageList {
	    $cv.il.filenames insert end [file tail $filename]
	}
    }
}

# Optional param of the rect to look in. Defaults to current selection
proc findImagesOnRect {{bbox ""} } {
    global cv
    global cv_ImageList
    globalset cv_showimage 0

   # set LB $cv.il.filenames
    set len [llength $cv_ImageList]
    set count 0
    while {$count < [llength $cv_ImageList]} {
	set curfile [lindex $cv_ImageList $count]
	showProjection $curfile
	set pixcount [eval plv_countPixels 0 0 1 255 255 255 $bbox ]
	if {$pixcount > 0} {
	    set cv_ImageList [lreplace $cv_ImageList $count $count]
	} else {
	    set count [expr $count + 1]
	}
    }
    updateCVBox
}


proc displayRawImage {} {
    global currentSelectedMesh
    set imageName [getListboxSelection .colorvis.il.filenames]

    puts $currentSelectedMesh/color/$imageName
    exec display $currentSelectedMesh/color/$imageName
}


# This can be called externally to set the notion of current color
# Mesh in this tool For instance the pickColor button uses it.
proc	color_vis_set_mesh {selected} {
    global currentSelectedMesh
    global cv_cur_scan

    set cv_cur_scan $selected

    # Get the filename of this scan
    set fname [plv_get_scan_filename "$selected"]
    set dirname [file dirname $fname]
    set ext [file extension $dirname]
    if {$ext == ".sd"} {set fname $dirname}

    # Change the current mesh name
    set currentSelectedMesh  $fname

    # Set vars
    globalset litOnly 1
    globalset cv_curmesh 0

    # Make a new list
    makeImageList

}

proc color_vis_proj_color {x y } {
    global toglPane
    global cv_ImageList
    global cv
    global cv_showimage
    global currentSelectedMesh
    global cv_cur_scan

    # Turn on green mode
    set cv_showimage 0

    # Show only the relevant scan
    set mlist [getVisibleMeshes]
    puts $mlist
    showOnlyMesh $cv_cur_scan

    # Search for correct set of images
    set height [lindex [$toglPane config -height] 4]
    set y [expr $height - $y - 1]
    set xpp [expr $x + 1]
    set ypp [expr $y + 1]
    findImagesOnRect "$x $y $xpp $ypp"

    # Turn back on all previous meshes
    eval showOnlyMesh "$mlist"

    # Pick the first one and show it
    set image [lindex $cv_ImageList 0]
    if  [winfo exists .colorvis] {
	$cv.il.filenames selection set 0
    }
    set cv_showimage 1
    showProjection $image
}

