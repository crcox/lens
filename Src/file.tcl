
proc .fileWritable fullname {
    set filename [file tail $fullname]
    if {$fullname == {}} {
	tk_dialog .dia "Write Failed" "You must specify a file if you want to write to it." error 0 "Yeah, whatever"
	return 0
    }

    if [file exists $fullname] {
	if ![file writable $fullname] {
	    tk_dialog .dia "Write Failed" "Sorry, the file \"$filename\" isn't writable.  Try again." error 0 "Dern it"
	    return 0
	}
	if {[tk_dialog .dia "Save Warning" "Warning!  The file \"$filename\" already exists.  Are you sure you want to write over it?" \
		 questhead 1 "I'm totally sure" Oops] == 1} {
	    return 0
	}
    } else {
	if ![file writable [file dirname $fullname]] {
	    tk_dialog .dia "Write Failed" "Sorry, the directory \"[file dirname $fullname]\" isn't writable.  Try again." error 0 Arghh
	    return 0
	}
    }
    
    return 1
}

proc .fileReadable fullname {
    set filename [file tail $fullname]
    if ![file readable $fullname] {
	tk_dialog .dia "Read Failed" "Sorry, the file \"$fullname\" doesn't exist or isn't readable.  Try again." error 0 Ok
	return 0
    }
    return 1
}

proc .runScript {} {
    global _fileselect _script
    .fileselect .fsel $_script(path) $_script(file) $_script(pattern) \
	"Run Script" $_script(showdot) 0 0 0
    if {$_fileselect(button) != 0} return
    set _script(path) $_fileselect(path)
    set _script(file) $_fileselect(file)
    set _script(pattern) $_fileselect(pattern)
    set _script(showdot) $_fileselect(showdot)
    set file $_fileselect(selection)

    if {![.fileReadable "$file"]} {
	.runScript
	return
    }

    return [uplevel #0 source \"$file\"]
}

proc .getSetName {} {
    global .setName .done .borderColor .numExamples
    set .done -1
    set w .getset
    toplevel $w
    wm protocol $w WM_DELETE_WINDOW {set .done 1}
    wm transient $w [winfo toplevel [winfo parent $w]]
    wm title $w "Load Examples"
    frame $w.topo -relief ridge -bd 4 -bg ${.borderColor}
    frame $w.top

    button $w.top.question -bitmap questhead -relief flat -state disabled \
	-disabledforeground black
    pack $w.top.question -side left -padx 6

    label $w.top.label -text "Set in which to load examples:"
    entry $w.top.entry -width 30 -textvar .setName
    bind $w <Return> ".invokeButton $w.buttons.ok"
    pack $w.top.label $w.top.entry -padx 2

    set .numExamples "all"
    label $w.top.label2 -text "Number of examples:"
    entry $w.top.entry2 -width 30 -textvar .numExamples
    bind $w <Return> ".invokeButton $w.buttons.ok"
    pack $w.top.label2 $w.top.entry2 -padx 2

    frame $w.top.mode
    global .exmode
    set .exmode "REPLACE"
    radiobutton $w.top.mode.replace -text "Replace" -variable .exmode \
	-value "REPLACE"
    radiobutton $w.top.mode.append -text "Append" -variable .exmode \
	-value "APPEND"
    radiobutton $w.top.mode.pipe -text "Pipe" -variable .exmode \
	-value "PIPE"
    pack $w.top.mode.replace $w.top.mode.append $w.top.mode.pipe -side left
    pack $w.top.mode
    pack $w.top -in $w.topo -fill x

    frame $w.buttonso -relief ridge -bd 4 -bg ${.borderColor}
    frame $w.buttons
    button $w.buttons.ok -text Ok -width 10 -command {set .done 0} \
	-default active
    button $w.buttons.cancel -text Cancel -width 10 -command {set .done 1}
    pack $w.buttons.ok $w.buttons.cancel -side left -expand 1 -fill x
    pack $w.buttons -in $w.buttonso -fill x
    pack $w.topo $w.buttonso -fill x -expand 1


    focus $w.top.entry
    grab set $w
    $w.top.entry selection range 0 end

    .placeDialog $w

    tkwait variable .done
    destroy $w

    if ${.done} {
	return {}
    } else {
	return ${.setName}
    }
}

proc .setPath {type path} {
    global $type
    set dir [.normalizeDir {} [file dirname $path]]
    if {$dir != {}} {set ${type}(path) $dir}
    set ${type}(file) [file tail $path]
}

proc .loadExamples {} {
    global _fileselect _examples .exmode .numExamples
    .fileselect .fsel $_examples(path) $_examples(file) $_examples(pattern) \
	"Load Examples" $_examples(showdot) 0 0 1
    if {$_fileselect(button) != 0} return
    set _examples(path) $_fileselect(path)
    set _examples(file) $_fileselect(file)
    set _examples(pattern) $_fileselect(pattern)
    set _examples(showdot) $_fileselect(showdot)
    set file $_fileselect(selection)

    if {([string index $file 0] != "|") && ![.fileReadable $file]} {
	.loadExamples
	return
    }
    
    global .setName
    set .setName [.exampleSetRoot $_examples(file)]
    if {[set setname [.getSetName]] == {}} {
	return
    } else {
	if {${.numExamples} == "all"} {set .numExamples {}} \
	    else {set .numExamples "-n ${.numExamples}"}
	return [eval loadExamples $file -s $setname ${.numExamples} \
		    -m ${.exmode}]
    }
}

set .saveBinary ""
set .saveValues 1
set .saveLinks ""

proc .getSaveOptions {} {
    global .done .borderColor .ADVANCED .saveBinary .saveValues .saveLinks

    set .done -1
    set w .getsaveopt
    toplevel $w
    wm protocol $w WM_DELETE_WINDOW {set .done 1}
    wm transient $w [winfo toplevel [winfo parent $w]]
    wm title $w "Save Link Values"
    frame $w.topo -relief ridge -bd 4 -bg ${.borderColor}
    frame $w.top
    button $w.top.question -bitmap questhead -relief flat -state disabled \
	-disabledforeground black
    frame $w.top.l
    label $w.top.l.l -text "Format:"
    radiobutton $w.top.l.binary -text Binary -variable .saveBinary \
	-value ""
    radiobutton $w.top.l.text -text Text -variable .saveBinary \
	-value "-text"
    pack $w.top.l.l
    pack $w.top.l.binary $w.top.l.text -anchor w

    frame $w.top.r
    label $w.top.r.l -text "Values To Store:"
    radiobutton $w.top.r.val1 -text "Only Weights" -variable .saveValues \
	-value 1
    radiobutton $w.top.r.val2 -text "Weights & Deltas" -variable .saveValues \
	-value 2
    pack $w.top.r.l 
    pack $w.top.r.val1 $w.top.r.val2 -anchor w
    if {${.ADVANCED}} {
	radiobutton $w.top.r.val3 -text "Weights, Deltas,\n& Other Values" \
	    -variable .saveValues -value 3
	pack $w.top.r.val3 -side top -anchor w
    }

    frame $w.top.f
    label $w.top.f.l -text "Links Involved:"
    radiobutton $w.top.f.val1 -text "All Links" -variable .saveLinks \
	-value ""
    radiobutton $w.top.f.val2 -text "Only Thawed Links" -variable .saveLinks \
	-value "-noFrozen"
    radiobutton $w.top.f.val3 -text "Only Frozen Links" -variable .saveLinks \
	-value "-onlyFrozen"
    pack $w.top.f.l 
    pack $w.top.f.val1 $w.top.f.val2 $w.top.f.val3 -anchor w

    pack $w.top.question -side left -padx 6
    pack $w.top.l $w.top.r $w.top.f -side left -anchor n -padx 2
    pack $w.top -in $w.topo -fill x

    frame $w.buttonso -relief ridge -bd 4 -bg ${.borderColor}
    frame $w.buttons
    button $w.buttons.ok -text Ok -width 10 -command {set .done 0} \
	-default active
    bind $w <Return> ".invokeButton $w.buttons.ok"
    button $w.buttons.cancel -text Cancel -width 10 -command {set .done 1}
    pack $w.buttons.ok $w.buttons.cancel -side left -expand 1 -fill x
    pack $w.buttons -in $w.buttonso -fill x
    pack $w.topo $w.buttonso -fill x -expand 1
    
    focus $w.top
    grab set $w
    
    .placeDialog $w

    tkwait variable .done
    destroy $w

    if ${.done} {
	return {}
    } else {
	return "-v ${.saveValues} ${.saveBinary} ${.saveLinks}"
    }
}

proc .saveWeights {} {
    global _fileselect _weights
    
    .fileselect .fsel $_weights(path) $_weights(file) $_weights(pattern) \
	"Save Weights" $_weights(showdot) 0 1 1
    if {$_fileselect(button) != 0} return
    set _weights(path) $_fileselect(path)
    set _weights(file) $_fileselect(file)
    set _weights(pattern) $_fileselect(pattern)
    set _weights(showdot) $_fileselect(showdot)
    set file $_fileselect(selection)

    if {([string index $file 0] != "|") && ![.fileWritable $file]} {
	.saveWeights
	return
    }
    if {[set options [.getSaveOptions]] == {}} {
	return
    } else {
	return [eval saveWeights $file $options]
    }
}

set .loadLinks ""
proc .getLoadOptions {} {
    global .done .borderColor .loadLinks

    set .done -1
    set w .getloadopt
    toplevel $w
    wm protocol $w WM_DELETE_WINDOW {set .done 1}
    wm transient $w [winfo toplevel [winfo parent $w]]
    wm title $w "Load Link Values"
    frame $w.topo -relief ridge -bd 4 -bg ${.borderColor}
    frame $w.top
    button $w.top.question -bitmap questhead -relief flat -state disabled \
	-disabledforeground black
    frame $w.top.l

    frame $w.top.f
    label $w.top.f.l -text "Links Involved:"
    radiobutton $w.top.f.val1 -text "All Links" -variable .loadLinks \
	-value ""
    radiobutton $w.top.f.val2 -text "Only Thawed Links" -variable .loadLinks \
	-value "-noFrozen"
    radiobutton $w.top.f.val3 -text "Only Frozen Links" -variable .loadLinks \
	-value "-onlyFrozen"
    pack $w.top.f.l 
    pack $w.top.f.val1 $w.top.f.val2 $w.top.f.val3 -anchor w

    pack $w.top.question -side left -padx 6
    pack $w.top.f -side left -anchor n -padx 2
    pack $w.top -in $w.topo -fill x

    frame $w.buttonso -relief ridge -bd 4 -bg ${.borderColor}
    frame $w.buttons
    button $w.buttons.ok -text Ok -width 10 -command {set .done 0} \
	-default active
    bind $w <Return> ".invokeButton $w.buttons.ok"
    button $w.buttons.cancel -text Cancel -width 10 -command {set .done 1}
    pack $w.buttons.ok $w.buttons.cancel -side left -expand 1 -fill x
    pack $w.buttons -in $w.buttonso -fill x
    pack $w.topo $w.buttonso -fill x -expand 1
    
    focus $w.top
    grab set $w
    
    .placeDialog $w
    
    tkwait variable .done
    destroy $w

    if ${.done} {
	return cancel
    } else {
	return ${.loadLinks}
    }
}

proc .loadWeights {} {
    global _fileselect _weights
    
    .fileselect .fsel $_weights(path) $_weights(file) $_weights(pattern) \
	"Load Weights" $_weights(showdot) 0 0 1
    if {$_fileselect(button) != 0} return
    set _weights(path) $_fileselect(path)
    set _weights(file) $_fileselect(file)
    set _weights(pattern) $_fileselect(pattern)
    set _weights(showdot) $_fileselect(showdot)
    set file $_fileselect(selection)

    if {([string index $file 0] != "|") && ![.fileReadable $file]} {
	.loadWeights
	return
    }
    if {[set options [.getLoadOptions]] == "cancel"} {
	return
    } else {
	return [eval loadWeights $file $options]
    }
}

proc .printBrowse {} {
    global _fileselect _print .printFile
    
    .fileselect .fsel [file dirname ${.printFile}] [file tail ${.printFile}] \
	$_print(pattern) "Print To File" $_print(showdot) 0 1 0
    if {$_fileselect(button) != 0} return
    set _print(path) $_fileselect(path)
    set _print(file) $_fileselect(file)
    set _print(pattern) $_fileselect(pattern)
    set _print(showdot) $_fileselect(showdot)
    set .printFile $_fileselect(selection)
}

proc .printCanvas win {
    global .printFile .printDest .printCommand .printOrient .printColor
    if {${.printDest} == 0} {
	eval exec ${.printCommand} << \
	  \[$win postscript -rotate ${.printOrient} -colormode ${.printColor}\]
    } else {
	if {![.fileWritable "${.printFile}"]} {return}
	$win postscript -rotate ${.printOrient} -colormode ${.printColor} \
	    -file ${.printFile}
    }
    destroy .print
}

set .printDest 0
set .printCommand "lpr"
set .printOrient 0
set .printColor color
proc .printWin win {
    set w .print
    toplevel $w
    wm transient $w [winfo toplevel [winfo parent $w]]
    wm title $w "Print"
    wm resizable $w 0 0

    global .printDest .borderColor
    frame $w.topo -relief ridge -bd 4 -bg ${.borderColor}
    frame $w.top
    frame $w.top.1
    label $w.top.1.l -text "Print To: " -width 14 -anchor e
    radiobutton $w.top.1.p -text "Printer" -variable .printDest -value 0 \
	-width 8 -anchor w \
	-command "$w.top.2.e config -state normal -fg black; \
          $w.top.3.e config -state disabled -fg gray70; \
          $w.top.3.b config -state disabled; focus $w.top.2.e"
    radiobutton $w.top.1.f -text "File" -variable .printDest -value 1 \
	-command "$w.top.2.e config -state disabled -fg gray70; \
          $w.top.3.e config -state normal -fg black; \
          $w.top.3.b config -state normal; focus $w.top.3.e"
    pack $w.top.1.l $w.top.1.p $w.top.1.f -side left

    frame $w.top.2
    label $w.top.2.l -text "Print Command: " -width 14 -anchor e
    entry $w.top.2.e -textvar .printCommand -width 40
    pack $w.top.2.l $w.top.2.e -side left -fill x -expand 1

    frame $w.top.3
    label $w.top.3.l -text "File Name: " -width 14 -anchor e
    entry $w.top.3.e -textvar .printFile -width 28
    button $w.top.3.b -text "Browse..." -command \
	".printBrowse; $w.top.3.e xview end"
    pack $w.top.3.l $w.top.3.e $w.top.3.b -side left -fill x

    pack $w.top.1 $w.top.2 $w.top.3 -anchor w -pady 0 -padx 2
    pack $w.top -in $w.topo

    frame $w.mido -relief ridge -bd 4 -bg ${.borderColor}
    frame $w.mid 
    frame $w.mid.1
    label $w.mid.1.l -text "Orientation: " -width 14 -anchor e
    radiobutton $w.mid.1.p -text "Portrait" -variable .printOrient -value 0 \
	-width 8 -anchor w
    radiobutton $w.mid.1.r -text "Landscape" -variable .printOrient -value 1
    pack $w.mid.1.l $w.mid.1.p $w.mid.1.r -side left

    frame $w.mid.2
    label $w.mid.2.l -text "Color Mode: " -width 14 -anchor e
    radiobutton $w.mid.2.c -text "Color" -variable .printColor -value color \
	-width 8 -anchor w
    radiobutton $w.mid.2.g -text "Grayscale" -variable .printColor -value gray
    pack $w.mid.2.l $w.mid.2.c $w.mid.2.g -side left
    
    pack $w.mid.1 $w.mid.2 -anchor w -pady 0 -padx 2
    pack $w.mid -in $w.mido -fill x

    frame $w.buttonso -relief ridge -bd 4 -bg ${.borderColor}
    frame $w.buttons
    button $w.buttons.print -text "  Print  " \
	-command ".printCanvas $win" -default active
    bind $w <Return> ".invokeButton $w.buttons.print"
    button $w.buttons.cancel -text Cancel -command "destroy $w"
    pack $w.buttons.print $w.buttons.cancel -side left -expand 1 -fill x \
	-pady 0 
    pack $w.buttons -in $w.buttonso -fill x -padx 0 -pady 0

    pack $w.topo $w.mido $w.buttonso -fill x -expand 1

    global .printDest
    $w.top.3.e xview end
    if {${.printDest} == 1} "$w.top.1.f invoke" \
	else "$w.top.1.p invoke"
    focus $w.top
    grab set $w
    .placeDialog $w
}

proc .openOutputFile {} {
    global _fileselect _record 
    
    .fileselect .fsel $_record(path) $_record(file) $_record(pattern) \
	"Output Record File" $_record(showdot) 0 1 1
    if {$_fileselect(button) != 0} return
    set _record(path) $_fileselect(path)
    set _record(file) $_fileselect(file)
    set _record(pattern) $_fileselect(pattern)
    set _record(showdot) $_fileselect(showdot)
    set file $_fileselect(selection)
    
    if {([string index $file 0] != "|") && ![.fileWritable $file]} {
	.openOutputFile
	return
    }
    
    set type [tk_dialog .dia "Record Format" \
		  "In what format should the outputs be written?" \
		  questhead 0 "Text" "Binary" "Cancel"]

    if {$type == 2} return
    if {$type == 1} {
	openNetOutputFile $file -binary
    } else {
	openNetOutputFile $file 
    }
}
