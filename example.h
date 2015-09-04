#ifndef EXAMPLE_H
#define EXAMPLE_H

typedef struct exampleSet *ExampleSet;
typedef struct example    *Example;
typedef struct event      *Event;
typedef struct range      *Range;

#include "network.h"
#include "extension.h"
#include "defaults.h"


struct exampleSet {
  char      *name;
  int        num;
  mask       mode;

  int        numExamples;
  int        numEvents;
  ExSetExt   ext;

  Example   *example;
  Example   *permuted;
  int        currentExampleNum;
  Example    currentExample;
  Example    firstExample;
  Example    lastExample;
  Example    pipeExample;
  ParseRec   pipeParser;
  flag       pipeLoop;
  int        pipeExampleNum;

  Tcl_Obj   *proc;
  Tcl_Obj   *chooseExample;
  real       maxTime;
  real       minTime;
  real       graceTime;
  real       defaultInput;
  real       activeInput;
  real       defaultTarget;
  real       activeTarget;
  short      numGroupNames; /* hidden */
  short      maxGroupNames; /* hidden */
  char     **groupName;     /* hidden */

  flag     (*loadEvent)(Event V);
  flag     (*loadExample)(Example E);
  flag     (*nextExample)(ExampleSet S);
};


struct example {
  char      *name;
  int        num;
  int        numEvents;
  Event      event;
  ExampleSet set;
  Example    next;
  ExampleExt ext;

  real       frequency;
  real       probability;
  Tcl_Obj   *proc;
};


struct event {
  Range      input;
  flag       sharedInputs;
  Range      target;
  flag       sharedTargets;

  real       maxTime;
  real       minTime;
  real       graceTime;
  real       defaultInput;
  real       activeInput;
  real       defaultTarget;
  real       activeTarget;

  Tcl_Obj   *proc;
  Example    example;
  EventExt   ext;
};


struct range {
  char      *groupName;         /* If null, unit offsets are for the net */
  int        numUnits;
  int        firstUnit;         /* Only used for dense encodings */
  real      *val;               /* Only used for dense encodings */
  real       value;             /* Only used for sparse encodings */
  int       *unit;              /* Only used for sparse encodings */

  Range next;
};

#define FOR_EACH_SET(proc) {\
  int s; ExampleSet S;\
  for (s = 0; s < Root->numExampleSets; s++) {\
    S = Root->set[s];\
    {proc;}}}

#define FOR_EACH_SET_IN_LIST(list, proc) {\
  char *l = list; flag code;\
  if (!strcmp(list, "*")) {\
    FOR_EACH_SET(proc);\
  } else {\
    ExampleSet S;\
    String _name = newString(32);\
    while ((l = readName(l, _name, &code))) {\
      if (code == TCL_ERROR) return TCL_ERROR;\
      if (!(S = lookupExampleSet(_name->s)))\
        return warning("%s: example set \"%s\" doesn't exist", commandName, _name->s);\
      proc;\
    }\
    freeString(_name);\
  }}

extern flag useTrainingSet(ExampleSet S);
extern flag useTestingSet(ExampleSet S);
extern ExampleSet lookupExampleSet(char *name);
extern flag loadExamples(char *setName, Tcl_Obj *fileNameObj,
			 int mode, int numExamples);
extern flag writeExampleFile(ExampleSet S, Tcl_Obj *fileNameObj, flag append);
extern flag writeBinaryExampleFile(ExampleSet S, Tcl_Obj *fileNameObj);
extern flag openNetOutputFile(Tcl_Obj *fileNameObj, flag binary, flag append);
extern flag closeNetOutputFile(void);
extern void groupWriteBinaryValues(Group G, int tick, Tcl_Channel channel);
extern void groupWriteTextValues(Group G, int tick, Tcl_Channel channel);
extern flag standardWriteExample(Example E);
extern flag deleteExampleSet(ExampleSet S);
extern flag deleteAllExampleSets(void);
extern flag moveExamples(ExampleSet from, char *toName, int first, int num,
			 real proportion, flag copy);
/*extern flag moveAllExamples(ExampleSet from, char *toName);*/
extern flag resetExampleSet(ExampleSet S);
extern flag exampleSetMode(ExampleSet S, mask mode);
extern flag loadDenseRange(Range L, Unit *netUnit, int numNetUnits, mask type);
extern flag loadSparseRange(Range L, Unit *netUnit, int numNetUnits,
			    int offset);
extern void injectExternalInputNoise(void);
extern flag standardLoadEvent(Event V);
extern flag standardLoadExample(Example E);
extern flag loadNextExample(ExampleSet S);
extern void registerExampleModeTypes(void);

extern void registerExampleSet(ExampleSet S);
extern ExampleSet newExampleSet(char *name);
extern Example newExample(ExampleSet S);
extern void initEvent(Event V, Example E);
extern Range newRange(Event V, Range L, flag doingInputs);

#endif /* EXAMPLE_H */
