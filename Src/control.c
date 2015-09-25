#include "system.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include <signal.h>
#include "util.h"
#include "type.h"
#include "network.h"
#include "connect.h"
#include "example.h"
#include "display.h"
#include "graph.h"
#include "control.h"
#include "parallel.h"
#include "canvRect.h"

typedef struct task *Task;
struct task {
  mask type;
  flag halted;
  Task prev;
  unsigned int timeStarted;
} *Tasks;

flag queryUser(char *fmt, ...) {
  va_list args;
  flag answer = 2;
  String buf = newString(strlen(fmt) + 64);
  int val;
  Tcl_Obj *result;

  va_start(args, fmt);
  if (!Batch) {
    sprintf(buf->s, "tk_dialog .query \"Hey, you\" {%s} questhead 1 No Yes",
	    fmt);
    vsprintf(Buffer, buf->s, args);
    Tcl_Eval(Interp, Buffer);
    result = Tcl_GetObjResult(Interp);
    Tcl_GetIntFromObj(Interp, result, &val);
    if (val==1) {
        answer = TRUE;
    } else {
      answer = FALSE;
    }
  } else do {
    print(0, "\n");
    vsprintf(buf->s, fmt, args);
    print(0, buf->s);
    print(0, " ");
    fgets(buf->s, 16, stdin);
    if (!strcmp(buf->s, "y\n") || !strcmp(buf->s, "yes\n"))
      answer = TRUE;
    if (!strcmp(buf->s, "n\n") || !strcmp(buf->s, "no\n"))
      answer = FALSE;
  } while (answer == 2);
  va_end(args);
  freeString(buf);
  return answer;
}


/******************************* Command Creation ****************************/

flag createCommand(Tcl_ObjCmdProc *proc, char *name) {
  if (!Tcl_CreateObjCommand(Interp, name, proc,
			 (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL))
    fatalError("%s, failed to create command \"%s\"",
	       Tcl_GetStringResult(Interp), name);
  return TCL_OK;
}

flag registerSynopsis(char *name, char *synopsis) {
  if (eval(".registerCommand %s {%s}", name, synopsis))
    return error("%s, failed to register \"%s\"",
		 Tcl_GetStringResult(Interp), name);
  return TCL_OK;
}

flag registerCommand(Tcl_ObjCmdProc *proc, char *name, char *synopsis) {
  if (proc) {
    if (createCommand(proc, name))
      fatalError("failed to create command \"%s\"", name);
    sprintf(Buffer, ".registerCommand %s {%s}", name, synopsis);
  } else sprintf(Buffer, ".registerCommand {}");
  if (Tcl_Eval(Interp, Buffer))
    return error("%s, failed to perform \"%s\"", Tcl_GetStringResult(Interp),
		 Buffer);
  return TCL_OK;
}

flag printSynopsis(char *name) {
  return eval(".printSynopsis %s", name);
}


/******************************* Interface Updating **************************/

/* This should only be called if Gui is true */
flag configureDisplay(void) {
  flag net = TRUE, training = (Net != NULL), testing = (Net != NULL);
  Task T;
  char *state, *on = "normal", *off = "disabled";
  if (eval("wm title . \"%s\"", (Net) ? Net->name : "Lens")) return TCL_ERROR;

  for (T = Tasks; T; T = T->prev) {
    switch (T->type) {
    case TRAINING:
    case PARA_TRAINING:
      net = training = FALSE;
      break;
    case TESTING:
      net = testing = FALSE;
      if (!Net || !Net->testingSet) training = FALSE;
      break;
    case BUILDING_LINKS:
      net = FALSE;
      break;
    }
  }
  if (!Root->numNetworks) net = FALSE;
  if (!Root->numExampleSets) training = testing = FALSE;

  if (eval(".infoPanel.l.network config -state %s;"
	   ".infoPanel.m.trainingSet config -state %s;"
	   ".infoPanel.r.testingSet config -state %s;",
	   (net) ? on : off, (training) ? on : off,
	   (testing) ? on : off)) return TCL_ERROR;
  if (eval("foreach .i {.infoPanel.l.entry "
	   ".displayPanel.l.unit .displayPanel.l.grph .displayPanel.r.link "
	   ".fileCommandPanel.l.swgt .fileCommandPanel.r.lwgt "
	   ".trainingControlPanel.l.rstn .trainingControlPanel.l.test "
	   ".trainingControlPanel.r.outp .trainingControlPanel.train} "
	   "{${.i} config -state %s}", (Net) ? on : off)) return TCL_ERROR;
  if (eval(".infoPanel.m.entry config -state %s;"
	   ".infoPanel.r.entry config -state %s",
	   (Net && Net->trainingSet) ? on : off,
	   (Net && Net->testingSet)  ? on : off)) return TCL_ERROR;
  if (eval(".trainingControlPanel.r.rste config -state %s;"
	   ".trainingControlPanel.train config -state %s;"
	   ".trainingControlPanel.l.test config -state %s",
	   (Net && Net->trainingSet) ? on : off,
	   (Net && Net->trainingSet) ? on : off,
	   (Net && (Net->trainingSet || Net->testingSet)) ? on : off))
    return TCL_ERROR;

  if (Net) {
    if (eval("set .color [.displayPanel.l.unit cget -foreground]"))
      return TCL_ERROR;
    state = on;
  } else {
    if (eval("set .color [.displayPanel.l.unit cget -disabledforeground]"))
      return TCL_ERROR;
    state = off;
  }
  if (eval("for {set .i 0} {${.i} < ${.numAlgorithms}} {incr .i} {"
	   ".algorithmPanel.$_algShortName(${.i}) config -state %s};"
	   "for {set .i 0} {${.i} < ${.numAlgorithmParams}} {incr .i} {"
	   "set shortName [set _Algorithm.parShortName(${.i})];"
	   ".algorithmParamPanel.${shortName}_l config -fg ${.color};"
	   ".algorithmParamPanel.${shortName}_e config -state %s};"
	   "for {set .i 0} {${.i} < ${.numTrainingParams}} {incr .i} {"
	   "set shortName [set _Training.parShortName(${.i})];"
	   ".trainingParamPanel.${shortName}_l config -fg ${.color};"
	   ".trainingParamPanel.${shortName}_e config -state %s}",
	   state, state, state))
    return TCL_ERROR;

  if (UnitUp) {
    if (eval(".unit.menubar.set config -state %s;"
	     ".unit.menubar.set.menu entryconfig 0 -state %s;"
	     ".unit.menubar.set.menu entryconfig 1 -state %s",
	     (Net && (Net->testingSet || Net->trainingSet)) ? on : off,
	     (Net && Net->trainingSet) ? on : off,
	     (Net && Net->testingSet) ? on : off)) return TCL_ERROR;
  }
  if (LinkUp) {
    if (eval(".link.menubar.win.menu entryconfig 1 -state %s;"
	     ".link.menubar.win.menu entryconfig 2 -state %s;"
	     ".link.menubar.val config -state %s", state, state, state))
      return TCL_ERROR;
  }
  return TCL_OK;
}

flag signalCurrentNetChanged(void) {
  Tcl_UnlinkVar(Interp, ".algorithm");
  Tcl_UnlinkVar(Interp, ".unitUpdates");
  Tcl_UnlinkVar(Interp, ".unitCellSize");
  Tcl_UnlinkVar(Interp, ".unitCellSpacing");
  Tcl_UnlinkVar(Interp, ".unitDisplaySet");
  Tcl_UnlinkVar(Interp, ".unitDisplayValue");
  Tcl_UnlinkVar(Interp, ".unitLogTemp");
  Tcl_UnlinkVar(Interp, ".unitPalette");
  Tcl_UnlinkVar(Interp, ".unitExampleProc");
  Tcl_UnlinkVar(Interp, ".linkUpdates");
  Tcl_UnlinkVar(Interp, ".linkCellSize");
  Tcl_UnlinkVar(Interp, ".linkCellSpacing");
  Tcl_UnlinkVar(Interp, ".linkDisplayValue");
  Tcl_UnlinkVar(Interp, ".linkLogTemp");
  Tcl_UnlinkVar(Interp, ".linkPalette");
  if (Net) {
    Tcl_LinkVar(Interp, ".algorithm", (char *) &(Net->algorithm),
		TCL_LINK_INT);

    Tcl_LinkVar(Interp, ".unitUpdates", (char *) &(Net->unitUpdates),
		TCL_LINK_INT);
    Tcl_LinkVar(Interp, ".unitCellSize", (char *) &(Net->unitCellSize),
		TCL_LINK_INT);
    Tcl_LinkVar(Interp, ".unitCellSpacing", (char *) &(Net->unitCellSpacing),
		TCL_LINK_INT);
    Tcl_LinkVar(Interp, ".unitDisplaySet", (char *) &(Net->unitDisplaySet),
		TCL_LINK_INT);
    Tcl_LinkVar(Interp, ".unitDisplayValue", (char *) &(Net->unitDisplayValue),
		TCL_LINK_INT);
    Tcl_LinkVar(Interp, ".unitLogTemp", (char *) &(Net->unitLogTemp),
		TCL_LINK_DOUBLE);
    Tcl_LinkVar(Interp, ".unitPalette", (char *) &(Net->unitPalette),
		TCL_LINK_INT);
    Tcl_LinkVar(Interp, ".unitExampleProc", (char *) &(Net->unitExampleProc),
		TCL_LINK_INT);

    Tcl_LinkVar(Interp, ".linkUpdates", (char *) &(Net->linkUpdates),
		TCL_LINK_INT);
    Tcl_LinkVar(Interp, ".linkCellSize", (char *) &(Net->linkCellSize),
		TCL_LINK_INT);
    Tcl_LinkVar(Interp, ".linkCellSpacing", (char *) &(Net->linkCellSpacing),
		TCL_LINK_INT);
    Tcl_LinkVar(Interp, ".linkDisplayValue", (char *) &(Net->linkDisplayValue),
		TCL_LINK_INT);
    Tcl_LinkVar(Interp, ".linkLogTemp", (char *) &(Net->linkLogTemp),
		TCL_LINK_DOUBLE);
    Tcl_LinkVar(Interp, ".linkPalette", (char *) &(Net->linkPalette),
		TCL_LINK_INT);
  }

  if (Net) eval("set .name {%s}; .getParameters", Net->name);
  else Tcl_Eval(Interp, "set .name {}; .clearParameters");

  signalSetsChanged((Net) ? Net->unitDisplaySet : 0);
  if (UnitUp) {
    chooseUnitSet();
    drawUnitsLater();
  }
  if (LinkUp) {
    buildLinkGroupMenus();
    drawLinksLater();
  }
  reacquireGraphObjects();
  if (Gui) Tcl_Eval(Interp, ".refreshObjectWindows");

  return TCL_OK;
}

flag signalSetsChanged(int set) {
  eval("set .trainingSet.name \"%s\"; set .testingSet.name \"%s\"",
       (Net && Net->trainingSet) ? Net->trainingSet->name : "",
       (Net && Net->testingSet) ? Net->testingSet->name : "");
  if (UnitUp && (!Net || (set == Net->unitDisplaySet ||
			  Net->unitDisplaySet == NO_SET)))
    chooseUnitSet();
  if (Gui) return configureDisplay();
  return TCL_OK;
}

flag signalNetListChanged(void) {
  if (Gui) {
    if (Tcl_Eval(Interp, ".updateNetworkMenu"))
      return error(Tcl_GetStringResult(Interp));
    return configureDisplay();
  }
  return TCL_OK;
}

flag signalSetListsChanged(void) {
  if (Gui) {
    if (Tcl_Eval(Interp, ".updateExampleSetMenus"))
      return error(Tcl_GetStringResult(Interp));
    return configureDisplay();
  }
  return TCL_OK;
}

flag signalNetStructureChanged(void) {
  if (Net->autoPlot) autoPlot(0);
  else if (UnitUp) drawUnitsLater();
  if (LinkUp) {
    buildLinkGroupMenus();
    drawLinksLater();
  }
  reacquireGraphObjects();
  return result("");
}


/************************************ Tasks **********************************/

void startTask(mask type) {
  Task T = (Task) safeMalloc(sizeof(struct task), "startTask:T");
  T->type = type;
  T->halted = FALSE;
  T->prev = Tasks;
  T->timeStarted = getTime();
  Tasks = T;
  if (Gui) {
    if (configureDisplay()) error("%s", Tcl_GetStringResult(Interp));
    eval(".startTask {%s}", (Tasks) ? lookupTypeName(Tasks->type, TASK) : "");
  }
  eval("if {${.taskDepth} == {}} {set .taskDepth 1} else {incr .taskDepth}");
}

/* This stops the top task of that type */
void stopTask(mask type) {
  Task T;
  char *res;
  if (!Tasks || Tasks->type != type)
    error("attempted to stop task %s but task %s was running",
	  lookupTypeName(type, TASK),
	  lookupTypeName(Tasks->type, TASK));
  else {
    T = Tasks;
    Tasks = Tasks->prev;
    FREE(T);
    res = copyString(Tcl_GetStringResult(Interp));
    if (Gui) {
      if (configureDisplay()) error("%s", Tcl_GetStringResult(Interp));
      eval(".startTask {%s}", (Tasks) ? lookupTypeName(Tasks->type, TASK): "");
    }
    eval("incr .taskDepth -1; if {${.taskDepth} <= 0} {set .taskDepth {}}");
    result(res);
    FREE(res);
  }
}

flag unsafeToRun(const char *command, mask badTasks) {
  Task T;
  for (T = Tasks; T; T = T->prev)
    if (T->type & badTasks)
      return warning("%s: it is unsafe to run this command while %s",
		     command, lookupTypeName(T->type, TASK));
  return TCL_OK;
}

/* This should be the only thing called to update the displays. */
/* Returns TCL_ERROR if the current task should halt */
flag smartUpdate(flag forceUpdate) {
  static unsigned long lastUpdate = 0;
  static flag niced = FALSE;
  unsigned long time = 0;

  if (!Tasks || Tasks->halted) return TCL_ERROR;
  if (!niced) time = getTime();

  if (!Batch || forceUpdate) {
    if (niced) time = getTime();
    if (!forceUpdate && timeElapsed(lastUpdate, time) < UPDATE_INTERVAL)
      return TCL_OK;
    lastUpdate = time;
    /*if (forceUpdate || ParallelState != SERVER)*/ Tcl_Eval(Interp, "update");
    /*else Tcl_Eval(Interp, "update idletasks");*/
  }
#ifndef MACHINE_WINDOWS
  if (!niced) {
    if (getpriority(PRIO_PROCESS, 0) >= AUTO_NICE_VALUE)
      niced = TRUE;
    else if (timeElapsed(Tasks->timeStarted, time) >=
	     60000 * AUTO_NICE_DELAY) {
      setpriority(PRIO_PROCESS, 0, AUTO_NICE_VALUE);
      niced = TRUE;
    }
  }
#endif /* MACHINE_WINDOWS */

  return TCL_OK;
}

/* This stops the top-most unhalted wait */
flag haltWaiting(void) {
  Task T;
  for (T = Tasks; T && !(T->type == WAITING && !T->halted); T = T->prev);
  if (T) {
    T->halted = TRUE;
    if (Gui && T == Tasks) eval(".disableStopButton");
    beep();
  }
  return TCL_OK;
}

/* If there are tasks on the stack, this halts the top one.  Otherwise, this
   does a soft exit. I'm not sure if the pending stuff is really necessary. */
void signalHalt(void) {
  static flag pending = FALSE;

  if (Tasks) {
    Tasks->halted = TRUE;
    print(1, "Halting %s...\n", lookupTypeName(Tasks->type, TASK));
    if (Gui) eval(".disableStopButton");
    beep();
  } else {
    if (pending) return;
    pending = TRUE;
    if (!Batch)
      Tcl_Eval(Interp, ".queryExit");
    else if (queryUser("Are you sure you want to exit Lens?")) {
      printf("exiting");
      Tcl_Exit(0);
    }
    pending = FALSE;
  }
}

void signalHandler(int sig) {
  switch (sig) {
  case SIGABRT:
    error("Got an abort signal, shutting down.");
    Tcl_Exit(1);
    break;
  case SIGFPE:
    error("Whoa there, got a floating point error.\n"
	  "Better tell Doug about this one (dr+@cs.cmu.edu).");
    Tcl_Exit(1);
  case SIGILL:
    error("Uh oh, got an illegal instruction.\n"
	  "Better tell Doug about this one (dr+@cs.cmu.edu).");
    Tcl_Exit(1);
  case SIGINT: /* Ctrl-C does this */
    signalHalt();
    break;
  case SIGSEGV:
    error("Bummer, looks like we got a seg fault.\n"
	  "Better tell Doug about this one (dr+@cs.cmu.edu).");
    Tcl_Exit(1);
  case SIGTERM: /* kill does this */
    error("No, please don't kill me!  Anything but that!  Aaaaggghhhh!!!");
    Tcl_Exit(1);
  case SIGPIPE:
    error("Got a SIGPIPE.  A client or the server probably died.");
    break;
  case SIGUSR1:
    print(1, "\nReceived signal USR1\n");
    if (Net && Net->sigUSR1Proc) {
      if (Tcl_EvalObjEx(Interp, Net->sigUSR1Proc, TCL_EVAL_GLOBAL))
	error(Tcl_GetStringResult(Interp));
      else print(1, "%s", Tcl_GetStringResult(Interp));
    }
    break;
  case SIGUSR2:
    print(1, "\nReceived signal USR2\n");
    if (Net && Net->sigUSR2Proc) {
      if (Tcl_EvalObjEx(Interp, Net->sigUSR2Proc, TCL_EVAL_GLOBAL))
	error(Tcl_GetStringResult(Interp));
      else print(1, "%s", Tcl_GetStringResult(Interp));
    }
    break;
  default:
    error("Got an unrecognized signal: %d", sig);
  }
  signal(sig, signalHandler);
}

/***************************** Initialization ********************************/

void initializeSimulator(void) {
  mask i;

  if (sizeof(int) != 4)
    fatalError("sizeof(int) (%d) is not equal to 4 on this machine",
	       sizeof(int));
  if (sizeof(flag) != 4)
    fatalError("sizeof(flag) (%d) is not equal to 4 on this machine",
	       sizeof(flag));
  if (sizeof(float) != 4)
    fatalError("sizeof(float) (%d) is not equal to 4 on this machine",
	       sizeof(float));
  if (sizeof(double) != 8)
    fatalError("sizeof(double) (%d) is not equal to 8 on this machine",
	       sizeof(double));
  if (sizeof(long) != sizeof(void *))
    fatalError("sizeof(long) (%d) and sizeof(void *) (%d) are not the same "
	       "on this machine", sizeof(long), sizeof(void *));
  /*
    if (sizeof(real) != sizeof(int))
    fatalError("sizeof(real) (%d) and sizeof(int) (%d) are not the same "
    "on this machine", sizeof(real), sizeof(int));
  */

  if (sizeof(struct link) % 4)
    fatalError("sizeof(struct link) (%d) is not divisible by 4 "
	       "on this machine", sizeof(struct link));

#ifndef AVOID_NAN_TEST
  if (!isNaN(NaN) || !isNaNf(NaNf) || !isNaNd(NaNd))
    fatalError("NaN is not NaN on this machine.\nSet the AVOID_NAN_TEST variable to override this check.");
  if (NaNf < 0.0 || NaNf >= 0.0 || NaNd < 0.0 || NaNd >= 0.0)
    fatalError("NaN is not working properly on this machine.\nSet the AVOID_NAN_TEST variable to override this check.");
#endif

  initTypes();
  initLinkTypes();
  registerLinkType(BIAS_NAME, &i);
  buildSigmoidTable();
  timeSeedRand();

  signal(SIGABRT, signalHandler);
  signal(SIGFPE,  signalHandler);
  signal(SIGILL,  signalHandler);
  signal(SIGINT,  signalHandler);
  signal(SIGSEGV, signalHandler);
  signal(SIGTERM, signalHandler);
  signal(SIGUSR1, signalHandler);
  signal(SIGUSR2, signalHandler);
}
