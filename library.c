#include <stdio.h>
#include <unistd.h>
#include "system.h"
#include "util.h"
#include "type.h"
#include "extension.h"
#include "command.h"
#include "control.h"
#include "canvRect.h"
#include "train.h"
#include "lens.h"
#include <tk.h>
#ifdef MACHINE_WINDOWS
#  include <windows.h>
#endif /* MACHINE_WINDOWS */

#define VERSION "2.63"

char Version[32];

int initError(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  beep();
  fprintf(stderr, "Error: ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
  return TCL_ERROR;
}

//int Tcl_AppInit(Tcl_Interp *interp) {
//
//  if (Tcl_Init(interp) == TCL_ERROR)
//    return initError("Tcl_Init failed:\n%s\n"
//	     "Have you set the LENSDIR environment variable properly?\n"
//	     "If not, see Running Lens in the online manual.",
//	     Tcl_GetStringResult(interp));
//
//  if (!Batch) {
//    if (Tk_Init(interp) == TCL_ERROR)
//      return initError("Tk_Init failed: %s", Tcl_GetStringResult(interp));
//  }
//
//  Tcl_LinkVar(interp, ".GUI", (char *) &Gui, TCL_LINK_INT);
//  Tcl_LinkVar(interp, ".BATCH", (char *) &Batch, TCL_LINK_INT);
//  Tcl_LinkVar(interp, ".CONSOLE", (char *) &Console, TCL_LINK_INT);
//#ifdef MACHINE_WINDOWS
//  eval("set .WINDOWS 1");
//#else
//  eval("set .WINDOWS 0");
//#endif /* MACHINE_WINDOWS */
//
//  Tcl_SetVar(interp, ".RootDir", RootDir, TCL_GLOBAL_ONLY);
//
//#ifdef ADVANCED
//  Tcl_Eval(interp, "set .ADVANCED 1");
//  sprintf(Version, "%s%sa", VERSION, VersionExt);
//#else
//  Tcl_Eval(interp, "set .ADVANCED 0");
//  sprintf(Version, "%s%s", VERSION, VersionExt);
//#endif /* ADVANCED */
//
//#ifdef DOUBLE_REAL
//  strcat(Version, " dbl");
//#endif
//
//  Tcl_SetVar(interp, ".Version", Version, TCL_GLOBAL_ONLY);
//
//  sprintf(Buffer, "%s/Src/shell.tcl", RootDir);
//  if (Tcl_EvalFile(interp, Buffer) != TCL_OK)
//    return initError("%s\nwhile reading %s/Src/shell.tcl",
//		 Tcl_GetStringResult(interp), RootDir);
//
//  registerCommands();
//  registerAlgorithms();
//  createObjects();
//  if (!Batch) createCanvRectType();
//  if (userInit()) return initError("user initialization failed");
//  initObjects();
//
//  if (!Batch) {
//    if (Tcl_Eval(Interp, "wm withdraw ."))
//      return initError("error withdrawing main window");
//  }
//
//  if (Console) createConsole(TRUE);
//
//  eval("set _script(path) [pwd]; set _script(file) {}");
//
//  Tcl_Eval(Interp, "update");
//
//  return TCL_OK;
//}

/* Returns 1 on failure, 0 on success. */
flag startLens(char *progName, flag batchMode) {
  int tty;
  Gui = FALSE;
  Batch = batchMode;

  if (!(RootDir = getenv("LENSDIR")))
    return initError("The LENSDIR environment variable was not set properly.\n"
		     "See Running Lens in the online manual.");
#ifdef MACHINE_WINDOWS
  if (!Batch) Console = TRUE;
  sprintf(Buffer, "%s/TclTk/tcl8.3", RootDir);
  SetEnvironmentVariableA("TCL_LIBRARY", Buffer);
#else
  sprintf(Buffer, "TCL_LIBRARY=%s/TclTk/tcl%s/library", RootDir, TCL_VERSION);
  if (putenv(copyString(Buffer)))
    return initError("Lens couldn't set %s", Buffer);
  sprintf(Buffer, "TK_LIBRARY=%s/TclTk/tk%s/library", RootDir, TCL_VERSION);
  if (putenv(copyString(Buffer)))
    return initError("Lens couldn't set %s", Buffer);
#endif /* MACHINE_WINDOWS */

  initializeSimulator();

  Tcl_FindExecutable(progName);
  if (!(Interp = Tcl_CreateInterp()))
    return initError("Lens couldn't create the interpreter");

  /* Set the "tcl_interactive" variable. */
  tty = isatty(0);
  Tcl_SetVar(Interp, "tcl_interactive",
	     (tty) ? "1" : "0", TCL_GLOBAL_ONLY);

  if (Tcl_AppInit(Interp) != TCL_OK)
    return initError("Lens initialization failed");

  return 0;   /* Needed only to prevent compiler warning. */
}

flag lens(char *fmt, ...) {
  char *command;
  flag result;
  va_list args;
  if (fmt == Buffer) {
    error("lens() called on Buffer, report this to Doug");
    return TCL_ERROR;
  }
  va_start(args, fmt);
  vsprintf(Buffer, fmt, args);
  command = copyString(Buffer);
  result = Tcl_EvalEx(Interp, command, -1, TCL_EVAL_GLOBAL);
  va_end(args);
  FREE(command);
  if (result)
    fprintf(stderr, "Lens Error: %s\a\n", Tcl_GetStringResult(Interp));
  return result;
}
