proc readfile {args} {
    cursor watch

    without_redraw {
	set names ""
	set files [lsort [eval glob $args]]

	progress start [llength $files] "Read [llength $files] files"

	foreach filename $files {
	    if {[catch {
		if {[file extension $filename] == ".session"} {
		    scz_session load $filename
		} else {
		    puts "Reading scan $filename..."
		    set name [plv_readfile $filename]
		    addMeshToWindow $name
		    lappend names $name
		}
	    } err]} {
		puts "Scan read failed: $err"
	    }

	    progress updateinc 1
	}

	progress done
    } maskerrors

    cursor restore

    return $names
}


# Expected formats for proc loadgroup:
# $args = "group1.gp group2.gp..."
# $group = "group1.gp"
# $groupfiles = "/n/luma/mich8/practice/test/back1.sd \
#                /n/luma/mich8/practice/test/front2.sd..."
# $filesloaded = "left1 right2..."
# $meshname = "back1"


proc loadgroup {args} {
    set failed ""
    # list of all groups that could not be loaded

    foreach group $args {
	set members ""
	puts -nonewline "Loading group from file $group..."
	if {[catch {
	    set groupfiles [plv_readgroupmembers $group]
	} err]} {
	    puts "failed"
	    return 0
	} else {
	    puts "\nGroup members are $groupfiles"
	    set filesloaded [plv_listscans leaves]
	    foreach groupfile $groupfiles {
		set meshname [file rootname [file tail $groupfile]]
		set ext [file extension $groupfile]
		if { $ext == ".gp" } {
		    # i.e. the "mesh" is actually a group
		    # so we check if this group has already been loaded -
		    # if not, then we load it. This is to prevent
		    # recursively reloading a group several times
		    # as could happen with a *.gp on the command line.
		    set meshControlsList [plv_listscans groups]
		    set loaded [lsearch -exact $meshControlsList $meshname]
		    if {$loaded == -1} {
			set try [loadgroup $groupfile]
			if {!$try} {
			    set failed [concat $failed $groupfile]
			    set meshname ""
			}
		    }
		} else {
		    set loaded [lsearch -exact $filesloaded $meshname]
		    if {$loaded != -1} {
			puts "Mesh $meshname is already loaded."
		    } else {
			readfile $groupfile
		    }
		}
		set members [concat $members $meshname]
		#puts "added $meshname to members"
	    }
	    if { $members != ""} {
		group_createNamedGroup [file rootname $group] $members 0
	    } else {
		set failed [concat $failed $groupfile]
	    }
	}
    }
    if {$failed != ""} puts "Couldn't create the following groups:\n\t $failed"
    return 1

    # Do not redraw - instead use the without_redraw
    # script in scanalyze_util.tcl when calling this in order
    # to ensure that there are no unnec redraws when loading many groups
}

proc read_sweeps_from_dir {args} {
    cursor watch
    redraw block

    foreach arg $args {
	foreach dir [lsort [glob $arg]] {
	    puts "Heading for directory $dir..."

	    set stem [fileroot $dir]
	    if {[file exists $stem.xf]} {
		set matrix [xform_readFromFile $stem.xf]
	    } else {
		set matrix "1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1"
	    }

	    foreach file [lsort [glob $dir/*.sd]] {
		set sweep [plv_readfile $file]
		addMeshToWindow $sweep

		# if sweep doesn't have .xf, get rid of default vertical xform
		set sweepxf [fileroot $file].xf
		if {![file exists $sweepxf]} {
		    scz_xform_scan $sweep matrix \
			"1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1"
		}
		# and concat on directory xform
		scz_xform_scan $sweep matrix $matrix relative
	    }
	}
    }

    redraw flush
    cursor restore
}


proc openScanFile {} {
    set types {
	{{Generic ply file}       {.ply}}
	{{Multires ply set}       {.set}}
	{{ModelMaker scan}        {.cta}}
	{{New ModelMaker Format}  {.mms}}
	{{Cyberware ScanData}     {.sd}}
	{{Cyra scan}              {.pts}}
	{{Qsplat model}           {.qs}}
	{{Statue session}         {.session}}
    }

    set name [tk_getOpenFile -defaultextension .ply -filetypes $types]
    if {$name != ""} {
	readfile $name
    }
}

proc openScanDir {} {
    set dir [tk_chooseDirectory -title "Browse for .sd directory"]
    if {$dir == ""} return
    if {[file extension $dir] != ".sd"} {
	guiError "$dir is not an .sd scan directory."
	return
    }

    readfile $dir
}

proc openGroupFile {} {
    set types {
	{{Group file}       {.gp}}
    }
    set name [tk_getOpenFile -defaultextension .gp -filetypes $types]
    if {$name != ""} {
	without_redraw {
	    loadgroup $name
	} maskerrors
    }
}

proc saveScanFile {mesh} {
    if {[catch {plv_write_scan $mesh} result]} {
	if {$result == "unnamed"} {
	    return [saveScanFileAs $mesh]
	} else {
	    error $result
	    return 0
	}
    } else {
	return 1
    }
}


proc saveScanFileAs {mesh} {
    set defname [plv_get_scan_filename $mesh]
    set name [tk_getSaveFile -title "Save scan $mesh as" \
		  -initialfile $defname]
    if {$name != ""} {
	plv_write_scan $mesh $name
    }
}


proc saveVisibleScansToDir {dir} {
    if {![file exists $dir]} {
	file mkdir $dir
    }

    set DB [open "$dir/filenames" a]
    puts $DB "Saving scans loaded from [pwd]"

    foreach mesh [getVisibleMeshes] {
	set oldname [plv_get_scan_filename $mesh]
	set ext [file extension $oldname]
	set newname "$dir/${mesh}.$ext"

	if {[catch {plv_write_scan $mesh $newname} err]} {
	    puts "Can't write $newname: $err"
	    puts $DB "FAILED to write $oldname as $newname"
	} else {
	    puts $DB "$oldname saved as $newname"
	}
    }

    close $DB
}


proc saveVisibleScans {} {
    set dir [askQuestion "What directory to save to?"]
    if {$dir != ""} {
	saveVisibleScansToDir $dir
    }
}


proc saveScanResolutionMeshAs {mesh {extended 0}} {
    set options ""
    if {$extended != 0} {
	set options [file_getXformOptions]
    }

    if {$options != "cancel"} {
	set res [plv_getcurrentres $mesh]
	set name [tk_getSaveFile -title "Save $res-poly mesh of $mesh as"]
	if {$name != ""} {
	    plv_write_resolutionmesh $mesh $res $name $options
	}
    }
}

proc saveScanAsPlySet {mesh} {
    set dir [tk_getSaveFile -title "Save plyset of $mesh as"]
    if {$dir != ""} {
	saveScanAsPlySetToDir $mesh $dir
    }
}

proc exportAnalysisAsText { togl winName } {
#    set dir [tk_getSaveFile -title "Export analysis data to text" -initialfile $winName]
    set dir [tk_getSaveFile -title "Export analysis data to text"]
    set ext ".ctl"
    if { $dir != "" } {
	plv_export_graph_as_text $togl $dir$ext
    }
}


proc saveAllScansAsPlySets {} {
    # Added by lucas, to write out all cyra scans as plysets
    puts "Saving all scans as ply sets..."

    set errors ""
    foreach mesh [plv_listscans leaves] {
	if {[catch {saveScanAsPlySetToDir $mesh $mesh} err]} {
	    set errors "$mesh: $err\n$errors"
	}
	# link xform inside directory so set file can see it if
	# the .xf exists...
	set xfname "$mesh.xf"
	if {[file readable $xfname]} {
	    if {[file readable $mesh/$xfname]} {
		exec rm $mesh/$xfname
	    }
	    exec ln -s "../$xfname" $mesh/$xfname
        }

    }

    if {$errors != ""} {
	tk_messageBox -title "Scanalyze: saveAllScansAsPlySets failed." \
	    -message "Errors occurred while writing plysets:\n\n$errors"
	return 0
    }

    return 1
}

proc saveScanAsPlySetToDir {mesh dir} {
    # broken off from saveScanAsPlySet so that saveAllScansAsPlySets
    # can jump into it directly
    if {$dir != ""} {
	file mkdir $dir

	set slash [string last "/" $dir]
	set name [string range $dir [expr 1 + $slash] end]
	set SET [open "${dir}/${name}.set" "w"]

	set reslist [plv_getreslist $mesh]
	puts $SET "NumMeshes = [llength $reslist]"
	puts $SET "DefaultRes = [extractRes [lindex $reslist end]]"

	foreach res $reslist {
	    set res [extractRes $res]
	    set ply "${name}_${res}.ply"
	    if {[catch {plv_write_resolutionmesh \
			    $mesh $res ${dir}/${ply}} err]} {
		puts "Can't write res $res: $err";
	    } else {
		puts $SET "noload $res $ply"
	    }
	}

	close $SET
    }
}


proc saveScanMetaData {mesh data} {
    plv_write_metadata $mesh $data
}


proc file_getXformOptions {} {
    global __xfo

    set xfo [_file_getXformOptions __xfo]
    grab set $xfo
    tkwait window $xfo

    set temp $__xfo
    unset __xfo
    return $temp
}


proc _file_getXformOptions {result} {
    toplevel .xfo
    wm title .xfo "Save xform'd mesh"
    wm resizable .xfo 0 0

    label .xfo.l -text "Transformation to apply to mesh vertices:" \
	-anchor w

    frame .xfo.xf -relief groove -border 2
    globalset $result ""
    radiobutton .xfo.xf.none -text "None" -variable $result -val ""
    radiobutton .xfo.xf.flat -text "Mesh's transform (flatten)" \
	-variable $result -val "flatten"
    radiobutton .xfo.xf.mat  -text "Matrix:" \
	-variable $result -val "matrix"

    frame .xfo.xf.omatf
    frame .xfo.xf.omatf.space -width 15
    set mat [frame .xfo.xf.omatf.mat -relief groove -border 3]
    for {set i 0} {$i < 4} {incr i} {
	set f [frame $mat.row$i]
	for {set j 0} {$j < 4} {incr j} {
	    set e [entry $f.c$j -width 4]
	    if {$i == $j} {
		$e insert 0 1.0
	    } else {
		$e insert 0 0.0
	    }
	}
	packchildren $f -side left
    }

    packchildren .xfo.xf.omatf.mat -side top
    packchildren .xfo.xf.omatf -side left -pady 2

    packchildren .xfo.xf -side top -anchor w

    frame .xfo.go
    button .xfo.go.go -text OK \
	-command "_file_GFXO_update $result $mat; destroy .xfo"
    button .xfo.go.no -text Cancel \
	-command "set $result cancel; destroy .xfo"
    packchildren .xfo.go -side left -fill x -expand 1

    packchildren .xfo -side top -anchor w -fill x -expand 1 -pady 3
    return .xfo
}


proc _file_GFXO_update {result mat} {
    global $result

    if {[set $result] == "matrix"} {
	for {set j 0} {$j < 4} {incr j} {
	    for {set i 0} {$i < 4} {incr i} {
		set entry $mat.row$i.c$j
		set val [$entry get]
		lappend $result " $val"
	    }
	}
    }
}


proc fileWriteAllScanXforms {} {
    set errors ""
    foreach mesh [plv_listscans] {
	if {[catch {saveScanMetaData $mesh xform} err]} {
	    set errors "$mesh: $err\n$errors"
	}
    }

    if {$errors != ""} {
	tk_messageBox -title "Scanalyze: xform write failed." \
	    -message "Errors occurred while writing xforms:\n\n$errors"
	return 0
    }

    return 1
}

proc fileWriteVisibleScanXforms {} {
    set errors ""
    foreach mesh [getVisibleMeshes] {
	if {[catch {saveScanMetaData $mesh xform} err]} {
	    set errors "$mesh: $err\n$errors"
	}
    }

    if {$errors != ""} {
	tk_messageBox -title "Scanalyze: xform write failed." \
	    -message "Errors occurred while writing xforms:\n\n$errors"
	return 0
    }

    return 1
}

proc fileSession {access} {
    set types {
	{{Statue session}       {.session}}
    }

    if {$access == "open"} {
	set mode Open
	set impex load
    } else {
	set mode Save
	set impex save
    }

    set name [tk_get${mode}File -defaultextension .session -filetypes $types\
		  -title "$impex statue session"]
    if {$name != ""} {
	scz_session $impex $name
    }
}


proc saveScreenDump {{sourceTogl ""}} {
    set types {
	{{Iris image file}       {.rgb}}
    }

    if {$sourceTogl == ""} {
	set sourceTogl [globalset toglPane]
    }

    set name [tk_getSaveFile -defaultextension .rgb -filetypes $types\
		  -title "Save current rendering as" -parent $sourceTogl]

    if {"$name" != ""} {
	redraw
	if {[catch {plv_writeiris $sourceTogl $name} err]} {
	    tk_messageBox -type ok -icon error \
		-message "Could not save image to $name:\n$err"
	} else {
	    puts "Image written to $name"
	}
    }
}


proc writeply args {

    global meshFrame

    set len [llength $args]
    if {$len < 1 || $len > 3} {
	puts "writeply: wrong number of args"
	return
    }

    set meshName [lindex $args 0]
    set names [array names meshFrame]
    set meshExists [lsearch $names $meshName]

    if {$meshExists < 0} {
	puts "writeply: mesh not found"
	return
    }


    if {[llength $args] > 1} {
       set filename [lindex $args 1]
       set theRest [lreplace $args 0 1]
    } else {
       set filename $meshName
    }

    set filelist $filename

    set root [fileroot $filename]
    set ext [file extension $filename]
    set i 2
    for {set i 2} {$i <= 4} {incr i} {
        set nextName ${root}_res${i}${ext}
        lappend filelist $nextName
    }

    set filesExist 0
    foreach x $filelist {
	if {[file readable $x]} {
	    set filesExist 1
	}
    }

    if {$filesExist} {
	puts ""
	puts -nonewline "File(s) already exist.  OK to overwrite? (y/n) "
	flush stdout
	gets stdin doit
	if {!($doit == "y")} {
	    puts "No files written."
	    return
	}

    }

    set command [concat plv_writeply $meshName 4 $filelist $theRest]

    eval $command
}


proc readcyb args {
    global toglPane

    set name [lindex $args 0]

    set dontUseTexture 0
    if {[lindex $args 1] == "-notex"} {
       set dontUseTexture 1
    }

    if {[lindex $args 1] == "-head"} {
	plv_param -maxlen 100
    }

    if {![file readable $name]} {
        puts "Could not read file $name."
        return
    }

    if {!$dontUseTexture} {
       set cybTextureName ${name}.color
       set finalTextureName ${name}.rgb

       if {![file readable $cybTextureName] && \
	     ![file readable $finalTextureName]} {
	  set noTexture 1
       } else {
	  set noTexture 0
       }

       if {[file readable $finalTextureName]} {
	  set time1 [file mtime $name]
	  set time2 [file mtime $finalTextureName]
	  if {$time1 > $time2} {
	     set makeNewTexture 1
	  } else {
	     set makeNewTexture 0
	  }
       } else {
	  set makeNewTexture 1
       }
    } else {
       set noTexture 1
    }

    if {$noTexture} {
	set name [plv_readcyb $name]
    } elseif {!$makeNewTexture} {
	set name[plv_readcyb $name -tex $finalTextureName]
    } else {
	puts "Creating new texture map $finalTextureName..."

	if {1} {

	    # All of this attempts to make up for crummy color
	    # from standard ee from the MS platform

	    exec iflip $cybTextureName /usr/tmp/__temp.rgb 90
	    exec chmod 666 /usr/tmp/__temp.rgb
	    exec /usr/graphics/bin/fixcybcolor \
		    /usr/tmp/__temp.rgb /usr/tmp/__temp2.rgb
	    exec chmod 666 /usr/tmp/__temp2.rgb
	    exec greyscale /usr/tmp/__lin7.bw 1 7 10
	    exec chmod 666 /usr/tmp/__lin7.bw
	    exec convolve /usr/tmp/__temp2.rgb /usr/tmp/__temp.rgb /usr/tmp/__lin7.bw
	    exec hipass3 /usr/tmp/__temp.rgb /usr/tmp/__temp2.rgb 0.35
	    exec iflip /usr/tmp/__temp2.rgb $finalTextureName 180
	    exec rm /usr/tmp/__temp.rgb
	    exec rm /usr/tmp/__temp2.rgb

	} else {

	    # All of this attempts to make up for crummy color
	    # from standard ee from the PS platform

	    exec iflip $cybTextureName /usr/tmp/__temp.rgb 270
	    exec chmod 666 /usr/tmp/__temp.rgb
	    exec /usr/graphics/bin/fixcybcolor \
		    /usr/tmp/__temp.rgb $finalTextureName
	    exec rm /usr/tmp/__temp.rgb
	}

	set name [plv_readcyb $name -tex $finalTextureName]
    }

    addMeshToWindow $name

    plv_viewall $toglPane

    plv_draw $toglPane
}


proc writeimage {filename} {
    plv_writeiris [globalset toglPane] $filename
}


proc writeimage2 {filename} {
    global toglPane

    set photo [image create photo tempPhoto]
    plv_fillphoto $toglPane $photo
    $photo write $filename -format ppm
    image delete $photo
}


proc writeppmframe {name count} {

    # Set up frame count suffix
    if {$count < 10} {
	set suffix 000${count}
    } elseif {$count < 100} {
	set suffix 00${count}
    } elseif {$count < 1000} {
	set suffix 0${count}
    } else {
	set suffix ${count}
    }

    # Write out IRIS rgb image
    writeimage ${name}${suffix}.rgb

    # Convert to ppm for mpeg usage
    exec /usr/common/bin/toppm ${name}${suffix}.rgb ${name}${suffix}.ppm

    # Remove the IRIS rgb file
    exec /bin/rm ${name}${suffix}.rgb
}


proc writeirisframe {name count} {

    # Set up frame count suffix
    if {$count < 10} {
	set suffix 000${count}
    } elseif {$count < 100} {
	set suffix 00${count}
    } elseif {$count < 1000} {
	set suffix 0${count}
    } else {
	set suffix ${count}
    }

    # Write out IRIS rgb image
    writeimage ${name}${suffix}.rgb
}

proc writebwframe {name count} {

    # Set up frame count suffix
    if {$count < 10} {
	set suffix 000${count}
    } elseif {$count < 100} {
	set suffix 00${count}
    } elseif {$count < 1000} {
	set suffix 0${count}
    } else {
	set suffix ${count}
    }

    # Write out IRIS rgb image
    writeimage /usr/tmp/__temp.rgb
    exec tobw /usr/tmp/__temp.rgb ${name}${suffix}.rgb
}

proc writejpgframe {name count} {

    # Set up frame count suffix
    if {$count < 10} {
	set suffix 000${count}
    } elseif {$count < 100} {
	set suffix 00${count}
    } elseif {$count < 1000} {
	set suffix 0${count}
    } else {
	set suffix ${count}
    }

    # Write out IRIS rgb image
    writeimage /usr/tmp/__temp.rgb

    # Write out IRIS rgb image
    exec tojpg -q 90 /usr/tmp/__temp.rgb ${name}${suffix}.jpg
}


proc file_locateScan {basename} {
    # search current directory, and all 1-level subdirectories
    # for scan with recognized extension and given basename.

    # current dir first
    foreach dir {. *} {
	set possibilities [glob -nocomplain $dir/$basename.*]
	foreach scan $possibilities {
	    switch -exact -- [file extension $scan] {
		.sd -
		.ply -
		.set -
		.pts -
		.mms -
		.cta {return $scan}
	    }
	}
    }

    # failed.
    return ""
}


proc fileroot {name} {
    # this is like file root, but file root doesn't like double periods
    # ie, [file root bogus..sd] returns bogus, and while it's not pretty,
    # we need bogus. so we can append .xf and get bogus..xf, not bogus.xf.

    set l [string last . $name]
    if {$l == -1} {
	return $name
    } else {
	return [string range $name 0 [expr $l - 1]]
    }
}
