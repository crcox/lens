#include "system.h"
#include <string.h>
#include <netinet/in.h>
#include <ctype.h>
#include "util.h"
#include "type.h"
#include "network.h"
#include "example.h"
#include "control.h"
#include "object.h"

String buf = NULL;


/* This doesn't reset the set state so you can switch sets and continue where
   you left off. */
flag useTrainingSet(ExampleSet S) {
  Net->trainingSet = S;
  signalSetsChanged(TRAIN_SET);
  return TCL_OK;
}

flag useTestingSet(ExampleSet S) {
  Net->testingSet = S;
  signalSetsChanged(TEST_SET);
  return TCL_OK;
}

ExampleSet lookupExampleSet(char *name) {
  int s;
  for (s = 0; s < Root->numExampleSets; s++)
    if (!strcmp(Root->set[s]->name, name)) return Root->set[s];
  return NULL;
}

/* Stores a link to the set in the Root set table. */
/* Assumes set with same name is not there already */
void registerExampleSet(ExampleSet S) {
  Root->numExampleSets++;
  Root->set = (ExampleSet *) safeRealloc(Root->set, Root->numExampleSets *
					 sizeof(ExampleSet),
					 "registerExampleSet:root->set");
  S->num = Root->numExampleSets - 1;
  Root->set[S->num] = S;
  eval("catch {.initExSet root.set(%d)}", S->num);
  signalSetListsChanged();
}

/* Removes the link to a set from the Root set table. */
/* And any links to the ExampleSet in networks */
static void unregisterExampleSet(ExampleSet S) {
  int s, n;
  Network N;

  Root->numExampleSets--;
  for (s = S->num; s < Root->numExampleSets; s++) {
    Root->set[s] = Root->set[s + 1];
    Root->set[s]->num = s;
  }
  if (Root->numExampleSets == 0)
    FREE(Root->set);

  for (n = 0; n < Root->numNetworks; n++) {
    N = Root->net[n];
    if (N->trainingSet == S) {
      if (N == Net)
	useTrainingSet(NULL);
      else N->trainingSet = NULL;
    }
    if (N->testingSet == S) {
      if (N == Net)
	useTestingSet(NULL);
      else N->testingSet = NULL;
    }
  }
  signalSetListsChanged();
}

static void registerExample(Example E, ExampleSet S) {
  E->next = NULL;
  if (!S->firstExample) {
    S->firstExample = E;
    S->lastExample = E;
  } else {
    S->lastExample->next = E;
    S->lastExample = E;
  }
  S->numExamples++;
  E->set = S;
}

ExampleSet newExampleSet(char *name) {
  ExampleSet S = (ExampleSet) safeCalloc(1, sizeof *S, "addExamples:S");
  S->name               = copyString(name);
  S->pipeLoop           = DEF_S_pipeLoop;
  S->maxTime            = DEF_S_maxTime;
  S->minTime            = DEF_S_minTime;
  S->graceTime          = DEF_S_graceTime;
  S->defaultInput       = DEF_S_defaultInput;
  S->activeInput        = DEF_S_activeInput;
  S->defaultTarget      = DEF_S_defaultTarget;
  S->activeTarget       = DEF_S_activeTarget;
  S->loadEvent          = standardLoadEvent;
  S->loadExample        = standardLoadExample;
  S->numGroupNames      = 0;
  S->maxGroupNames      = 0;
  S->groupName          = NULL;
  exampleSetMode(S, DEF_S_mode);
  initExSetExtension(S);
  return S;
}

Example newExample(ExampleSet S) {
  Example E = (Example) safeCalloc(1, sizeof *E, "newExample:E");
  E->frequency = DEF_E_frequency;
  E->set = S;
  initExampleExtension(E);
  return E;
}

static void cleanExample(Example E) {
  int i;
  Event V;
  Range L, N;
  if (!E) return;
  FREE(E->name);
  if (E->proc) Tcl_DecrRefCount(E->proc);
  if (E->event) {
    for (i = 0; i < E->numEvents; i++) {
      V = E->event + i;
      if (!V->sharedInputs) {
	for (L = V->input; L; L = N) {
	  N = L->next;
	  FREE(L->val);
	  FREE(L->unit);
	  free(L);
	}
      }
      if (!V->sharedTargets) {
	for (L = V->target; L; L = N) {
	  N = L->next;
	  FREE(L->val);
	  FREE(L->unit);
	  free(L);
	}
      }
      if (V->proc) Tcl_DecrRefCount(V->proc);
      freeEventExtension(V);
    }
    free(E->event);
  }
}

/* Doesn't affect the extension or the set */
static void clearExample(Example E) {
  E->name = NULL;
  E->num = 0;
  E->numEvents = 0;
  E->event = NULL;
  E->next = NULL;
  E->frequency = DEF_E_frequency;
  E->probability = 0.0;
  E->proc = NULL;
}

static void freeExample(Example E) {
  if (!E) return;
  cleanExample(E);
  freeExampleExtension(E);
  free(E);
}

void initEvent(Event V, Example E) {
  ExampleSet S = E->set;
  V->example = E;
  V->maxTime       = DEF_V_maxTime;
  V->minTime       = DEF_V_minTime;
  V->graceTime     = DEF_V_graceTime;
  V->defaultInput  = S->defaultInput;
  V->activeInput   = S->activeInput;
  V->defaultTarget = S->defaultTarget;
  V->activeTarget  = S->activeTarget;
  initEventExtension(V);
}

/* This is used when writing an example file */
static flag normalEvent(Event V, ExampleSet S) {
  if (V->proc != NULL)                           return FALSE;
  if (!SAME(V->maxTime, DEF_V_maxTime))          return FALSE;
  if (!SAME(V->minTime, DEF_V_minTime))          return FALSE;
  if (!SAME(V->graceTime, DEF_V_graceTime))      return FALSE;
  if (!SAME(V->defaultInput, S->defaultInput))   return FALSE;
  if (!SAME(V->activeInput, S->activeInput))     return FALSE;
  if (!SAME(V->defaultTarget, S->defaultTarget)) return FALSE;
  if (!SAME(V->activeTarget, S->activeTarget))   return FALSE;
  return TRUE;
}

static flag parseError(ParseRec R, const char *fmt, ...) {
  char message[256];
  va_list args;
  if (fmt[0]) {
    va_start(args, fmt);
    vsprintf(message, fmt, args);
    va_end(args);
    error("loadExamples: %s on %s %d of file \"%s\"",
	  message, (R->binary) ? "example" : "line", R->line, R->fileName);
  }
  if (R->channel) closeChannel(R->channel);
  R->channel = NULL;
  Tcl_DecrRefCount(R->fileName);
  freeString(R->buf);
  R->buf = NULL;

  return TCL_ERROR;
}

static void freeExampleSet(ExampleSet S) {
  Example E, N;

  if (!S) return;
  FREE(S->name);
  for (E = S->firstExample; E; E = N) {
    N = E->next;
    freeExample(E);
  }
  FREE(S->example);
  FREE(S->permuted);
  if (S->proc) Tcl_DecrRefCount(S->proc);
  if (S->pipeParser) {
    parseError(S->pipeParser, "");
    free(S->pipeParser);
  }
  FREE(S->groupName);

  freeExSetExtension(S);
  free(S);
}

/* This puts all the examples in arrays and does some other calculations */
static void compileExampleSet(ExampleSet S) {
  Example E;
  int i, v;
  real totalFreq = 0.0, scale, sum;
  struct Tcl_CmdInfo junk;

  /* Count the number of examples and events */
  for (S->numExamples = 0, S->numEvents = 0, E = S->firstExample;
       E; E = E->next, S->numExamples++)
    S->numEvents += E->numEvents;

  /* Build the example and permuted arrays */
  FREE(S->example);
  if (S->numExamples)
    S->example = (Example *) safeMalloc(S->numExamples * sizeof(Example),
					"compileExampleSet:S->example");
  FREE(S->permuted);
  if (S->numExamples)
    S->permuted = (Example *) safeMalloc(S->numExamples * sizeof(Example),
					 "compileExampleSet:S->permuted");
  /* Fill the arrays and call the user-defined init procedures on each example
     and event */
  for (i = 0, E = S->firstExample; E; E = E->next, i++) {
    S->example[i] = S->permuted[i] = E;
    E->num = i;
    totalFreq += E->frequency;
  }
  /* Should this be done on the pipe example? */
  if (Tcl_GetCommandInfo(Interp, ".initExample", &junk))
    for (E = S->firstExample; E; E = E->next)
      eval("catch {.initExample root.set(%d).example(%d)}", S->num, E->num);
  if (Tcl_GetCommandInfo(Interp, ".initEvent", &junk))
    for (E = S->firstExample; E; E = E->next)
      for (v = 0; v < E->numEvents; v++)
	eval("catch {.initEvent root.set(%d).example(%d).event(%d)",
	     S->num, E->num, v);

  /* Determine the event probabilities for probabilistic choosing. */
  if (S->numExamples) {
    scale = 1.0 / totalFreq;
    for (i = 0, sum = 0.0; i < S->numExamples; i++) {
      sum += S->example[i]->frequency * scale;
      S->example[i]->probability = sum;
    }
    S->example[S->numExamples - 1]->probability = 1.0;
  }
  S->currentExample = NULL;
  S->currentExampleNum = -1;
}

/*****************************************************************************/

/* This parses a list of event numbers */
static flag parseEventList(ParseRec R, char *eventActive, int num) {
  flag empty = TRUE;
  int v, w;
  bzero((void *) eventActive, num);
  skipBlank(R);
  while (isdigit((int) R->s[0]) || R->s[0] == '*') {
    empty = FALSE;
    if (stringMatch(R, "*")) {
      for (v = 0; v < num; v++) eventActive[v] = TRUE;
    } else {
      if (readInt(R, &v))
	return parseError(R, "error reading event list");
      if (v < 0 || v >= num)
	return parseError(R, "event (%d) out of range", v);
      if (stringMatch(R, "-")) {
	if (readInt(R, &w)) return parseError(R, "error reading event range");
	if (w <= v || w >= num)
	  return parseError(R, "event (%d) out of range", w);
      } else w = v;
      for (; v <= w; v++)
	eventActive[v] = TRUE;
    }
    skipBlank(R);
  }
  if (empty) for (v = 0; v < num; v++) eventActive[v] = TRUE;
  return TCL_OK;
}

static void tidyUpRange(Range L, int *unit, real *val, flag sparseMode) {
  int i;
  if (!L) return;
  if (sparseMode) {
    L->unit = intArray(L->numUnits, "tidyUpRange:L->unit");
    for (i = 0; i < L->numUnits; i++)
      L->unit[i] = unit[i];
  } else {
    L->val = realArray(L->numUnits, "tidyUpRange:L->val");
    for (i = 0; i < L->numUnits; i++)
      L->val[i] = val[i];
  }
}

Range newRange(Event V, Range L, flag doingInputs) {
  Range N = (Range) safeCalloc(1, sizeof(struct range), "newRange:N");
  N->value = (doingInputs) ? V->activeInput : V->activeTarget;
  if (L) L->next = N;
  else {
    if (doingInputs) V->input = N;
    else V->target = N;
  }
  return N;
}

static char *registerGroupName(char *name, ExampleSet S) {
  int i;
  for (i = 0; i < S->numGroupNames && strcmp(S->groupName[i], name); i++);
  if (i == S->numGroupNames) {
    if (S->maxGroupNames == 0) {
      S->maxGroupNames = 16;
      S->groupName = (char **) safeMalloc(S->maxGroupNames * sizeof(char *),
					  "registerGroupName");
    } else {
      S->maxGroupNames *= 2;
      S->groupName = (char **) safeRealloc(S->groupName,
	    S->maxGroupNames * sizeof(char *), "registerGroupName");
    }
    S->groupName[S->numGroupNames++] = copyString(name);
  }
  return S->groupName[i];
}

static flag readEventRanges(Event V, ExampleSet S, ParseRec R,
			    flag doingInputs, flag sparseMode) {
  Range L = NULL;
  flag done = FALSE;
  int  *unit, maxUnits, maxVals;
  real *val;

  maxUnits = maxVals = 2;
  unit = safeMalloc(maxUnits * sizeof(int), "readEventRanges:unit");
  val  = safeMalloc(maxVals * sizeof(real), "readEventRanges:val");

  do {
    if (stringMatch(R, "{")) {
      tidyUpRange(L, unit, val, sparseMode);
      L = newRange(V, L, doingInputs);
      while (!stringMatch(R, "}")) {
	if (isNumber(R)) {
	  if (readReal(R, &L->value)) {
	    parseError(R, "couldn't read sparse active value");
	    goto error;}
	}
	else {
	  if (readBlock(R, buf)) {
	    parseError(R, Tcl_GetStringResult(Interp));
	    goto error;}
	  if (buf->s[0])
	    if (!(L->groupName = registerGroupName(buf->s, S))) {
	      parseError(R, "too many group names used");
	      goto error;}
	}
      }
      sparseMode = TRUE;
    }
    else if (stringMatch(R, "(")) {
      tidyUpRange(L, unit, val, sparseMode);
      L = newRange(V, L, doingInputs);
      while (!stringMatch(R, ")")) {
	if (isNumber(R)) {
	  if (readInt(R, &L->firstUnit)) {
	    parseError(R, "couldn't read dense first unit");
	    goto error;}
	}
	else {
	  if (readBlock(R, buf)) {
	    parseError(R, Tcl_GetStringResult(Interp));
	    goto error;}
	  if (buf->s[0])
	    if (!(L->groupName = registerGroupName(buf->s, S))) {
	      parseError(R, "too many groups used");
	      goto error;}
	}
      }
      sparseMode = FALSE;
    }
    else if (stringMatch(R, "*")) {
      if (!L) L = newRange(V, L, doingInputs);
      if (!sparseMode || L->numUnits) {
	parseError(R, "* may only be the first thing in a sparse range");
	goto error;}
      if (L->numUnits >= maxUnits) {
	maxUnits *= 2;
	unit = safeRealloc(unit, maxUnits * sizeof(int),
			   "readEventRanges:unit");
      }
      unit[L->numUnits++] = -1;
    }
    else if (isNumber(R)) {
      if (!L) L = newRange(V, L, doingInputs);
      if (sparseMode) {
	if (L->numUnits && unit[0] < 0) {
	  parseError(R, "cannot have other units listed after *");
	  goto error;}
	if (L->numUnits >= maxUnits) {
	  maxUnits *= 2;
	  unit = safeRealloc(unit, maxUnits * sizeof(int),
			     "readEventRanges:unit");
	}
	if (readInt(R, unit + L->numUnits)) {
	  parseError(R, "error reading a sparse unit number");
	  goto error;}
	if (unit[L->numUnits] < 0 &&
	    (L->numUnits == 0 ||
	     -unit[L->numUnits] < unit[L->numUnits - 1])) {
	  parseError(R, "invalid sparse unit range");
	  goto error;}
	L->numUnits++;
      } else {
	if (L->numUnits >= maxVals) {
	  maxVals *= 2;
	  val  = safeRealloc(val, maxVals * sizeof(real),
			     "readEventRanges:val");
	}
	if (readReal(R, val + L->numUnits++)) {
	  parseError(R, "couldn't read dense values");
	  goto error;}
      }
    }
    else done = TRUE;
  } while (!done);

  tidyUpRange(L, unit, val, sparseMode);
  FREE(unit); FREE(val);
  return TCL_OK;
 error:
  FREE(unit); FREE(val);
  return TCL_ERROR;
}

/* This parses a text example */
static flag readExample(Example E, ParseRec R) {
  char *eventActive;
  Event V, W;
  ExampleSet S = E->set;
  flag inputsSeen, targetsSeen, done;
  int v, w, nextInputEvent, nextTargetEvent;

  /* Read the example header */
  if (fileDone(R))
    return parseError(R, "file ended prematurely at start of an example");
  E->numEvents = 1;
  do {
    done = TRUE;
    if (stringMatch(R, "name:")) {
      if (readBlock(R, buf))
	return parseError(R, "error reading example name");
      E->name = copyString(buf->s);
      done = FALSE;
    }
    if (stringMatch(R, "freq:")) {
      if (readReal(R, &(E->frequency)))
	return parseError(R, "error reading example frequency");
      done = FALSE;
    }
    if (stringMatch(R, "proc:")) {
      if (readBlock(R, buf))
	return parseError(R, "error reading example proc");
      E->proc = Tcl_NewStringObj(buf->s, strlen(buf->s));
      Tcl_IncrRefCount(E->proc);
      done = FALSE;
    }
    skipBlank(R);
    if (isdigit((int) R->s[0])) {
      if (readInt(R, &E->numEvents))
	return parseError(R, "error reading num-events");
      if (E->numEvents <= 0)
	return parseError(R, "num-events (%d) must be positive", E->numEvents);
      done = FALSE;
    }
  } while (!done);

  E->event = safeCalloc(E->numEvents, sizeof(struct event),
			"readExample:E->event");
  for (v = 0; v < E->numEvents; v++)
    initEvent(E->event + v, E);

  eventActive = (char *) safeCalloc(E->numEvents, sizeof(char),
				     "readExample:eventActive");

  nextInputEvent = nextTargetEvent = 0;
  inputsSeen = targetsSeen = TRUE;

  while (!stringMatch(R, ";")) {
    if (stringMatch(R, "[")) {
      /* Get the list of events */
      if (parseEventList(R, eventActive, E->numEvents)) goto abort;
      /* This makes V the first active event, and v its index */
      for (v = 0, V = NULL; v < E->numEvents && !V; v++)
	if (eventActive[v]) V = E->event + v--;
      if (!V) {
	parseError(R, "no events specified");
	goto abort;}

      /* Parse the event headers */
      while (!stringMatch(R, "]")) {
	if (stringMatch(R, "proc:")) {
	  if (readBlock(R, buf))
	    return parseError(R, "error reading example proc");
	  for (w = v; w < E->numEvents; w++)
	    if (eventActive[w]) {
	      W = E->event + w;
	      W->proc = Tcl_NewStringObj(buf->s, strlen(buf->s));
	      Tcl_IncrRefCount(W->proc);
	    }
	}
	else if (stringMatch(R, "max:")) {
	  if (readReal(R, &V->maxTime)) {
	    parseError(R, "missing value after \"max:\" in event header");
	    goto abort;}
	  for (w = v + 1; w < E->numEvents; w++)
	    if (eventActive[w]) E->event[w].maxTime = V->maxTime;
	}
	else if (stringMatch(R, "min:")) {
	  if (readReal(R, &V->minTime)) {
	    parseError(R, "missing value after \"min:\" in event header");
	    goto abort;}
	  for (w = v + 1; w < E->numEvents; w++)
	    if (eventActive[w]) E->event[w].minTime = V->minTime;
	}
	else if (stringMatch(R, "grace:")) {
	  if (readReal(R, &V->graceTime)) {
	    parseError(R, "missing value after \"grace:\" in event header");
	    goto abort;}
	  for (w = v + 1; w < E->numEvents; w++)
	    if (eventActive[w]) E->event[w].graceTime = V->graceTime;
	}
	else if (stringMatch(R, "defI:")) {
	  if (readReal(R, &V->defaultInput)) {
	    parseError(R, "missing value after \"defI:\" in event header");
	    goto abort;}
	  for (w = v + 1; w < E->numEvents; w++)
	    if (eventActive[w]) E->event[w].defaultInput = V->defaultInput;
	}
	else if (stringMatch(R, "actI:")) {
	  if (readReal(R, &V->activeInput)) {
	    parseError(R, "missing value after \"actI:\" in event header");
	    goto abort;}
	  for (w = v + 1; w < E->numEvents; w++)
	    if (eventActive[w]) E->event[w].activeInput = V->activeInput;
	}
	else if (stringMatch(R, "defT:")) {
	  if (readReal(R, &V->defaultTarget)) {
	    parseError(R, "missing value after \"defT:\" in event header");
	    goto abort;}
	  for (w = v + 1; w < E->numEvents; w++)
	    if (eventActive[w]) E->event[w].defaultTarget = V->defaultTarget;
	}
	else if (stringMatch(R, "actT:")) {
	  if (readReal(R, &V->activeTarget)) {
	    parseError(R, "missing value after \"actT:\" in event header");
	    goto abort;}
	  for (w = v + 1; w < E->numEvents; w++)
	    if (eventActive[w]) E->event[w].activeTarget = V->activeTarget;
	}
	else {
	  parseError(R, "something unexpected (%s) in event header",
		     R->s);
	  goto abort;}
      }
      inputsSeen = targetsSeen = FALSE;
    }

    else if (stringPeek(R, "I:") || stringPeek(R, "i:") ||
	     stringPeek(R, "T:") || stringPeek(R, "t:") ||
	     stringPeek(R, "B:") || stringPeek(R, "b:")) {
      flag sparseMode = FALSE, doingInputs = FALSE, doingTargets = FALSE;
      Event I = NULL, T = NULL;
      switch(R->s[0]) {
      case 'I':
	sparseMode = FALSE; doingInputs = TRUE;  doingTargets = FALSE; break;
      case 'i':
	sparseMode = TRUE;  doingInputs = TRUE;  doingTargets = FALSE; break;
      case 'T':
	sparseMode = FALSE; doingInputs = FALSE; doingTargets = TRUE;  break;
      case 't':
	sparseMode = TRUE;  doingInputs = FALSE; doingTargets = TRUE;  break;
      case 'B':
	sparseMode = FALSE; doingInputs = TRUE;  doingTargets = TRUE;  break;
      case 'b':
	sparseMode = TRUE;  doingInputs = TRUE;  doingTargets = TRUE;  break;
      }
      R->s += 2;

      if (doingInputs) {
	if (inputsSeen) {
	  /* Inputs have already been given since the last event list */
	  if (nextInputEvent >= E->numEvents) {
	    parseError(R, "attempted to specify inputs for event %d",
		       nextInputEvent); goto abort;}
	  I = E->event + nextInputEvent++;
	} else {
	  /* This will apply to the active events */
	  for (v = 0; v < E->numEvents && !I; v++)
	    if (eventActive[v]) I = E->event + v--;
	  if (!I) {parseError(R, "no events specified"); goto abort;}
	  if (I->input) {parseError(R, "multiple inputs given for event %d",
				    v); goto abort;}
	}
      }
      if (doingTargets) {
	if (targetsSeen) {
	  /* Targets have already been given since the last event list */
	  if (nextTargetEvent >= E->numEvents) {
	    parseError(R, "attempted to specify targets for event %d",
		       nextTargetEvent); goto abort;}
	  T = E->event + nextTargetEvent++;
	} else {
	  /* This will apply to the active events */
	  for (v = 0; v < E->numEvents && !T; v++)
	    if (eventActive[v]) T = E->event + v--;
	  if (!T) {parseError(R, "no events specified"); goto abort;}
	  if (T->target) {parseError(R, "multiple targets given for event %d",
				     v); goto abort;}
	}
      }

      V = (I) ? I : T;
      if (readEventRanges(V, S, R, (V == I), sparseMode)) goto abort;

      if (I && T) {
	if (T->target) {
	  parseError(R, "multiple targets specified for event %d",
		     (targetsSeen) ? nextTargetEvent - 1: v);
	  goto abort;}
	T->target = I->input;
	T->sharedTargets = TRUE;
      }

      if (doingInputs && !inputsSeen) {
	for (w = v + 1; w < E->numEvents; w++)
	  if (eventActive[w]) {
	    W = E->event + w;
	    if (W->input) {
	      parseError(R, "multiple inputs given for event %d", w);
	      goto abort;}
	    W->input = I->input;
	    W->sharedInputs = TRUE;
	  }
	for (w = nextInputEvent; w < E->numEvents; w++)
	  if (eventActive[w]) nextInputEvent = w + 1;
	inputsSeen = TRUE;
      }
      if (doingTargets && !targetsSeen) {
	for (w = v + 1; w < E->numEvents; w++)
	  if (eventActive[w]) {
	    W = E->event + w;
	    if (W->target) {
	      parseError(R, "multiple targets given for event %d", w);
	      goto abort;}
	    W->target = T->target;
	    W->sharedTargets = TRUE;
	  }
	for (w = nextTargetEvent; w < E->numEvents; w++)
	  if (eventActive[w]) nextTargetEvent = w + 1;
	targetsSeen = TRUE;
      }
    }


#ifdef JUNK
    /* Read inputs */
    else if (stringPeek(R, "I:") || stringPeek(R, "i:")) {
      if (stringMatch(R, "I:")) sparseMode = FALSE;
      else {
	stringMatch(R, "i:");
	sparseMode = TRUE;
      }

      /* Figure out which events the inputs are being specified for */
      if (inputsSeen) {
	/* Inputs have already been given since the last event list */
	if (nextInputEvent >= E->numEvents) {
	  parseError(R, "attempted to specify inputs for event %d",
		     nextInputEvent);
	  goto abort;}
	V = E->event + nextInputEvent++;
	usingActive = FALSE;
      } else {
	/* This will apply to the active events */
	for (v = 0, V = NULL; v < E->numEvents && !V; v++)
	  if (eventActive[v]) V = E->event + v--;
	if (!V) {
	  parseError(R, "no events specified");
	  goto abort;}
	usingActive = TRUE;
      }
      if (V->input) {
	parseError(R, "multiple inputs specified for event %d", v);
	goto abort;}

      if (readEventRanges(V, S, R, TRUE, sparseMode)) goto abort;

      if (usingActive) {
	for (w = v + 1; w < E->numEvents; w++)
	  if (eventActive[w]) {
	    W = E->event + w;
	    if (W->input) {
	      parseError(R, "multiple inputs specified for event %d", w);
	      goto abort;}
	    W->input = V->input;
	    W->sharedInputs = TRUE;
	  }
	for (v = nextInputEvent; v < E->numEvents; v++)
	  if (eventActive[v]) nextInputEvent = v + 1;
      }
      inputsSeen = TRUE;
    }

    /* Read targets */
    else if (stringPeek(R, "T:") || stringPeek(R, "t:")) {
      if (stringMatch(R, "T:")) sparseMode = FALSE;
      else {
	stringMatch(R, "t:");
	sparseMode = TRUE;
      }

      /* Figure out which events the targets are being specified for */
      if (targetsSeen) {
	/* Targets have already been given since the last event list */
	if (nextTargetEvent >= E->numEvents) {
	 parseError(R, "attempted to specify targets for event %d",
		     nextTargetEvent);
	  goto abort;}
	V = E->event + nextTargetEvent++;
	usingActive = FALSE;
      } else {
	/* This will apply to the active events */
	for (v = 0, V = NULL; v < E->numEvents && !V; v++)
	  if (eventActive[v]) V = E->event + v--;
	if (!V) {
	  parseError(R, "no events specified");
	  goto abort;}
	usingActive = TRUE;
      }
      if (V->target) {
	parseError(R, "multiple targets specified for event %d", v);
	goto abort;}

      if (readEventRanges(V, S, R, FALSE, sparseMode)) goto abort;

      if (usingActive) {
	for (w = v + 1; w < E->numEvents; w++)
	  if (eventActive[w]) {
	    W = E->event + w;
	    if (W->target) {
	      parseError(R, "multiple targets specified for event %d", w);
	      goto abort;}
	    W->target = V->target;
	    W->sharedTargets = TRUE;
	  }
	for (v = nextTargetEvent; v < E->numEvents; v++)
	  if (eventActive[v]) nextTargetEvent = v + 1;
      }
      targetsSeen = TRUE;
    }
#endif

    else {
      parseError(R, "missing semicolon or something unexpected (%s)", R->s);
      goto abort;}
  }

  FREE(eventActive);
  return TCL_OK;

 abort:
  FREE(eventActive);
  return TCL_ERROR;
}

static flag readBinaryExampleSet(char *setName, ParseRec R, ExampleSet *Sp,
				 flag pipe, int maxExamples);
static flag readExampleSet(char *setName, Tcl_Obj *fileNameObj, ExampleSet *Sp,
			   flag pipe, int maxExamples) {
  ExampleSet S = *Sp;
  Example E = NULL;
  int val, examplesRead;
  flag halted;
  struct parseRec rec;
  ParseRec R = &rec;
  char *fileName = Tcl_GetStringFromObj(fileNameObj, NULL);

  if (!buf) buf = newString(128);
  if (!(R->channel = readChannel(fileNameObj)))
    return error("readExampleSet: couldn't open the file \"%s\"", fileName);
//  R->fileName = copyString(fileName);
  R->fileName = Tcl_NewStringObj(fileName, -1);
  R->buf = NULL;
  R->s = NULL;
  R->line = 0;

  /* Look for the binary cookie */
  if (readBinInt(R->channel, &val))
    return parseError(R, "example file empty");
  if (val == BINARY_EXAMPLE_COOKIE)
    return readBinaryExampleSet(setName, R, Sp, pipe, maxExamples);
  R->buf = newString(128);
  R->binary = FALSE;
  if (startParser(R, HTONL(val))) {
    parseError(R, "couldn't read the header"); goto abort;}

  /* This gets rid of any numInputs and numTargets that may be there */
  readInt(R, &val);
  readInt(R, &val);
  if (!S) registerExampleSet((S = *Sp = newExampleSet(setName)));

  /* Read the example set header */
  while (!stringMatch(R, ";")) {
    if (stringMatch(R, "proc:")) {
      if (readBlock(R, buf)) {
	parseError(R, "error reading code segment"); goto abort;}
      S->proc = Tcl_NewStringObj(buf->s, strlen(buf->s));
      Tcl_IncrRefCount(S->proc);
      if (Tcl_EvalObjEx(Interp, S->proc, TCL_EVAL_GLOBAL) != TCL_OK)
	return TCL_ERROR;
    }
    else if (stringMatch(R, "max:")) {
      if (readReal(R, &(S->maxTime))) {
	parseError(R, "missing value after \"max:\" in the set header");
	goto abort;}
    }
    else if (stringMatch(R, "min:")) {
      if (readReal(R, &(S->minTime))) {
	parseError(R, "missing value after \"min:\" in the set header");
	goto abort;}
    }
    else if (stringMatch(R, "grace:")) {
      if (readReal(R, &(S->graceTime))) {
	parseError(R, "missing value after \"grace:\" in the set header");
	goto abort;}
    }
    else if (stringMatch(R, "defI:")) {
      if (readReal(R, &(S->defaultInput))) {
	parseError(R, "missing value after \"defI:\" in the set header");
	goto abort;}
    }
    else if (stringMatch(R, "actI:")) {
      if (readReal(R, &(S->activeInput))) {
	parseError(R, "missing value after \"actI:\" in the set header");
	goto abort;}
    }
    else if (stringMatch(R, "defT:")) {
      if (readReal(R, &(S->defaultTarget))) {
	parseError(R, "missing value after \"defT:\" in the set header");
	goto abort;}
    }
    else if (stringMatch(R, "actT:")) {
      if (readReal(R, &(S->activeTarget))) {
	parseError(R, "missing value after \"actT:\" in the set header");
	goto abort;}
    }
    else break;
  }

  if (pipe) {
    if (S->pipeParser)
      parseError(S->pipeParser, "");
    else
      S->pipeParser = (ParseRec) safeMalloc(sizeof(struct parseRec),
					    "readExampleSet:S->pipeParser");
    S->pipeParser->channel = R->channel;
    S->pipeParser->fileName = R->fileName;
    S->pipeParser->line = R->line;
    S->pipeParser->buf = R->buf;
    S->pipeParser->s = R->s;
    S->pipeParser->binary = FALSE;
    memcpy(S->pipeParser->cookie, R->cookie, sizeof(int));
    S->pipeParser->cookiePos = R->cookiePos;
    exampleSetMode(S, PIPE);
    return TCL_OK;
  }

  /* This loops over examples */
  for (examplesRead = 0, halted = FALSE; !fileDone(R) && !halted &&
	 (!maxExamples || examplesRead < maxExamples); examplesRead++) {
    E = newExample(S);
    if (readExample(E, R)) goto abort;
    registerExample(E, S);

    halted = smartUpdate(FALSE);
  }

  /* This is called to clean up the parser */
  parseError(R, "");
  compileExampleSet(S);
  if (halted) return result("readExampleSet: halted prematurely");
  return TCL_OK;

abort:
  if (S) {
    if (lookupExampleSet(S->name))
      compileExampleSet(S);
    else freeExampleSet(S);
  }
  if (E) freeExample(E);
  return TCL_ERROR;
}


static flag parseBinaryEventList(ParseRec R, char *eventActive, int num) {
  int events, v = 0, w, i;
  bzero((void *) eventActive, num);
  if (readBinInt(R->channel, &events))
    return TCL_ERROR;
  for (i = 0; i < events; i++) {
    if (readBinInt(R->channel, &w))
      return TCL_ERROR;
    if (w < 0) {
      w = -w;
      for (v++; v <= w; v++)
	eventActive[v] = TRUE;
    } else eventActive[v = w] = TRUE;
  }
  return TCL_OK;
}

static flag readBinaryEventRanges(Event V, ExampleSet S, Tcl_Channel channel,
				  flag doingInputs, ParseRec R) {
  Range L = NULL;
  int i, r, numRanges;
  flag sparseMode;

  if (readBinInt(channel, &numRanges)) {
    parseError(R, "error reading numRanges");
    return TCL_ERROR;}
  for (r = 0; r < numRanges; r++) {
    L = newRange(V, L, doingInputs);
    /* Read the group name if there is one */
    if (readBinString(channel, buf)) {
      parseError(R, "error reading group name");
      return TCL_ERROR;}
    if (buf->s[0])
      if (!(L->groupName = registerGroupName(buf->s, S))) {
	parseError(R, "too many group names used");
	return TCL_ERROR;}
    if (readBinInt(channel, &(L->numUnits)) || L->numUnits < 0) {
      parseError(R, "error reading range numUnits");
      return TCL_ERROR;}
    if (readBinFlag(channel, &sparseMode)) {
      parseError(R, "error reading sparse mode flag");
      return TCL_ERROR;}
    if (sparseMode) {
      if (readBinReal(channel, &(L->value))) {
	parseError(R, "error reading sparse value");
	return TCL_ERROR;}
      L->unit = intArray(L->numUnits, "readBinaryEventRanges:L->unit");
      for (i = 0; i < L->numUnits; i++)
	if (readBinInt(channel, L->unit + i)) {
	  parseError(R, "error reading sparse unit list");
	  return TCL_ERROR;
	}
    } else {
      if (readBinInt(channel, &(L->firstUnit))) {
	parseError(R, "error reading first unit");
	return TCL_ERROR;}
      L->val = realArray(L->numUnits, "readBinaryEventRanges:L->val");
      for (i = 0; i < L->numUnits; i++)
	if (readBinReal(channel, L->val + i)) {
	  parseError(R, "error reading dense value list");
	  return TCL_ERROR;
	}
    }
  }
  return TCL_OK;
}

static flag readBinaryExample(Example E, ParseRec R) {
  char *eventActive;
  Event V, W;
  ExampleSet S = E->set;
  Tcl_Channel channel = R->channel;
  int i, v, w, sets, s;
  flag sharedTargets;

  R->line++;
  /* Read the example header */
  if (readBinString(channel, buf))
    return parseError(R, "file ended prematurely");
  if (buf->s[0])
    E->name = copyString(buf->s);
  else E->name = NULL;
  if (readBinString(channel, buf))
    return parseError(R, "file ended prematurely in proc");
  if (buf->numChars) {
    E->proc = Tcl_NewStringObj(buf->s, buf->numChars);
    Tcl_IncrRefCount(E->proc);
  }
  if (readBinReal(channel, &E->frequency))
    return parseError(R, "couldn't read example frequency");
  if (E->frequency < 0.0)
    return parseError(R, "bad frequency (%g)", E->frequency);
  if (readBinInt(channel, &E->numEvents))
    return parseError(R, "couldn't read numEvents");
  if (E->numEvents <= 0)
    return parseError(R, "bad numEvents (%d)", E->numEvents);

  /* Read the event headers */
  E->event = safeCalloc(E->numEvents, sizeof(struct event),
			"readBinaryExample:E->event");
  for (v = 0; v < E->numEvents; v++)
    initEvent(E->event + v, E);

  if (readBinInt(channel, &sets) || sets < 0 || sets > E->numEvents)
    return parseError(R, "couldn't read the number of event headers");
  for (i = 0; i < sets; i++) {
    if (readBinInt(channel, &v))
      return parseError(R, "couldn't read the header event number");
    if (v < 0 || v >= E->numEvents)
      return parseError(R, "bad header event number (%d)", v);

    V = E->event + v;
    if (readBinString(channel, buf))
      return parseError(R, "file ended in event proc");
    if (buf->numChars) {
      V->proc = Tcl_NewStringObj(buf->s, buf->numChars);
      Tcl_IncrRefCount(V->proc);
    }
    if (readBinReal(channel, &V->maxTime))
      return parseError(R, "couldn't read event maxTime");
    if (readBinReal(channel, &V->minTime))
      return parseError(R, "couldn't read event minTime");
    if (readBinReal(channel, &V->graceTime))
      return parseError(R, "couldn't read event graceTime");
    if (readBinReal(channel, &V->defaultInput))
      return parseError(R, "couldn't read event defaultInput");
    if (readBinReal(channel, &V->activeInput))
      return parseError(R, "couldn't read event activeInput");
    if (readBinReal(channel, &V->defaultTarget))
      return parseError(R, "couldn't read event defaultTarget");
    if (readBinReal(channel, &V->activeTarget))
      return parseError(R, "couldn't read event activeTarget");
    initEventExtension(V);
  }

  eventActive = (char *) safeCalloc(E->numEvents, sizeof(char),
				     "readExample:eventActive");

  /* Read the inputs */
  if (readBinInt(channel, &sets) || sets < 0) {
    parseError(R, "error reading number of input sets");
    goto abort;}

  for (s = 0; s < sets; s++) {
    if (parseBinaryEventList(R, eventActive, E->numEvents)) {
      parseError(R, "error reading event list");
      goto abort;}
    /* This makes V the first active event, and v its index */
    for (v = 0, V = NULL; v < E->numEvents && !V; v++)
      if (eventActive[v]) V = E->event + v--;
    if (!V) {
      parseError(R, "no events specified");
      goto abort;}

    if (readBinaryEventRanges(V, S, channel, TRUE, R)) goto abort;

    for (w = v + 1; w < E->numEvents; w++)
      if (eventActive[w]) {
	W = E->event + w;
	W->input = V->input;
	W->sharedInputs = TRUE;
      }
    /* This handles shared inputs and targets. */
    if (readBinFlag(channel, &sharedTargets)) {
      parseError(R, "error reading shared targets code");
      goto abort;}
    if (sharedTargets) {
      if (parseBinaryEventList(R, eventActive, E->numEvents)) {
	parseError(R, "error reading event list");
	goto abort;}
      for (w = 0; w < E->numEvents; w++)
	if (eventActive[w]) {
	  W = E->event + w;
	  W->target = V->input;
	  W->sharedTargets = TRUE;
	}
    }
  }

  /* Read the targets */
  if (readBinInt(channel, &sets) || sets < 0) {
    parseError(R, "error reading number of target sets");
    goto abort;}

  for (s = 0; s < sets; s++) {
    if (parseBinaryEventList(R, eventActive, E->numEvents)) {
      parseError(R, "error reading event list");
      goto abort;}
    /* This makes V the first active event, and v its index */
    for (v = 0, V = NULL; v < E->numEvents && !V; v++)
      if (eventActive[v]) V = E->event + v--;
    if (!V) {
      parseError(R, "no events specified");
      goto abort;}

    if (readBinaryEventRanges(V, S, channel, FALSE, R)) goto abort;

    for (w = v + 1; w < E->numEvents; w++)
      if (eventActive[w]) {
	W = E->event + w;
	W->target = V->target;
	W->sharedTargets = TRUE;
      }
  }

  FREE(eventActive);
  return TCL_OK;

 abort:
  FREE(eventActive);
  return TCL_ERROR;
}

static flag readBinaryExampleSet(char *setName, ParseRec R, ExampleSet *Sp,
				 flag pipe, int maxExamples) {
  ExampleSet S = *Sp;
  Example E = NULL;
  Tcl_Channel channel = R->channel;
  int val, i, numExamples;
  flag halted;

  R->binary = TRUE;
  binaryEncoding(channel);
  if (!S) registerExampleSet((S = *Sp = newExampleSet(setName)));

  if (readBinInt(channel, &val)) {
    parseError(R, "error reading sizeof(real)");
    goto abort;}
  if (val != sizeof(real)) {
    parseError(R, "sizeof(real) (%d) should be (%d)", val, sizeof(real));
    goto abort;}

  if (readBinString(channel, buf)) {
    parseError(R, "file ended prematurely in set proc");
    goto abort;}
  if (buf->numChars) {
    S->proc = Tcl_NewStringObj(buf->s, buf->numChars);
    Tcl_IncrRefCount(S->proc);
    if (Tcl_EvalObjEx(Interp, S->proc, TCL_EVAL_GLOBAL) != TCL_OK)
      return TCL_ERROR;
  }
  if (readBinReal(channel, &S->maxTime)) {
    parseError(R, "couldn't read set maxTime");
    goto abort;}
  if (readBinReal(channel, &S->minTime)) {
    parseError(R, "couldn't read set minTime");
    goto abort;}
  if (readBinReal(channel, &S->graceTime)) {
    parseError(R, "couldn't read set graceTime");
    goto abort;}
  if (readBinReal(channel, &S->defaultInput)) {
    parseError(R, "couldn't read set defaultInput");
    goto abort;}
  if (readBinReal(channel, &S->activeInput)) {
    parseError(R, "couldn't read set activeInput");
    goto abort;}
  if (readBinReal(channel, &S->defaultTarget)) {
    parseError(R, "couldn't read set defaultTarget");
    goto abort;}
  if (readBinReal(channel, &S->activeTarget)) {
    parseError(R, "couldn't read set activeTarget");
    goto abort;}
  if (readBinInt(channel, &numExamples)) {
    parseError(R, "error reading numExamples");
    goto abort;}
  if (numExamples < 0) {
    parseError(R, "bad numExamples (%d)", numExamples);
    goto abort;}

  if (pipe) {
    if (S->pipeParser)
      parseError(S->pipeParser, "");
    else
      S->pipeParser = (ParseRec) safeMalloc(sizeof(struct parseRec),
					    "readExampleSet:S->pipeParser");
    S->pipeParser->channel = R->channel;
    S->pipeParser->fileName = R->fileName;
    S->pipeParser->line = R->line;
    S->pipeParser->buf = NULL;
    S->pipeParser->s = NULL;
    S->pipeParser->binary = TRUE;
    exampleSetMode(S, PIPE);
    return TCL_OK;
  }

  /* Process each example */
  for (i = 0, halted = FALSE;
       i < numExamples && (!maxExamples || i < maxExamples) && !halted; i++) {
    E = newExample(S);
    if (readBinaryExample(E, R)) goto abort;
    registerExample(E, S);

    halted = smartUpdate(FALSE);
  }

  /* This is called to clean up the parser */
  parseError(R, "");
  compileExampleSet(S);
  if (halted) return result("readBinaryExampleSet: halted prematurely");
  return TCL_OK;

abort:
  if (S) {
    if (lookupExampleSet(S->name))
      compileExampleSet(S);
    else freeExampleSet(S);
  }
  if (E) freeExample(E);
  return TCL_ERROR;
}

/* A mode of 0 means do nothing if the set exists.
   1 means override the set
   2 means add to the set
   3 means use as a pipe */
flag loadExamples(char *setName, Tcl_Obj *fileNameObj, int mode, int numExamples) {
  ExampleSet S;

  if ((S = lookupExampleSet(setName)) && (mode == 1)) {
    deleteExampleSet(S);
    S = NULL;
  }
  if (!S || mode != 0)
    if (readExampleSet(setName, fileNameObj, &S, (mode == 3), numExamples)) {
      deleteExampleSet(S);
      return TCL_ERROR;
    }
  if (Net && S) {
    if (!Net->trainingSet) {
      if (useTrainingSet(S)) return TCL_ERROR;
    } else if (!Net->testingSet && useTestingSet(S)) return TCL_ERROR;
  }
  return result(setName);
}

static flag readPipeExample(ExampleSet S) {
  ParseRec R = S->pipeParser;

  if (!R || !R->channel) {
    error("example set \"%s\" is in PIPE mode but has no open pipe",
	    S->name);
    goto abort;}
  if (fileDone(R)) {
    if (S->pipeLoop) {
      /* Reopen the pipe (and close the old one) */
      readExampleSet(S->name, R->fileName, &S, TRUE, 0);
    } else {
      parseError(R, "");
      error("example set \"%s\" has exhausted the pipe", S->name);
      goto abort;
    }
  }

  cleanExample(S->pipeExample);
  clearExample(S->pipeExample);
  if (R->binary) {
    if (readBinaryExample(S->pipeExample, R))
      goto abort;
  } else {
    if (readExample(S->pipeExample, R))
      goto abort;
  }

  S->pipeExampleNum++;
  return result("");

 abort:
  cleanExample(S->pipeExample);
  clearExample(S->pipeExample);
  S->pipeParser = NULL;
  S->pipeExampleNum = 0;
  return TCL_ERROR;
}

/*****************************************************************************/

static flag writeEventList(Example E, int v, int offset, Tcl_Channel channel) {
  int w, range, start, didone = 0;
  Event W;
  void *match = ObjP((char *) (E->event + v), offset);

  if (E->numEvents == 1) return TCL_OK;
  cprintf(channel, "[");
  start = v;
  range = 1;
  for (w = v + 1; w < E->numEvents; w++) {
    W = E->event + w;
    if (ObjP((char *) W, offset) == match) {
      if (range) range++;
      else {
	start = w;
	range = 1;
      }
    } else {
      if (range) {
	if (didone) cprintf(channel, " ");
	cprintf(channel, "%d", start);
	if (start < w - 1)
	  cprintf(channel, "-%d", w - 1);
	didone++;
      }
      range = 0;
    }
  }
  if (range) {
    if (didone) cprintf(channel, " ");
    if (start == 0) cprintf(channel, "*");
    else {
      cprintf(channel, "%d", start);
      if (start < w - 1)
	cprintf(channel, "-%d", w - 1);
    }
  }
  cprintf(channel, "]\n");
  return TCL_OK;
}

static void writeEventRanges(Range R, Tcl_Channel channel, real activeValue) {
  int i;
  Range L;
  flag isDense = (R->val != 0);
  for (L = R; L; L = L->next) {
    if (L != R) cprintf(channel, "  ");
    if (L->val) {
      if (!isDense || L->groupName || L->firstUnit) cprintf(channel, "(");
      if (L->groupName) {
	cprintf(channel, "\"%s\"", L->groupName);
	if (L->firstUnit) cprintf(channel, " %d", L->firstUnit);
      } else if (L->firstUnit) cprintf(channel, "%d", L->firstUnit);
      if (!isDense || L->groupName || L->firstUnit) cprintf(channel, ")");
      for (i = 0; i < L->numUnits; i++)
	cprintf(channel, " %g", L->val[i]);
      isDense = TRUE;
    } else {
      if (isDense || L->groupName || L->value != activeValue)
	cprintf(channel, "{");
      if (L->groupName) {
	cprintf(channel, "\"%s\"", L->groupName);
	if (L->value != activeValue) cprintf(channel, " %g", L->value);
      } else if (L->value != activeValue) cprintf(channel, "%g", L->value);
      if (isDense || L->groupName || L->value != activeValue)
	cprintf(channel, "}");
      if (L->unit[0] < 0) cprintf(channel, " *");
      else for (i = 0; i < L->numUnits; i++)
	cprintf(channel, "%s%d", (L->unit[i] < 0) ? "" : " ", L->unit[i]);
      isDense = FALSE;
    }
    cprintf(channel, "\n");
  }
}

flag writeExampleFile(ExampleSet S, Tcl_Obj *fileNameObj, flag append) {
  int v, w;
  flag halted;
  Example E;
  Event V;
  Tcl_Channel channel;
  const char *fileName = Tcl_GetStringFromObj(fileNameObj, NULL);

  if (!(channel = writeChannel(fileNameObj, append)))
    return error("writeExampleFile: couldn't open the file \"%s\"",
		 fileName);

  /* fprintf(file, "%d %d", S->numInputs, S->numTargets); */
  if (!append) {
    if (S->proc)
      cprintf(channel, "\nproc:{%s}", Tcl_GetStringFromObj(S->proc, NULL));
    if (!SAME(S->maxTime, DEF_S_maxTime))
      writeReal(channel, S->maxTime, "\nmax:", "");
    if (!SAME(S->minTime, DEF_S_minTime))
      writeReal(channel, S->minTime, "\nmin:", "");
    if (!SAME(S->graceTime, DEF_S_graceTime))
      writeReal(channel, S->graceTime, "\ngrace:", "");
    if (!SAME(S->defaultInput, DEF_S_defaultInput))
      writeReal(channel, S->defaultInput, "\ndefI:", "");
    if (!SAME(S->activeInput, DEF_S_activeInput))
      writeReal(channel, S->activeInput, "\nactI:", "");
    if (!SAME(S->defaultTarget, DEF_S_defaultTarget))
      writeReal(channel, S->defaultTarget, "\ndefT:", "");
    if (!SAME(S->activeTarget, DEF_S_activeTarget))
      writeReal(channel, S->activeTarget, "\nactT:", "");
    cprintf(channel, ";\n");
  }

  /* Do each example */
  for (E = S->firstExample, halted = FALSE; E && !halted; E = E->next) {
    if (E->name) cprintf(channel, "name:{%s}\n", E->name);
    if (E->proc) cprintf(channel, "proc:{%s}\n",
			 Tcl_GetStringFromObj(E->proc, NULL));
    if (E->frequency != DEF_E_frequency)
      writeReal(channel, E->frequency, "freq:", "\n");
    if (E->numEvents > 1) cprintf(channel, "%d\n", E->numEvents);

    /* Write the headers for all non-normal events */
    for (v = 0; v < E->numEvents; v++) {
      V = E->event + v;
      if (!normalEvent(V, S)) {
	cprintf(channel, "[%d", v);
	if (V->proc) cprintf(channel, " proc:{%s}",
			     Tcl_GetStringFromObj(V->proc, NULL));
	if (!SAME(V->maxTime, DEF_V_maxTime))
	  writeReal(channel, V->maxTime, " max:", "");
	if (!SAME(V->minTime, DEF_V_minTime))
	  writeReal(channel, V->minTime, " min:", "");
	if (!SAME(V->graceTime, DEF_V_graceTime))
	  writeReal(channel, V->graceTime, " grace:", "");
	if (!SAME(V->defaultInput, S->defaultInput))
	  writeReal(channel, V->defaultInput, " defI:", "");
	if (!SAME(V->activeInput, S->activeInput))
	  writeReal(channel, V->activeInput, " actI:", "");
	if (!SAME(V->defaultTarget, S->defaultTarget))
	  writeReal(channel, V->defaultTarget, " defT:", "");
	if (!SAME(V->activeTarget, S->activeTarget))
	  writeReal(channel, V->activeTarget, " actT:", "");
	cprintf(channel, "]\n");
      }
    }

    /* Write the inputs for all events and targets that share inputs. */
    for (v = 0; v < E->numEvents; v++) {
      V = E->event + v;
      if (V->sharedInputs || !V->input) continue;
      writeEventList(E, v, OFFSET(V, input), channel);
      cprintf(channel, "%c:", (V->input->val) ? 'I' : 'i');
      writeEventRanges(V->input, channel, V->activeInput);

      for (w = 0; w < E->numEvents && E->event[w].target != V->input; w++);
      if (w < E->numEvents) {
	writeEventList(E, w, OFFSET(V, target), channel);
	cprintf(channel, "%c:", (V->input->val) ? 'T' : 't');
	writeEventRanges(E->event[w].target, channel, V->activeTarget);
      }
    }
    /* Write the targets for all events */
    for (v = 0; v < E->numEvents; v++) {
      V = E->event + v;
      if (V->sharedTargets || !V->target) continue;
      writeEventList(E, v, OFFSET(V, target), channel);
      cprintf(channel, "%c:", (V->target->val) ? 'T' : 't');
      writeEventRanges(V->target, channel, V->activeTarget);
    }
    cprintf(channel, ";\n");

    halted = smartUpdate(FALSE);
  }

  closeChannel(channel);
  if (halted)
    return error("writeExampleFile: halted prematurely");
  return TCL_OK;
}


static flag writeBinaryEventList(Example E, int v, int offset,
				 Tcl_Channel channel) {
  int w, range, vals;
  Event W;
  void *match = ObjP((char *) (E->event + v), offset);

  for (w = v + 1, vals = 1, range = 1; w < E->numEvents; w++) {
    W = E->event + w;
    if (ObjP((char *) W, offset) == match) {
      if (range) range++;
      else {
	vals++;
	range = 1;
      }
    } else {
      if (range > 1) vals++;
      range = 0;
    }
  }
  if (range > 1) vals++;
  writeBinInt(channel, vals);
  writeBinInt(channel, v);
  for (w = v + 1, range = 1; w < E->numEvents; w++) {
    W = E->event + w;
    if (ObjP((char *) W, offset) == match) {
      if (range) range++;
      else {
	writeBinInt(channel, w);
	range = 1;
      }
    } else {
      if (range > 1)
	writeBinInt(channel, -(w-1));
      range = 0;
    }
  }
  if (range > 1) writeBinInt(channel, -(w - 1));
  return TCL_OK;
}

static void writeBinaryEventRanges(Range R, Tcl_Channel channel) {
  int i;
  Range L;
  for (L = R, i = 0; L; L = L->next, i++);
  writeBinInt(channel, i);
  for (L = R; L; L = L->next) {
    writeBinString(channel, L->groupName, -1);
    writeBinInt(channel, L->numUnits);
    if (L->val) {
      writeBinFlag(channel, FALSE);
      writeBinInt(channel, L->firstUnit);
      for (i = 0; i < L->numUnits; i++)
	writeBinReal(channel, L->val[i]);
    } else {
      writeBinFlag(channel, TRUE);
      writeBinReal(channel, L->value);
      for (i = 0; i < L->numUnits; i++)
	writeBinInt(channel, L->unit[i]);
    }
  }
}


flag writeBinaryExampleFile(ExampleSet S, Tcl_Obj *fileNameObj) {
  int v, w, len, ins, targs, lots;
  flag halted;
  Example E;
  Event V;
  Tcl_Channel channel;
  char *s;
  const char *fileName = Tcl_GetStringFromObj(fileNameObj, NULL);

  if (!(channel = writeChannel(fileNameObj, FALSE)))
    return error("writeBinaryExampleFile: couldn't open the file \"%s\"",
		 fileName);
  binaryEncoding(channel);

  /* Write the header */
  writeBinInt(channel, BINARY_EXAMPLE_COOKIE);
  /*
  writeBinInt(file, S->numInputs);
  writeBinInt(file, S->numTargets);
  */
  writeBinInt(channel, sizeof(real));
  if (S->proc) {
    s = Tcl_GetStringFromObj(S->proc, &len);
    writeBinString(channel, s, len);
  } else writeBinString(channel, NULL, 0);
  writeBinReal(channel, S->maxTime);
  writeBinReal(channel, S->minTime);
  writeBinReal(channel, S->graceTime);
  writeBinReal(channel, S->defaultInput);
  writeBinReal(channel, S->activeInput);
  writeBinReal(channel, S->defaultTarget);
  writeBinReal(channel, S->activeTarget);
  writeBinInt(channel, S->numExamples);

  /* Write each example */
  for (E = S->firstExample, halted = FALSE; E && !halted; E = E->next) {
    /* Write the example header */
    writeBinString(channel, E->name, -1);
    if (E->proc) {
      s = Tcl_GetStringFromObj(E->proc, &len);
      writeBinString(channel, s, len);
    } else writeBinString(channel, NULL, 0);
    writeBinReal(channel, E->frequency);
    writeBinInt(channel, E->numEvents);

    /* Write the event headers (only for non-normal events) */
    for (v = lots = ins = targs = 0; v < E->numEvents; v++) {
      V = E->event + v;
      if (!normalEvent(V, S)) lots++;
      if (!V->sharedInputs  && V->input)  ins++;
      if (!V->sharedTargets && V->target) targs++;
    }
    writeBinInt(channel, lots);
    for (v = 0; v < E->numEvents; v++) {
      V = E->event + v;
      if (normalEvent(V, S)) continue;
      writeBinInt(channel, v);
      if (V->proc) {
	s = Tcl_GetStringFromObj(V->proc, &len);
	writeBinString(channel, s, len);
      } else writeBinString(channel, NULL, 0);
      writeBinReal(channel, V->maxTime);
      writeBinReal(channel, V->minTime);
      writeBinReal(channel, V->graceTime);
      writeBinReal(channel, V->defaultInput);
      writeBinReal(channel, V->activeInput);
      writeBinReal(channel, V->defaultTarget);
      writeBinReal(channel, V->activeTarget);
    }

    /* Write the inputs */
    writeBinInt(channel, ins);
    for (v = 0; v < E->numEvents; v++) {
      V = E->event + v;
      if (V->sharedInputs || !V->input) continue;
      writeBinaryEventList(E, v, OFFSET(V, input), channel);
      writeBinaryEventRanges(V->input, channel);
      /* Write shared targets */
      for (w = 0; w < E->numEvents && E->event[w].target != V->input; w++);
      if (w < E->numEvents) {
	writeBinFlag(channel, 1);
	writeBinaryEventList(E, w, OFFSET(V, target), channel);
      } else writeBinFlag(channel, 0);
    }

    /* Write the targets */
    writeBinInt(channel, targs);
    for (v = 0; v < E->numEvents; v++) {
      V = E->event + v;
      if (V->sharedTargets || !V->target) continue;
      writeBinaryEventList(E, v, OFFSET(V, target), channel);
      writeBinaryEventRanges(V->target, channel);
    }
    halted = smartUpdate(FALSE);
  }

  closeChannel(channel);
  if (halted)
    return error("writeBinaryExampleFile: halted prematurely");
  return TCL_OK;
}

/*****************************************************************************/
/* These procedures are for writing network targets and outputs to a file so
   performance can be analyzed by an external program. */

/* This closes the previous one, unless you choose append and the filename is
   the same, in which case nothing happens. */
flag openNetOutputFile(Tcl_Obj *fileNameObj, flag binary, flag append) {
  Tcl_Channel channel;
  const char *fileName = Tcl_GetStringFromObj(fileNameObj, NULL);
  if (Net->outputFile) {
    if (append && !strcmp(fileName, Net->outputFileName))
      return TCL_OK;
    else closeNetOutputFile();
  }
  if (!(Net->outputFile = channel = writeChannel(fileNameObj, append)))
    return error("openNetOutputFile: couldn't open the file \"%s\"",
		 fileName);
  Net->binaryOutputFile = (binary) ? TRUE : FALSE;
  Net->outputFileName = copyString(fileName);
  return TCL_OK;
}

flag closeNetOutputFile(void) {
  if (!Net->outputFile)
    return error("netCloseOutputFile: no file open");
  closeChannel(Net->outputFile);
  Net->outputFile = NULL;
  FREE(Net->outputFileName);
  return TCL_OK;
}

void groupWriteBinaryValues(Group G, int tick, Tcl_Channel channel) {
  int index = HISTORY_INDEX(tick);
  writeBinInt(channel, G->numUnits);
  writeBinFlag(channel, (G->type & OUTPUT) ? 1 : 0);
  FOR_EVERY_UNIT(G, {
    writeBinReal(channel, GET_HISTORY(U, outputHistory, index));
    if (G->type & OUTPUT)
      writeBinReal(channel, GET_HISTORY(U, targetHistory, index));
  });
}

void groupWriteTextValues(Group G, int tick, Tcl_Channel channel) {
  int index = HISTORY_INDEX(tick);
  cprintf(channel, "%d %d\n", G->numUnits, (G->type & OUTPUT) ? 1 : 0);
  FOR_EVERY_UNIT(G, {
    writeReal(channel, GET_HISTORY(U, outputHistory, index), "", "");
    if (G->type & OUTPUT)
      writeReal(channel, GET_HISTORY(U, targetHistory, index), " ","");
    cprintf(channel, "\n");
  });
}

flag standardWriteExample(Example E) {
  int tick, groups = 0;
  Tcl_Channel channel = Net->outputFile;

  if (!channel)
    return error("writeExample: no output file open");
  FOR_EACH_GROUP(if (G->type & WRITE_OUTPUTS) groups++);
  if (Net->binaryOutputFile) {
    writeBinInt(channel, Net->totalUpdates);
    writeBinInt(channel, E->num);
    writeBinInt(channel, Net->ticksOnExample);
    writeBinInt(channel, groups);
    for (tick = 0; tick < Net->ticksOnExample; tick++) {
      writeBinInt(channel, tick);
      writeBinInt(channel, Net->eventHistory[tick]);
      FOR_EACH_GROUP({
	if (G->type & WRITE_OUTPUTS) groupWriteBinaryValues(G, tick, channel);
      });
    }
  } else {
    cprintf(channel, "%d %d\n", Net->totalUpdates, E->num);
    cprintf(channel, "%d %d\n", Net->ticksOnExample, groups);
    for (tick = 0; tick < Net->ticksOnExample; tick++) {
      cprintf(channel, "%d %d\n", tick, Net->eventHistory[tick]);
      FOR_EACH_GROUP({
	if (G->type & WRITE_OUTPUTS) groupWriteTextValues(G, tick, channel);
      });
    }
  }
  Tcl_Flush(channel);
  return TCL_OK;
}

/*****************************************************************************/

flag deleteExampleSet(ExampleSet S) {
  if (!S) return TCL_OK;
  unregisterExampleSet(S);
  freeExampleSet(S);
  return TCL_OK;
}

flag deleteAllExampleSets(void) {
  int s;
  for (s = Root->numExampleSets - 1; s >= 0; s--)
    deleteExampleSet(Root->set[s]);
  return TCL_OK;
}

Range copyRange(Range R) {
  Range C;
  if (R == NULL) return NULL;
  C = safeMalloc(sizeof(struct range), "copyRange");
  memcpy(C, R, sizeof(struct range));
  if (C->val) {
    real *old = C->val;
    C->val = safeMalloc(C->numUnits * sizeof(real), "copyRange");
    memcpy(C->val, old, C->numUnits * sizeof(real));
  }
  if (C->unit) {
    int *old = C->unit;
    C->unit = safeMalloc(C->numUnits * sizeof(int), "copyRange");
    memcpy(C->unit, old, C->numUnits * sizeof(int));
  }
  C->next = copyRange(C->next);
  return C;
}

Event copyEvents(Example E) {
  int i;
  Event C = safeCalloc(E->numEvents, sizeof(struct event),
		       "readExample:E->event");
  memcpy(C, E->event, E->numEvents * sizeof(struct event));
  for (i = 0; i < E->numEvents; i++) {
    Event V = C + i;
    if (V->proc) {
      V->proc = Tcl_DuplicateObj(V->proc);
      Tcl_IncrRefCount(V->proc);
    }
    V->input = copyRange(V->input);
    V->sharedInputs = FALSE;
    V->target = copyRange(V->target);
    V->sharedTargets = FALSE;
    V->example = E;
    V->ext = NULL;
    initEventExtension(V);
  }
  return C;
}

Example copyExample(Example E) {
  Example C = (Example) safeCalloc(1, sizeof *E, "copyExample:C");
  memcpy(C, E, sizeof(struct example));
  C->name = copyString(C->name);
  if (C->proc) {
    C->proc = Tcl_DuplicateObj(C->proc);
    Tcl_IncrRefCount(C->proc);
  }
  C->event = copyEvents(C);
  C->next = NULL;
  C->ext = NULL;
  initExampleExtension(C);
  return C;
}

/* If num is > 0, moves from start to start + num - 1.
   Else moves a random proportion. */
flag moveExamples(ExampleSet from, char *toName, int first, int num,
		  real proportion, flag copy) {
  int i, last;
  ExampleSet to;

  if (first < 0 || num < 0 || first + num > from->numExamples)
    return error("moveExamples: example indices (%d, %d) out of bounds",
		 first, num);
  if (num == 0 && (proportion < 0.0 || proportion > 1.0))
    return error("moveExamples: proportion (%f) must be in the range [0,1]",
		 proportion);

  if (!(to = lookupExampleSet(toName))) {
    to = newExampleSet(toName);
    if ((to->proc = from->proc))
      Tcl_IncrRefCount(to->proc);
    to->maxTime            = from->maxTime;
    to->minTime            = from->minTime;
    to->graceTime          = from->graceTime;
    to->defaultInput       = from->defaultInput;
    to->activeInput        = from->activeInput;
    to->defaultTarget      = from->defaultTarget;
    to->activeTarget       = from->activeTarget;
    to->loadEvent          = from->loadEvent;
    to->loadExample        = from->loadExample;
    registerExampleSet(to);
  }
  if (num > 0) {
    for (i = first; i < first + num; i++)
      from->example[i]->num = -1;
  } else {
    int tomark = from->numExamples * proportion;
    for (i = 0; tomark; i++) {
      if (randProb() < ((real) tomark / (from->numExamples - i))) {
	from->example[i]->num = -1;
	tomark--;
	num++;
      }
    }
  }

  if (copy) {
    for (i = 0; i < from->numExamples; i++) {
      if (from->example[i]->num == -1) {
	Example C = copyExample(from->example[i]);
	registerExample(C, to);
	from->example[i]->num = i;
      }
    }
  } else {
    from->firstExample = NULL;
    from->lastExample = NULL;
    for (i = 0, last = -1; i < from->numExamples; i++) {
      if (from->example[i]->num == -1) {
	if (last >= 0)
	  from->example[last]->next = from->example[i]->next;
	registerExample(from->example[i], to);
      } else {
	if (!from->firstExample)
	  from->firstExample = from->example[i];
	last = i;
      }
    }
    if (last >= 0) {
      from->lastExample = from->example[last];
      from->lastExample->next = NULL;
    }
  }

  if (!copy) {
    from->currentExampleNum = -1;
    compileExampleSet(from);
  }
  compileExampleSet(to);

  if (Net->trainingSet == from) {
    useTrainingSet(from);
    if (Net->testingSet == NULL)
      useTestingSet(to);
  }

  return result("%d", num);
}

#ifdef JUNK
flag moveAllExamples(ExampleSet from, char *toName, flag copy) {
  Example E, N;
  ExampleSet to;
  if (!(to = lookupExampleSet(toName))) {
    FREE(from->name);
    from->name = copyString(toName);
    if (Net->trainingSet == from)
      signalSetsChanged(TRAIN_SET);
    if (Net->testingSet == from)
      signalSetsChanged(TEST_SET);
    signalSetListsChanged();
    return TCL_OK;
  }
  if (from == to)
    return error("moveAllExamples: you can't move an example set to itself");

  for (E = from->firstExample; E; E = N) {
    N = E->next;
    registerExample(E, to);
  }
  from->firstExample = NULL;

  if (from->pipeParser) {
    if (to->pipeParser) parseError(to->pipeParser, "");
    to->pipeParser = from->pipeParser;
    if (to->pipeExample) freeExample(to->pipeExample);
    to->pipeExample = from->pipeExample;
    to->pipeLoop = from->pipeLoop;
  }

  compileExampleSet(to);
  if (Net && (Net->trainingSet == from))
    useTrainingSet(to);
  if (Net && (Net->testingSet == from))
    useTestingSet(to);
  deleteExampleSet(from);
  return TCL_OK;
}
#endif

flag resetExampleSet(ExampleSet S) {
  if (S->pipeParser) {
    S->currentExample = S->pipeExample;
    clearExample(S->currentExample);
  }
  else
    S->currentExample = NULL;
  S->currentExampleNum = -1;
  return TCL_OK;
}

flag exampleSetMode(ExampleSet S, mask mode) {
  if (mode == PIPE) {
    if (!S->pipeParser)
      return error("exampleSetMode: set \"%s\" has no pipe open", S->name);
    S->pipeExampleNum = 0;
    if (!S->pipeExample) S->pipeExample = newExample(S);
  }
  S->mode = mode;
  S->nextExample = nextExample(S->mode);
  return resetExampleSet(S);
}


/************************* Example Selection and Loading *********************/

flag loadDenseRange(Range L, Unit *netUnit, int numNetUnits, mask type) {
  int i, u;
  Group G;

  /* Check if this range applies to a named group */
  if (L->groupName && L->groupName[0]) {
    if (!(G = lookupGroup(L->groupName)))
      /* return error("loadEvent: group \"%s\" doesn't exist",
	 L->groupName);*/
      return TCL_OK;
  } else G = NULL;
  if (G) {            /* The range applies to a group */
    if ((u = L->firstUnit) < 0 || u >= G->numUnits)
      return error("loadEvent: unit %d out of range", u);
    if (u + L->numUnits > G->numUnits)
      return error("loadEvent: too many units listed in the "
		     "dense range");
    if (type == INPUT)
      for (i = 0; i < L->numUnits; i++, u++)
	G->unit[u].externalInput = L->val[i];
    else
      for (i = 0; i < L->numUnits; i++, u++)
	G->unit[u].target = L->val[i];
  } else {            /* The range applies to the whole net */
    if ((u = L->firstUnit) < 0 || u >= numNetUnits)
      return error("loadEvent: unit %d out of range", u);
    if (u + L->numUnits > numNetUnits)
      return error("loadEvent: too many units listed in the "
		     "dense range");
    if (type == INPUT)
      for (i = 0; i < L->numUnits; i++, u++)
	netUnit[u]->externalInput = L->val[i];
    else
      for (i = 0; i < L->numUnits; i++, u++)
	netUnit[u]->target = L->val[i];
  }
  return TCL_OK;
}

flag loadSparseRange(Range L, Unit *netUnit, int numNetUnits, int offset) {
  int i, u, v;
  Group G;

  /* Check if this range applies to a named group */
  if (L->groupName && L->groupName[0]) {
    if (!(G = lookupGroup(L->groupName)))
      /* return error("loadEvent: group \"%s\" doesn't exist",
	 L->groupName);*/
      return TCL_OK;
  } else G = NULL;
  if (G) {            /* The range applies to a group */
    if (L->unit[0] < 0) /* This is a * so all units are set */
      for (u = 0; u < G->numUnits; u++)
	*((real *) Obj((char *) (G->unit + u), offset)) = L->value;
    else for (i = 0; i < L->numUnits; i++) {
      if (i + 1 < L->numUnits && L->unit[i + 1] < 0) { /* This is a span */
	if ((u = L->unit[i]) < 0 || u >= G->numUnits)
	  return error("loadEvent: unit %d out of range "
			 "in group %s", u, G->name);
	if ((v = -L->unit[++i]) < u || v >= G->numUnits)
	  return error("loadEvent: span end unit %d out of "
			 "range in group %s", v, G->name);
	for (; u <= v; u++)
	  *((real *) Obj((char *) (G->unit + u), offset)) = L->value;
      } else { /* Just a single unit */
	if ((u = L->unit[i]) < 0 || u >= G->numUnits)
	  return error("loadEvent: unit %d out of range "
			 "in group %s", u, G->name);
	*((real *) Obj((char *) (G->unit + u), offset)) = L->value;
      }
    }
  } else {            /* The range applies to the whole net */
    if (L->unit[0] < 0) /* This is a * so all units are set */
      for (u = 0; u < numNetUnits; u++)
	*((real *) Obj((char *) (netUnit[u]), offset)) = L->value;
    else for (i = 0; i < L->numUnits; i++) {
      if (i + 1 < L->numUnits && L->unit[i + 1] < 0) { /* This is a span */
	if ((u = L->unit[i]) < 0 || u >= numNetUnits)
	  return error("loadEvent: unit %d out of range", u);
	if ((v = -L->unit[++i]) < u || v >= numNetUnits)
	  return error("loadEvent: span end unit %d out of range",v);
	for (; u <= v; u++)
	  *((real *) Obj((char *) (netUnit[u]), offset)) = L->value;
      } else { /* Just a single unit */
	if ((u = L->unit[i]) < 0 || u >= numNetUnits)
	  return error("loadEvent: unit %d out of range", u);
	*((real *) Obj((char *) (netUnit[u]), offset)) = L->value;
      }
    }
  }
  return TCL_OK;
}

void injectExternalInputNoise(void) {
  FOR_EACH_GROUP({
    if (G->type & EXT_INPUT_NOISE) {
      FOR_EACH_UNIT(G, {
	real range = chooseValue(G->noiseRange, Net->noiseRange);
	if (!isNaN(U->externalInput))
	  U->externalInput = G->noiseProc(U->externalInput, range);
      });
    }
  });
}

/* Loads inputs and targets.  This no longer respects held groups, so just
   don't change the input or targets when you want to hold a group.
   First the external inputs to all input units are set to the default value.
   If this is NaN, the units will not be hard-clamped.  Then the sparse and
   dense ranges are applied.  The same is done for targets, where NaN means no
   error or derivative. */
flag standardLoadEvent(Event V) {
  int u;
  Range L;

  /* First set all units' external inputs to the event's default input */
  for (u = 0; u < Net->numInputs; u++)
    Net->input[u]->externalInput = V->defaultInput;
  /* Now process each of the input ranges */
  for (L = V->input; L; L = L->next) {
    if (L->val) {         /* This is a dense range */
      if (loadDenseRange(L, Net->input, Net->numInputs, INPUT))
	return TCL_ERROR;
    } else if (L->unit) { /* This is a sparse range */
      if (loadSparseRange(L, Net->input, Net->numInputs,
			  OFFSET(Net->input[0], externalInput)))
	return TCL_ERROR;
    }
  }
  injectExternalInputNoise();

  /* First set all units' targets to the event's default target */
  for (u = 0; u < Net->numOutputs; u++)
    Net->output[u]->target = V->defaultTarget;
  /* Now process each of the target ranges */
  for (L = V->target; L; L = L->next) {
    if (L->val) {         /* This is a dense range */
      if (loadDenseRange(L, Net->output, Net->numOutputs, OUTPUT))
	return TCL_ERROR;
    } else if (L->unit) { /* This is a sparse range */
      if (loadSparseRange(L, Net->output, Net->numOutputs,
			  OFFSET(Net->output[0], target)))
	return TCL_ERROR;
    }
  }

  if (V->proc && Tcl_EvalObjEx(Interp, V->proc, TCL_EVAL_GLOBAL) != TCL_OK)
    return error(Tcl_GetStringResult(Interp));
  return TCL_OK;
}

/* This sets S->currentExample, runs proc, but does not start first event */
flag standardLoadExample(Example E) {
  ExampleSet S;
  if (!E) return error("standardLoadExample: no example given");

  if (!(S = E->set)) return error("standardLoadExample: no example set");

  S->currentExample = Net->currentExample = E;

  if (E->proc && Tcl_EvalObjEx(Interp, E->proc, TCL_EVAL_GLOBAL) != TCL_OK)
    return error(Tcl_GetStringResult(Interp));

  return TCL_OK;
}


static flag orderedLoadNextExample(ExampleSet S) {
  if (S->numExamples <= 0)
    return error("orderedLoadNextExample: set has no examples");
  if (++S->currentExampleNum == S->numExamples) S->currentExampleNum = 0;
  return S->loadExample(S->example[S->currentExampleNum]);
}

static flag permutedLoadNextExample(ExampleSet S) {
  Example temp;
  int next, i;
  if (S->numExamples <= 0)
    return error("permutedLoadNextExample: set has no examples");
  i = ++S->currentExampleNum;
  if (i == S->numExamples) i = 0;
  temp = S->permuted[i];
  next = i + randInt(S->numExamples - i);
  S->permuted[i] = S->permuted[next];
  S->permuted[next] = temp;
  S->currentExampleNum = i;
  return S->loadExample(S->example[S->permuted[i]->num]);
}

static flag randomizedLoadNextExample(ExampleSet S) {
  if (S->numExamples <= 0)
    return error("randomizedLoadNextExample: set has no examples");
  S->currentExampleNum = randInt(S->numExamples);
  return S->loadExample(S->example[S->currentExampleNum]);
}

/* This uses binary search to efficiently find the chosen example. */
static flag probabilisticLoadNextExample(ExampleSet S) {
  int l = 0, r = S->numExamples - 1, m;
  real prob = randProb();
  if (S->numExamples <= 0)
    return error("probabilisticLoadNextExample: set has no examples");
  while (l < r) {
    m = (l + r) / 2;
    if (S->example[m]->probability < prob) l = m + 1;
    else r = m;
  }
  S->currentExampleNum = l;
  return S->loadExample(S->example[l]);
}

static flag pipeLoadNextExample(ExampleSet S) {
  if (readPipeExample(S)) return TCL_ERROR;
  S->currentExampleNum = -1;
  return S->loadExample(S->pipeExample);
}

static flag customLoadNextExample(ExampleSet S) {
  int n;
  if (!S->chooseExample)
    return error("Example set \"%s\" needs a chooseExample procedure",
		   S->name);
  if (Tcl_EvalObj(Interp, S->chooseExample)) return TCL_ERROR;
  Tcl_GetIntFromObj(Interp, Tcl_GetObjResult(Interp), &n);
  if (n < 0 || n >= S->numExamples)
    return error("Example %d out of range", n);
  return S->loadExample(S->example[n]);
}

/* This should pick the next example based on the example selection mode and
   call S->loadExample */
flag loadNextExample(ExampleSet S) {
  if (!S) fatalError("standardLoadNextExample: no example set");
  return S->nextExample(S);
}

void registerExampleModeTypes(void) {
  registerExampleModeType("ORDERED", ORDERED, orderedLoadNextExample);
  registerExampleModeType("PERMUTED", PERMUTED, permutedLoadNextExample);
  registerExampleModeType("RANDOMIZED", RANDOMIZED, randomizedLoadNextExample);
  registerExampleModeType("PROBABILISTIC", PROBABILISTIC,
			  probabilisticLoadNextExample);
  registerExampleModeType("PIPE", PIPE, pipeLoadNextExample);
  registerExampleModeType("CUSTOM", CUSTOM, customLoadNextExample);
}
