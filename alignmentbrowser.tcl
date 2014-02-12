proc newAlignmentBrowser {meshes} {
    # create a new window on each invocation
    set br [toplevel .alignBrowser_[getUniqueInt]]
    wm title $br "Registration pair browser -- $meshes"
    window_Register $br

    # set up scrollable pane (main) with header and footer
    ab_create3PaneWindow $br header main footer

    # now we can put whatever we want in main, header and footer
    # header: sortable column headers
    label $header.instr -text "Sort by"
    label $header.errormetrics -text "Error metrics"
    grid $header.instr - x x x $header.errormetrics - - - - -row 0
    abm_buildframe $header 1 Mesh Partner M/A Qual "#Points" \
	max avg rms pw_point pw_plane Date

    # for each pair:
    # srcName destName m/a qual errmetrics(all) ptcount date
    # TODO: overlap, other 2nd-order quality metrics?
    set i 0
    foreach mesh $meshes {
	foreach partner [plv_globalreg listpairsfor $mesh] {
	    eval abm_buildframe $main $i $mesh $partner \
		[plv_globalreg getstats $mesh $partner]
	    incr i
	}
    }

    # footer: buttons for:
    # (re)View: view those two meshes, w/respective registration
    # Edit: view, and send those two meshes to ICP
    # Delete: nuke pair
    # Grade: set quality
    # TODO: activate these
    button $footer.view -text "reView" -state disabled
    button $footer.edit -text "Edit" -state disabled
    button $footer.delete -text "Delete" -state disabled
    label $footer.ql -text "Quality:"
    set qualMenu [tk_optionMenu $footer.qual RegQuality_$br \
		      "0 - Unspecified" "1 - Bad" "2 - Fair" "3 - Good"]
    packchildren $footer -side left -fill x
    # TODO: way to select individual row
    # TODO: also need way to select range of rows based on one column value

    # and make column headers sort by that column
    set columntype {name name name num num num num num num num name}
    set nCol [lindex [grid size $header] 0]
    for {set i 0} {$i < $nCol} {incr i} {
	set widget [grid slaves $header -row 1 -column $i]
	set type [lindex $columntype $i]
	if {$type == "name"} {
	    set order "-dictionary -increasing"
	} else {
	    set order "-real -decreasing"
	}
	bind $widget <Button-1> "grid_sortRows $main $i $order"
    }
}


proc AlignmentSummaryDialog {} {
    if {[window_Activate .alignSummary]} return

    set sum [toplevel .alignSummary]
    wm title $sum "Registration pair browser -- summary"
    window_Register $sum

    # set up scrollable pane (main) with header and footer
    ab_create3PaneWindow $sum header main footer

    # now we can put whatever we want in main, header and footer
    # header:
    label $header.instr -text "Sort by"
    label $header.partners -text "\# partners"
    label $header.err -text "error"
    label $header.qual -text "quality"
    grid $header.instr $header.partners - - $header.err - - \
	$header.qual - - - -row 0
    abs_buildframe $header 1 Mesh tot man auto min avg max 0 1 2 3
    eval abs_buildframe $header 2 All [plv_globalreg getstatsummary * pnt]

    # footer:
    label $footer.instr -text "Click header to sort by that column; click mesh name to send to browser; click other entry to send matching pairs to browser"
    #button $footer.browse -text "Send to browser"
    packchildren $footer -side left -fill x -expand 1

    # main:
    set i 0
    foreach mesh [getMeshList] {
	eval abs_buildframe $main $i $mesh \
	    [plv_globalreg getstatsummary $mesh pnt]
	incr i
    }

    # and set expansion properties
    grid rowconfig $header 2 -weight 1
    #grid columnconfig $header 0 -weight 7
    #foreach index {4 5 6} {grid columnconfig $header $index -weight 2}
    #foreach index {1 2 3 7 8 9 10} {grid columnconfig $header $index -weight 1}

    # and make column headers sort by that column
    set nCol [lindex [grid size $header] 0]
    set order "-dictionary -increasing"
    for {set i 0} {$i < $nCol} {incr i} {
	set widget [grid slaves $header -row 1 -column $i]
	bind $widget <Button-1> "grid_sortRows $main $i $order"
	# and for all columns after first:
	set order "-real -decreasing"
    }

    # and make individual entry-clicks do the right thing
    set nRow [lindex [grid size $main] 1]
    for {set ii 0} {$ii < $nCol} {incr ii} {
	for {set i 0} {$i < $nRow} {incr i} {
	    set widget [grid slaves $main -row $i -column $ii]
	    bind $widget <Button-1> "abs_clickEntry $main $i $ii"
	}

	# BUGBUG, TODO:
	# need redirection table, so that abs_clickEntry row col
	# still works after resorting the rows.

	set widget [grid slaves $header -row 2 -column $ii]
	bind $widget <Button-1> "abs_clickEntry $header -1 $ii"
    }
}


proc abs_clickEntry {grid row col} {
    if {$row == -1} {
	set mesh *
	set row 2
    } else {
	set mesh [grid slaves $grid -row $row -column 0]
	set mesh [lindex [$mesh config -text] 4]
    }

    if {$col <= 1} {
	set criteria "all pairings"
    } else {
	set w [grid slaves $grid -row $row -column $col]
	set crit [string range $w [expr 1 + [string last _ $w]] end]

	if {[string range $crit 0 2] == "err"} {
	    set criteria "$crit above [lindex [$w conf -text] 4]"
	} else {
	    set criteria "$crit"
	}
    }

    # TODO
    puts "This would invoke browser for:"
    puts "Mesh = $mesh, $criteria"
}


proc abm_buildframe {parent iRow mesh partner
		     method quality pointcount
		     errMax errAvg errRms errRms_pwpoint errRms_pwplane
		     date} {
    set f $parent.pairframe_${mesh}_${partner}

    set iCol 0
    foreach widget { mesh partner
	method quality pointcount
	errMax errAvg errRms errRms_pwpoint errRms_pwplane
	date} {

	if {$iCol < 2} {set dir w} else {set dir e}

	set w [label ${f}_${widget} -text [set $widget] -anchor $dir]
	grid $w -sticky $dir -row $iRow -column $iCol
	incr iCol
    }

    foreach index {0 1} {grid columnconfig $parent $index -weight 7}
    foreach index {4} {grid columnconfig $parent $index -weight 1}
    foreach index {5 6 7 8 9} {grid columnconfig $parent $index -weight 2}
    foreach index {10} {grid columnconfig $parent $index -weight 4}
}


proc abs_buildframe {parent iRow mesh
		     total man auto errmin erravg errmax q0 q1 q2 q3} {

    set f $parent.meshframe_${mesh}

    label ${f}_mesh -text $mesh -anchor w
    grid ${f}_mesh -sticky w -row $iRow -column 0

    set iCol 1
    foreach widget {total man auto errmin erravg errmax q0 q1 q2 q3} {
	set w [label ${f}_${widget} \
		   -text [set $widget] -anchor e]
	grid $w -sticky e -row $iRow -column $iCol
	incr iCol
    }

    grid columnconfig $parent 0 -weight 7
    foreach index {4 5 6} {grid columnconfig $parent $index -weight 2}
    foreach index {1 2 3 7 8 9 10} {grid columnconfig $parent $index -weight 1}
}


# will create widgets under parent named header, footer, and main
# will also send these variables down
# you can do with these as you'd like
proc ab_create3PaneWindow {parent vHeader vMain vFooter} {
    upvar 1 $vHeader header $vMain main $vFooter footer

    set bar  [scrollbar $parent.bar -command "$parent.mover yview"]
    set mover [canvas $parent.mover -width 0 -height 0 \
		   -yscrollcommand "$parent.bar set" -border 1 -relief sunken]
    set main [frame $parent.main]
    set idMain [$mover create window 0 0 -window $main -anchor nw]
    pack $bar -side right -fill y -anchor e
    pack $mover -side left -fill both -expand 1
    bind $mover <Configure> "ab_3pw_resizeMoverChild %W $mover $idMain"

    set header [frame $parent.header]
    set footer [frame $parent.footer]
    grid $header -row 0 -sticky news
    grid $mover -row 1 -column 0 -sticky news
    grid $bar -row 1 -column 1 -sticky nse
    grid $footer - -row 2 -sticky news
    grid rowconfigure $parent 1 -weight 1
    grid columnconfigure $parent 0 -weight 1

    # and update scrollbar size.  Cool hack of the day:
    # nested "after idle" causes it to go to the end of the idle queue at
    # that point, which is necessary because if we schedule it now, it
    # the first idle callback happens before all the geometry stuff
    # happens and the window requests its height.  This way, we schedule
    # an event now that when idle (then, the geometry manager events will
    # be in queue but not processed) puts our updatesize in queue to be
    # processed after the geometry manager events.
    after idle "after idle {ab_3pw_setMoverScroll $mover $main}"
}


proc ab_3pw_setMoverScroll {mover main} {
    $mover config -scrollregion "0 0 [winfo reqw $main] [winfo reqh $main]"
}


proc ab_3pw_resizeMoverChild {from mover idMain} {
    if {$from == $mover} {
	$mover itemconfigure $idMain -width [expr [winfo width $mover] - 6]
    }
}


proc grid_sortRows {grid iColumn args} {
    set nRows [lindex [grid size $grid] 1]
    set nCols [lindex [grid size $grid] 0]

    # get info about rows, and un-grid them
    for {set i 0} {$i < $nRows} {incr i} {

	#set slaves($i) [grid slaves $grid -row $i]
	for {set i2 0} {$i2 < $nCols} {incr i2} {
	    lappend slaves($i) [grid slaves $grid -row $i -column $i2]
	}

	set widget [lindex $slaves($i) $iColumn]

	set value [lindex [$widget config -text] 4]
	lappend table [list $i $value]

	eval grid remove $slaves($i)
    }

    # sort according to given column's value
    set table [eval lsort $args -index 1 [list $table]]

    # add rows back to grid in correct order
    for {set i 0} {$i < $nRows} {incr i} {
	set iRow [lindex [lindex $table $i] 0]
	eval grid $slaves($iRow) -row $i
    }
}
