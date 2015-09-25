#include <stdio.h>
#include <string.h>
#include "system.h"
#include "util.h"
#include "network.h"
#include "connect.h"
#include "type.h"
#include "train.h"
#include "act.h"

typedef struct type *Type;
struct type {
  char *name;
  mask mask;
  unsigned int class;
  void *data;
  Type next;
};

Type TypeTable = NULL;
char *typeClassName[] = {"NETWORK", "GROUP", "GROUP_INPUT", "GROUP_OUTPUT",
			 "GROUP_COST", "UNIT", "LINK", "PROJECTION", 
			 "EXAMPLE_MODE", "ALGORITHM", "TASK", "UPDATE_SIGNAL"};

/* This stores the bit mask for the named type from the given class into 
 *result.  It matches as long as the class is the same and the typeName 
 query is a prefix of the type name.  If the query is not unique it will match
 the alphabetically first type. */
flag lookupTypeMask(char *typeName, unsigned int typeClass, mask *result) {
  int val, len = strlen(typeName);
  Type T;
  for (T = TypeTable, val = -1; T && 
	 (((val = strncmp(T->name, typeName, len)) < 0) ||
	  (val == 0 && T->class < typeClass)); T = T->next);
  if (T && T->class == typeClass && val == 0) {
    *result = T->mask;
    return TCL_OK;
  }
  return TCL_ERROR;
}

/* This returns the type mask and class given its name. */
flag lookupType(char *typeName, unsigned int *typeClass, mask *type) {
  int val, len = strlen(typeName);
  Type T;
  for (T = TypeTable, val = -1; T && 
	 (val = strncmp(T->name, typeName, len)) < 0; T = T->next);
  if (T && val == 0) {
    *type = T->mask;
    *typeClass = T->class;
    return TCL_OK;
  }
  return TCL_ERROR;
}

flag lookupGroupType(char *typeName, unsigned int *typeClass, mask *type) {
  int class;
  if (lookupType(typeName, typeClass, type) ||
      ((class = *typeClass) != GROUP &&
       class != GROUP_INPUT &&
       class != GROUP_OUTPUT &&
       class != GROUP_COST)) {
    warning("Invalid group type: \"%s\"\n"
	    "Run \"groupType\" for a list of valid group types.", typeName);
    /* printGroupTypes(); */
    return TCL_ERROR;
  }
  return TCL_OK;
}

/* This returns the name of the type given its mask and class */
const char *lookupTypeName(mask typeMask, unsigned int typeClass) {
  Type T;
  for (T = TypeTable; T && (T->class != typeClass || T->mask != typeMask);
       T = T->next);
  if (T) return T->name;
  else return NULL;
}

static flag setTypeData(mask typeMask, unsigned int typeClass, void *data) {
  Type T;
  for (T = TypeTable; T && (T->class != typeClass || T->mask != typeMask);
       T = T->next);
  if (T) {
    T->data = data;
    return TCL_OK;
  }
  else return TCL_ERROR;
}

static void *getTypeData(mask typeMask, unsigned int typeClass) {
  Type T;
  for (T = TypeTable; T && (T->class != typeClass || T->mask != typeMask);
       T = T->next);
  if (T) return T->data;
  else return NULL;
}

/* This keeps them sorted by name alphabetically and then class if there is
   conflict. */
flag registerType(char *typeName, mask typeMask, unsigned int typeClass) {
  int val;
  Type *Tp, new;
  if (typeClass >= MAX_TYPE_CLASS)
    return warning("Type class %d is out of bounds", typeClass);
  if (lookupTypeName(typeMask, typeClass))
    return warning("A type with mask %d and class %s already exists, "
		   "can't register \"%s\".", 
		   typeMask, typeClassName[typeClass], typeName);
  for (Tp = &TypeTable, val = -1; *Tp && 
	 (((val = strcmp((*Tp)->name, typeName)) < 0) ||
	  (val == 0 && (*Tp)->class < typeClass)); Tp = &((*Tp)->next));
  if (*Tp && val == 0 && (*Tp)->class == typeClass)
    return warning("A type with name \"%s\" and class %d already exists.", 
		   typeName);
  new = (Type) safeCalloc(1, sizeof *new, "registerType:T");
  new->name = copyString(typeName);
  new->mask = typeMask;
  new->class = typeClass;
  new->next = *Tp;
  *Tp = new;
  return TCL_OK;
}

/* This prints all types of the given class whose bit is in the filter mask.
   It prints nothing if there are none. */
void printTypes(char *label, unsigned int class, mask filter) {
  Type T;
  flag found = FALSE;
  for (T = TypeTable; T && !found; T = T->next)
    if (T->class == class && (T->mask & filter))
      found = TRUE;
  if (!found) return;
  
  append("%s\n", label);
  for (T = TypeTable; T; T = T->next)
    if (T->class == class && (T->mask & filter))
      append("  %s\n", T->name);
}


/******************************** Network Types ******************************/

void registerNetType(char *name, mask type, flag (*initNetType)(Network N)) {
  registerType(name, type, NETWORK);
  setTypeData(type, NETWORK, (void *) initNetType);
}

flag initNetTypes(Network N) {
  int i;
  void *init;
  initStandardNet(N);
  for (i = 0; i < 32; i++) {
    if ((N->type & (1 << i)) && (init = getTypeData((1 << i), NETWORK)))
      ((flag (*)(Network N)) init)(N);
  }
  return TCL_OK;
}

/********************************* Group Types *******************************/

void registerGroupType(char *name, mask type, unsigned int class, 
		       void (*initGroupType)(Group G, GroupProc P)) {
  registerType(name, type, class);
  setTypeData(type, class, (void *) initGroupType);
}

/* This cleans up everything but the type and otherData. */
static void initializeGroupProc(Group G, GroupProc P) {
  P->group = G;
  P->forwardProc = P->backwardProc = NULL;
  FREE(P->groupHistoryData);
  FREE(P->unitData);
  FREE_MATRIX(P->unitHistoryData);
}

flag initGroupTypes(Group G) {
  int i;
  void *init;
  GroupProc P;
  for (i = 0; i < 32; i++) {
    if ((G->type & (1 << i)) && (init = getTypeData((1 << i), GROUP)))
      ((void (*)(Group G, GroupProc P)) init)(G, NULL);
  }
  for (P = G->inputProcs; P; P = P->next) {
    initializeGroupProc(G, P);
    if ((init = getTypeData(P->type, GROUP_INPUT)))
      ((void (*)(Group G, GroupProc P)) init)(G, P);
  }
  for (P = G->outputProcs; P; P = P->next) {
    initializeGroupProc(G, P);
    if ((init = getTypeData(P->type, GROUP_OUTPUT)))
      ((void (*)(Group G, GroupProc P)) init)(G, P);
  }
  for (P = G->costProcs; P; P = P->next) {
    initializeGroupProc(G, P);
    if ((init = getTypeData(P->type, GROUP_COST)))
      ((void (*)(Group G, GroupProc P)) init)(G, P);
  }
  return TCL_OK;
}

void printGroupType(void *V) {
  Group G = (Group) V;
  GroupProc P;
  const char *name;
  
  printTypes("Base Type", GROUP, G->type & FIXED_MASKS);
  if (G->inputProcs) {
    append("Input Type\n");
    for (P = G->inputProcs; P; P = P->next) {
      if (!(name = lookupTypeName(P->type, GROUP_INPUT)))
	error("unknown input type: %d\n", P->type);
      append("  %s\n", name);
    }
  }
  if (G->outputProcs) {
    append("Output Type\n");
    for (P = G->outputProcs; P; P = P->next) {
      if (!(name = lookupTypeName(P->type, GROUP_OUTPUT)))
	error("unknown output type: %d\n", P->type);
      append("  %s\n", name);
    }
  }
  if (G->costProcs) {
    append("Cost Type\n");
    for (P = G->costProcs; P; P = P->next) {
      if (!(name = lookupTypeName(P->type, GROUP_COST)))
	error("unknown cost type: %d\n", P->type);
      append("  %s\n", name);
    }
  }
  printTypes("Group Criterion Type", GROUP, G->type & CRITERION_MASKS);
  printTypes("History Types", GROUP, G->type & USE_HIST_MASKS);
  printTypes("Other Types", GROUP, G->type &
	     ~(FIXED_MASKS | CRITERION_MASKS | USE_HIST_MASKS));
}

void printGroupTypes(void) {
  printTypes("Base Types", GROUP, FIXED_MASKS);
  printTypes("Input Types", GROUP_INPUT, ALL);
  printTypes("Output Types", GROUP_OUTPUT, ALL);
  printTypes("Error Cost Types", GROUP_COST, ERROR_MASKS | TARGET_COPY);
  printTypes("Unit Cost Types", GROUP_COST, UNIT_COST_MASKS);
  printTypes("Group Criterion Types", GROUP, CRITERION_MASKS);
  printTypes("History Types", GROUP, USE_HIST_MASKS);
  printTypes("Other Types", GROUP, 
	     ~(FIXED_MASKS | CRITERION_MASKS | USE_HIST_MASKS));
}


/*************************** Group Type Cleanup ******************************/

#define SET_TYPE(type, add, remove) type = (((type) & ~(remove)) | (add))

/* This eliminates all but the lowest numbered type from the range (if there
   is one) and returns the new type. */
static mask selectFirstType(mask type, mask range) {
  int i;
  if (!(type & range)) return type;
  for (i = 0; i < sizeof(int); i++)
    if ((1 << i) & range & type) {
      SET_TYPE(type, 1 << i, range);
      return type;
    }
  return type;
}

/* This actually computes a max. */
static real updateMin(real val, real new) {
  if (isNaN(val) || new > val) return new;
  return val;
}

/* This actually computes a min. */
static real updateMax(real val, real new) {
  if (isNaN(val) || new < val) return new;
  return val;
}

void estimateOutputBounds(void *V) {
  GroupProc P;
  Group S, G = (Group) V;
  real min = NaN, max = NaN, init = 0.0;
  for (P = G->outputProcs; P; P = P->next) {
    switch (P->type) {
    case BIAS_CLAMP:
      min = 0;
      max = init = 1.0;
      break;
    case LOGISTIC:
    case GAUSSIAN:
    case SOFT_MAX:
    case KOHONEN:
    case OUT_BOLTZ:
    case OUT_NORM:
      min = updateMin(min, 0.0);
      max = updateMax(max, 1.0);
      if (P->type == SOFT_MAX) 
	init = 1.0 / G->numUnits;
      else init = 0.5;
      break;
    case TERNARY:
    case TANH:
      min = updateMin(min, -1.0);
      max = updateMax(max, 1.0);
      init = 0.0;
      break;
    case EXPONENTIAL:
      min = updateMin(min, 0.0);
      break;
    case ELMAN_CLAMP:
      if ((S = (Group) P->otherData)) {
	if (isNaN(min)) min = S->minOutput;
	else min += S->minOutput;
	if (isNaN(max)) max = S->maxOutput;
	else max += S->maxOutput;
	init = S->initOutput;
      }
      break;
    }
  }
  /* This assumes that clamping will be in the range [0,1]. */
  if (isNaN(min) && isNaN(max) && 
      (G->outputType & (CLAMPING_OUTPUT_TYPES & ~ELMAN_CLAMP))) {
    min = 0.0;
    max = 1.0;
    init = 0.5;
  }

  G->minOutput = min;
  G->maxOutput = max;
  if (!isNaN(G->initOutput)) G->initOutput = init;
}

extern void cleanupGroupType(void *V) {
  Group G = (Group) V;
  GroupProc P;
  flag special;

  /* Clean up the fixed type */
  /* Can't be BIAS and another fixed type */
  if (G->type & BIAS)
    SET_TYPE(G->type, BIAS, FIXED_MASKS | CRITERION_MASKS);

  /* A group is special if it is not HIDDEN and not OUTPUT. */
  special = (G->type & (INPUT | ELMAN | BIAS) && !(G->type & OUTPUT));

  /* Non-special groups get bias input by default. */
  if (!special) G->type |= BIASED;

  /* Whether or not a group is reset at the start of an example by default. */
  if (Net->type & CONTINUOUS) {
    if (DEF_G_continuousReset) G->type |= RESET_ON_EXAMPLE;
  } else if (DEF_G_standardReset) G->type |= RESET_ON_EXAMPLE;

  /* Force a single group criterion type. */
  G->type = selectFirstType(G->type, CRITERION_MASKS);

  /* We do nothing about history control params or dynamic states */

  /* Output groups get WRITE_OUTPUTS by default. */
  if (G->type & OUTPUT) G->type |= WRITE_OUTPUTS;

  /* Set the inputType mask */
  G->inputType = 0;
  for (P = G->inputProcs; P; P = P->next)
    G->inputType |= P->type;
  /* Set the outputType mask */
  G->outputType = 0;
  for (P = G->outputProcs; P; P = P->next)
    G->outputType |= P->type;
  /* Set the errorType mask */
  G->costType = 0;
  for (P = G->costProcs; P; P = P->next)
    G->costType |= P->type;


  /* If there is no basic inputProc, choose a default one. */
  if (!(G->inputType & BASIC_INPUT_TYPES) && !special) {
    if (Net->type & BOLTZMANN)
      prependProc(G, GROUP_INPUT, IN_BOLTZ);
    else
      prependProc(G, GROUP_INPUT, DOT_PRODUCT);
  }

  /* If there is no basic or clamping outputProc, choose a default one. */
  if (!(G->outputType & (BASIC_OUTPUT_TYPES | CLAMPING_OUTPUT_TYPES))) {
    if (special && (G->type & BIAS))
      prependProc(G, GROUP_OUTPUT, BIAS_CLAMP);
    else if (special && (G->type & INPUT) && !(G->inputType & SOFT_CLAMP))
      prependProc(G, GROUP_OUTPUT, HARD_CLAMP);
    else if (special && (G->type & ELMAN))
      prependProc(G, GROUP_OUTPUT, ELMAN_CLAMP);
    else if (Net->type & BOLTZMANN)
      prependProc(G, GROUP_OUTPUT, OUT_BOLTZ);
    else
      prependProc(G, GROUP_OUTPUT, LOGISTIC);
  }

  /* If assumeIntegrate is on and there is no integrator, add an output one. */
  if ((Net->type & CONTINUOUS) && 
      (!special || ((G->type & INPUT) && !(G->outputType & HARD_CLAMP))) && 
      !(G->inputType & IN_INTEGR_TYPES) && !(G->outputType & OUT_INTEGR_TYPES))
    appendProc(G, GROUP_OUTPUT, OUT_INTEGR);

  /* Guess the minOutput and maxOutput for the group */
  estimateOutputBounds(G);

  /* Add error for OUTPUT groups without any. */
  if (!(G->costType & ERROR_MASKS) && (G->type & OUTPUT) && 
      !(G->outputType & (KOHONEN))) {
    if (G->outputType & SOFT_MAX)
      appendProc(G, GROUP_COST, DIVERGENCE);
    else if (G->outputType & LINEAR)
      appendProc(G, GROUP_COST, SUM_SQUARED);
    else
      appendProc(G, GROUP_COST, CROSS_ENTROPY);
  }
  
  /* Output groups in continuous networks get the standard crit. by default */
  if ((Net->type & CONTINUOUS) && (G->type & OUTPUT) && 
      !(G->type & CRITERION_MASKS))
    G->type |= STANDARD_CRIT;
}


/******************************* Projection Types ****************************/

void registerProjectionType(char *name, mask type,
			    flag (*connectGroups)(Group, Group, mask, 
						  real, real, real)) {
  registerType(name, type, PROJECTION);
  setTypeData(type, PROJECTION, (void *) connectGroups);
}

flag (*projectionProc(mask type))(Group, Group, mask, real, real, real) {
  return (flag (*)(Group, Group, mask, real, real, real)) 
    getTypeData(type, PROJECTION);
}


/***************************** Example Mode Types ****************************/

void registerExampleModeType(char *name, mask type,
			     flag (*loadNextExample)(ExampleSet)) {
  registerType(name, type, EXAMPLE_MODE);
  setTypeData(type, EXAMPLE_MODE, (void *) loadNextExample);
}

flag (*nextExample(mask type))(ExampleSet) {
  return (flag (*)(ExampleSet)) getTypeData(type, EXAMPLE_MODE);
}


/******************************** Initialization *****************************/

extern void initTypes(void) {
  registerType("FROZEN", FROZEN, NETWORK);

  registerType("BIAS", BIAS, GROUP);
  registerType("INPUT", INPUT, GROUP);
  registerType("OUTPUT", OUTPUT, GROUP);
  registerType("ELMAN", ELMAN, GROUP);

  registerType("USE_INPUT_HIST", USE_INPUT_HIST, GROUP);
  registerType("USE_OUTPUT_HIST", USE_OUTPUT_HIST, GROUP);
  registerType("USE_TARGET_HIST", USE_TARGET_HIST, GROUP);
  registerType("USE_OUT_DERIV_HIST", USE_OUT_DERIV_HIST, GROUP);

  registerType("BIASED", BIASED, GROUP);
  registerType("RESET_ON_EXAMPLE", RESET_ON_EXAMPLE, GROUP);
  registerType("ADAPTIVE_GAIN", ADAPTIVE_GAIN, GROUP);
  registerType("DERIV_NOISE", DERIV_NOISE, GROUP);
  registerType("EXT_INPUT_NOISE", EXT_INPUT_NOISE, GROUP);
  registerType("WRITE_OUTPUTS", WRITE_OUTPUTS, GROUP);

  registerType("LESIONED", LESIONED, GROUP);
  registerType("FROZEN", FROZEN, GROUP);

  /* This registers the other group and network types */
  registerNetAndGroupTypes();

  /* There are no true unit or link types yet, but they'd go here */

  registerProjectionTypes();

  registerExampleModeTypes();

  registerType("Waiting", WAITING, TASK);
  registerType("Training", TRAINING, TASK);
  registerType("Testing", TESTING, TASK);
  registerType("Parallel Training", PARA_TRAINING, TASK);
  registerType("Loading Examples", LOADING_EXAMPLES, TASK);
  registerType("Saving Examples", SAVING_EXAMPLES, TASK);
  registerType("Building Link Viewer", BUILDING_LINKS, TASK);

  registerType("NEVER", ON_NEVER, UPDATE_SIGNAL);
  registerType("TICKS", ON_TICK, UPDATE_SIGNAL);
  registerType("EVENTS", ON_EVENT, UPDATE_SIGNAL);
  registerType("EXAMPLES", ON_EXAMPLE, UPDATE_SIGNAL);
  registerType("WEIGHT_UPDATES", ON_UPDATE, UPDATE_SIGNAL);
  registerType("PROGRESS_REPORTS", ON_REPORT, UPDATE_SIGNAL);
  registerType("TRAINING_AND_TESTING", ON_TRAINING, UPDATE_SIGNAL);
  registerType("USER_SIGNALS", ON_CUSTOM, UPDATE_SIGNAL);
}
