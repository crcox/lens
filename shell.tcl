###############################################################################

set .numAlgorithms 0
set .numAlgorithmParams 0
set .numTrainingParams  0
set .name {}
set .taskDepth {}

source ${.RootDir}/Src/auto.tcl
source ${.RootDir}/Src/init.tcl

###############################################################################

proc min {a b} {
    if {$a < $b} {return $a} else {return $b}
}
proc max {a b} {
    if {$a > $b} {return $a} else {return $b}
}

proc .wait msec {
    set var .w[randInt 1000]
    set $var 0
    after $msec "set $var 1"
    vwait $var
}

###############################################################################

proc .registerCommand {command {synopsis {}}} {
    global .Commands _Synopsis
    if {$command == {}} {
	lappend .Commands {}
    } else {
	lappend .Commands $command
	set _Synopsis($command) $synopsis
    }
    return
}

proc .listCommands {} {
    global .Commands _Synopsis
    set list {}
    foreach command ${.Commands} {
	if {$command != {}} {
	    append list [format "%-19s - %s\n" $command $_Synopsis($command)]
	} else {append list "\n"}
    }
    return $list
}

proc .sortCommands {} {
    global .Commands _Synopsis
    set list {}
    foreach command [lsort ${.Commands}] {
	if {$command != {}} {
	    append list [format "%-19s - %s\n" $command $_Synopsis($command)]
	}
    }
    return $list
}

proc .printSynopsis command {
    global _Synopsis
    puts "$_Synopsis($command)"
    return
}

proc .completeCommand command {
    global .Commands
    set completions {}
    foreach c ${.Commands} {
	if {[string first $command $c] == 0} {lappend completions $c}
    }
    return [lsort $completions]
}

proc .showHelp command {
    global .RootDir
    if [file exists ${.RootDir}/UserCommands/${command}.txt] {
	uplevel "less ${.RootDir}/UserCommands/${command}.txt"
    } else {
	uplevel "less ${.RootDir}/Commands/${command}.txt"
    }
    return
}

proc help {{command {}}} {
    global .Commands _Aliases
    if {$command == {}} {
	uplevel {less << [.listCommands]}
    } elseif {$command == "-s"} {	
	uplevel {less << [.sortCommands]}
    } elseif {$command == "-h"} {	
	.showHelp help
    } else {
	if {[lsearch ${.Commands} $command] == -1} {
	    if {[lsearch [array name _Aliases] $command] > -1} {
		return "$command is an alias for \"$_Aliases($command)\""
	    }
	    set completion [.completeCommand $command]
	    if {[llength $completion] == 1} {
		.showHelp ${completion}
	    } elseif {[llength $completion] > 1} {
		return "ambiguous command name \"$command\": ${completion}"
	    } else {
	       return "$command is not a valid command or no help is available"
	    }
	} else {.showHelp $command}
    }
    return
}

proc manual {} {
    global .remoteManual .openManual .manualPage .WINDOWS
    if {${.WINDOWS}} {set redir ">&NUL: <NUL:"} else \
	{set redir ">&/dev/null </dev/null"}
    if [info exists .remoteManual] {
	if {![catch {eval exec $redir ${.remoteManual}}]} \
	    return
    }
    eval exec $redir ${.openManual} &
    return
}

###############################################################################

proc version {} {
    global .Version
    return ${.Version}
}

proc args args {
    foreach a $args {puts $a}
    return
}

proc alias {{command {}} {substitute -1}} {
    global _Aliases
    if {$command == "-h"} {
	return [help alias]
    } elseif {$command == {}} {
	set list {}
	foreach i [array names _Aliases] {
	    set list "$list[format "%-10s %s\n" $i $_Aliases($i)]"
	}
	return $list
    } elseif {$substitute == -1} {
	if {[lsearch [array names _Aliases] $command] != -1} {
	    return $_Aliases($command)
	} else {
	    return ""
	}
    } elseif {$substitute == {}} {
	unalias $command
	return
    }
    set _Aliases($command) $substitute
    return $substitute
}

proc unalias command {
    global _Aliases
    if {$command == "-h"} {
	return [help unalias]
    }
    catch {unset _Aliases($command)}
    return
}

proc .histsub patsub {
    set patsub [split $patsub ^]
    set pat [lindex $patsub 1]
    set sub [lindex $patsub 2]
    set event [history event]
    set pos [string first $pat $event]
    if {$pos == -1} {return $event}
    set command [string range $event 0 [expr $pos - 1]]$sub[string range \
	   $event [expr $pos + [string length $pat]] end]
#    history
    return $command
}

auto_load history 
rename history defhistory
proc history args {
    set len [llength $args]
    if {[lindex $args 0] != "add" || [lindex $args 1] != "\n"} {
      eval defhistory $args
    }
}

rename unknown unknown.default
proc unknown {command args} {
    global _Aliases errorInfo errorCode .CONSOLE .sourceDepth

# First try for an alias
    if {[lsearch [array name _Aliases] $command] > -1} {
	set code [catch {uplevel $_Aliases($command) $args} mess]
# Then try for an executable
    } elseif {[set cmd [auto_execok $command]] != ""} {
	if {${.CONSOLE}} {
	    if {${.sourceDepth} == 0 && [info level] <= 1} {
		set code [catch {uplevel .consoleExec $cmd $args 2>@ x} mess]
	    } else {
		set code [catch {uplevel exec $cmd $args} mess]
	    }
	} else {
	    set code [catch {uplevel exec >&@stdout <@stdin $cmd $args} mess]
	}
# Then try for a history command repeat
    } elseif {[string index $command 0] == "!"} {
	if {[string index $command 1] == "!"} {
	    set command "[history event][string range $command 2 end] $args"
	} else {
	    set command [history event [string range $command 1 end]]
	}
	history change $command
	set code [catch {uplevel $command} mess]
# Then try for a history command substitution
    } elseif {[string index $command 0] == "^"} {
	set command [.histsub $command]
	history change $command
	set code [catch {uplevel $command} mess]
# Then try for a command completion
    } else {
	set cmds [info commands $command*]
	if {[llength $cmds] == 1} {
	    set code [catch {uplevel $cmds $args} mess]
# Otherwise, go for the default one
	} else {
	    set code [catch {uplevel unknown.default $command $args} mess]
	}
    }
    switch $code {
	1 {return -code error -errorinfo $errorInfo \
	       -errorcode $errorCode $mess}
	2 {return -code return $mess}
	3 {return}
	default {return -code $code $mess}
    }
}

proc repeat {var iters {body .nobody}} {
  if {$body == ".nobody"} {
    set body $iters
    set iters $var
    set var {}
  }

  if {$var != {}} {upvar $var v}
  if {[llength $iters] == 2} {
    set start [lindex $iters 0]
    set iters [lindex $iters 1]
  } else {
    set start 0
    if {$iters > 0} {incr iters -1} else {incr iters}
  }
  if {$start <= $iters} {
    for {set i $start} {$i <= $iters} {incr i} {
      if {$var != {}} {set v $i}
      uplevel $body
    }
  } else {
    for {set i $start} {$i >= $iters} {incr i -1} {
      if {$var != {}} {set v $i}
      uplevel $body
    }
  }
}

proc alarm {} {
    beep
    after 250
    beep
    after 250
    beep
}

###############################################################################

proc view {{flag {}}} {
    global .RootDir .BATCH
    if {$flag == "-h"} {return [help view]}
    if {${.BATCH}} {return "view is not available in batch mode"}
    catch {wm iconify .}
    uplevel #0 source ${.RootDir}/Src/interface.tcl
    catch {wm deiconify .}
}

proc sendObject {object value} {
    if {$object == "-h"} {return [help sendObject]}
    uplevel #0 setObject $object $value
    sendEval "setObject $object $value"
}

###############################################################################

# This turns a relative or absolute directory into an absolute 
# with no .'s or ..'s
proc .normalizeDir {path dir} {
    if {[file pathtype $dir] == "relative"} {
	if {$path != {}} {
	    set dir "$path$dir"
	} else {
	    set dir "[pwd]/$dir"
	}
    }
    if {[string index $dir 0] == "/"} {set initSlash 1} else {set initSlash 0}
    set list [split $dir "/"]
    set dir {}
    foreach i $list {
	switch $i {
	    ""     -
	    "."    {}
	    ".."   {if {[llength $dir] > 0} \
			{set dir [lrange $dir 0 [expr [llength $dir] - 2]]}
	    }
	    default "lappend dir $i"
	}
    }
    
    if {$dir == ""} {return "/"}
    if {$initSlash} {
	set dir /[join $dir "/"]/
    } else {
	set dir [join $dir "/"]/
    }
    return $dir
}

proc .exampleSetRoot file {
    set file [file tail $file]
    if {[file extension $file] == ".gz" || [file extension $file] == ".Z"} {
	set file [file rootname $file]
    }
    if {[file extension $file] == ".ex" || [file extension $file] == ".bex"} {
	set file [file rootname $file]
    }
    return $file
}

###############################################################################

proc .clearParameters {} {
    global .numAlgorithmParams .numTrainingParams \
	_Algorithm.parShortName _Training.parShortName .algorithm
    for {set i 0} {$i < ${.numAlgorithmParams}} {incr i} {
	set shortName [set _Algorithm.parShortName($i)]
	global .$shortName
	set .$shortName ""
    }
    for {set i 0} {$i < ${.numTrainingParams}} {incr i} {
	set shortName [set _Training.parShortName($i)]
	global .$shortName
	set .$shortName ""
    }
    set .algorithm {}
    return
}

proc .getParameters {} {
    global .numAlgorithmParams .numTrainingParams \
	_Algorithm.parShortName _Training.parShortName .algorithm
    for {set i 0} {$i < ${.numAlgorithmParams}} {incr i} {
	set shortName [set _Algorithm.parShortName($i)]
	global .$shortName
	set .$shortName [getObject $shortName]
    }
    for {set i 0} {$i < ${.numTrainingParams}} {incr i} {
	set shortName [set _Training.parShortName($i)]
	global .$shortName
	set .$shortName [getObject $shortName]
    }
    set .algorithm ${.algorithm}
    return
}

###############################################################################

proc .checkNum val {
    if {[catch {expr $val}] != 0} {
	error "\"$val\" is not a valid number"
    }
    return
}

proc .enforceGTE {var val limit int} {
    global $var
    .checkNum $val
    if $int {set $var [expr int($val)]}
    if {$val < $limit} {error "Bad value for $var: $val.\
          It cannot be less than $limit."}
}

proc .enforceGT {var val limit int} {
    global $var
    .checkNum $val
    if $int {set $var [expr int($val)]}
    if {$val <= $limit} {error "Bad value for $var: $val.\
          It must be greater than $limit."}
}

proc .enforceLTE {var val limit int} {
    global $var
    .checkNum $val
    if $int {set $var [expr int($val)]}
    if {$val > $limit} {error "Bad value for $var: $val.\
          It cannot be greater than $limit."}
}

proc .enforceLT {var val limit int} {
    global $var
    .checkNum $val
    if $int {set $var [expr int($val)]}
    if {$val >= $limit} {error "Bad value for $var: $val.\
          It must be less than $limit."}
}

proc .enforce0L {var val limit int} {
    global $var
    .checkNum $val
    if $int {set $var [expr int($val)]}
    if {($val < 0) || ($val >= $limit)} {
	error "Bad value for $var: $val. It must be in the range \[0,$limit\)."
    }
}

proc .enforceANY {var val limit int} {}

###############################################################################

proc .registerAlgorithm {shortName longName code} {
    global .numAlgorithms _algShortName _algLongName _algCode

    set _algShortName(${.numAlgorithms}) $shortName 
    set _algLongName(${.numAlgorithms}) $longName 
    set _algCode(${.numAlgorithms}) $code

    incr .numAlgorithms
# This creates a command such as "steepest"
    proc $shortName args \
	"if \{\$args == \"-h\"\} \{return \[help $shortName\]\}; \
         eval train \$args -a $shortName"
}

proc .registerParameter {set shortName longName checkProc limit int} {
    global .num${set}Params _$set.parShortName _$set.parLongName
    if {[info commands .set$shortName] != {}} return
    eval set num $\{.num${set}Params\}
    set _$set.parShortName($num) $shortName
    set _$set.parLongName($num) $longName
    incr .num${set}Params
    
    proc .set$shortName {} "
	global .$shortName
	$checkProc \{$longName\} $\{.$shortName\} $limit $int
	setObject $shortName $\{.$shortName\}
	return
    "
}


###############################################################################

proc .queryExit {} {
  if {[tk_dialog .exit {Warning} {Are you sure you want to kill Lens?} \
	   questhead 0 "Quite Sure" "Oops, Scratch That"] == 0} {
      puts {}
      exit
  }
}

###############################################################################

proc .showAbout {} {
    global .RootDir .LargeFont .Version
    set w .about
    catch {destroy $w}
    toplevel $w
    wm title $w "About"
    
    catch {font delete .bigfont}
    frame $w.top -relief raised -bd 1
    image create photo $w.title -file ${.RootDir}/Src/Images/lens.medium.gif
    label $w.top.title -image $w.title
    image create photo $w.icon -file ${.RootDir}/Src/Images/lensIcon.gif
    label $w.top.icon -image $w.icon

    label $w.top.slogan -text "the light, efficient network simulator" \
	-fg \#888888 -font .LargeFont 
    label $w.top.version -text "version ${.Version}\nwritten by Doug Rohde (dr+lens@cs.cmu.edu)\nCenter for the Neural Basis of Cognition\nCarnegie Mellon University" -font .LargeFont -justify center

    label $w.top.message -text "Lens and its source code is available free of charge only for experimental research at academic institutions.  A license must be obtained from Doug Rohde if Lens is to be used by other individuals or for other purposes.\n\nLens was originally based on the Xerion neural network simulator produced by Drew van Camp and Tony Plate at the University of Toronto.  The design of the graphical displays and the implementation of the training algorithms borrows somewhat from Xerion V3.1.\n\nThe graphical and scripting interfaces to Lens were written using the Tcl/Tk Toolkit, available at http://www.scriptics.com." \
	-justify left -wraplength 500
    button $w.top.copy -text "Tcl/Tk Copyright Notice" -command .tclCopyright

    pack $w.top.title $w.top.slogan $w.top.version $w.top.message $w.top.copy \
      -padx 10 -pady 5
    place $w.top.icon -x 2 -y 30

    frame $w.bot -relief raised -bd 1
    button $w.bot.ok -text "Okey Dokey" -default active \
      -command "destroy $w"
    pack $w.bot.ok -expand 1 -fill x -padx 10

    pack $w.top $w.bot -fill x -expand 1
}

proc .tclCopyright {} {
    set w .tclcopy
    catch {destroy $w}
    toplevel $w
    wm title $w "Tcl/Tk Copyright Notice"
      
    frame $w.top -relief raised -bd 1
    label $w.top.msg -text "This software is copyrighted by the Regents of the University of California, Sun Microsystems, Inc., and other parties.  The following terms apply to all files associated with the software unless explicitly disclaimed in individual files.

The authors hereby grant permission to use, copy, modify, distribute, and license this software and its documentation for any purpose, provided that existing copyright notices are retained in all copies and that this notice is included verbatim in any distributions. No written agreement, license, or royalty fee is required for any of the authorized uses. Modifications to this software may be copyrighted by their authors and need not follow the licensing terms described here, provided that the new terms are clearly indicated on the first page of each file where they apply.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN \"AS IS\" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

GOVERNMENT USE: If you are acquiring this software on behalf of the U.S. government, the Government shall have only \"Restricted Rights\" in the software and related documentation as defined in the Federal Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you are acquiring the software on behalf of the Department of Defense, the software shall be classified as \"Commercial Computer Software\" and the Government shall have only \"Restricted Rights\" as defined in Clause 252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing, the authors grant the U.S. Government and others acting in its behalf permission to use and distribute the software in accordance with the terms specified in this license." -justify left -wraplength 500 
    pack $w.top.msg -padx 10 -pady 5

    frame $w.bot -relief raised -bd 1
    button $w.bot.ok -text "Very Nice" -default active -command "destroy $w"
    pack $w.bot.ok -expand 1 -fill x -padx 10

    pack $w.top $w.bot -fill x -expand 1
}

###############################################################################

source ${.RootDir}/Src/lensrc.tcl
if [file readable ~/.lensrc] {
    source ~/.lensrc
}
auto_load_index
auto_load bgerror

if {!${.BATCH}} {
    tk_setPalette ${.mainColor}
    option add *background ${.mainColor}
    option add *activeBackground ${.activeBGColor}
    option add *activeForeground ${.activeFGColor}
    catch {font delete .NormalFont}
    eval font create .NormalFont ${.normalFont}
    catch {font delete .LargeFont}
    eval font create .LargeFont ${.largeFont}
    option add *font .NormalFont userDefault
    option add *Dialog.msg.font .LargeFont userDefault
    option add *Dialog.msg.wrapLength 5i userDefault

    bind Entry <Key-Delete> "tkEntryBackspace %W"
    source ${.RootDir}/Src/file.tcl
    source ${.RootDir}/Src/fileselect.tcl
}

###############################################################################

if {!${.BATCH}} {
    image create bitmap .refresh -foreground black \
	-file ${.RootDir}/Src/Images/reload.xbm
    foreach i {up up2 up3 down left left2 left3 right right2 right3 \
		   right2b right3b} {
	image create bitmap .${i} -foreground black \
	    -file ${.RootDir}/Src/Images/${i}.xbm
    }
    source ${.RootDir}/Src/display.tcl
    source ${.RootDir}/Src/object.tcl
    source ${.RootDir}/Src/graph.tcl
} else {
    proc viewObject {{path ""}} {
	if {$path == "-h"} {return [help viewObject]}
	return "viewObject is not available in batch mode"
    }
    proc graphObject {{path error}} {
	if {$path == "-h"} {return [help graphObject]}
	return "graphObject is not available in batch mode"
    }
}
# This means the unit display is not showing
set .unitUp 0
# This means the link display is not showing
set .linkUp 0

return
