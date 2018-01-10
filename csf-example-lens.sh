#!/bin/bash

# -- SGE optoins (whose lines must begin with #$)

#$ -S /bin/bash # The jobscript is written for the bash shell
#$ -V # Inherit environment settings (e.g., from loaded modulefiles)
#$ -e csf-build-lens-logfiles
#$ -o csf-build-lens-logfiles
#$ -cwd # Run the job in the current directory
#$ -l short

# IMPORTANT! You need to set the LENSDIR environment variable before this will
# work!
# The output will be in the log file!

${LENSDIR}/Bin/lens -batch "${LENSDIR}/Examples/digits_autorun.in"
