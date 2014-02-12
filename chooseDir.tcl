# magi: imported to scanalyze, slightly modified
# original source http://www.wizvax.net/chrisn/TclTkProgRef/
# thanks to Christopher Nelson

# A platform-independent, pure Tcl version of tk_chooseDirectory
# for use on non-Win32 platforms.

# (magi: tk_chooseDirectory was added to Tk 8.3b2 for Win32, but the
# final Tk 8.3 has it for all platforms I think)

package require opt

namespace eval ::tkChooseDirectory {
    variable value
}

::tcl::OptProc ::tkChooseDirectory::tk_chooseDirectory {
    {-initialdir -string ""
            "Initial directory for browser"}
    {-mustexist
            "If specified, user can't type in a new directory"}
    {-parent     -string "."
            "Parent window for browser"}
    {-title      -string "Choose Directory"
            "Title for browser window"}
} {
    # Handle default directory
    if {[string length $initialdir] == 0} {
	set initialdir [pwd]
    }

    # Handle default parent window
    if {[string compare $parent "."] == 0} {
	set parent ""
    }

    set w [toplevel $parent.choosedirectory]
    wm title $w $title

    label $w.label -text "Directory:"
    grid $w.label -sticky w

    # Some frames for grouping

    frame $w.list
    grid $w.list -sticky news -pady 1m -padx 1m

    if {[llength [file volumes]]} {
	# On Macs it would be nice to add a volume combobox here.
    }

    frame $w.entry
    grid $w.entry -sticky news -pady 1m -padx 1m

    frame $w.buttons
    grid $w.buttons -sticky news -pady 1m -padx 1m

    # Only directory list expands height
    grid rowconfigure $w 1 -weight 1
    # All resize width
    grid columnconfigure $w 0 -weight 1

    #----------------------------------------
    # Fill the top frame (user entry)
    label $w.entry.l -text "Selection:"
    pack $w.entry.l -side left
    entry $w.entry.e -width 30
    pack $w.entry.e -side left -expand 1 -fill x

    $w.entry.e insert end $initialdir

    #----------------------------------------
    # Fill the middle frame (directory content list)
    listbox $w.list.lb -height 8 \
	    -yscrollcommand [list $w.list.sb set] \
	    -selectmode browse \
	    -setgrid true \
	    -exportselection 0 \
	    -takefocus 1
    scrollbar $w.list.sb -orient vertical \
	    -command [list $w.list.lb yview]

    grid $w.list.lb $w.list.sb -sticky news
    grid configure $w.list.lb -ipadx 2m
    grid rowconfigure $w.list 0 -weight 1
    grid columnconfigure $w.list 0 -weight 1

    #----------------------------------------
    # Commands for various bindings (which follow)
    set okCommand  [namespace code \
	    [list Done $w ok ::tkChooseDirectory::value($w)]]

    set cancelCommand  [namespace code \
	    [list Done $w cancel ::tkChooseDirectory::value($w)]]

    #----------------------------------------
    # Fill the bottom frame (buttons)
    button $w.buttons.ok     -width 8 -text OK \
	    -command $okCommand
    button $w.buttons.cancel -width 8 -text Cancel \
	    -command $cancelCommand
    pack $w.buttons.ok $w.buttons.cancel -side left -expand 1

    #-----------------------------------------
    # Other bindings
    # <Return> is the same as OK
    bind $w <Return> $okCommand

    # <Escape> is the same as cancel
    bind $w <Escape> $cancelCommand

    # Closing the window is the same as cancel
    wm protocol $w WM_DELETE_WINDOW $cancelCommand

    #----------------------------------------
    # Fill listbox and bind for browsing
    Refresh $w.list.lb $initialdir

    bind $w.list.lb <Double-Button-1> [namespace code \
	    [list Update $w.entry.e $w.list.lb]]

    # WUZ - need to center $w over parent

    # Set the min size when the size is known
#    tkwait visibility $w
#    tkChooseDirectory::MinSize $w

    focus $w.entry.e
    $w.entry.e selection range 0 end
    grab set $w

    # Wait for OK, Cancel or close
    tkwait window $w

    grab release $w

    set dir $::tkChooseDirectory::value($w)
    unset ::tkChooseDirectory::value($w)
    return $dir
}
# tkChooseDirectory::tk_chooseDirectory

proc ::tkChooseDirectory::MinSize { w } {
    set geometry [wm geometry $w]

    regexp {([0-9]*)x([0-9]*)\+} geometry whole width height

    wm minsize $w $width $height
}

proc ::tkChooseDirectory::Done { w why varName } {
    variable value

    switch -- $why {
	ok {
	    # If mustexist, validate with [cd]
	    set value($w) [$w.entry.e get]
	}
	cancel {
	    set value($w) ""
	}
    }

    destroy $w
}

proc ::tkChooseDirectory::Refresh { listbox dir } {
    $listbox delete 0 end

    $listbox insert end ".."
    foreach f [lsort [glob -nocomplain $dir/*]] {
	if {[file isdirectory $f]} {
	    $listbox insert end "[file tail $f]/"
	}
    }
}

proc ::tkChooseDirectory::Update { entry listbox } {
    set subdir [$listbox get [$listbox curselection]]
    if {[string compare $subdir ".."] == 0} {
	set fullpath [file dirname [$entry get]]
    } else {
	set fullpath [file join [$entry get] $subdir]
    }
    $entry delete 0 end
    $entry insert end $fullpath
    Refresh $listbox $fullpath
}

catch {rename ::tk_chooseDirectory tk_chooseDir}

proc tk_chooseDirectory { args } {
    uplevel ::tkChooseDirectory::tk_chooseDirectory $args
}
