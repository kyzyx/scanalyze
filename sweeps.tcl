# A bunch of routines to switch from scans to sweeps and back as
# well as grouping and ungrouping of above.


# Takes the name of a scan and converts it to a bunch of sweeps
proc ss_convertScanToSweep {scanname} {

    # Get scan filename (dirname)
    set dirname [plv_get_scan_filename $scanname]

    # Verify dirname is a scanset
    if { [file isdirectory $dirname] != 1  || \
	 [string last ".sd" $dirname] != [expr [string length $dirname] - [string length ".sd"]] } {
	return;
    }

    # Delete current scan
    confirmDeleteMesh $scanname

    # Load sweeps
    read_sweeps_from_dir $dirname

    # Add context name

}

# Takes the name of a scan that is actually a sweep and finds
# all the other sweeps of this scan and converts back to a scan
proc ss_convertSweepToScan {sweepname} {
    # Find all related sweeps
    set sweeplist [ss_findRelatedSweeps $sweepname]

    # Find scan dir name
    set firstsweep [lindex $sweeplist 0]
    set filename [plv_get_scan_filename $firstsweep]
    set dirname [file dirname $filename]

    # Verify dirname is a scan set
    if { [file isdirectory $dirname] != 1  || \
	     [string last ".sd" $dirname] != [expr [string length $dirname] - [string length ".sd"]] } {
	return;
    }

    # Turn off screen update
    redraw block

    # Remove meshes from MeshList window
    removeMeshFromWindow $sweeplist

    # Delete sweeps
    foreach mesh $sweeplist {
	plv_meshsetdelete $mesh
    }

    # Load scan
    readfile $dirname

    # Turn on screen update
    redraw flush

}

# Given the name of a Sweep finds all the other sweeps in the same scan
proc ss_findRelatedSweeps {sweepname} {

    # Initial list is nothing
    set allSweeps ""

    # Find the sweep base name
    set basename [ss_findSweepBaseName $sweepname]

    # If not really a sweep return
    if {$basename == ""} {
	return;
    }

    # Loop over all meshes searching for str matches
    foreach mesh [getMeshList] {
	if {$basename == [ss_findSweepBaseName $mesh]} {
	    lappend allSweeps $mesh
	}
    }

    # return result
    return $allSweeps
}

# Find the base name of a sweep (the scan name)
proc ss_findSweepBaseName {sweepname} {

    # Return "" if this isn't in sweepname format
    set retval ""

    #find the part before the double underscore __
    set ind [string last "__" "$sweepname"]
    if { $ind != -1 } {
	set retval [string range $sweepname 0 [expr $ind - 1]]
    }

    return $retval
}


# Adds the menu item in mesh controls to do the conversion
proc ss_addConvertSweepScanToMeshControlMenu {meshMenu name} {

    set sweepFilename [plv_get_scan_filename $name]
    # Make sure its a CyberScan/Sweep
    #added this so IRIX can have convert to sweep functionaity if the extension is ..sd
    if {[string last ".sd" $sweepFilename] != [expr [string length $sweepFilename] - [string length ".sd"]]} {
	#puts returning...
	return
    }

    # Add the correct direction to the menu
    $meshMenu add separator
    if {[ss_findSweepBaseName $name] == ""} {
	$meshMenu add command -label "Convert to Sweeps" \
	    -command "ss_convertScanToSweep $name"
    } else {
	$meshMenu add command -label "Convert to Scan" \
	    -command "ss_convertSweepToScan $name"
	$meshMenu add command -label "Show scan sweeps" \
	    -command "ss_showScanSweeps $name"
	$meshMenu add command -label "Hide scan sweeps" \
	    -command "ss_hideScanSweeps $name"
    }
}

proc ss_showScanSweeps {name} {
    set sweepList [ss_findRelatedSweeps $name]
    eval "showMesh $sweepList"
}

proc ss_hideScanSweeps {name} {
    set sweepList [ss_findRelatedSweeps $name]
    eval "hideMesh $sweepList"
}