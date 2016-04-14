#include <string.h>
#include "system.h"
#include "util.h"
#include "type.h"
#include "network.h"
#include "connect.h"
#include "command.h"
#include "control.h"
#include "act.h"
#include "display.h"
#include "object.h"


/******************************* Connection Commands *************************/

int C_connectGroups(TCL_CMDARGS) {
  Group H;
  int arg, firstType;
  char *curString, *prevString, *typeName = NULL;
  mask type = FULL, linkType, biLinkType;
  real strength = 1.0, range = NaN, mean = NaN;
  flag bidirectional = FALSE, typeSet = FALSE;
  flag (*connectProc)(Group, Group, mask, real, real, real) =
    fullConnectGroups;
  double tempDbl;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "connectGroups <group-list1> (<group-list>)* [-projection "
    "<proj-type> |\n\t-strength <strength> | -mean <mean> | -range <range> |\n"
    "\t-type <link-type> | -bidirectional]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc < 3) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);
  if (unsafeToRun(commandName, NETWORK_TASKS)) return TCL_ERROR;

  for (arg = 1; arg < objc && isGroupList(Tcl_GetStringFromObj(objv[arg], NULL)); arg++);
  if (arg <= 2) return usageError(commandName, usage);

  /* Read the options. */
  for (firstType = arg; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    curString = Tcl_GetStringFromObj(objv[arg], NULL);
    print(1,"Option: %s\n", curString);
    switch (curString[1]) {
    case 'p':
      if (++arg >= objc) return usageError(commandName, usage);
      curString = Tcl_GetStringFromObj(objv[arg], NULL);
      if (lookupTypeMask(curString, PROJECTION, &type))
        return warning("%s: bad projection type: %s\n", commandName, curString);
      if (!(connectProc = projectionProc(type)))
        return warning("%s: bad projection type: %s\n", commandName, curString);
      break;
    case 's':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetDoubleFromObj(interp, objv[arg], &tempDbl);
      strength = (real)tempDbl;
      if (strength <= 0.0 || strength > 1.0)
	return warning("%s: strength value (%f) must be in the range (0,1]",
		       commandName, strength);
      break;
    case 'm':
      if (++arg >= objc) return usageError(commandName, usage);
        Tcl_GetDoubleFromObj(interp, objv[arg], &tempDbl);
        mean = (real)tempDbl;
      break;
    case 'r':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetDoubleFromObj(interp, objv[arg], &tempDbl);
      range = (real)tempDbl;
      if (range < 0.0) return warning("%s: range must be positive", commandName);
      break;
    case 't':
      if (++arg >= objc) return usageError(commandName, usage);
      typeName = Tcl_GetStringFromObj(objv[arg], NULL);
      typeSet = TRUE;
      break;
    case 'b':
      bidirectional = TRUE;
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

  if (type == ONE_TO_ONE) {
    if (isNaN(mean)) mean = 1.0;
    if (isNaN(range)) range = 0.0;
  }

  for (arg = 2; arg < firstType; arg++) {
    curString = Tcl_GetStringFromObj(objv[arg], NULL);
    prevString = Tcl_GetStringFromObj(objv[arg-1], NULL);
    print(1,"Groups: %s %s\n", prevString, curString);
    FOR_EACH_GROUP_IN_LIST(prevString, {
      H = G;
      if (!typeSet) typeName = H->name;
      if (registerLinkType(typeName, &linkType)) return TCL_ERROR;
      FOR_EACH_GROUP_IN_LIST(curString, {
        if (connectProc(H, G, linkType, strength, range, mean))
          return TCL_ERROR;
        if (bidirectional) {
          if (!typeSet) typeName = G->name;
          if (registerLinkType(typeName, &biLinkType)) return TCL_ERROR;
          if (connectProc(G, H, biLinkType, strength, range, mean))
            return TCL_ERROR;
        }
      });
    });
  }

  return signalNetStructureChanged();
}

int C_connectGroupToUnits(TCL_CMDARGS) {
  int arg;
  mask linkType;
  char *curString, *typeName = NULL;
  real range = NaN, mean = NaN;
  flag typeSet = FALSE;
  double tempDbl;

  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "connectGroupToUnits <group-list> <unit-list> [-mean <mean> |\n"
    "\t-range <range> | -type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc < 3) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);
  if (unsafeToRun(commandName, NETWORK_TASKS)) return TCL_ERROR;

  /* Skip the unit list. */
  for (arg = 3; arg < objc && lookupUnit(Tcl_GetStringFromObj(objv[arg], NULL)); arg++);

  /* Read the options. */
  for (; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    curString = Tcl_GetStringFromObj(objv[arg], NULL);
    switch (curString[1]) {
    case 'm':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetDoubleFromObj(interp, objv[arg], &tempDbl);
      mean = (real)tempDbl;
      break;
    case 'r':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetDoubleFromObj(interp, objv[arg], &tempDbl);
      range = (real)tempDbl;
      if (range < 0.0) return warning("%s: range must be positive", commandName);
      break;
    case 't':
      if (++arg >= objc) return usageError(commandName, usage);
      typeName = Tcl_GetStringFromObj(objv[arg], NULL);
      typeSet = TRUE;
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

  FOR_EACH_GROUP_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), {
    if (!typeSet) typeName = G->name;
    if (registerLinkType(typeName, &linkType)) return TCL_ERROR;
    FOR_EACH_UNIT_IN_LIST(Tcl_GetStringFromObj(objv[2], NULL), {
      if (connectGroupToUnit(G, U, linkType, range, mean, FALSE))
        return TCL_ERROR;
    });
  });

  return signalNetStructureChanged();
}

int C_connectUnits(TCL_CMDARGS) {
  Unit V;
  int arg, firstType;
  mask linkType;
  char *curString, *typeName = NULL;
  real range = NaN, mean = NaN;
  flag bidirectional = FALSE;
  double tempDbl;

  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "connectUnits <unit-list1> (<unit-list>)* [-mean <mean>\n"
    "\t| -range <range> | -type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc < 3) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);
  if (unsafeToRun(commandName, NETWORK_TASKS)) return TCL_ERROR;

  for (arg = 1; arg < objc && isUnitList(Tcl_GetStringFromObj(objv[arg], NULL)); arg++);
  if (arg <= 2) return usageError(commandName, usage);

  /* Read the options. */
  for (firstType = arg; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    curString = Tcl_GetStringFromObj(objv[arg], NULL);
    switch (curString[1]) {
    case 'm':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetDoubleFromObj(interp, objv[arg], &tempDbl);
      mean = (real)tempDbl;
      break;
    case 'r':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetDoubleFromObj(interp, objv[arg], &tempDbl);
      range = (real)tempDbl;
      if (range < 0.0) return warning("%s: range must be positive", commandName);
      break;
    case 't':
      if (++arg >= objc) return usageError(commandName, usage);
      typeName = Tcl_GetStringFromObj(objv[arg], NULL);
      break;
    case 'b':
      bidirectional = TRUE;
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

  for (arg = 2; arg < firstType; arg++)
    FOR_EACH_UNIT_IN_LIST(Tcl_GetStringFromObj(objv[arg-1], NULL), {
      V = U;
      if (!typeName) typeName = V->group->name;
      if (registerLinkType(typeName, &linkType)) return TCL_ERROR;
      FOR_EACH_UNIT_IN_LIST(Tcl_GetStringFromObj(objv[arg], NULL), {
        if (connectUnits(V, U, linkType, range, mean, FALSE))
            return TCL_ERROR;
        if (bidirectional)
          if (connectUnits(U, V, linkType, range, mean, FALSE))
                  return TCL_ERROR;
      });
    });

  return signalNetStructureChanged();
}

int C_elmanConnect(TCL_CMDARGS) {
  Group source, context;
  int arg, curInt;
  double tempDbl;
  char *curString, *glab1, *glab2;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "elmanConnect <source-group> <context-group>\n"
    "\t[-reset <reset-on-example> | -initOutput <init-output>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc < 3) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);
  if (unsafeToRun(commandName, NETWORK_TASKS)) return TCL_ERROR;

  glab1 = Tcl_GetStringFromObj(objv[1], NULL);
  glab2 = Tcl_GetStringFromObj(objv[2], NULL);
  if (!(source = lookupGroup(glab1)))
    return warning("%s: group \"%s\" doesn't exist", commandName, glab1);
  if (!(context = lookupGroup(glab2)))
    return warning("%s: group \"%s\" doesn't exist", commandName, glab2);

  /* Read the options. */
  for (arg = 3; arg < objc && Tcl_GetStringFromObj(objv[arg],NULL)[0] == '-'; arg++) {
    Tcl_GetIntFromObj(interp, objv[arg], &curInt);
    curString = Tcl_GetStringFromObj(objv[arg], NULL);
    switch (curString[1]) {
    case 'r':
      if (++arg >= objc) return usageError(commandName, usage);
      if (curInt)
        changeGroupType(source, GROUP, RESET_ON_EXAMPLE, ADD_TYPE);
      else
        changeGroupType(source, GROUP, RESET_ON_EXAMPLE, REMOVE_TYPE);
      break;
    case 'i':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetDoubleFromObj(interp, objv[arg], &tempDbl);
      source->initOutput = (real)tempDbl;
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

  if (elmanConnect(source, context)) return TCL_ERROR;
  if (updateUnitDisplay()) return TCL_ERROR;
  return updateLinkDisplay();
}

int C_copyConnect(TCL_CMDARGS) {
  Group source, copy;
  int offset, set = 0;
  Unit U;
  GroupProc P;
  CopyData D;
  char *curString;

  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "copyConnect <source-group> <copy-group> <field>";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 4) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);
  if (unsafeToRun(commandName, NETWORK_TASKS)) return TCL_ERROR;


  if (!(source = lookupGroup(Tcl_GetStringFromObj(objv[1], NULL))))
    return warning("%s: group \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[1], NULL));
  if (!(copy = lookupGroup(Tcl_GetStringFromObj(objv[2], NULL))))
    return warning("%s: group \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[2], NULL));

  if (source->numUnits != copy->numUnits)
    return warning("%s: source and copy groups aren't the same size", commandName);

  curString = Tcl_GetStringFromObj(objv[3], NULL);
  if (subString(curString, "inputs", 6))
    offset = OFFSET(U, input);
  else if (subString(curString, "externalInputs", 1))
    offset = OFFSET(U, externalInput);
  else if (subString(curString, "outputs", 7))
    offset = OFFSET(U, output);
  else if (subString(curString, "targets", 1))
    offset = OFFSET(U, target);
  else if (subString(curString, "inputDerivs", 6))
    offset = OFFSET(U, inputDeriv);
  else if (subString(curString, "outputDerivs", 7))
    offset = OFFSET(U, outputDeriv);
  else return warning("%s: bad field type: %s\n", commandName, curString);

  for (P = copy->inputProcs; P && !set; P = P->next)
    if (P->type == IN_COPY && !P->otherData) {
      set = 1; break;}
  if (!P) for (P = copy->outputProcs; P && !set; P = P->next)
    if (P->type == OUT_COPY && !P->otherData) {set = 2; break;}
  if (!P) for (P = copy->costProcs; P && !set; P = P->next)
    if (P->type == TARGET_COPY && !P->otherData) {set = 3; break;}

  if (!set) return warning("Group \"%s\" has no empty *_COPY slots.",
			   copy->name);

  D = (CopyData) safeMalloc(sizeof(struct copyData), "copyConnect");
  D->source = source;
  D->offset = offset;
  P->otherData = (void *) D;

  if (set == 2) {
    if (isNaN(copy->minOutput)) copy->minOutput = source->minOutput;
    if (isNaN(copy->maxOutput)) copy->maxOutput = source->maxOutput;
  }

  return TCL_OK;
}

int C_addLinkType(TCL_CMDARGS) {
  mask type;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "addLinkType [<link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc > 2) return usageError(commandName, usage);

  if (objc == 1) return listLinkTypes();
  return registerLinkType(Tcl_GetStringFromObj(objv[1], NULL), &type);
}

int C_deleteLinkType(TCL_CMDARGS) {
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "deleteLinkType [<link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc > 2) return usageError(commandName, usage);

  if (objc == 1) return listLinkTypes();
  return unregisterLinkType(Tcl_GetStringFromObj(objv[1], NULL));
}

int C_saveWeights(TCL_CMDARGS) {
  flag binary = TRUE, frozen = TRUE, thawed = TRUE;
  int arg, values = 1;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "saveWeights <file-name> [-values <num-values> | -text |\n"
    "\t-noFrozen | -onlyFrozen]";
  Tcl_Obj *fileNameObj = objv[1];
  const char *fileName = Tcl_GetStringFromObj(objv[1], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc < 2) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  /* Read the options. */
  for (arg = 2; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    switch (Tcl_GetStringFromObj(objv[arg], NULL)[1]) {
    case 'v':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetIntFromObj(interp, objv[arg], &values);
      if (values < 1)
	return warning("%s: numValues must be positive", commandName);
      break;
    case 't':
      binary = FALSE;
      break;
    case 'n':
      frozen = FALSE;
      break;
    case 'o':
      thawed = FALSE;
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

#ifdef ADVANCED
  if (values > 3) values = 3;
#else
  if (values > 2) values = 2;
#endif /* ADVANCED */

  eval(".setPath _weights %s", objv[1]);

  if (Net->saveWeights(fileNameObj, binary, values, thawed, frozen))
    return TCL_ERROR;
  return result(fileName);
}

int C_loadWeights(TCL_CMDARGS) {
  int arg;
  flag frozen = TRUE, thawed = TRUE;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "loadWeights <file-name> [-noFrozen | -onlyFrozen]";
  const char *currentArgument;

  Tcl_Obj *fileNameObj = objv[1];
  const char *fileName = Tcl_GetStringFromObj(objv[1], NULL);
  if (objc == 2 && !strcmp(fileName, "-h"))
    return commandHelp(commandName);
  if (objc != 2 && objc != 3) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  /* Read the options. */
  for (arg = 2; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0]=='-'; arg++) {
    currentArgument = Tcl_GetStringFromObj(objv[arg], NULL);
    switch (currentArgument[1]) {
    case 'n':
      frozen = FALSE;
      break;
    case 'o':
      thawed = FALSE;
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

  eval(".setPath _weights %s", objv[1]);

  if (Net->loadWeights(fileNameObj, thawed, frozen)) return TCL_ERROR;
  if (updateUnitDisplay()) return TCL_ERROR;
  if (updateLinkDisplay()) return TCL_ERROR;
  return result(fileName);
}

int C_loadXerionWeights(TCL_CMDARGS) {
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "loadXerionWeights <file-name>";
  Tcl_Obj *fileNameObj = objv[1];
  const char *fileName = Tcl_GetStringFromObj(objv[1], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);
  eval(".setPath _weights %s", fileName);

  if (loadXerionWeights(fileNameObj)) return TCL_ERROR;
  if (updateUnitDisplay()) return TCL_ERROR;
  return updateLinkDisplay();
}


/****************************** Disconnection Commands ***********************/

int C_disconnectGroups(TCL_CMDARGS) {
  Group preGroup, postGroup;
  mask linkType = ALL_LINKS;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "disconnectGroups <group1> <group2> [-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 3 && objc != 5) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);
  if (unsafeToRun(commandName, NETWORK_TASKS)) return TCL_ERROR;

  if (!(preGroup = lookupGroup(Tcl_GetStringFromObj(objv[1], NULL))))
    return warning("%s: group \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[1], NULL));
  if (!(postGroup = lookupGroup(Tcl_GetStringFromObj(objv[2], NULL))))
    return warning("%s: group \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[2], NULL));
  if (objc == 5) {
    if (!subString(Tcl_GetStringFromObj(objv[3], NULL), "-type", 2)) return usageError(commandName, usage);
    if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[4], NULL))))
      return warning("%s: link type \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[4], NULL));
  }

  if (disconnectGroups(preGroup, postGroup, linkType)) return TCL_ERROR;
  return linksChanged();
}

int C_disconnectGroupUnit(TCL_CMDARGS) {
  Unit U;
  Group G;
  mask linkType = ALL_LINKS;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "disconnectGroupUnit <group> <unit> [-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 3 && objc != 5) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);
  if (unsafeToRun(commandName, NETWORK_TASKS)) return TCL_ERROR;

  if (!(G = lookupGroup(Tcl_GetStringFromObj(objv[1], NULL))))
    return warning("%s: group \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[1], NULL));
  if (!(U = lookupUnit(Tcl_GetStringFromObj(objv[2], NULL))))
    return warning("%s: unit \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[2], NULL));
  if (objc == 5) {
    if (!subString(Tcl_GetStringFromObj(objv[3], NULL), "-type", 2)) return usageError(commandName, usage);
    if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[4], NULL))))
      return warning("%s: link type \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[4], NULL));
  }

  if (disconnectGroupFromUnit(G, U, linkType)) return TCL_ERROR;
  return signalNetStructureChanged();
}

int C_disconnectUnits(TCL_CMDARGS) {
  Unit preUnit, postUnit;
  mask linkType = ALL_LINKS;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "disconnectUnits <unit1> <unit2> [-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 3 && objc != 5) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);
  if (unsafeToRun(commandName, NETWORK_TASKS)) return TCL_ERROR;

  if (!(preUnit = lookupUnit(Tcl_GetStringFromObj(objv[1], NULL))))
    return warning("%s: unit \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[1], NULL));
  if (!(postUnit = lookupUnit(Tcl_GetStringFromObj(objv[2], NULL))))
    return warning("%s: unit \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[2], NULL));
  if (objc == 5) {
    if (!subString(Tcl_GetStringFromObj(objv[3], NULL), "-type", 2)) return usageError(commandName, usage);
    if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[4], NULL))))
      return warning("%s: link type \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[4], NULL));
  }

  if (disconnectUnits(preUnit, postUnit, linkType)) return TCL_ERROR;
  return linksChanged();
}

int C_deleteLinks(TCL_CMDARGS) {
  mask linkType = ALL_LINKS;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "deleteLinks [-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 1 && objc != 3) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);
  if (unsafeToRun(commandName, NETWORK_TASKS)) return TCL_ERROR;

  if (objc == 3) {
    if (!subString(Tcl_GetStringFromObj(objv[1], NULL), "-type", 2)) return usageError(commandName, usage);
    if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[2], NULL))))
      return warning("%s: link type \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[2], NULL));
  }

  if (deleteAllLinks(linkType)) return TCL_ERROR;
  return linksChanged();
}

int C_deleteGroupInputs(TCL_CMDARGS) {
  mask linkType = ALL_LINKS;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "deleteGroupInputs <group-list> [-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2 && objc != 4) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);
  if (unsafeToRun(commandName, NETWORK_TASKS)) return TCL_ERROR;

  if (objc == 4) {
    if (!subString(Tcl_GetStringFromObj(objv[2], NULL), "-type", 2)) return usageError(commandName, usage);
    if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[3], NULL))))
      return warning("%s: link type \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[3], NULL));
  }

  FOR_EACH_GROUP_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), {
    if (deleteGroupInputs(G, linkType)) return TCL_ERROR;
  });
  return linksChanged();
}

int C_deleteGroupOutputs(TCL_CMDARGS) {
  mask linkType = ALL_LINKS;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "deleteGroupOutputs <group-list> [-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2 && objc != 4) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);
  if (unsafeToRun(commandName, NETWORK_TASKS)) return TCL_ERROR;

  if (objc == 4) {
    if (!subString(Tcl_GetStringFromObj(objv[2], NULL), "-type", 2)) return usageError(commandName, usage);
    if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[3], NULL))))
      return warning("%s: link type \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[3], NULL));
  }

  FOR_EACH_GROUP_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), {
    if (deleteGroupOutputs(G, linkType)) return TCL_ERROR;
  });
  return linksChanged();
}

int C_deleteUnitInputs(TCL_CMDARGS) {
  mask linkType = ALL_LINKS;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "deleteUnitInputs <unit-list> [-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2 && objc != 4) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);
  if (unsafeToRun(commandName, NETWORK_TASKS)) return TCL_ERROR;

  if (objc == 4) {
    if (!subString(Tcl_GetStringFromObj(objv[2], NULL), "-type", 2))
      return usageError(commandName, usage);
    if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[3], NULL))))
      return warning("%s: link type \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[3], NULL));
  }

  FOR_EACH_UNIT_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), {
    if (deleteUnitInputs(U, linkType)) return TCL_ERROR;
  });
  return linksChanged();
}

int C_deleteUnitOutputs(TCL_CMDARGS) {
  mask linkType = ALL_LINKS;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "deleteUnitOutputs <unit-list> [-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2 && objc != 4) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);
  if (unsafeToRun(commandName, NETWORK_TASKS)) return TCL_ERROR;

  if (objc == 4) {
    if (!subString(Tcl_GetStringFromObj(objv[2], NULL), "-type", 2)) return usageError(commandName, usage);
    if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[3], NULL))))
      return warning("%s: link type \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[3], NULL));
  }

  FOR_EACH_UNIT_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), {
    if (deleteUnitOutputs(U, linkType)) return TCL_ERROR;
  });
  return linksChanged();
}


/***************************** Link Value Commands ***************************/

int C_randWeights(TCL_CMDARGS) {
  int arg;
  mask linkType = ALL_LINKS;
  real range = NaN, mean = NaN;
  double tempDbl;
  char *groupList = NULL, *unitList = NULL;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "randWeights [-group <group-list> | -unit <unit-list> |\n"
    "\t-mean <mean> | -range <range> | -type <link-type>]";

  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (!Net) return warning("%s: no current network", commandName);

  for (arg = 1; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    switch (Tcl_GetStringFromObj(objv[arg], NULL)[1]) {
    case 'g':
      if (++arg >= objc) return usageError(commandName, usage);
      groupList = Tcl_GetStringFromObj(objv[arg], NULL);
      break;
    case 'u':
      if (++arg >= objc) return usageError(commandName, usage);
      unitList = Tcl_GetStringFromObj(objv[arg], NULL);
      break;
    case 'm':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetDoubleFromObj(interp, objv[arg], &tempDbl);
      mean = (real)tempDbl;
      break;
    case 'r':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetDoubleFromObj(interp, objv[arg], &tempDbl);
      range = (real)tempDbl;
      if (range < 0.0) return warning("%s: range must be positive", commandName);
      break;
    case 't':
      if (++arg >= objc) return usageError(commandName, usage);
      if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[arg], NULL))))
	return warning("%s: link type %s doesn't exist", commandName, objv[arg]);
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

  if (groupList) {
    FOR_EACH_GROUP_IN_LIST(groupList, {
      randomizeGroupWeights(G, chooseValue(range, Net->randRange),
			    chooseValue(mean, Net->randMean), range, mean,
			    linkType, TRUE);
    });
  }
  if (unitList) {
    FOR_EACH_UNIT_IN_LIST(unitList, {
      randomizeUnitWeights(U, chooseValue3(range, U->group->randRange,
					   Net->randRange),
			   chooseValue3(mean, U->group->randMean,
					Net->randMean),
			   range, mean, linkType, TRUE);
    });
  }
  if (!groupList && !unitList)
    randomizeWeights(range, mean, linkType, TRUE);
  if (updateUnitDisplay()) return TCL_ERROR;
  return updateLinkDisplay();
}

#ifdef JUNK
int C_randGroupWeights(TCL_CMDARGS) {
  int arg;
  mask linkType = ALL_LINKS;
  real range = NaN, mean = NaN;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "randGroupWeights <group-list> [-mean <mean> | -range <range>"
    " |\n\t-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc < 2) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  for (arg = 2; arg < objc && objv[arg][0] == '-'; arg++) {
    switch (objv[arg][1]) {
    case 'm':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetDoubleFromObj(interp, objv[arg], &mean);
      break;
    case 'r':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetDoubleFromObj(interp, objv[arg], &range);
      if (range < 0.0) return warning("%s: range must be positive", commandName);
      break;
    case 't':
      if (++arg >= objc) return usageError(commandName, usage);
      if (!(linkType = lookupLinkType(objv[arg])))
	return warning("%s: link type %s doesn't exist", commandName, objv[arg]);
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

  FOR_EACH_GROUP_IN_LIST(objv[1], {
    randomizeGroupWeights(G, chooseValue(range, Net->randRange),
			  chooseValue(mean, Net->randMean), range, mean,
			  linkType, TRUE);
  });
  if (updateUnitDisplay()) return TCL_ERROR;
  return updateLinkDisplay();
}

int C_randUnitWeights(TCL_CMDARGS) {
  int arg;
  mask linkType = ALL_LINKS;
  real range = NaN, mean = NaN;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "randUnitWeights <unit-list> [-mean <mean> -range <range>\n"
    "\t-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc < 2) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  for (arg = 2; arg < objc && objv[arg][0] == '-'; arg++) {
    switch (objv[arg][1]) {
    case 'm':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetDoubleFromObj(interp, objv[arg], &mean);
      break;
    case 'r':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetDoubleFromObj(interp, objv[arg], &range);
      if (range < 0.0) return warning("%s: range must be positive", commandName);
      break;
    case 't':
      if (++arg >= objc) return usageError(commandName, usage);
      if (!(linkType = lookupLinkType(objv[arg])))
	return warning("%s: link type %s doesn't exist", commandName, objv[arg]);
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

  FOR_EACH_UNIT_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), {
    randomizeUnitWeights(U, chooseValue3(range, U->group->randRange,
					 Net->randRange),
			 chooseValue3(mean, U->group->randMean,
				      Net->randMean),
			 range, mean, linkType, TRUE);
  });
  if (updateUnitDisplay()) return TCL_ERROR;
  return updateLinkDisplay();
}
#endif

int C_setLinkValues(TCL_CMDARGS) {
  mask linkType = ALL_LINKS;
  flag ext = FALSE;
  MemInfo M;
  int arg;
  char *groupList = NULL, *unitList = NULL;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "setLinkValues <parameter> <value> [-group <group-list> |\n"
    "\t-unit <unit-list> | -type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc < 3) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  if (!(M = lookupMember(Tcl_GetStringFromObj(objv[1], NULL), BlockInfo))) {
    if (!(M = lookupMember(Tcl_GetStringFromObj(objv[1], NULL), BlockExtInfo)))
      return warning("%s: %s is not a field of a block or block extension",
		     commandName, Tcl_GetStringFromObj(objv[1], NULL));
    ext = TRUE;
  }
  if (!(M->info->setValue && M->info->setStringValue && M->writable))
    return warning("%s: %s is not a writable field", commandName, Tcl_GetStringFromObj(objv[1], NULL));
  M->info->setStringValue(Buffer, Tcl_GetStringFromObj(objv[2], NULL));

  for (arg = 3; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    switch (Tcl_GetStringFromObj(objv[arg], NULL)[1]) {
    case 'g':
      if (++arg >= objc) return usageError(commandName, usage);
      groupList = Tcl_GetStringFromObj(objv[arg], NULL);
      break;
    case 'u':
      if (++arg >= objc) return usageError(commandName, usage);
      unitList = Tcl_GetStringFromObj(objv[arg], NULL);
      break;
    case 't':
      if (++arg >= objc) return usageError(commandName, usage);
      if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[arg], NULL))))
	return warning("%s: link type %s doesn't exist", commandName, Tcl_GetStringFromObj(objv[arg], NULL));
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

  if (groupList) {
    FOR_EACH_GROUP_IN_LIST(groupList, {
      setGroupBlockValues(G, ext, M, Buffer, linkType);
    });
  }
  if (unitList) {
    FOR_EACH_UNIT_IN_LIST(unitList, {
      setUnitBlockValues(U, ext, M, Buffer, linkType);
    });
  }
  if (!groupList && !unitList)
    setBlockValues(ext, M, Buffer, linkType);
  return TCL_OK;
}

#ifdef JUNK
int C_setGroupBlockValues(TCL_CMDARGS) {
  mask linkType = ALL_LINKS;
  flag ext = FALSE;
  MemInfo M;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "setGroupBlockValues <group-list> <parameter> <value>\n"
    "\t[-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 4 && objc != 6) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  if (!(M = lookupMember(Tcl_GetStringFromObj(objv[2], NULL), BlockInfo))) {
    if (!(M = lookupMember(Tcl_GetStringFromObj(objv[2], NULL), BlockExtInfo)))
      return warning("%s: %s is not a field of a block or block extension",
		     commandName, objv[2]);
    ext = TRUE;
  }
  if (!(M->info->setValue && M->info->setStringValue && M->writable))
    return warning("%s: %s is not a writable field", commandName, Tcl_GetStringFromObj(objv[2], NULL));
  M->info->setStringValue(Buffer, Tcl_GetStringFromObj(objv[3], NULL));

  if (objc == 6) {
    if (!subString(Tcl_GetStringFromObj(objv[4], NULL), "-type", 2)) return usageError(commandName, usage);
    if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[5], NULL))))
      return warning("%s: link type %s doesn't exist", commandName, Tcl_GetStringFromObj(objv[5], NULL));
  }

  FOR_EACH_GROUP_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), {
    setGroupBlockValues(G, ext, M, Buffer, linkType);
  });
  return TCL_OK;
}

int C_setUnitBlockValues(TCL_CMDARGS) {
  mask linkType = ALL_LINKS;
  flag ext = FALSE;
  MemInfo M;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "setUnitBlockValues <unit-list> <parameter> <value>\n"
    "\t[-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 4 && objc != 6) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  if (!(M = lookupMember(Tcl_GetStringFromObj(objv[2], NULL), BlockInfo))) {
    if (!(M = lookupMember(Tcl_GetStringFromObj(objv[2], NULL), BlockExtInfo)))
      return warning("%s: %s is not a field of a block or block extension",
		     commandName, Tcl_GetStringFromObj(objv[2], NULL));
    ext = TRUE;
  }
  if (!(M->info->setValue && M->info->setStringValue && M->writable))
    return warning("%s: %s is not a writable field",
        commandName, Tcl_GetStringFromObj(objv[2], NULL));
  M->info->setStringValue(Buffer, Tcl_GetStringFromObj(objv[3], NULL));

  if (objc == 6) {
    if (!subString(Tcl_GetStringFromObj(objv[4], NULL), "-type", 2))
      return usageError(commandName, usage);
    if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[5], NULL))))
      return warning("%s: link type %s doesn't exist",
          commandName, Tcl_GetStringFromObj(objv[5], NULL));
  }

  FOR_EACH_UNIT_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), {
    setUnitBlockValues(U, ext, M, Buffer, linkType);
  });
  return TCL_OK;
}
#endif

int C_printLinkValues(TCL_CMDARGS) {
  mask linkType = ALL_LINKS;
  int i, link, code = TCL_OK, arg;
  Tcl_Channel channel;
  char message[256];
  Group F;
  char *fromGroups, *printProc, *gl1, *gl2;
  flag append = FALSE;

  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "printLinkValues <file> <printProc> <group-list1> "
    "<group-list2>\n\t[-type <link-type> | -append]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc < 5) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  printProc = Tcl_GetStringFromObj(objv[2], NULL);
  gl1 = Tcl_GetStringFromObj(objv[3], NULL);
  gl2 = Tcl_GetStringFromObj(objv[4], NULL);

  for (arg = 5; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    switch (Tcl_GetStringFromObj(objv[arg], NULL)[1]) {
    case 't':
      if (++arg >= objc) return usageError(commandName, usage);
      if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[arg], NULL)))) {

        return warning("%s: link type %s doesn't exist",
            commandName, Tcl_GetStringFromObj(objv[arg], NULL));
      }
      break;
    case 'a':
      append = TRUE; break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

  if (!(channel = writeChannel(objv[1], append)))
    return warning("%s: couldn't write to the file %s", commandName, Tcl_GetStringFromObj(objv[1], NULL));

  fromGroups = (char *) safeCalloc(1, Net->numGroups, "printLinkValues");
  FOR_EACH_GROUP_IN_LIST(gl1, fromGroups[G->num] = 1);

  FOR_EACH_GROUP_IN_LIST(gl2, {
    FOR_EACH_UNIT(G, {
      link = 0;
      FOR_EACH_BLOCK(U, {
        F = B->unit->group;
        if (LINK_TYPE_MATCH(linkType, B) && fromGroups[F->num]) {
          for (i = 0; i < B->numUnits; i++, link++) {
            if (eval("%s group(%d) group(%d).unit(%d) group(%d) "
               "group(%d).unit(%d) %d %d",
               printProc, F->num, F->num, (B->unit + i)->num,
               G->num, G->num, U->num, b, link)) {
              strcpy(message, Tcl_GetStringResult(Interp));
              code = TCL_ERROR;
              goto abort;
            }
            Tcl_Write(channel, Tcl_GetStringResult(Interp), -1);
          }
        } else {
          link += B->numUnits;
        }
      });
    });
  });

 abort:
  FREE(fromGroups);
  closeChannel(channel);
  if (code) Tcl_SetResult(Interp, message, TCL_VOLATILE);
  else result("");
  return code;
}


/*********************** Freezing, Lesioning, and Noise **********************/

int C_freezeWeights(TCL_CMDARGS) {
  mask linkType = ALL_LINKS;
  int arg;
  char *groupList = NULL, *unitList = NULL;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "freezeWeights [-group <group-list> | -unit <unit-list> | "
    "-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (!Net) return warning("%s: no current network", commandName);

  for (arg = 1; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    switch (Tcl_GetStringFromObj(objv[arg], NULL)[1]) {
    case 'g':
      if (++arg >= objc) return usageError(commandName, usage);
      groupList = Tcl_GetStringFromObj(objv[arg], NULL);
      break;
    case 'u':
      if (++arg >= objc) return usageError(commandName, usage);
      unitList = Tcl_GetStringFromObj(objv[arg], NULL);
      break;
    case 't':
      if (++arg >= objc) return usageError(commandName, usage);
      if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[arg], NULL)))) {
        return warning("%s: link type %s doesn't exist",
            commandName, Tcl_GetStringFromObj(objv[arg], NULL));
      }
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

  if (groupList) {
    FOR_EACH_GROUP_IN_LIST(groupList, freezeGroupInputs(G, linkType));
  }
  if (unitList) {
    FOR_EACH_UNIT_IN_LIST(unitList, freezeUnitInputs(U, linkType));
  }
  if (!groupList && !unitList)
    freezeAllLinks(linkType);
  return linksChanged();
}

#ifdef JUNK
int C_freezeGroups(TCL_CMDARGS) {
  mask linkType = ALL_LINKS;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "freezeGroups <group-list> [-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2 && objc != 4) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  if (objc == 4) {
    if (!subString(Tcl_GetStringFromObj(objv[2], NULL), "-type", 2)) return usageError(commandName, usage);
    if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[3], NULL))))
      return warning("%s: link type %s doesn't exist",
          commandName, Tcl_GetStringFromObj(objv[3], NULL));
  }

  FOR_EACH_GROUP_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), freezeGroupInputs(G, linkType));
  return linksChanged();
}

int C_freezeUnits(TCL_CMDARGS) {
  mask linkType = ALL_LINKS;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "freezeUnits <unit-list> [-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2 && objc != 4)  return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  if (objc == 4) {
    if (!subString(Tcl_GetStringFromObj(objv[2], NULL), "-type", 2)) return usageError(commandName, usage);
    if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[3], NULL))))
      return warning("%s: link type %s doesn't exist",
          commandName, Tcl_GetStringFromObj(objv[3], NULL));
  }

  FOR_EACH_UNIT_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), freezeUnitInputs(U, linkType));
  return linksChanged();
}
#endif

int C_thawWeights(TCL_CMDARGS) {
  mask linkType = ALL_LINKS;
  int arg;
  char *groupList = NULL, *unitList = NULL;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "thawWeights [-group <group-list> | -unit <unit-list> | "
    "-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (!Net) return warning("%s: no current network", commandName);

  for (arg = 1; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    switch (Tcl_GetStringFromObj(objv[arg], NULL)[1]) {
    case 'g':
      if (++arg >= objc) return usageError(commandName, usage);
      groupList = Tcl_GetStringFromObj(objv[arg], NULL);
      break;
    case 'u':
      if (++arg >= objc) return usageError(commandName, usage);
      unitList = Tcl_GetStringFromObj(objv[arg], NULL);
      break;
    case 't':
      if (++arg >= objc) return usageError(commandName, usage);
      if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[arg], NULL))))
	return warning("%s: link type %s doesn't exist", commandName, Tcl_GetStringFromObj(objv[arg], NULL));
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

  if (groupList) {
    FOR_EACH_GROUP_IN_LIST(groupList, thawGroupInputs(G, linkType));
  }
  if (unitList) {
    FOR_EACH_UNIT_IN_LIST(unitList, thawUnitInputs(U, linkType));
  }
  if (!groupList && !unitList)
    thawAllLinks(linkType);
  return linksChanged();
}

#ifdef JUNK
int C_thawNet(TCL_CMDARGS) {
  mask linkType = ALL_LINKS;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "thawNet [-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc > 2)
    return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  if (objc == 3) {
    if (!subString(Tcl_GetStringFromObj(objv[1], NULL), "-type", 2)) return usageError(commandName, usage);
    if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[2], NULL))))
      return warning("%s: link type %s doesn't exist", commandName, Tcl_GetStringFromObj(objv[2], NULL));
  }

  thawAllLinks(linkType);
  return linksChanged();
}

int C_thawGroups(TCL_CMDARGS) {
  mask linkType = ALL_LINKS;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "thawGroups <group-list> [-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2 && objc != 3)
    return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  if (objc == 4) {
    if (!subString(Tcl_GetStringFromObj(objv[2], NULL), "-type", 2)) return usageError(commandName, usage);
    if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[3], NULL))))
      return warning("%s: link type %s doesn't exist", commandName, Tcl_GetStringFromObj(objv[3], NULL));
  }

  FOR_EACH_GROUP_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), thawGroupInputs(G, linkType));
  return linksChanged();
}

int C_thawUnits(TCL_CMDARGS) {
  mask linkType = ALL_LINKS;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "thawUnits <unit-list> [-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2 && objc != 3)
    return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  if (objc == 4) {
    if (!subString(Tcl_GetStringFromObj(objv[2], NULL), "-type", 2)) return usageError(commandName, usage);
    if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[3], NULL))))
      return warning("%s: link type %s doesn't exist", commandName, Tcl_GetStringFromObj(objv[3], NULL));
  }

  FOR_EACH_UNIT_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), thawUnitInputs(U, linkType));
  return linksChanged();
}
#endif

#ifdef JUNK
int C_holdGroups(TCL_CMDARGS) {
  Group G;
  int i;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  /* char *usage = "holdGroups [<group> ...]"; */
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (!Net) return warning("%s: no current network", commandName);

  if (objc == 1) {
    FOR_EACH_GROUP(if (!(G->type & HELD)) append("\"%s\" ", G->name));
    return TCL_OK;
  }

  FOR_EACH_GROUP(G, G->type &= ~HOLD_PENDING);

  for (i = 1; i < objc; i++) {
    if (!(G = lookupGroup(Tcl_GetStringFromObj(objv[i], NULL))))
      return warning("%s: group \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[i], NULL));
    G->type |= HOLD_PENDING;
  }
  return holdGroups();
}

int C_releaseGroups(TCL_CMDARGS) {
  Group G;
  int i;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  /* char *usage = "releaseGroups [<group> ...]"; */
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);

  if (!Net) return warning("%s: no current network", commandName);

  if (objc == 1) {
    FOR_EACH_GROUP(if (G->type & HELD) append("\"%s\" ", G->name));
    return TCL_OK;
  }

  FOR_EACH_GROUP(G, G->type &= ~HOLD_PENDING);

  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "*")) {
    FOR_EACH_GROUP(G, if (G->type & HELD) G->type |= HOLD_PENDING);
  } else
    for (i = 1; i < objc; i++) {
      if (!(G = lookupGroup(Tcl_GetStringFromObj(objv[i], NULL))))
	return warning("%s: group \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[i], NULL));
      G->type |= HOLD_PENDING;
    }

  return releaseGroups();
}
#endif

int C_lesionLinks(TCL_CMDARGS) {
  int arg;
  mask linkType = ALL_LINKS;
  real proportion = 1.0, value = NaN, range = NaN;
  flag mult = FALSE, flat = FALSE, discard = FALSE;
  real (*proc)(real value, real range);
  double tempDbl;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "lesionLinks <group-list> [-proportion <proportion> |\n"
    "\t-value <value> | -range <range> | -multiply | -flat | -discard\n"
    "\t-type <link-type>]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc < 2) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  /* Read the options. */
  for (arg = 2; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    switch (Tcl_GetStringFromObj(objv[arg], NULL)[1]) {
    case 'p':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetDoubleFromObj(interp, objv[arg], &tempDbl);
      proportion = (real)tempDbl;
      if (proportion <= 0.0 || proportion > 1.0)
	return warning("%s: link proportion (%f) must be in the range (0,1]",
		       commandName, proportion);
      break;
    case 'v':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetDoubleFromObj(interp, objv[arg], &tempDbl);
      value = (real)tempDbl;
      break;
    case 'r':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetDoubleFromObj(interp, objv[arg], &tempDbl);
      range = (real)tempDbl;
      break;
    case 'm':
      mult = TRUE; break;
    case 'f':
      flat = TRUE; break;
    case 'd':
      discard = TRUE; break;
    case 't':
      if (++arg >= objc) return usageError(commandName, usage);
      if (!(linkType = lookupLinkType(Tcl_GetStringFromObj(objv[arg], NULL))))
        return warning("%s: link type %s doesn't exist", commandName, objv[arg]);
            break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

  if (isNaN(value) && isNaN(range)) {
    FOR_EACH_GROUP_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), {
      if (lesionLinks(G, proportion, 0, 0, NULL, linkType)) return TCL_ERROR;
    });
    return linksChanged();
  } else if (isNaN(range)) {
    FOR_EACH_GROUP_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), {
      if (lesionLinks(G, proportion, 1, value, NULL, linkType))
	return TCL_ERROR;
    });
  } else {
    if (discard) {
      proc = (flat) ? addUniformNoise : addGaussianNoise;
    } else {
      if (flat)	proc = (mult) ? multUniformNoise : addUniformNoise;
      else	proc = (mult) ? multGaussianNoise : addGaussianNoise;
    }
    FOR_EACH_GROUP_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), {
      if (lesionLinks(G, proportion, (discard) ? 3:2, range, proc, linkType))
	return TCL_ERROR;
    });
  }

  if (updateUnitDisplay()) return TCL_ERROR;
  return updateLinkDisplay();
}

int C_lesionUnits(TCL_CMDARGS) {
  real proportion;
  double tempDbl;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "lesionUnits (<group-list> <proportion> | <unit-list>)";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2 && objc != 3)
    return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  if (objc == 2) {
    FOR_EACH_UNIT_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), lesionUnit(U));
    return TCL_OK;
  }

  Tcl_GetDoubleFromObj(interp, objv[2], &tempDbl);
  proportion = (real)tempDbl;
  if (proportion <= 0.0 || proportion > 1.0)
    return warning("%s: proportion of units (%f) must be in the range (0,1]",
		   commandName, proportion);

  FOR_EACH_GROUP_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), {
    FOR_EACH_UNIT(G, if (randProb() < proportion) lesionUnit(U));
  });
  return signalNetStructureChanged();
}

int C_healUnits(TCL_CMDARGS) {
  real proportion;
  double tempDbl;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "healUnits [(<group-list> <proportion> | <unit-list>)]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc > 3)
    return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  if (objc == 1) {
    FOR_EACH_GROUP(FOR_EVERY_UNIT(G, if (U->type & LESIONED) healUnit(U)));
    return TCL_OK;
  }
  else if (objc == 2) {
    FOR_EACH_UNIT_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), healUnit(U));
    return TCL_OK;
  }

  Tcl_GetDoubleFromObj(interp, objv[2], &tempDbl);
  proportion = (real)tempDbl;
  if (proportion <= 0.0 || proportion > 1.0)
    return warning("%s: proportion of units (%f) must be in the range (0,1]",
		   commandName, proportion);
  FOR_EACH_GROUP_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), {
    FOR_EACH_UNIT(G, if (randProb() < proportion) healUnit(U));
  });
  return signalNetStructureChanged();
}

int C_noiseType(TCL_CMDARGS) {
  flag mult = FALSE, flat = FALSE, rangeSet = FALSE;
  real range = NaN, (*proc)(real value, real range);
  int arg;
  double tempDbl;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  const char *usage = "noiseType <group-list> [-range <range> | -multiply | -flat]";
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc < 2) return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  for (arg = 2; arg < objc; arg++) {
    if (Tcl_GetStringFromObj(objv[arg], NULL)[0] != '-')
      return usageError(commandName, usage);
    switch (Tcl_GetStringFromObj(objv[arg], NULL)[1]) {
    case 'r':
      Tcl_GetDoubleFromObj(interp, objv[++arg], &tempDbl);
      range = (real)tempDbl;
      rangeSet = TRUE;
      break;
    case 'm':
      mult = TRUE;
      break;
    case 'f':
      flat = TRUE;
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

  if (flat)
    proc = (mult) ? multUniformNoise : addUniformNoise;
  else
    proc = (mult) ? multGaussianNoise : addGaussianNoise;

  FOR_EACH_GROUP_IN_LIST(Tcl_GetStringFromObj(objv[1], NULL), {
    if (rangeSet) G->noiseRange = range;
    G->noiseProc = proc;
  });

  return TCL_OK;
}

/****************************** Command Registration *************************/

void registerConnectCommands(void) {
  registerCommand((Tcl_ObjCmdProc *)C_connectGroups, "connectGroups",
		  "creates links with a specified pattern between groups");
  registerCommand((Tcl_ObjCmdProc *)C_connectGroupToUnits, "connectGroupToUnits",
		  "creates links from all units in a group to given units");
  registerCommand((Tcl_ObjCmdProc *)C_connectUnits, "connectUnits",
		  "creates a link from one unit to another");
  registerCommand((Tcl_ObjCmdProc *)C_elmanConnect, "elmanConnect",
		  "connects a source group to an ELMAN context group");
  registerCommand((Tcl_ObjCmdProc *)C_copyConnect, "copyConnect",
		  "connects a group to an IN_, OUT_, or TARGET_COPY group");
  registerCommand((Tcl_ObjCmdProc *)C_addLinkType, "addLinkType",
		  "creates a new link type");
  registerCommand((Tcl_ObjCmdProc *)C_deleteLinkType, "deleteLinkType",
		  "deletes a user-defined link type");
  registerCommand((Tcl_ObjCmdProc *)C_saveWeights, "saveWeights",
		  "saves the link weights and other values in a file");
  registerCommand((Tcl_ObjCmdProc *)C_loadWeights, "loadWeights",
		  "loads the link weights and other values from a file");
  registerCommand((Tcl_ObjCmdProc *)C_loadXerionWeights, "loadXerionWeights",
		  "loads weights from a Xerion text-format weight file");

  registerCommand((Tcl_ObjCmdProc *)NULL, "", "");
  registerCommand((Tcl_ObjCmdProc *)C_disconnectGroups, "disconnectGroups",
		  "deletes links of a specified type between two groups");
  registerCommand((Tcl_ObjCmdProc *)C_disconnectGroupUnit, "disconnectGroupUnit",
		  "deletes links from a group to a unit");
  registerCommand((Tcl_ObjCmdProc *)C_disconnectUnits, "disconnectUnits",
		  "deletes links of a specified type between two units");
  registerCommand((Tcl_ObjCmdProc *)C_deleteLinks, "deleteLinks",
		  "deletes all links of a specified type");
  registerCommand((Tcl_ObjCmdProc *)C_deleteGroupInputs, "deleteGroupInputs",
		  "deletes inputs to a group (including Elman inputs)");
  registerCommand((Tcl_ObjCmdProc *)C_deleteGroupOutputs, "deleteGroupOutputs",
		  "deletes outputs from a group (including Elman outputs)");
  registerCommand((Tcl_ObjCmdProc *)C_deleteUnitInputs, "deleteUnitInputs",
		  "deletes inputs of a specified type to a unit");
  registerCommand((Tcl_ObjCmdProc *)C_deleteUnitOutputs, "deleteUnitOutputs",
		  "deletes outputs of a specified type from a unit");

  registerCommand((Tcl_ObjCmdProc *)NULL, "", "");
  registerCommand((Tcl_ObjCmdProc *)C_randWeights, "randWeights",
		  "randomizes all of the weights of a selected type");
  registerCommand((Tcl_ObjCmdProc *)C_freezeWeights, "freezeWeights",
		  "stops weight updates on specified links");
  registerCommand((Tcl_ObjCmdProc *)C_thawWeights, "thawWeights",
		  "resumes weight updates on specified links");
  registerCommand((Tcl_ObjCmdProc *)C_setLinkValues, "setLinkValues",
		  "sets parameters for specified links");
  registerCommand((Tcl_ObjCmdProc *)C_printLinkValues, "printLinkValues",
		  "prints values for specified links to a file");

  registerCommand((Tcl_ObjCmdProc *)NULL, "", "");
  registerCommand((Tcl_ObjCmdProc *)C_lesionLinks, "lesionLinks",
		  "removes links or sets or adds noise to their weights");
  registerCommand((Tcl_ObjCmdProc *)C_lesionUnits, "lesionUnits",
		  "inactivates units");
  registerCommand((Tcl_ObjCmdProc *)C_healUnits, "healUnits",
		  "restores lesioned units");
  registerCommand((Tcl_ObjCmdProc *)C_noiseType, "noiseType",
		  "sets a group's noise parameters");
}
