######################################################################
#
# xform.tcl
# created 2/7/99  magi, wsh
#
# manage ui for manually specifying 3d transformations
#
######################################################################



proc matrixXformUI {} {
    global __xfo

    set xfd [_matrixXformDialog __xfd]
    grab set $xfd
    tkwait window $xfd
}


proc _matrixXformDialog {result} {
    toplevel .xfo
    wm title .xfo "Specify transform for mesh [globalset theMesh]"
    wm resizable .xfo 0 0

    label .xfo.l -text "Transformation matrix:" \
	-anchor w
    set to [createMatrixEntry [frame .xfo.tomat]]
    radiobutton .xfo.abs -text "Absolute" -anchor w \
	-variable _matxform_mode -value abs
    radiobutton .xfo.rel -text "Relative to current" -anchor w \
	-variable _matxform_mode -value relcur
    radiobutton .xfo.relto -text "Relative to:" -anchor w\
	-variable _matxform_mode -value relto
    globalset _matxform_mode abs

    set rel [createMatrixEntry [frame .xfo.relmat]]

    frame .xfo.go
    button .xfo.go.go -text OK \
	-command {
	    set mat [getMatrixEntries .xfo.tomat]
	    if {$_matxform_mode == "abs"} {
		scz_xform_scan [globalset theMesh] matrix $mat
	    } elseif {$_matxform_mode == "relcur"} {
		scz_xform_scan [globalset theMesh] matrix $mat relative
	    } else {
		set mat2 [getMatrixEntries .xfo.relmat]
		scz_xform_scan [globalset theMesh] matrix $mat $mat2
	    }

	    destroy .xfo
	}
    button .xfo.go.no -text Cancel \
	-command "destroy .xfo"
    packchildren .xfo.go -side left -fill x -expand 1

    packchildren .xfo -side top -anchor w -fill x -expand 1
    return .xfo
}

# manualXFormUI
# a note about the actual transformations -- each rotation is performed
# as a seperate rotation (x, y, then z), so you'll have to undo 3 times
# if you rotate around all axes to return to the original position
proc manualXFormUI {} {
    if [window_Activate .manxform] return

    set mxf [toplevel .manxform]

    wm title $mxf "Manual Transform"
    wm resizable $mxf 0 0
    window_Register $mxf

    radiobutton $mxf.moveViewer -text "Move Viewer" \
	-variable theMover -value viewer -anchor w -padx 8

    radiobutton $mxf.moveMesh -text "Move Mesh" \
	-variable theMover -value mesh -anchor w -padx 8

    label $mxf.rr -text "Rotation (x, y, z)" -anchor w -pady 6

    frame $mxf.rot

    entry $mxf.rot.x -textvariable rx -relief sunken -width 4
    entry $mxf.rot.y -textvariable ry -relief sunken -width 4
    entry $mxf.rot.z -textvariable rz -relief sunken -width 4

    frame $mxf.tra

    label $mxf.tt -text "Translation (x, y, z)" -anchor w -pady 6
    entry $mxf.tra.x -textvariable tx -relief sunken -width 4
    entry $mxf.tra.y -textvariable ty -relief sunken -width 4
    entry $mxf.tra.z -textvariable tz -relief sunken -width 4

    button $mxf.go -text "OK" -command {
	plv_manrotate $rx $ry $rz
	plv_mantranslate $tx $ty $tz
	redraw 1
    }

    frame $mxf.line -height 3 -relief sunken -borderwidth 2

    button $mxf.rc -text "Reset camera Xform" -command {
	plv_resetxform
    }
    button $mxf.cl -text "Clear entries" -command {
	.manxform.rot.x delete 0 255;
	.manxform.rot.y delete 0 255;
	.manxform.rot.z delete 0 255;

	.manxform.tra.x delete 0 255;
	.manxform.tra.y delete 0 255;
	.manxform.tra.z delete 0 255;
    }

    pack $mxf.moveViewer -side top
    pack $mxf.moveMesh -side top

    pack $mxf.rr -anchor w -fill x -side top
    pack $mxf.rot.x $mxf.rot.y $mxf.rot.z -side left
    pack $mxf.rot -side top -anchor w -padx 20

    pack $mxf.tt -anchor w -fill x -side top
    pack $mxf.tra.x $mxf.tra.y $mxf.tra.z -side left
    pack $mxf.tra -side top -anchor w -padx 20

    pack $mxf.go -pady 2

    pack $mxf.line -side top -fill x -pady 4
    pack $mxf.rc $mxf.cl -side top -padx 2 -pady 2

    bind $mxf <KeyPress-Return> {
	plv_manrotate $rx $ry $rz
	plv_mantranslate $tx $ty $tz
	redraw 1
    }
}



proc createMatrixEntry {parent {entries ""}} {
    set mat [frame $parent.xf -relief groove -border 3]
    for {set i 0} {$i < 4} {incr i} {
	set f [frame $mat.row$i]
	for {set j 0} {$j < 4} {incr j} {
	    set ord [expr 4 * $i + $j]
	    set e [entry $f.c$j -width 6]

	    if {$ord < [llength $entries]} {
		$e insert [lindex $entries $ord]
	    } elseif {$i == $j} {
		$e insert 0 1.0
	    } else {
		$e insert 0 0.0
	    }
	}
	packchildren $f -side left
    }

    packchildren $mat -side top

    set b [frame $parent.buttons]
    button $b.ident -text "Identity" \
	-anchor w -padx 2 -pady 2 \
	-command "makeMatrixIdentity $parent"
    button $b.load -text "Load .xf file" \
	-anchor w -padx 2 -pady 2 \
	-command "browseMatrixFile $parent"
    packchildren $b -side top -anchor n -fill x

    packchildren $parent -side left

    return $mat
}


proc getMatrixEntries {mat} {
    set mat $mat.xf

    for {set j 0} {$j < 4} {incr j} {
	for {set i 0} {$i < 4} {incr i} {
	    set entry $mat.row$i.c$j
	    set val [$entry get]
	    lappend result " $val"
	}
    }

    return $result
}


proc makeMatrixIdentity {mat} {
    set mat $mat.xf

    for {set i 0} {$i < 4} {incr i} {
	set f $mat.row$i
	for {set j 0} {$j < 4} {incr j} {
	    set ord [expr 4 * $i + $j]
	    set e $f.c$j

	    $e delete 0 end
	    if {$i == $j} {
		$e insert 0 1.0
	    } else {
		$e insert 0 0.0
	    }
	}
    }
}


proc makeMatrixXffile {mat file} {
    set mat $mat.xf

    set FILE [open $file r]
    for {set i 0} {$i < 4} {incr i} {
	set line [gets $FILE]
	set f $mat.row$i

	for {set j 0} {$j < 4} {incr j} {
	    set ord [expr 4 * $i + $j]
	    set e $f.c$j

	    $e delete 0 end
	    $e insert 0 [lindex $line $j]
	}
    }

    close $FILE
}


proc browseMatrixFile {mat} {
    set file [tk_getOpenFile -parent $mat \
		  -filetypes {{{Transform file} {.xf}}} \
		  -title "Choose transform file"]

    if {$file != ""} {
	makeMatrixXffile $mat $file
    }
}


proc showCurrentScanXform {mode} {
    set mesh [globalset theMesh]
    set res [scz_get_scan_xform $mesh $mode]
    puts "Transform for $mesh:\n$res"
}


proc xform_readFromFile {file} {
    set matrix ""

    set FILE [open $file r]

    # need to transpose as we read
    for {set i 0} {$i < 4} {incr i} {
	set line [gets $FILE]

	for {set j 0} {$j < 4} {incr j} {
	    append col$j "[lindex $line $j] "
	}
    }
    close $FILE

    for {set j 0} {$j < 4} {incr j} {
	append matrix [set col$j]
    }

    return $matrix
}
