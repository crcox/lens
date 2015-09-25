#include "system.h"
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include "tk.h"
#include "util.h"
#include "type.h"
#include "command.h"
#include "control.h"

int SourceDepth = 0;

flag usageError(const char *command, const char *usage) {
  return warning("%s: usage\n    %s", command, usage);
}

flag commandHelp(const char *command) {
  return eval(".showHelp %s", command);
}

typedef struct ConsoleInfo {
    Tcl_Interp *consoleInterp;	/* Interpreter displaying the console. */
    Tcl_Interp *interp;		/* Interpreter controlled by console. */
    int refCount;
} ConsoleInfo;

/*
 * Each console channel holds an instance of the ChannelData struct as
 * its instance data.  It contains ConsoleInfo, so the channel can work
 * with the appropriate console window, and a type value to distinguish
 * the stdout channel from the stderr channel.
 */

typedef struct ChannelData {
    ConsoleInfo *info;
    int type;			/* TCL_STDOUT or TCL_STDERR */
} ChannelData;

static int C_beep(TCL_CMDARGS) {
  beep();
  return TCL_OK;
}

static int C_seed(TCL_CMDARGS) {
  const char *usage = "seed [<seed-value>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  int seedValue = 0;
  if (objc > 1 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc > 2)
    return usageError(commandName, usage);
  if (objc == 2) {
    Tcl_GetIntFromObj(interp, objv[1], &seedValue);
    seedRand(seedValue);
  } else {
    timeSeedRand();
  }
  return result("%u", getSeed());
}

static int C_getSeed(TCL_CMDARGS) {
  const char *usage = "getSeed";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc > 1 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 1)
    return usageError(commandName, usage);

  return result("%u", getSeed());
}

static int C_rand(TCL_CMDARGS) {
  real min = 0.0, max = 1.0, temp;
  double tempDbl;
  const char *usage = "rand [[<min>] <max>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc > 1 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc > 3)
    return usageError(commandName, usage);
  if (objc == 2) {
    Tcl_GetDoubleFromObj(interp, objv[1], &tempDbl);
    max = (real)tempDbl;
  }
  else if (objc == 3) {
    Tcl_GetDoubleFromObj(interp, objv[1], &tempDbl);
    min = (real)tempDbl;
    Tcl_GetDoubleFromObj(interp, objv[2], &tempDbl);
    max = (real)tempDbl;
  }
  if (min > max) {
    temp = min;
    min = max;
    max = temp;
  }
  return result("%g", randReal((min + max) / 2, (max - min) / 2));
}

static int C_randInt(TCL_CMDARGS) {
  int min = 0, max = 2, temp;
  const char *usage = "randInt [[<min>] <max>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc > 1 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc > 3)
    return usageError(commandName, usage);
  if (objc == 2)
    Tcl_GetIntFromObj(interp, objv[1], &max);
  else if (objc == 3) {
    min = Tcl_GetIntFromObj(interp, objv[1], &min);
    max = Tcl_GetIntFromObj(interp, objv[2], &max);
  }
  if (min > max) {
    temp = min;
    min = max;
    max = temp;
  }
  return result("%d", min + randInt(max - min));
}

#ifndef MACHINE_WINDOWS
static int C_nice(TCL_CMDARGS) {
  int val;
  const char *usage = "nice [<priority-increment>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc > 1 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc > 2) return usageError(commandName, usage);
  if (objc == 2) {
    Tcl_GetIntFromObj(interp, objv[1], &val);
    if (val < 0) return warning("%s: priority increment can't be negative", commandName);
    nice(val);
  }
  return result("%d", getpriority(PRIO_PROCESS, 0));
}
#else
static int C_exit(TCL_CMDARGS) {
  int val = 0;
  if (objc > 1) val = atoi(objv[1]);
  exit(val);
  return TCL_ERROR;
}
#endif /* MACHINE_WINDOWS */


static int C_time(TCL_CMDARGS) {
  real userticks, systicks, realticks;
  struct tms timesstart, timesstop;
  int i, iters = 1;
  const char *usage = "time <command> [-iterations <iterations>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc > 1 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2 && objc != 4) return usageError(commandName, usage);

  if (objc == 4) {
    const char *key = Tcl_GetStringFromObj(objv[2], NULL);
    if (subString(key, "-iterations", 2))
      Tcl_GetIntFromObj(interp, objv[3], &iters);
    else return usageError(commandName, usage);
  }

  realticks = times(&timesstart);

  for (i = 0; i < iters; i++)
    eval(Tcl_GetStringFromObj(objv[1], NULL));
  print(1, "%s\n", Tcl_GetStringResult(Interp));

  realticks = times(&timesstop) - realticks;
  userticks = timesstop.tms_utime - timesstart.tms_utime;
  systicks  = timesstop.tms_stime - timesstart.tms_stime;

  return result("%.3f active  %.3f user  %.3f system  %.3f real",
		(real) (userticks + systicks) / CLOCKS_PER_SEC,
		(real) userticks / CLOCKS_PER_SEC,
		(real) systicks / CLOCKS_PER_SEC,
		(real) realticks / CLOCKS_PER_SEC);
}

static int C_wait(TCL_CMDARGS) {
  unsigned long stopTime = -1;
  const char *usage = "wait [<max-time>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc > 1 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc > 2) return usageError(commandName, usage);

  if (objc == 2) {
    //Tcl_GetLongFromObj(interp, objv[1], &stopTime);
    const char *stopTimeString = Tcl_GetStringFromObj(objv[1], NULL);
    sscanf(stopTimeString, "%lu", &stopTime);
    if (stopTime == 0) return haltWaiting();
  }
  startTask(WAITING);
  if (stopTime != -1) stopTime = getTime() + stopTime * 1e3;

  while ((stopTime == -1 || getTime() < stopTime) && !smartUpdate(TRUE))
    eval("after 50");
  stopTask(WAITING);
  return TCL_OK;
}

static int C_signal(TCL_CMDARGS) {
  int code;
  const char *usage = "signal [<code>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *arg1 = Tcl_GetStringFromObj(objv[1], NULL);
  if (objc > 1 && !strcmp(arg1, "-h"))
    return commandHelp(commandName);
  if (objc > 2) return usageError(commandName, usage);
  if (objc == 1)
    return result("Useful codes are:\n"
		  "INT   interrupts ongoing work or does a soft quit\n"
		  "USR1  runs the sigUSR1Proc\n"
		  "USR2  runs the sigUSR2Proc\n"
		  "QUIT, KILL, ABRT, ALRM, and TERM all kill the process.\n"
		  "Any other code can be produced by specifying the "
		  "integer value.");
  if (!strcmp(arg1, "INT")) code = SIGINT;
  else if (!strcmp(arg1, "USR1")) code = SIGUSR1;
  else if (!strcmp(arg1, "USR2")) code = SIGUSR2;
  else if (!strcmp(arg1, "QUIT")) code = SIGQUIT;
  else if (!strcmp(arg1, "KILL")) code = SIGKILL;
  else if (!strcmp(arg1, "ABRT")) code = SIGABRT;
  else if (!strcmp(arg1, "ALRM")) code = SIGALRM;
  else if (!strcmp(arg1, "TERM")) code = SIGTERM;
  else Tcl_GetIntFromObj(interp, objv[1], &code);
  if (code <= 0)
    return warning("%s: unrecognized code: %s", commandName, arg1);
  raise(code);
  return TCL_OK;
}


/*****************************************************************************/

static int C_source(TCL_CMDARGS) {
  int val = TCL_OK;
  Tcl_Obj *filename;
  Tcl_Obj *message;
  const char *pathToFile;
  char *olddir;
  const char *usage = "source <script file>";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  String script;
  if (objc > 1 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);

  if (objc != 2) return usageError(commandName, usage);

  /* olddir becomes the original directory */
  eval("pwd");
  olddir = copyString(Tcl_GetStringResult(interp));
  pathToFile = Tcl_GetStringFromObj(objv[1], NULL);
  if (eval("cd [file dirname \"%s\"]; file tail \"%s\"", pathToFile, pathToFile)) {
    return TCL_ERROR;
  }
  filename = Tcl_GetObjResult(interp);
  Tcl_IncrRefCount(filename);

  /* actually evaluate the script */
  script = newString(1024);
  if ((val = readFileIntoString(script, filename)) == TCL_OK) {
    SourceDepth++;
    val = Tcl_Eval(interp, script->s);
    SourceDepth--;
  }
  freeString(script);

  /* now message is the return value of the script */
  message = Tcl_GetObjResult(interp);
  Tcl_IncrRefCount(message);

  eval("cd %s\n", olddir);
  free(olddir);

  /* This was commented out because when Tcl sources something it can screw
     up the user's desired path. */
  eval("set .dir [file dirname %s];"
       "if {${.dir} != $tcl_library && ${.dir} != $tk_library} {"
       "set _script(path) [.normalizeDir {} ${.dir}];"
       "set _script(file) [file tail %s]}", objv[1], objv[1]);

  if (val == TCL_ERROR)
    result("in script \"%s\": %s", Tcl_GetStringFromObj(filename, NULL),
	   Tcl_GetStringFromObj(message, NULL));
  else Tcl_SetObjResult(interp, message);

  Tcl_DecrRefCount(filename);
  Tcl_DecrRefCount(message);
  return val;
}

/*****************************************************************************/

/* These use a cached directory name so pwd is faster.
   Hopefully this is not buggy */
char *DirName;

static int C_cd(TCL_CMDARGS) {
  flag result;
  Tcl_DString ds;
  const char *usage = "cd [<directory>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);

  if (objc > 2) return usageError(commandName, usage);
  const char *dirName = (objc == 2) ? Tcl_GetStringFromObj(objv[1], NULL) : "~";
  if (Tcl_TranslateFileName(interp, dirName, &ds) == NULL) {
    return TCL_ERROR;
  }
  result = Tcl_Chdir(Tcl_DStringValue(&ds));
  FREE(DirName);
  Tcl_DStringFree(&ds);
  if (result != 0) {
    Tcl_AppendResult(interp, "couldn't change working directory to \"",
		     dirName, "\": ", Tcl_PosixError(interp), (char *) NULL);
    return TCL_ERROR;
  }
  return TCL_OK;
}

static int C_pwd(TCL_CMDARGS) {
  Tcl_DString ds;
  const char *usage = "pwd";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 1) return usageError(commandName, usage);
  if (DirName) return result(DirName);
  if (Tcl_GetCwd(interp, &ds) == NULL) {
    return TCL_ERROR;
  }
  DirName = copyString(Tcl_DStringValue(&ds));
  Tcl_DStringResult(interp, &ds);
  return TCL_OK;
}

static int C_verbosity(TCL_CMDARGS) {
  char *usage = "verbosity [<level>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc > 1 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc > 2) return usageError(commandName, usage);
  if (objc == 2) Tcl_GetIntFromObj(interp, objv[1], &Verbosity);
  if (Verbosity < 0) Verbosity = 0;
  return result("%d", Verbosity);
}

/*****************************************************************************/

typedef struct TclFile_ *TclFile;
extern int  TclpCloseFile (TclFile file);

#ifdef MACHINE_WINDOWS
typedef struct pipeState {
  struct PipeInfo *nextPtr;   /* Pointer to next registered pipe. */
  Tcl_Channel channel;        /* Pointer to channel structure. */
  int validMask;              /* OR'ed combination of TCL_READABLE,
			       * TCL_WRITABLE, or TCL_EXCEPTION: indicates
			       * which operations are valid on the file. */
  int watchMask;              /* OR'ed combination of TCL_READABLE,
			       * TCL_WRITABLE, or TCL_EXCEPTION: indicates
			       * which events should be reported. */
  int flags;                  /* State flags, see above for a list. */
  TclFile readFile;           /* Output from pipe. */
  TclFile writeFile;          /* Input from pipe. */
  TclFile errorFile;          /* Error output from pipe. */
  int numPids;                /* Number of processes attached to pipe. */
  Tcl_Pid *pidPtr;            /* Pids of attached processes. */
} *PipeState;
#else
typedef struct pipeState {
  Tcl_Channel channel;
  TclFile readFile;           /* Output from pipe. */
  TclFile writeFile;          /* Input to pipe. */
  TclFile errorFile;          /* Error output from pipe. */
  int numPids;                /* Number of processes attached to pipe. */
  Tcl_Pid *pidPtr;            /* Pids of attached processes. */
} *PipeState;
#endif /* MACHINE_WINDOWS */

Tcl_Channel consoleChannel = NULL, errorChannel = NULL;
PipeState   channelState = NULL;
mask        channelMode = 0;

static void closeConsoleExecChannel(Tcl_Channel channel) {
  if (!channel) return;
  if (channel == errorChannel) {
    Tcl_UnregisterChannel(Interp, channel);
    errorChannel = NULL;
  }
  if (channel == consoleChannel) {
    Tcl_Close(Interp, channel);
    consoleChannel = NULL;
    channelState = NULL;
    channelMode = 0;
    eval("consoleinterp eval stopExecFile");
  }
}

static void closeConsoleExec(Tcl_Channel channel, mask mode) {
  if (channel == consoleChannel) {
    channelMode &= ~mode;
    if (!channelMode) closeConsoleExecChannel(channel);
  } else closeConsoleExecChannel(channel);
}

static void handleConsoleExecReadable(ClientData data, int event) {
  Tcl_Channel channel = (Tcl_Channel) data;
  int bytes = 0;
  while ((bytes = Tcl_Read(channel, Buffer, BUFFER_SIZE - 1)) > 0) {
    Buffer[bytes] = '\0';
    Tcl_Write(channel, Buffer, bytes);
  }
  if (bytes == -1) {
    error("An error occurred on an exec channel");
    closeConsoleExec(channel, TCL_READABLE);
    return;
  }
  if (bytes == 0) {
    if (Tcl_Eof(channel)) {
      closeConsoleExec(channel, TCL_READABLE);
      return;
    }
  }
}

static void handleConsoleExecException(ClientData data, int event) {
  Tcl_Channel channel = (Tcl_Channel) data;
  closeConsoleExec(channel, TCL_WRITABLE);
}

static int C_consoleExec(TCL_CMDARGS) {
  static char *errorChannelName = NULL;
  int i;
  CONST84 char **channelArgs = (CONST84 char **) malloc(sizeof(char *) * objc-1);
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);

  if (!errorChannel) {
    FREE(errorChannelName);
    if (eval("open |cat RDWR"))
      return warning("%s: failed to open error channel", commandName);
    errorChannelName = copyString(Tcl_GetStringResult(interp));

    if (!(errorChannel = Tcl_GetChannel(interp, errorChannelName, NULL)))
      return warning("%s: no error channel registered", commandName);

    if (Tcl_SetChannelOption(interp, errorChannel, "-blocking", "0"))
      return TCL_ERROR;
    if (Tcl_SetChannelOption(interp, errorChannel, "-buffering", "none"))
      return TCL_ERROR;

    Tcl_CreateChannelHandler(errorChannel, TCL_READABLE,
			     handleConsoleExecReadable,
			     (ClientData) errorChannel);
  }

  for (i=0; i<(objc-1); i++) {
    channelArgs[i] = Tcl_GetStringFromObj(objv[i+1], NULL);
  }
  channelArgs[objc-1] = errorChannelName;
  if (!(consoleChannel = Tcl_OpenCommandChannel(interp, objc-1, channelArgs,
						TCL_STDIN | TCL_STDOUT)))
    return TCL_ERROR;
  channelArgs[objc-1] = Tcl_GetStringFromObj(objv[objc-1], NULL);

  channelMode = Tcl_GetChannelMode(consoleChannel);

  if (channelMode & (TCL_READABLE | TCL_WRITABLE)) {
    if (channelMode & TCL_WRITABLE) {
      if (Tcl_SetChannelOption(interp, consoleChannel, "-buffering", "line"))
        return TCL_ERROR;
    }
    if (channelMode & TCL_READABLE) {
      Tcl_DString option;
      Tcl_DStringInit(&option);
      if (Tcl_SetChannelOption(interp, consoleChannel, "-blocking", "0"))
        return TCL_ERROR;
      Tcl_GetChannelOption(interp, consoleChannel, NULL, &option);
      Tcl_CreateChannelHandler(consoleChannel, TCL_READABLE,
			       handleConsoleExecReadable, (ClientData)
			       consoleChannel);
    }
    Tcl_CreateChannelHandler(consoleChannel, TCL_EXCEPTION,
			     handleConsoleExecException, (ClientData)
			     consoleChannel);
    channelState = (PipeState) Tcl_GetChannelInstanceData(consoleChannel);
    eval("consoleinterp eval startExecFile");
    eval("after 100 .consoleExecCleanup");
  } else closeConsoleExecChannel(consoleChannel);
  free(channelArgs);
  return result("");
}

static int C_consoleExecWrite(TCL_CMDARGS) {
  int bytes = 0;
  const char *s = Tcl_GetStringFromObj(objv[1], &bytes);
  if (!consoleChannel) return TCL_OK;
  if (Tcl_Write(consoleChannel, s, bytes) != bytes) {
    closeConsoleExec(consoleChannel, TCL_WRITABLE);
    return warning("write failed on console exec channel");
  }
  return TCL_OK;
}

static int C_consoleExecClose(TCL_CMDARGS) {
  if (!consoleChannel) return TCL_OK;
  Tcl_Flush(consoleChannel);
  if (channelState->writeFile)
    TclpCloseFile(channelState->writeFile);
  channelState->writeFile = NULL;
  closeConsoleExec(consoleChannel, TCL_WRITABLE);
  return TCL_OK;
}

static int C_consoleExecCleanup(TCL_CMDARGS) {
  int i, status;
  Tcl_Pid pid;
  if (!consoleChannel) return TCL_OK;
  for (i = 0; i < channelState->numPids; i++) {
    pid = Tcl_WaitPid(channelState->pidPtr[i], &status, WNOHANG);
    if (!consoleChannel) return TCL_OK;
    if (pid == 0) break;
  }
  if (i == channelState->numPids) {
    closeConsoleExecChannel(consoleChannel);
    return TCL_OK;
  }
  eval("after 100 .consoleExecCleanup");
  return TCL_OK;
}


/********************************** More *************************************/

/* The isprint junk here is to force it to ignore formatting codes that appear
   in the .txt files. */
static char *printLine(char *s, int columns) {
  char *t, n = '\n', *x = Buffer;
  Tcl_Channel channel;
  for (t = s; *t && *t != '\n' && (x-Buffer) < columns; t++) {
    if (isprint(*t)) *(x++) = *t;
    else if (x) x--;
  }
  channel = Tcl_GetStdChannel(TCL_STDOUT);
  Tcl_WriteChars(channel, Buffer, x - Buffer);
  Tcl_WriteChars(channel, &n, 1);
  if (*t && *(t+1)) return t+1;
  else return NULL;
}

static int C_more(TCL_CMDARGS) {
  int i, rows, columns, nameLength = 0;
  String S;
  Tcl_Channel channel;
  char *s;
  char code = ' ';
  const char *usage = "more (<filename> | << <string>)";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  Tcl_Obj *fileNameObj = objv[1];
  const char *fileName = Tcl_GetStringFromObj(objv[1], &nameLength);

  if (objc > 3 || objc == 1) return usageError(commandName, usage);
  if (objc == 2) {
    if (nameLength > 256) return usageError(commandName, usage);
    S = newString(1024);
    if (readFileIntoString(S, fileNameObj))
      return warning("%s: couldn't open file %s", commandName, fileName);
    s = S->s;
  } else {
    if (strcmp(fileName, "<<"))
      return usageError(commandName, usage);
    s = Tcl_GetStringFromObj(objv[2], NULL);
  }
  eval("lindex [split [consoleinterp eval {wm geometry .}] {x+}] 0");
  Tcl_GetIntFromObj(interp, Tcl_GetObjResult(interp), &columns);
  eval("lindex [split [consoleinterp eval {wm geometry .}] {x+}] 1");
  Tcl_GetIntFromObj(interp, Tcl_GetObjResult(interp), &rows);

  eval("consoleinterp eval startMore");

  while (s && code != 'q') {
    switch (code) {
    case ' ':
      for (i = 0; i < rows - 1 && s; i++) {
        s = printLine(s, columns);
      }
      break;
    case '-':
      s = printLine(s, columns);
      break;
    }

    if (s) {
      channel = Tcl_GetStdChannel(TCL_STDERR);
      Tcl_WriteChars(channel, "--More--", 8);
      eval("set .moreCode {}");
      eval("vwait .moreCode");
      eval("set .moreCode");
      code = Tcl_GetStringResult(interp)[0];
      eval("consoleinterp eval removeMoreMessage");
    }
  }
  freeString(S);

  eval("consoleinterp eval stopMore");
  return TCL_OK;
}

/*****************************************************************************/

//extern void InitConsoleChannels(Tcl_Interp *interp);
//extern flag CreateConsoleWindow(Tcl_Interp *interp);

flag createConsole(flag firstTime) {
  Tk_InitConsoleChannels(Interp);
  if (Tk_CreateConsoleWindow(Interp))
    return warning("viewConsole: Tk_CreateConsoleWindow failed: %s",
		   Tcl_GetStringResult(Interp));
  if (firstTime) {
    createCommand((Tcl_ObjCmdProc *)C_more, "more");
    createCommand((Tcl_ObjCmdProc *)C_more, "less");
    createCommand((Tcl_ObjCmdProc *)C_consoleExec,        ".consoleExec");
    createCommand((Tcl_ObjCmdProc *)C_consoleExecWrite,   ".consoleExecWrite");
    createCommand((Tcl_ObjCmdProc *)C_consoleExecClose,   ".consoleExecClose");
    createCommand((Tcl_ObjCmdProc *)C_consoleExecCleanup, ".consoleExecCleanup");
  }
  Console = TRUE;
  return TCL_OK;
}

/*****************************************************************************/

static int C_signalHalt(TCL_CMDARGS) {
  signalHalt();
  return TCL_OK;
}

static int C_configureDisplay(TCL_CMDARGS) {
  return configureDisplay();
}

/*****************************************************************************/

static void registerShellCommands(void) {
  registerSynopsis("help",     "prints this list or explains a command");
  registerSynopsis("manual",   "opens the Lens manual in a web browser");
  registerSynopsis("set",      "creates and sets the value of a variable");
  registerSynopsis("unset",    "removes variables");
  registerSynopsis("alias",    "creates or prints a command alias");
  registerSynopsis("unalias",  "deletes a command alias");
  registerSynopsis("index",    "creates a .tclIndex file storing the locations of commands");
  registerSynopsis("history",  "manipulates the command history list");
  registerSynopsis("exec",     "runs a subprocess");
  registerSynopsis("expr",     "performs a mathematical calculation");
  registerSynopsis("open",     "opens a Tcl channel");
  registerSynopsis("close",    "closes a Tcl channel");
  registerSynopsis("puts",     "prints a string to standard output or a Tcl channel");
  registerSynopsis("ls",       "lists the contents of a directory");
  registerCommand((Tcl_ObjCmdProc *)C_cd, "cd",  "changes the current directory");
  registerCommand((Tcl_ObjCmdProc *)C_pwd, "pwd","returns the current directory");
  createCommand((Tcl_ObjCmdProc *)C_beep, "beep");
  registerCommand((Tcl_ObjCmdProc *)C_seed, "seed",
		  "seeds the random number generator");
  registerCommand((Tcl_ObjCmdProc *)C_getSeed, "getSeed",
		  "returns the last seed used on the random number generator");
  registerCommand((Tcl_ObjCmdProc *)C_rand, "rand",
		  "returns a random real number in a given range");
  registerCommand((Tcl_ObjCmdProc *)C_randInt, "randInt",
		  "returns a random integer in a given range");
  registerSynopsis("pid",      "returns the current process id");
#ifndef MACHINE_WINDOWS
  registerCommand((Tcl_ObjCmdProc *)C_nice, "nice",
		  "increments or returns the process's priority");
#endif /* MACHINE_WINDOWS */
  registerCommand((Tcl_ObjCmdProc *)C_time, "time",
		  "runs a command and returns the time elapsed");
  registerSynopsis("glob",     "performs file name lookup");
  registerCommand((Tcl_ObjCmdProc *)C_signal, "signal",
		  "raises a signal in the current process");
  registerCommand((Tcl_ObjCmdProc *)NULL, "", "");

  registerSynopsis("source",   "runs a script file");
  registerSynopsis("eval",     "performs an evaluation pass on a command");
  registerSynopsis("if",       "conditional test");
  registerSynopsis("for",      "for loop");
  registerSynopsis("while",    "while loop");
  registerSynopsis("foreach",  "loops over the elements in a list");
  registerSynopsis("repeat",   "loops a specified number of times");
  registerSynopsis("switch",   "switch statement");
  registerSynopsis("return",   "returns from the current procedure or script");
  registerCommand((Tcl_ObjCmdProc *)C_wait, "wait",
		  "stops the interactive shell for batch jobs");
  registerCommand((Tcl_ObjCmdProc *)C_verbosity, "verbosity",
		  "check or set how verbose Lens is (mostly during training)");
#ifdef MACHINE_WINDOWS
  if (Batch) createCommand(C_exit, "exit");
#endif
  registerSynopsis("exit",     "exits from the simulator");
  createCommand((Tcl_ObjCmdProc *)C_signalHalt, ".signalHalt");
  createCommand((Tcl_ObjCmdProc *)C_configureDisplay, ".configureDisplay");
}

/*****************************************************************************/
void registerCommands(void) {
  registerNetworkCommands();
  registerCommand((Tcl_ObjCmdProc *)NULL, "", "");

  registerConnectCommands();
  registerCommand((Tcl_ObjCmdProc *)NULL, "", "");

  registerExampleCommands();
  registerCommand((Tcl_ObjCmdProc *)NULL, "", "");

  registerTrainingCommands();
  registerCommand((Tcl_ObjCmdProc *)NULL, "", "");

  registerObjectCommands();
  registerCommand((Tcl_ObjCmdProc *)NULL, "", "");

  registerViewCommands();
  registerCommand((Tcl_ObjCmdProc *)NULL, "", "");

  registerDisplayCommands();
  registerCommand((Tcl_ObjCmdProc *)NULL, "", "");

  registerGraphCommands();
  registerCommand((Tcl_ObjCmdProc *)NULL, "", "");

  registerParallelCommands();
  registerCommand((Tcl_ObjCmdProc *)NULL, "", "");

  registerShellCommands();
  createCommand((Tcl_ObjCmdProc *)C_source, "source");

  Tcl_LinkVar(Interp, ".sourceDepth", (char *) &SourceDepth,
	      TCL_LINK_INT);
}
