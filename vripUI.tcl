# vripUI.tcl
# front-end UI to vrip
# magi@cs.stanford.edu
# created 6/8/2000 (moved code from file.tcl)


############################################################
#
# CyberScan export to vrip
# this will be the main code path
#
############################################################

proc file_esdfvd_setRes {from resIndex} {
    if {$resIndex == ""} return

    set f .sdvr.res

    if {$from == "r"} {
	for {set i 0} {$i < 6} {incr i} {
	    if {[expr (1 << $i) / 4.0] >= $resIndex} break
	}
	set resIndex $i
    }

    if {$from != "v"} {
	$f.v delete 0 end
	$f.v insert 0 $resIndex
    }

    if {$from != "r"} {
	set res [expr (1 << $resIndex) / 4.0]
	$f.r delete 0 end
	$f.r insert 0 $res
    }

    if {$from != "s"} {
	$f.s set $resIndex
    }
}


proc file_setWidgetFromBrowseDir {toSet} {
    set dir [tk_chooseDirectory]
    if {$dir != ""} {
	$toSet delete 0 end
	$toSet insert 0 $dir
    }
}


proc file_browseButton {name toSet} {
    button $name -padx 0 -pady 0 -text "..." \
	-command "file_setWidgetFromBrowseDir $toSet"
}


proc file_exportSDForVrip {res outDir outOld subsweeps deletesweeps} {
    # create directory
    if {[catch {file mkdir $outDir/input} err]} {
	guiError "Error writing meshes for vrip: $err"
	return
    }

    # link old plys over, if necessary
    set bConfOnly 0
    if {$outOld != ""} {
	foreach ply [glob $outOld/input/*.ply] {
	    catch {exec ln -s $ply $outDir/input}
	}
	set bConfOnly 1
    }

    if {[catch {plv_write_sd_for_vrip $res $outDir/input\
		    $subsweeps $deletesweeps $bConfOnly} err]} {
	guiError "Error writing meshes for vrip: $err"
    }
}

proc file_exportSDForVripDialog {} {
    if [window_Activate .sdvr] return

    toplevel .sdvr
    wm title .sdvr "SD->ply->vrip"
    window_Register .sdvr

    set f [frame .sdvr.res]
    label $f.l -text "Res index (0-6, 0=max):"
    entry $f.v -width 2
    scale $f.s -orient horiz -from 0 -to 6 -showvalue 0 \
	-command { file_esdfvd_setRes s }
    label $f.l2 -text "Res:"
    entry $f.r -width 4
    label $f.l3 -text "mm"
    bind $f.v <KeyRelease> {file_esdfvd_setRes v [%W get]}
    bind $f.r <KeyRelease> {file_esdfvd_setRes r [%W get]}

    $f.s set 5
    packchildren $f -side left

    set f [frame .sdvr.dir]
    label $f.l -text "Output directory:"
    entry $f.v -width 30
    file_browseButton $f.b $f.v
    packchildren $f -side left

    set f [frame .sdvr.dirOld]
    label $f.l -text "Reuse plys from:"
    entry $f.v -width 30
    file_browseButton $f.b $f.v
    packchildren $f -side left

    set f [frame .sdvr.subsweeps]
    checkbutton $f.v -text "Use sub-sweeps (currently buggy!)" \
	-variable useSubSweeps -onvalue subsweeps -offvalue sweeps
    packchildren $f -side left

    checkbutton .sdvr.delsweeps -text "Delete meshes after export" \
	-variable deleteVripSweeps

    button .sdvr.go -text "Go for it" \
	-command {
	    file_exportSDForVrip [.sdvr.res.v get] [.sdvr.dir.v get] \
		[.sdvr.dirOld.v get] $useSubSweeps $deleteVripSweeps
	}

    packchildren .sdvr -side top -anchor w
}


############################################################
#
# MMS and Ply exporters
# old legacy codepaths, should be rewritten in above model
# or merged with it
############################################################


proc file_exportPlyForVripDialog {} {
    if [window_Activate .plyvr] return

    toplevel .plyvr
    wm title .plyvr "Ply->vrip"
    window_Register .plyvr

    set f [frame .plyvr.res]
    label $f.l -text "Res index (0-3, 0=max):"
    entry $f.v -width 2
    $f.v insert 0 0
    packchildren $f -side left

    set f [frame .plyvr.dir]
    label $f.l -text "Output directory:"
    entry $f.v -width 30
    packchildren $f -side left


    button .plyvr.go -text "Go" \
	-command {
	    file_exportPlyForVrip [.plyvr.res.v get] [.plyvr.dir.v get]
	}

    packchildren .plyvr -side top -anchor w
}


proc file_exportPlyForVrip {res dir} {
    set confstr [plv_write_ply_for_vrip $res $dir]
    set CONF [open "$dir/vrip.conf" a]
    foreach bmesh $confstr {
        puts $CONF $bmesh
    }
    close $CONF
}

proc file_exportMMSForVripDialog {} {
    if [window_Activate .mmsvr] return

    toplevel .mmsvr
    wm title .mmsvr "MMS->ply->vrip"
    window_Register .mmsvr

    set f [frame .mmsvr.res]
    label $f.l -text "Res index (0-5, 0=max):"
    entry $f.v -width 2
    $f.v insert 0 5
    packchildren $f -side left

    set f [frame .mmsvr.dir]
    label $f.l -text "Output directory:"
    entry $f.v -width 30
    packchildren $f -side left

    checkbutton .mmsvr.xf -text "No Z-axis align" -variable noAlign

    button .mmsvr.go -text "Go for it" \
	-command {
	    file_exportMMSForVrip [.mmsvr.res.v get] [.mmsvr.dir.v get] \
		    $noAlign
	}

    packchildren .mmsvr -side top -anchor w
}


proc file_exportMMSForVrip {res dir noAlign} {
    if {$noAlign == "1"} {
	puts "Not Aligning with Z-Axis"
	set confstr [plv_write_mm_for_vrip $res $dir "-noxform"]
    } else {
	puts "Aligning with Z-Axis for VRIP"
	set confstr [plv_write_mm_for_vrip $res $dir]
	set CONF [open "$dir/vrip.conf" a]
	foreach bmesh $confstr {
	    puts $CONF $bmesh
	}
	close $CONF
    }
}
