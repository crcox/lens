### Standard fonts
if {${.WINDOWS}} {
  set .normalFont {-family "comic sans ms" -size 8}
  set .largeFont  {-family "comic sans ms" -weight bold -size 10}
  set .consoleFont {-family "lucida console" -size 8}
# Alternate windows fonts:
#  set .normalFont {-family "ms sans serif" -size 8}
#  set .largeFont  {-family "ms sans serif" -size 10}
#  set .consoleFont {-family "courier new" -size 8}
} else {
  set .normalFont {-family helvetica -size -10 -weight bold}
  set .largeFont  {-family helvetica -size -14 -weight bold}
  set .consoleFont {-family courier -size -12}
}

### Prompts
set tcl_prompt1 {puts -nonewline {lens> }}
# This is the secondary prompt used when a command extends over multiple lines
set tcl_prompt2 {}

### Main colors
## This is the application-wide main color scheme:
# set .mainColor   #fffff0
set .mainColor   grey90
## This is for the ridged highlights around frames:
set .borderColor grey60
## This is the background when things are highlighted:
set .activeBGColor white
## This is the foreground when things are highlighted:
set .activeFGColor black

set .entryNormalColor  ${.mainColor}
set .entryPendingColor pink
set .consoleActiveColor blue

### Any calls to "option" must be protected in here because they won't
### work in batch mode.
if {!${.BATCH}} {
  ### Uncomment any of the following to override the default colors
  ## This is the default background of all frames
  #option add *background          pink
  ## The color of text
  #option add *foreground          black
  ## The color of buttons under the cursor
  #option add *activeBackground    red
  ## The color of text on buttons under the cursor
  #option add *activeForeground    black
  ## The color of selected items in list boxes
  #option add *selectBackground    #cecece
  #option add *selectForeground    black
  ## The color of selected items in radio and check boxes
  if {${.WINDOWS}} {option add *selectColor ${.mainColor}} \
  else {option add *selectColor    grey20}
  ## The color of unselected items in radio and check boxes

  ## The color of text on disabled buttons and entries
  #option add *disabledForeground  #acacac
  ## The color of widgets that have the input focus
  #option add *highlightBackground grey90
  #option add *highlightColor      black
  ## The color to use as background in the area covered by the insertion cursor
  #option add *insertBackground    
  ## The color to use for the trough areas in scales and scrollbars
  #option add *troughColor	   #cecece
  ## This sets the width of scrollbars and the scales
  if {${.WINDOWS}} {option add *Scrollbar.width 16} \
  else {option add *Scrollbar.width 12}
  option add *Scale.width 12
  option add *pady 0 0
}

### This sets the parameters that will appear in the main window.
.registerParameter Algorithm learningRate "Learning Rate" .enforceGTE 0.0 0
.registerParameter Algorithm targetRadius "Target Radius" .enforceGTE 0.0 0
.registerParameter Algorithm momentum "Momentum" .enforce0L 1 0
.registerParameter Algorithm zeroErrorRadius "Zero Error Radius" \
	.enforceGTE 0.0 0
.registerParameter Algorithm weightDecay "Weight Decay" .enforce0L 1 0
.registerParameter Algorithm testGroupCrit "Test Group Crit" .enforceGTE \
	-1.0 0
.registerParameter Algorithm outputCostStrength "Cost Strength" \
	.enforceGTE 0.0 0
.registerParameter Algorithm clampStrength "Clamp Strength" .enforce0L 1 0
.registerParameter Algorithm randRange "Init Rand Range" .enforceGTE 0.0 0
.registerParameter Algorithm gain "Gain" \ .enforceGT 0.0 0
if {${.ADVANCED}} {
    .registerParameter Algorithm rateIncrement "Rate Increment" \
	.enforceGTE 0.0 0
    .registerParameter Algorithm rateDecrement "Rate Decrement" \
	.enforce0L 1.0 0
}

.registerParameter Training  batchSize "Batch Size" .enforceGTE 0 1
.registerParameter Training  criterion "Error Criterion" \
	.enforceGTE -1.0 0
.registerParameter Training  numUpdates "Weight Updates" .enforceGT 0 1
.registerParameter Training  reportInterval "Report Interval" .enforceGTE 0 1

### These flags determine which panels will appear by default.
set _showPanel(info)            1
set _showPanel(display)         1
set _showPanel(fileCommand)     1
set _showPanel(algorithm)       1
set _showPanel(algorithmParam)  1
set _showPanel(trainingParam)   1
set _showPanel(trainingControl) 1
set _showPanel(exitButton)      1

### These set the default values for the file browser
set .fileBrowserSize 45x15
## path is the directory in which to begin
## file is the default file name
## pattern is used to filter the directory contents
## showdot is a flag that determines whether to show files beginning with .
## These are used when running scripts
#set _script(path)      .
#set _script(file)      ""
set _script(pattern)   "*.in *.tcl"
set _script(showdot)   0
## These are used when loading or saving weights
set _weights(path)     .
set _weights(file)     ""
set _weights(pattern)  "*.wt *.wt.gz *.wt.bz *.wt.bz2 *.wt.Z"
set _weights(showdot)  0
## These are used when loading or saving examples
set _examples(path)    .
set _examples(file)    ""
set _examples(pattern) "*.*ex *.*ex.gz *.*ex.bz *.*ex.bz2 *.*ex.Z"
set _examples(showdot) 0
## These are used for postscript output files
set _print(path)       .
set _print(file)       ""
set _print(pattern)    "*.ps *.eps"
set _print(showdot)    0
## These are used for exported graph data files
set _export(path)       .
set _export(file)       ""
set _export(pattern)    "*.data"
set _export(showdot)    0
## These are used for network output record files
set _record(path)       .
set _record(file)       ""
set _record(pattern)    "*.out *.out.gz *.out.bz *.out.bz2 *.out.Z *.or *.or.gz *.or.bz *.or.bz2 *.or.Z"
set _record(showdot)    0

### Manual opening
set .manualPage http://www.cs.cmu.edu/~dr/Lens/
if {${.WINDOWS}} {
    set .openManual   {/Program\ Files/Netscape/Communicator/Program/netscape \
	    ${.manualPage}}
} else {
    set .remoteManual {netscape -remote openURL(${.manualPage})}
    set .openManual   {netscape ${.manualPage}}
}

### Adds directories to the auto_path which is used to automatically source
# files that define needed procedures
set auto_path [linsert $auto_path 0 "."]

### Floating point number precision converting from real to string
set tcl_precision 8

### These are the default procedures executed when a USR1 or USR2 signal is
# received.  You can send the signal with "kill -USR1 <process-id>".

set defaultSigUSR1Proc {saveWeights quicksave.wt.gz -v 3}
set defaultSigUSR2Proc {view}

### These procedures will be called whenever an object of the corresponding
# type is created, after the object's extension has been initialized.  You can
# use these to change the default parameter settings in the object or
# extension or to do anything else you'd like.
# The object parameter to all of the procedures (other than .initNetwork) will
# be the path of the object that was just created.

proc .initNetwork {} {
#  setObj learningRate 0.2
}
proc .initGroup   object {
#  setObj $object.targetRadius 0.1
}
proc .initUnit    object {}
proc .initBlock   object {}
proc .initExSet   object {}
proc .initExample object {}
proc .initEvent   object {}

### Aliases
alias mo more
alias ex exec
