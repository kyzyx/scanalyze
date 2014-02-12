proc beep {} {
    # TODO: silent mode would bail here
    puts -nonewline "\07"
}


proc createNamedCascade {parent name caption {opts ""}} {
    set child [menu $parent.$name \
		   -tearoffcommand "setMenuTitle \"$caption\""]
    if {$opts == ""} {
	$parent add cascade -label $caption -menu $child
    } else {
	eval "$parent add cascade -label $caption -menu $child $opts"
    }
    return $child
}


proc globalset {var {val "~"}} {
    global [getglobalname $var]

    if {$val != "~"} {
	set $var $val
    } else {
	set $var
    }
}


proc globalexists {var} {
    global $var
    return [info exists $var]
}


proc getglobalname {var} {
    set av $var
    set arr [string first "(" $av]
    if {$arr == -1} {
	return $av
    }

    return [string range $av 0 [expr $arr - 1]]
}

proc globalinit {var {val ""}} {
    global $var
    uplevel "global $var"

    if {![info exists $var]} {
	set $var $val
    }
}

# Creates an alias for a command name
proc alias {newname oldname} {
    eval "proc $newname args \{eval \"$oldname \$args\"\}"
}

proc makeproc {name args def} {
    proc $name $args $def
}


proc getBG {widget {force no}} {
    set on [lindex [$widget config -value] 4]
    if {[globalset [lindex [$widget config -variable] 4]] == "$on"
	|| $force == "yes"} {
	set bg -selectcolor
	set ofs 4
    } else {
	set bg -activebackground
	set ofs 3
    }

    set b [lindex [$widget config $bg] $ofs]
    return $b
}


proc fixButtonColors {args} {
    # relevant entries are activebackground, background, highlightbackground
    # and activeforeground, foreground, highlightcolor, highlightthickness
    # and selectcolor

    # unselected: background, mouse makes it activebackground
    # selected: selectcolor, mouse makes it activebackground

    foreach widget $args {
	bind $widget <Enter>    { %W config -activebackground [getBG %W]}
	bind $widget <Button-1> { %W config -activebackground [getBG %W \
							       yes]}
    }
}


proc getListboxSelection {listbox} {
    set sel [$listbox cursel]
    if {[string compare $sel ""] == 0} {
	return ""
    } else {
	return [$listbox get $sel]
    }
}


proc Scroll_Set {scrollbar geoCmd offset size} {
    if {$offset != 0.0 || $size != 1.0} {
	eval $geoCmd;
	$scrollbar set $offset $size
    } else {
	set manager [lindex $geoCmd 0]
	$manager forget $scrollbar
    }
}

proc Scrolled_Listbox {f args } {
    frame $f
    listbox $f.list \
	    -yscrollcommand [list Scroll_Set $f.yscroll \
	       [list grid $f.yscroll -row 0 -column 1 -sticky ns]]
    eval {$f.list configure} $args
    scrollbar $f.yscroll -orient vertical \
	    -command [list $f.list yview]
    grid $f.list $f.yscroll -sticky news
    grid columnconfigure $f 0 -weight 1
    return $f.list
}

proc ldelete {list first {last ""}} {

    if {$last == ""} {set last $first}

    set front [lrange $list 0 [expr $first - 1]]
    set tail [lrange $list [expr $last + 1] end]

    return [concat $front $tail]
}

proc flash {var value} {
    upvar $var flashee
    set old [set flashee]
    set flashee $value
    set flashee $old
}


proc globaltrace {var access proc} {
    global $var
    trace variable $var $access $proc
}


proc packchildren {parent args} {
    foreach child [winfo children $parent] {
	eval pack $child $args
    }
}

proc verboseOptionMenu {w caption varName args} {
    upvar #0 $varName var

    menubutton $w -text $caption -indicatoron 1 -menu $w.menu \
	-relief raised -bd 2 -highlightthickness 2 -anchor c \
	-direction flush -padx 0 -pady 0
    menu $w.menu -tearoff 0
    foreach i $args {
	set val  [lindex $i 0]
	if {[llength $i] > 1} {
	    set desc [lindex $i 1]
	} else {
	    set desc $val
	}
        $w.menu add radiobutton -label $desc \
	    -variable $varName -value $val
    }
    return $w.menu
}


proc prebind {widget binding script} {
    set oldscript [bind $widget $binding]
    bind $widget $binding $script
    bind $widget $binding +$oldscript
}


proc swap {var1 var2} {
    upvar $var1 v1
    upvar $var2 v2

    set swap $v2
    set v2 $v1
    set v1 $swap
}


proc menuInvokeByName {menu name} {
    # make non-ambiguous string index out of name; otherwise, if
    # numeric, it'll be taken as a count-based index

    set v1 [string index $name 0]
    set vr [string range $name 1 end]
    set name \[$v1\]$vr

    set ind [$menu index $name]

    $menu invoke $ind
    return $ind
}


proc resetBGcolor {widget} {
    # resets a widget's background color to that of its parent
    set bg [lindex [[winfo parent $widget] config -bg] 4]
    $widget config -bg $bg
}


proc repeat {n script} {
    for {set i 0} {$i < $n} {incr i} {eval $script}
}


proc centerWindow {window} {
    wm withdraw $window
    update idletasks

    set w [winfo reqwidth $window]
    set h [winfo reqheight $window]

    set x [expr ([winfo screenwidth  $window] - $w) / 2]
    set y [expr ([winfo screenheight $window] - $h) / 2]

    wm geometry $window "+$x+$y"
    wm deiconify $window
}


proc bindCommandToAllMenuItems {menu script} {
    set n [$menu index end]
    for {set i 0} {$i <= $n} {incr i} {
	$menu entryconfigure $i -command $script
    }
}


proc vertbutton {args} {
    set next 0
    set outargs ""

    for {set i 0} {$i < [llength $args]} {incr i} {
	if {[lindex $args $i] == "-text"} {
	    lappend outargs [lindex $args $i]
	    set next 1
	} else {
	    if {$next == 1} {
		set inlabel [lindex $args $i]
		set label ""
		foreach char [split $inlabel {}] {
		    if {$label != ""} {
			set label "$label\n"
		    }
		    set label "$label$char"
		}
		lappend outargs $label
	    } else {
		lappend outargs [lindex $args $i]
	    }
	    set next 0
	}
    }

    eval button $outargs
}
