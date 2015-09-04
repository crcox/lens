###############################################################################

catch {destroy .menubar .top .infoPanel .displayPanel .fileCommandPanel \
	   .algorithmPanel .algorithmParamPanel .trainingParamPanel \
	   .trainingControlPanel .exitButtonPanel .buttons .bottom}

# wm geometry . +20+105
wm title . "Lens"
wm resizable . 0 0
wm sizefrom . program
wm positionfrom . program
#wm geometry . 0x0
wm protocol . WM_DELETE_WINDOW {.queryExit}

frame .menubar -relief raised -bd 2
set f .menubar
menubutton $f.panels -text Lens -menu $f.panels.menu
set m $f.panels.menu
menu $m -tearoff 0
$m add cascade -label Panels: -menu $m.panels
menu $m.panels -title Panels
$m.panels add checkbutton -label "Network Info" -variable \
    _showPanel(info)            -command ".togglePanel info"
$m.panels add checkbutton -label "Displays"          -variable \
    _showPanel(display)         -command ".togglePanel display"
$m.panels add checkbutton -label "File Commands"     -variable \
    _showPanel(fileCommand)     -command ".togglePanel fileCommand"
$m.panels add checkbutton -label "Algorithms"        -variable \
    _showPanel(algorithm)       -command ".togglePanel algorithm"
$m.panels add checkbutton -label "Algorithm Parameters"  -variable \
    _showPanel(algorithmParam)  -command ".togglePanel algorithmParam"
$m.panels add checkbutton -label "Training Parameters"   -variable \
    _showPanel(trainingParam)   -command ".togglePanel trainingParam"
$m.panels add checkbutton -label "Training Controls" -variable \
    _showPanel(trainingControl) -command ".togglePanel trainingControl"
$m.panels add checkbutton -label "Task Controls" -variable \
    _showPanel(exitButton) -command ".togglePanel exitButton"
$m add command -label "Refresh" -command .refreshMainWindow
$m add separator
$m add command -label "View Console" -command viewConsole
$m add separator
$m add command -label Close -command "wm withdraw .; set .GUI 0"
$m add command -label Exit -command .queryExit

menubutton $f.help -text "Help" -menu $f.help.menu
menu $f.help.menu -tearoff 0
set m $f.help.menu
$m add command -label "Manual..." -command manual
$m add command -label "About..." -command .showAbout

pack $f.panels -side left -padx 4
pack $f.help -side right -padx 4
pack $f -fill x

#frame .top -height 0
#pack .top -pady 0 -ipady 0

###############################################################################
frame .infoPanel -relief ridge -bd 4 -bg ${.borderColor}
set f .infoPanel
frame $f.f

frame $f.l
menubutton $f.l.network -text Network -menu $f.l.network.menu
# menu $f.l.network.menu
entry $f.l.entry -relief groove -bd 2 -textvar .name -justify center -width 17
bind $f.l.entry <Return> "setObject name \${.name}; wm title . \${.name}"
bind $f.l.entry <KeyRelease> ".checkValue $f.l.entry name"
pack $f.l.network $f.l.entry -fill x

frame $f.m
menubutton $f.m.trainingSet -text "Training Set" -menu $f.m.trainingSet.menu
# menu $f.m.trainingSet.menu
entry $f.m.entry -relief groove -bd 2 -textvar .trainingSet.name \
    -justify center -width 17
bind $f.m.entry <Return> "setObject trainingSet.name \${.trainingSet.name}"
bind $f.m.entry <KeyRelease> ".checkValue $f.m.entry trainingSet.name"
pack $f.m.trainingSet $f.m.entry -fill x

frame $f.r
menubutton $f.r.testingSet -text "Testing Set" -menu $f.r.testingSet.menu
# menu $f.r.testingSet.menu
entry $f.r.entry -relief groove -bd 2 -textvar .testingSet.name \
    -justify center -width 17
bind $f.r.entry <Return> "setObject testingSet.name \${.testingSet.name}"
bind $f.r.entry <KeyRelease> ".checkValue $f.r.entry testingSet.name"
pack $f.r.testingSet $f.r.entry -fill x -expand 1

pack $f.f -fill x
pack $f.r $f.m $f.l -in $f.f -side right -expand 1 -padx 0 -pady 1
if {$_showPanel(info)} {pack $f -expand 1 -fill x}

###############################################################################

frame .displayPanel -relief ridge -bd 4 -bg ${.borderColor}
set f .displayPanel

frame $f.l
button $f.l.unit -width 24 -text "Unit Viewer" -command viewUnits
button $f.l.grph -width 24 -text "New Graph" -command .graphNewObj
pack $f.l.unit $f.l.grph -fill x

frame $f.r
button $f.r.link -width 24 -text "Link Viewer" -command viewLinks
button $f.r.objs -width 24 -text "New Object Viewer" \
    -command {viewObject ""}
pack $f.r.link $f.r.objs -fill x

pack $f.l $f.r -side left -fill x -expand 1
if {$_showPanel(display)} {pack $f -fill x -expand 1}

###############################################################################

frame .fileCommandPanel -relief ridge -bd 4 -bg ${.borderColor}
set f .fileCommandPanel

frame $f.l
button $f.l.rscr -width 24 -text "Run Script" -command .runScript
button $f.l.swgt -width 24 -text "Save Weights" -command .saveWeights
pack $f.l.rscr $f.l.swgt -fill x

frame $f.r
button $f.r.lexa -width 24 -text "Load Examples" -command .loadExamples
button $f.r.lwgt -width 24 -text "Load Weights" -command .loadWeights
pack $f.r.lexa $f.r.lwgt -fill x

pack $f.l $f.r -side left -fill x -expand 1
if {$_showPanel(fileCommand)} {pack $f -expand 1 -fill x}

###############################################################################

frame .algorithmPanel -relief ridge -bd 4 -bg ${.borderColor}
set f .algorithmPanel
frame $f.l
frame $f.r

for {set i 0} {$i < ${.numAlgorithms}} {incr i} {
    radiobutton $f.$_algShortName($i) -text $_algLongName($i) \
	-variable .algorithm -value $_algCode($i) -anchor w -width 17
    if {$i % 2} {
	pack $f.$_algShortName($i) -in $f.r
    } else {
	pack $f.$_algShortName($i) -in $f.l
    }
}

pack $f.l $f.r -side left -expand 1 -fill both -anchor n
if {$_showPanel(algorithm)} {pack $f -expand 1 -fill x}

###############################################################################

frame .algorithmParamPanel -relief ridge -bd 4 -bg ${.borderColor}
set f .algorithmParamPanel
frame $f.f
frame $f.l
frame $f.r

for {set i 0} {$i < ${.numAlgorithmParams}} {incr i} {
    set shortName [set _Algorithm.parShortName($i)]
    set longName  [set _Algorithm.parLongName($i)]
    frame $f.${shortName}_f
    label $f.${shortName}_l -text ${longName}: -width 15 -anchor w
    entry $f.${shortName}_e -relief groove -bd 2 -width 10 -textvar .$shortName
    bind $f.${shortName}_e <Key-Return> ".set$shortName"
    bind $f.${shortName}_e <KeyRelease> \
	".checkValue $f.${shortName}_e $shortName"
    pack $f.${shortName}_e $f.${shortName}_l -in $f.${shortName}_f -side right
    if {$i % 2} {
	pack $f.${shortName}_f -in $f.r -fill x
    } else {
	pack $f.${shortName}_f -in $f.l -fill x
    }
}

pack $f.l $f.r -in $f.f -side left -expand 1 -fill x -pady 1 -padx 2 -anchor n
pack $f.f -fill x
if {$_showPanel(algorithmParam)} {pack $f -expand 1 -fill x}

###############################################################################

frame .trainingParamPanel -relief ridge -bd 4 -bg ${.borderColor}
set f .trainingParamPanel
frame $f.f
frame $f.l
frame $f.r

for {set i 0} {$i < ${.numTrainingParams}} {incr i} {
    set shortName [set _Training.parShortName($i)]
    set longName  [set _Training.parLongName($i)]
    frame $f.${shortName}_f
    label $f.${shortName}_l -text $longName -width 15 -anchor w
    entry $f.${shortName}_e -relief groove -bd 2 -width 10 -textvar .$shortName
    bind $f.${shortName}_e <Key-Return> ".set$shortName"
    bind $f.${shortName}_e <KeyRelease> \
	".checkValue $f.${shortName}_e $shortName"
    pack $f.${shortName}_e $f.${shortName}_l -in $f.${shortName}_f -side right
    if {$i % 2} {
	pack $f.${shortName}_f -in $f.r -fill x
    } else {
	pack $f.${shortName}_f -in $f.l -fill x
    }
}

pack $f.l $f.r -in $f.f -side left -expand 1 -fill x -pady 1 -padx 2 -anchor n
pack $f.f -fill x
if {$_showPanel(trainingParam)} {pack $f -expand 1 -fill x}

###############################################################################

frame .trainingControlPanel -relief ridge -bd 4 -bg ${.borderColor}
set f .trainingControlPanel

frame $f.l
button $f.l.rstn -width 24 -text "Reset Network" -command resetNet
button $f.l.test -width 24 -text "Test Network" -command "puts {}; test"
pack $f.l.rstn $f.l.test -fill x

frame $f.r
button $f.r.rste -width 24 -text "Reset Training Set" \
    -command "resetExampleSet \"\${.trainingSet.name}\""
button $f.r.outp -width 24 -text "Record Outputs" -command .openOutputFile
pack $f.r.rste $f.r.outp -fill x

button $f.train -text "Train Network" \
    -command {puts {}; catch train msg; puts $msg} -bd 3
pack $f.train -side bottom -fill x
pack $f.l $f.r -side left -fill x -expand 1
if {$_showPanel(trainingControl)} {pack $f -fill x -expand 1}

###############################################################################

frame .exitButtonPanel -relief ridge -bd 4 -bg ${.borderColor}
entry .exitButtonPanel.taskd -relief groove -bd 2 -width 6 \
    -textvariable .taskDepth -justify center -state disabled
button .exitButtonPanel.but -width 24 -text "Exit" -command .queryExit \
    -cursor pirate
button .exitButtonPanel.wait -width 4 -text "Wait" -command wait \
    -state disabled
pack .exitButtonPanel.wait -side left 
pack .exitButtonPanel.but -side left -fill x -expand 1
pack .exitButtonPanel.taskd -side left -fill y
if {$_showPanel(exitButton)} {pack .exitButtonPanel -fill x}

###############################################################################

set .panelOrder {.exitButtonPanel .trainingControlPanel .trainingParamPanel \
		     .algorithmParamPanel .algorithmPanel .fileCommandPanel \
		     .displayPanel .infoPanel .menubar}

proc .togglePanel panel {
    global _showPanel .panelOrder
    wm resizable . 0 1
    wm positionfrom . user
    
    if {$_showPanel($panel)} {
	set newOrder [pack slaves .]
	set oldOrder [lrange ${.panelOrder} \
			  [lsearch ${.panelOrder} .${panel}Panel] end]
	foreach p $oldOrder {
	    if {[lsearch $newOrder $p] != -1} {
		pack .${panel}Panel -after $p -expand 1 -fill x
		return
	    }
	}
    } else {pack forget .${panel}Panel}
    update idletasks
    wm resizable . 0 0
    wm withdraw .
    wm deiconify .
}

###############################################################################

proc .updateNetworkMenu {} {
    set netList [useNet]
    set f .infoPanel.l.network
    catch {destroy $f.menu}
    menu $f.menu -tearoff 0
    foreach n $netList {
	$f.menu add command -label $n -command "useNet \"$n\""
    }
}

proc .updateExampleSetMenus {} {
    global .name
    set setList [useTrainingSet]
    set r .infoPanel.m.trainingSet
    set e .infoPanel.r.testingSet
    catch {destroy $r.menu $e.menu}
    menu $r.menu -tearoff 0
    menu $e.menu -tearoff 0
    foreach n $setList {
	$r.menu add command -label $n -command "useTrainingSet \"$n\""
	$e.menu add command -label $n -command "useTestingSet \"$n\""
    }
    $r.menu add command -label "<none>" -command "useTrainingSet {}"
    $e.menu add command -label "<none>" -command "useTestingSet {}"
}

###############################################################################

proc .checkValue {win var} {
    global .$var .entryNormalColor .entryPendingColor .name
    set val [set .$var]
    if {[catch {getObj $var} obj]} {
	$win configure -bg ${.entryNormalColor}
	return
    }
    if {$val == $obj} {
	$win configure -bg ${.entryNormalColor}
    } else {
	$win configure -bg ${.entryPendingColor}
    }
}

proc .checkValues {} {
    global .numAlgorithmParams .numTrainingParams \
	_Algorithm.parShortName _Training.parShortName
    .checkValue .infoPanel.l.entry name
    .checkValue .infoPanel.m.entry trainingSet.name
    .checkValue .infoPanel.r.entry testingSet.name
    for {set i 0} {$i < ${.numAlgorithmParams}} {incr i} {
	set shortName [set _Algorithm.parShortName($i)]
	.checkValue .algorithmParamPanel.${shortName}_e $shortName
    }
    for {set i 0} {$i < ${.numTrainingParams}} {incr i} {
	set shortName [set _Training.parShortName($i)]
	.checkValue .trainingParamPanel.${shortName}_e $shortName
    }
}

proc .refreshMainWindow {} {
    global .name
    .configureDisplay
    .updateNetworkMenu
    .updateExampleSetMenus
    if {${.name} != {}} {.getParameters} else {.clearParameters}
}

###############################################################################

proc .startTask label {
    if {$label != {}} {
	.exitButtonPanel.but configure -state normal \
	    -text "Stop $label" -command .signalHalt
	.exitButtonPanel.wait configure -state normal
    } else {
	.exitButtonPanel.but configure -state normal \
	    -text "Exit" -command .queryExit
	.exitButtonPanel.wait configure -state disabled
    }
}

proc .disableStopButton {} {
    .exitButtonPanel.but configure -state disabled -text ""
}

###############################################################################

# This creates scrollbar binding for wheel mice:
proc .wheelScroll {w args} {
  foreach s $args {
    bind $s <Button-4> "$w yview scroll -1 unit"
    bind $s <Button-5> "$w yview scroll 1 unit"
  }
}

###############################################################################

proc .invokeButton button {
    $button configure -state active -relief sunken
    update idletasks
    after 100
    $button invoke
}

proc .placeDialog w {
    wm withdraw $w
    update idletasks
    set width  [winfo reqwidth $w]
    set height [winfo reqheight $w]
#    puts "$width $height"
#    set x [expr [winfo screenwidth $w]/2 - [winfo reqwidth $w]/2 \
#            - [winfo vrootx [winfo parent $w]]]
#   set y [expr [winfo screenheight $w]/2 - [winfo reqheight $w]/2 \
#            - [winfo vrooty [winfo parent $w]]]
    set x [expr [winfo pointerx $w] - $width/2]
    if {($x + $width) > [winfo screenwidth $w]} {
	set x [expr [winfo screenwidth $w] - $width]
    }
    if {$x < 0} {set x 0}

    set y [expr [winfo pointery $w] - $height + 20]
    if {($y + $height) > [winfo screenheight $w]} {
	set y [expr [winfo screenheight $w] - $height]
    }
    if {$y < 0} {set y 0}

    wm geom $w +$x+$y
    wm deiconify $w
}

proc tk_dialog {w title text bitmap default args} {
    global tkPriv tcl_platform

    # 1. Create the top-level window and divide it into top
    # and bottom parts.

    catch {destroy $w}
    toplevel $w -class Dialog
    wm title $w $title
    wm iconname $w Dialog
    wm protocol $w WM_DELETE_WINDOW { }

    # The following command means that the dialog won't be posted if
    # [winfo parent $w] is iconified, but it's really needed;  otherwise
    # the dialog can become obscured by other windows in the application,
    # even though its grab keeps the rest of the application from being used.

    wm transient $w [winfo toplevel [winfo parent $w]]
    if {$tcl_platform(platform) == "macintosh"} {
        unsupported1 style $w dBoxProc
    }

    global .borderColor
    frame $w.boto -relief ridge -bd 4 -bg ${.borderColor}
    frame $w.topo -relief ridge -bd 4 -bg ${.borderColor}
    frame $w.bot
    frame $w.top
    pack $w.bot -in $w.boto -expand 1 -fill both
    pack $w.boto -side bottom -fill both
    pack $w.top -in $w.topo -expand 1 -fill both
    pack $w.topo -side top -fill both -expand 1

    # 2. Fill the top part with bitmap and message (use the option
    # database for -wraplength so that it can be overridden by
    # the caller).

    option add *Dialog.msg.wrapLength 3i widgetDefault
    label $w.msg -justify left -text $text -font .LargeFont
    pack $w.msg -in $w.top -side right -expand 1 -fill both -padx 3m -pady 3m
    if {$bitmap != ""} {
        if {($tcl_platform(platform) == "macintosh") && ($bitmap == "error")} {
            set bitmap "stop"
        }
        label $w.bitmap -bitmap $bitmap
        pack $w.bitmap -in $w.top -side left -padx 3m -pady 3m
    }

    # 3. Create a row of buttons at the bottom of the dialog.

    set i 0
    foreach but $args {
        button $w.button$i -text $but -command "set tkPriv(button) $i" \
	    -highlightthickness 0
        if {$i == $default} {
            $w.button$i configure -default active
        } else {
            $w.button$i configure -default normal
        }
        grid $w.button$i -in $w.bot -column $i -row 0 -sticky ew -padx 10
        grid columnconfigure $w.bot $i
        # We boost the size of some Mac buttons for l&f
        if {$tcl_platform(platform) == "macintosh"} {
            set tmp [string tolower $but]
            if {($tmp == "ok") || ($tmp == "cancel")} {
                grid columnconfigure $w.bot $i -minsize [expr 59 + 20]
            }
        }
        incr i
    }

    # 4. Create a binding for <Return> on the dialog if there is a
    # default button.

    if {$default >= 0} {
        bind $w <Return> "
            $w.button$default configure -state active -relief sunken
            update idletasks
            after 100
            set tkPriv(button) $default
        "
    }

    # 5. Create a <Destroy> binding for the window that sets the
    # button variable to -1;  this is needed in case something happens
    # that destroys the window, such as its parent window being destroyed.

    bind $w <Destroy> {set tkPriv(button) -1}

    # 6. Center the window under the cursor.

    .placeDialog $w

    # 7. Set a grab and claim the focus too.

    set oldFocus [focus]
    set oldGrab [grab current $w]
    if {$oldGrab != ""} {
        set grabStatus [grab status $oldGrab]
    }
    grab $w
    if {$default >= 0} {
        focus $w.button$default
    } else {
        focus $w
    }

    # 8. Wait for the user to respond, then restore the focus and
    # return the index of the selected button.  Restore the focus
    # before deleting the window, since otherwise the window manager
    # may take the focus away so we can't redirect it.  Finally,
    # restore any grab that was in effect.

    tkwait variable tkPriv(button)
    catch {focus $oldFocus}
    catch {
        # It's possible that the window has already been destroyed,
        # hence this "catch".  Delete the Destroy handler so that
        # tkPriv(button) doesn't get reset by it.

        bind $w <Destroy> {}
        destroy $w
    }
    if {$oldGrab != ""} {
        if {$grabStatus == "global"} {
            grab -global $oldGrab
        } else {
            grab $oldGrab
        }
    }
    return $tkPriv(button)
}

###############################################################################

set .GUI 1
.refreshMainWindow
wm deiconify .
return
