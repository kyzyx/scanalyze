######################################################################
#
# windows.tcl       Code for managing scanalyze's window menu
# 10/29/98          magi
#
######################################################################
#
# Exports:
#   window_Register
#   window_Activate
#   window_Minimize
#
# globals:
#   menuWindow
#   window_visible
#
######################################################################



proc window_Register {window {status normal}} {
    global menuWindow
    global window_visible

    set title [wm title $window]

    set window_visible($window) 1
    $menuWindow add checkbutton -label $title \
	-variable window_visible($window) \
	-command "setWindowVisible $window"

    if {$status == "undeletable"} {
	window_MarkUndeletable $window
    } else {
	bind $window <Destroy> \
	    "+if {\"%W\"==\"$window\"} {window_Unregister $window}"
    }
}


proc window_Activate {window} {
    if {[winfo exists $window]} {
	# bring it to foreground
	# don't move the window around
	# (at least not by more than required by grid)
	set geom [wm geometry $window]
	#puts $geom
	wm withdraw $window
	wm deiconify $window
	wm geometry $window $geom
	return 1
    } else {
	return 0
    }
}


proc window_ActivateFromY {y} {
    global menuWindow

    for {set i 0} {$i < [$menuWindow index end]} {incr i} {
	if {[$menuWindow yposition $i] > $y} {
	    break
	}
    }

    #puts "found index $i"
    #puts "found item [$menuWindow entryconfig $i -command]"
    set cmd [$menuWindow entryconfig $i -command]
    set cmd [lindex $cmd 4]
    if {[lindex $cmd 0] == "setWindowVisible"} {
	set window [lindex $cmd 1]
	setWindowVisible $window 0
	window_Activate $window
    }
}


proc window_Minimize {state} {
    global window_visible
    global window_visible_restore
    global window_visible_geometry

    if {$state == "save"} {
	# minimize all registered windows, and remember what we minimized
	if {[info exists window_visible_restore]} {
	    unset window_visible_restore
	}

	foreach win [array names window_visible] {
	    set window_visible_restore($win) [wm state $win]
	    set window_visible_geometry($win) [wm geometry $win]
	    wm withdraw $win
	}
    } else {
	# restore everything that we minimized
	if {[info exists window_visible_restore]} {
	    foreach win [array names window_visible_restore] {
		if {$window_visible_restore($win) == "iconic"} {
		    wm iconify $win
		} elseif {$window_visible_restore($win) == "normal"} {
		    set geom $window_visible_geometry($win)

		    # need to extract position from geometry string
		    # (which has size and position)
		    # position could be signalled by either + or -
		    set leftp [string first + $geom]
		    set leftm [string first - $geom]
		    if {$leftp > $leftm && $leftm >= 0} {set leftp $leftm}
		    set geom [string range $geom $leftp end]

		    wm deiconify $win
		    wm geometry $win $geom
		}
	    }
	    unset window_visible_restore
	    unset window_visible_geometry
	}
    }
}


proc window_Unregister {window} {
    global menuWindow
    global window_visible

    unset window_visible($window)
    set title [wm title $window]
    $menuWindow delete $title
}


proc setWindowVisible { window {show ""}} {
    global window_visible

    if {$show != ""} {
	set window_visible($window) $show
    }

    if {$window_visible($window)} {
	wm deiconify $window
    } else {
	wm withdraw $window
    }
}


#
# Mark that this window can't be deleted -- any attempt to close it
# just hides it
#

proc window_MarkUndeletable {window} {
    wm protocol $window WM_DELETE_WINDOW \
	"setWindowVisible $window 0"
}

# Force the window manager to resize so that the rendering frame
# is the desired size
proc setMainWindowSize {width height} {
    global toglPane
    set renderingwidth [lindex [$toglPane config -width] 4]
    set renderingheight [lindex [$toglPane config -height] 4]

    scan [wm geometry .] "%dx%d" winwidth winheight
    set finalwidth [ expr " $winwidth - $renderingwidth + $width " ]
    set finalheight [ expr " $winheight - $renderingheight + $height " ]

    # scanalyze typically uses a somewhat conservative minimum size for
    # the root window; if you ask for a small rendering, we might bump
    # into this, so make sure we're allowed to do the final
    # wm geometry command.
    scan [wm minsize .] "%d %d" minw minh
    if {$minw > $finalwidth}  { set minw $finalwidth }
    if {$minh > $finalheight} { set minh $finalheight }
    wm minsize . $minw $minh

    wm geometry . [format "%sx%s" $finalwidth $finalheight ]

}


# Trys to match the desired aspect ratio
# returns a multiplier from the actual window set size to the
# requested size
proc setMainWindowAspect {width height} {
    global toglPane
    set renderingwidth [lindex [$toglPane config -width] 4]
    set renderingheight [lindex [$toglPane config -height] 4]


    set desiredAspect [expr 1.0 *  $width / $height]
    set currentAspect [expr 1.0 * $renderingwidth / $renderingheight]

    # Check if we are already the right size
    if {$desiredAspect == $currentAspect} {
	return [expr 1.0 * $renderingwidth / $width];
    }

    if {$desiredAspect > $currentAspect} {
	# Too tall right now
	set newWidth $renderingwidth
	set newHeight [expr $newWidth / $desiredAspect]
    } else {
	# Too wide right now
	set newHeight $renderingheight
	set newWidth [expr $newHeight * $desiredAspect]
    }

    set newWidth [expr int ($newWidth)]
    set newHeight [expr int( $newHeight)]
    setMainWindowSize $newWidth $newHeight

    return [expr 1.0 * $newWidth / $width]

}


proc sendImageTo {remoteDisplay} {

puts $remoteDisplay:0
plv_writeiris [globalset toglPane] /tmp/tmp.rgb
exec xv -display $remoteDisplay:0 /tmp/tmp.rgb &

}

proc remoteDisplayUI {} {
    if [window_Activate .remoteDisplay] return

    set rd [toplevel .remoteDisplay]
    wm title $rd "Remote Display"
    window_Register $rd

    global G_remoteDisplay
    set G_remoteDisplay ""

    frame $rd.name -relief groove -borderwidth 4
    label $rd.name.lab -text "Remote Display:"
    entry $rd.name.display -relief sunken -textvariable G_remoteDisplay
    pack $rd.name.lab $rd.name.display

    frame $rd.buttons -relief groove -borderwidth 4
    button $rd.buttons.addtomenu -text "Add to Menu" \
	-command {if {$G_remoteDisplay != ""} \
		      {addRemoteDisplayToMenu $G_remoteDisplay}}
    button $rd.buttons.addtomain -text "Add to Main Window" \
	-command {if {$G_remoteDisplay != ""} \
		      {addRemoteDisplayToMain $G_remoteDisplay}}
    button $rd.buttons.send -text  "Send Now" \
	-command {if {$G_remoteDisplay != ""} \
		       {sendImageTo $G_remoteDisplay}}
    pack $rd.buttons.addtomenu $rd.buttons.addtomain $rd.buttons.send -fill x

    pack $rd.name $rd.buttons
}

proc addRemoteDisplayToMain {display} {
    regsub -all {\.} "$display" "x" shortdisplay

    eval button .tools.ro.to$shortdisplay -text To$display -padx 0 -pady 0 \
	-command \"sendImageTo $display\"
    packchildren  .tools.ro -side top -expand 1 -fill x -padx 0
}

proc addRemoteDisplayToMenu {display } {
    .menubar.menuFile.remoteDisplay add command -label $display \
	-command "sendImageTo $display"
}


