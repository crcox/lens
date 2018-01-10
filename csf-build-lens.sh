#!/bin/bash

# -- SGE optoins (whose lines must begin with #$)

#$ -S /bin/bash # The jobscript is written for the bash shell
#$ -V # Inherit environment settings (e.g., from loaded modulefiles)
#$ -e csf-build-lens-logfiles
#$ -o csf-build-lens-logfiles
#$ -cwd # Run the job in the current directory
#$ -l short

module load compilers/gcc/6.3.0
TCLVERSION=8.6.4
PREFIX="${HOME}"

# Compile Tcl
cd "TclTk/tcl${TCLVERSION}/unix"
./configure --enable-shared --prefix=${PREFIX} && make && make install

# Compile Tk
cd "../../tk${TCLVERSION}/unix"
./configure --enable-shared --with-tcl=../../tcl${TCLVERSION}/unix/ --prefix=${PREFIX} && make && make install

# Compile Lens
cd ../../../
export CFLAGS="-I${PREFIX}/include"
export LDFLAGS="-L${PREFIX}/lib -L${PREFIX}/lib/tcl8.6 -L${PREFIX}/lib/tk8.6"
export LD_RUN_PATH="${PREFIX}/lib:${PREFIX}/lib/tcl8.6:${PREFIX}/lib/tk8.6"
make
