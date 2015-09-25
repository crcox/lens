# LENS
The Light Efficient Network Simulator (LENS) was originally written by Douglas Rhode, copyright 1998-2004. Prior to this repo, the last "stable" release was version 2.64-cnbc.1, hosted at Stanford University at http://web.stanford.edu/group/mbc/LENSManual/. This release was still being built against Tcl/Tk 8.3, and was a challenge to build on modern systems.

The source code in this repo has been updated to build against Tcl/Tk 8.6.4. It has been tested on:
- Ubuntu 14.04 (trusty) 64-bit.
- Scientific Linux 6 64-bit.

## Install
The build process is laid out in compile.sh.

First, create a directory to install this local build of Tcl/Tk into:
```{bash}
mkdir -p usr/local
TOP=$(pwd)
```

Then, build Tcl:
```{bash}
# Compile Tcl
cd "TclTk/tcl8.6.4/unix"
./configure --enable-shared --prefix=${TOP}/usr/local
make
make install
```
Then, build Tk:
```{bash}
# Compile Tk
cd "../../tk8.6.4/unix"
./configure --enable-shared --prefix=${TOP}/usr/local --with-tcl=../../tcl8.6.4/unix/
make
make install
```

Finally, build Lens:
```{bash}
# Compile Lens
cd ${TOP}
make
```

## Running Lens
For Lens to run, some environment variables need to be set. If you are running Lens on a distributed cluster, run_Lens.sh will handle setting up the environment on the remote machine.

The spirit of the instructions at http://web.stanford.edu/group/mbc/LENSManual/Manual/running.html still apply to setting up your environment to run Lens, with slight differences. The following instructions will work with bash or zsh.

First, set the `LENSDIR` environment variable.
```{bash}
# Your LENSDIR will probably be $TOP, created at the start of the build process.
export LENSDIR=<your Lens directory>
```

Next, you need to let Lens/your system know where to find the Tcl/Tk libraries we compiled. Add this location to your `LD_LIBRARY_PATH` environment variable.
```{bash}
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${LENSDIR}/usr/local/lib
```

Finally, you need to let your system know where the the `Lens` executable is on your system. Note that the build process did not install Lens anywhere other than `${LENSDIR}/Bin`. So, you need to add that to the system path:
```{bash}
export PATH=${PATH}:${LENSDIR}/Bin
```

You might add all of these `export` commands to your .bashrc (or .zshrc, as the case may be). Equivalent commands are different in other shells, so you may need to Google to make this work with your shell of choice.

## Why LENS?
LENS is a neural network simulator written specifically for cognitive science, and so has many features that more generic packages do not have. It is also lightweight, and can be run in batch mode without ever loading a GUI. This is useful when scaling up or when running many iterations of a simulation in a distributed computing environment. It is written to be extensible. The LENS is (Tcl) script driven, and so you have a great deal of flexibility without needing to write a new C extension.


## Help Wanted
I am working on this revitilization of LENS because I see a pressing need for it. However, there are many things that I am learning as I go in order to do so---this is my first project in C, and by extension using the Tcl API. I have never written a make file, and know very little about configuration and build tools. I am excited to learn, but also very much looking for help even on things that strike you as trivial, because it's probably not trivial from where I am sitting.
