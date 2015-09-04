#set .GraphWindows {}

#proc .unusedGraphWindowIndex {} {
#    global .GraphWindows
#    for {set i 1} {[lsearch ${.GraphWindows} $i] != -1} {incr i} {}
#    lappend .GraphWindows $i
#    return $i
#}

#proc .removeGraphWindowIndex i {
#    global .GraphWindows
#    set l [lsearch ${.GraphWindows} $i]
#    set .GraphWindows "[lrange ${.GraphWindows} 0 [expr $l - 1]] \
#			   [lrange ${.GraphWindows} [expr $l + 1] end]"
#}

#proc .destroyGraphWindowIndex i {
#  .removeGraphWindowIndex $i
#  .deleteGraph $i
#  catch {destroy .grProp$i}
#  catch {destroy .graph$i}
#  return
#}

set .objName error
proc .getObjectName oldName {
  global .objName .done
  set .done -1
  set w .getobj
  
  if {$oldName != {}} {set .objName $oldName}
  toplevel $w
  wm protocol $w WM_DELETE_WINDOW {set .done 1}
  wm transient $w [winfo toplevel [winfo parent $w]]
  wm title $w "Create Graph"
  
  frame $w.top -relief raised -bd 1
  button $w.top.question -bitmap questhead -relief flat -state disabled \
      -disabledforeground black
  label $w.top.label -text "Value or command to graph:"
  entry $w.top.entry -width 30 -textvar .objName -relief groove -bd 2
  bind $w.top.entry <Return> {set .done 0}
  pack $w.top.question -side left -padx 6
  pack $w.top.label $w.top.entry -pady 4 -padx 2
  
  frame $w.buttons -relief raised -bd 1
  button $w.buttons.ok -text "   Ok   " -default active \
      -command {set .done 0}
  button $w.buttons.cancel -text Cancel -command {set .done 1}
  pack $w.buttons.ok $w.buttons.cancel -side left -expand 1
  pack $w.top $w.buttons -fill x -expand 1
  focus $w.top.entry
  grab set $w
  .displayPanel.l.grph configure -state normal
  $w.top.entry selection range 0 end
  
  .placeDialog $w
  
  tkwait variable .done
  destroy $w
  
  if ${.done} {
    return {}
  } else {
    return ${.objName}
  }
}

proc .graphNewObj {} {
  set obj [.getObjectName {}]
  if {$obj != {}} {graphObject $obj}
}

#proc .graphSetUpdate {i u} {
#  global _updateLabel
#  set w .grProp$i
#  $w.t.m1 config -text $_updateLabel($u)
#  .configGraph $i -updateOn u
#}

proc .configEntry {e k} {
  global .entryNormalColor .entryPendingColor  
  if {$k == "Return"} {
    $e config -bg ${.entryNormalColor}
  } else {
    $e config -bg ${.entryPendingColor}
  }
}

proc .setGraphParam {g variable value} {
  setObject graph($g).$variable $value
  graph refresh $g
}

proc .setTraceParam {g t variable value} {
  setObject graph($g).trace($t).$variable $value
  graph refresh $g
}

proc .entryBind {e command} {
  bind $e <Return> "eval $command \{\[$e get\]\}"
  bind $e <KeyRelease> "+.configEntry $e %K"
}

proc .setColor junk {
  global .hue .sat .bri .cname
  set .cname [.colorName ${.hue} ${.sat} ${.bri}]
  .color.c config -bg ${.cname}
}

proc .getColor {g t color} {
  global .hue .sat .bri .cname .borderColor
  catch {destroy .color}
  set w .color
  toplevel $w
  wm title $w Color
  set .hue 0.0
  set .sat 1.0
  set .bri 0.8
  frame $w.f1 -relief ridge -bd 4 -bg ${.borderColor}
  canvas $w.c -width 150 -height 150 -bg $color
  frame $w.f
  scale $w.h -length 100 -orient horizontal -from 0 -to 1 -resolution 0.001 \
      -label Hue -showvalue 1 -variable .hue -command ".setColor"
  scale $w.s -length 100 -orient horizontal -from 0 -to 1 -resolution 0.001 \
      -label Saturation -showvalue 1 -variable .sat -command ".setColor"
  scale $w.b -length 100 -orient horizontal -from 0 -to 1 -resolution 0.001 \
      -label Brightness -showvalue 1 -variable .bri -command ".setColor"
  pack $w.h $w.s $w.b -in $w.f -side top 
  pack $w.f -in $w.f1 -side right -fill y
  pack $w.c -in $w.f1 -fill both -expand 1

  frame $w.f2 -relief ridge -bd 4 -bg ${.borderColor}
  button $w.b1 -text Cancel -command "destroy .color"
  button $w.b2 -text Ok -command "destroy .color; \
      setObject graph($g).trace($t).color \${.cname}; \
      graph refresh $g"
  pack $w.b1 $w.b2 -in $w.f2 -side left -fill x -expand 1
  pack $w.f1 -side top -fill both -expand 1
  pack $w.f2 -side top -fill x
}

proc .drawGraphProps g {
  global .graphPropUp_$g .borderColor _updateLabel .traceProps
  set w .grProp$g
  if [winfo exists $w] {
    set geo +[winfo x $w]+[winfo y $w]
    destroy $w
  } else {set geo +200+200}
  set .graphPropUp_$g 1
  toplevel $w
  wm title $w "Graph $g Properties"
  wm protocol $w WM_DELETE_WINDOW "set .graphPropUp_$g 0; destroy $w"
  wm geometry $w $geo
  wm resizable $w 1 0

  frame $w.t -relief ridge -bd 4 -bg ${.borderColor}

  frame $w.t.f1
  frame $w.t.f6
  label $w.t.l1 -text "Update Every:"
  entry $w.t.e1 -width 3 -relief groove -bd 2
  .entryBind $w.t.e1 ".setGraphParam $g updateEvery"
  menubutton $w.t.m1 -text "" -menu $w.t.m1.menu
  menu $w.t.m1.menu -tearoff 0
  foreach u {1 2 4 8 16 64 0} {
    $w.t.m1.menu add command -label $_updateLabel($u) \
	-command ".setGraphParam $g updateOn $u"
  }
  pack $w.t.l1 $w.t.e1 $w.t.m1 -in $w.t.f6 -side left

  frame $w.t.f7
  frame $w.t.p1 -width 5
  label $w.t.l5 -text "On Net Reset:"
  checkbutton $w.t.c2 -text "Store" -var .g${g}_storeOnReset \
      -command ".setGraphParam $g storeOnReset \[set .g${g}_storeOnReset\]"
  checkbutton $w.t.c1 -text "Clear" -var .g${g}_clearOnReset \
      -command ".setGraphParam $g clearOnReset \[set .g${g}_clearOnReset\]"
  button $w.t.ref -image .refresh -bd 1 -command "graph refresh $g"
  pack $w.t.l5 $w.t.c2 $w.t.c1 -in $w.t.f7 -side left

  pack $w.t.f6 $w.t.f7 -in $w.t.f1 -side left -expand 1
  pack $w.t.ref -in $w.t.f1 -side right
#  pack $w.t.l5 $w.t.c2 $w.t.c1 -in $w.t.f1 -side left

  frame $w.t.f2

  frame $w.t.f3
  label $w.t.l2 -text " Columns:"
  entry $w.t.e2 -width 5 -relief groove -bd 2
  .entryBind $w.t.e2 ".setGraphParam $g cols"
  pack $w.t.l2 $w.t.e2 -in $w.t.f3 -side left

  frame $w.t.f4
  label $w.t.l3 -text "  Max:"
  menubutton $w.t.m2 -text "Auto" -menu $w.t.m2.menu
  menu $w.t.m2.menu -tearoff 0
  $w.t.m2.menu add command -label Auto \
    -command ".setGraphParam $g fixMax 0"
  $w.t.m2.menu add command -label Fixed \
    -command ".setGraphParam $g fixMax 1"
  entry $w.t.e3 -width 10 -relief groove -bd 2
  .entryBind $w.t.e3 ".setGraphParam $g max"
  pack $w.t.l3 $w.t.m2 $w.t.e3 -in $w.t.f4 -side left

  frame $w.t.f5
  label $w.t.l4 -text "  Min:"
  menubutton $w.t.m3 -text "Auto" -menu $w.t.m3.menu
  menu $w.t.m3.menu -tearoff 0
  $w.t.m3.menu add command -label Auto \
    -command ".setGraphParam $g fixMin 0"
  $w.t.m3.menu add command -label Fixed \
    -command ".setGraphParam $g fixMin 1"
  entry $w.t.e4 -width 10 -relief groove -bd 2
  .entryBind $w.t.e4 ".setGraphParam $g min"
  pack $w.t.l4 $w.t.m3 $w.t.e4 -in $w.t.f5 -side left

  pack $w.t.f3 $w.t.f4 $w.t.f5 -in $w.t.f2 -side left -expand 1
  
  pack $w.t.f1 $w.t.f2 -in $w.t -side top -fill x

  frame $w.t.f8
  button $w.t.b1 -text "Store Graph" -command "graph store $g"
  button $w.t.b2 -text "Clear Graph" -command "graph clear $g"
  button $w.t.b3 -text "Add Trace" -command "trace create $g"
  pack $w.t.b1 $w.t.b2 $w.t.b3 -in $w.t.f8 -side left -fill x -expand 1
  pack $w.t.f8 -in $w.t -fill x
  pack $w.t -side top -fill x
  .refreshGraphProps $g
  set .traceProps($g) {}
  .drawAllTraceProps $g
}

proc .setEntry {w value} {
  $w delete 0 end
  $w insert 0 $value
}

proc .refreshGraphProps g {
  global _updateLabel
  set w .grProp$g
# Update Every
  .setEntry $w.t.e1 [getObj graph($g).updateEvery]
# Update On
  set u [expr int([getObj graph($g).updateOn])]
  if [info exists _updateLabel($u)] {
    $w.t.m1 config -text $_updateLabel($u)
  } else {
    $w.t.m1 config -text "Multiple Signals"
  }
# Store/Clear On Reset
  global .g${g}_clearOnReset .g${g}_storeOnReset
  set .g${g}_storeOnReset [getObj graph($g).storeOnReset]
  set .g${g}_clearOnReset [getObj graph($g).clearOnReset]
# Columns
  .setEntry $w.t.e2 [getObj graph($g).cols]
# Max
  if {[getObj graph($g).fixMax]} {
   $w.t.m2 config -text Fixed
  } else {
   $w.t.m2 config -text Auto
  }
  .setEntry $w.t.e3 [getObj graph($g).max]
# Min
  if {[getObj graph($g).fixMin]} {
   $w.t.m3 config -text Fixed
  } else {
   $w.t.m3 config -text Auto
  }
  .setEntry $w.t.e4 [getObj graph($g).min]
}

proc .deleteTraceProps g {
  global .traceProps
  foreach w [set .traceProps($g)] {
    catch {destroy $w}
  }
}

proc .drawTraceProps {g t} {
  global .borderColor .traceProps
  set w .grProp$g.tr$t
  lappend .traceProps($g) $w
  frame $w -relief ridge -bd 4 -bg ${.borderColor}

  frame $w.f1
  frame $w.f2
  label $w.name -text "Trace $t) "

  frame $w.f3
  label $w.l1 -text "Label:"
  entry $w.e1 -width 12 -relief groove -bd 2
  .entryBind $w.e1 ".setTraceParam $g $t label"
  pack $w.l1 $w.e1 -in $w.f3 -side left

  frame $w.f4
  label $w.l3 -text "Object/Proc:"
  entry $w.e2 -width 25 -relief groove -bd 2
  .entryBind $w.e2 ".setTraceParam $g $t object"
  pack $w.l3 $w.e2 -in $w.f4 -side left

  pack $w.name -in $w.f1 -side left 
  pack $w.f3 $w.f4 -in $w.f1 -side left -expand 1

  frame $w.f5
  label $w.l2 -text " Color:"
  button $w.b1 -width 2 -bd -2 \
      -command ".getColor $g $t \[getObject graph($g).trace($t).color\]"
  pack $w.l2 $w.b1 -in $w.f5 -side left

  checkbutton $w.c1 -text "Active" -var .g${g}_${t}_active \
      -command ".setTraceParam $g $t active \$\{.g${g}_${t}_active\}"
  checkbutton $w.c2 -text "Visible" -var .g${g}_${t}_visible \
      -command ".setTraceParam $g $t visible \$\{.g${g}_${t}_visible\}"
  checkbutton $w.c3 -text "Transient" -var .g${g}_${t}_transient \
      -command ".setTraceParam $g $t transient \$\{.g${g}_${t}_transient\}"

  button $w.b2 -text Store -command "trace store $g $t"
  button $w.b3 -text Clear -command "trace clear $g $t"
  button $w.b4 -text Delete -command "trace delete $g $t"
  pack $w.f5 $w.c1 $w.c2 $w.c3 $w.b2 $w.b3 $w.b4 \
      -in $w.f2 -side left -fill x -expand 1

  pack $w.f1 $w.f2 -in $w -side top -fill x
  pack $w -side top -fill x
  .refreshTraceProps $g $t
}

proc .refreshTraceProps {g t} {
  global .g${g}_${t}_active .g${g}_${t}_visible .g${g}_${t}_transient
  set w .grProp$g.tr$t
  .setEntry $w.e1 [getObj graph($g).trace($t).label]
  set color [getObj graph($g).trace($t).color]
  $w.b1 config -bg $color -activeback $color
  .setEntry $w.e2 [getObj graph($g).trace($t).object]
  set .g${g}_${t}_active [getObj graph($g).trace($t).active]
  set .g${g}_${t}_visible [getObj graph($g).trace($t).visible]
  set .g${g}_${t}_transient [getObj graph($g).trace($t).transient]
}

proc .drawGraph g {
  global .borderColor .graphPropUp

  set w .graph$g
  set .graphPropUp($g) 0
  catch {destroy $w}
  toplevel $w
  wm title $w "Graph $g"
  wm protocol $w WM_DELETE_WINDOW "graph delete $g"
  wm geometry $w 600x240
  wm minsize  $w 200 160
  
  frame $w.menubar -relief raised -bd 2
  menubutton $w.menubar.gra -text Graph -menu $w.menubar.gra.menu
  menu $w.menubar.gra.menu -tearoff 0
  $w.menubar.gra.menu add command -label Refresh -command "graph refresh $g"
  $w.menubar.gra.menu add command -label Store -command "graph store $g"
  $w.menubar.gra.menu add command -label Clear -command "graph clear $g"
  $w.menubar.gra.menu add command -label Hide -command "graph hide $g"
  $w.menubar.gra.menu add separator
  $w.menubar.gra.menu add command -label "Add Trace" \
      -command "trace create $g; .drawGraphProps $g"
  $w.menubar.gra.menu add separator
  $w.menubar.gra.menu add command -label "Export Data..."\
      -command ".exportGraph $g"
  $w.menubar.gra.menu add command -label "Print..." -command ".printWin $w.g"
  $w.menubar.gra.menu add separator
  $w.menubar.gra.menu add command -label Close \
      -command "graph delete $g"
  
  button $w.menubar.pro -text Properties -bd 0 \
      -command ".drawGraphProps $g"

  pack $w.menubar.gra $w.menubar.pro -side left -padx 4
  pack $w.menubar -side top -fill x

  frame $w.f -relief ridge -bd 4 -bg ${.borderColor}
  frame $w.l -bd 0
  canvas $w.y -width 40 -bd 0 -highlightthickness 0
  pack $w.y -in $w.l -fill y -padx 0 -expand 1
  
  canvas $w.g -xscrollcommand ".graphScrollX $g" -bd 0 \
      -highlightthickness 0
  scrollbar $w.s -orient horizontal -command "$w.g xview"

  frame $w.buf -height 2
  pack $w.buf -in $w.f -side top -fill x
  pack $w.s -in $w.f -side bottom -fill x
  pack $w.l -in $w.f -side left -fill y
  pack $w.g -in $w.f -fill both -expand 1 -padx 0
  pack $w.f -fill both -expand 1
}

proc .graphScrollX {g a b}  {
  set w .graph$g
  .setGraphSize $g [winfo width $w.g] [winfo height $w.g]
  $w.s set $a $b
}

proc .exportBrowse {} {
  global _fileselect _export .exportFile
  
  .fileselect .fsel [file dirname ${.exportFile}] [file tail ${.exportFile}] \
      $_export(pattern) "Export Data To File" $_export(showdot) 0 1 0
  if {$_fileselect(button) != 0} return
  set _export(path) $_fileselect(path)
  set _export(file) $_fileselect(file)
  set _export(pattern) $_fileselect(pattern)
  set _export(showdot) $_fileselect(showdot)
  set .exportFile $_fileselect(selection)
}

proc .exportIt {g file mode labels} {
  if {![.fileWritable $file]} return
  set command "exportGraph $g $file"
  if {$labels} {append command " -l"}
  if {$mode} {append command " -g"}
  eval $command
  destroy .eg
}

set .gnuplotMode 0
proc .exportGraph g {
  global .exportFile .borderColor .gnuplotMode
  set w .eg
  catch {destroy $w}
  toplevel $w
  wm title $w "Export Graph Data"
  wm resizable $w 0 0

  frame $w.f1 -relief ridge -bd 4 -bg ${.borderColor}
  frame $w.f4
  label $w.l1 -text "File:"
  entry $w.e -width 28 -textvar .exportFile
  button $w.b1 -text "Browse..." -command \
      ".exportBrowse; $w.e xview end"
  pack $w.l1 $w.e $w.b1 -in $w.f4 -side left -fill x
  pack $w.f4 -in $w.f1 -fill x

  frame $w.f2 -relief ridge -bd 4 -bg ${.borderColor}
  frame $w.f5
  label $w.l2 -text Format:
  radiobutton $w.r1 -text "Tabular" -var .gnuplotMode -val 0
  radiobutton $w.r2 -text "Gnuplot" -var .gnuplotMode -val 1
  pack $w.l2 -in $w.f5 -side left
  pack $w.r1 $w.r2 -in $w.f5 -side left -padx 10
  pack $w.f5 -in $w.f2 -fill x

  frame $w.f7 -relief ridge -bd 4 -bg ${.borderColor}
  frame $w.f8
  checkbutton $w.c1 -text "Include Trace Labels" -var .exportLabels
  pack $w.c1 -in $w.f8
  pack $w.f8 -in $w.f7 -fill x

  frame $w.f3 -relief ridge -bd 4 -bg ${.borderColor} 
  frame $w.f6
  button $w.b2 -text Export -default active \
      -command ".exportIt $g \${.exportFile} \${.gnuplotMode} \${.exportLabels}"
  bind $w <Return> ".invokeButton $w.b2"
  button $w.b3 -text Cancel -command "destroy $w"
  pack $w.b2 $w.b3 -in $w.f6 -side left -expand 1 -fill x -pady 0
  pack $w.f6 -in $w.f3 -fill x
  pack $w.f1 $w.f2 $w.f7 $w.f3 -fill x -expand 1

  focus $w
  grab set $w
  .placeDialog $w
}
