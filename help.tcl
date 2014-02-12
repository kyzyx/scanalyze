# Make binding so when we mouse over a menu
# it sets that to the current help file
proc help_bindHelpToMenus {} {
    bind Menu <<MenuSelect>> {
	help_menuSelectHelper %W
    }
}

# make bindings to that when we mouse over buttons
# those become the current help file
proc help_bindHelpToButtons {} {
    set oldScript [bind Button <Enter>]
    bind Button <Enter> " \
        $oldScript\
	help_buttonSelectHelper %W"

    set oldScript [bind Checkbutton <Enter>]
    bind Checkbutton <Enter> " \
        $oldScript\
	help_buttonSelectHelper %W"

    set oldScript [bind Radiobutton <Enter>]
    bind Radiobutton <Enter> " \
        $oldScript\
	help_buttonSelectHelper %W"

}

proc help_menuSelectHelper {curMenu} {
    help_globalVars
    if {$help_onoff} {
	set itemIndex [$curMenu index active]
	set typeName [$curMenu type active]
	# puts "$curMenu $itemIndex $typeName :"

	# Only change help on menu entries. not on leave menu event
	if { $typeName != "tearoff" && $itemIndex != "none" } {
	    set dot "."
	    set filename "help$curMenu$dot$itemIndex"
	    #puts $filename
	    help_setCurrentFile $filename
	}
    }
}

proc help_buttonSelectHelper {curButton} {
    help_globalVars
    if {$help_onoff} {
	# Explicitly ignore save button on help window
	if {$curButton != ".helpWindow.b.save" && $curButton != ".helpWindow.b.sync" } {

	    #puts "button$curButton"
	    set filename help.button$curButton
	    help_setCurrentFile $filename
	}
    }
}

# Set the directory that all these help files get stored in
proc help_setHelpDirectory {dirname} {
    help_globalVars
    set help_sourceDirectory $dirname
}

# set the current help file
# to associate help with some event
# you want to call help_setCurrentFile <some_unique_event_name>
proc help_setCurrentFile {filename} {
    help_globalVars
    set help_currentFile $filename
    help_updateWindow
}

# show the help window
proc help_showWindow {} {
    help_globalVars

    # Scanalyze window management
    if { [window_Activate .helpWindow] } return

    if { ![winfo exists .helpWindow] } {
	set helpWindow [toplevel .helpWindow]
	wm title $helpWindow "Help Window"

	#scanalyze window management
	window_Register $helpWindow

	#frame for buttons
	frame $helpWindow.b
	button $helpWindow.b.save -text "Save" \
	    -command {help_setHelpText [.helpWindow.tb.text get 1.0 end ]}
	button $helpWindow.b.sync -text "Sync CVS Versions" \
	    -command {help_syncCVS}
	label $helpWindow.b.label -text "Help File: "
	label $helpWindow.b.filename -textvariable help_currentFile

	packchildren $helpWindow.b -side left -anchor w

	# textbox
	frame $helpWindow.tb
	text $helpWindow.tb.text -relief raised -bd 2 \
	    -yscrollcommand "$helpWindow.tb.scroll set"
	scrollbar $helpWindow.tb.scroll -command "$helpWindow.tb.text yview"

	pack $helpWindow.tb.scroll -side right -fill y
	pack $helpWindow.tb.text -side left -fill x -fill y

	#pack
	packchildren $helpWindow -side bottom -anchor w -fill x -fill y
    }


}

# hide the help window
proc help_hideWindow {} {
     help_globalVars

    if { [winfo exists .helpWindow] } {
	destroy $helpWindow
	set helpWindow ""
    }
}

# update the contents of the help window
proc help_updateWindow {} {
    help_globalVars
    if { [winfo exists .helpWindow] } {
	.helpWindow.tb.text delete 1.0 end
	.helpWindow.tb.text insert end [help_getHelpText]
    }
}

# get the help text stored in the current file
proc help_getHelpText {} {

    help_globalVars

    # set up vars
    set slash "/"
    set fullpath $help_sourceDirectory$slash$help_currentFile
    set buf ""

    # load file
    if { [file exists $fullpath] } {
	#puts "File exits"
	set f [open $fullpath]
	while {![eof $f]} {
	    append buf [read $f 1000]
	}
	close $f
    }

    return $buf;
}

# set the help text stored in the current file
proc help_setHelpText {savetext} {

    help_globalVars

    set slash "/"
    set fullpath $help_sourceDirectory$slash$help_currentFile
    set buf ""

    # Check if file previously existed
    set oldFile [file exists $fullpath]
    # puts $oldFile

    # save file
    set f [open $fullpath w]
    puts $f $savetext
    close $f

    # change the mode to be readable
    exec chmod a+rw $fullpath

    # add to cvs repositroy if new
    if { $oldFile == "0" } {
	exec cvs add -m "New help file" $fullpath >& /dev/null
    }
#puts "saving"
#puts $savetext

}

# Sync the current help files with the repository
proc help_syncCVS {} {

    help_globalVars

    set allfiles [glob $help_sourceDirectory/*]

    # Add every file
    foreach f $allfiles {
	puts "Adding File: $f"
	catch {exec cvs add $f >& /dev/null}
    }
    #catch {exec cvs add $allfiles >& /dev/null}

    # Update from the CVS repository
    exec cvs update $help_sourceDirectory >& /dev/null


    # commit all the new files
    exec cvs commit -m "log" $help_sourceDirectory >& /dev/null
    #foreach f $allfiles {
#	puts "Commiting File: $f"
#	catch {exec cvs commit $allfiles >& /dev/null}
   # }




}

proc help_toggleOnOff {} {
    help_globalVars
    set help_onoff [expr 1 - $help_onoff]
}

# some global variables used in this module
proc help_globalVars {} {
    uplevel {
	globalinit help_sourceDirectory
	globalinit help_currentFile
	globalinit helpWindow
	globalinit help_onoff 1
    }
}