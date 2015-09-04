#include <string.h>
#include "system.h"
#include "util.h"
#include "type.h"
#include "network.h"
#include "connect.h"
#include "act.h"
#include "control.h"
#include "display.h"
#include "train.h"

Network Net = NULL;       /* The currently active net */
struct rootrec root = {0, NULL, 0, NULL};
RootRec Root = &root;


/******************************* Finding Structures **************************/

/* Looks up a unit of the given name in the specified group. */
static Unit lookupUnitInGroup(char *name, Group G) {
  int len = strlen(G->name);

  if (!strncmp(G->name, name, len) && name[len] == ':') {
    int u = atoi(name + len + 1);
    if (u >= 0 && u < G->numUnits && !strcmp(G->unit[u].name, name))
      return G->unit + u;
  }
  FOR_EVERY_UNIT(G, if (!strcmp(U->name, name)) return U);
  return NULL;
}

/* Looks up a unit of the given name in the entire network. */
Unit lookupUnit(char *name) {
  Unit U;
  if (!Net) return NULL;
  FOR_EACH_GROUP(if ((U = lookupUnitInGroup(name, G))) return U);
  return NULL;
}

Unit lookupUnitNum(int g, int u) {
  Group G;
  if (g < 0 || g >= Net->numGroups) return NULL;
  G = Net->group[g];
  if (u < 0 || u >= G->numUnits) return NULL;
  return G->unit + u;
}

Group lookupGroup(char *name) {
  if (!Net) return NULL;
  FOR_EACH_GROUP(if (!strcmp(G->name, name)) return G);
  return NULL;
}

/* Looks up the network in the Root table.  Returns NULL if not found. */
Network lookupNet(char *name) {
  int n;
  for (n = 0; n < Root->numNetworks; n++)
    if (!strcmp(Root->net[n]->name, name)) return Root->net[n];
  return NULL;
}

/* Stores the network in the Root table. */
/* Assumes net with same name is not there already (if so, fatalError) */
static void registerNet(Network N) {
  Root->numNetworks++;
  Root->net = (Network *) safeRealloc(Root->net,
				      Root->numNetworks * sizeof(Network),
				      "registerNet:root->net");
  N->num = Root->numNetworks - 1;
  Root->net[N->num] = N;
  signalNetListChanged();
}

/* Removes the link to a network from the Root table. */
static void unregisterNet(Network N) {
  int n;
  Root->numNetworks--;
  for (n = N->num; n < Root->numNetworks; n++) {
    Root->net[n] = Root->net[n + 1];
    Root->net[n]->num = n;
  }
  if (Root->numNetworks == 0)
    FREE(Root->net);
  signalNetListChanged();
}

flag isGroupList(char *s) {
  String groupName = newString(32);
  flag code, val;
  if (!strcmp(s, "*")) return TRUE;
  readName(s, groupName, &code);
  if (code) val = FALSE;
  else val = (lookupGroup(groupName->s)) ? TRUE : FALSE;
  freeString(groupName);
  return val;
}

flag isUnitList(char *s) {
  String unitName = newString(32);
  flag code, val;
  if (!strcmp(s, "*")) return TRUE;
  readName(s, unitName, &code);
  if (code) val = FALSE;
  else val = (lookupUnit(unitName->s)) ? TRUE : FALSE;
  freeString(unitName);
  return val;
}


/******************************* Freeing Procedures **************************/

/* This doesn't remove the links coming out of a unit, only those going in
   and doesn't free the unit itself because it was allocated in a block. */
static void freeUnit(Unit U, flag all) {
  if (!U) return;
  if (all) {
    FREE(U->name);
    FREE(U->inputHistory);
    FREE(U->outputHistory);
    FREE(U->targetHistory);
    FREE(U->outputDerivHistory);
    freeUnitExtension(U);
  }
  if (!(U->group->net->type & OPTIMIZED)) {
    FREE(U->block);
    FREE(U->incoming);
    FREE(U->incoming2);
  }
}

static void freeGroupProc(GroupProc P) {
  FREE(P->groupHistoryData);
  FREE(P->unitData);
  FREE_MATRIX(P->unitHistoryData);
  FREE(P);
}

static void freeGroup(Group G, flag all) {
  GroupProc P, Q;
  if (!G) return;
  if (all) {
    FREE(G->name);
    for (P = G->inputProcs;  P; P = Q) {Q = P->next; freeGroupProc(P);}
    for (P = G->outputProcs; P; P = Q) {Q = P->next; freeGroupProc(P);}
    for (P = G->costProcs;   P; P = Q) {Q = P->next; freeGroupProc(P);}
    freeGroupExtension(G);
  }
  FOR_EVERY_UNIT(G, freeUnit(U, all));
  if (!(G->net->type & OPTIMIZED)) {
    FREE(G->output);
    FREE(G->unit);
    free(G);
  }
}

/* If all then everything is freed.  Otherwise, non-optimized things are
   spared. */
static void freeNet(Network N, flag all) {
  int g;
  if (!N) return;
  if (all) {
    FREE(N->name);
    FREE(N->eventHistory);
    FREE(N->resetHistory);
    if (N->preEpochProc)    Tcl_DecrRefCount(N->preEpochProc);
    if (N->postEpochProc)   Tcl_DecrRefCount(N->postEpochProc);
    if (N->postUpdateProc)  Tcl_DecrRefCount(N->postUpdateProc);
    if (N->preExampleProc)  Tcl_DecrRefCount(N->preExampleProc);
    if (N->postExampleProc) Tcl_DecrRefCount(N->postExampleProc);
    if (N->preExampleBackProc) Tcl_DecrRefCount(N->preExampleBackProc);
    if (N->preEventProc)    Tcl_DecrRefCount(N->preEventProc);
    if (N->postEventProc)   Tcl_DecrRefCount(N->postEventProc);
    if (N->preTickProc)     Tcl_DecrRefCount(N->preTickProc);
    if (N->postTickProc)    Tcl_DecrRefCount(N->postTickProc);
    if (N->preTickBackProc) Tcl_DecrRefCount(N->preTickBackProc);
    if (N->sigUSR1Proc)     Tcl_DecrRefCount(N->sigUSR1Proc);
    if (N->sigUSR2Proc)     Tcl_DecrRefCount(N->sigUSR2Proc);
    FREE(N->outputFileName);
    if (N->outputFile) closeChannel(N->outputFile);
    freeNetworkExtension(N);
  }
  for (g = 0; g < N->numGroups; g++)
    freeGroup(N->group[g], all);
  if (!(N->type & OPTIMIZED)) {
    FREE(N->group);
    FREE(N->input);
    FREE(N->output);
  }
  free(N);
}

/******************************* Building Units ******************************/

/* Find a name of the form group:n for a new unit that is not used anywhere
   else in the network.  It starts counting at G->numUnits for speed which
   means that names will not necessarily be consecutive and may be reused if a
   unit is deleted. */
static char *unusedUnitName(Group G) {
  String name = newString(64);
  char *new;
  int n = G->numUnits;
  do sprintf(name->s, "%s:%d", G->name, n++);
  while (lookupUnitInGroup(name->s, G) || lookupUnit(name->s));
  new = copyString(name->s);
  freeString(name);
  return new;
}

static void buildUnitHistories(Group G, Unit U) {
  FREE(U->inputHistory);
  FREE(U->outputHistory);
  FREE(U->targetHistory);
  FREE(U->outputDerivHistory);

  if (Net->historyLength <= 0) return;

  U->inputHistory = realArray(Net->historyLength,
			      "buildUnitHistories:U->inputHistory");
  U->outputHistory = realArray(Net->historyLength,
			       "buildUnitHistories:U->outputHistory");
  if (G->costType & ERROR_MASKS || G->type & USE_TARGET_HIST)
    U->targetHistory = realArray(Net->historyLength,
				 "buildUnitHistories:U->targetHistory");
  if (G->costType || G->type & USE_OUT_DERIV_HIST)
    U->outputDerivHistory = realArray(Net->historyLength,
            "buildUnitHistories:U->outputDerivHistory");
}

/* Assumes there is a current network.  The name is not copied here for
   complicated reasons, so don't pass this a string that is temporary or will
   be freed. */
static flag initUnit(Unit U, char *name, int num, Group G) {
  Group bias;
  int type;

  if (!name) name = unusedUnitName(G);

  /* The structure begins zeroed out except history arrays. */
  U->name  = name;
  U->num   = num;
  U->group = G;

  U->target        = DEF_U_target;
  U->externalInput = DEF_U_externalInput;
  U->gain          = chooseValue(G->gain, Net->gain);
  U->dtScale       = DEF_U_dtScale;

  /* If BIAS_NAME has been deleted from the link types, the type will be
     ALL_LINKS */
  type = lookupLinkType(BIAS_NAME);
  if ((G->type & BIASED) && (bias = lookupGroup(BIAS_NAME)))
    connectGroupToUnit(bias, U, type, NaN, NaN, FALSE);

  if (initUnitExtension(U)) return TCL_ERROR;
  return eval("catch {.initUnit group(%d).unit(%d)}", G->num, U->num);
}


/******************************* Building Groups *****************************/

static void fillOutputArray(void) {
  int i = 0;
  FOR_EACH_GROUP(if (G->type & OUTPUT) FOR_EVERY_UNIT(G, Net->output[i++] = U));
}

static void fillInputArray(void) {
  int i = 0;
  FOR_EACH_GROUP(if (G->type & INPUT) FOR_EVERY_UNIT(G, Net->input[i++] = U));
}

static void changeNumOutputs(int change) {
  Net->numOutputs += change;
  if (Net->numOutputs < 0)
    fatalError("Net->numOutputs became %d", Net->numOutputs);
  if (!(Net->type & OPTIMIZED)) FREE(Net->output);
  if (Net->numOutputs) {
    Net->output = (Unit *) safeMalloc(Net->numOutputs * sizeof(Unit),
				      "changeNumOutputs:Net->output");
    fillOutputArray();
  }
}

static void changeNumInputs(int change) {
  Net->numInputs += change;
  if (Net->numInputs < 0)
    fatalError("Net->numInputs became %d", Net->numInputs);
  if (!(Net->type & OPTIMIZED)) FREE(Net->input);
  if (Net->numInputs) {
    Net->input = (Unit *) safeMalloc(Net->numInputs * sizeof(Unit),
				     "changeNumInputs:Net->input");
    fillInputArray();
  }
}

static void appendGroupProc(GroupProc *L, mask *typeMask,
			    unsigned int class, mask type) {
  GroupProc P = (GroupProc) safeCalloc(1, sizeof(*P), "appendGroupProc:P");
  P->type = type;
  P->class = class;
  if (*L) {
    P->prev = (*L)->prev;
    P->prev->next = P;
    (*L)->prev = P;
  } else {
    *L = P;
    P->prev = P;
  }
  *typeMask |= type;
}

void appendProc(Group G, unsigned int class, mask type) {
  switch (class) {
  case GROUP_INPUT:
    appendGroupProc(&G->inputProcs, &G->inputType, class, type);
    break;
  case GROUP_OUTPUT:
    appendGroupProc(&G->outputProcs, &G->outputType, class, type);
    break;
  case GROUP_COST:
    appendGroupProc(&G->costProcs, &G->costType, class, type);
    break;
  default: fatalError("appendProc: bad class %x", class);
  }
}

static void prependGroupProc(GroupProc *L, mask *typeMask,
			     unsigned int class, mask type) {
  GroupProc P = (GroupProc) safeCalloc(1, sizeof(*P), "prependGroupProc:P");
  P->type = type;
  P->class = class;
  if ((P->next = *L)) {
    P->prev = P->next->prev;
    P->next->prev = P;
  } else P->prev = P;
  *L = P;
  *typeMask |= type;
}

void prependProc(Group G, unsigned int class, mask type) {
  switch (class) {
  case GROUP_INPUT:
    prependGroupProc(&G->inputProcs, &G->inputType, class, type);
    break;
  case GROUP_OUTPUT:
    prependGroupProc(&G->outputProcs, &G->outputType, class, type);
    break;
  case GROUP_COST:
    prependGroupProc(&G->costProcs, &G->costType, class, type);
    break;
  default: fatalError("prependProc: bad class %x", class);
  }
}

/* This is silent if the proc doesn't exist. */
static void removeGroupProc(GroupProc *L, mask type) {
  GroupProc P;
  for (P = *L; P && P->type != type; P = P->next);
  if (P) {
    if (P == *L) *L = P->next;
    else {
      P->prev->next = P->next;
      if (P == (*L)->prev) (*L)->prev = P->prev;
    }
    if (P->next) P->next->prev = P->prev;

    freeGroupProc(P);
  }
}

/* This removes only the first one that matches. */
void removeProc(Group G, unsigned int class, mask type) {
  switch (class) {
  case GROUP_INPUT:
    removeGroupProc(&(G->inputProcs), type);
    break;
  case GROUP_OUTPUT:
    removeGroupProc(&(G->outputProcs), type);
    break;
  case GROUP_COST:
    removeGroupProc(&(G->costProcs), type);
    break;
  default: fatalError("prependProc: bad class %x", class);
  }
}

flag finalizeGroupType(Group G, flag wasOutput, flag wasInput) {
  if (initGroupTypes(G)) return TCL_ERROR;

  if (wasOutput) {
    if (!(G->type & OUTPUT))   changeNumOutputs(-G->numUnits);
  } else if (G->type & OUTPUT) changeNumOutputs(G->numUnits);
  if (wasInput) {
    if (!(G->type & INPUT))    changeNumInputs(-G->numUnits);
  } else if (G->type & INPUT)  changeNumInputs(G->numUnits);

  FOR_EVERY_UNIT(G, buildUnitHistories(G, U));
  return TCL_OK;
}

void changeGroupType(Group G, unsigned int class, mask type,
		     unsigned int mode) {
  if (mode == TOGGLE_TYPE)
    mode = (GROUP_HAS_TYPE(G, class, type)) ? REMOVE_TYPE : ADD_TYPE;
  switch (mode) {
  case ADD_TYPE:
    if (class == GROUP) G->type |= type;
    else appendProc(G, class, type);
    break;
  case REMOVE_TYPE:
    if (class == GROUP) G->type &= ~type;
    else removeProc(G, class, type);
    break;
  default:
    fatalError("bad type mode: %d", mode);
  }
}

flag setGroupType(Group G, int numTypes, unsigned int *typeClass, mask *type,
		  unsigned int *typeMode) {
  int i;
  flag wasOutput = G->type & OUTPUT, wasInput  = G->type & INPUT;

  G->type = G->type & FROZEN;
  while (G->inputProcs)
    removeGroupProc(&G->inputProcs, G->inputProcs->type);
  while (G->outputProcs)
    removeGroupProc(&G->outputProcs, G->outputProcs->type);
  while (G->costProcs)
    removeGroupProc(&G->costProcs, G->costProcs->type);
  G->inputType = G->outputType = G->costType = 0;

  for (i = 0; i < numTypes; i++) {
    if (typeMode[i] != TOGGLE_TYPE) continue;
    changeGroupType(G, typeClass[i], type[i], ADD_TYPE);
  }
  cleanupGroupType(G);
  for (i = 0; i < numTypes; i++)
    if (typeMode[i] != TOGGLE_TYPE)
      changeGroupType(G, typeClass[i], type[i], typeMode[i]);

  return finalizeGroupType(G, wasOutput, wasInput);
}

flag addGroup(char *name, int numTypes, unsigned int *typeClass, mask *type,
	      unsigned int *typeMode, int numUnits) {
  Group G;

  if (!Net)
    return warning("Cannot add group \"%s\", no current network", name);
  if (numUnits <= 0) warning("Cannot create group \"%s\" with %d units",
			     name, numUnits);

  if ((G = lookupGroup(name))) {
    if (!queryUser("Group \"%s\" already exists, replace it?", name))
      return result("addGroup: group \"%s\" not replaced", name);
    deleteGroup(G);
  }

  Net->numGroups++;
  Net->group = safeRealloc(Net->group, Net->numGroups * sizeof(Group),
			   "addGroup:Net->group");
  G = safeCalloc(1, sizeof(struct group), "addGroup:G");
  Net->group[Net->numGroups - 1] = G;
  G->name = copyString(name);
  G->num = Net->numGroups - 1;
  G->net = Net;

  /* Default Variable Values */
  G->trainGroupCrit     = DEF_G_trainGroupCrit;
  G->testGroupCrit      = DEF_G_testGroupCrit;

  G->learningRate       = DEF_G_learningRate;
  G->momentum           = DEF_G_momentum;
  G->weightDecay        = DEF_G_weightDecay;
  G->gainDecay          = DEF_G_gainDecay;
  G->outputCostScale    = DEF_G_outputCostScale;
  G->outputCostPeak     = DEF_G_outputCostPeak;
  G->targetRadius       = DEF_G_targetRadius;
  G->zeroErrorRadius    = DEF_G_zeroErrorRadius;
  G->errorScale         = DEF_G_errorScale;

  G->dtScale            = DEF_G_dtScale;
  G->gain               = DEF_G_gain;
  G->ternaryShift       = DEF_G_ternaryShift;
  G->clampStrength      = DEF_G_clampStrength;
  G->initInput          = DEF_G_initInput;

  G->randMean           = DEF_G_randMean;
  G->randRange          = DEF_G_randRange;
  G->noiseRange         = DEF_G_noiseRange;
  G->noiseProc          = DEF_G_noiseProc;

  G->showIncoming       = DEF_G_showIncoming;
  G->showOutgoing       = DEF_G_showOutgoing;
  G->inPosition = G->outPosition = 0;
  G->numColumns         = DEF_G_numColumns;
  G->neighborhood       = DEF_G_neighborhood;
  G->periodicBoundary   = DEF_G_periodicBoundary;

  /* Constructing arrays and units */
  G->numUnits = numUnits;
  Net->numUnits += numUnits;
  G->unit = safeCalloc(numUnits, sizeof(struct unit), "addGroup:G->unit");
  G->output = realArray(2 * numUnits, "addGroup:G->output");
  G->outputDeriv = G->output + numUnits;

  if (setGroupType(G, numTypes, typeClass, type, typeMode)) return TCL_ERROR;

  FOR_EVERY_UNIT(G, {
    sprintf(Buffer, "%s:%d", G->name, u);
    if (initUnit(U, copyString(Buffer), u, G)) return TCL_ERROR;
  });

  if (initGroupExtension(G)) return TCL_ERROR;
  return eval("catch {.initGroup group(%d)}", G->num);
}


/***************************** Building Networks *****************************/

flag useNet(Network N) {
  flag change = (Net != N) ? TRUE : FALSE;
  Net = N;
  if (change) signalCurrentNetChanged();
  return TCL_OK;
}

flag addNet(char *name, mask type, int timeIntervals,
	    int ticksPerInterval) {
  Network N;
  unsigned int class, mode;

  if ((N = lookupNet(name))) {
    if (!queryUser("Network \"%s\" already exists, replace it?", name))
      return warning("addNet: network \"%s\" not replaced", name);
    deleteNet(N);
  }

  N = (Network) safeCalloc(1, sizeof *N, "addNet:N");
  N->name = copyString(name);
  N->type = type;
  N->root = Root;
  initNetTypes(N);

  /* Default Variable Values */
  N->backpropTicks   = DEF_N_backpropTicks;

  N->numUpdates      = DEF_N_numUpdates;
  N->batchSize       = DEF_N_batchSize;
  N->reportInterval  = DEF_N_reportInterval;
  N->criterion       = DEF_N_criterion;
  N->trainGroupCrit  = DEF_N_trainGroupCrit;
  N->testGroupCrit   = DEF_N_testGroupCrit;
  N->groupCritRequired = DEF_N_groupCritRequired;
  N->minCritBatches  = DEF_N_minCritBatches;
  N->pseudoExampleFreq = DEF_N_pseudoExampleFreq;

  N->algorithm       = DEF_N_algorithm;
  N->learningRate    = DEF_N_learningRate;
#ifdef ADVANCED
  N->rateIncrement   = DEF_N_rateIncrement;
  N->rateDecrement   = DEF_N_rateDecrement;
#endif /* ADVANCED */
  N->momentum        = DEF_N_momentum;
  N->adaptiveGainRate = DEF_N_adaptiveGainRate;
  N->weightDecay     = DEF_N_weightDecay;
  N->gainDecay       = DEF_N_gainDecay;
  N->outputCostStrength = DEF_N_outputCostStrength;
  N->outputCostPeak  = DEF_N_outputCostPeak;
  N->targetRadius    = DEF_N_targetRadius;
  N->zeroErrorRadius = DEF_N_zeroErrorRadius;

  N->gain            = DEF_N_gain;
  N->ternaryShift    = DEF_N_ternaryShift;
  N->clampStrength   = DEF_N_clampStrength;
  N->initOutput      = DEF_N_initOutput;
  N->initInput       = DEF_N_initInput;
  N->initGain        = DEF_N_initGain;
  N->finalGain       = DEF_N_finalGain;
  N->annealTime      = DEF_N_annealTime;

  N->randMean        = DEF_N_randMean;
  N->randRange       = DEF_N_randRange;
  N->noiseRange      = DEF_N_noiseRange;

  N->autoPlot        = DEF_N_autoPlot;
  N->plotCols        = DEF_N_plotCols;
  N->unitCellSize    = DEF_N_unitCellSize;
  N->unitCellSpacing = DEF_N_unitCellSpacing;
  N->unitDisplayValue = DEF_N_unitDisplayValue;
  N->unitDisplaySet  = DEF_N_unitDisplaySet;
  N->unitUpdates     = DEF_N_unitUpdates;
  N->unitTemp        = DEF_N_unitTemp;
  N->unitPalette     = DEF_N_unitPalette;
  if (N->type & BOLTZMANN)
    N->unitExampleProc = DEF_N_boltzUnitExampleProc;
  else
    N->unitExampleProc = DEF_N_unitExampleProc;

  N->linkCellSize    = DEF_N_linkCellSize;
  N->linkCellSpacing = DEF_N_linkCellSpacing;
  N->linkDisplayValue = DEF_N_linkDisplayValue;
  N->linkUpdates     = DEF_N_linkUpdates;
  N->linkTemp        = DEF_N_linkTemp;
  N->linkPalette     = DEF_N_linkPalette;

  N->writeExample    = DEF_N_writeExample;

  registerNet(N);      /* Put it in the Root table */
  useNet(N);

  if (setTime(N, timeIntervals, ticksPerInterval, -1, TRUE))
    return TCL_ERROR;

  /* This must be after useNet */
  type = BIAS; class = GROUP; mode = TOGGLE_TYPE;
  if (addGroup(BIAS_NAME, 1, &class, &type, &mode, 1))
    return TCL_ERROR;

  eval("setObj sigUSR1Proc $defaultSigUSR1Proc");
  eval("setObj sigUSR2Proc $defaultSigUSR2Proc");

  if (initNetworkExtension(N)) return TCL_ERROR;
  return eval("catch {.initNetwork}");
}

flag setTime(Network N, int timeIntervals, int ticksPerInterval,
	     int historyLength, flag setDT) {
  int maxTicks;
  if (timeIntervals < 1)
    return warning("setTime: timeIntervals must be positive");
  N->timeIntervals = timeIntervals;

  if (ticksPerInterval < 1)
    return warning("setTime: ticksPerInterval must be positive");

  N->ticksPerInterval = ticksPerInterval;
  if (setDT) N->dt = (real) 1.0 / ticksPerInterval;
  maxTicks = timeIntervals * ticksPerInterval;
  if (N->type & CONTINUOUS) maxTicks++;
  if (maxTicks != N->maxTicks) {
    N->maxTicks = maxTicks;
    FREE(N->eventHistory);
    N->eventHistory = intArray(N->maxTicks, "setTime:N->eventHistory");
  }

  if (historyLength < 0) historyLength = N->maxTicks;
  if (N->historyLength == historyLength) return TCL_OK;
  N->historyLength = historyLength;
  FREE(N->resetHistory);
  N->resetHistory = (flag *) intArray(N->historyLength,
				      "setHistoryLength:N->resetHistory");
  FOR_EACH_GROUP({
    FOR_EVERY_UNIT(G, buildUnitHistories(G, U));
    if (initGroupTypes(G)) return TCL_ERROR;
  });
  return TCL_OK;
}

flag orderGroups(int argc, char *argv[]) {
  flag changed = 0;
  int g;
  Group G, temp;

  if (argc > Net->numGroups)
    return warning("orderGroups: invalid number of groups: %d", argc);
  for (g = 0; g < argc; g++) {
    if (!(G = lookupGroup(argv[g])))
      return warning("orderGroups: group \"%s\" doesn't exist", argv[g]);
    if (G->num < g)
      return warning("orderGroups: group \"%s\" was listed twice", argv[g]);
    if (G->num != g) {
      temp = Net->group[g];
      Net->group[g] = Net->group[G->num];
      Net->group[G->num] = temp;
      temp->num = G->num;
      G->num = g;
      changed = 1;
    }
  }
  if (changed) {
    fillOutputArray();
    fillInputArray();
  }
  return TCL_OK;
}

flag orderGroupsObj(int objc, Tcl_Obj *objv[]) {
  flag changed = 0;
  int g;
  Group G, temp;

  if (objc > Net->numGroups)
    return warning("orderGroups: invalid number of groups: %d", objc);
  for (g = 0; g < objc; g++) {
    if (!(G = lookupGroup(Tcl_GetStringFromObj(objv[g], NULL))))
      return warning("orderGroups: group \"%s\" doesn't exist", Tcl_GetStringFromObj(objv[g], NULL));
    if (G->num < g)
      return warning("orderGroups: group \"%s\" was listed twice", Tcl_GetStringFromObj(objv[g], NULL));
    if (G->num != g) {
      temp = Net->group[g];
      Net->group[g] = Net->group[G->num];
      Net->group[G->num] = temp;
      temp->num = G->num;
      G->num = g;
      changed = 1;
    }
  }
  if (changed) {
    fillOutputArray();
    fillInputArray();
  }
  return TCL_OK;
}

/******************************** Deleting Stuff *****************************/

flag deleteGroup(Group G) {
  int g;
  deleteGroupOutputs(G, ALL_LINKS);
  deleteGroupInputs(G, ALL_LINKS);
  Net->numUnits -= G->numUnits;
  Net->numGroups--;
  if (G->type & OUTPUT) changeNumOutputs(-G->numUnits);
  if (G->type & INPUT)  changeNumInputs(-G->numUnits);

  /* Shift any following groups, don't bother reallocing */
  if (G->num != Net->numGroups) {
    memmove(Net->group + G->num, Net->group + G->num + 1,
	    (Net->numGroups - G->num) * sizeof(Group));
    /* renumber the remaining groups */
    for (g = G->num; g < Net->numGroups; g++)
      Net->group[g]->num = g;
  }
  freeGroup(G, TRUE);
  if (Net->numGroups == 0) FREE(Net->group);

  return TCL_OK;
}

flag deleteNet(Network N) {
  unregisterNet(N);
  freeNet(N, TRUE);
  if (Net == N) useNet(NULL);
  return TCL_OK;
}

flag deleteAllNets(void) {
  int n;
  for (n = 0; n < Root->numNetworks; n++)
    freeNet(Root->net[n], TRUE);
  FREE(Root->net);
  Root->numNetworks = 0;
  useNet(NULL);
  signalNetListChanged();
  return TCL_OK;
}


/************************************* Resetting *****************************/

flag copyUnitValues(Group from, int fromoff, Group to, int tooff) {
  int i;
  if (from->numUnits != to->numUnits)
    return warning("copyUnitValues: group \"%s\" and group \"%s\" aren't "
                   "the same size", from, to);
  for (i = 0; i < from->numUnits; i++)
    *(real *) ((char *) (to->unit + i) + tooff) =
      *(real *) ((char *) (from->unit + i) + fromoff);
  return TCL_OK;
}

flag setUnitValues(Group G, int offset, real value) {
  int i;
  for (i = 0; i < G->numUnits; i++)
    *(real *) ((char *) (G->unit + i) + offset) = value;
  return TCL_OK;
}

static void resetUnitValues(Unit U) {
  int size = Net->historyLength * sizeof(real);
  U->output = chooseValue(U->group->initOutput, Net->initOutput);
  U->input  = chooseValue(U->group->initInput,  Net->initInput);
  U->outputDeriv = U->inputDeriv = U->gainDeriv = 0.0;
  U->externalInput = U->target = NaN;
  U->gain = chooseValue(U->group->gain, Net->gain);

  if (U->inputHistory)
    memset(U->inputHistory, 0, size);
  if (U->outputHistory)
    memset(U->outputHistory, 0, size);
  if (U->targetHistory)
    memset(U->targetHistory, 0, size);
  if (U->outputDerivHistory)
    memset(U->outputDerivHistory, 0, size);
}

flag standardResetNet(flag randomize) {
  FOR_EACH_GROUP({
    FOR_EACH_UNIT(G, {
      resetUnitValues(U);
      FOR_EACH_LINK(U, initLinkValues(L, getLink2(U, L)));
    });
    resetForwardIntegrators(G);
    resetBackwardIntegrators(G);
    cacheOutputs(G);
    cacheOutputDerivs(G);
    G->error = 0.0;
  });

  if (randomize) randomizeWeights(NaN, NaN, ALL_LINKS, FALSE);

  Net->totalUpdates = 0;
  Net->error = 0.0;
  Net->weightCost = 0.0;
  Net->gradientLinearity = 0.0;
  Net->currentExample = NULL;
  Net->exampleHistoryStart = 0;
  Net->ticksOnExample = 0;
  Net->currentTick = 0;

  if (Net->ext) return resetNetworkExtension(Net);
  return TCL_OK;
}

flag resetDerivs(void) {
  if (!Net) return TCL_ERROR;
  FOR_EACH_GROUP({
    FOR_EACH_UNIT(G, {
      U->outputDeriv = U->inputDeriv = U->gainDeriv = 0.0;
      FOR_EACH_LINK(U, L->deriv = 0.0);
    });
  });
  return TCL_OK;
}

/*********************************** Unit Lesioning **************************/

static void fillNaN(real *a, int len) {
  int i;
  if (!a) return;
  for (i = 0; i < len; i++)
    a[i] = NaN;
}

flag lesionUnit(Unit U) {
  Group G = U->group;
  U->output = U->outputDeriv = 0;
  G->output[U->num] = G->outputDeriv[U->num] = 0;
  U->input = U->inputDeriv = U->target = U->adjustedTarget = NaN;
  if (U->inputHistory)
    fillNaN(U->inputHistory, Net->historyLength);
  if (U->outputHistory)
    fillNaN(U->outputHistory, Net->historyLength);
  if (U->targetHistory)
    fillNaN(U->targetHistory, Net->historyLength);
  if (U->outputDerivHistory)
    fillNaN(U->outputDerivHistory, Net->historyLength);
  U->type |= LESIONED;
  G->type |= LESIONED;
  return TCL_OK;
}

flag healUnit(Unit V) {
  flag allHealthy = TRUE;
  V->type &= ~LESIONED;
  FOR_EVERY_UNIT(V->group, {
    if (U->type & LESIONED) {allHealthy = FALSE; break;}
  });
  if (allHealthy) V->group->type &= ~LESIONED;
  return TCL_OK;
}

#ifdef JUNK
flag printGroupUnitValues(Tcl_Channel channel, Group G, int *field,
			  char **format, int numFields) {
  int i;
  char format[32];
  FOR_EACH_UNIT(G, {
    for (i = 0; i < numFields, i++) {
      sprintf(format, "%%%s", format[i]);
      switch (field[i]) {
      case 1:
	strcat(format, "d"); sprintf(Buffer, format, G->num); break;
      case 2:
	strcat(format, "s"); sprintf(Buffer, format, G->name); break;
      case 3:
	strcat(format, "d"); sprintf(Buffer, format, U->num); break;
      case 4:
	strcat(format, "s"); sprintf(Buffer, format, U->name); break;
      case 5:
	strcat(format, ""); sprintf(Buffer, format, Net->currentExample->num);
	break;
      case 6:
	strcat(format, ""); sprintf(Buffer, format, Net->currentExample->name);
	break;
      case 7:
	strcat(format, ""); sprintf(Buffer, format, Net->currentTick); break;
      case 8:
	strcat(format, ""); sprintf(Buffer, format, ); break;
      case 9:
	strcat(format, ""); sprintf(Buffer, format, ); break;
      case 10:
	strcat(format, ""); sprintf(Buffer, format, ); break;
      case 11:
	strcat(format, ""); sprintf(Buffer, format, ); break;
      case 12:
	strcat(format, ""); sprintf(Buffer, format, ); break;
      case 13:
	strcat(format, ""); sprintf(Buffer, format, ); break;
      default: return warning("bad field: %d", field[i]);
      }
    }
    if (i < numFields - 1) cprintf(channel, " ");
  });
  return TCL_OK;
}
#endif /* JUNK */

/******************************** Optimization *******************************/

/* This creates a duplicate network in a single block of memory and replaces
   the original.  Extension fields are left as they were. */

static int unitSize(Unit U) {
  return U->numBlocks * sizeof(struct block) +
    U->numIncoming * (sizeof(struct link) + sizeof(struct link2));
}

static int groupSize(Group G) {
  int size = sizeof(struct group);
  size += 2 * G->numUnits * sizeof(real);
  size += G->numUnits * sizeof(struct unit);
  FOR_EACH_UNIT(G, size += unitSize(U));
  return size;
}

static int netSize(void) {
  int size = sizeof(struct network);
  size += Net->numGroups * sizeof(Group);
  size += Net->numInputs * sizeof(Unit);
  size += Net->numOutputs * sizeof(Unit);
  FOR_EACH_GROUP(size += groupSize(G));
  return size;
}

void *duplicate(void *data, int size, int command) {
  static char *chunk;
  static int chunkSize;
  switch (command) {
  case 0:
    chunk = data;
    chunkSize = size;
    break;
  case 1:
    if (size == 0) return NULL;
    if (size > chunkSize) fatalError("optimize failed");
    if (data) memcpy(chunk, data, size);
    chunk += size;
    chunkSize -= size;
    return chunk - size;
  case 2:
    if (chunkSize != 0) print(1, "%d remaining", chunkSize);
    break;
  default: fatalError("duplicate called with %d", command);
  }
  return NULL;
}

Group duplicateGroup(Group H, Network New) {
  GroupProc P;
  Group G = (Group) duplicate(H, sizeof(struct group), 1);
  G->net  = New;
  G->unit = duplicate(G->unit, G->numUnits * sizeof(struct unit), 1);
  for (P = G->inputProcs;  P; P = P->next) P->group = G;
  for (P = G->outputProcs; P; P = P->next) {
    P->group = G;
    if (P->type & ELMAN_CLAMP && P->otherData)
      P->otherData = (void *) New->group[((Group) P->otherData)->num];
  }
  for (P = G->costProcs;   P; P = P->next) P->group = G;
  return G;
}

void duplicateCaches(Group G) {
  G->output = duplicate(G->output, 2 * G->numUnits * sizeof(real), 1);
  G->outputDeriv = G->output + G->numUnits;
}

void duplicateLinks(Group G) {
  FOR_EVERY_UNIT(G, {
    U->group = G;
    U->block = duplicate(U->block, U->numBlocks * sizeof(struct block), 1);
    U->incoming = duplicate(U->incoming, U->numIncoming *
			    sizeof(struct link), 1);
  });
}

void fixLinks(Group G) {
  Network N = G->net;
  FOR_EVERY_UNIT(G, {
    U->incoming2 = duplicate(U->incoming2,
			     U->numIncoming * sizeof(struct link2), 1);
    FOR_EACH_BLOCK(U, {
      Group H = N->group[B->unit->group->num];
      B->unit   = H->unit + B->unit->num;
      B->output = H->output + B->unit->num;
    });
  });
}

Network duplicateNet(void) {
  int i;
  Network New = (Network) duplicate(Net, sizeof(struct network), 1);
  New->group  = duplicate(Net->group, New->numGroups * sizeof(Group), 1);
  for (i = 0; i < New->numGroups; i++)
    New->group[i] = duplicateGroup(New->group[i], New);
  for (i = 0; i < New->numGroups; i++) {
    duplicateLinks(New->group[i]);
    duplicateCaches(New->group[i]);
  }
  for (i = 0; i < New->numGroups; i++)
    fixLinks(New->group[i]);
  New->input  = duplicate(Net->input, New->numInputs * sizeof(Unit), 1);
  for (i = 0; i < New->numInputs; i++)
    New->input[i] = New->group[New->input[i]->group->num]->unit +
      New->input[i]->num;
  New->output = duplicate(Net->output, New->numOutputs * sizeof(Unit), 1);
  for (i = 0; i < New->numOutputs; i++)
    New->output[i] = New->group[New->output[i]->group->num]->unit +
      New->output[i]->num;

  return New;
}

flag optimizeNet(void) {
  char *chunk;
  Network New;
  int size = netSize();
  if (!(chunk = malloc(size)))
    return warning("not enough memory to optimize the network");
  duplicate(chunk, size, 0);
  New = duplicateNet();
  duplicate(NULL, 0, 2);
  New->type |= OPTIMIZED;

  Root->net[Net->num] = New;
  freeNet(Net, FALSE);
  useNet(New);
  return result("%d", size);
}
