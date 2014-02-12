
# draws a new selection line
proc draw_line {x1 y1 x2 y2} {
    # clears current line
    plv_drawlineselection 0 0 start
    plv_drawlineselection 0 0 move
    plv_drawlineselection 0 0 stop

    # start drawing new line
    plv_drawlineselection $x1 $y1 start
    plv_drawlineselection $x2 $y2 move
    plv_drawlineselection $x2 $y2 stop

puts "$x1 $y1, $x2 $y2"
}

proc auto_a {spacing num fileName} {
    global toglPane

    set info [plv_getselectioninfo]

    if { [string compare [lindex $info 0] "line"] != 0 } {
	# error
	set badbox [tk_messageBox -type ok -default ok -message \
	   "Can't auto_analyze with this selection type" -icon error]
	return
    }

    # okay, can use line
    set x1 [lindex $info 1]
    set y1 [expr [lindex [$toglPane configure -height] 4]- [lindex $info 2]]
    set x2 [lindex $info 3]
    set y2 [expr [lindex [$toglPane configure -height] 4]- [lindex $info 4]]
# puts "1($x1, $y1) 2($x2, $y2)"

    # figure which of the start and end points are "smaller" and the deltas
    if { $x2 > $x1 } {
	set x0 $x1
	set dx [expr $x2 - $x1]
    } else {
	set x0 $x2
	set dx [expr $x1 - $x2]
    }
    if { $y2 > $y1 } {
	set y0 $y1
	set dy [expr $y2 - $y1]
    } else {
	set y0 $y2
	set dy [expr $y1 - $y2]
    }


# puts "($x0, $y0), d($dx, $dy)"

    if { $dx > $dy } {
	auto_analyze_help "y" $x0 $y0 $dx $spacing $num $fileName
    } else {
	if { $dx < $dy } {
	    auto_analyze_help "x" $x0 $y0 $dy $spacing $num $fileName
	} else {
	    # problem, but you shouldn't be using sloped lines
	}
    }


}


# automatically analyzes a number of cross sections
proc auto_analyze_help {axis x0 y0 len spacing num fileName} {
    global toglPane

    set origx $x0
    set origy $y0

    set windows ""
# puts "$axis $x0 $y0 $len $spacing $num"

    for {set i 0} {$i < $num} {incr i} {
	if { [string compare $axis "x"] == 0 } {
	    draw_line $x0 $y0 $x0 [expr $y0 + $len]
	    set x0 [expr $x0 + $spacing]
	} else {
	    draw_line $x0 $y0 [expr $x0 + $len] $y0
	    set y0 [expr $y0 + $spacing]
	}

	if {[catch {set win [analyze_line_depth]} msg]} {
	    puts -nonewline "Auto-line-analysis failed,"
	    puts " pass [expr 1+$i] of $num, due to:"
	    puts $msg
	    continue
	} else {
#	    puts "win: $win"
	    if { [string compare $fileName ""] != 0 } {
		set ctl ".ctl"
		set bar "_"
		if { [string compare $axis "x"] == 0} {
		    plv_export_graph_as_text $win $fileName$bar$x0$ctl
		} else {
		    plv_export_graph_as_text $win $fileName$bar$y0$ctl
		}
	    }
	}

	# not a smart fix, but it seems to work...
	redraw 1
	# don't want new window interfering with reads of frame
	# buffer, so hide it and remember that
	wm withdraw $win
	lappend windows $win
    }

    if {[string compare $fileName ""] != 0 } {
	plv_draw_analyze_lines $toglPane $axis $origx $origy \
	    $len $spacing $num
	redraw 1
	set ext ".rgb"
	plv_writeiris $toglPane $fileName$ext
	plv_clear_analyze_lines
    }

    foreach win $windows {
	wm deiconify $win
    }
}

proc wsh_AlignToPlane {} {
    global theMesh
    global toglPane

    set info [plv_getselectioninfo]

    if { [string compare [lindex $info 0] "shape"] != 0 } {
	# error
	set badbox [tk_messageBox -type ok -default ok -message \
	   "Can't auto_analyze with this selection type" -icon error]
	return
    }

    # okay, can use the shape -- only looks at first 3 points
    # doesn't error check for less!
    for {set i 1} {$i < 4} {incr i} {
	set x$i [lindex $info [expr 2 * $i - 1]]
	set y$i [lindex $info [expr 2 * $i]]

    }

#puts "1($x1, $y1) 2($x2, $y2) 3($x3, $y3)"

    wsh_align_points_to_plane $theMesh $x1 $y1 $x2 $y2 $x3 $y3
}