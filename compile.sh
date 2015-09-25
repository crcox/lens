#!/bin/bash
set -e

# This script can take one argument, which should be a tar.gz file that
# contains the source for Tcl/Tk 8.6.4 and Lens. If there are no arguments, the
# script will just proceed assuming that the files are all where they should
# be.
# Note that Tcl/Tk libraries will be installed into ${TOP}/usr/local, where
# $TOP is the directory from which you called this script.

mkdir -p usr/local
TOP=$(pwd)

if [ $# -eq 1]
then
  tar xzvf $1
fi

if [ $# -gt 1]
then
  echo "Too many arguments. Exiting..."
  echo "Usage: compile.sh [sourcedist.tar.gz]"
  exit 1
fi

# Compile Tcl
cd "TclTk/tcl8.6.4/unix"
./configure --enable-shared --prefix=${TOP}/usr/local
make
make install

# Compile Tk
cd "../../tk8.6.4/unix"
./configure --enable-shared --prefix=${TOP}/usr/local --with-tcl=../../tcl8.6.4/unix/
make
make install

# Compile Lens
cd ${TOP}
make
