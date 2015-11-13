VERSION= 2.63
SRCS= type util network connect example act train object command control \
	display graph parallel networkCom connectCom exampleCom trainCom \
	objectCom displayCom graphCom parallelCom canvRect extension

O=      ../Obj/$(HOSTTYPE)
TCLVER= 8.6.4
TCLDIR= ../TclTk
BINDIR= $(TOP)/Bin/$(HOSTTYPE)
LIBDIR= $(TOP)/usr/local/lib
LDOPTS= -Wl,-rpath,$(LIBDIR)
INCL=   -I$(TCLDIR)/tcl$(TCLVER)/generic -I$(TCLDIR)/tk$(TCLVER)/generic \
        -I$(TCLDIR)/tcl$(TCLVER)/unix    -I$(TCLDIR)/tk$(TCLVER)/unix
LIBS=   -L$(LIBDIR) -ltk8.6 -ltcl8.6 -lm -lX11 $(SYSLIB)

EXEC_OBJS= $(O)/main.o $(patsubst %,$(O)/%.o,$(SRCS))
LIB_OBJS= $(O)/library.o $(patsubst %,$(O)/%.o,$(SRCS))
OBJS= $(O)/main.o $(O)/library.o $(patsubst %,$(O)/%.o,$(SRCS))

default:
	@echo "You shouldn't be calling this Makefile directly"

$(EXEC)$(EXT): ../$(EXEC)$(EXT)
../$(EXEC)$(EXT): $(EXEC_OBJS) Makefile
	$(CC) $(LDOPTS) -o $(BINDIR)/$(EXEC)-$(VERSION)$(EXT) $(EXEC_OBJS) $(LIBS)
	ln -s -f $(BINDIR)/$(EXEC)-$(VERSION)$(EXT) $(BINDIR)/$(EXEC)$(EXT)
	ln -s -f $(BINDIR)/$(EXEC)-$(VERSION)$(EXT) ../$(EXEC)$(EXT)

lib$(EXEC): $(BINDIR)/lib$(EXEC)$(VERSION).a
$(BINDIR)/lib$(EXEC)$(VERSION).a:	$(LIB_OBJS) Makefile
	ar cr $(BINDIR)/lib$(EXEC)$(VERSION).a $(LIB_OBJS)

$O/main.o      :main.c system.h util.h network.h type.h connect.h command.h \
		control.h display.h canvRect.h train.h extension.h Makefile
$O/library.o   :library.c system.h util.h type.h extension.h command.h \
		control.h train.h lens.h
$O/type.o      :type.c system.h util.h network.h connect.h type.h train.h act.h

$O/util.o      :util.c util.h type.h parallel.h defaults.h

$O/network.o   :network.c system.h util.h type.h network.h connect.h act.h \
		control.h display.h train.h
$O/connect.o   :connect.c system.h util.h type.h network.h connect.h

$O/example.o   :example.c system.h util.h type.h network.h example.h \
		control.h object.h
$O/act.o       :act.c system.h util.h type.h network.h act.h control.h \
		display.h graph.h train.h connect.h
$O/train.o     :train.c system.h util.h type.h network.h act.h train.h \
		control.h display.h graph.h
$O/object.o    :object.c system.h util.h type.h network.h connect.h example.h \
		object.h graph.h
$O/command.o   :command.c util.h type.h command.h control.h

$O/control.o   :control.c system.h util.h type.h network.h connect.h \
		example.h display.h graph.h control.h parallel.h canvRect.h
$O/display.o   :display.c system.h util.h type.h network.h connect.h \
		control.h example.h display.h parallel.h graph.h
$O/graph.o     :graph.c system.h util.h type.h network.h object.h graph.h \
		parallel.h
$O/parallel.o  :parallel.c util.h type.h network.h train.h act.h display.h \
		graph.h control.h parallel.h
$O/networkCom.o:networkCom.c system.h util.h type.h network.h connect.h \
		command.h control.h act.h display.h
$O/connectCom.o:connectCom.c system.h util.h type.h network.h connect.h \
		command.h control.h act.h display.h object.h
$O/exampleCom.o:exampleCom.c system.h util.h type.h example.h network.h \
		command.h control.h display.h act.h defaults.h
$O/trainCom.o  :trainCom.c system.h util.h type.h network.h train.h act.h \
		command.h control.h display.h graph.h
$O/objectCom.o :objectCom.c system.h util.h type.h network.h object.h \
		command.h control.h
$O/displayCom.o:displayCom.c system.h util.h type.h network.h connect.h act.h \
		command.h control.h display.h graph.h
$O/graphCom.o  :graphCom.c system.h util.h network.h command.h control.h \
		graph.h
$O/parallelCom.o:parallelCom.c system.h util.h type.h network.h command.h \
		control.h train.h parallel.h
$O/canvRect.o  :canvRect.c system.h util.h type.h network.h connect.h \
		display.h
$O/extension.o :extension.c system.h util.h type.h network.h connect.h \
		example.h train.h act.h command.h control.h object.h

$(OBJS):
	$(CC) $(CFLAGS) -DMACHINE_$(MACHINE) -DVERSION=\"$(VERSION)\" \
	-DTCLVER=\"$(TCLVER)\" $(INCL) $(PKGS) -c -o $@ \
	$(*F).c

clean:
	rm -f $(OBJS)
