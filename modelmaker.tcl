proc recurBind {frameName keyName funcName} {
    foreach slave [pack slaves $frameName] {
	bind $slave $keyName $funcName
    }
    bind $frameName $keyName $funcName
}

proc recurShade {frameName newCol shouldRedraw} {
    global theMesh
    set meshNum [lindex [$frameName.label config -text] 4]

    if {$newCol == "invert"} {
	set currentCol [lindex [$frameName config -background] 4]
	if {$currentCol == "#d9d9d9"} {
	    set newCol "DarkGrey"
	} else {
	    set newCol "#d9d9d9"
	}
    }

    if {$newCol == "#d9d9d9"} {
	mms_setscanvisible $theMesh $meshNum "1"
    } else {
	mms_setscanvisible $theMesh $meshNum "0"
    }

    $frameName configure -background $newCol
    $frameName.label configure -background $newCol

    # if we want to update the poly count, we have to update the mesh's
    # resolution list... so probably not worth the trouble
    if {$shouldRedraw == "1"} {
	mms_resetcache $theMesh
	redraw 1
    }
}

proc setAllScans {midFrameName codeNum} {
    global theMesh

    foreach colSlave [pack slaves $midFrameName] {
	foreach rowSlave [pack slaves $colSlave] {
	    if {$codeNum == "0"} {
		recurShade $rowSlave "DarkGrey" 0
	    } elseif {$codeNum == "1"} {
		recurShade $rowSlave "#d9d9d9" 0
	    } else {
		recurShade $rowSlave "invert" 0
	    }
	}
    }
    mms_resetcache $theMesh
    redraw 1
}

proc deleteScan {theLabel mmWindowName} {
    global theMesh

    set numScans [mms_numscans $theMesh]
    set meshNum [lindex [$theLabel configure -text] 4]
    if {$meshNum != "--"} {
	mms_deletescan $theMesh $meshNum
	reindexButtons $mmWindowName $meshNum $numScans
	redraw 1
    }
}

proc flipScanNorms {theLabel mmWindowName} {
    global theMesh

    set numScans [mms_numscans $theMesh]
    set meshNum [lindex [$theLabel configure -text] 4]
    if {$meshNum != "--"} {
	mms_flipscannorms $theMesh $meshNum
	redraw 1
    }
}

proc reindexButtons {mmWindowName meshNum numScans} {
    set theButton 0
    set ourFrame $mmWindowName.midFrame
    foreach colSlave [pack slaves $ourFrame] {
	foreach rowSlave [pack slaves $colSlave] {
	    set theNum [lindex [$rowSlave.label configure -text] 4]
	    if {$theNum == $meshNum} {
		set theButton $rowSlave
	    } elseif {$theNum > $meshNum} {
		$rowSlave.label configure -text [expr $theNum - 1]
	    }
	}
    }
    pack forget $theButton
    set meshSel $mmWindowName.topFrame.meshSel
    $meshSel.color configure -background grey
    $meshSel.label configure -text "--"

 }

proc makeSelection {selecFrame sourceFrame} {
    set meshNum [lindex [$sourceFrame.label configure -text] 4]
    set meshCol [lindex [$sourceFrame.color configure -background] 4]
    $selecFrame.label configure -text $meshNum
    $selecFrame.color configure -background $meshCol
}

proc MM {} {
    global theMesh

    if {$theMesh == ""} {
	tk_messageBox -type ok -icon error \
		-message "No mesh selected"
	return
    }

    set mmtk [toplevel .mmtk$theMesh]

    pack [label $mmtk.meshName -text $theMesh] -side top -anchor n

    frame $mmtk.topFrame -borderwidth 5

    set meshSel [frame $mmtk.topFrame.meshSel -relief groove]

    frame $meshSel.color -width 10 -height 10 -background grey
    label $meshSel.label -text "--" -padx 3

    set buttons [frame $mmtk.topFrame.buttons -borderwidth 2]

    button $buttons.delButton -height 1 -text "Delete" -padx 0 -pady 0 \
	    -command "deleteScan $meshSel.label $mmtk"
    button $buttons.flipButton -height 1 -text "FlipNrm" -padx 0 -pady 0 \
	    -command "flipScanNorms $meshSel.label $mmtk"

    pack $buttons.delButton $buttons.flipButton -side left

    pack $meshSel.color $meshSel.label -side left

    pack $buttons $meshSel -side left -anchor w

    frame $mmtk.midFrame -borderwidth 5

    set i 0
    set numScans [mms_numscans $theMesh]
    set numCols [expr $numScans / 5.0]
    set numCols [expr round($numCols + 0.499)]
    if {$numCols == 0} {set numCols 1}
    if {$numCols > 5} {set numCols 5}
    set scansPerCol [expr round(double($numScans) / $numCols + 0.499)]
    for {set col 0} {$col < $numCols} {incr col} {
	set meshCol [frame $mmtk.midFrame.meshCol$col]
	set iBound [expr $i + $scansPerCol]
	if {$iBound >= $numScans} {set iBound $numScans}
	for {} \
		{$i < $iBound} {incr i} {
	    set meshFrame [frame $meshCol.meshFrame$i -relief groove \
		    -borderwidth 1]
	    set meshLabel [label $meshFrame.label -text $i -padx 3]
	    set colorName [mms_getscanfalsecolor $theMesh $i]
	    set meshColor [frame $meshFrame.color -width 10 -height 10 \
		    -background $colorName]
	    pack $meshColor $meshLabel -side left -anchor w

	    set commStr "makeSelection $meshSel $meshFrame"
	    recurBind $meshFrame <ButtonRelease-3> $commStr
	    set commStr "recurShade $meshFrame invert 1"
	    recurBind $meshFrame <ButtonRelease-1> $commStr
	    pack $meshFrame -side top -anchor e -fill x
	}
	pack $meshCol -side left -anchor w -fill x
    }

    frame $mmtk.botFrame -borderwidth 3

    button $mmtk.botFrame.showAll -text "ShowAll" -padx 0 -pady 0 \
     -command "setAllScans $mmtk.midFrame 1"
    button $mmtk.botFrame.hideAll -text "HideAll" -padx 0 -pady 0 \
     -command "setAllScans $mmtk.midFrame 0"
    button $mmtk.botFrame.invt    -text "Inv" -padx 0 -pady 0 \
     -command "setAllScans $mmtk.midFrame 2"

    pack $mmtk.botFrame.showAll $mmtk.botFrame.hideAll $mmtk.botFrame.invt \
	    -side left -anchor w

    pack $mmtk.topFrame $mmtk.midFrame $mmtk.botFrame -side top -anchor w

    wm title $mmtk "MMTools"

}

