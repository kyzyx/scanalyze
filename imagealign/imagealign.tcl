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

    # Set default camera type
    if {![info exists cameraType]} {
	set cameraType DKCST5_2V
    }

    # Set default display scale
    if {![info exists displayscalevar]} {
	set displayscalevar 100
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
    global imageactive
    set imageactive 0

    #buttons section 1 establishing things
    button $iaw.opendisplay -text "Start Correlation" -command opendisplay
    button $iaw.establishname -text "Establish Image Name..." -command establishname
    label $iaw.curfilelabel -relief sunken -textvariable shortIAF
    label $iaw.curmeshlabel -relief sunken -textvariable shortPM
    button $iaw.setpastemesh -text "Establish paste mesh..." -command setpastemesh
    scale $iaw.displaysize -relief ridge -orient horizontal -resolution 5 -label "Image Display Scale %" -from 5 -to 100 -variable displayscalevar

    # buttons section 2 running correlation
    button $iaw.refresh -text "Refresh 5-tuples" -command load5tuples
    button $iaw.doneclicking -text "End Correlation" -command doneclicking
    button $iaw.savexyzuv -text "Create .xyzuv" -command savexyzuv
    button $iaw.createtsai -text "Create .tsai" -command createtsai
    button $iaw.dopastecolor -text "Paste Color (This Image)" -command dopastecolor

    #buttons section 3 pasting and viewing color
    button $iaw.dopasteallcolor -text "Paste Color (Merged Images)" -command dopasteallcolor
    button $iaw.loadlocalcolor -text "Load colored mesh(This Image)" -command loadlocalcolor
    button $iaw.loadallcolor -text "Load colored mesh(Merged Images)" -command loadallcolor
    button $iaw.projectivecolor -text "View Color Projection" -command projectivecolor

    # Viewpoint buttons
    button $iaw.setview -text "Zoom to .tsai camera viewpoint" -command setview
    button $iaw.undoview -text "Undo viewpoint change" -command undoview
    button $iaw.gentexrgb -text "Generate this projected .rgb" -command gentexrgb
    button $iaw.gentexrgbs -text "Generate all projected .rgbs" -command gentexrgbs

    # Seperators (ugly ones)
    label $iaw.sep1  -background "#000"
    label $iaw.sep2  -background "#000"
    label $iaw.sep3  -background "#000"

    # Camera types -- just add more to the list below as needed...
    set cams [frame $iaw.cameras]
    label $cams.l -text "Cam Type:" -anchor w
    tk_optionMenu $cams.sort cameraType \
	"DKCST5_2" \
	"DKCST5_2V" \
	"DKCST5_rawpixels" \
	"DKCST5_640x480" \
	"DKC5000"
    packchildren $cams -side left -fill x -expand 1

    # The five tuple region
    message $iaw.points -width 60c -relief sunken -textvariable fivetuples

    # Pack it all
    pack   $iaw.establishname \
	$iaw.curfilelabel $iaw.setpastemesh $iaw.curmeshlabel\
	$iaw.cameras $iaw.displaysize \
	$iaw.sep1 \
	$iaw.opendisplay $iaw.refresh $iaw.doneclicking \
	$iaw.savexyzuv $iaw.createtsai  \
	$iaw.sep2 \
	$iaw.projectivecolor \
	$iaw.dopastecolor $iaw.loadlocalcolor \
	$iaw.dopasteallcolor $iaw.loadallcolor \
	$iaw.sep3 \
	$iaw.setview $iaw.undoview $iaw.gentexrgb $iaw.gentexrgbs\
	$iaw.points -fill x

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

proc setview {} {
    global curImageAlignFile
    set tsaifile [file rootname $curImageAlignFile].tsai
    # We reset the transform before setting it to the new camera
    # because accumulated trackball junk screws things up
    savecam previousCameraView
    plv_resetxform
    plv_positioncamera $tsaifile ; redraw 1

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
}

proc establishname {} {
    global xyztuples
    set types {
	{{Image Files}       {.rgb}}
	{{PNG Files}         {.png}}
    }


    set filename [tk_getOpenFile -defaultextension .rgb -filetypes $types]
    if {$filename != ""} {
	globalset curImageAlignFile $filename
	globalset shortIAF [file tail $filename]
	set xyztuples ""
	exec echo " " > [glob ~/.tmp2dpoints]
	load5tuples
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
    set xyzuvfile [file rootname $curImageAlignFile].xyzuv
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
    load5tuples
    if {$imageactive == 1} {
	after 1000 idlerefresh
    }
}

proc load5tuples {} {
	global fivetuples
	global xyztuples

#make a 5tuple file
    exec echo $xyztuples > ~/.xyztuples
    exec paste [glob ~/.xyztuples] [glob ~/.tmp2dpoints] > ~/.fivetuples

#load 5tuple file
    set f [open ~/.fivetuples]
    set fivetuples [read $f 10000]
    close $f

}

proc addxyzpoint {xyz} {
    global imageactive
    global xyztuples
# Check to make sure the imagealignment window is open
# check to make sure we are taking correspondances now
# check finally that we got a numeric result and not "hit background"
    if [winfo exists .imagealign] {
	if {$imageactive == 1 && [scan $xyz "%f %f %f" foo1 foo2 foo3]}  {
	    set xyztuples "$xyztuples $xyz \n"
	    load5tuples
	}
    }
}
