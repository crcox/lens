#include <string.h>
#include "system.h"
#include "util.h"
#include "type.h"
#include "network.h"
#include "train.h"
#include "act.h"
#include "command.h"
#include "control.h"
#include "display.h"
#include "graph.h"

int C_train(TCL_CMDARGS) {
  int numUpdatesInt,arg = 1;
  char *numUpdatesStr;
  flag result, train = TRUE;
  const char *usage = "train [<num-updates>] [-report <report-interval> | -algorithm"
    " <algorithm> | -setOnly]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp("train");

  if (!Net) return warning("%s: no current network", commandName);


  if (objc > 1) {
    numUpdatesStr = Tcl_GetStringFromObj(objv[1], NULL);
    Tcl_GetIntFromObj(interp, objv[1], &numUpdatesInt);
    if (isInteger(numUpdatesStr)) {
      Net->numUpdates = numUpdatesInt;
      arg = 2;
    }
  }

  for (; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    switch (Tcl_GetStringFromObj(objv[arg], NULL)[1]) {
    case 'r':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetIntFromObj(interp, objv[arg], &Net->reportInterval);
      break;
    case 'a':
      if (++arg >= objc) return usageError(commandName, usage);
      if (lookupTypeMask(Tcl_GetStringFromObj(objv[arg], NULL), ALGORITHM, &(Net->algorithm)))
	return warning("%s: unrecognized algorithm: %s", commandName, Tcl_GetStringFromObj(objv[arg], NULL));
      break;
    case 's':
      train = FALSE;
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

  Tcl_Eval(Interp, ".getParameters");
  if (!train) return TCL_OK;
  startTask(TRAINING);
  result = Net->netTrain();
  stopTask(TRAINING);
  return result;
}

int C_updateWeights(TCL_CMDARGS) {
  Algorithm A;
  int arg = 1;
  mask alg;
  flag reset = TRUE, report = FALSE;
  const char *usage = "updateWeights [-algorithm <algorithm> | -noreset | -report]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);

  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h")) return commandHelp(commandName);
  if (!Net) return warning("%s: no current network", commandName);
  alg = Net->algorithm;

  for (; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    switch (Tcl_GetStringFromObj(objv[arg], NULL)[1]) {
    case 'n':
      reset = FALSE;
      break;
    case 'r':
      report = TRUE;
      break;
    case 'a':
      if (++arg >= objc) return usageError(commandName, usage);
      if (lookupTypeMask(Tcl_GetStringFromObj(objv[arg], NULL), ALGORITHM, &alg))
	return warning("%s: unrecognized algorithm: %s", commandName, Tcl_GetStringFromObj(objv[arg], NULL));
      break;
    default: return usageError(commandName, usage);
    }
  }
  A = getAlgorithm(alg);
  A->updateWeights(report);
  RUN_PROC(postUpdateProc);
  if (report) printReport(0, Net->numUpdates, 0);
  if (reset) resetDerivs();
  return TCL_OK;
}

int C_test(TCL_CMDARGS) {
  int arg = 1, numExamplesInt = 0;
  flag retval, resetError = TRUE, print = TRUE;
  char *res, *numExamplesStr;
  const char *usage = "test [<num-examples>] [-noreset | -return]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h")) return commandHelp(commandName);
  if (objc > 3) return usageError(commandName, usage);

  if (!Net) return warning("test: no current network");
  numExamplesStr = Tcl_GetStringFromObj(objv[1], NULL);
  Tcl_GetIntFromObj(interp, objv[1], &numExamplesInt);
  if (objc > 1 && isInteger(numExamplesStr)) {
    if (numExamplesInt < 0)
      return warning("%s: number of examples (%d) can't be negative",
		     commandName, numExamplesInt);
    arg = 2;
  }
  for (; arg < objc; arg++) {
    if (subString(Tcl_GetStringFromObj(objv[arg], NULL), "-noreset", 2)) resetError = FALSE;
    else if (subString(Tcl_GetStringFromObj(objv[arg], NULL), "-return", 2)) print = FALSE;
    else return usageError(commandName, usage);
  }

  startTask(TESTING);
  retval = Net->netTestBatch(numExamplesInt, resetError, print);
  res = copyString(Tcl_GetStringResult(Interp));
  stopTask(TESTING);
  result(res);
  return retval;
}

int C_resetNet(TCL_CMDARGS) {
  const char *usage = "resetNet";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h")) return commandHelp(commandName);
  if (objc != 1) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  if (Net->resetNet(TRUE)) return TCL_ERROR;
  updateUnitDisplay();
  updateLinkDisplay();
  resetGraphs();
  return TCL_OK;
}

int C_resetDerivs(TCL_CMDARGS) {
  const char *usage = "resetDerivs";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h")) return commandHelp(commandName);
  if (objc != 1) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  if (resetDerivs()) return TCL_ERROR;
  return TCL_OK;
}

int C_openNetOutputFile(TCL_CMDARGS) {
  int arg;
  flag binary = FALSE, append = FALSE;
  const char *usage = "openNetOutputFile <file-name> [-binary | -append]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *fileName;
  Tcl_Obj *fileNameObj;
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h")) return commandHelp(commandName);
  if (objc < 2) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  for (arg = 2; arg < objc; arg++) {
    if (Tcl_GetStringFromObj(objv[arg], NULL)[0] != '-') return usageError(commandName, usage);
    switch (Tcl_GetStringFromObj(objv[arg], NULL)[1]) {
    case 'b':
      binary = TRUE;
      break;
    case 'a':
      append = TRUE;
      break;
    default: return usageError(commandName, usage);
    }
  }

  fileNameObj = objv[1];
  fileName = Tcl_GetStringFromObj(objv[1], NULL);
  eval("set _record(path) [.normalizeDir {} [file dirname %s]];"
       "set _record(file) [file tail %s]", fileName, fileName);

  if (openNetOutputFile(fileNameObj, binary, append)) return TCL_ERROR;
  if (Gui) return eval(".trainingControlPanel.r.outp configure -text "
		       "\"Stop Recording\" -command closeNetOutputFile");
  else return TCL_OK;
}

int C_closeNetOutputFile(TCL_CMDARGS) {
  const char *usage = "closeNetOutputFile";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h")) return commandHelp(commandName);
  if (objc != 1)
    return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  if (closeNetOutputFile()) return TCL_ERROR;
  if (Gui) return eval(".trainingControlPanel.r.outp configure -text "
		       "\"Record Outputs\" -command .openOutputFile");
  else return TCL_OK;
}

#ifdef JUNK
int C_testBinary(TCL_CMDARGS) {
  int arg = 1, numExamples = 0;
  flag result, resetError = TRUE;
  char *numExamplesStr;
  const char *usage = "test [<num-examples>] [-noreset]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h")) return commandHelp(commandName);
  if (objc > 3) return usageError(commandName, usage);

  if (!Net) return warning("test: no current network");
  numExamplesStr = Tcl_GetStrFromObj(objv[1], NULL);
  if (objc > 1 && isInteger(numExamplesStr)) {
    Tcl_GetIntFromObj(interp, objv[1], &numExamples)
    if (numExamples < 0)
      return warning("%s: number of examples (%d) can't be negative",
		     commandName, numExamples);
    arg = 2;
  }
  if (objc > arg) {
    if (subString(objv[arg], "-noreset", 2)) resetError = FALSE;
    else return usageError(commandName, usage);
  }

  startTask(TESTING);
  result = Net->netTestBatch(numExamples, resetError);
  stopTask(TESTING);
  return result;
}

int C_testCost(TCL_CMDARGS) {
  Group G;
  Unit U;
  int i, steps = 100;
  real r;
  const char *usage = "testCost <group>";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h")) return commandHelp(commandName);
  if (objc != 2) return usageError(commandName, usage);
  if (!(G = lookupGroup(Tcl_GetStringFromObj(objv[1], NULL))))
    return warning("no such group");
  Net->outputCostStrength = 1.0;
  U = G->unit;

  result("");
  for (i = 0; i <= steps; i++) {
    r = (real) i / steps;
    U->output = r;
    append("%.2g ", r);

    Net->outputCost = 0.0;
    U->outputDeriv = 0.0;
    boundedLinearCost(G);
    boundedLinearCostBack(G);
    append("%f %f  ", Net->outputCost, U->outputDeriv);

    Net->outputCost = 0.0;
    U->outputDeriv = 0.0;
    boundedQuadraticCost(G);
    boundedQuadraticCostBack(G);
    append("%f %f  ", Net->outputCost, U->outputDeriv);

    Net->outputCost = 0.0;
    U->outputDeriv = 0.0;
    convexQuadraticCost(G);
    convexQuadraticCostBack(G);
    append("%f %f  ", Net->outputCost, U->outputDeriv);

    Net->outputCost = 0.0;
    U->outputDeriv = 0.0;
    logisticCost(G);
    logisticCostBack(G);
    append("%f %f  ", Net->outputCost, U->outputDeriv);

    Net->outputCost = 0.0;
    U->outputDeriv = 0.0;
    cosineCost(G);
    cosineCostBack(G);
    append("%f %f\n", Net->outputCost, U->outputDeriv);
  }

  return TCL_OK;
}
#endif /*JUNK*/

void registerTrainingCommands(void) {
  /* createCommand(C_testCost, "testCost"); */

  registerCommand((Tcl_ObjCmdProc *)C_train, "train",
		  "trains the network using a specified algorithm");
  registerSynopsis("steepest", "trains the network using steepest descent");
  registerSynopsis("momentum", "trains the network using momentum descent");
  registerSynopsis("dougsMomentum",
		   "trains the network using normalized momentum descent");
#ifdef ADVANCED
  registerSynopsis("deltaBarDelta",
		   "trains the network using delta-bar-delta");
  /*
  registerSynopsis("quickProp",
		   "trains the network using the quick-prop algorithm");
  */
#endif /* ADVANCED */
  registerCommand((Tcl_ObjCmdProc *)C_updateWeights, "updateWeights",
		  "updates the weights using the current link derivatives");
  registerCommand((Tcl_ObjCmdProc *)C_test, "test", "tests the network on the testing set");
  registerCommand((Tcl_ObjCmdProc *)C_resetNet, "resetNet",
		  "randomizes unfrozen weights and clears direction info.");
  registerCommand((Tcl_ObjCmdProc *)C_resetDerivs, "resetDerivs",
		  "clears the unit and link error derivatives.");
  registerCommand((Tcl_ObjCmdProc *)C_openNetOutputFile, "openNetOutputFile",
		  "begins writing OUTPUT group outputs to a file");
  registerCommand((Tcl_ObjCmdProc *)C_closeNetOutputFile, "closeNetOutputFile",
		  "stops writing OUTPUT group outputs to the output file");
}
