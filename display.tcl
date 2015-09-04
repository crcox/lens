set _valueLabel(0) "Outputs and Targets"
set _valueLabel(1) "Outputs"
set _valueLabel(2) "Targets"
set _valueLabel(3) "Inputs"
set _valueLabel(4) "External Inputs"
set _valueLabel(5) "Output Derivatives"
set _valueLabel(6) "Input Derivatives"
set _valueLabel(7) "Gains"
set _valueLabel(8) "Link Weights"
set _valueLabel(9) "Link Derivs"
set _valueLabel(10) "Link Deltas"

set _unitSetLabel(1) "Training Set"
set _unitSetLabel(2) "Testing Set"

set _updateLabel(0)  "Never"
set _updateLabel(1)  "Ticks"
set _updateLabel(2)  "Events"
set _updateLabel(4)  "Examples"
set _updateLabel(8)  "Weight Updates"
set _updateLabel(16) "Progress Reports"
set _updateLabel(32) "Training and Testing"
set _updateLabel(64) "User Signals"

set _paletteLabel(0) "Blue-Black-Red"
set _paletteLabel(1) "Blue-Red-Yellow"
set _paletteLabel(2) "Black-Gray-White"
set _paletteLabel(3) "Hinton Diagram"


# This is my modified version of the Listbox bindings
foreach i [bind Listbox] {
    bind MyListbox $i [bind Listbox $i]
}
bind MyListbox <Down> {if {[%W curselection] >= [%W size] - 1} \
    {event generate %W <Control-Home>} else {tkListboxUpDown %W 1}}
bind MyListbox <space> {if {[%W curselection] >= [%W size] - 1} \
    {event generate %W <Control-Home>} else {tkListboxUpDown %W 1}}
bind MyListbox <Up> {if {[%W curselection] <= 0} \
    {event generate %W <Control-End>} else {tkListboxUpDown %W -1}}
bind MyListbox <Next> {+%W selection clear 0 end; %W selection set active; \
    event generate %W <<ListboxSelect>>}
bind MyListbox <Prior> {+%W selection clear 0 end; %W selection set active; \
    event generate %W <<ListboxSelect>>}
bind MyListbox <Control-Next> {event generate %W <Control-End>}
bind MyListbox <Control-Prior> {event generate %W <Control-Home>}
bind MyListbox <Return> {event generate %W <<ListboxSelect>>}

proc .viewUnits {} {
  global .unitUp _valueLabel _unitSetLabel .borderColor .unitUpdates \
      .unitCellSize .unitCellSpacing _updateLabel .unitDisplaySet \
      .unitDisplayValue .unitLogTemp _paletteLabel .unitPalette \
      .unitExampleProc
  
  if {${.unitUp}} {
    .chooseUnitSet
    drawUnits
    wm iconify .unit
    wm deiconify .unit
    return
  }
  
  set w .unit
  catch {destroy $w}
  toplevel $w
  wm title $w "Unit Viewer"
  wm protocol $w WM_DELETE_WINDOW "set .unitUp 0; destroy $w"
  wm minsize $w 520 120
  
  frame $w.menubar -relief raised -bd 2
  menubutton $w.menubar.win -text Viewer -menu $w.menubar.win.menu
  menu $w.menubar.win.menu -tearoff 0
  $w.menubar.win.menu add cascade -label "Update After:" \
      -menu $w.menubar.win.menu.updates
  $w.menubar.win.menu add cascade -label "Cell Size:" \
      -menu $w.menubar.win.menu.size
  $w.menubar.win.menu add cascade -label "Cell Spacing:" \
      -menu $w.menubar.win.menu.spacing
  $w.menubar.win.menu add separator
  $w.menubar.win.menu add command -label Update -command .updateUnitDisplay
  $w.menubar.win.menu add command -label Refresh \
      -command ".chooseUnitSet; drawUnits"
  $w.menubar.win.menu add command -label "Print..." \
      -command ".printWin $w.c.c"
  $w.menubar.win.menu add separator
  $w.menubar.win.menu add command -label Close \
      -command "set .unitUp 0; destroy $w"
  
  menu $w.menubar.win.menu.updates -title Updates -tearoff 0
  foreach i {4 8 16 32 0} {
    $w.menubar.win.menu.updates add radiobutton -label $_updateLabel($i) \
	-var .unitUpdates -val $i
  }
  
  menu $w.menubar.win.menu.size -title "Cell Size" -tearoff 0
  foreach i {5 6 7 8 9 10 11 12 14 16 18 20 24 28 32 40} {
    $w.menubar.win.menu.size add radiobutton -label $i -var .unitCellSize \
	-value $i -command viewUnits
  }
  menu $w.menubar.win.menu.spacing -title "Cell Spacing" -tearoff 0
  foreach i {0 1 2 3 4 6 8 10} {
    $w.menubar.win.menu.spacing add radiobutton -label $i \
	-value $i -var .unitCellSpacing -command viewUnits
  }
  
  menubutton $w.menubar.set -text "Example Set" -menu $w.menubar.set.menu
  menu $w.menubar.set.menu -tearoff 0
  foreach i {1 2} {
    $w.menubar.set.menu add radiobutton -label $_unitSetLabel($i) \
	-command ".setUnitSet $i" -var .unitDisplaySet -val $i
  }
  
  global .trainingSet.name .testingSet.name
  if {${.trainingSet.name} == ""} {
    $w.menubar.set.menu entryconfigure 0 -state disabled
  }
  if {${.testingSet.name} == ""} {
    $w.menubar.set.menu entryconfigure 1 -state disabled
  }
  if {${.trainingSet.name} == "" && ${.testingSet.name} == ""} {
    $w.menubar.set configure -state disabled
  }
  
  menubutton $w.menubar.proc -text "Procedure" -menu $w.menubar.proc.menu
  menu $w.menubar.proc.menu -tearoff 0
  $w.menubar.proc.menu add radiobutton -label "Test" \
      -command ".setUnitExampleProc 0" -var .unitExampleProc -val 0
  $w.menubar.proc.menu add radiobutton -label "Train" \
      -command ".setUnitExampleProc 1" -var .unitExampleProc -val 1
  
  menubutton $w.menubar.val -text Value -menu $w.menubar.val.menu
  menu $w.menubar.val.menu -tearoff 0
  repeat i {0 7} {
    $w.menubar.val.menu add radiobutton -label $_valueLabel($i) \
	-command ".setUnitValue $i" -var .unitDisplayValue -val $i
  }
  $w.menubar.val.menu add separator
  repeat i {8 10} {
    $w.menubar.val.menu add radiobutton -label $_valueLabel($i) \
	-command ".setUnitValue $i" -var .unitDisplayValue -val $i
  }
  
  menubutton $w.menubar.pal -text Palette -menu $w.menubar.pal.menu
  menu $w.menubar.pal.menu -tearoff 0
  foreach i {0 1 2 3} {
    $w.menubar.pal.menu add radiobutton -label $_paletteLabel($i) \
	-command ".setPalette 0 $i" -var .unitPalette -val $i
  }
  
  global .name
  if {${.name} == ""} {
    $w.menubar.win.menu entryconfigure 1 -state disabled
    $w.menubar.win.menu entryconfigure 2 -state disabled
    $w.menubar.val configure -state disabled
  }
  
  pack $w.menubar.win $w.menubar.set $w.menubar.proc $w.menubar.val \
      $w.menubar.pal -side left -padx 4
  pack $w.menubar -side top -fill x
  
  
  frame $w.tout -relief ridge -bd 4 -bg ${.borderColor}
  frame $w.t
  
  proc .setEntryColor {w value} {
    global .entryNormalColor .entryPendingColor
    if {$value == [$w get]} {
      $w configure -bg ${.entryNormalColor}
    } else {
      $w configure -bg ${.entryPendingColor}
    }
  }
  
  frame $w.t.l
  frame $w.t.f1
  label $w.t.eventl -text "Event"
  frame $w.t.f1b
  entry $w.t.event -justify center -width 3 \
      -relief groove -bd 2 -state disabled
  bind $w.t.event <Return> ".viewChangeEvent \[$w.t.event get\]"
  bind $w.t.event <KeyRelease> \
      ".setEntryColor $w.t.event \${.viewEvent}"
  label $w.t.eventofl -text "/"
  entry $w.t.eventof -justify center -width 3 \
      -relief groove -bd 2 -state disabled
  pack $w.t.event $w.t.eventofl $w.t.eventof -in $w.t.f1b -side left
  pack $w.t.eventl $w.t.f1b -in $w.t.f1 -side top
  
  frame $w.t.f2
  label $w.t.tickl -text "Example Time"
  frame $w.t.f2b
  entry $w.t.tick -justify center -width 5 \
      -relief groove -bd 2 -state disabled
  bind $w.t.tick <Return> ".viewChangeTime \[$w.t.tick get\]"
  bind $w.t.tick <KeyRelease> \
      ".setEntryColor $w.t.tick \${.viewTime}"
  label $w.t.tickofl -text "/"
  entry $w.t.tickof -justify center -width 5 \
      -relief groove -bd 2 -state disabled
  pack $w.t.tick $w.t.tickofl $w.t.tickof -in $w.t.f2b -side left
  pack $w.t.tickl $w.t.f2b -in $w.t.f2 -side top
  
  frame $w.t.f3
  label $w.t.etickl -text "Event Time"
  frame $w.t.f3b
  entry $w.t.etick -justify center -width 5 \
      -relief groove -bd 2 -state disabled
  bind $w.t.etick <Return> ".viewChangeEventTime \[$w.t.etick get\]"
  bind $w.t.etick <KeyRelease> \
      ".setEntryColor $w.t.etick \${.viewEventTime}"
  label $w.t.etickofl -text "/"
  entry $w.t.etickof -justify center \
      -width 5 -relief groove -bd 2 -state disabled
  pack $w.t.etick $w.t.etickofl $w.t.etickof -in $w.t.f3b -side left
  pack $w.t.etickl $w.t.f3b -in $w.t.f3 -side top
  
  pack $w.t.f1 $w.t.f2 $w.t.f3 -in $w.t.l -side left -padx 2
  
  frame $w.t.m
#    label $w.t.unitl -text "Unit:"
    entry $w.t.unit -justify center -width 18 \
	-relief groove -bd 2 -state disabled -textvar .unitUnit
#    label $w.t.valuel -text " Value:"
    entry $w.t.value -justify center -width 18 \
	-relief groove -bd 2 -state disabled -textvar .unitValue
    pack $w.t.unit $w.t.value -in $w.t.m -side top

#    pack $w.t.unitl $w.t.unit $w.t.valuel $w.t.value -in $w.t.b -side left
#    pack $w.t.t $w.t.b -in $w.t.l -side top -anchor e

    frame $w.t.r
    frame $w.t.rt
    button $w.t.left3 -image .left3 -bd 1 \
	-command ".viewChange 0" -state disabled
    button $w.t.left2 -image .left2 -bd 1 \
	-command ".viewChange 1" -state disabled
    button $w.t.left -image .left -bd 1 \
	-command ".viewChange 2" -state disabled
    button $w.t.right -image .right -bd 1 \
	-command ".viewChange 3" -state disabled
    button $w.t.right2 -image .right2 -bd 1 \
	-command ".viewChange 4" -state disabled
    button $w.t.right3 -image .right3 -bd 1 \
	-command ".viewChange 5" -state disabled
    pack $w.t.left3 $w.t.left2 $w.t.left -in $w.t.rt -side left
    pack $w.t.right3 $w.t.right2 $w.t.right -in $w.t.rt -side right

    frame $w.t.rb
    frame $w.t.sc -relief sunken -bd 2 -highlightthickness 1
    scale $w.t.scale -from 2000 -to 50 -resolution 50 -orient horizontal \
	-showvalue 0 -bd 0 -highlightthickness 0 \
	-takefocus 0 -length 90
    $w.t.scale set 0
    button $w.t.step2 -image .right2b -bd 1 \
	-command ".viewChange 6" -state disabled
    button $w.t.step3 -image .right3b -bd 1 \
	-command ".viewChange 7" -state disabled
    pack $w.t.scale -in $w.t.sc -fill x -expand 1
    pack $w.t.sc -in $w.t.rb -side left -padx 1 -anchor s
    pack $w.t.step2 $w.t.step3 -in $w.t.rb -side left
    pack $w.t.rt -in $w.t.r -side top -fill x
    pack $w.t.rb -in $w.t.r -side top -fill x

    pack $w.t.l -side left -pady 2 -anchor s -expand 1
    pack $w.t.m $w.t.r -side left -expand 1
    pack $w.t -in $w.tout -fill x
    pack $w.tout -side top -fill x 

    frame $w.l -relief ridge -bd 4 -bg ${.borderColor}
    label $w.l.label -text {}
    pack $w.l.label -side top -fill x
    listbox $w.l.list -yscrollcommand "$w.l.scrolly set" \
	-xscrollcommand "$w.l.scrollx set" -width 24 \
	-setgrid 0 -relief sunken -selectmode browse
    bindtags .unit.l.list {MyListbox .unit.l.list .unit all}
    bind $w.l.list <<ListboxSelect>> {.selectExample [%W curselection]}
    focus $w.l.list

    scrollbar $w.l.scrollx -command "$w.l.list xview" -orient horizontal \
	-takefocus 0
    scrollbar $w.l.scrolly -command "$w.l.list yview" -takefocus 0
    .wheelScroll $w.l.list $w.l.list $w.l.scrolly 
    pack $w.l.scrolly -side right -fill y
    pack $w.l.scrollx -side bottom -fill x
    pack $w.l.list -fill y -expand 1
    pack $w.l -side left -fill y

    frame $w.c -relief ridge -bd 4 -bg ${.borderColor}
    label $w.c.label -text $_valueLabel(${.unitDisplayValue})
    pack $w.c.label -side top -fill x

    frame $w.c.scfr -relief sunken -bd 2 -highlightthickness 1
    scale $w.c.scale -from 10 -to -4 -orient vertical -showvalue 0 \
	-resolution 0.5 -command ".setUnitTemp" -bd 0 -highlightthickness 0 \
	-takefocus 0 -var .unitLogTemp
    $w.c.scale set 0

    pack $w.c.scale -in $w.c.scfr -fill y -expand 1
    pack $w.c.scfr -side right -fill y

    scrollbar $w.c.scrollx -command "$w.c.c xview" -orient horizontal \
	-takefocus 0
    scrollbar $w.c.scrolly -command "$w.c.c yview" -takefocus 0
    pack $w.c.scrolly -side right -fill y
    pack $w.c.scrollx -side bottom -fill x

    frame $w.c.sun -relief sunken -bd 2 -highlightthickness 1
    frame $w.c.ver
    canvas $w.c.c -yscrollcommand "$w.c.scrolly set" \
	-xscrollcommand "$w.c.scrollx set" -highlightthickness 0
    pack $w.c.c -in $w.c.ver -side left
    pack $w.c.ver -in $w.c.sun -fill y -expand 1
    pack $w.c.sun -fill both -expand 1 -ipadx 1 -ipady 1
    pack $w.c -fill both -expand 1
    .wheelScroll $w.c.c $w.c.c $w.c.scrolly 
    foreach win "$w.c.sun $w.c.c" {
      bind $win <Button-1> {if {!${.lockedUnit}} .lockUnit; set .lockedUnit 0}
    }

    set .unitUp 1
    catch {.chooseUnitSet}
    catch {drawUnits}
    return
}
set .lockedUnit 0

proc .setViewerSize {width height} {
    set geo [split [winfo geometry .unit] "+"]
    .unit.c.c configure -width $width -height $height
    update
    set width [max [winfo reqwidth .unit] [lindex [wm minsize .unit] 0]]
    set height [max [winfo reqheight .unit] [lindex [wm minsize .unit] 1]]
    if {$geo == "1x1 0 0"} {
	wm geometry .unit ${width}x${height}
    } else {
	wm geometry .unit ${width}x${height}+[lindex $geo 1]+[lindex $geo 2]
    }
}

proc .entrySet {w value} {
    set state [$w cget -state]
    $w configure -state normal
    $w delete 0 end
    $w insert 0 $value
    $w configure -state $state
}

proc .setUnitEntries {} {
    global .viewEvent .viewEvents .viewTime .viewMaxTime .viewEventTime \
	.viewMaxEventTime
    set w .unit
    .entrySet $w.t.event   ${.viewEvent}
    .entrySet $w.t.eventof ${.viewEvents}
    .entrySet $w.t.tick    ${.viewTime}
    .entrySet $w.t.tickof  ${.viewMaxTime}
    .entrySet $w.t.etick   ${.viewEventTime}
    .entrySet $w.t.etickof ${.viewMaxEventTime}
}

proc .clearUnitInfo {} {
  .unitInfo {} {} {} 0
#    global .unitName .unitUnit .unitValue
  
#    set .unitName {}
#    set .unitUnit {}
#    set .unitValue {}
}

proc .viewLinks {} {
    global .linkUp _valueLabel .borderColor .linkUpdates .linkCellSize \
	.linkCellSpacing .linkPalette .linkLogTemp _updateLabel _paletteLabel \
	.linkDisplayValue

    if {${.linkUp}} {
	.drawLinks
	wm iconify .link
	wm deiconify .link
	return
    }
    
    set w .link
    catch {destroy $w}
    toplevel $w
    wm title $w "Link Viewer"
    wm protocol $w WM_DELETE_WINDOW "set .linkUp 0; destroy $w"
    wm minsize $w 515 145

    frame $w.menubar -relief raised -bd 2
    menubutton $w.menubar.win -text Viewer -menu $w.menubar.win.menu
    menu $w.menubar.win.menu -tearoff 0
    $w.menubar.win.menu add cascade -label "Update After:" \
	-menu $w.menubar.win.menu.updates
    $w.menubar.win.menu add cascade -label "Cell Size:" \
	-menu $w.menubar.win.menu.size
    $w.menubar.win.menu add cascade -label "Cell Spacing:" \
	-menu $w.menubar.win.menu.spacing
    $w.menubar.win.menu add separator
    $w.menubar.win.menu add command -label Update -command .updateLinkDisplay
    $w.menubar.win.menu add command -label Refresh -command ".drawLinks"
    $w.menubar.win.menu add command -label "Print..." \
	-command ".printWin $w.c.c"
    $w.menubar.win.menu add separator
    $w.menubar.win.menu add command -label Close \
	-command "set .linkUp 0; destroy $w"
    
    menu $w.menubar.win.menu.updates -title Updates
    foreach i {8 16 32 0} {
      $w.menubar.win.menu.updates add radiobutton -label $_updateLabel($i) \
	  -var .linkUpdates -val $i
    }

    menu $w.menubar.win.menu.size -title "Cell Size"
    foreach i {2 3 4 5 6 7 8 9 10 11 12 14 16 18 20 24 30} {
	$w.menubar.win.menu.size add radiobutton -label $i \
	    -command viewLinks -var .linkCellSize -val $i
    }
    menu $w.menubar.win.menu.spacing -title "Cell Spacing"
    foreach i {0 1 2 3 4 6 8 10} {
	$w.menubar.win.menu.spacing add radiobutton -label $i \
	    -command viewLinks -var .linkCellSpacing -val $i
    }

    menubutton $w.menubar.pre -text "Sending Groups" -state disabled \
	-menu $w.menubar.pre.menu
    menubutton $w.menubar.post -text "Receiving Groups" -state disabled \
	-menu $w.menubar.post.menu
    
    menubutton $w.menubar.val -text Value -menu $w.menubar.val.menu
    menu $w.menubar.val.menu -tearoff 0
    repeat i {8 10} {
	$w.menubar.val.menu add radiobutton -label $_valueLabel($i) \
	    -command ".setLinkValue $i" -var .linkDisplayValue -val $i
    }

    menubutton $w.menubar.pal -text Palette -menu $w.menubar.pal.menu
    menu $w.menubar.pal.menu -tearoff 0
    foreach i {0 1 2 3} {
      $w.menubar.pal.menu add radiobutton -label $_paletteLabel($i) \
	  -command ".setPalette 1 $i" -var .linkPalette -val $i
    }

    global .name
    if {${.name} == ""} {
	$w.menubar.win.menu entryconfigure 1 -state disabled
	$w.menubar.win.menu entryconfigure 2 -state disabled
	$w.menubar.val configure -state disabled
    }

    pack $w.menubar.win $w.menubar.pre $w.menubar.post $w.menubar.val \
	$w.menubar.pal -side left -padx 4
    pack $w.menubar -side top -fill x
    
    frame $w.t -relief ridge -bd 4 -bg ${.borderColor}
    frame $w.t.t
    frame $w.t.l
    frame $w.t.m
    label $w.t.m.l -text "Mean"
    entry $w.t.m.e -relief groove -bd 2 -width 9 -textvar .linkMean \
	-just center
    pack $w.t.m.l $w.t.m.e -side top

    frame $w.t.v
    label $w.t.v.l -text "Variance"
    entry $w.t.v.e -relief groove -bd 2 -width 9 -textvar .linkVar \
	-just center
    pack $w.t.v.l $w.t.v.e -side top

    frame $w.t.ma
    label $w.t.ma.l -text "Mean Abs."
    entry $w.t.ma.e -relief groove -bd 2 -width 9 -textvar .linkMAbs \
	-just center
    pack $w.t.ma.l $w.t.ma.e -side top

    frame $w.t.va
    label $w.t.va.l -text "Mean Dist."
    entry $w.t.va.e -relief groove -bd 2 -width 9 -textvar .linkMDist \
	-just center
    pack $w.t.va.l $w.t.va.e -side top

    frame $w.t.x
    label $w.t.x.l -text "Maximum"
    entry $w.t.x.e -relief groove -bd 2 -width 9 -textvar .linkMax \
	-just center
    pack $w.t.x.l $w.t.x.e -side top

    pack $w.t.m $w.t.v $w.t.ma $w.t.va $w.t.x -in $w.t.l -side left -padx 2

    frame $w.t.r
    frame $w.t.u
    entry $w.t.u.f -relief groove -bd 2 -width 12 -textvar .linkFromUnit \
	-just center
    label $w.t.u.l -text "->"
    entry $w.t.u.t -relief groove -bd 2 -width 12 -textvar .linkToUnit \
	-just center
    pack $w.t.u.f $w.t.u.l $w.t.u.t -in $w.t.u -side left

    entry $w.t.val -relief groove -bd 2 -width 9 -textvar .linkValue \
	-just center
    pack $w.t.u $w.t.val -in $w.t.r -side top

    pack $w.t.l -in $w.t.t -side left -expand 1 -pady 2 -anchor s
    pack $w.t.r -in $w.t.t -side left -expand 1 -pady 2
    pack $w.t.t -fill x -expand 1
    pack $w.t -side top -fill x

    frame $w.c -relief ridge -bd 4 -bg ${.borderColor}

    label $w.c.label -text $_valueLabel(${.linkDisplayValue})
    pack $w.c.label -side top -fill x

    frame $w.c.scfr -relief sunken -bd 2 -highlightthickness 1
    scale $w.c.scale -from 12 -to -2 -orient vertical -showvalue 0 \
	-resolution 0.5 -command ".setLinkTemp" -bd 0 -highlightthickness 0 \
	-takefocus 0
    $w.c.scale set 2
    pack $w.c.scale -in $w.c.scfr -fill y -expand 1
    pack $w.c.scfr -side right -fill y

    scrollbar $w.c.scrolly -command "$w.c.c yview" -takefocus 0
    scrollbar $w.c.scrollx -command "$w.c.c xview" -orient horizontal \
	-takefocus 0
    pack $w.c.scrolly -side right -fill y
    pack $w.c.scrollx -side bottom -fill x

    canvas $w.c.c -yscrollcommand "$w.c.scrolly set" \
	-xscrollcommand "$w.c.scrollx set" -relief sunken -bd 2 -height 400
    pack $w.c.c -fill both -expand 1
    pack $w.c -fill both -expand 1
    .wheelScroll $w.c.c $w.c.c $w.c.scrolly 

    foreach win "$w.c $w.c.c" {
      bind $win <Button-1> {if {!${.lockedLink}} .lockLink; set .lockedLink 0}
    }

    set .linkUp 1
    .drawLinks
}

proc .clearLinkInfo {} {
  .linkInfo {} {} {} 0
#   global .linkFromUnit .linkToUnit .linkValue
#    set .linkFromUnit {}
#    set .linkToUnit {}
#    set .linkValue {}
}

proc .buildLinkGroupMenus {} {
    set groupList [.getLinkGroups]
    catch {destroy .link.menubar.pre.menu .link.menubar.post.menu}
    .link.menubar.pre configure -state disabled
    .link.menubar.post configure -state disabled
    menu .link.menubar.pre.menu -tearoff no
    menu .link.menubar.post.menu -tearoff no
    .link.menubar.pre.menu add command -label All -command \
	{.setAllLinkGroups 0 1}
    .link.menubar.pre.menu add command -label None -command \
	{.setAllLinkGroups 0 0}
    .link.menubar.post.menu add command -label All -command \
	{.setAllLinkGroups 1 1}
    .link.menubar.post.menu add command -label None -command \
	{.setAllLinkGroups 1 0}
    foreach group $groupList {
	set name [lindex $group 0]
	set num  [lindex $group 1]
	set send [lindex $group 2]
	set receive [lindex $group 3]
	if {$send != -1} {
	    global .link.pre.$num
	    .link.menubar.pre.menu add checkbutton -label $name \
		-variable .link.pre.$num -command \
		".setLinkGroup 0 \"$name\" \${.link.pre.$num}"
	    set .link.pre.$num $send
	    .link.menubar.pre configure -state normal
	}
	if {$receive != -1} {
	    global .link.post.$num
	    .link.menubar.post.menu add checkbutton -label $name \
		-variable .link.post.$num -command \
		".setLinkGroup 1 \"$name\" \${.link.post.$num}"
	    set .link.post.$num $receive
	    .link.menubar.post configure -state normal
	}
    }
}

proc .unitMenu {g u targets x y} {
  catch {destroy .unit.m}
  menu .unit.m -tearoff 0
  .unit.m add command -label "Print this value" -command ".unitInfo $g $u $targets 1"
  .unit.m add command -label "Graph this value" -command ".graphUnitValue $g $u $targets"
  .unit.m add command -label "View this unit" -command "viewObject group($g).unit($u)"
  tk_popup .unit.m $x $y
}

proc .setUnitBindings {tag g u targets} {
  .unit.c.c bind $tag <Enter> ".unitInfo $g $u $targets 0"
  .unit.c.c bind $tag <Leave> .clearUnitInfo
  .unit.c.c bind $tag <Button-1> ".lockUnit $g $u; set .lockedUnit 1"
  .unit.c.c bind $tag <Button-2> ".unitMenu $g $u $targets %X %Y"
  .unit.c.c bind $tag <Button-3> ".setUnitUnit $g $u"
}

proc .linkMenu {g1 u1 g2 u2 lnum x y} {
  catch {destroy .link.m}
  menu .link.m -tearoff 0
  .link.m add command -label "Print this value" -command ".linkInfo $g2 $u2 $lnum 1"
  .link.m add command -label "Graph this value" -command ".graphLinkValue $g2 $u2 $lnum"
  .link.m add command -label "View this link" -command "viewObject group($g2).unit($u2).incoming($lnum)"
  tk_popup .link.m $x $y
}

proc .setLinkBindings {tag g1 u1 g2 u2 lnum} {
  .link.c.c bind $tag <Enter> ".linkInfo $g2 $u2 $lnum 0"
  .link.c.c bind $tag <Leave> .clearLinkInfo
  .link.c.c bind $tag <Button-1> ".lockLink $g2 $u2 $lnum; set .lockedLink 1"
  .link.c.c bind $tag <Button-2> ".linkMenu $g1 $u1 $g2 $u2 $lnum %X %Y"
}

proc .drawHinton {win args} {
  eval .$win.c.c create $args
}

proc .updateUnitInfo {} {
  global .unitGroupNum .unitNum .unitTargets
#  if {${.unitName} != {}} {
   .unitInfo ${.unitGroupNum} ${.unitNum} ${.unitTargets} 0
#  }
}

proc .updateLinkInfo {} {
  global .linkG2 .linkU2 .linkLNum
#  if {${.linkFromUnit} != {}} {
  .linkInfo ${.linkG2} ${.linkU2} ${.linkLNum} 0
#  }
}
