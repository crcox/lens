#include <string.h>
#include "system.h"
#include "util.h"
#include "type.h"
#include "network.h"
#include "connect.h"
#include "act.h"
#include "command.h"
#include "control.h"
#include "display.h"
#include "graph.h"
#include "tk.h"

int C_resetPlot(TCL_CMDARGS) {
  int width;
  const char *usage = "resetPlot [<plot-columns>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (Batch) return result("%s is not available in batch mode", commandName);
  if (objc > 2)
    return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  if (objc == 2) {
    Tcl_GetIntFromObj(interp, objv[1], &width);
    if (width < 0)
      return warning("%s: plot width (%d) cannot be negative", commandName, width);
    Net->plotCols = width;
  }
  FOR_EACH_GROUP(FOR_EVERY_UNIT(G, U->plotCol = U->plotRow = 0));
  Net->plotRows = 0;
  Net->autoPlot = 0;
  return TCL_OK;
}

int C_plotRow(TCL_CMDARGS) {
  int numRows, unitsPlotted, i, arg;
  const char *numRowsStr;
  const char *usage = "plotRow [<numRows>] [next <group> <n> | lnext <group> <n> |\n"
    "\tcnext <group> <n> | rnext <group> <n> | span <group> <start> <n> |\n"
    "\tunit <unit> | blank <n> | fill]*";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (Batch) return result("%s is not available in batch mode", commandName);
  if (!Net) return warning("%s: no current network", commandName);

  numRowsStr = Tcl_GetStringFromObj(objv[1], NULL);
  if (objc > 1 && numRowsStr[0] == '*') {
    do {
      if (plotRowObj(interp, objc - 2, objv + 2, &unitsPlotted))
	return TCL_ERROR;
    } while (unitsPlotted > 0);
    Net->plotRows--;
  }
  else {
    arg = 1;
    if (objc > 1) {
      Tcl_GetIntFromObj(interp, objv[1], &numRows);
      if (numRows < 0 || (numRows == 0 && !strcmp(numRowsStr, "0")))
	return usageError(commandName, usage);
      if (numRows == 0)
	numRows = 1;
      else arg = 2;
    } else numRows = 1;

    for (i = 0; i < numRows; i++)
      if (plotRowObj(interp, objc - arg, objv + arg, &unitsPlotted))
	return TCL_ERROR;
  }
  Net->autoPlot = 0;
  return TCL_OK;
}

int C_plotAll(TCL_CMDARGS) {
  Tcl_Obj *args[3];
  const char c = 'c';
  int unitsPlotted;
  const char *usage = "plotAll <group>";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (Batch) return result("%s is not available in batch mode", commandName);
  if (objc > 2)
    return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  args[0] = Tcl_NewStringObj(&c, -1);
  Tcl_IncrRefCount(args[0]);

  args[1] = Tcl_NewStringObj(Tcl_GetStringFromObj(objv[1], NULL),-1);
  Tcl_IncrRefCount(args[1]);

  sprintf(Buffer, "%d", Net->plotCols);
  args[2] = Tcl_NewStringObj(Buffer, -1);
  Tcl_IncrRefCount(args[2]);

  do {
    if (plotRowObj(interp, 3, args, &unitsPlotted))
      return TCL_ERROR;
  } while (unitsPlotted > 0);
  Net->plotRows--;
  Net->autoPlot = 0;
  Tcl_DecrRefCount(args[0]);
  Tcl_DecrRefCount(args[1]);
  Tcl_DecrRefCount(args[2]);
  return TCL_OK;
}


int C_drawUnits(TCL_CMDARGS) {
  const char *usage = "drawUnits [<cell-size> [<cell-spacing>]]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (Batch) return result("%s is not available in batch mode", commandName);
  if (objc > 3)
    return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  if (objc >= 2)
    Tcl_GetIntFromObj(interp, objv[1], &Net->unitCellSize);
  if (objc >= 3)
    Tcl_GetIntFromObj(interp, objv[2], &Net->unitCellSpacing);

  return drawUnitsLater();
}

int C_autoPlot(TCL_CMDARGS) {
  int columns = 0;
  const char *usage = "autoPlot [<plot-columns>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (Batch) return result("%s is not available in batch mode", commandName);
  if (objc > 2)
    return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  if (objc >= 2)
    Tcl_GetIntFromObj(interp, objv[1], &columns);
  if (columns < 0)
    return warning("%s: numColumns cannot be negative", commandName);
  Net->autoPlot = 1;
  return autoPlot(columns);
}

int C_viewUnits(TCL_CMDARGS) {
  int arg, updates = 5;
  const char *usage = "viewUnits [-size <unitCellSize> | -gap <unitCellSpacing> |\n"
    "\t-updates <updateRate>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);

  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (Batch) return result("%s is not available in batch mode", commandName);
  for (arg = 1; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    switch (Tcl_GetStringFromObj(objv[arg], NULL)[1]) {
    case 's':
      if (++arg >= objc) return usageError(commandName, usage);
      if (Net) Tcl_GetIntFromObj(interp, objv[arg], &Net->unitCellSize);
      break;
    case 'g':
      if (++arg >= objc) return usageError(commandName, usage);
      if (Net) Tcl_GetIntFromObj(interp, objv[arg], &Net->unitCellSpacing);
      break;
    case 'u':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetIntFromObj(interp, objv[arg], &updates);
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);
  return eval(".viewUnits");
}

/* Not user callable */
int C_setUnitValue(TCL_CMDARGS) {
  int val;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 2)
    return warning("%s was called improperly", commandName);
  if (!Net) return warning("%s: no current network", commandName);

  Tcl_GetIntFromObj(interp, objv[1], &val);
  if (val < UV_OUT_TARG || val > UV_LINK_DELTAS)
    return warning("%s: bad value id: %d", commandName, val);

  return setUnitValue(val);
}

/* Not user callable */
int C_setUnitSet(TCL_CMDARGS) {
  int val;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 2)
    return warning("%s was called improperly", commandName);
  if (!Net) return warning("%s: no current network", commandName);

  Tcl_GetIntFromObj(interp, objv[1], &val);
  if (val != TEST_SET && val != TRAIN_SET)
    return warning("%s: bad value id: %d", commandName, val);
  if (val == TRAIN_SET && !Net->trainingSet)
    return warning("%s: no current testing set", commandName);
  if (val == TEST_SET && !Net->testingSet)
    return warning("%s: no current testing set", commandName);

  eval(".unit.l.label configure -text $_unitSetLabel(%d)", val);

  return loadExampleDisplay();
}

/* Not user callable */
int C_viewChange(TCL_CMDARGS) {
  int step, event, newTick, tick;
  long delay;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 2)
    return warning("%s was called improperly", commandName);

  event = Net->eventHistory[ViewTick];
  newTick = ViewTick;
  Tcl_GetIntFromObj(interp, objv[1], &step);
  switch (step) {
  case 0:
    newTick = 0;
    break;
  case 1:
    if (newTick <= 0) break;
    if (Net->eventHistory[newTick] != Net->eventHistory[newTick - 1]) {
      event--; newTick--;}
    while (newTick >= 0 &&
	   Net->eventHistory[newTick] == event) newTick--;
    if (Net->eventHistory[newTick] != event) newTick++;
    break;
  case 2:
    newTick = imax(newTick - 1, 0);
    break;
  case 3:
    newTick = imin(newTick + 1, Net->ticksOnExample - 1);
    break;
  case 4:
  case 6:
    if (newTick == Net->ticksOnExample - 1) break;
    if (Net->eventHistory[newTick] != Net->eventHistory[newTick + 1]) {
      event++; newTick++;}
    while (newTick < Net->ticksOnExample &&
	   Net->eventHistory[newTick] == event) newTick++;
    newTick--;
    break;
  case 5:
  case 7:
    newTick = Net->ticksOnExample - 1;
    break;
  default: return warning("%s: bad case: %d", commandName, step);
  }
  if (step != 6 && step != 7) {
    ViewTick = newTick;
    return updateUnitDisplay();
  }

  for (tick = ViewTick; tick < newTick; tick++) {
    Tcl_ExprLong(interp, "[.unit.t.scale get]", &delay);
    eval(".wait %ld", delay);
    if (tick != ViewTick) break;
    ViewTick++;
    updateUnitDisplay();
  }
  return TCL_OK;
}

/* Not user callable */
int C_viewChangeEvent(TCL_CMDARGS) {
  int event;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 2)
    return warning("%s was called improperly", commandName);
  if (!Net) return warning("%s: no current network", commandName);
  if (!Net->currentExample) return warning("%s: no current example", commandName);


  if (Tcl_GetStringFromObj(objv[1], NULL)[0] == '\0')
    event = -1;
  else {
    Tcl_GetIntFromObj(interp, objv[1], &event);
    if (event < 0) event = -1;
    event = imin(event, Net->currentExample->numEvents - 1);
  }
  for (ViewTick = 0; ViewTick < Net->ticksOnExample &&
	 Net->eventHistory[ViewTick] < event; ViewTick++);

  return updateUnitDisplay();
}

/* Not user callable */
int C_viewChangeTime(TCL_CMDARGS) {
  int interval;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 2)
    return warning("%s was called improperly", commandName);
  if (!Net) return warning("%s: no current network", commandName);
  if (sscanf(Tcl_GetStringFromObj(objv[1], NULL), "%d:%d", &interval, &ViewTick) != 2)
    return warning("Bad time format: %s", Tcl_GetStringFromObj(objv[1], NULL));
  ViewTick += interval * Net->ticksPerInterval - 1;
  ViewTick = MAX(ViewTick, 0);
  ViewTick = imin(ViewTick, Net->ticksOnExample - 1);
  return updateUnitDisplay();
}

/* Not user callable */
int C_viewChangeEventTime(TCL_CMDARGS) {
  int event, interval, tick;
  const char *timeCode;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 2)
    return warning("%s was called improperly", commandName);
  if (!Net) return warning("%s: no current network", commandName);
  event = Net->eventHistory[ViewTick];
  timeCode = Tcl_GetStringFromObj(objv[1], NULL);
  if (sscanf(timeCode, "%d:%d", &interval, &tick) != 2)
    return warning("Bad time format: %s", timeCode);
  tick += interval * Net->ticksPerInterval - 1;
  for (ViewTick = 0; Net->eventHistory[ViewTick] < event; ViewTick++);
  ViewTick += tick;
  ViewTick = MAX(ViewTick, 0);
  ViewTick = imin(ViewTick, Net->ticksOnExample - 1);
  return updateUnitDisplay();
}


/* Not user callable */
int C_chooseUnitSet(TCL_CMDARGS) {
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 1)
    return warning("%s was called improperly", commandName);
  return chooseUnitSet();
}

/* Not user callable */
int C_setUnitUnit(TCL_CMDARGS) {
  Unit newUnit;
  int g, u;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 3)
    return warning("%s was called improperly", commandName);
  if (!Net) return warning("%s: no current network", commandName);
  Tcl_GetIntFromObj(interp, objv[1], &g);
  Tcl_GetIntFromObj(interp, objv[2], &u);
  if (!(newUnit = lookupUnitNum(g, u)))
    return warning("%s: unit \"%d %d\" doesn't exist", commandName,
		   g, u);

  if (Net->unitDisplayValue < UV_LINK_WEIGHTS) {
    /* If we were in a unit value mode, this changes to link weight mode */
    Net->unitDisplayUnit = newUnit;
    return setUnitValue(UV_LINK_WEIGHTS);
  } else if (Net->unitDisplayUnit == newUnit) {
    /* If we were in a link weight mode and we click the active unit, returns
       to unit value mode */
    return setUnitValue(UV_OUT_TARG);
  } else {
    Net->unitDisplayUnit = newUnit;
    return updateUnitDisplay();
  }
}

/* Not user callable */
int C_lockUnit(TCL_CMDARGS) {
  Unit U;
  int g, u;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 3 && objc != 1)
    return warning("%s was called improperly", commandName);
  if (!Net) return warning("%s: no current network", commandName);
  if (objc == 1) Net->unitLocked = NULL;
  else {
    Tcl_GetIntFromObj(interp, objv[1], &g);
    Tcl_GetIntFromObj(interp, objv[2], &u);
    if (!(U = lookupUnitNum(g, u)))
      return warning("%s: unit \"%d %d\" doesn't exist", commandName,
                     g, u);
    if (U == Net->unitLocked) Net->unitLocked = NULL;
    else Net->unitLocked = U;
  }
  return updateUnitDisplay();
}

/* Not user callable */
int C_unitInfo(TCL_CMDARGS) {
  Unit U, T = NULL;
  Link L;
  int tick, g, u;
  flag printIt, targets;
  real value = NaN;
  char *from = NULL, *to = NULL, *valName = NULL;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);

  if (objc != 5)
    return warning("%s was called improperly", commandName);
  if (!Net) return warning("%s: no current network", commandName);

  tick = ViewTick;
  Tcl_GetIntFromObj(interp, objv[3], &targets);
  Tcl_GetIntFromObj(interp, objv[4], &printIt);

  if (!printIt && (U = Net->unitLocked)) {
    targets = 0;
  } else {
    if (!Tcl_GetStringFromObj(objv[1], NULL)[0]) {
      eval("set .unitUnit {}; set .unitValue {};");
      return TCL_OK;
    }
    Tcl_GetIntFromObj(interp, objv[1], &g);
    Tcl_GetIntFromObj(interp, objv[2], &u);
    if (!(U = lookupUnitNum(g, u)))
      return warning("%s: unit \"%d %d\" doesn't exist", commandName,
	  	     g, u);
  }

  if (!printIt)
    eval("set .unitUnit {%s}; set .unitName {%s}; set .unitTargets %d;"
	 "set .unitGroupNum %d; set .unitNum %d;",
	 U->name, U->name, targets, U->group->num, U->num);

  if (Net->unitDisplayValue == UV_LINK_WEIGHTS ||
      Net->unitDisplayValue == UV_LINK_DERIVS ||
      Net->unitDisplayValue == UV_LINK_DELTAS) {
    if (!Net->unitDisplayUnit) {
      if (printIt) print(0, "Use the right mouse button to select a unit.\n");
      else return TCL_OK;
    }
    if ((L = lookupLink(U, Net->unitDisplayUnit, NULL, ALL_LINKS))) {
      from = U->name; to = Net->unitDisplayUnit->name;
      T = Net->unitDisplayUnit;
    } else if ((L = lookupLink(Net->unitDisplayUnit, U, NULL, ALL_LINKS))) {
      from = Net->unitDisplayUnit->name; to = U->name; T = U;
    }
    if (L) {
      switch (Net->unitDisplayValue) {
      case UV_LINK_WEIGHTS: valName = "Weight"; value = L->weight; break;
      case UV_LINK_DERIVS:  valName = "Deriv";  value = L->deriv;  break;
      case UV_LINK_DELTAS:  valName = "Delta";
	value = getLink2(T, L)->lastWeightDelta;
	break;
      }
      if (printIt) {
	print(0, "%-6s = %9.6f for link \"%s\"->\"%s\" of type \"%s\"\n",
	      valName, value, from, to, LinkTypeName[getLinkType(T, L)]);
      } else eval("set .unitUnit {%s%s%s}; set .unitValue %9.6f",
		  (to == U->name) ? "->" : "", U->name, (from == U->name) ?
		  "->" : "", value);
    } else if (printIt) {
      print(0, "There is no link between \"%s\" and \"%s\"\n",
	    Net->unitDisplayUnit->name, U->name);
    }
  } else {
    if (tick < 0 || tick >= Net->ticksOnExample || !Net->currentExample)
      return TCL_OK;
    switch (Net->unitDisplayValue) {
    case UV_OUT_TARG:
      if (targets) {
        if (!U->targetHistory || Net->ticksOnExample == 1) value = U->target;
        else if (ViewTick >= Net->ticksOnExample - Net->historyLength)
          value = GET_HISTORY(U, targetHistory, HISTORY_INDEX(ViewTick));
        valName = "Target";
      } else {
        if (!U->outputHistory || Net->ticksOnExample == 1) value = U->output;
        else if (ViewTick >= Net->ticksOnExample - Net->historyLength)
          value = GET_HISTORY(U, outputHistory, HISTORY_INDEX(ViewTick));
        valName = "Output";
      }
      break;
    case UV_OUTPUTS:
      if (!U->outputHistory || Net->ticksOnExample == 1) value = U->output;
      else if (ViewTick >= Net->ticksOnExample - Net->historyLength)
        value = GET_HISTORY(U, outputHistory, HISTORY_INDEX(ViewTick));
            valName = "Output";
      break;
    case UV_TARGETS:
      if (!U->targetHistory || Net->ticksOnExample == 1) value = U->target;
      else if (ViewTick >= Net->ticksOnExample - Net->historyLength)
	value = GET_HISTORY(U, targetHistory, HISTORY_INDEX(ViewTick));
      valName = "Target";
      break;
    case UV_INPUTS:
      if (!U->inputHistory || Net->ticksOnExample == 1) value = U->input;
      else if (ViewTick >= Net->ticksOnExample - Net->historyLength)
        value = GET_HISTORY(U, inputHistory, HISTORY_INDEX(ViewTick));
      valName = "Input";
      break;
    case UV_EXT_INPUTS:
      value = (tick == Net->ticksOnExample - 1) ? U->externalInput : NaN;
      valName = "Ext. Input";
      break;
    case UV_OUTPUT_DERIVS:
      if (!U->outputDerivHistory || Net->ticksOnExample == 1)
        value = U->outputDeriv;
      else if (ViewTick >= Net->ticksOnExample - Net->historyLength)
        value = GET_HISTORY(U, outputDerivHistory, HISTORY_INDEX(ViewTick));
            valName = "OutputDeriv";
      break;
    case UV_INPUT_DERIVS:
      value = (tick == Net->ticksOnExample - 1) ? U->inputDeriv : NaN;
      valName = "InputDeriv";
      break;
    case UV_GAIN:
      value = U->gain;
      valName = "Gain";
      break;
    default:
      return warning("%s: bad unitValue %d\n", commandName, Net->unitDisplayValue);
    }

    if (printIt) {
      if (isNaN(value)) print(0, "%-11s not stored on tick %d for unit "
			      "\"%s\"\n", valName, tick, U->name);
      else print(0, "%-11s = %9.6f on tick %d for unit \"%s\"\n",
		 valName, value, tick, U->name);
    } else eval("set .unitValue {%s%9.6f}",
		(Net->unitDisplayValue == UV_OUT_TARG) ? (targets) ?
		"T: " : "O: " : "", value);
  }
  return TCL_OK;
}

int C_graphUnitValue(TCL_CMDARGS) {
  Unit U;
  Link L;
  flag targets;
  char value[128], update[32];
  int g, u, lnum;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);

  if (objc != 4)
    return warning("%s was called improperly", commandName);
  if (!Net) return warning("%s: no current network", commandName);

  Tcl_GetIntFromObj(interp, objv[1], &g);
  Tcl_GetIntFromObj(interp, objv[2], &u);
  Tcl_GetIntFromObj(interp, objv[3], &targets);
  if (!(U = lookupUnitNum(g, u)))
    return warning("%s: unit \"%d %d\" doesn't exist", commandName, g, u);

  sprintf(value, "group(%d).unit(%d).", g, u);
  if (Net->unitDisplayValue == UV_LINK_WEIGHTS ||
      Net->unitDisplayValue == UV_LINK_DERIVS ||
      Net->unitDisplayValue == UV_LINK_DELTAS) {
    if (!Net->unitDisplayUnit)
      return warning("Use the right mouse button to select a unit.\n");

    if ((L = lookupLink(U, Net->unitDisplayUnit, &lnum, ALL_LINKS))) {
      sprintf(value, "group(%d).unit(%d).", Net->unitDisplayUnit->group->num,
              Net->unitDisplayUnit->num);
    } else L = lookupLink(Net->unitDisplayUnit, U, &lnum, ALL_LINKS);
    if (L) {
      switch (Net->unitDisplayValue) {
      case UV_LINK_WEIGHTS:
        sprintf(value + strlen(value), "incoming(%d).weight", lnum);
        break;
      case UV_LINK_DERIVS:
        sprintf(value + strlen(value), "incoming(%d).deriv", lnum);
        break;
      case UV_LINK_DELTAS:
        sprintf(value + strlen(value), "incoming2(%d).lastWeightDelta", lnum);
        break;
      }
    } else return warning("There is no link between \"%s\" and \"%s\"\n",
                          Net->unitDisplayUnit->name, U->name);
    sprintf(update, "WEIGHT_UPDATES");
  } else {
    switch (Net->unitDisplayValue) {
    case UV_OUT_TARG:
      if (targets) strcat(value, "target");
      else strcat(value, "output");
      break;
    case UV_OUTPUTS:
      strcat(value, "output");
      break;
    case UV_TARGETS:
      strcat(value, "target");
      break;
    case UV_INPUTS:
      strcat(value, "input");
      break;
    case UV_EXT_INPUTS:
      strcat(value, "externalInput");
      break;
    case UV_OUTPUT_DERIVS:
      strcat(value, "outputDeriv");
      break;
    case UV_INPUT_DERIVS:
      strcat(value, "inputDeriv");
      break;
    case UV_GAIN:
      strcat(value, "gain");
      break;
    default:
      return warning("%s: bad unitValue %d\n", commandName, Net->unitDisplayValue);
    }
    sprintf(update, "TICKS");
  }
  return eval("graphObject %s -u %s", value, update);
}

/* Not user callable */
int C_updateUnitDisplay(TCL_CMDARGS) {
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 1)
    return warning("%s was called improperly", commandName);
  if (Net) return updateUnitDisplay();
  return TCL_OK;
}

/* Not user callable */
int C_setUnitTemp(TCL_CMDARGS) {
  double val;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 2)
    return warning("%s was called improperly", commandName);
  Tcl_GetDoubleFromObj(interp, objv[1], &val);
  return setUnitTemp(val);
}

int C_setUnitExampleProc(TCL_CMDARGS) {
  ExampleSet S;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  int proc;
  if (objc != 2)
    return warning("%s was called improperly", commandName);
  Tcl_GetIntFromObj(interp, objv[1], &proc);
  Tcl_UpdateLinkedVar(interp, ".unitExampleProc");
  if (proc) {
    Net->netRunExample = Net->netTrainExample;
    Net->netRunTick = Net->netTrainTick;
  } else {
    Net->netRunExample = Net->netTestExample;
    Net->netRunTick = Net->netTestTick;
  }
  if (Net->unitDisplaySet == TRAIN_SET)
    S = Net->trainingSet;
  else
    S = Net->testingSet;
  if (Net->currentExample && S) {
    if (Net->netRunExampleNum(Net->currentExample->num, S)) return TCL_ERROR;
    if (UnitUp) updateUnitDisplay();
  }
  return TCL_OK;
}

/* Not user callable */
int C_selectExample(TCL_CMDARGS) {
  ExampleSet S;
  int ex = 0;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 2)
    return warning("%s was called improperly", commandName);
  if (Net->unitDisplaySet == TRAIN_SET)
    S = Net->trainingSet;
  else
    S = Net->testingSet;
  Tcl_GetIntFromObj(interp, objv[1], &ex);
  if (Net->netRunExampleNum(ex, S)) return TCL_ERROR;
  if (UnitUp) updateUnitDisplay();
  return TCL_OK;
}

/**************/

int C_viewLinks(TCL_CMDARGS) {
  int arg, curInt, updates = 5;
  char *curStr;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "viewLinks [-out <group-list> | -in <group-list> | "
    "-size <linkCellSize> |\n\t-gap <linkCellSpacing> | "
    "-updates <updateRate>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (Batch) return result("%s is not available in batch mode", commandName);
  for (arg = 1; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    curStr = Tcl_GetStringFromObj(objv[arg], NULL);
    Tcl_GetIntFromObj(interp, objv[arg], &curInt);
    switch (curStr[1]) {
    case 'o':
      if (++arg >= objc) return usageError(commandName, usage);
      FOR_EACH_GROUP(G->showOutgoing = FALSE);
      FOR_EACH_GROUP_IN_LIST(curStr, G->showOutgoing = TRUE);
      break;
    case 'i':
      if (++arg >= objc) return usageError(commandName, usage);
      FOR_EACH_GROUP(G->showIncoming = FALSE);
      FOR_EACH_GROUP_IN_LIST(curStr, G->showIncoming = TRUE);
      break;
    case 's':
      if (++arg >= objc) return usageError(commandName, usage);
      if (Net) Net->linkCellSize = curInt;
      break;
    case 'g':
      if (++arg >= objc) return usageError(commandName, usage);
      if (Net) Net->linkCellSpacing = curInt;
      break;
    case 'u':
      if (++arg >= objc) return usageError(commandName, usage);
      updates = curInt;
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);
  return eval(".viewLinks");
}

/* Not user callable */
int C_setLinkValue(TCL_CMDARGS) {
  int val;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 2)
    return warning("%s was called improperly", commandName);
  if (!Net) return warning("%s: no current network", commandName);

  Tcl_GetIntFromObj(interp, objv[1], &val);
  if (val < UV_LINK_WEIGHTS || val > UV_LINK_DELTAS)
    return warning("%s: bad value id: %d", commandName, val);

  return setLinkValue(val);
}

/* Not user callable */
int C_lockLink(TCL_CMDARGS) {
  Unit U;
  int lnum, g, u;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 4 && objc != 1)
    return warning("%s was called improperly", commandName);
  if (!Net) return warning("%s: no current network", commandName);
  if (objc == 1) Net->linkLockedUnit = NULL;
  else {
    Tcl_GetIntFromObj(interp, objv[1], &g);
    Tcl_GetIntFromObj(interp, objv[2], &u);
    if (!(U = lookupUnitNum(g, u)))
      return warning("%s: unit \"%d %d\" doesn't exist", commandName,
                     g, u);
    Tcl_GetIntFromObj(interp, objv[3], &lnum);
    if (lnum >= U->numIncoming)
      return warning("%s: link %d out of range", commandName, lnum);
    if (U == Net->linkLockedUnit && lnum == Net->linkLockedNum)
      Net->linkLockedUnit = NULL;
    else {
      Net->linkLockedUnit = U;
      Net->linkLockedNum = lnum;
    }
  }
  return updateLinkDisplay();
}

/* Not user callable */
int C_linkInfo(TCL_CMDARGS) {
  Unit preUnit, postUnit;
  Link L;
  int lnum, g, u;
  real value;
  char *valName;
  flag printIt;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);

  if (objc != 5)
    return warning("%s was called improperly", commandName);
  if (!Net) return warning("%s: no current network", commandName);
  Tcl_GetIntFromObj(interp, objv[4], &printIt);

  if (!printIt && (postUnit = Net->linkLockedUnit)) {
    lnum = Net->linkLockedNum;
  } else {
    if (!Tcl_GetStringFromObj(objv[1], NULL)[0]) {
      eval("set .linkFromUnit {}; set .linkToUnit {}; set .linkValue {};");
      return TCL_OK;
    }
    Tcl_GetIntFromObj(interp, objv[1], &g);
    Tcl_GetIntFromObj(interp, objv[2], &u);
    if (!(postUnit = lookupUnitNum(g, u)))
      return warning("%s: unit \"%d %d\" doesn't exist", commandName, g, u);
    Tcl_GetIntFromObj(interp, objv[3], &lnum);
  }
  if (lnum >= postUnit->numIncoming)
    return warning("%s: link %d out of range\n", lnum);
  L = postUnit->incoming + lnum;

  if (!(preUnit = lookupPreUnit(postUnit, lnum)))
    return warning("%s: pre unit not found", commandName);

  switch (Net->linkDisplayValue) {
  case UV_LINK_WEIGHTS:
    value = L->weight; valName = "Weight"; break;
  case UV_LINK_DERIVS:
    value = L->deriv; valName = "Deriv"; break;
  case UV_LINK_DELTAS:
    value = postUnit->incoming2[lnum].lastWeightDelta;
    valName = "Delta"; break;
  default:
    return warning("%s: bad linkValue %d\n", commandName, Net->linkDisplayValue);
  }
  if (printIt) {
    print(0, "%-6s = %9.6f for link \"%s\"->\"%s\" of type %s\n", valName,
	  value, preUnit->name, postUnit->name,
	  LinkTypeName[getLinkType(postUnit, L)]);
  } else {
    eval("set .linkFromUnit {%s}; set .linkToUnit {%s}; set .linkValue %f; "
         "set .linkG2 %d; set .linkU2 %d; set .linkLNum %d",
         preUnit->name, postUnit->name, value, postUnit->group->num,
         postUnit->num, lnum);
  }
  return TCL_OK;
}

int C_graphLinkValue(TCL_CMDARGS) {
  Unit U;
  char value[128];
  int g, u, lnum;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);

  if (objc != 4)
    return warning("%s was called improperly", commandName);
  if (!Net) return warning("%s: no current network", commandName);

  Tcl_GetIntFromObj(interp, objv[1], &g);
  Tcl_GetIntFromObj(interp, objv[2], &u);
  Tcl_GetIntFromObj(interp, objv[3], &lnum);
  if (!(U = lookupUnitNum(g, u)))
    return warning("%s: unit \"%d %d\" doesn't exist", commandName, g, u);

  sprintf(value, "group(%d).unit(%d).", g, u);
  switch (Net->linkDisplayValue) {
  case UV_LINK_WEIGHTS:
    sprintf(value + strlen(value), "incoming(%d).weight", lnum);
    break;
  case UV_LINK_DERIVS:
    sprintf(value + strlen(value), "incoming(%d).deriv", lnum);
    break;
  case UV_LINK_DELTAS:
    sprintf(value + strlen(value), "incoming2(%d).lastWeightDelta", lnum);
    break;
  }
  return eval("set .g [graphObject %s -u WEIGHT_UPDATES]; setObject graph(${.g}).fixMin 0", value);
}

/* Not user callable */
int C_updateLinkDisplay(TCL_CMDARGS) {
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 1)
    return warning("%s was called improperly", commandName);
  if (Net) return updateLinkDisplay();
  return TCL_OK;
}

/* Not user callable */
int C_drawLinks(TCL_CMDARGS) {
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 1)
    return warning("%s was called improperly", commandName);
  if (!Net) return warning("viewLinks: no current net");
  return drawLinksLater();
}

/* Not user callable */
int C_setLinkTemp(TCL_CMDARGS) {
  double val;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 2)
    return warning("%s was called improperly", commandName);
  Tcl_GetDoubleFromObj(interp, objv[1], &val);
  return setLinkTemp(val);
}

/* Not user callable */
int C_getLinkGroups(TCL_CMDARGS) {
  int pre, post;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 1)
    return warning("%s was called improperly", commandName);
  Tcl_ResetResult(interp);
  if (!Net) return TCL_OK;
  FOR_EACH_GROUP_BACK({
    pre = (!G->numOutgoing) ? -1 : (G->showOutgoing) ? 1 : 0;
    post = (!G->numIncoming) ? -1 : (G->showIncoming) ? 1 : 0;
    append("{ \"%s\" %d %d %d } ", G->name, G->num, pre, post);
  });
  return TCL_OK;
}

/* Not user callable */
int C_setLinkGroup(TCL_CMDARGS) {
  int post, state;
  char *glab;
  Group G;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 4)
    return warning("%s was called improperly", commandName);
  Tcl_GetIntFromObj(interp, objv[1], &post);
  glab = Tcl_GetStringFromObj(objv[2], NULL);
  if (!(G = lookupGroup(glab)))
    return warning("%s: group \"%s\" doesn't exist", commandName, glab);
  Tcl_GetIntFromObj(interp, objv[3], &state);
  if (post)
    G->showIncoming = state;
  else
    G->showOutgoing = state;
  return drawLinksLater();
}

/* Not user callable */
int C_setAllLinkGroups(TCL_CMDARGS) {
  int post, on;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 3)
    return warning("%s was called improperly", commandName);
  Tcl_GetIntFromObj(interp, objv[1], &post);
  Tcl_GetIntFromObj(interp, objv[2], &on);
  FOR_EACH_GROUP_BACK({
    if (post) {
      G->showIncoming = on;
      eval("set .link.post.%d %d", G->num, on);
    } else {
      G->showOutgoing = on;
      eval("set .link.pre.%d %d", G->num, on);
    }
  });
  return drawLinksLater();
}

/* Not user callable */
int C_setPalette(TCL_CMDARGS) {
  int win, style;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 3)
    return warning("%s was called improperly", commandName);
  if (!Net) return TCL_OK;
  Tcl_GetIntFromObj(interp, objv[1], &win);
  Tcl_GetIntFromObj(interp, objv[2], &style);
  fillPalette(style);
  if (win == 0) {
    Net->unitPalette = style;
    updateUnitDisplay();
  } else {
    Net->linkPalette = style;
    updateLinkDisplay();
  }
  return TCL_OK;
}

int C_viewConsole(TCL_CMDARGS) {
  const char *usage = "viewConsole";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc > 1) return usageError(commandName, usage);
  if (Batch) return warning("%s is not available in batch mode", commandName);
  if (!Console &&
      !queryUser("Starting the console will permanently close your other "
		 "shell interface.  Start it anyway?")) return TCL_OK;
  createConsole(!Console);
  eval("consoleinterp eval tkConsolePrompt");
  return result("");
}

/********************/

void registerViewCommands(void) {
  registerSynopsis("view",      "opens the main control display");
  registerCommand((Tcl_ObjCmdProc *)C_viewConsole, "viewConsole", "opens the shell console");
  registerCommand((Tcl_ObjCmdProc *)C_viewUnits, "viewUnits", "opens the unit viewer");
  registerCommand((Tcl_ObjCmdProc *)C_viewLinks, "viewLinks", "opens the link viewer");
  registerSynopsis("viewObject", "opens an object viewer");
}

void registerDisplayCommands(void) {
  registerCommand((Tcl_ObjCmdProc *)C_resetPlot, "resetPlot",
	          "resets the plot layout");
  registerCommand((Tcl_ObjCmdProc *)C_plotRow, "plotRow",
		  "defines one or more rows of the unit display plot");
  registerCommand((Tcl_ObjCmdProc *)C_plotAll, "plotAll",
		  "plots all the units in a group in as many rows as needed");
  registerCommand((Tcl_ObjCmdProc *)C_drawUnits, "drawUnits",
		  "actually renders a new plot on the unit display");
  registerCommand((Tcl_ObjCmdProc *)C_autoPlot, "autoPlot",
		  "plots the entire network in a charming format");
  createCommand((Tcl_ObjCmdProc *)C_setUnitValue, ".setUnitValue");
  createCommand((Tcl_ObjCmdProc *)C_setUnitSet,   ".setUnitSet");
  createCommand((Tcl_ObjCmdProc *)C_chooseUnitSet,".chooseUnitSet");
  createCommand((Tcl_ObjCmdProc *)C_setUnitUnit,  ".setUnitUnit");
  createCommand((Tcl_ObjCmdProc *)C_lockUnit,     ".lockUnit");
  createCommand((Tcl_ObjCmdProc *)C_unitInfo,     ".unitInfo");
  createCommand((Tcl_ObjCmdProc *)C_graphUnitValue, ".graphUnitValue");
  createCommand((Tcl_ObjCmdProc *)C_updateUnitDisplay, ".updateUnitDisplay");
  createCommand((Tcl_ObjCmdProc *)C_setUnitTemp,  ".setUnitTemp");
  createCommand((Tcl_ObjCmdProc *)C_setUnitExampleProc,  ".setUnitExampleProc");
  createCommand((Tcl_ObjCmdProc *)C_selectExample,".selectExample");
  createCommand((Tcl_ObjCmdProc *)C_viewChange,   ".viewChange");
  createCommand((Tcl_ObjCmdProc *)C_viewChangeEvent, ".viewChangeEvent");
  createCommand((Tcl_ObjCmdProc *)C_viewChangeTime, ".viewChangeTime");
  createCommand((Tcl_ObjCmdProc *)C_viewChangeEventTime, ".viewChangeEventTime");

  createCommand((Tcl_ObjCmdProc *)C_setLinkValue, ".setLinkValue");
  createCommand((Tcl_ObjCmdProc *)C_lockLink,     ".lockLink");
  createCommand((Tcl_ObjCmdProc *)C_linkInfo,     ".linkInfo");
  createCommand((Tcl_ObjCmdProc *)C_graphLinkValue, ".graphLinkValue");
  createCommand((Tcl_ObjCmdProc *)C_updateLinkDisplay, ".updateLinkDisplay");
  createCommand((Tcl_ObjCmdProc *)C_setLinkTemp,  ".setLinkTemp");
  createCommand((Tcl_ObjCmdProc *)C_drawLinks,    ".drawLinks");
  createCommand((Tcl_ObjCmdProc *)C_getLinkGroups,".getLinkGroups");
  createCommand((Tcl_ObjCmdProc *)C_setLinkGroup, ".setLinkGroup");
  createCommand((Tcl_ObjCmdProc *)C_setAllLinkGroups, ".setAllLinkGroups");
  createCommand((Tcl_ObjCmdProc *)C_setPalette,   ".setPalette");

  Tcl_LinkVar(Interp, ".unitUp", (char *) &UnitUp, TCL_LINK_INT);
  Tcl_LinkVar(Interp, ".linkUp", (char *) &LinkUp, TCL_LINK_INT);
  eval(".setPalette 0 ${.unitPalette}; .setPalette 1 ${.linkPalette}");
}
