#!/bin/bash
set -e

# This script can take one argument, which should be a tar.gz file that
# contains the source for Tcl/Tk 8.6.4 and Lens. If there are no arguments, the
# script will just proceed assuming that the files are all where they should
# be.
# Note that Tcl/Tk libraries will be installed into ${TOP}/usr/local, where
# $TOP is the directory from which you called this script.

while [[ $# > 1 ]]
do
  key="$1"
  case $key in
      -f)
      TARFILE="$2"
      shift
      ;;
      -t|--tcl-prefix)
      TCLPREFIX="$2"
      shift
      ;;
      -l|--lens-prefix)
      LENSPREFIX="$2"
      shift
      ;;
      -V|--tcltk-version)
      TCLVERSION="$2"
      shift
      ;;
      *)
      echo "Unknown option $1. Exiting..."
      exit 1
      ;;
  esac
  shift
done

TOP=$(pwd)

if [ -z ${TCLVERSION} ]
then
  # Current default version
  TCLVERSION=8.6.4
fi

if [ ! -z ${TARFILE} ]
then
  tar xzvf ${TARFILE}
fi

if [ ! -z ${TCLPREFIX} ]
then
  if [ ! "${TCLPREFIX:0:1}" = "/" ]
  then
    TCLPREFIX="${TOP}/${TCLPREFIX}"
  fi

  mkdir -p "${TCLPREFIX}"
  TCLPREFIXSTATEMENT="--prefix=${TCLPREFIX}"
else
  TCLPREFIXSTATEMENT=""
fi

# Compile Tcl
cd "TclTk/tcl${TCLVERSION}/unix"
./configure --enable-shared $TCLPREFIXSTATEMENT
make
make install

# Compile Tk
cd "../../tk${TCLVERSION}/unix"
./configure --enable-shared --with-tcl=../../tcl${TCLVERSION}/unix/ $TCLPREFIXSTATEMENT
make
make install

# Compile Lens
cd ${TOP}
make
