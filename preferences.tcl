######################################################################
#
# preferences.tcl
# created 11/2/98  magi
#
# read/write/access preference data
#
######################################################################
#
# exports:
#   get_preference
#   set_preference
#   write_preferences
#   prefs_AutoPersistVar
#
# globals:
#   global_preferences
#   global_prefs_persistvars
#
######################################################################

set global_prefsRead 0

proc get_preference {name {default ""}} {
    global global_preferences

    # read prefs from file if they haven't been
    if {[expr ![globalset global_prefsRead]]} {
	read_preferences
    }

    if {[expr ![info exists global_preferences($name)]]} {
	set global_preferences($name) $default
    }

    return $global_preferences($name)
}


proc set_preference {name value} {
    global global_preferences

    set global_preferences($name) $value
}


proc prefs_AutoPersistVar {var {default ""}} {
    # mark given variable as persistent between sessions
    # so read its value from configuration file, and mark
    # it for write at shutdown

    # if variable is array member, we only want to global the array
    set av [getglobalname $var]

    # read in the variable
    global $av
    set $var [get_preference $var $default]

    # and mark it for later
    global global_prefs_persistvars
    lappend global_prefs_persistvars $var
}


proc prefs_UpdatePersistVars {} {
    global global_prefs_persistvars

    if {[info exists global_prefs_persistvars]} {
	foreach persistvar $global_prefs_persistvars {
	    set_preference $persistvar [globalset $persistvar]
	}
    }
}


proc read_preferences {} {
    set prefs [getPrefFilename]

    global global_preferences
    # force array to exist
    set global_preferences() ""

    # read whatever's in the file
    if {[file exists $prefs]} {
	puts "Reading settings from $prefs"
	set file [open $prefs r]

	while {[expr ![eof $file]]} {
	    set line [gets $file]
	    #puts "read: $line"

	    if {$line != ""} {
		set sep [string first = $line]
		if {$sep} {
		    set name [string range $line 0 [expr $sep - 1]]
		    set value [string range $line [expr $sep + 1] end]

		    if {$value == ""} {
			puts "Warning: ignoring blank setting $line"
		    } else {
			set global_preferences($name) "$value"
		    }
		} else {
		    puts "Malformed default: $line"
		}
	    }
	}

	close $file
    }

    # and flag that we've read the file
    globalset global_prefsRead 1
}


proc write_preferences {} {

    # open file
    set prefs [getPrefFilename]
    set file [open $prefs w]

    # update the auto-save vars
    prefs_UpdatePersistVars

    # the write out the preferences
    global global_preferences
    foreach name [array names global_preferences] {
	if {$name != ""} {
	    puts $file "$name=$global_preferences($name)"
	}
    }
    close $file
}


proc getPrefFilename {} {
    global env
    return "$env(HOME)/.scanalyzedefaults"
}


proc prefs_AutoPrefsVar {var description values default {command ""}} {
    # uses global array prefs_pref_vars
    global prefs_pref_vars

    # save as a preference
    set prefs_pref_vars($var) [list $description $values $command]
    # get the value, and mark it for auto-save
    prefs_AutoPersistVar $var $default

    # if it has a command to execute when it changes, do that now,
    # since we just read the value
    eval $command
}


proc prefs_sortByDesc {p1 p2} {
    global prefs_pref_vars
    set d1 [lindex $prefs_pref_vars($p1) 0]
    set d2 [lindex $prefs_pref_vars($p2) 0]
    return [string compare $d1 $d2]
}


proc prefsDialog {} {
    if {[window_Activate .prefs]} {return}
    set p [toplevel .prefs]
    wm title $p "Scanalyze preferences"
    window_Register $p

    global prefs_pref_vars
    foreach pref [lsort -command prefs_sortByDesc \
		      [array names prefs_pref_vars]] {
	set data $prefs_pref_vars($pref)
	set desc [lindex $data 0]
	set values [lindex $data 1]
	set command [lindex $data 2]

	if {$values == "bool"} {
	    set f [checkbutton $p.$pref -variable $pref -text $desc \
		      -command $command -anchor w]
	    set ys 1
	} else {
	    set f [frame $p.$pref -relief groove -border 2]
	    label $f.l -text $desc
	    set l2 [frame $f.l2]
	    foreach valuepair $values {
		set value [lindex $valuepair 0]
		set valdesc [lindex $valuepair 1]

		set r [radiobutton $l2.$value -text $valdesc \
			   -variable $pref -value $value \
			  -command $command]
		pack $r -side left
	    }
	    pack $f.l $l2 -side top -anchor w
	    set ys 3
	}
	pack $f -side top -anchor w -padx 3 -pady $ys -fill x
    }

    set ui [frame $p.ui]
    button $ui.save -text "Save preferences now" \
	-command write_preferences
    button $ui.close -text "Close" -command "destroy $p"
    packchildren $ui -side left -fill x -expand true
    pack $ui -side top -fill x -expand true
}


proc prefs_setRendererBasedDefaults {} {
    set r [plv_getrendererstring]

    # BUGBUG instead of defaulting to 1 and turning off for specific systems,
    # this should default off and then turn on for IR or Impact (Octane),
    # because they're the only ones it's good for.  I've yet to see a single
    # PC system that can handle z-buffer reads reasonably.  Alternatively, we
    # could implement a lighter-weight display cache that only caches frame
    # buffer and not Z buffer (which would mean anything that needs Z buffer
    # would have to trigger a redraw), since on most PC cards, frame buffer
    # reads are OK but Z buffer reads are deathly slow.
    set cache 1

    set renderer [lindex $r 0]
    if {$renderer == "CRIME"} {
	puts "O2; disabling display cache"
	set cache 0
    } elseif {$renderer == "NEWPORT"} {
	puts "Indy; disabling display cache"
	set cache 0
    } elseif {$renderer == "GDI"} {
	puts "Software renderer; disabling display cache"
	set cache 0
    } elseif {$renderer == "3Dfx/Voodoo3"} {
	puts "Voodoo3; disabling display cache; expect lighting to be broken"
	set cache 0
    } elseif {[regexp "GeForce.*" $renderer]} {
	puts "GeForce; disabling display cache"
	set cache 0
    } elseif {[regexp "Quadro.*" $renderer]} {
	puts "Quadro; disabling display cache"
	set cache 0
    } elseif {$renderer == "ELSA"} {
	puts "Elsa Nvidia card of some variety; disabling display cache"
	set cache 0
    }

    puts "render $renderer; cache $cache"

    globalset styleCacheRender $cache
}


# The following was written by jed to handle saving variables to specific
# files. Initially inteneded for contexts, but hopefully useful in general.


# Save a bunch of variables to a file. Arrays are ok as well.
proc pref_saveVarsToFile {filename args} {

    # Open the file
    set fd [open $filename w]

    # Write the vars
    foreach var $args {
	upvar $var varprox
	if {[array size varprox] == 0} {
	    # Normal var
	    set str "set $var \"$varprox\""
	    puts $fd $str
	} else {
	    # Array var
	    foreach el [array names varprox] {
		set str "set $var\($el\) \"$varprox($el)\""
		puts $fd $str
	    }
	}

    }

    # Close file
    close $fd
}

# Assuming you used the above routine to write the file, we can just source
# it to read it into tcl
proc pref_loadVarsFromFile {filename} {
    eval uplevel {
	 source $filename
    }
}