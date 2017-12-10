# Defaults flags:
CC        = gcc
CFLAGS    = -g -Wall
MACHINE   = LINUX
MAKE      = /usr/bin/make
SYSLIB    = -export-dynamic -ldl

DEF= CC="$(CC)" CFLAGS="$(CFLAGS)" MACHINE=$(MACHINE) SYSLIB="$(SYSLIB)" \
	TOP=$(shell pwd) EXT=$(EXT)

MAKE_LENS  = $(MAKE) -C Src lens$(EXT) $(DEF) EXEC=lens PKGS=
MAKE_ALENS = $(MAKE) -C Src alens$(EXT) $(DEF) EXEC=alens PKGS="-DADVANCED"
MAKE_LIBLENS  = $(MAKE) -C Src liblens $(DEF) EXEC=lens PKGS=
MAKE_LIBALENS = $(MAKE) -C Src libalens $(DEF) EXEC=alens PKGS="-DADVANCED"
MAKE_CLEAN = $(MAKE) -C Src $(DEF) clean

lens:: dirs
	$(MAKE_LENS)
liblens:: dirs
	$(MAKE_LIBLENS)
alens:: dirs
	$(MAKE_ALENS)
libalens:: dirs
	$(MAKE_LIBALENS)
clean::
	$(MAKE_CLEAN)

all:: dirs
	rm -f lens$(EXT) alens$(EXT)
	$(MAKE_CLEAN)
	$(MAKE_ALENS)
	$(MAKE_CLEAN)
	$(MAKE_LENS)

# This builds the necessary system-dependent subdirectories on unix machines
ifeq ($(MACHINE),WINDOWS)
dirs::
else
dirs:: Obj/$(HOSTTYPE) Bin/$(HOSTTYPE)
Obj/$(HOSTTYPE)::
	if test ! -d Obj/$(HOSTTYPE); \
	then mkdir -p Obj/$(HOSTTYPE); fi
Bin/$(HOSTTYPE)::
	if test ! -d Bin/$(HOSTTYPE); \
	then mkdir -p Bin/$(HOSTTYPE); fi
endif

sdist:
	-rm send.tgz
	tar czhf send.tgz Src TclTk Commands Examples Makefile
