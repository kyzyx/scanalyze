proc zoomInClick {x y s} {
    if {$s == 1} {
	# shift key down
	zoomSetBaseView
    }

    zoomNow 50 $x $y


}

proc zoomOutClick {x y s} {
    if { $s == 1} {
	# shift key down
	zoomRestoreBaseView
	return;
    }

    zoomNow 200 $x $y
}

proc zoomNow {{percent 50} {x ""} {y ""}} {
    # Get screen size
    global toglPane
    set renderingwidth [lindex [$toglPane config -width] 4]
    set renderingheight [lindex [$toglPane config -height] 4]

    # Figure pixel offsets
    set xoffset [expr "$renderingwidth  * $percent / 200 "]
    set yoffset [expr "$renderingheight * $percent / 200 "]

    # Check if we need screen center
    if {"$x" == ""} {set x [expr "$renderingwidth /2"]}
    if {"$y" == ""} {set y [expr "$renderingheight /2"]}

    # Figure coordinates
    set left [expr "$x - $xoffset"]
    set top [expr "$y - $yoffset"]
    set right [expr "$x + $xoffset"]
    set bottom [expr "$y + $yoffset"]

    puts "$left $top $right $bottom"

    # Can't decide if this is desirable behavior (?)
    # Set center of rotation
    #  setRotationCenter $x $y

    # Zoom
    plv_zoom_to_rect $left $top $right $bottom
}

proc zoomSetBaseView {} {
    vg_CreateView ZOOM_BASE
}

proc zoomRestoreBaseView {} {
    vg_RestoreState ZOOM_BASE
}