# console.tcl --
#
# This code constructs the console window for an application.  It
# can be used by non-unix systems that do not have built-in support
# for shells.
#
# SCCS: @(#) console.tcl 1.44 97/06/20 14:10:12
#
# Copyright (c) 1995-1996 Sun Microsystems, Inc.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

# TODO: history - remember partially written command

# tkConsoleInit --
# This procedure constructs and configures the console windows.
#
# Arguments:
# 	None.

set consoleMode normal
set prompted 1
# This allows the initial prompt to work correctly
consoleinterp eval {set tcl_prompt1 "set tcl_prompt1 \"$tcl_prompt1\"; \
  consoleinterp eval tkConsolePrompt"}

proc .wheelScroll {w s} {
  bind $w <Button-4> "$w yview scroll -1 unit"
  bind $s <Button-4> "$w yview scroll -1 unit"
  bind $w <Button-5> "$w yview scroll 1 unit"
  bind $s <Button-5> "$w yview scroll 1 unit"
}

proc tkConsoleInit {} {
    global tcl_platform

    if {! [consoleinterp eval {set tcl_interactive}]} {
	wm withdraw .
    }

    if {"$tcl_platform(platform)" == "macintosh"} {
	set mod "Cmd"
    } else {
	set mod "Alt"
    }

    frame .menubar -relief raised -bd 2
#    menu .menubar
#    .menubar add cascade -label File -menu .menubar.file -underline 0
#    .menubar add cascade -label Edit -menu .menubar.edit -underline 0

#    menu .menubar.file -tearoff 0
#    .menubar.file add command -label "Source..." -underline 0 \
#	-command tkConsoleSource
#    .menubar.file add command -label "Hide Console" -underline 0 \
#	-command {wm withdraw .}
#    if {"$tcl_platform(platform)" == "macintosh"} {
#	.menubar.file add command -label "Quit" -command exit -accel Cmd-Q
#    } else {
#	.menubar.file add command -label "Exit" -underline 1 -command exit
#    }

    menubutton .menubar.win -text Window -menu .menubar.win.menu
    menu .menubar.win.menu -tearoff 0
    .menubar.win.menu add command -label "Run Script" \
	-command {consoleinterp eval .runScript}
    .menubar.win.menu add separator
    .menubar.win.menu add command -label "View Main Window" \
	-command {consoleinterp eval view}
    .menubar.win.menu add separator
    .menubar.win.menu add command -label "Close" \
	-command tkConsoleExit
    .menubar.win.menu add command -label "Exit" \
	-command {consoleinterp eval .queryExit}

    menubutton .menubar.edit -text Edit -menu .menubar.edit.menu
    menu .menubar.edit.menu -tearoff 0
    .menubar.edit.menu add command -label "Cut"  \
	-command { event generate .console <<Cut>> } -accel "$mod+X"
    .menubar.edit.menu add command -label "Copy"  \
	-command { event generate .console <<Copy>> } -accel "$mod+C"
    .menubar.edit.menu add command -label "Paste"  \
	-command { event generate .console <<Paste>> } -accel "$mod+V"
    .menubar.edit.menu add command -label "Clear" \
	-command { event generate .console <<Clear>> }

    menubutton .menubar.help -text Help -menu .menubar.help.menu
    menu .menubar.help.menu -tearoff 0
    .menubar.help.menu add command -label "Manual..." \
	-command {consoleinterp eval manual}
    .menubar.help.menu add command -label "About..." \
	-command {consoleinterp eval .showAbout}

    pack .menubar.win .menubar.edit -side left -padx 4
    pack .menubar.help -side right -padx 4

#    . conf -menu .menubar
    pack .menubar -side top -fill x

    global .borderColor
    frame .bd -relief ridge -bd 4 -bg ${.borderColor}

    text .console  -yscrollcommand ".sb set" -setgrid true \
	-font .ConsoleFont -highlightthickness 0 -relief flat
    scrollbar .sb -command ".console yview" -width 12
    .wheelScroll .console .sb
    
    pack .sb -in .bd -side right -fill both
    pack .console -in .bd -fill both -expand 1 -side left
    pack .bd -fill both -expand 1
    if {$tcl_platform(platform) == "macintosh"} {
        .console configure -font {Monaco 9 normal} -highlightthickness 0
    }

    tkConsoleBind .console

    global .consoleActiveColor
    .console tag configure stderr -foreground red
    .console tag configure stdin -foreground ${.consoleActiveColor}

    focus .console
    
    wm protocol . WM_DELETE_WINDOW { wm withdraw . }
    wm title . "Lens Console"
    flush stdout
    .console mark set output [.console index "end - 1 char"]
    tkTextSetCursor .console end
    .console mark set promptEnd end
    .console mark gravity promptEnd left
}

# tkConsoleSource --
#
# Prompts the user for a file to source in the main interpreter.
#
# Arguments:
# None.

proc tkConsoleSource {} {
    set filename [tk_getOpenFile -defaultextension .tcl -parent . \
		      -title "Select a file to source" \
		      -filetypes {{"Tcl Scripts" .tcl} {"All Files" *}}]
    if {"$filename" != ""} {
    	set cmd [list source $filename]
	if [catch {consoleinterp eval $cmd} result] {
	    tkConsoleOutput stderr "$result\n"
	}
    }
}

proc startExecFile {} {
    global consoleMode
    set consoleMode exec
    return
}

proc stopExecFile {} {
    global consoleMode prompted
    set consoleMode normal
    tkConsolePrompt
    set prompted 1
    return
}

proc startMore {} {
    global consoleMode
    set consoleMode more
    bind .console <KeyPress> {
	if "{%A} != {{}}" {consoleinterp eval "set .moreCode {%A}"}
	break
    }
    bind .console <Return> {
	consoleinterp eval "set .moreCode -"
	break
    }
    return
}

proc stopMore {} {
    global consoleMode prompted
    set consoleMode normal
    tkConsolePrompt
    set prompted 1
    bind .console <KeyPress> {
	tkConsoleInsert %W %A
	break
    }
    bind .console <Return> {
	%W mark set insert {end - 1c}
	tkConsoleInsert %W "\n"
	tkConsoleInvoke
	break
    }
    return
}

proc removeMoreMessage {} {
    set range [.console tag prevrange stderr output]
    eval .console delete $range
    return
}

# tkConsoleInvoke --
# Processes the command line input.  If the command is complete it
# is evaled in the main interpreter.  Otherwise, the continuation
# prompt is added and more input may be added.
#
# Arguments:
# None.

proc complete cmd {
    if {![info complete $cmd]} {return 0}
    if {([string length $cmd] - [string length [string trimright $cmd \\]]) % 2 == 1} {return 0}
    return 1
}

proc tkConsoleInvoke {args} {
    global consoleMode prompted
    set ranges [.console tag ranges input]
    set cmd ""
    if {$ranges != ""} {
	set pos 0
	while {[lindex $ranges $pos] != ""} {
	    set start [lindex $ranges $pos]
	    set end [lindex $ranges [incr pos]]
	    append cmd [.console get $start $end]
	    incr pos
	}
    }
    if {$consoleMode == "normal"} {
	set cmd [string trim $cmd]
	if {$cmd == ""} {
	    tkConsolePrompt
	} elseif {[complete $cmd]} {
	    set prompted 0
	    .console mark set output end
	    .console tag delete input
#	    set code [catch {consoleinterp record .ccm.$cmd} result]
	    set code [catch {consoleinterp record $cmd} result]
	    if {$result != ""} {
		if {$code == 0} {
 		  .console insert insert "$result\n"
		} else {.console insert insert "$result\n" stderr}
	    }
	    tkConsoleHistory reset
	    if {$prompted == 0} tkConsolePrompt
	    set prompted 1
	} else {
	    tkConsolePrompt partial
	}
    } else {
	consoleinterp eval ".consoleExecWrite \"$cmd\""
	tkConsoleHistory reset
	.console mark set output end
	.console tag delete input
	tkConsolePrompt
    }
    
    .console yview -pickplace insert
}

# tkConsoleHistory --
# This procedure implements command line history for the
# console.  In general is evals the history command in the
# main interpreter to obtain the history.  The global variable
# histNum is used to store the current location in the history.
#
# Arguments:
# cmd -	Which action to take: prev, next, reset.

set histNum 1
proc tkConsoleHistory {cmd} {
    global histNum consoleMode
    if {$consoleMode != "normal"} return
    
    switch $cmd {
    	prev {
	    incr histNum -1
	    if {$histNum == 0} {
		set cmd {history event [expr [history nextid] -1]}
	    } else {
		set cmd "history event $histNum"
	    }
    	    if {[catch {consoleinterp eval $cmd} cmd]} {
    	    	incr histNum
    	    	return
    	    }
	    .console delete promptEnd end
    	    .console insert promptEnd $cmd {input stdin}
    	}
    	next {
	    incr histNum
	    if {$histNum == 0} {
		set cmd {history event [expr [history nextid] -1]}
	    } elseif {$histNum > 0} {
		set cmd ""
		set histNum 1
	    } else {
		set cmd "history event $histNum"
	    }
	    if {$cmd != ""} {
		catch {consoleinterp eval $cmd} cmd
	    }
	    .console delete promptEnd end
	    .console insert promptEnd $cmd {input stdin}
    	}
    	reset {
    	    set histNum 1
    	}
    }
}

# tkConsolePrompt --
# This procedure draws the prompt.  If tcl_prompt1 or tcl_prompt2
# exists in the main interpreter it will be called to generate the 
# prompt.  Otherwise, a hard coded default prompt is printed.
#
# Arguments:
# partial -	Flag to specify which prompt to print.

proc tkConsolePrompt {{partial normal}} {
    global consoleMode
    
    if {$consoleMode != "normal"} {
	set temp [.console index output]
	.console mark set output end
    } elseif {$partial == "normal"} {
	set temp [.console index "end - 1 char"]
	.console mark set output end
    	if [consoleinterp eval "info exists tcl_prompt1"] {
    	    consoleinterp eval "eval \[set tcl_prompt1\]"
    	} else {
    	    puts -nonewline "% "
    	}
    } else {
	set temp [.console index output]
	.console mark set output end
    	if [consoleinterp eval "info exists tcl_prompt2"] {
    	    consoleinterp eval "eval \[set tcl_prompt2\]"
    	} else {
	    puts -nonewline "> "
    	}
    }
    flush stdout
    .console mark set output $temp
    tkTextSetCursor .console end
    .console mark set promptEnd insert
    .console mark gravity promptEnd left
}

# tkConsoleBind --
# This procedure first ensures that the default bindings for the Text
# class have been defined.  Then certain bindings are overridden for
# the class.
#
# Arguments:
# None.

proc tkConsoleBind {win} {
    bindtags $win "$win Text . all"

    # Ignore all Alt, Meta, and Control keypresses unless explicitly bound.
    # Otherwise, if a widget binding for one of these is defined, the
    # <KeyPress> class binding will also fire and insert the character,
    # which is wrong.  Ditto for <Escape>.

    bind $win <Alt-KeyPress> {# nothing }
    bind $win <Meta-KeyPress> {# nothing}
    bind $win <Control-KeyPress> {# nothing}
    bind $win <Escape> {# nothing}
    bind $win <KP_Enter> {# nothing}

	bind $win <Alt-x> {event generate .console <<Cut>>}
	bind $win <Alt-c> {event generate .console <<Copy>>}
	bind $win <Alt-v> {event generate .console <<Paste>>}

    bind $win <Tab> {
	tkConsoleComplete %W
#	tkConsoleInsert %W \t
#	focus %W
	break
    }
    bind $win <Return> {
	%W mark set insert {end - 1c}
	tkConsoleInsert %W "\n"
	tkConsoleInvoke
	break
    }
    bind $win <Delete> {
	if {[%W tag nextrange sel 1.0 end] != ""} {
	    %W tag remove sel sel.first promptEnd
	} else {
	    if [%W compare insert < promptEnd] {
		break
	    }
	}
    }
    bind $win <BackSpace> {
	if {[%W tag nextrange sel 1.0 end] != ""} {
	    %W tag remove sel sel.first promptEnd
	} else {
	    if [%W compare insert <= promptEnd] {
		break
	    }
	}
    }
    foreach left {Control-a Home} {
	bind $win <$left> {
	    if [%W compare insert < promptEnd] {
		tkTextSetCursor %W {insert linestart}
	    } else {
		tkTextSetCursor %W promptEnd
            }
	    break
	}
    }
    foreach right {Control-e End} {
	bind $win <$right> {
	    tkTextSetCursor %W {insert lineend}
	    break
	}
    }
    bind $win <Control-c> {
	global consoleMode
	if {$consoleMode == "exec"} {
	    consoleinterp eval .consoleExecClose
	} elseif {$consoleMode == "more"} {
	    consoleinterp eval {set .moreCode q}
	} else {
	    consoleinterp eval "signal INT"
	}
    }

    bind $win <Control-d> {
	if [%W compare insert < promptEnd] {
	    break
	}
    }
    bind $win <Control-k> {
	if [%W compare insert < promptEnd] {
	    %W mark set insert promptEnd
	}
    }
    bind $win <Control-t> {
	if [%W compare insert < promptEnd] {
	    break
	}
    }
    bind $win <Meta-d> {
	if [%W compare insert < promptEnd] {
	    break
	}
    }
    bind $win <Meta-BackSpace> {
	if [%W compare insert <= promptEnd] {
	    break
	}
    }
    bind $win <Control-h> {
	if [%W compare insert <= promptEnd] {
	    break
	}
    }
    foreach prev {Control-p Up} {
	bind $win <$prev> {
	    tkConsoleHistory prev
	    break
	}
    }
    foreach prev {Control-n Down} {
	bind $win <$prev> {
	    tkConsoleHistory next
	    break
	}
    }
    bind $win <Insert> {
	catch {tkConsoleInsert %W [selection get -displayof %W]}
	break
    }
    bind $win <KeyPress> {
#	%W mark set promptEnd insert
	tkConsoleInsert %W %A
#	bind %W <KeyPress> {
#	    tkConsoleInsert %W %%A
#	    break
#	}
	break
    }
    foreach left {Control-b Left} {
	bind $win <$left> {
	    if [%W compare insert == promptEnd] {
		break
	    }
	    tkTextSetCursor %W insert-1c
	    break
	}
    }
    foreach right {Control-f Right} {
	bind $win <$right> {
	    tkTextSetCursor %W insert+1c
	    break
	}
    }
#    bind $win <F9> {
#	eval destroy [winfo child .]
#	if {$tcl_platform(platform) == "macintosh"} {
#	    source -rsrc Console
#	} else {
#	    source [file join $tk_library console.tcl]
#	}
#    }
    bind $win <<Cut>> {
        continue
    }
    bind $win <<Copy>> {
	if {[selection own -displayof %W] == "%W"} {
	    clipboard clear -displayof %W
	    catch {
		clipboard append -displayof %W [selection get -displayof %W]
	    }
	}
	break
    }
    bind $win <<Paste>> {
	catch {
	    set clip [selection get -displayof %W -selection CLIPBOARD]
	    set list [split $clip \n\r]
	    tkConsoleInsert %W [lindex $list 0]
	    foreach x [lrange $list 1 end] {
		%W mark set insert {end - 1c}
		tkConsoleInsert %W "\n"
		tkConsoleInvoke
		tkConsoleInsert %W $x
	    }
	}
	break
    }

    bind $win <B2-ButtonRelease> {
	catch {
	    set clip [selection get -displayof %W]
	    set list [split $clip \n\r]
	    %W mark set insert current
	    tkConsoleInsert %W [lindex $list 0]
	    foreach x [lrange $list 1 end] {
		%W mark set insert {end - 1c}
		tkConsoleInsert %W "\n"
		tkConsoleInvoke
		tkConsoleInsert %W $x
	    }
	}
	break
    }
}

# tkConsoleInsert --
# Insert a string into a text at the point of the insertion cursor.
# If there is a selection in the text, and it covers the point of the
# insertion cursor, then delete the selection before inserting.  Insertion
# is restricted to the prompt area.
#
# Arguments:
# w -		The text window in which to insert the string
# s -		The string to insert (usually just a single character)

proc tkConsoleInsert {w s} {
    if {$s == ""} {
	return
    }
    catch {
	if {[$w compare sel.first <= insert]
		&& [$w compare sel.last >= insert]} {
	    $w tag remove sel sel.first promptEnd
	    $w delete sel.first sel.last
	}
    }
    if {[$w compare insert < promptEnd]} {
	$w mark set insert end	
    }
    $w insert insert $s {input stdin}
    $w delete 0.0 "insert - 2000 lines linestart"
    $w see insert
}

proc maxPrefix {w1 w2} {
    set len [string length $w1]
    for {set i 0} {$i < $len} {incr i} {
	if {[string index $w1 $i] != [string index $w2 $i]} {
	    return [string range $w1 0 [expr $i-1]]
	}
    }
    return $w1
}

# This gives the longest initial subsequence of all elements in list
proc maxListPrefix list {
    if {[llength $list] == 0} return {}
    set longest [lindex $list 0]
    foreach word [lrange $list 1 end] {set longest [maxPrefix $longest $word]}
    return $longest
}

proc lastWord w {
    set end 0
    set char [$w get insert]
    while {$char != {} && "$char" != " "} {
	incr end
	set char [$w get "insert + $end c"]
    }
    if {$end > 0} {incr end -1}
    set start 0
    set char [$w get "insert - 1 c"]
    while {$char != {} && "$char" != " "} {
	incr start -1
	set char [$w get "insert + $start c"]
    }
    if {$start < 0} {incr start}
    return "[$w get "insert + $start c" "insert + $end c"] $start $end"
}


proc tkConsoleComplete w {
    global consoleMode
    if {$consoleMode != "normal"} return
    set last [lastWord $w]
    set word [lindex $last 0]
    if {[$w compare "insert + [lindex $last 1] c" <= promptEnd]} {
	set list [consoleinterp eval "info commands ${word}*"]
	set newword "[lindex $list 0] "
    } else {
	if [catch {set list [glob ${word}*]}] {set list {}}
	set newword "[lindex $list 0]"
	if [file isdirectory $newword] {
	    set newword "$newword/"
	} else {set newword "$newword "}
    }
    if {[llength $list] == 0} {consoleinterp eval beep; return}
    if {$word != " "} {
	$w delete "insert + [lindex $last 1] c" "insert + [lindex $last 2] c"
    }
    if {[llength $list] == 1} {
	$w insert insert $newword {input stdin}
    } else {
	$w insert insert [maxListPrefix $list] {input stdin}
	consoleinterp eval beep
    }
}

# tkConsoleOutput --
#
# This routine is called directly by ConsolePutsCmd to cause a string
# to be displayed in the console.
#
# Arguments:
# dest -	The output tag to be used: either "stderr" or "stdout".
# string -	The string to be displayed.

proc tkConsoleOutput {dest string} {
    .console insert output $string $dest
    .console delete 0.0 "insert - 2000 lines linestart"
    .console see insert
}

# tkConsoleExit --
#
# This routine is called by ConsoleEventProc when the main window of
# the application is destroyed.  Don't call exit - that probably already
# happened.  Just delete our window.
#
# Arguments:
# None.

proc tkConsoleExit {} {
    destroy .
}

# tkConsoleAbout --
#
# This routine displays an About box to show Tcl/Tk version info.
#
# Arguments:
# None.

proc tkConsoleAbout {} {
    global tk_patchLevel
    tk_messageBox -type ok -message "Tcl for Windows
Copyright \251 1996 Sun Microsystems, Inc.
Modified by Doug Rohde
Tcl [info patchlevel]
Tk $tk_patchLevel"
}

# now initialize the console

tkConsoleInit
