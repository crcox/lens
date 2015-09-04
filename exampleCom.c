#include <string.h>
#include "system.h"
#include "util.h"
#include "type.h"
#include "example.h"
#include "network.h"
#include "command.h"
#include "control.h"
#include "display.h"
#include "act.h"
#include "defaults.h"

int C_loadExamples(TCL_CMDARGS) {
  ExampleSet S;
  char *setName = NULL;
  int arg, mode = 0, numExamples = 0;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  flag code, madeName = FALSE;
  mask exmode = DEF_S_mode;
  const char *usage = "loadExamples <file-name> [-set <example-set> | -num "
    "<num-examples> |\n\t-exmode <example-mode> | -mode <load-mode>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc < 2) return usageError(commandName, usage);
  if (unsafeToRun(commandName, LOADING_EXAMPLES | SAVING_EXAMPLES))
    return TCL_ERROR;

  for (arg = 2; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    switch (Tcl_GetStringFromObj(objv[arg], NULL)[1]) {
    case 's':
      if (++arg >= objc) return usageError(commandName, usage);
      if (!Tcl_GetStringFromObj(objv[arg], NULL)[0])
	return warning("%s: example set must have a real name", commandName);
      else setName = Tcl_GetStringFromObj(objv[arg], NULL);
      break;
    case 'n':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetIntFromObj(interp, objv[arg], &numExamples);
      break;
    case 'e':
      if (++arg >= objc) return usageError(commandName, usage);
      if (lookupTypeMask(Tcl_GetStringFromObj(objv[arg], NULL), EXAMPLE_MODE, &exmode)) {
        warning("%s: invalid example mode: \"%s\"\n"
          "Try one of the following:\n", commandName, Tcl_GetStringFromObj(objv[arg], NULL));
        printTypes("Example Modes", EXAMPLE_MODE, ~0);
	return TCL_ERROR;
      }
      break;
    case 'm':
      if (++arg >= objc) return usageError(commandName, usage);
      switch (Tcl_GetStringFromObj(objv[arg], NULL)[0]) {
      case 'R': mode = 1; break;
      case 'A': mode = 2; break;
      case 'P': mode = 3; exmode = PIPE; break;
      default:
        return warning("%s: invalid mode name: %s.  Try REPLACE, APPEND, or "
                 "PIPE.", commandName, Tcl_GetStringFromObj(objv[arg], NULL));
            }
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

  if (!setName) {
    if (eval(".exampleSetRoot {%s}", Tcl_GetStringFromObj(objv[1], NULL)))
      return TCL_ERROR;
    setName = copyString(Tcl_GetStringResult(Interp));
    madeName = TRUE;
  }

  startTask(LOADING_EXAMPLES);
  eval(".setPath _examples %s", Tcl_GetStringFromObj(objv[1], NULL));
  code = loadExamples(setName, objv[1], mode, numExamples);
  if ((S = lookupExampleSet(setName)) && S->mode != exmode)
    exampleSetMode(S, exmode);
  if (code == TCL_OK) result(setName);
  if (madeName) FREE(setName);
  stopTask(LOADING_EXAMPLES);
  return code;
}

int C_useTrainingSet(TCL_CMDARGS) {
  int s;
  ExampleSet S;

  const char *usage = "useTrainingSet [<example-set>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc > 2)
    return usageError(commandName, usage);

  /* If no arguments, return a list of the training sets */
  if (objc == 1) {
    Tcl_ResetResult(interp);
    for (s = 0; s < Root->numExampleSets; s++) {
      S = Root->set[s];
      /* if (S->checkCompatibility(S) == TCL_OK) */
      append("\"%s\" ", S->name);
    }
    return TCL_OK;
  }

  if (!Net) return warning("%s: no current network", commandName);
  if (unsafeToRun(commandName, NETWORK_TASKS)) return TCL_ERROR;
  if (!Tcl_GetStringFromObj(objv[1], NULL)[0]) S = NULL;
  else {   /* Otherwise, lookup the specified set and use it */
    if (!(S = lookupExampleSet(Tcl_GetStringFromObj(objv[1], NULL))))
      return warning("%s: example set \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[1], NULL));
    /*
    if (S->checkCompatibility(S))
      return warning("%s: set \"%s\" isn't compatible with the "
		     "current network", commandName, objv[1]);
		     */
  }
  if (S == Net->trainingSet) return TCL_OK;
  return useTrainingSet(S);
}

int C_useTestingSet(TCL_CMDARGS) {
  int s;
  ExampleSet S;

  const char *usage = "useTestingSet [<example-set>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc > 2)
    return usageError(commandName, usage);

  /* If no arguments, return a list of the networks */
  if (objc == 1) {
    Tcl_ResetResult(interp);
    for (s = 0; s < Root->numExampleSets; s++) {
      S = Root->set[s];
      /* if (S->checkCompatibility(S) == TCL_OK) */
      append("\"%s\" ", S->name);
    }
    return TCL_OK;
  }
  if (!Net) return warning("%s: no current network", commandName);
  if (unsafeToRun(commandName, NETWORK_TASKS)) return TCL_ERROR;
  if (!Tcl_GetStringFromObj(objv[1], NULL)[0]) S = NULL;
  else {   /* Otherwise, lookup the specified set and use it */
    if (!(S = lookupExampleSet(Tcl_GetStringFromObj(objv[1], NULL))))
      return warning("%s: example set \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[1], NULL));
    /*
    if (S->checkCompatibility(S))
      return warning("%s: set \"%s\" isn't compatible with the "
		     "current network", commandName, objv[1]);
		     */
  }
  if (S == Net->testingSet) return TCL_OK;
  return useTestingSet(S);
}

int C_deleteExampleSets(TCL_CMDARGS) {
  const char *usage = "deleteExampleSets <example-set-list>";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);

  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2) return usageError(commandName, usage);
  if (unsafeToRun(commandName, NETWORK_TASKS)) return TCL_ERROR;

  if (!strcmp(Tcl_GetStringFromObj(objv[1], NULL), "*")) deleteAllExampleSets();
  else FOR_EACH_SET_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), deleteExampleSet(S));

  signalSetsChanged(TRAIN_SET);
  signalSetsChanged(TEST_SET);
  signalSetListsChanged();
  return TCL_OK;
}

int C_moveExamples(TCL_CMDARGS) {
  int first = 0, num = 0;
  real proportion = 1.0;
  flag copy = FALSE;
  ExampleSet S;
  const char *usage = "moveExamples <example-set1> <example-set2>\n"
    "\t([<first-example> [<num-examples>]] | <proportion> | ) [-copy]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc < 3 || objc > 6)
    return usageError(commandName, usage);
  if (!Tcl_GetStringFromObj(objv[2], NULL)[0])
    return warning("%s: example set must have a real name", commandName);

  if (!(S = lookupExampleSet(Tcl_GetStringFromObj(objv[1], NULL))))
    return warning("%s: example set \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[1], NULL));
  if (Tcl_GetStringFromObj(objv[objc-1], NULL)[0] == '-' &&
      Tcl_GetStringFromObj(objv[objc-1], NULL)[1] == 'c') {
    copy = TRUE;
    objc--;
  }
  /* if (objc == 3) return moveAllExamples(S, objv[2]); */
  if (objc > 3) {
    Tcl_GetIntFromObj(interp, objv[3], &first);
    if (objc == 5) {
      Tcl_GetIntFromObj(interp, objv[4], &num);
    } else if (objc == 4) {
      if ((first == 0 && strcmp(Tcl_GetStringFromObj(objv[3], NULL), "0")) ||
        (first == 1 && strcmp(Tcl_GetStringFromObj(objv[3], NULL), "1"))) {
        Tcl_GetDoubleFromObj(interp, objv[3], &proportion);
      } else num = 1;
    }
  }
  return moveExamples(S, Tcl_GetStringFromObj(objv[2], NULL), first, num, proportion, copy);
}

int C_saveExamples(TCL_CMDARGS) {
  int arg;
  ExampleSet S;
  flag code, binary = FALSE, append = FALSE;
  Tcl_Obj *fileNameObj;
  const char *fileName;
  const char *usage = "saveExamples <example-set> <file-name> [-binary | -append]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc < 3) return usageError(commandName, usage);
  if (unsafeToRun(commandName, LOADING_EXAMPLES | SAVING_EXAMPLES))
    return TCL_ERROR;
  if (!(S = lookupExampleSet(Tcl_GetStringFromObj(objv[1], NULL))))
    return warning("%s: example set \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[1], NULL));

  for (arg = 3; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    switch (Tcl_GetStringFromObj(objv[arg], NULL)[1]) {
    case 'b': binary = TRUE; break;
    case 'a': append = TRUE; break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

  if (binary && append)
    return warning("saveExamples: you cannot use both the -binary and -append flags");

  startTask(SAVING_EXAMPLES);
  fileNameObj = objv[2];
  fileName = Tcl_GetStringFromObj(objv[2], NULL);
  if (binary)
    code = writeBinaryExampleFile(S, fileNameObj);
  else
    code = writeExampleFile(S, fileNameObj, append);
  stopTask(SAVING_EXAMPLES);
  if (code) return TCL_ERROR;
  return result(fileName);
}

int C_exampleSetMode(TCL_CMDARGS) {
  mask mode;
  char *modeName;
  const char *usage = "exampleSetMode <example-set-list> [<mode>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2 && objc != 3)
    return usageError(commandName, usage);

  if (objc == 3) {
    if (lookupTypeMask(Tcl_GetStringFromObj(objv[2], NULL), EXAMPLE_MODE, &mode)) {
      warning("%s: invalid example mode: \"%s\"\n"
	      "Try one of the following:\n", commandName, objv[2]);
      printTypes("Example Modes", EXAMPLE_MODE, ~0);
      return TCL_ERROR;
    }
  }
  result("");
  FOR_EACH_SET_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), {
    if (objc == 2) {
      if (!(modeName = (char *) lookupTypeName(S->mode, EXAMPLE_MODE)))
        return warning("%s: example set \"%s\" has an invalid mode: %d\n",
                 commandName, S->name, S->mode);
      append("{%s} %s\n", S->name, modeName);
    } else if (exampleSetMode(S, mode)) return TCL_ERROR;
  });
  return TCL_OK;
}

int C_resetExampleSets(TCL_CMDARGS) {
  const char *usage = "resetExampleSets <example-set-list>";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2)
    return usageError(commandName, usage);

  FOR_EACH_SET_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), {
    resetExampleSet(S);
    if (Net && UnitUp &&
	(((Net->trainingSet == S) && Net->unitDisplaySet == TRAIN_SET) ||
	 ((Net->testingSet == S) && Net->unitDisplaySet == TEST_SET)))
      updateUnitDisplay();
  });
  return TCL_OK;
}

int C_doExample(TCL_CMDARGS) {
  ExampleSet S = NULL;
  int arg = 1, num = -1;
  char *numStr;
  char testing = 0;
  const char *usage = "doExample [<example-num>] [-set <example-set> | "
    "-train | -test]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (!Net) return warning("%s: no current network", commandName);

  numStr = Tcl_GetStringFromObj(objv[1], NULL);
  if (objc >= 2 && isInteger(numStr)) {
    Tcl_GetIntFromObj(interp, objv[1], &num);
    arg = 2;
  }

  for (; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    switch (Tcl_GetStringFromObj(objv[arg], NULL)[1]) {
    case 's':
      if (++arg >= objc) return usageError(commandName, usage);
      if (!(S = lookupExampleSet(Tcl_GetStringFromObj(objv[arg], NULL))))
	return warning("%s: example set \"%s\" doesn't exist",
		       commandName, Tcl_GetStringFromObj(objv[arg], NULL));
      break;
    case 't':
      if (subString(Tcl_GetStringFromObj(objv[arg], NULL)+1, "train", 2)) {
	Net->netRunExample = Net->netTrainExample;
	Net->netRunTick = Net->netTrainTick;
      } else if (subString(Tcl_GetStringFromObj(objv[arg], NULL)+1, "test", 2)) {
        Net->netRunExample = Net->netTestExample;
        Net->netRunTick = Net->netTestTick;
        testing = 1;
      } else return usageError(commandName, usage);
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

  if (!S) {
    if (Net->trainingSet && (!Net->testingSet || !testing))
      S = Net->trainingSet;
    else if (Net->testingSet) S = Net->testingSet;
    else return warning("%s: there are no current example sets", commandName);
  }
  if (Net->netRunExampleNum(num, S)) return TCL_ERROR;
  updateDisplays(ON_EXAMPLE);
  return TCL_OK;
}

void registerExampleCommands(void) {
  registerCommand((Tcl_ObjCmdProc *)C_loadExamples, "loadExamples",
		  "reads an example file into an example set");
  registerCommand((Tcl_ObjCmdProc *)C_useTrainingSet, "useTrainingSet",
		  "makes an example set the current training set");
  registerCommand((Tcl_ObjCmdProc *)C_useTestingSet, "useTestingSet",
		  "makes an example set the current testing set");
  /*
    registerCommand((Tcl_ObjCmdProc *)C_listExampleSets, "listExampleSets",
    "returns a list of all example sets");
  */
  registerCommand((Tcl_ObjCmdProc *)C_deleteExampleSets, "deleteExampleSets",
		  "deletes a list of example sets");
  registerCommand((Tcl_ObjCmdProc *)C_moveExamples, "moveExamples",
		  "moves examples from one set to another");
  /*
  registerCommand((Tcl_ObjCmdProc *)C_moveAllExamples, "moveAllExamples",
		  "moves all examples from one set to another");
  */
  registerCommand((Tcl_ObjCmdProc *)C_saveExamples, "saveExamples",
		  "writes an example set to an example file");
  registerCommand((Tcl_ObjCmdProc *)C_exampleSetMode, "exampleSetMode",
		  "sets or returns the example set's example selection mode");
  registerCommand((Tcl_ObjCmdProc *)C_resetExampleSets, "resetExampleSets",
		  "returns the example set to the first example");
  registerCommand((Tcl_ObjCmdProc *)C_doExample, "doExample",
		  "runs the network on the next or a specified example");
}
