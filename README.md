# LENS
The Light Efficient Network Simulator (LENS) was originally written by Douglas Rhode, copyright 1998-2004. Prior to this repo, the last "stable" release was version 2.64-cnbc.1, hosted at Stanford University at http://web.stanford.edu/group/mbc/LENSManual/. This release was still being built against Tcl/Tk 8.3, and was a challenge to build on modern systems.

The source code in this repo has been updated to build against Tcl/Tk 8.6.4. It has been tested on:
- Ubuntu 14.04 (trusty) 64-bit.

## Why LENS?
LENS is a neural network simulator written specifically for cognitive science, and so has many features that more generic packages do not have. It is also lightweight, and can be run in batch mode without ever loading a GUI. This is useful when scaling up or when running many iterations of a simulation in a distributed computing environment. It is written to be extensible. The LENS is (Tcl) script driven, and so you have a great deal of flexibility without needing to write a new C extension.


## Help Wanted
I am working on this revitilization of LENS because I see a pressing need for it. However, there are many things that I am learning as I go in order to do so---this is my first project in C, and by extension using the Tcl API. I have never written a make file, and know very little about configuration and build tools. I am excited to learn, but also very much looking for help even on things that strike you as trivial, because it's probably not trivial from where I am sitting.
