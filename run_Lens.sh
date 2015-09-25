#!/bin/bash
cleanup() {
  # Remove the Lens distribution
  if [ -f "LensCRC.tgz" ]; then
    rm -v "LensCRC.tgz"
  fi
  # Remove the Tcl add-ons distribution
  if [ -f "tcl_procs.tgz" ]; then
    rm -v "tcl_procs.tgz"
  fi
  # Check the home directory for any transfered files.
  if [ -f ALLURLS ]; then
    while read url; do
      fname=$(basename "$url")
      if [ -f "$fname" ]; then
        rm -v "$fname"
      fi
    done < ALLURLS
  fi
  # Package the weights for transfer and clean up
  # First, check if any weight files exist.
  nwt=$(ls -1 *.wt 2>/dev/null | wc -l)
  if [ $nwt -ne 0 ]; then
    tar czf wt.tgz *.wt && rm *.wt
  fi
  nin=$(ls -1 *.in 2>/dev/null | wc -l)
  if [ $nin -ne 0 ]; then
    tar czf wt.tgz *.wt && rm *.wt
  fi
  ntcl=$(ls -1 *.tcl 2>/dev/null | wc -l)
  if [ $ntcl -ne 0 ]; then
    rm *.tcl
  fi
  if [ -d "Lens" ]; then
    rm -rf Lens
  fi
  if [ -d "ex" ]; then
    rm -rf ex/
  fi
}

abort() {
  echo >&2 '
*************
** ABORTED **
*************
'
  echo >&2 "Files at time of error/interrupt"
  echo >&2 "--------------------------------"
  ls >&2 -l
  cleanup
  echo "An error occured. Exiting ..." >&2
  exit 1
}

success() {
  echo '
*************
** SUCCESS **
*************
'
  cleanup
  exit 0
}

# If an exit or interrupt occurs while the script is executing, run the abort
# function.
trap abort EXIT SIGTERM

set -e

## Set variables
root=$( pwd )
trainscript="trainscript_phase01.tcl"
export LENSDIR=$root/Lens
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:$root/Lens/usr/local/lib

## Install Lens
if [ ! -f LensCRC.tgz ]; then
  wget -q "http://proxy.chtc.wisc.edu/SQUID/crcox/Lens/LensCRC.tgz"
fi
tar xzf "LensCRC.tgz"

if [ ! -f tcl_procs.tgz ]; then
  wget -q "http://proxy.chtc.wisc.edu/SQUID/crcox/Lens/tcl_procs.tgz"
fi
tar xzf "tcl_procs.tgz"

chmod u+x $LENSDIR/Bin/lens-2.63
>&2 echo "INSTALLED Lens at ${root}/Lens/Bin"

# Run Lens
>&2 echo "STARTING LENS"
$LENSDIR/Bin/lens-2.63 -b "$trainscript"
>&2 echo "LENS ENDED"

# Directory state after running lens and cleaning up
ls -ltr

# Close connection to execute node and return remaining files.
exit
