## #!/usr/bin/wish

# set fsNormalFont "-*-helvetica-bold-r-normal-*-*-100-*-*-*-*-*-*"
# set fsEntryFont  "-*-helvetica-medium-r-normal-*-*-100-*-*-*-*-*-*"

# option add *font $fsNormalFont
# option add *Entry*font $fsEntryFont
# option add *Listbox*font $fsEntryFont

# file selector box
# -----------------
# w is the name of the file selection window
# dir is the initial directory
# file is the initial file
# pattern is the initial filter
# showdot is true if you want dot files displayed initially
# allowdir is true if the result can be a directory
# allownew is true if the result can be a new filename

proc .wheelScroll {w args} {
  foreach s $args {
    bind $s <Button-4> "$w yview scroll -1 unit"
    bind $s <Button-5> "$w yview scroll 1 unit"
  }
}

proc .fileselect {w dir file pattern title showdot allowdir allownew \
		      allowpipe} {
    global _fileselect

    # local functions
    # ---------------

    # This sets up the order of tabbing
    proc settabstops {toplevel list} {
	for {set i 0} {$i < [llength $list]} {incr i} {
	    set tab1 [lindex $list $i]
	    set tab2 [lindex $list [expr ( $i + 1 ) % [llength $list]]]
	    bindtags $tab1 [list $toplevel [winfo class $tab1] $tab1]
	    bind $tab1 <Tab> "focus $tab2"
	    bind $tab2 <Shift-Tab> "focus $tab1"
	}
    }

    proc dirnormalize {dir} {
	global _fileselect
	return [.normalizeDir $_fileselect(path) $dir]
    }

    proc globmatch {file dir pattern} {
	foreach pat $pattern {
	    if [string match $dir$pat $file] {return 1}
	}
	return 0
    }

    # dir can be a full or relative path
    proc readdir {w dir file pattern} {
	global _fileselect

	set dir [dirnormalize $dir]
	if {![file readable $dir]} {
	    bell
	    return
	}
	
	if {$pattern == ""} { set pattern "*" }
	
	set _fileselect(path) $dir
	set _fileselect(file) $file
	set _fileselect(pattern) $pattern
	if {[string index $file 0] == "|"} {
	    set _fileselect(selection) $file
	} else {set _fileselect(selection) $dir$file}
	$w.filter.entry delete 0 end
	$w.filter.entry insert 0 "$pattern"
	
#        set glob_pat "$dir$pattern"
        set dot_pat  "$dir.*"

	set file_list [lsort [glob -nocomplain "$dir{.,}*"]]
	$w.lists.frame1.list delete 0 end
	$w.lists.frame2.list delete 0 end
	foreach i $file_list {
	    if {[file isdirectory $i] && ($_fileselect(showdot) ||
					  ![string match $dot_pat $i])} {
		set i [file tail $i]
		if {$i != "." && $i != ".."} {
		    $w.lists.frame1.list insert end [file tail $i]
		}
	    } elseif {[globmatch $i $dir $pattern] && ($_fileselect(showdot) ||
		![string match $dot_pat $i])} {
		    $w.lists.frame2.list insert end [file tail $i]
	    }
	}
	BuildMenu $w
    }


    # event actions
    # -------------

    proc cmd_showdot {w} {
	global _fileselect

	readdir $w . $_fileselect(file) $_fileselect(pattern)
    }

    proc cmd_filesel_filter {w} {
	readdir $w . "" [$w.filter.entry get]
    }

    proc cmd_filesel_left_dblb1 {w list} {
	global _fileselect

	if {[$list curselection] == {}} return
	if {[$list size] > 0 } {
	    readdir $w "$_fileselect(path)[$list get [$list curselection]]/" \
		    "" $_fileselect(pattern)
	}
    }

    proc cmd_filesel_right_b1 {w list} {
	global _fileselect

	if {[$list curselection] == {}} return
	if {[$list size] > 0} {
	    set _fileselect(file) [$list get [$list curselection]]
	    set _fileselect(selection) "$_fileselect(path)$_fileselect(file)"
	}
    }

    proc cmd_filesel_right_dblb1 {w list} {
	if {[$list curselection] == {}} return
	if {[$list size] > 0} {
	    $w.buttons.ok flash
	    $w.buttons.ok invoke
	}
    }

    proc cmd_selection_return {w allowdir} {
	global _fileselect
	if {!$allowdir && [file isdirectory $_fileselect(selection)]} {
	    set _fileselect(path) {}
	    readdir $w $_fileselect(selection) {} $_fileselect(pattern)
	} else {
	    $w.buttons.ok flash; $w.buttons.ok invoke
	}
    }

    proc cmd_invoke_ok {w allowdir allownew allowpipe} {
	global _fileselect
	if {$allowpipe && [string index $_fileselect(selection) 0] == "|"} {
	    set _fileselect(file) $_fileselect(selection)
	    set _fileselect(button) 0
	    return
	}
	if {![file exists $_fileselect(selection)]} {
	    if {!$allownew} {
		tk_dialog .dia {} \
			"Sorry, the selected file must already exist." \
			error 0 Ok
		return
	    }
	    set _fileselect(selection) \
		    [string trimright $_fileselect(selection) /]
	    set path [file dirname $_fileselect(selection)]
	    if {![file isdirectory $path]} {
		bell
		return
	    }
	} elseif [file isdirectory $_fileselect(selection)] {
	    if {!$allowdir} {
		tk_dialog .dia {} \
			"Sorry, the selection cannot be a directory." \
			error 0 Ok
		return
	    } else {
		set _fileselect(selection) \
			[string trimright $_fileselect(selection) /]/
	    }
	}
	set _fileselect(file) [file tail $_fileselect(selection)]
	set _fileselect(path) [file dirname $_fileselect(selection)]
	set _fileselect(button) 0
    }
	
#    proc MenuPop w {
#	global _fileselect released
#	if {$released || ($_fileselect(path) == "/")} {
#	    readdir $w . $_fileselect(file) $_fileselect(pattern)
#	} else {
#	    set b $w.lists.frame1.path
#	    set x [winfo rootx $b]
#	    set y [expr [winfo rooty $b] + [winfo height $b]]
#	    $w.menu post $x $y
#	    grab set $w.menu
#	}
#    }

    proc BuildMenu w {
	global _fileselect
	set b $w.lists.frame1.m.path
	set m $b.menu
	set path [string trimright $_fileselect(path) /]
	$m delete 0 last
	if [string length $path] {
	    $b config -text [file tail $path]
	    set path [file dirname $path]
	    while {$path != [file dirname $path]} {
		$m add command -label [file tail $path] \
		    -command "readdir $w $path {} \$_fileselect(pattern); \
			focus $w.selection.entry"
		set path [file dirname $path]
	    }
	    $m add command -label $path \
		-command "readdir $w $path {} \$_fileselect(pattern)"
	    $w.lists.frame1.m.up config -state normal
	} else {
	    $b config -text "/"
	    $w.lists.frame1.m.up config -state disabled
	}
    }

    # GUI
    # ---

    catch {destroy $w}
    toplevel $w
    wm protocol $w WM_DELETE_WINDOW { }
    wm transient $w [winfo toplevel [winfo parent $w]]
    wm title $w $title
#    wm resizable $w 0 0

#    frame $w.title -relief raised -borderwidth 1

#    label $w.title.label -text $title
#    pack $w.title.label -in $w.title -expand 1 -pady 4

    global .borderColor

    frame $w.lists

    frame $w.lists.frame1 -relief ridge -bd 4 -bg ${.borderColor}
    frame $w.lists.frame1.m -relief raised -bd 2
    menubutton $w.lists.frame1.m.path -menu $w.lists.frame1.m.path.menu \
	-anchor w
    menu $w.lists.frame1.m.path.menu -tearoff 0 -postcommand "BuildMenu $w"

    button $w.lists.frame1.m.up -image .down -bd 1 \
	-command "readdir $w .. {} \$_fileselect(pattern)"
    pack $w.lists.frame1.m.path -side left -padx 4 -ipady 0 -fill x -expand 1
    pack $w.lists.frame1.m.up -side left
#    bind $w.lists.frame1.path <ButtonPress> \
#	    "set released 0; after 50 \"MenuPop $w\""
#    bind $w.lists.frame1.path <ButtonRelease> "set released 1"

    scrollbar $w.lists.frame1.yscroll -relief sunken \
	    -command "$w.lists.frame1.list yview"
    scrollbar $w.lists.frame1.xscroll -relief sunken -orient horizontal \
	    -command "$w.lists.frame1.list xview"
    listbox $w.lists.frame1.list \
	    -yscroll "$w.lists.frame1.yscroll set" \
	    -xscroll "$w.lists.frame1.xscroll set" \
	    -relief sunken -setgrid 1 -selectmode single
    .wheelScroll $w.lists.frame1.list $w.lists.frame1.list \
	$w.lists.frame1.yscroll

#    pack $w.lists.frame1.path -in $w.lists.frame1.f -side left \
#	    -fill x -expand 1
#    pack $w.lists.frame1.up -in $w.lists.frame1.f -side right
    pack $w.lists.frame1.m -side top -fill x
    pack $w.lists.frame1.yscroll -side right -fill y
    pack $w.lists.frame1.xscroll -side bottom -fill x
    pack $w.lists.frame1.list -expand yes -fill both

    frame $w.lists.frame22 -relief ridge -bd 4 -bg ${.borderColor}
    frame $w.lists.frame2
    label $w.lists.frame2.label -text "Files:" -anchor w
    scrollbar $w.lists.frame2.yscroll -relief sunken \
	    -command "$w.lists.frame2.list yview"
    scrollbar $w.lists.frame2.xscroll -relief sunken -orient horizontal \
	    -command "$w.lists.frame2.list xview"
    listbox $w.lists.frame2.list \
	    -yscroll "$w.lists.frame2.yscroll set" \
	    -xscroll "$w.lists.frame2.xscroll set" \
	    -relief sunken -setgrid 1 -selectmode single
    .wheelScroll $w.lists.frame2.list $w.lists.frame2.list \
	 $w.lists.frame2.yscroll

    pack $w.lists.frame2.label -side top -fill x -pady 2 -ipady 2 -padx 2
    pack $w.lists.frame2.yscroll -side right -fill y
    pack $w.lists.frame2.xscroll -side bottom -fill x
    pack $w.lists.frame2.list -expand yes -fill both
    pack $w.lists.frame2 -in $w.lists.frame22 -expand yes -fill both

    pack $w.lists.frame1 -side left -fill both -expand 1
    pack $w.lists.frame22 -side right -fill both -expand 1

    frame $w.filter2 -relief ridge -bd 4 -bg ${.borderColor}
    frame $w.filter
    label $w.filter.label -width 8 -text "Filter:" -anchor w
    checkbutton $w.filter.showdot -text "Show Dot Files" \
	    -command "cmd_showdot $w" -variable _fileselect(showdot)
    entry $w.filter.entry -relief groove -bd 2
    pack $w.filter.label -side left -padx 2
    pack $w.filter.entry -side left -fill x -expand 1 -pady 1
    pack $w.filter.showdot -side right -padx 1
    pack $w.filter -in $w.filter2 -fill x

    frame $w.selection2 -relief ridge -bd 4 -bg ${.borderColor}
    frame $w.selection
    label $w.selection.label -width 8 -text "Selection:" -anchor w
    entry $w.selection.entry -textvariable _fileselect(selection) \
	-relief groove -bd 2
    pack $w.selection.label -side left -padx 2
    pack $w.selection.entry -side left -fill x -expand 1 -pady 1
    pack $w.selection -in $w.selection2 -fill x

    frame $w.buttons2 -relief ridge -bd 4 -bg ${.borderColor}
    frame $w.buttons
    button $w.buttons.ok -text "Ok" -width 10 -default active
    button $w.buttons.filter -text "Filter" -width 10
    button $w.buttons.cancel -text "Cancel" -width 10
    pack $w.buttons.ok $w.buttons.filter $w.buttons.cancel -side left \
	-expand 1 -fill x
    pack $w.buttons -in $w.buttons2 -fill x

    pack $w.lists -side top -fill both -expand 1
    pack $w.filter2 $w.selection2 $w.buttons2 -side top -fill x


    # event bindings    # --------------

    $w.buttons.ok config \
	-command "cmd_invoke_ok $w $allowdir $allownew $allowpipe"
    $w.buttons.cancel config -command "set _fileselect(button) 2"
    $w.buttons.filter config -command "cmd_filesel_filter $w"

    bind Entry <Key-Delete> "tkEntryBackspace %W"

    bind $w.lists.frame1.list <Double-Button-1> "cmd_filesel_left_dblb1 $w %W"
    bind $w.lists.frame1.list <Double-space> "cmd_filesel_left_dblb1 $w %W"
    bind $w.lists.frame2.list <Button-1> "cmd_filesel_right_b1 $w %W"
    bind $w.lists.frame2.list <space> "cmd_filesel_right_b1 $w %W"
    bind $w.lists.frame2.list <Double-Button-1> "cmd_filesel_right_dblb1 $w %W"
    bind $w.lists.frame2.list <Double-space> "cmd_filesel_right_dblb1 $w %W"

    bind $w.filter.entry <Return> \
	    "$w.buttons.filter flash; $w.buttons.filter invoke"
    bind $w.selection.entry <Return> \
	    "cmd_selection_return $w $allowdir"
    bind $w <Escape> "$w.buttons.cancel flash; $w.buttons.cancel invoke"

    settabstops $w [list $w.lists.frame1.m.path $w.lists.frame1.m.up \
	    $w.lists.frame1.list $w.lists.frame2.list $w.filter.entry \
	    $w.filter.showdot $w.selection.entry $w.buttons.ok \
	    $w.buttons.filter $w.buttons.cancel]


    # initialization
    # --------------

    set _fileselect(path) ""
    set _fileselect(file) ""
    set _fileselect(pattern) $pattern
    set _fileselect(showdot) $showdot
    set _fileselect(button) 0
    set _fileselect(selection) ""

    readdir $w $dir $file $pattern
    
    global .fileBrowserSize
    wm geometry $w ${.fileBrowserSize}
    wm minsize $w 15 10
    .placeDialog $w

    set old_focus [focus]
    grab $w
    focus $w.selection.entry
    $w.selection.entry icursor end
    tkwait variable _fileselect(button)
    grab release $w
    catch {focus $old_focus}
    destroy $w

    rename settabstops {}
    rename dirnormalize {}
    rename readdir {}
    rename cmd_showdot {}
    rename cmd_filesel_filter {}
    rename cmd_filesel_left_dblb1 {}
    rename cmd_filesel_right_b1 {}
    rename cmd_filesel_right_dblb1 {}
    rename cmd_selection_return {}
    rename cmd_invoke_ok {}
    rename BuildMenu {}

    return $_fileselect(selection)
}

# test program
# ------------

# wm withdraw .
# bind Entry <Key-Delete> "tkEntryBackspace %W"

# set dir "."
# set file "test.txt"
# set pattern "*"
# set showdot 0
# set allowdir 0
# set allownew 0
# set allowpipe 0

# fileselect .fsel $dir $file $pattern "Select A File" \
# 	$showdot $allowdir $allownew $allowpipe
# if {$fileselect(button) == 0} {
#     set dir $fileselect(path)
#     set file $fileselect(file)
#     set pattern $fileselect(pattern)
#     set showdot $fileselect(showdot)
# }
# puts "fileselect(button)   : $fileselect(button)"
# puts "fileselect(selection): $fileselect(selection)"
# puts "fileselect(path)     : $fileselect(path)"
# puts "fileselect(file      : $fileselect(file)"
# puts "fileselect(pattern)  : $fileselect(pattern)"
# puts "fileselect(showdot)  : $fileselect(showdot)"
# puts ""
# exit 0
