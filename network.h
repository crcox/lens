#ifndef NETWORK_H
#define NETWORK_H

typedef struct network   *Network;
typedef struct group     *Group;
typedef struct groupProc *GroupProc;
typedef struct unit      *Unit;
typedef struct block     *Block;
typedef struct link      *Link;
typedef struct link2     *Link2;
typedef struct rootrec   *RootRec;

#include <stdlib.h>
#include "example.h"
#include "graph.h"
#include "object.h"
#include "extension.h"


/* This holds all of the user-accessible global stuff.
   There is a global Root pointer to it and each network has a copy. */
struct rootrec {
  int numNetworks;
  Network *net;
  int numExampleSets;
  ExampleSet *set;
  int numGraphs;
  Graph *graph;
};


struct network {
  char      *name;
  int        num;
  mask       type;

  int        numGroups;
  Group     *group;
  int        numUnits;
  int        numInputs;
  Unit      *input;
  int        numOutputs;
  Unit      *output;
  int        numLinks;
  NetExt     ext;
  RootRec    root;

  ExampleSet trainingSet;
  ExampleSet testingSet;
  Example    currentExample;

  int        timeIntervals;
  int        ticksPerInterval;
  int        maxTicks;
  int        historyLength;
  int        backpropTicks;

  int        totalUpdates;
  real       error;
  real       weightCost;
  real       outputCost;
  real       gradientLinearity;

  int        exampleHistoryStart;
  int        ticksOnExample;
  int        currentTick;
  flag       inGracePeriod;            /* hidden */
  int       *eventHistory;
  flag      *resetHistory;

  int        numUpdates;
  int        batchSize;
  int        reportInterval;
  real       criterion;
  real       trainGroupCrit;
  real       testGroupCrit;
  flag       groupCritRequired;
  int        minCritBatches;
  flag       pseudoExampleFreq;

  mask       algorithm;                /* hidden */
  real       learningRate;
#ifdef ADVANCED
  real       rateIncrement;
  real       rateDecrement;
#endif /* ADVANCED */
  real       momentum;
  real       adaptiveGainRate;
  real       weightDecay;
  real       gainDecay;
  real       outputCostStrength;
  real       outputCostPeak;
  real       targetRadius;
  real       zeroErrorRadius;

  real       dt;
  real       gain;
  real       ternaryShift;
  real       clampStrength;
  real       initOutput;
  real       initInput;
  real       initGain;
  real       finalGain;
  real       annealTime;

  real       randMean;
  real       randRange;
  real       noiseRange;

  flag       autoPlot;
  int        plotCols;
  int        plotRows;
  int        unitCellSize;
  int        unitCellSpacing;
  int        unitUpdates;              /* hidden */
  int        unitDisplayValue;         /* hidden */
  Unit       unitDisplayUnit;          /* hidden */
  Unit       unitLocked;               /* hidden */
  int        unitDisplaySet;           /* hidden */
  real       unitTemp;                 /* hidden */
  double     unitLogTemp;              /* hidden */
  int        unitPalette;              /* hidden */
  int        unitExampleProc;          /* hidden */

  int        linkCellSize;
  int        linkCellSpacing;
  int        linkUpdates;              /* hidden */
  int        linkDisplayValue;         /* hidden */
  real       linkTemp;                 /* hidden */
  double     linkLogTemp;              /* hidden */
  int        linkPalette;              /* hidden */
  Unit       linkLockedUnit;           /* hidden */
  int        linkLockedNum;            /* hidden */

  Tcl_Obj   *preEpochProc;
  Tcl_Obj   *postEpochProc;
  Tcl_Obj   *postUpdateProc;

  Tcl_Obj   *preExampleProc;
  Tcl_Obj   *postExampleProc;
  Tcl_Obj   *preExampleBackProc;

  Tcl_Obj   *preEventProc;
  Tcl_Obj   *postEventProc;

  Tcl_Obj   *preTickProc;
  Tcl_Obj   *postTickProc;
  Tcl_Obj   *preTickBackProc;

  Tcl_Obj   *sigUSR1Proc;
  Tcl_Obj   *sigUSR2Proc;

  char      *outputFileName;
  Tcl_Channel outputFile;               /* hidden */
  flag       binaryOutputFile;

  /* Update and training functions (all hidden). */
  flag     (*netTrain)(void);
  flag     (*netTrainBatch)(flag *allCorrect);
  flag     (*netTrainExample)(Example E, flag (*tickProc)(Event),
			      flag *correct);
  flag     (*netTrainExampleBack)(Example E);
  flag     (*netTrainTick)(Event V);

  flag     (*netTestBatch)(int numExamples, flag resetError,
                           flag printResults);
  flag     (*netTestExample)(Example E, flag (*tickProc)(Event),
			     flag *correct);
  flag     (*netTestTick)(Event V);

  flag     (*netRunExampleNum)(int num, ExampleSet S);
  flag     (*netRunExample)(Example E, flag (*tickProc)(Event),
			    flag *correct);
  flag     (*netRunTick)(Event V);

  flag     (*resetNet)(flag randomize);
  flag     (*saveWeights)(Tcl_Obj *fileNameObj, flag binary, int numValues,
			  flag thawed, flag frozen);
  flag     (*loadWeights)(Tcl_Obj *fileNameObj, flag thawed, flag frozen);
  flag     (*writeExample)(Example E);
};


struct group {
  char      *name;
  int        num;
  mask       type;
  mask       inputType;
  GroupProc  inputProcs;
  mask       outputType;
  GroupProc  outputProcs;
  mask       costType;
  GroupProc  costProcs;

  Network    net;
  int        numUnits;
  Unit       unit;
  real      *output;
  real      *outputDeriv;
  int        numIncoming;
  int        numOutgoing;
  GroupExt   ext;

  real       trainGroupCrit;
  real       testGroupCrit;

  real       learningRate;
  real       momentum;
  real       weightDecay;
  real       gainDecay;
  real       targetRadius;
  real       zeroErrorRadius;
  real       outputCostScale;
  real       outputCostPeak;
  real       outputCost;
  real       errorScale;
  real       error;
  real       polaritySum;
  int        polarityNum;

  real       dtScale;
  real       gain;
  real       ternaryShift;
  real       clampStrength;
  real       minOutput;
  real       maxOutput;
  real       initOutput;
  real       initInput;

  real       randMean;
  real       randRange;
  real       noiseRange;
  real     (*noiseProc)(real value, real range); /* hidden */

  flag       showIncoming;
  flag       showOutgoing;
  int        inPosition;                      /* hidden */
  int        outPosition;                     /* hidden */
  int        numColumns;
  real       neighborhood;
  flag       periodicBoundary;

  flag     (*groupCriterionReached)(Group G, real criterion);
};


struct groupProc {
  mask type;
  unsigned int class;                         /* hidden */
  Group group;
  void (*forwardProc)(Group G, GroupProc P);  /* hidden */
  void (*backwardProc)(Group G, GroupProc P); /* hidden */

  real *groupHistoryData;                     /* historyLength */
  real *unitData;                             /* numUnits */
  real **unitHistoryData;                     /* historyLength x numUnits */
  void *otherData;                            /* hidden */

  GroupProc next;
  GroupProc prev;
};

typedef struct copyData {
  Group source;
  int offset;
} *CopyData;

struct unit {
  char      *name;
  int        num;
  mask       type;

  Group      group;
  int        numBlocks;
  Block      block;
  int        numIncoming;
  Link       incoming;
  Link2      incoming2;
  int        numOutgoing;
  UnitExt    ext;

  real       input;
  real       output;
  real       target;
  real       externalInput;
  real       adjustedTarget;
  real       inputDeriv;
  real       outputDeriv;

  real      *inputHistory;
  real      *outputHistory;
  real      *targetHistory;
  real      *outputDerivHistory;

  real       gain;
  real       gainDeriv;
  real       dtScale;

  int        plotRow;
  int        plotCol;
};


struct block {
  mask       type;
  int        numUnits;
  Unit       unit;
  int        groupUnits;
  real      *output;

  real       learningRate;
  real       momentum;
  real       weightDecay;
  real       randMean;
  real       randRange;
  real       min;
  real       max;

  BlockExt   ext;
};


/* Keep this as small as possible.  It is most of the memory usage. */
struct link {
  real       weight;
  real       deriv;
};

struct link2 {
  real       lastWeightDelta;
#ifdef ADVANCED
  real       lastValue;
#endif /* ADVANCED */
};


extern Network Net;              /* This is the current network */
extern RootRec Root;             /* Holds the arrays of networks and sets */


/*********************************** Macros *********************************/
#define IN_BLOCK(U, B) (((U) >= (B)->unit) && \
			((U) < ((B)->unit + (B)->numUnits)))
#define IN_GROUP(B, G) (((B)->unit >= (G)->unit) && \
			((B)->unit < ((G)->unit + (G)->numUnits)))

#define FOR_EACH_GROUP(proc) {\
  int g; Group G;\
  for (g = 0; g < Net->numGroups; g++) {\
    G = Net->group[g];\
    {proc;}}}

#define FOR_EACH_GROUP_BACK(proc) {\
  int g; Group G;\
  for (g = Net->numGroups - 1; g >= 0; g--) {\
    G = Net->group[g];\
    {proc;}}}

#define FOR_EACH_GROUP_IN_RANGE(first, last, proc) {\
  int g; Group G;\
  for (g = first; g <= last; g++) {\
    G = Net->group[g];\
    {proc;}}}

#define FOR_EACH_GROUP_IN_RANGE_BACK(last, first, proc) {\
  int g; Group G;\
  for (g = last; g >= first; g--) {\
    G = Net->group[g];\
    {proc;}}}

#define FOR_EACH_GROUP_IN_LIST(_list, _proc) {\
  char *_l = _list; flag _code;\
  if (!strcmp(_list, "*")) {\
    FOR_EACH_GROUP(_proc);\
  } else {\
    Group G;\
    String _name = newString(32);\
    while ((_l = readName(_l, _name, &_code))) {\
      if (_code == TCL_ERROR) return TCL_ERROR;\
      if (!(G = lookupGroup(_name->s)))\
        return warning("%s: group \"%s\" doesn't exist", commandName, _name->s);\
      _proc;\
    }\
    freeString(_name);\
  }}


#define FOR_EACH_UNIT(G, proc) {\
  Unit U; Unit sU;\
  if (G->type & LESIONED) {\
    for (U = G->unit, sU = U + G->numUnits; U < sU; U++)\
      if (!(U->type & LESIONED)) {proc;}\
  } else {\
    for (U = G->unit, sU = U + G->numUnits; U < sU; U++)\
      {proc;}}}

#define FOR_EACH_UNIT2(G, proc) {\
  int u; Unit U; Unit sU;\
  if (G->type & LESIONED) {\
    for (u = 0, U = G->unit, sU = U + G->numUnits; U < sU; u++, U++)\
      if (!(U->type & LESIONED)) {proc;}\
  } else {\
    for (u = 0, U = G->unit, sU = U + G->numUnits; U < sU; u++, U++)\
      {proc;}}}

#define FOR_EVERY_UNIT(G, proc) {\
  int u; Unit U; Unit sU;\
  for (u = 0, U = G->unit, sU = U + G->numUnits; U < sU; u++, U++)\
    {proc;}}

#define FOR_EACH_UNIT_IN_LIST(_list, _proc) {\
  char *_l = _list; flag _code;\
  if (!strcmp(_list, "*")) {\
    FOR_EACH_GROUP(FOR_EACH_UNIT(G, _proc));\
  } else {\
    Unit U;\
    String _name = newString(32);\
    while ((_l = readName(_l, _name, &_code))) {\
      if (_code == TCL_ERROR) return TCL_ERROR;\
      if (!(U = lookupUnit(_name->s)))\
        return warning("%s: unit \"%s\" doesn't exist", Tcl_GetStringFromObj(objv[0], NULL), _name->s);\
      _proc;\
    }\
    freeString(_name);\
  }}


#define FOR_EACH_BLOCK(U, proc) {\
  int b; Block B;\
  for (b = 0; b < U->numBlocks; b++) {\
    B = U->block + b;\
    {proc;}}}

#define FOR_EACH_LINK(U, proc) {\
  Link L; Link sL;\
  for (L = U->incoming, sL = L + U->numIncoming; L < sL; L++)\
    {proc;}}


#define FOR_EACH_GRAPH(proc) {\
  int g; Graph G;\
  for (g = 0; g < Root->numGraphs; g++) {\
    if ((G = Root->graph[g])) {proc;}}}

#define FOR_EACH_GRAPH_IN_LIST(_list, _proc) {\
  char *_l = _list; flag _code;\
  if (!strcmp(_list, "*")) {\
    FOR_EACH_GRAPH(_proc);\
  } else {\
    Graph G;\
    String _name = newString(32);\
    while ((_l = readName(_l, _name, &_code))) {\
      int g = atoi(_name->s);\
      if (_code == TCL_ERROR) return TCL_ERROR;\
      if (g >= Root->numGraphs || !(G = Root->graph[g]))\
        return warning("%s: graph \"%s\" doesn't exist", commandName, _name->s);\
      _proc;\
    }\
    freeString(_name);\
  }}


#define FOR_EACH_TRACE(G, proc) {\
  int t; Trace T;\
  for (t = 0; t < G->numTraces; t++) {\
    if ((T = G->trace[t])) {proc;}}}

#define FOR_EACH_TRACE_IN_LIST(G, _list, _proc) {\
  char *_l = _list; flag _code;\
  if (!strcmp(_list, "*")) {\
    FOR_EACH_TRACE(G, _proc);\
  } else {\
    Trace T;\
    String _name = newString(32);\
    while ((_l = readName(_l, _name, &_code))) {\
      int t = atoi(_name->s);\
      if (_code == TCL_ERROR) return TCL_ERROR;\
      if (t >= G->numTraces || !(T = G->trace[t]))\
        return warning("%s: trace \"%s\" doesn't exist in graph %d", commandName,\
                       _name->s, G->num);\
       _proc;\
    }\
    freeString(_name);\
  }}


#define FOR_EACH_STRING_IN_LIST(_list, _proc) {\
  char *_l = _list; flag _code;\
  String S = newString(32);\
  while ((_l = readName(_l, S, &_code))) {\
    if (_code == TCL_ERROR) return TCL_ERROR;\
    _proc;\
  }\
  freeString(S);\
}


#define HISTORY_INDEX(tick) \
     ((Net->historyLength) ? \
     (((tick) + Net->exampleHistoryStart + (Net->historyLength << 10)) \
      % Net->historyLength) : 0)
#define SET_HISTORY(U, array, index, value) \
     if ((U)->array) (U)->array[index] = (value)
#define GET_HISTORY(U, array, index) \
     (((U)->array) ? (U)->array[index] : NaN)


#define STORE_TO_HISTORY(G, value, array, tick) {\
  int index = HISTORY_INDEX(tick);\
  FOR_EACH_UNIT(G, if (U->array) U->array[index] = U->value;)}

#define STORE_TO_OTHER_HISTORY(G, value, H, array, tick) {\
  int index = HISTORY_INDEX(tick);\
  FOR_EACH_UNIT2(H, if (U->array) U->array[index] = G->unit[u].value;)}

#define RESTORE_FROM_HISTORY(G, array, tick, value) {\
  int index = HISTORY_INDEX(tick);\
  FOR_EACH_UNIT(G, U->value = (U->array) ? U->array[index] : NaN;)}

#define RESTORE_FROM_OTHER_HISTORY(G, array, tick, H, value) {\
  int index = HISTORY_INDEX(tick);\
  FOR_EACH_UNIT2(G, H->unit[u].value = (U->array) ? U->array[index] : NaN;)}

#define SET_VALUES(G, value, val) {\
  FOR_EACH_UNIT(G, U->value = val);}

#define COPY_VALUES(F, valueF, T, valueT) {\
  if (F->numUnits > T->numUnits)\
    error("COPY_VALUES used with mis-matched groups");\
  FOR_EACH_UNIT2(F, T->unit[u].valueT = U->valueF);}

#define COPY_HISTORY(F, arrayF, tickF, T, arrayT, tickT) {\
  int indexF = HISTORY_INDEX(tickF), indexT = HISTORY_INDEX(tickF);\
  if (F->numUnits > T->numUnits)\
    error("COPY_HISTORY used with mis-matched groups");\
  FOR_EACH_UNIT2(F, T->unit[u].arrayT[indexT] = U->arrayF[indexF]);}

#define CLEAR_HISTORY(G, tick) {\
  int index = HISTORY_INDEX(tick);\
  FOR_EACH_UNIT(G, SET_HISTORY(U, outputHistory, index, NaN));\
  if (G->costType & ERROR_MASKS)\
    FOR_EACH_UNIT(G, SET_HISTORY(U, targetHistory, index, NaN));}

#define STORE_HISTORY(G, tick) {\
  STORE_TO_HISTORY(G, output, outputHistory, tick);\
  if (G->costType & ERROR_MASKS)\
    STORE_TO_HISTORY(G, target, targetHistory, tick);}

#define RUN_PROC(proc) {\
  if (Net->proc && Tcl_EvalObjEx(Interp, Net->proc, TCL_EVAL_GLOBAL) \
    != TCL_OK) return error(Tcl_GetStringResult(Interp));}


extern Unit lookupUnit(char *name);
extern Unit lookupUnitNum(int g, int u);
extern Group lookupGroup(char *name);
extern Network lookupNet(char *name);
extern flag isGroupList(char *s);
extern flag isUnitList(char *s);

extern void appendProc(Group G, unsigned int class, mask type);
extern void prependProc(Group G, unsigned int class, mask type);
extern void removeProc(Group G, unsigned int class, mask type);
extern flag finalizeGroupType(Group G, flag wasOutput, flag wasInput);
extern void changeGroupType(Group G, unsigned int class, mask type,
			    unsigned int mode);
extern flag setGroupType(Group G, int numTypes, unsigned int *typeClass,
			 mask *type, unsigned int *typeMode);
extern flag addGroup(char *name, int numTypes, unsigned int *typeClass,
		     mask *type, unsigned int *typeMode, int numUnits);

extern flag useNet(Network N);
extern flag addNet(char *name, mask type, int numTimeIntervals,
		   int numTicksPerInterval);
extern flag setTime(Network N, int numTimeIntervals, int numTicksPerInterval,
		    int historyLength, flag setDT);
extern flag orderGroups(int argc, char *argv[]);
extern flag orderGroupsObj(int objc, Tcl_Obj *objv[]);
extern flag deleteGroup(Group G);
extern flag deleteNet(Network N);
extern flag deleteAllNets(void);

extern flag copyUnitValues(Group from, int fromoff, Group to, int tooff);
extern flag setUnitValues(Group G, int offset, real value);
extern flag standardResetNet(flag randomize);
extern flag resetDerivs(void);

extern flag lesionUnit(Unit U);
extern flag healUnit(Unit U);
#ifdef JUNK
extern flag printGroupUnitValues(Tcl_Channel channel, Group G, int *field,
				 char **format, int numFields);
#endif
extern flag optimizeNet(void);

#endif /* NETWORK_H */
