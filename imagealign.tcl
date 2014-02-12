# Open the main image alignment window
proc imageAlignmentUI {} {
    if [window_Activate .imagealign] return

    set iaw [toplevel .imagealign]
    wm title $iaw "Image Alignment"
    window_Register $iaw

    # How can I keep variables available to callbacks except global?
    global curImageAlignFile
    global shortIAF
    global fivetuples
    global xyztuples
    globalset fivetuples ""
    globalset xyztuples ""
    global curPasteMesh
    global shortPM
    global cameraType
    global displayscalevar
    ia_globals

    # Set default camera type
    if {![info exists cameraType]} {
	set cameraType DKCST5_2V
    }

    # Set default display scale
    if {![info exists displayscalevar]} {
	set displayscalevar 25
    }

    # Clear the current point correspondances when we start the window
    # Lucas change:
    # Used to be:     exec echo "" > [glob ~/.tmp2dpoints]
    # Removed the glob, because if the file didn't exist (e.g. a user's
    # first time), the glob would not return a name, and the image
    # align window would never come up.
    exec echo "" > ~/.tmp2dpoints
    exec echo "" > ~/.xyztuples

    # Set the initial state to not corresponding
    globalinit imageactive 0

    menubutton $iaw.menuB -text "Commands Menu" -padx 3 -pady 1 -relief raised\
	-menu $iaw.menuB.iamenu -underline 0
    set iamenu [menu  $iaw.menuB.iamenu \
		-tearoffcommand {setMenuTitle "Image Alignment Controls"}]

    $iamenu add command -label "Open Image..." \
	-command establishname
    $iamenu add command -label "Close Image" -command doneclicking
    $iamenu add command -label "Save Correlation Info" -command ia_saveCorrFile
    $iamenu add command -label "Establish paste mesh..." -command setpastemesh

    $iamenu add separator
    set camMenu [createNamedCascade $iamenu camMenu \
		   "Camera Type"]
    $camMenu add radiobutton -label DKCST5_2 -variable cameraType -value DKCST5_2
    $camMenu add radiobutton -label DKCST5_2V -variable cameraType -value DKCST5_2V
    $camMenu add radiobutton -label DKCST5_640x480 -variable cameraType -value DKCST5_640x480
    $camMenu add radiobutton -label DKC5000_gantry -variable cameraType -value DKC5000_gantry
    $camMenu add radiobutton -label DKC5000_chapel -variable cameraType -value DKC5000_chapel

    set scaleMenu [createNamedCascade $iamenu scaleMenu \
		   "Display Scale"]
    for {set i 5} {$i <= 100} {incr i 5} {
	$scaleMenu add radiobutton -label $i% -variable displayscalevar -value $i
    }

    $iamenu add separator
    $iamenu add command -label "Delete Selected Point" -command ia_deleteSelectedPoint
    $iamenu add command -label "Delete All Points" -command {ia_clearCorrList; ia_updateUI}
    $iamenu add separator
    $iamenu add checkbutton -label "Auto Zooming" -variable ia_autoZooming
    $iamenu add checkbutton -label "Zooming On" -variable ia_zoomOn
    $iamenu add checkbutton -label "Mesh Picking On" -variable ia_zoomPickingOn
    $iamenu add checkbutton -label "Picked Mesh Only On" -variable ia_zoomSingleVisOn
    $iamenu add checkbutton -label "Picked Mesh to Res 0 On" -variable ia_zoomResChangeOn
    $iamenu add command -label "Zoom to Feature" -command  {
	global ia_lastZoomPoint
	set ia_lastZoomPoint ""
	ia_zoomToFeature
    }
    $iamenu add command -label "Zoom to .tsai camera viewpoint" -command setview
    $iamenu add command -label "Undo viewpoint change" -command undoview
    $iamenu add separator

#    $iamenu add command -label "Refresh 5-tuples" -command load5tuples
#   $iamenu add command -label "Create .xyzuv" -command savexyzuv
    $iamenu add command -label "Create .tsai" -command createtsai
    $iamenu add separator
    $iamenu add command -label "Paste Color (This Image)" -command dopastecolor
    $iamenu add command -label "Paste Color (Merged Images)" -command dopasteallcolor
    $iamenu add command -label "Load colored mesh(This Image)" -command loadlocalcolor
    $iamenu add command -label "Load colored mesh(Merged Images)" -command loadallcolor
    $iamenu add separator
    $iamenu add command -label "View Color Projection" -command projectivecolor
    $iamenu add command -label "Generate this projected .rgb" -command gentexrgb
    $iamenu add command -label "Generate all projected .rgbs" -command gentexrgbs
    $iamenu add separator
    $iamenu add command -label "Reload imagealign.tcl" -command ia_reload

 #   $iamenu add command -label "" -command
 #   $iamenu add command -label "" -command
  #  $iamenu add command -label "" -command

   # $iamenu add separator

    #buttons section 1 establishing things
    label $iaw.curfilelabel  -textvariable shortIAF
    label $iaw.curmeshlabel  -textvariable shortPM
    scale $iaw.displaysize -relief ridge -orient horizontal -resolution 5 -label "Image Display Scale %" -from 5 -to 100 -variable displayscalevar

    # Camera types

    frame $iaw.cameras
    label $iaw.cam -text "Cam Type:"
    radiobutton $iaw.horz -text DKCST5_2 -variable cameraType -value DKCST5_2
    radiobutton $iaw.vert -text DKCST5_2V -variable cameraType -value DKCST5_2V
    radiobutton $iaw.ntsc -text DKCST5_640x480 -variable cameraType -value DKCST5_640x480
    radiobutton $iaw.fivek -text DKC5000 -variable cameraType -value DKC5000
    pack $iaw.cam $iaw.horz $iaw.vert $iaw.ntsc $iaw.fivek -in  $iaw.cameras -side left


    # The five tuple region
    listbox $iaw.points  -relief sunken -height 25 -width 50

    # Pack it all
    pack   $iaw.menuB -anchor w
    pack $iaw.curfilelabel $iaw.curmeshlabel\
	$iaw.points -fill x

    doneclicking

    ia_updateUI
}

proc gentexrgb {} {
    global curImageAlignFile
    global env
    set tsaifile [file rootname $curImageAlignFile].tsai
    set infile [file rootname $tsaifile].rgb
    set grayrgbfile [file rootname $tsaifile].gray.rgb
    set projrgbfile [file rootname $tsaifile].proj.rgb
    plv_resetxform
    plv_positioncamera $tsaifile
    globalset theColorMode gray
    setShininess 0
    plv_writeiris [globalset toglPane] $grayrgbfile
    plv_load_projective_texture $infile
    globalset theColorMode texture
    plv_writeiris [globalset toglPane] $projrgbfile
}

proc gentexrgbs {} {
    global curImageAlignFile
    global env
    set tsaifiles [glob [file dirname $curImageAlignFile]/*.tsai]

    for {set i 0} {$i < [llength $tsaifiles]} {incr i} {
	set tsaifile [lindex $tsaifiles $i]
	set infile [file rootname $tsaifile].rgb
	set grayrgbfile [file rootname $tsaifile].gray.rgb
	set projrgbfile [file rootname $tsaifile].proj.rgb
	plv_resetxform
	plv_positioncamera $tsaifile
	globalset theColorMode gray
	setShininess 0
	plv_writeiris [globalset toglPane] $grayrgbfile
	plv_load_projective_texture $infile
	globalset theColorMode texture
	# redraw 1
	plv_writeiris [globalset toglPane] $projrgbfile
    }
}

proc undoview {} {
    restorecam previousCameraView; redraw 1
}

proc ia_getAspectFromTsai {{filename ""}} {

    if {$filename == ""} {
	ia_globals
	global curImageAlignFile
	set filename [file rootname $curImageAlignFile].tsai
    }

    if {![file exists $filename]} return

    set fid [open $filename]
    gets $fid line
    scan $line "%f" width
    gets $fid line
    scan $line "%f" height
    close $fid

    return "$width $height"
}

proc setview {} {
    global curImageAlignFile
    global renderOnlyActiveMesh
    ia_globals

    set tsaifile [file rootname $curImageAlignFile].tsai
    # We reset the transform before setting it to the new camera
    # because accumulated trackball junk screws things up

    #resize window to match file
    set ia_windowSizeMultiplier [ eval "setMainWindowAspect [ia_getAspectFromTsai]"]

    # move the camera
    savecam previousCameraView
    plv_resetxform
    plv_positioncamera $tsaifile ; redraw 1

    # Set the new position as the TSAI_POSITION
    vg_CreateView TSAI_POSITION
    set renderOnlyActiveMesh 0
}


proc projectivecolor {} {
    global curImageAlignFile
    plv_load_projective_texture $curImageAlignFile
    globalset theColorMode texture

}


proc doneclicking {} {
    global imageactive;
    set imageactive 0
    catch {exec killall displayclick}
    globalset shortIAF ""
    globalset curImageAlignFile ""
    ia_clearCorrList
    ia_updateUI
}

proc establishname {} {
    global xyztuples
    set types {
	{{Image Files}       {.rgb}}
	{{PNG Files}         {.png}}
    }


    set filename [tk_getOpenFile -defaultextension .rgb -filetypes $types]
    if {$filename != ""} {
	# Kill old images
	doneclicking

	# Open new image file
	globalset curImageAlignFile $filename
	globalset shortIAF [file tail $filename]
	set xyztuples ""
#	exec echo " " > [glob ~/.tmp2dpoints]
#	load5tuples
	ia_clearCorrList
	ia_loadCorrFile
	opendisplay
	ia_updateUI
    }
}

proc opendisplay {} {

    global curImageAlignFile
    global xyztuples
    global imageactive
    global displayscalevar

    if {($curImageAlignFile != "") && ($imageactive != 1)} {
	exec displayclick -geometry $displayscalevar{%} $curImageAlignFile > [glob ~/.tmp2dpoints] &
	globalset xyztuples ""
	globalset imageactive 1
	idlerefresh
    }
}

proc loadlocalcolor {} {
    global curImageAlignFile
    set meshfile [file rootname $curImageAlignFile].cs.set
#    set scanname [file rootname $curImageAlignFile].cs
    set scanname [readfile $meshfile]
#   puts "name:$scanname"
    selectScan $scanname
    showOnlyMesh $scanname
    globalset theColorMode true
}

proc loadallcolor {} {
    global curPasteMesh
    set meshfile [file rootname $curPasteMesh].cs.set
    set scanname [readfile $meshfile]
#    puts "name:$scanname"
    selectScan $scanname
    showOnlyMesh $scanname
    globalset theColorMode true
}

proc dopastecolor {} {
    global curPasteMesh
    global curImageAlignFile
    global env
    set tsaifile [file rootname $curImageAlignFile].tsai
    set meshfile [file rootname $curImageAlignFile].cs.ply
    set meshcfile [file rootname $curImageAlignFile].c.ply


    if {[file exists $curPasteMesh]  && [file exists $tsaifile]} {
	exec /bin/rm -f $meshfile $meshcfile
    	exec $env(SCANALYZE_DIR)/imagealign/tsai2cply $curPasteMesh $tsaifile &
	}

}

proc dopasteallcolor {} {
    global curPasteMesh
    global curImageAlignFile
    global env
    set tsaifile [glob [file dirname $curImageAlignFile]/*.tsai]

    set meshfile [file rootname $curPasteMesh].cs.ply
    set meshcfile [file rootname $curPasteMesh].c.ply


    if {[file exists $curPasteMesh]} {
	exec /bin/rm -f $meshfile $meshcfile
    	exec $env(SCANALYZE_DIR)/imagealign/tsai2mergecply $curPasteMesh $tsaifile &
	}

}

proc setpastemesh {} {

    set types {
	{{Ply Files}       {.ply}}
    }

    set filename [tk_getOpenFile -defaultextension .ply -filetypes $types]
    if {$filename != ""} {
	globalset curPasteMesh $filename
	globalset shortPM [file tail $filename]

    }
}




proc savexyzuv {} {

puts obsolete
return

    global curImageAlignFile
    global fivetuples
    set newfile [file rootname $curImageAlignFile].xyzuv
    if [file exists $newfile] {
	exec date >> $newfile.bak
	exec cat $newfile >> $newfile.bak
    }
    if [file exists $curImageAlignFile] {
	exec echo $fivetuples > $newfile
	puts "$newfile saved"
    }
}

proc createtsai {} {
    global curImageAlignFile
    global env
    global cameraType
    set xyzuvfile [file rootname $curImageAlignFile].corr
    set tsaifile [file rootname $curImageAlignFile].tsai
    set rootname [file rootname $curImageAlignFile]
    if [file exists $tsaifile] {
	exec date >> $tsaifile.bak
	exec cat $tsaifile >> $tsaifile.bak
    }
    if [file exists $curImageAlignFile] {
#	exec echo "path ('/mich4/uvdavid/matlab',path); feval('find_extrinsics','$rootname') ; disp('$tsaifile created.')" | matlab  &
	exec $env(SCANALYZE_DIR)/imagealign/xyzuv2tsai -o -c $cameraType $xyzuvfile &
	puts "Creating $tsaifile ..."

    }
}

proc idlerefresh {} {
    global imageactive
    #load5tuples
    ia_checkNewUVpoints
    ia_checkAutoZooming
    if {$imageactive == 1} {
	after 1000 idlerefresh
    }
}

proc load5tuples {} {
	global fivetuples
	global xyztuples

#make a 5tuple file
    exec echo $xyztuples > [glob ~/.xyztuples]
    exec paste [glob ~/.xyztuples] [glob  ~/.tmp2dpoints] > [glob ~/.fivetuples]

#load 5tuple file
    set f [open [glob ~/.fivetuples]]
    set fivetuples [read $f 10000]
    close $f

}

proc addxyzpoint {xyz u v} {
    global imageactive
    ia_globals
# Check to make sure the imagealignment window is open
# check to make sure we are taking correspondances now
# check finally that we got a numeric result and not "hit background"
    if [winfo exists .imagealign] {
	if {$imageactive == 1 && [scan $xyz "%f %f %f" foo1 foo2 foo3]}  {
	    set cs [.imagealign.points curselection]
	    if {$cs != ""} {
		set ia_xyzPoints [lreplace $ia_xyzPoints $cs $cs $xyz]
		ia_updateScanName $cs $u $v
	#	ia_updateSweepPoint $cs
		ia_updateUI
		ia_setPointSelection [expr $cs + 1]

	    }
	}
    }
}

proc ia_updateScanName {num u v} {
    ia_globals

    plv_pickscan init
    set scanname [plv_pickscan get $u $v]
    puts $scanname
    set ia_scanName [lreplace $ia_scanName $num $num $scanname]
    plv_pickscan exit

    set ia_fileName [lreplace $ia_fileName $num $num \
			 [plv_get_scan_filename $scanname]]
}

proc ia_updateSweepPoint {num} {
    ia_globals

    set scanname [lindex $ia_scanName $num]
    set world [lindex $ia_xyzPoints $num]
    puts $scanname
    puts $world
    set sweepStr [eval plv_worldCoordToSweepCoord $scanname $world]
    set ia_sweepPoints [lreplace $ia_sweepPoints $num $num $sweepStr]
}

proc ia_deleteSelectedPoint {} {
    set cs [.imagealign.points curselection]
    if {$cs != ""} {
	ia_deleteCorrListItem $cs
    }
    ia_updateUI
}

proc ia_setPointSelection {arg} {
    .imagealign.points selection clear 0 end


    .imagealign.points selection set $arg

}

proc ia_checkNewUVpoints {} {
    global ia_uvPointsFileLen
    ia_globals

    # Set the file length to 0 to start with
    if {![info exists ia_uvPointsFileLen]} {set ia_uvPointsFileLen 0}

    # Read the current state of the file
    set buf ""
    set f [open [glob ~/.tmp2dpoints]]
    while {[gets $f line] >= 0} {
	lappend buf $line
    }
    close $f

    # If the current file is longer, add  new points to correlation list
    if {[llength $buf] > $ia_uvPointsFileLen} {
	# Add new points
	for {set i $ia_uvPointsFileLen} {$i < [llength $buf]} {incr i} {
	    ia_addCorrListItem [lindex $buf $i]
	}


	# set the selection to the first item
	set firstEmpty [lsearch -exact $ia_xyzPoints ""]
	#puts $firstEmpty
	ia_setPointSelection $firstEmpty

	# Update UI
	ia_updateUI
    }

    # Keep a record of the current file length
    set ia_uvPointsFileLen [llength $buf]
}

proc ia_loadCorrFile {} {

    ia_globals
    global curImageAlignFile
    set newfile [file rootname $curImageAlignFile].corr

    if {![file exists $newfile]} return

    set fid [open $newfile]
    while { ![eof $fid] } {
     	gets $fid line

	scan $line "%c" firstchar
	if {$firstchar == "35"} {continue}

     	set uv ""
     	set xyz ""
     	set scanname ""
     	set filename ""
     	set sweepPoints ""
     	set x ""
     	set y ""
    	set z ""
     	set u ""
     	set v ""
     	if {[ scan $line "%f %f %f %f %f" j1 j2 j3 j4 j5 ] == 5} {
     	    # We have xyz data
     	    scan $line "%f %f %f %f %f %s %s " u v x y z scanname filename
     	} else {
     	    # We dont have xyz data
     	    scan $line "%f %f  " u v
     	}
       	if { $z != "" } {
    	    set xyz "$x $y $z "
    	}

 	if { $v != "" } {
    	    set uv "$u $v "
	    ia_addCorrListItem "$uv" "$xyz" "$scanname" "$filename" "$sweepPoints"
	}
    }

   close $fid
}


proc ia_saveCorrFile {} {
    ia_globals
    global curImageAlignFile
    set newfile [file rootname $curImageAlignFile].corr
    if [file exists $newfile] {
	exec date >> $newfile.bak
	exec cat $newfile >> $newfile.bak
    }
    if [file exists $curImageAlignFile] {
	exec echo "\# xyzPoints uvPoints scanName fileName sweepPoints" > $newfile
	for {set i 0} {$i < [llength $ia_uvPoints]} {incr i} {

	    set str [ia_buildCorrStr $i]
	    exec echo $str >> $newfile
	}
    }
}

proc ia_buildCorrStr {i} {
    ia_globals
    set uvPoint [lindex $ia_uvPoints $i]
    set xyzPoint [lindex $ia_xyzPoints $i]

    set str ""

    if {$xyzPoint == ""} {
	set xyzStr [format "       "  ]
    } else {
	set xyzStr [format "%7.1f %7.1f %7.1f    " [lindex $xyzPoint 0] [lindex $xyzPoint 1] [lindex $xyzPoint 2]]
    }

    set uvStr [format "%5.1f %5.1f " [lindex $uvPoint 0] [lindex $uvPoint 1]]
    set scanStr "[lindex $ia_scanName $i]   "
    set fileStr "[lindex $ia_fileName $i]   "
    set sweepStr [lindex $ia_sweepPoints $i]

    set str [format "%s%s%s%s%s" $xyzStr $uvStr $scanStr $fileStr $sweepStr]
    return $str
}

proc ia_updateUI {} {
    ia_globals

    set sel [.imagealign.points curselection]
    .imagealign.points delete 0 end
    for {set i 0} {$i < [llength $ia_uvPoints]} {incr i} {

	set str [ia_buildCorrStr $i]
	.imagealign.points insert end $str
    }

    if {$sel!=""} {
	.imagealign.points selection set $sel
    } else {
	set firstEmpty [lsearch -exact $ia_xyzPoints ""]
	puts $firstEmpty
	ia_setPointSelection $firstEmpty
    }
}

proc ia_deleteCorrListItem {item} {
    ia_globals

    set ia_uvPoints [ldelete $ia_uvPoints $item  ]
    set ia_xyzPoints [ldelete $ia_xyzPoints $item ]
    set ia_scanName [ldelete $ia_scanName $item ]
    set ia_fileName [ldelete $ia_fileName $item ]
    set ia_sweepPoints [ldelete $ia_sweepPoints $item ]
}

proc ia_setCorrListItem {} {
}

proc ia_addCorrListItem {uvPoint {xyzPoint ""} {scanName ""} {fileName ""} {sweepPoint ""} } {

global ia_uvPoints ia_xyzPoints ia_scanName ia_fileName ia_sweepPoints

lappend ia_uvPoints $uvPoint
lappend ia_xyzPoints $xyzPoint
lappend ia_scanName $scanName
lappend ia_fileName $fileName
lappend ia_sweepPoints $sweepPoint

}

proc ia_globals {} {
    uplevel {
	globalinit ia_uvPoints
	globalinit ia_xyzPoints
	globalinit ia_scanName
	globalinit ia_fileName
	globalinit ia_sweepPoints
	globalinit ia_autoZooming
	globalinit ia_windowSizeMultiplier
	globalinit ia_lastZoomPoint
	globalinit ia_zoomOn
	globalinit ia_zoomPickingOn
	globalinit ia_zoomResChangeOn
	globalinit ia_zoomSingleVisOn
    }
}

proc ia_clearCorrList {} {
    ia_globals

    set ia_uvPoints ""
    set ia_xyzPoints ""
    set ia_scanName ""
    set ia_fileName ""
    set ia_sweepPoints ""
}

proc ia_reload {} {
global env

source $env(SCANALYZE_DIR)/imagealign.tcl

}

proc ia_checkAutoZooming {} {
    ia_globals

    # If we have autozooming turned on

    if {$ia_autoZooming == 1} {

	ia_zoomToFeature
    }

}

proc ia_zoomToFeature {} {
    ia_globals
    global renderOnlyActiveMesh

    # If there is a current selection
    if [winfo exists .imagealign] {
	set cs [.imagealign.points curselection]
	if {$cs != ""} {
	    # Get the uv coord of the current selection
	    set uvPoint [lindex $ia_uvPoints $cs]

	    # Check that we know the correct sizes
	    if {$ia_windowSizeMultiplier == ""} {
		puts "Zooming makes no sense without tsai alignment"
		return
	    }

	    if {$uvPoint != $ia_lastZoomPoint} {
		# Set this as the current point
		set ia_lastZoomPoint $uvPoint

		# Fix this point to account for current window size
		set uPoint [lindex $uvPoint 0]
		set uPoint [expr $uPoint * $ia_windowSizeMultiplier]
		set vPoint [lindex $uvPoint 1]
		set vPoint [expr $vPoint * $ia_windowSizeMultiplier]

		# Restore the global view (handled by ia_)
		set renderOnlyActiveMesh 0
		vg_RestoreState TSAI_POSITION

		# Set the zooming base (as seen by zoom_)
		zoomSetBaseView

		# Zoom to the new view
		if {$ia_zoomOn==1} {
		    savecam previousCameraView
		    eval "zoomNow 40 $uPoint $vPoint"
		}

		# Pick selected mesh on zoom?
		if {$ia_zoomPickingOn==1 || $ia_zoomSingleVisOn==1 || $ia_zoomResChangeOn==1} {
		    plv_pickscan init
		    set selected [plv_pickscan get $uPoint $vPoint]
		    plv_pickscan exit
		    if {$selected != ""} {
			globalset theMesh $selected
		    }
		}

		# Show only selected mesh on zoom?
		if {$ia_zoomSingleVisOn==1} {
		    set ia_zoomPickingOn 1
		    set renderOnlyActiveMesh 1
		}

		# Change picked mesh to highest res?
		if {$ia_zoomResChangeOn==1} {
		    set ia_zoomPickingOn 1
		    if {$selected != ""} {
			selectNthResolutionForMesh 0 $selected
		    }
		}
	    }
	}
    }
}
