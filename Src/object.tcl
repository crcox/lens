set .ObjectWindows {}

proc .setObjectGeo win {
    global .memberHeight .entryPadX .entryPadY .buttonPadX .buttonPadY \
	    .menuButtonPadX .menuButtonPadY
    frame $win.e -relief groove -bd 2
    entry $win.e.e -width 16 -bd 0
    button $win.b -width 16 -bd 1 -padx 0 -pady 0
    frame $win.m -relief raised -bd 1
    menubutton $win.m.m -width 16 -bd 0
    pack $win.e.e -padx 0 -pady 0
    pack $win.m.m -padx 0 -pady 0 -ipadx 0 -ipady 0
    pack $win.e $win.b $win.m
    global .WINDOWS
    if {${.WINDOWS}} {update} else {update idletasks}
    set width [winfo width $win.e]
    if {[winfo width $win.b] > $width} {set width [winfo width $win.b]}
    if {[winfo width $win.m] > $width} {set width [winfo width $win.m]}
    set height [winfo height $win.e]
    if {[winfo height $win.b] > $height} {set height [winfo height $win.b]}
    if {[winfo height $win.m] > $height} {set height [winfo height $win.m]}

    set .memberHeight   [expr $height + 2]
    set .entryPadX      [expr ($width - [winfo width $win.e]) / 2]
    set .entryPadY      [expr ($height - [winfo height $win.e]) / 2]
    set .buttonPadX     [expr ($width - [winfo width $win.b]) / 2 + 1]
    set .buttonPadY     [expr ($height - [winfo height $win.b]) / 2 + 1]
    set .menuButtonPadX [expr ($width - [winfo width $win.m]) / 2]
    set .menuButtonPadY [expr ($height - [winfo height $win.m]) / 2]
    destroy $win.e $win.b $win.m
    uplevel #0 {rename .setObjectGeo {}}
}

set .spacerHeight 10
set .MAX_ARRAY 50

proc .unusedObjectWindowIndex {} {
    global .ObjectWindows
    for {set i 0} {[lsearch ${.ObjectWindows} $i] != -1} {incr i} {}
    lappend .ObjectWindows $i
    return $i
}

proc .destroyObjectWindowIndex i {
    global .ObjectWindows
    set l [lsearch ${.ObjectWindows} $i]
    set .ObjectWindows "[lrange ${.ObjectWindows} 0 [expr $l - 1]] \
			   [lrange ${.ObjectWindows} [expr $l + 1] end]"
    destroy .objwin$i
    return
}

proc .refreshObjectWindows {} {
    global .ObjectWindows
    foreach i ${.ObjectWindows} {
	.objwin$i.menubar.refresh invoke
    }
}

proc viewObject {{path ""}} {
    if {$path == "-h"} {return [help viewObject]}

    set i [.unusedObjectWindowIndex]
    set w .objwin$i
    catch {destroy $w}
    toplevel $w
    if {$i} {wm title $w "Object Viewer [expr $i+1]"} \
    else {wm title $w "Object Viewer"}
    wm protocol $w WM_DELETE_WINDOW ".destroyObjectWindowIndex $i"
    catch ".setObjectGeo $w"
    update idletasks
    wm minsize $w 0 100
    wm resizable $w 0 1

    global .pathName$w .newPathName$w .borderColor .RootDir
    set .pathName$w {}
    set .newPathName$w {}

    frame $w.menubar -relief raised -bd 2
    menubutton $w.menubar.win -text Viewer -menu $w.menubar.win.menu
    menu $w.menubar.win.menu -tearoff 0
#    $w.menubar.win.menu add command -label "Refresh" \
#	-command ".viewObject $w \${.pathName$w}"
#   $w.menubar.win.menu add separator
    $w.menubar.win.menu add command -label "Close" \
	-command ".destroyObjectWindowIndex $i"

    button $w.menubar.refresh -image .refresh -bd 1 \
	-command ".viewObject $w \${.pathName$w}" -state normal
    button $w.menubar.up -image .up -bd 1 \
	-command ".viewUp $w" -state disabled
    button $w.menubar.parent -image .up2 -bd 1 \
	-command ".viewParent $w" -state disabled
    button $w.menubar.net -image .up3 -bd 1 \
	-command ".viewObject $w {}" -state disabled
    button $w.menubar.prev -image .left -bd 1 \
	-command ".viewPrev $w" -state disabled
    button $w.menubar.next -image .right -bd 1 \
	-command ".viewNext $w" -state disabled
    pack $w.menubar.win -side left -padx 4 -ipady 0
    pack $w.menubar.refresh $w.menubar.next $w.menubar.prev $w.menubar.up \
	$w.menubar.parent $w.menubar.net -side right
    pack $w.menubar -side top -fill x

    frame $w.f -relief ridge -bd 4 -bg ${.borderColor}

#    frame $w.path -relief ridge -bd 4 -bg ${.borderColor}
    frame $w.ef
    entry $w.entry -relief groove -width 42 -textvar .newPathName$w
    bind $w.entry <Key-Return> ".viewObject $w \${.newPathName$w}"
    pack $w.entry -in $w.ef -side top -fill x -padx 1 -pady 2 -ipady 1 -ipadx 1
    pack $w.ef -in $w.f -side top -fill x
#    pack $w.path -pady 0 -fill x
    
    scrollbar $w.s -command "$w.c yview"
    pack $w.s -in $w.f -side right -fill y
    frame $w.cf2
    frame $w.cf -bd 2 -relief sunken
    global .mainColor
    canvas $w.c -yscrollcommand "$w.s set" -height 449 -width 0 \
	-highlightcolor ${.mainColor}
    pack $w.c -in $w.cf -expand 1 -fill both
    pack $w.cf -in $w.cf2 -expand 1 -fill both -padx 1 -pady 1
    pack $w.cf2 -in $w.f -fill both -expand 1
    pack $w.f -fill both -expand 1
    .wheelScroll $w.c $w.c $w.s

    bind $w <Up> "$w.c yview scroll -1 units"
    bind $w <Down> "$w.c yview scroll 1 units"
    bind $w <Prior> "$w.c yview scroll -1 pages"
    bind $w <Next> "$w.c yview scroll 1 pages"
    
    update idletasks
    wm geometry $w {}
    
    if [catch {.viewObject $w $path} msg] {
	.destroyObjectWindowIndex $i
	error "$msg"
    }
    return
}

proc .addSpacer w {
    global $w.spacers
    incr $w.spacers
}

proc .addLabel {w name label} {
    global .memberHeight .spacerHeight $w.members $w.spacers
    set n $w.c.v$name
    catch [$w.c delete $n]
    catch [destroy $n]
    frame $n
    label $n.lbl -text $label -bg red
    pack $n.lbl
    set y [eval expr ${.memberHeight} * \${$w.members} + \
	       ${.spacerHeight} * \${$w.spacers} + 2]
    $w.c create window [expr [winfo width $w.c] / 2] $y -window $n \
	-anchor n
    incr $w.members
    set y [expr $y + ${.memberHeight}]
    $w.c configure -scrollregion "0 0 0 $y"
    .wheelScroll $w.c $n.lbl
    return
}

proc .addMember {w name value writable} {
    global .pathName$w .memberHeight .spacerHeight .entryPadX .entryPadY \
	$w.members $w.spacers
    eval set path \${.pathName$w}
    set varname ".[string trimleft "$path.$name" "."]"
    global $varname
    set $varname $value
    set n $w.c.v$name
    catch {destroy $n}
    frame $n
    label $n.lbl -text $name
    frame $n.f -relief groove -bd 2
    entry $n.f.ent -width 16 -textvar $varname -bd 0
    if (!$writable) {$n.f.ent configure -state disabled} else {
	bind $n.f.ent <Return> \
	    "setObject {[string trim $varname "."]} \${$varname}"
	bind $n.f.ent <KeyRelease> \
	    ".checkValue $n.f.ent [string trimleft $varname "."]"
    }
    pack $n.f.ent -padx ${.entryPadX} -pady ${.entryPadY}
    pack $n.f -side right -padx 3
    pack $n.lbl -side right -padx 2
    set y [eval expr ${.memberHeight} * \${$w.members} + \
	       ${.spacerHeight} * \${$w.spacers} + 2]
    $w.c create window [winfo width $w.c] $y -window $n -anchor ne
    incr $w.members
    set y [expr $y + ${.memberHeight}]
    $w.c configure -scrollregion "0 0 0 $y"
    .wheelScroll $w.c $n $n.lbl $n.f $n.f.ent
    return
}

proc .addObject {w name type dest valid} {
    global .pathName$w .memberHeight .spacerHeight .buttonPadX .buttonPadY \
	$w.members $w.spacers
    eval set path \${.pathName$w}
    set n $w.c.v$name
    catch {destroy $n}
    frame $n
    label $n.lbl -text $name -anchor e
    button $n.but -width 16 -text $type -bd 1 \
	    -padx ${.buttonPadX} -pady ${.buttonPadY}
    if {$valid} {
	$n.but configure -command ".viewObject $w \{$path$dest\}"
    } else {
	$n.but configure -state disabled
    }
    pack $n.but -side right -padx 3 -anchor n -pady 0
    pack $n.lbl -side right -padx 1
    set y [eval expr ${.memberHeight} * \${$w.members} + \
	       ${.spacerHeight} * \${$w.spacers} + 1]
    $w.c create window [expr [winfo width $w.c]] $y -window $n \
	-anchor ne
    incr $w.members
    set y [expr $y + ${.memberHeight} + 1]
    $w.c configure -scrollregion "0 0 0 $y"
    .wheelScroll $w.c $n $n.lbl $n.but
    return
}

proc .buildObjectArray {w m name dest rows labels} {
    global .pathName$w
    eval set path \${.pathName$w}
    set items [llength $labels]
    for {set i 0} {$i < $items} {incr i} {
	$m add command -label [format "%2d) %s" $i [lindex $labels $i]] \
	    -command ".viewObject $w {$path${dest}($i)}"
    }
    if {$rows > $i} {
	$m add command -label "Array($rows) too large to display"
    }
    $m configure -postcommand {}
}

proc .addObjectArray {w name type dest valid} {
    global .pathName$w .memberHeight .spacerHeight .menuButtonPadX \
	.menuButtonPadY $w.members $w.spacers
    eval set path \${.pathName$w}
    set n $w.c.v$name
    catch {destroy $n}
    frame $n
    label $n.lbl -text $name
    frame $n.f -relief raised -bd 1
    set m $n.f.but.menu
    menubutton $n.f.but -width 16 -text " $type *" \
	-menu $m -bd 0
    if {!$valid} {$n.f.but configure -state disabled}
    menu $m -tearoff false
    
    $m configure -postcommand ".sendObjectArray $w $m $path${dest} $name $dest"

    pack $n.f.but -padx 0 -ipadx ${.menuButtonPadX} \
	-ipady ${.menuButtonPadY} -pady 0
    pack $n.f -side right -padx 4
    pack $n.lbl -side right -padx 1
    set y [eval expr ${.memberHeight} * \${$w.members} + \
	       ${.spacerHeight} * \${$w.spacers} + 2]
    $w.c create window [expr [winfo width $w.c]] $y -window $n \
	-anchor ne
    incr $w.members
    set y [expr $y + ${.memberHeight}]
    $w.c configure -scrollregion "0 0 0 $y"
    .wheelScroll $w.c $n $n.lbl $n.f $n.f.but
    return
}

proc .addObjectArrayArray {w name type dest rows valid} {
    global .pathName$w .memberHeight .spacerHeight .menuButtonPadX \
	.menuButtonPadY .MAX_ARRAY $w.members $w.spacers
    eval set path \${.pathName$w}
    set n $w.c.v$name
    catch {destroy $n}
    frame $n
    label $n.lbl -text $name
    frame $n.f -relief raised -bd 1
    set m $n.f.but.menu
    menubutton $n.f.but -width 16 -text " $type **" \
	-menu $m -bd 0
    if {!$valid} {$n.f.but configure -state disabled}
    menu $m -tearoff false 
    
    for {set i 0} {$i < $rows && $i < ${.MAX_ARRAY}} {incr i} {
	set x $n.f.but.menu$i
	$m add cascade -label $i -menu $x
	menu $x -tearoff false -postcommand \
	    ".sendObjectArray $w $x $path${dest}($i) $name ${dest}($i)"
    }
    if {$rows > $i} {
	$m add command -label "Array($rows) too large to display" 
    }

    pack $n.f.but -padx 0 -ipadx ${.menuButtonPadX} \
	-ipady ${.menuButtonPadY} -pady 0
    pack $n.f -side right -padx 4
    pack $n.lbl -side right -padx 1
    set y [eval expr ${.memberHeight} * \${$w.members} + \
	       ${.spacerHeight} * \${$w.spacers} + 2]
    $w.c create window [expr [winfo width $w.c]] $y -window $n \
	-anchor ne
    incr $w.members
    set y [expr $y + ${.memberHeight}]
    $w.c configure -scrollregion "0 0 0 $y"
    .wheelScroll $w.c $n $n.lbl $n.f $n.f.but
    return
}

proc .buildObjectArrayArray {w name rows cols labels} {
    global .pathName$w
    eval set path \${.pathName$w}
    set n $w.c.v$name
    set m $n.f.but.menu

    for {set i 0} {$i < $rows && $i < ${.MAX_ARRAY}} {incr i} {
	set x $n.f.but.menu$i
	$m add cascade -label $i -menu $x
	menu $x -tearoff false
	for {set j 0} {$j < $cols && $j < ${.MAX_ARRAY}} {incr j} {
	    $x add command -label $j \
		-command ".viewObject $w {$path.${name}($i,$j)}"
	}
	if {$cols > $j} {
	    $x add command -label "Array($cols) too large to display" 
	}
    }
    if {$rows > $i} {
	$m add command -label "Array($rows) too large to display" 
    }
    $m configure -postcommand {}
}

proc .viewObject {w path} {
    global .pathName$w .newPathName$w $w.members $w.spacers
    
    set path [.cleanPath $path]
    
    $w.c delete all
    set $w.members 0
    set $w.spacers 0
    .setNotArray $w
    
    set .pathName$w $path
    while {[catch {.loadObject $w $path} msg]} {
	$w.c delete all
	set $w.members 0
	set $w.spacers 0
	set path [.pathUp $path]
	set .pathName$w $path
    }
    set .newPathName$w $path
    if {$path == {}} {
	$w.menubar.net configure -state disabled
	$w.menubar.parent configure -state disabled
	$w.menubar.up configure -state disabled
    } else {
	$w.menubar.net configure -state normal
	$w.menubar.parent configure -state normal
	$w.menubar.up configure -state normal
    }
    update idletasks
}


proc .viewParent w {
    global .pathName$w
    .viewObject $w [eval .pathParent \${.pathName$w}]
}

proc .viewUp w {
    global .pathName$w
    .viewObject $w [eval .pathUp \${.pathName$w}]
}

proc .viewPrev w {
    global .pathName$w
    .viewObject $w [eval .pathLeft \${.pathName$w}]
}

proc .viewNext w {
    global .pathName$w
    .viewObject $w [eval .pathRight \${.pathName$w}]
}

proc .setNotArray w {
    $w.menubar.prev configure -state disabled
    $w.menubar.next configure -state disabled
}

proc .setArrayCanGoLeft w {
    $w.menubar.prev configure -state normal
}

proc .setArrayCanGoRight w {
    $w.menubar.next configure -state normal
}
