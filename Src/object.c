#include <string.h>
#include "system.h"
#include "util.h"
#include "type.h"
#include "network.h"
#include "connect.h"
#include "example.h"
#include "object.h"

ObjInfo RootInfo;
ObjInfo NetInfo;
ObjInfo GroupInfo;
ObjInfo GroupProcInfo;
ObjInfo UnitInfo;
ObjInfo BlockInfo;
ObjInfo LinkInfo;
ObjInfo Link2Info;
ObjInfo ExampleSetInfo;
ObjInfo ExampleInfo;
ObjInfo EventInfo;
ObjInfo RangeInfo;
ObjInfo GraphInfo;
ObjInfo TraceInfo;
ObjInfo PointInfo;

ObjInfo IntInfo;             /* int */
ObjInfo RealInfo;            /* real */
ObjInfo DoubleInfo;          /* double */
ObjInfo FlagInfo;            /* flag */
ObjInfo MaskInfo;            /* mask */
ObjInfo StringInfo;          /* char * */
ObjInfo TclObjInfo;          /* TclObj */

/*****************************************************************************/
static void rootName(void *root, char *dest) {
  sprintf(dest, "all nets and sets");
}

int getRootNumNets(void *R) {
  return ((RootRec) R)->numNetworks;
}

int getRootNumSets(void *R) {
  return ((RootRec) R)->numExampleSets;
}

int getRootNumGraphs(void *R) {
  return ((RootRec) R)->numGraphs;
}

static void initRootInfo(void) {
  RootRec R;
  addMember(RootInfo, "numNetworks", OBJ, OFFSET(R, numNetworks), FALSE, 
	    0, 0, IntInfo);
  addMember(RootInfo, "net", OBJPA, OFFSET(R, net), FALSE, 
	    getRootNumNets, 0, NetInfo);
  addMember(RootInfo, "numExampleSets", OBJ, OFFSET(R, numExampleSets), FALSE, 
	    0, 0, IntInfo);
  addMember(RootInfo, "set", OBJPA, OFFSET(R, set), FALSE, 
	    getRootNumSets, 0, ExampleSetInfo);
  addMember(RootInfo, "numGraphs", OBJ, OFFSET(R, numGraphs), FALSE, 
	    0, 0, IntInfo);
  addMember(RootInfo, "graph", OBJPA, OFFSET(R, graph), FALSE, 
	    getRootNumGraphs, 0, GraphInfo);
  addMember(RootInfo, "root", OBJ, 0, FALSE,
	    0, 0, RootInfo);
}

/*****************************************************************************/
static void netName(void *net, char *dest) {
  Network N = (Network) net;
  if (N && N->name)
    /*    strcpy(dest, N->name);*/
    sprintf(dest, "%s", N->name);
  else dest[0] = '\0';
}

int getNetNumGroups(void *N) {
  return ((Network) N)->numGroups;
}
int getNetNumInputs(void *N) {
  return ((Network) N)->numInputs;
}
int getNetNumOutputs(void *N) {
  return ((Network) N)->numOutputs;
}
int getNetMaxTicks(void *N) {
  return ((Network) N)->maxTicks;
}
int getNetHistoryLength(void *N) {
  return ((Network) N)->historyLength;
}

static void initNetInfo(void) {
  Network N;
  addMember(NetInfo, "name", OBJ, OFFSET(N, name), TRUE, 0, 0, StringInfo);

  addMember(NetInfo, "num",  OBJ, OFFSET(N, num), FALSE, 0, 0, IntInfo);

  addMember(NetInfo, "type", OBJ, OFFSET(N, type), FALSE, 0, 0, MaskInfo);

  addSpacer(NetInfo);

  addMember(NetInfo, "numGroups", OBJ, OFFSET(N, numGroups), FALSE, 
	    0, 0, IntInfo);
  addMember(NetInfo, "group", OBJPA, OFFSET(N, group), FALSE,
	    getNetNumGroups, 0, GroupInfo);
  addMember(NetInfo, "numUnits", OBJ, OFFSET(N, numUnits), FALSE, 
	    0, 0, IntInfo);
  addMember(NetInfo, "numInputs", OBJ, OFFSET(N, numInputs), FALSE, 
	    0, 0, IntInfo);
  addMember(NetInfo, "input", OBJPA, OFFSET(N, input), FALSE, 
	    getNetNumInputs, 0, UnitInfo);
  addMember(NetInfo, "numOutputs", OBJ, OFFSET(N, numOutputs), FALSE, 
	    0, 0, IntInfo);
  addMember(NetInfo, "output", OBJPA, OFFSET(N, output), FALSE, 
	    getNetNumOutputs, 0, UnitInfo);
  addMember(NetInfo, "numLinks", OBJ, OFFSET(N, numLinks), FALSE, 
	    0, 0, IntInfo);
  addMember(NetInfo, "ext", OBJP, OFFSET(N, ext), FALSE, 
	    0, 0, NetExtInfo);
  addMember(NetInfo, "root", OBJP, OFFSET(N, root), FALSE, 
	    0, 0, RootInfo);
  addSpacer(NetInfo);

  addMember(NetInfo, "trainingSet", OBJP, OFFSET(N, trainingSet), FALSE, 
	    0, 0, ExampleSetInfo);
  addMember(NetInfo, "testingSet", OBJP, OFFSET(N, testingSet), FALSE, 
	    0, 0, ExampleSetInfo);
  addMember(NetInfo, "currentExample", OBJP, OFFSET(N, currentExample), FALSE,
	    0, 0, ExampleInfo);
  addSpacer(NetInfo);

  addMember(NetInfo, "timeIntervals", OBJ, OFFSET(N, timeIntervals), 
	    FALSE, 0, 0, IntInfo);
  addMember(NetInfo, "ticksPerInterval", OBJ, 
	    OFFSET(N, ticksPerInterval), FALSE, 0, 0, IntInfo);
  addMember(NetInfo, "maxTicks", OBJ, OFFSET(N, maxTicks), FALSE, 
	    0, 0, IntInfo);
  addMember(NetInfo, "historyLength", OBJ, OFFSET(N, historyLength), FALSE, 
	    0, 0, IntInfo);
  addMember(NetInfo, "backpropTicks", OBJ, OFFSET(N, backpropTicks), TRUE, 
	    0, 0, IntInfo);
  addSpacer(NetInfo);

  addMember(NetInfo, "totalUpdates", OBJ, OFFSET(N, totalUpdates), TRUE, 
	    0, 0, IntInfo);
  addMember(NetInfo, "error", OBJ, OFFSET(N, error), TRUE, 
	    0, 0, RealInfo);
  addMember(NetInfo, "weightCost", OBJ, OFFSET(N, weightCost), TRUE, 
	    0, 0, RealInfo);
  addMember(NetInfo, "outputCost", OBJ, OFFSET(N, outputCost), TRUE, 
	    0, 0, RealInfo);
  addMember(NetInfo, "gradientLinearity", OBJ, OFFSET(N, gradientLinearity), 
	    TRUE, 0, 0, RealInfo);
  addSpacer(NetInfo);

  addMember(NetInfo, "exampleHistoryStart", OBJ, 
	    OFFSET(N, exampleHistoryStart), FALSE, 0, 0, IntInfo);
  addMember(NetInfo, "ticksOnExample", OBJ, OFFSET(N, ticksOnExample), FALSE, 
	    0, 0, IntInfo);
  addMember(NetInfo, "currentTick", OBJ, OFFSET(N, currentTick), FALSE, 
	    0, 0, IntInfo);
  addMember(NetInfo, "eventHistory", OBJA, OFFSET(N, eventHistory), FALSE, 
	    getNetMaxTicks, 0, IntInfo);
  addMember(NetInfo, "resetHistory", OBJA, OFFSET(N, resetHistory), FALSE, 
	    getNetHistoryLength, 0, FlagInfo);
  addSpacer(NetInfo);

  addMember(NetInfo, "numUpdates", OBJ, OFFSET(N, numUpdates), TRUE, 
	    0, 0, IntInfo);
  addMember(NetInfo, "batchSize", OBJ, OFFSET(N, batchSize), TRUE, 
	    0, 0, IntInfo);
  addMember(NetInfo, "reportInterval", OBJ, OFFSET(N, reportInterval), TRUE, 
	    0, 0, IntInfo);
  addMember(NetInfo, "criterion", OBJ, OFFSET(N, criterion), TRUE, 
	    0, 0, RealInfo);
  addMember(NetInfo, "trainGroupCrit", OBJ, OFFSET(N, trainGroupCrit), TRUE, 
	    0, 0, RealInfo);
  addMember(NetInfo, "testGroupCrit", OBJ, OFFSET(N, testGroupCrit), TRUE, 
	    0, 0, RealInfo);
  addMember(NetInfo, "groupCritRequired", OBJ, OFFSET(N, groupCritRequired), 
	    TRUE, 0, 0, FlagInfo);
  addMember(NetInfo, "minCritBatches", OBJ, OFFSET(N, minCritBatches), TRUE, 
	    0, 0, IntInfo);
  addMember(NetInfo, "pseudoExampleFreq", OBJ, OFFSET(N, pseudoExampleFreq), 
	    TRUE, 0, 0, FlagInfo);
  addSpacer(NetInfo);

  addMember(NetInfo, "learningRate", OBJ, OFFSET(N, learningRate), TRUE, 
	    0, 0, RealInfo);
#ifdef ADVANCED
  addMember(NetInfo, "rateIncrement", OBJ, OFFSET(N, rateIncrement), TRUE, 
	    0, 0, RealInfo);
  addMember(NetInfo, "rateDecrement", OBJ, OFFSET(N, rateDecrement), TRUE, 
	    0, 0, RealInfo);
#endif /* ADVANCED */
  addMember(NetInfo, "momentum", OBJ, OFFSET(N, momentum), TRUE, 
	    0, 0, RealInfo);
  addMember(NetInfo, "adaptiveGainRate", OBJ, OFFSET(N, adaptiveGainRate), 
	    TRUE, 0, 0, RealInfo);
  addMember(NetInfo, "weightDecay", OBJ, OFFSET(N, weightDecay), TRUE, 
	    0, 0, RealInfo);
  addMember(NetInfo, "gainDecay", OBJ, OFFSET(N, gainDecay), TRUE, 
	    0, 0, RealInfo);
  addMember(NetInfo, "outputCostStrength", OBJ, OFFSET(N, outputCostStrength),
	    TRUE, 0, 0, RealInfo);
  addMember(NetInfo, "outputCostPeak", OBJ, OFFSET(N, outputCostPeak), TRUE, 
	    0, 0, RealInfo);
  addMember(NetInfo, "targetRadius", OBJ, OFFSET(N, targetRadius), TRUE,
	    0, 0, RealInfo);
  addMember(NetInfo, "zeroErrorRadius", OBJ, OFFSET(N, zeroErrorRadius), TRUE,
	    0, 0, RealInfo);
  addSpacer(NetInfo);

  addMember(NetInfo, "dt", OBJ, OFFSET(N, dt), TRUE, 
	    0, 0, RealInfo);
  addMember(NetInfo, "gain", OBJ, OFFSET(N, gain), TRUE,
	    0, 0, RealInfo);
  addMember(NetInfo, "ternaryShift", OBJ, OFFSET(N, ternaryShift), TRUE,
	    0, 0, RealInfo);
  addMember(NetInfo, "clampStrength", OBJ, OFFSET(N, clampStrength), TRUE,
	    0, 0, RealInfo);
  addMember(NetInfo, "initOutput", OBJ, OFFSET(N, initOutput), TRUE, 
	    0, 0, RealInfo);
  addMember(NetInfo, "initInput", OBJ, OFFSET(N, initInput), TRUE, 
	    0, 0, RealInfo);
  addMember(NetInfo, "initGain", OBJ, OFFSET(N, initGain), TRUE,
	    0, 0, RealInfo);
  addMember(NetInfo, "finalGain", OBJ, OFFSET(N, finalGain), TRUE,
	    0, 0, RealInfo);
  addMember(NetInfo, "annealTime", OBJ, OFFSET(N, annealTime), TRUE,
	    0, 0, RealInfo);
  addSpacer(NetInfo);

  addMember(NetInfo, "randMean", OBJ, OFFSET(N, randMean), TRUE, 
	    0, 0, RealInfo);
  addMember(NetInfo, "randRange", OBJ, OFFSET(N, randRange), TRUE, 
	    0, 0, RealInfo);
  addMember(NetInfo, "noiseRange", OBJ, OFFSET(N, noiseRange), TRUE, 
	    0, 0, RealInfo);
  addSpacer(NetInfo);

  addMember(NetInfo, "autoPlot", OBJ, OFFSET(N, autoPlot), TRUE, 
	    0, 0, FlagInfo);
  addMember(NetInfo, "plotCols", OBJ, OFFSET(N, plotCols), TRUE, 
	    0, 0, IntInfo);
  addMember(NetInfo, "plotRows", OBJ, OFFSET(N, plotRows), TRUE, 
	    0, 0, IntInfo);
  addMember(NetInfo, "unitCellSize", OBJ, OFFSET(N, unitCellSize), TRUE, 
	    0, 0, IntInfo);
  addMember(NetInfo, "unitCellSpacing", OBJ, OFFSET(N, unitCellSpacing), TRUE,
	    0, 0, IntInfo);
  addMember(NetInfo, "linkCellSize", OBJ, OFFSET(N, linkCellSize), TRUE, 
	    0, 0, IntInfo);
  addMember(NetInfo, "linkCellSpacing", OBJ, OFFSET(N, linkCellSpacing), TRUE,
	    0, 0, IntInfo);
  addSpacer(NetInfo);

  addMember(NetInfo, "preEpochProc", OBJ, OFFSET(N, preEpochProc), TRUE,
	    0, 0, TclObjInfo);
  addMember(NetInfo, "postEpochProc", OBJ, OFFSET(N, postEpochProc), TRUE,
	    0, 0, TclObjInfo);
  addMember(NetInfo, "postUpdateProc", OBJ, OFFSET(N, postUpdateProc), TRUE,
	    0, 0, TclObjInfo);
  addMember(NetInfo, "preExampleProc", OBJ, OFFSET(N, preExampleProc), TRUE,
	    0, 0, TclObjInfo);
  addMember(NetInfo, "postExampleProc", OBJ, OFFSET(N, postExampleProc), TRUE,
	    0, 0, TclObjInfo);
  addMember(NetInfo, "preExampleBackProc", OBJ, OFFSET(N, preExampleBackProc),
	    TRUE, 0, 0, TclObjInfo);
  addMember(NetInfo, "preEventProc", OBJ, OFFSET(N, preEventProc), TRUE,
	    0, 0, TclObjInfo);
  addMember(NetInfo, "postEventProc", OBJ, OFFSET(N, postEventProc), TRUE,
	    0, 0, TclObjInfo);
  addMember(NetInfo, "preTickProc", OBJ, OFFSET(N, preTickProc), TRUE,
	    0, 0, TclObjInfo);
  addMember(NetInfo, "postTickProc", OBJ, OFFSET(N, postTickProc), TRUE,
	    0, 0, TclObjInfo);
  addMember(NetInfo, "preTickBackProc", OBJ, OFFSET(N, preTickBackProc), TRUE,
	    0, 0, TclObjInfo);
  addMember(NetInfo, "sigUSR1Proc", OBJ, OFFSET(N, sigUSR1Proc), TRUE,
	    0, 0, TclObjInfo);
  addMember(NetInfo, "sigUSR2Proc", OBJ, OFFSET(N, sigUSR2Proc), TRUE,
	    0, 0, TclObjInfo);
  addSpacer(NetInfo);
  addMember(NetInfo, "outputFileName", OBJ, OFFSET(N, outputFileName), FALSE,
	    0, 0, StringInfo);
  addMember(NetInfo, "binaryOutputFile", OBJ, OFFSET(N, binaryOutputFile), 
	    FALSE, 0, 0, FlagInfo);
}

/*****************************************************************************/
static void groupName(void *group, char *dest) {
  Group G = (Group) group;
  if (G && G->name)
    sprintf(dest, "%s", G->name);
  else dest[0] = '\0';
}

int getGroupNumUnits(void *G) {
  return ((Group) G)->numUnits;
}

static void initGroupInfo(void) {
  Group G;
  addMember(GroupInfo, "name", OBJ, OFFSET(G, name), TRUE, 
	    0, 0, StringInfo);
  addMember(GroupInfo, "num", OBJ, OFFSET(G, num), FALSE, 
	    0, 0, IntInfo);
  addMember(GroupInfo, "type", OBJ, OFFSET(G, type), FALSE, 
	    0, 0, MaskInfo);
  addMember(GroupInfo, "inputType", OBJ, OFFSET(G, inputType), FALSE, 
	    0, 0, MaskInfo);
  addMember(GroupInfo, "inputProcs", OBJP, OFFSET(G, inputProcs), FALSE,
	    0, 0, GroupProcInfo);
  addMember(GroupInfo, "outputType", OBJ, OFFSET(G, outputType), FALSE, 
	    0, 0, MaskInfo);
  addMember(GroupInfo, "outputProcs", OBJP, OFFSET(G, outputProcs), FALSE,
	    0, 0, GroupProcInfo);
  addMember(GroupInfo, "costType", OBJ, OFFSET(G, costType), FALSE, 
	    0, 0, MaskInfo);
  addMember(GroupInfo, "costProcs", OBJP, OFFSET(G, costProcs), FALSE,
	    0, 0, GroupProcInfo);
  addSpacer(GroupInfo);

  addMember(GroupInfo, "net", OBJP, OFFSET(G, net), FALSE, 
	    0, 0, NetInfo);
  addMember(GroupInfo, "numUnits", OBJ, OFFSET(G, numUnits), FALSE, 
	    0, 0, IntInfo);
  addMember(GroupInfo, "unit", OBJA, OFFSET(G, unit), FALSE, 
	    getGroupNumUnits, 0, UnitInfo);
  addMember(GroupInfo, "output", OBJA, OFFSET(G, output), TRUE, 
	    getGroupNumUnits, 0, RealInfo);
  addMember(GroupInfo, "outputDeriv", OBJA, OFFSET(G, outputDeriv), TRUE, 
	    getGroupNumUnits, 0, RealInfo);
  addMember(GroupInfo, "numIncoming", OBJ, OFFSET(G, numIncoming), FALSE, 
	    0, 0, IntInfo);
  addMember(GroupInfo, "numOutgoing", OBJ, OFFSET(G, numOutgoing), FALSE, 
	    0, 0, IntInfo);
  addMember(GroupInfo, "ext", OBJP, OFFSET(G, ext), FALSE, 
	    0, 0, GroupExtInfo);
  addSpacer(GroupInfo);
  
  addMember(GroupInfo, "trainGroupCrit", OBJ, OFFSET(G, trainGroupCrit), TRUE, 
	    0, 0, RealInfo);
  addMember(GroupInfo, "testGroupCrit", OBJ, OFFSET(G, testGroupCrit), TRUE, 
	    0, 0, RealInfo);
  addSpacer(GroupInfo);

  addMember(GroupInfo, "learningRate", OBJ, OFFSET(G, learningRate), TRUE, 
	    0, 0, RealInfo);
  addMember(GroupInfo, "momentum", OBJ, OFFSET(G, momentum), TRUE, 
	    0, 0, RealInfo);
  addMember(GroupInfo, "weightDecay", OBJ, OFFSET(G, weightDecay), TRUE, 
	    0, 0, RealInfo);
  addMember(GroupInfo, "gainDecay", OBJ, OFFSET(G, gainDecay), TRUE, 
	    0, 0, RealInfo);
  addMember(GroupInfo, "targetRadius", OBJ, OFFSET(G, targetRadius), TRUE,
	    0, 0, RealInfo);
  addMember(GroupInfo, "zeroErrorRadius", OBJ, OFFSET(G, zeroErrorRadius),
	    TRUE, 0, 0, RealInfo);
  addMember(GroupInfo, "outputCostScale", OBJ, 
	    OFFSET(G, outputCostScale), TRUE, 0, 0, RealInfo);
  addMember(GroupInfo, "outputCostPeak", OBJ, OFFSET(G, outputCostPeak), TRUE, 
	    0, 0, RealInfo);
  addMember(GroupInfo, "outputCost", OBJ, OFFSET(G, outputCost), TRUE, 
	    0, 0, RealInfo);
  addMember(GroupInfo, "errorScale", OBJ, OFFSET(G, errorScale), TRUE, 
	    0, 0, RealInfo);
  addMember(GroupInfo, "error", OBJ, OFFSET(G, error), TRUE, 
	    0, 0, RealInfo);
  addMember(GroupInfo, "polaritySum", OBJ, OFFSET(G, polaritySum), TRUE, 
	    0, 0, RealInfo);
  addMember(GroupInfo, "polarityNum", OBJ, OFFSET(G, polarityNum), TRUE, 
	    0, 0, IntInfo);
  addSpacer(GroupInfo);

  addMember(GroupInfo, "dtScale", OBJ, OFFSET(G, dtScale), TRUE, 
	    0, 0, RealInfo);
  addMember(GroupInfo, "gain", OBJ, OFFSET(G, gain), TRUE,
	    0, 0, RealInfo);
  addMember(GroupInfo, "ternaryShift", OBJ, OFFSET(G, ternaryShift), TRUE,
	    0, 0, RealInfo);
  addMember(GroupInfo, "clampStrength", OBJ, OFFSET(G, clampStrength),
	    TRUE, 0, 0, RealInfo);
  addMember(GroupInfo, "minOutput", OBJ, OFFSET(G, minOutput), TRUE, 
	    0, 0, RealInfo);
  addMember(GroupInfo, "maxOutput", OBJ, OFFSET(G, maxOutput), TRUE, 
	    0, 0, RealInfo);
  addMember(GroupInfo, "initOutput", OBJ, OFFSET(G, initOutput), TRUE, 
	    0, 0, RealInfo);
  addMember(GroupInfo, "initInput", OBJ, OFFSET(G, initInput), TRUE, 
	    0, 0, RealInfo);
  addSpacer(GroupInfo);

  addMember(GroupInfo, "randMean", OBJ, OFFSET(G, randMean), TRUE, 
	    0, 0, RealInfo);
  addMember(GroupInfo, "randRange", OBJ, OFFSET(G, randRange), TRUE, 
	    0, 0, RealInfo);
  addMember(GroupInfo, "noiseRange", OBJ, OFFSET(G, noiseRange), TRUE, 
	    0, 0, RealInfo);
  addSpacer(GroupInfo);

  addMember(GroupInfo, "showIncoming", OBJ, OFFSET(G, showIncoming),
	    TRUE, 0, 0, FlagInfo);
  addMember(GroupInfo, "showOutgoing", OBJ, OFFSET(G, showOutgoing),
	    TRUE, 0, 0, FlagInfo);
  addMember(GroupInfo, "numColumns", OBJ, OFFSET(G, numColumns),
	    TRUE, 0, 0, IntInfo);
  addMember(GroupInfo, "neighborhood", OBJ, OFFSET(G, neighborhood),
	    TRUE, 0, 0, RealInfo);
  addMember(GroupInfo, "periodicBoundary", OBJ, OFFSET(G, periodicBoundary),
	    TRUE, 0, 0, FlagInfo);
}

/*****************************************************************************/
static void groupProcName(void *proc, char *dest) {
  GroupProc P = (GroupProc) proc;
  if (P) sprintf(dest, "%s", lookupTypeName(P->type, P->class));
  else dest[0] = '\0';
}

int getProcNumUnits(void *p) {
  GroupProc P = (GroupProc) p;
  if (P->group) return P->group->numUnits;
  else return 0;
}
int getProcHistoryLength(void *p) {
  GroupProc P = (GroupProc) p;
  if (P->group && P->group->net) return P->group->net->historyLength;
  else return 0;
}

static void initGroupProcInfo(void) {
  GroupProc P;
  addMember(GroupProcInfo, "type", OBJ, OFFSET(P, type),
	    FALSE, 0, 0, MaskInfo);
  addMember(GroupProcInfo, "group", OBJP, OFFSET(P, group), 
	    FALSE, 0, 0, GroupInfo);
  addMember(GroupProcInfo, "groupHistoryData", OBJA, 
	    OFFSET(P, groupHistoryData), TRUE, getProcHistoryLength, 0, 
	    RealInfo);
  addMember(GroupProcInfo, "unitData", OBJA, OFFSET(P, unitData),
	    TRUE, getProcNumUnits, 0, RealInfo);
  addMember(GroupProcInfo, "unitHistoryData", OBJAA, 
	    OFFSET(P, unitHistoryData), TRUE, getProcHistoryLength, 
	    getProcNumUnits, RealInfo);
  addSpacer(GroupProcInfo);

  addMember(GroupProcInfo, "next", OBJP, OFFSET(P, next), 
	    FALSE, 0, 0, GroupProcInfo);
  addMember(GroupProcInfo, "prev", OBJP, OFFSET(P, prev), 
	    FALSE, 0, 0, GroupProcInfo);
}

/*****************************************************************************/
static void unitName(void *unit, char *dest) {
  Unit U = (Unit) unit;
  if (U && U->name)
    /* strcpy(dest, U->name);*/
    sprintf(dest, "%s", U->name);
  else dest[0] = '\0';
}

int getUnitNumIncoming(void *U) {
  return ((Unit) U)->numIncoming;
}
int getUnitNumBlocks(void *U) {
  return ((Unit) U)->numBlocks;
}
int getUnitHistoryLength(void *u) {
  Unit U = (Unit) u;
  if (U->group && U->group->net) return U->group->net->historyLength;
  else return 0;
}

static void initUnitInfo(void) {
  Unit U;
  addMember(UnitInfo, "name", OBJ, OFFSET(U, name), TRUE, 
	    0, 0, StringInfo);
  addMember(UnitInfo, "num", OBJ, OFFSET(U, num), FALSE, 
	    0, 0, IntInfo);
  addMember(UnitInfo, "type", OBJ, OFFSET(U, type), FALSE, 
	    0, 0, MaskInfo);
  addSpacer(UnitInfo);

  addMember(UnitInfo, "group", OBJP, OFFSET(U, group), FALSE, 
	    0, 0, GroupInfo);
  addMember(UnitInfo, "numBlocks", OBJ, OFFSET(U, numBlocks), FALSE, 
	    0, 0, IntInfo);
  addMember(UnitInfo, "block", OBJA, OFFSET(U, block), FALSE, 
	    getUnitNumBlocks, 0, BlockInfo);
  addMember(UnitInfo, "numIncoming", OBJ, OFFSET(U, numIncoming), FALSE, 
	    0, 0, IntInfo);
  addMember(UnitInfo, "incoming", OBJA, OFFSET(U, incoming), FALSE, 
	    getUnitNumIncoming, 0, LinkInfo);
  addMember(UnitInfo, "incoming2", OBJA, OFFSET(U, incoming2), FALSE, 
	    getUnitNumIncoming, 0, Link2Info);
  addMember(UnitInfo, "numOutgoing", OBJ, OFFSET(U, numOutgoing), FALSE, 
	    0, 0, IntInfo);
  addMember(UnitInfo, "ext", OBJP, OFFSET(U, ext), FALSE, 
	    0, 0, UnitExtInfo);
  addSpacer(UnitInfo);

  addMember(UnitInfo, "input", OBJ, OFFSET(U, input), TRUE, 
	    0, 0, RealInfo);
  addMember(UnitInfo, "output", OBJ, OFFSET(U, output), TRUE, 
	    0, 0, RealInfo);
  addMember(UnitInfo, "target", OBJ, OFFSET(U, target), TRUE, 
	    0, 0, RealInfo);
  addMember(UnitInfo, "externalInput", OBJ, OFFSET(U, externalInput), TRUE, 
	    0, 0, RealInfo);
  addMember(UnitInfo, "adjustedTarget", OBJ, OFFSET(U, adjustedTarget), TRUE, 
	    0, 0, RealInfo);
  addMember(UnitInfo, "inputDeriv", OBJ, OFFSET(U, inputDeriv), TRUE, 
	    0, 0, RealInfo); 
  addMember(UnitInfo, "outputDeriv", OBJ, OFFSET(U, outputDeriv), TRUE, 
	    0, 0, RealInfo);
  addSpacer(UnitInfo);

  addMember(UnitInfo, "inputHistory", OBJA, OFFSET(U, inputHistory), TRUE, 
	    getUnitHistoryLength, 0, RealInfo);
  addMember(UnitInfo, "outputHistory", OBJA, OFFSET(U, outputHistory), TRUE,
	    getUnitHistoryLength, 0, RealInfo);
  addMember(UnitInfo, "targetHistory", OBJA, OFFSET(U, targetHistory), TRUE,
	    getUnitHistoryLength, 0, RealInfo);
  addMember(UnitInfo, "outputDerivHistory", OBJA, 
	    OFFSET(U, outputDerivHistory), TRUE, getUnitHistoryLength, 0, 
	    RealInfo);
  addSpacer(UnitInfo);

  addMember(UnitInfo, "gain", OBJ, OFFSET(U, gain), TRUE, 
	    0, 0, RealInfo);
  addMember(UnitInfo, "gainDeriv", OBJ, OFFSET(U, gainDeriv), TRUE, 
	    0, 0, RealInfo);
  addMember(UnitInfo, "dtScale", OBJ, OFFSET(U, dtScale), TRUE, 
	    0, 0, RealInfo);

  addMember(UnitInfo, "plotRow", OBJ, OFFSET(U, plotRow), FALSE, 
	    0, 0, IntInfo);
  addMember(UnitInfo, "plotCol", OBJ, OFFSET(U, plotCol), FALSE, 
	    0, 0, IntInfo);
}

/*****************************************************************************/
static void blockName(void *block, char *dest) {
  Block B = (Block) block;
  if (B) sprintf(dest, "%-3d %s", B->numUnits, LinkTypeName[B->type]);
  else dest[0] = '\0';
}

int getBlockNumUnits(void *B) {
  return ((Block) B)->numUnits;
}

static void initBlockInfo(void) {
  Block B;
  addMember(BlockInfo, "type", OBJ, OFFSET(B, type), FALSE, 
	    0, 0, MaskInfo);
  addMember(BlockInfo, "numUnits", OBJ, OFFSET(B, numUnits), FALSE, 
	    0, 0, IntInfo);
  addMember(BlockInfo, "unit", OBJP, OFFSET(B, unit), FALSE,
	    0, 0, UnitInfo);
  addMember(BlockInfo, "groupUnits", OBJ, OFFSET(B, groupUnits), FALSE, 
	    0, 0, IntInfo);
  addMember(BlockInfo, "output", OBJA, OFFSET(B, output), TRUE, 
	    getBlockNumUnits, 0, RealInfo);
  addSpacer(BlockInfo);

  addMember(BlockInfo, "learningRate", OBJ, OFFSET(B, learningRate), TRUE, 
	    0, 0, RealInfo);
  addMember(BlockInfo, "momentum", OBJ, OFFSET(B, momentum), TRUE, 
	    0, 0, RealInfo);
  addMember(BlockInfo, "weightDecay", OBJ, OFFSET(B, weightDecay), TRUE, 
	    0, 0, RealInfo);
  addMember(BlockInfo, "randMean", OBJ, OFFSET(B, randMean), TRUE, 
	    0, 0, RealInfo);
  addMember(BlockInfo, "randRange", OBJ, OFFSET(B, randRange), TRUE, 
	    0, 0, RealInfo);
  addMember(BlockInfo, "min", OBJ, OFFSET(B, min), TRUE, 0, 0, RealInfo);

  addMember(BlockInfo, "max", OBJ, OFFSET(B, max), TRUE, 0, 0, RealInfo);

  addSpacer(BlockInfo);

  addMember(BlockInfo, "ext", OBJP, OFFSET(B, ext), FALSE, 
	    0, 0, BlockExtInfo);
}

/*****************************************************************************/
static void linkName(void *link, char *dest) {
  Link L = (Link) link;
  if (L) sprintf(dest, "%g", L->weight);
}

static void initLinkInfo(void) {
  Link L;
  addMember(LinkInfo, "weight", OBJ, OFFSET(L, weight), TRUE, 
	    0, 0, RealInfo);
  addMember(LinkInfo, "deriv", OBJ, OFFSET(L, deriv), FALSE, 
	    0, 0, RealInfo);
}


static void link2Name(void *link2, char *dest) {
  Link2 M = (Link2) link2;
  if (M) sprintf(dest, "%g", M->lastWeightDelta);
}

static void initLink2Info(void) {
  Link2 M;
  addMember(Link2Info, "lastWeightDelta", OBJ, OFFSET(M, lastWeightDelta), 
	    TRUE, 0, 0, RealInfo);
#ifdef ADVANCED
  addMember(Link2Info, "lastValue", OBJ, OFFSET(M, lastValue), TRUE, 
	    0, 0, RealInfo);
#endif /* ADVANCED */
}


/*****************************************************************************/
static void exampleSetName(void *set, char *dest) {
  ExampleSet S = (ExampleSet) set;
  if (S && S->name) sprintf(dest, "%s", S->name);
  else dest[0] = '\0';
}

int getSetNumExamples(void *S) {
  return ((ExampleSet) S)->numExamples;
}

static void initExampleSetInfo(void) {
  ExampleSet S;
  addMember(ExampleSetInfo, "name", OBJ, OFFSET(S, name), TRUE, 
	    0, 0, StringInfo);
  addMember(ExampleSetInfo, "num", OBJ, OFFSET(S, num), FALSE, 
	    0, 0, IntInfo);
  addMember(ExampleSetInfo, "mode", OBJ, OFFSET(S, mode), FALSE, 
	    0, 0, MaskInfo);
  addSpacer(ExampleSetInfo);

  addMember(ExampleSetInfo, "numExamples", OBJ, OFFSET(S, numExamples), FALSE,
	    0, 0, IntInfo);
  addMember(ExampleSetInfo, "numEvents", OBJ, OFFSET(S, numEvents), FALSE,
	    0, 0, IntInfo);
  addMember(ExampleSetInfo, "ext", OBJP, OFFSET(S, ext), FALSE,
	    0, 0, ExSetExtInfo);
  addSpacer(ExampleSetInfo);

  addMember(ExampleSetInfo, "example", OBJPA, OFFSET(S, example), FALSE, 
	    getSetNumExamples, 0, ExampleInfo);
  addMember(ExampleSetInfo, "permuted", OBJPA, OFFSET(S, permuted), FALSE, 
	    getSetNumExamples, 0, ExampleInfo);
  addMember(ExampleSetInfo, "currentExampleNum", OBJ, 
	    OFFSET(S, currentExampleNum), FALSE, 0, 0, IntInfo);
  addMember(ExampleSetInfo, "currentExample", OBJP, OFFSET(S, currentExample), 
	    FALSE, 0, 0, ExampleInfo);
  addMember(ExampleSetInfo, "firstExample", OBJP, OFFSET(S, firstExample), 
	    FALSE, 0, 0, ExampleInfo);
  addMember(ExampleSetInfo, "lastExample", OBJP, OFFSET(S, lastExample), 
	    FALSE, 0, 0, ExampleInfo);
  addMember(ExampleSetInfo, "pipeExample", OBJP, OFFSET(S, pipeExample), FALSE,
	    0, 0, ExampleInfo);
  addMember(ExampleSetInfo, "pipeLoop", OBJ, OFFSET(S, pipeLoop), TRUE,
	    0, 0, FlagInfo);
  addMember(ExampleSetInfo, "pipeExampleNum", OBJ, OFFSET(S, pipeExampleNum), 
	    FALSE, 0, 0, IntInfo);
  addSpacer(ExampleSetInfo);

  addMember(ExampleSetInfo, "proc", OBJ, OFFSET(S, proc), TRUE,
	    0, 0, TclObjInfo);
  addMember(ExampleSetInfo, "chooseExample", OBJ, OFFSET(S, chooseExample), 
	    TRUE, 0, 0, TclObjInfo);
  addMember(ExampleSetInfo, "maxTime", OBJ, OFFSET(S, maxTime), 
	    TRUE, 0, 0, RealInfo);
  addMember(ExampleSetInfo, "minTime", OBJ, OFFSET(S, minTime), 
	    TRUE, 0, 0, RealInfo);
  addMember(ExampleSetInfo, "graceTime", OBJ, OFFSET(S, graceTime), 
	    TRUE, 0, 0, RealInfo);
  addMember(ExampleSetInfo, "defaultInput", OBJ, OFFSET(S, defaultInput), 
	    TRUE, 0, 0, RealInfo);
  addMember(ExampleSetInfo, "activeInput", OBJ, OFFSET(S, activeInput), 
	    TRUE, 0, 0, RealInfo);
  addMember(ExampleSetInfo, "defaultTarget", OBJ, OFFSET(S, defaultTarget), 
	    TRUE, 0, 0, RealInfo);
  addMember(ExampleSetInfo, "activeTarget", OBJ, OFFSET(S, activeTarget), 
	    TRUE, 0, 0, RealInfo);
}

/*****************************************************************************/
static void exampleName(void *ex, char *dest) {
  Example E = (Example) ex;
  if (E) {
    if (E->name) sprintf(dest, "%s", E->name);
    else sprintf(dest, "%d", E->num);
  } else dest[0] = '\0';
}

int getExampleNumEvents(void *E) {
  return ((Example) E)->numEvents;
}

static void initExampleInfo(void) {
  Example E;
  addMember(ExampleInfo, "name", OBJ, OFFSET(E, name), TRUE, 
	    0, 0, StringInfo);
  addMember(ExampleInfo, "num", OBJ, OFFSET(E, num), FALSE, 
	    0, 0, IntInfo);
  addMember(ExampleInfo, "numEvents", OBJ, OFFSET(E, numEvents), FALSE, 
	    0, 0, IntInfo);
  addMember(ExampleInfo, "event", OBJA, OFFSET(E, event), FALSE, 
	    getExampleNumEvents, 0, EventInfo);
  addMember(ExampleInfo, "set", OBJP, OFFSET(E, set), FALSE,
	    0, 0, ExampleSetInfo);
  addMember(ExampleInfo, "next", OBJP, OFFSET(E, next), FALSE, 
	    0, 0, ExampleInfo);
  addMember(ExampleInfo, "ext", OBJP, OFFSET(E, ext), FALSE, 
	    0, 0, ExampleExtInfo);
  addSpacer(ExampleInfo);

  addMember(ExampleInfo, "frequency", OBJ, OFFSET(E, frequency), TRUE, 
	    0, 0, RealInfo);
  addMember(ExampleInfo, "probability", OBJ, OFFSET(E, probability), FALSE, 
	    0, 0, RealInfo);
  addMember(ExampleInfo, "proc", OBJ, OFFSET(E, proc), TRUE, 
	    0, 0, TclObjInfo);
}

/*****************************************************************************/
static void eventName(void *event, char *dest) {
  Event V = (Event) event;
  int shift;
  if (V) {
    if (isNaN(V->maxTime)) sprintf(dest, "%s %n", NO_VALUE, &shift);
    else sprintf(dest, "%g %n", V->maxTime, &shift);
    dest += shift;
    if (isNaN(V->minTime)) sprintf(dest, "%s %n", NO_VALUE, &shift);
    else sprintf(dest, "%g %n", V->minTime, &shift);
    dest += shift;
    if (isNaN(V->graceTime)) sprintf(dest, "%s %n", NO_VALUE, &shift);
    else sprintf(dest, "%g %n", V->graceTime, &shift);
    dest += shift;
  } else dest[0] = '\0';
}

static void initEventInfo(void) {
  Event V;
  addMember(EventInfo, "input", OBJP, OFFSET(V, input), TRUE, 
	    0, 0, RangeInfo);
  addMember(EventInfo, "sharedInputs", OBJ, OFFSET(V, sharedInputs), FALSE,
	    0, 0, FlagInfo);
  addMember(EventInfo, "target", OBJP, OFFSET(V, target), TRUE, 
	    0, 0, RangeInfo);
  addMember(EventInfo, "sharedTargets", OBJ, OFFSET(V, sharedTargets), FALSE,
	    0, 0, FlagInfo);
  addSpacer(EventInfo);

  addMember(EventInfo, "maxTime", OBJ, OFFSET(V, maxTime), TRUE, 
	    0, 0, RealInfo);
  addMember(EventInfo, "minTime", OBJ, OFFSET(V, minTime), TRUE, 
	    0, 0, RealInfo);
  addMember(EventInfo, "graceTime", OBJ, OFFSET(V, graceTime), TRUE,
	    0, 0, RealInfo);
  addMember(EventInfo, "defaultInput", OBJ, OFFSET(V, defaultInput), TRUE,
	    0, 0, RealInfo);
  addMember(EventInfo, "activeInput", OBJ, OFFSET(V, activeInput), TRUE,
	    0, 0, RealInfo);
  addMember(EventInfo, "defaultTarget", OBJ, OFFSET(V, defaultTarget), TRUE,
	    0, 0, RealInfo);
  addMember(EventInfo, "activeTarget", OBJ, OFFSET(V, activeTarget), TRUE,
	    0, 0, RealInfo);
  addSpacer(EventInfo);


  addMember(EventInfo, "proc", OBJ, OFFSET(V, proc), TRUE, 
	    0, 0, TclObjInfo);
  addMember(EventInfo, "example", OBJP, OFFSET(V, example), FALSE,
	    0, 0, ExampleInfo);
  addMember(EventInfo, "ext", OBJP, OFFSET(V, ext), FALSE, 
	    0, 0, EventExtInfo);
}

/*****************************************************************************/
static void rangeName(void *r, char *dest) {
  Range R = (Range) r;
  sprintf(dest, "%s %d %d %g", (R->groupName) ? R->groupName : "", 
	  R->numUnits, R->firstUnit, R->value);
}

int getRangeNumUnits(void *R) {
  return ((Range) R)->numUnits;
}

static void initRangeInfo(void) {
  Range R;
  /* These cannot be changed because they are shared strings */
  addMember(RangeInfo, "groupName", OBJ, OFFSET(R, groupName), FALSE,
	    0, 0, StringInfo);
  addMember(RangeInfo, "numUnits", OBJ, OFFSET(R, numUnits), FALSE,
	    0, 0, IntInfo);
  addMember(RangeInfo, "firstUnit", OBJ, OFFSET(R, firstUnit), TRUE,
	    0, 0, IntInfo);
  addMember(RangeInfo, "val", OBJA, OFFSET(R, val), TRUE,
	    getRangeNumUnits, 0, RealInfo);
  addMember(RangeInfo, "value", OBJ, OFFSET(R, value), TRUE,
	    0, 0, RealInfo);
  addMember(RangeInfo, "unit", OBJA, OFFSET(R, unit), TRUE,
	    getRangeNumUnits, 0, IntInfo);
  addSpacer(RangeInfo);

  addMember(RangeInfo, "next", OBJP, OFFSET(R, next), FALSE,
	    0, 0, RangeInfo);
}

/*****************************************************************************/
static void graphName(void *r, char *dest) {
  Graph G = (Graph) r;
  if (G) sprintf(dest, "Graph %d", G->num);
  else dest[0] = '\0';
}

int getGraphNumTraces(void *G) {
  return ((Graph) G)->numTraces;
}

static void initGraphInfo(void) {
  Graph G;
  addMember(GraphInfo, "num", OBJ, OFFSET(G, num), FALSE,
            0, 0, IntInfo);
  addMember(GraphInfo, "updateOn", OBJ, OFFSET(G, updateOn), TRUE,
            0, 0, MaskInfo);
  addMember(GraphInfo, "updateEvery", OBJ, OFFSET(G, updateEvery), TRUE,
            0, 0, IntInfo);
  addMember(GraphInfo, "storeOnReset", OBJ, OFFSET(G, storeOnReset), TRUE,
            0, 0, FlagInfo);
  addMember(GraphInfo, "clearOnReset", OBJ, OFFSET(G, clearOnReset), TRUE,
            0, 0, FlagInfo);
  addMember(GraphInfo, "hidden", OBJ, OFFSET(G, hidden), FALSE,
            0, 0, FlagInfo);
  addSpacer(GraphInfo);

  addMember(GraphInfo, "x", OBJ, OFFSET(G, x), FALSE,
            0, 0, IntInfo);
  addMember(GraphInfo, "maxX", OBJ, OFFSET(G, maxX), FALSE,
            0, 0, IntInfo);
  addMember(GraphInfo, "width", OBJ, OFFSET(G, width), FALSE,
            0, 0, IntInfo);
  addMember(GraphInfo, "height", OBJ, OFFSET(G, height), FALSE,
            0, 0, IntInfo);
  addMember(GraphInfo, "cols", OBJ, OFFSET(G, cols), TRUE,
            0, 0, IntInfo);
  addMember(GraphInfo, "scaleX", OBJ, OFFSET(G, scaleX), FALSE,
            0, 0, RealInfo);
  addSpacer(GraphInfo);

  addMember(GraphInfo, "min", OBJ, OFFSET(G, min), TRUE,
            0, 0, RealInfo);
  addMember(GraphInfo, "minVal", OBJ, OFFSET(G, minVal), TRUE,
            0, 0, RealInfo);
  addMember(GraphInfo, "fixMin", OBJ, OFFSET(G, fixMin), TRUE,
            0, 0, FlagInfo);
  addMember(GraphInfo, "max", OBJ, OFFSET(G, max), TRUE,
            0, 0, RealInfo);
  addMember(GraphInfo, "maxVal", OBJ, OFFSET(G, maxVal), TRUE,
            0, 0, RealInfo);
  addMember(GraphInfo, "fixMax", OBJ, OFFSET(G, fixMax), TRUE,
            0, 0, FlagInfo);
  addMember(GraphInfo, "scaleY", OBJ, OFFSET(G, scaleY), FALSE,
            0, 0, RealInfo);
  addSpacer(GraphInfo);

  addMember(GraphInfo, "numTraces", OBJ, OFFSET(G, numTraces), FALSE,
            0, 0, IntInfo);
  addMember(GraphInfo, "trace", OBJPA, OFFSET(G, trace), FALSE,
            getGraphNumTraces, 0, TraceInfo);
}

/*****************************************************************************/
static void traceName(void *r, char *dest) {
  Trace T = (Trace) r;
  if (!T)
    dest[0] = '\0';
  else if (T->label) 
    sprintf(dest, "%s", T->label);
  else
    sprintf(dest, "%s", T->object);
}

int getTraceNumVals(void *T) {
  return ((Trace) T)->numVals;
}

static void initTraceInfo(void) {
  Trace T;
  addMember(TraceInfo, "num", OBJ, OFFSET(T, num), FALSE,
            0, 0, IntInfo);
  addMember(TraceInfo, "graph", OBJP, OFFSET(T, graph), FALSE,
            0, 0, GraphInfo);
  addMember(TraceInfo, "label", OBJ, OFFSET(T, label), TRUE,
            0, 0, StringInfo);
  addMember(TraceInfo, "object", OBJ, OFFSET(T, object), TRUE,
            0, 0, StringInfo);
  addMember(TraceInfo, "value", OBJP, OFFSET(T, value), FALSE,
            0, 0, RealInfo);
  addSpacer(TraceInfo);

  addMember(TraceInfo, "numVals", OBJ, OFFSET(T, numVals), FALSE,
            0, 0, IntInfo); 
  addMember(TraceInfo, "maxVals", OBJ, OFFSET(T, maxVals), FALSE,
            0, 0, IntInfo); 
  addMember(TraceInfo, "val", OBJA, OFFSET(T, val), FALSE,
            getTraceNumVals, 0, PointInfo);
  addSpacer(TraceInfo);

  addMember(TraceInfo, "color", OBJ, OFFSET(T, color), TRUE,
            0, 0, StringInfo); 
  addMember(TraceInfo, "active", OBJ, OFFSET(T, active), TRUE,
            0, 0, FlagInfo); 
  addMember(TraceInfo, "transient", OBJ, OFFSET(T, transient), TRUE,
            0, 0, FlagInfo); 
  addMember(TraceInfo, "visible", OBJ, OFFSET(T, visible), TRUE,
            0, 0, FlagInfo); 
}

/*****************************************************************************/
static void pointName(void *r, char *dest) {
  Point P = (Point) r;
  sprintf(dest, "(%d, %g)", P->x, P->y);
}

static void initPointInfo(void) {
  Point P;
  addMember(PointInfo, "x", OBJ, OFFSET(P, x), TRUE,
            0, 0, IntInfo);
  addMember(PointInfo, "y", OBJ, OFFSET(P, y), TRUE,
            0, 0, RealInfo);
}

/*****************************************************************************/
static void intName(void *i, char *dest) {
  if (i) sprintf(dest, "%d", *(int *) i);
  else dest[0] = '\0';
}

static void setInt(void *i, char *val) {
  *(int *) i = *(int *) val;
}

static void setStringInt(void *i, char *val) {
  *(int *) i = atoi(val);
}

static void realName(void *r, char *dest) {
  real val;
  if (r) {
    val = *(real *) r;
    if (isNaN(val)) sprintf(dest, NO_VALUE);
    else sprintf(dest, "%.7g", val);
  }
  else dest[0] = '\0';
}

static void setReal(void *r, char *val) {
  *(real *) r = *(real *) val;
}

static void setStringReal(void *r, char *val) {
  *(real *) r = ator(val);
}

static void doubleName(void *r, char *dest) {
  double val;
  if (r) {
    val = *(double *) r;
    if (isNaN(val)) sprintf(dest, NO_VALUE);
    else sprintf(dest, "%.7g", val);
  }
  else dest[0] = '\0';
}

static void setDouble(void *r, char *val) {
  *(double *) r = *(double *) val;
}

static void setStringDouble(void *r, char *val) {
  *(double *) r = atof(val);
}

static void setFlag(void *r, char *val) {
  *(flag *) r = *(flag *) val;
}

static void setStringFlag(void *f, char *val) {
  if (atoi(val))
    *(flag *) f = TRUE;
  else *(flag *) f = FALSE;
}


static void maskName(void *m, char *dest) {
  if (m) sprintf(dest, "%#x", *(mask *) m);
  else dest[0] = '\0';
}

static void stringName(void *s, char *dest) {
  if (s && (*(char **) s)) sprintf(dest, "%s", *(char **) s);
  else dest[0] = '\0';
}

static void setString(void *s, char *val) {
  if (*(char **) s) free(*(char **) s);
  *(char **) s = copyString(val);
}

static void tclObjName(void *s, char *dest) {
  int len;
  if (s && (*(char **) s))
    strcpy(dest, Tcl_GetStringFromObj(*(Tcl_Obj **) s, &len));
  else dest[0] = '\0';
}

static void setTclObj(void *s, char *val) {
  Tcl_Obj **objp = (Tcl_Obj **) s;
  if (!val[0]) {
    if (*objp) {
      Tcl_DecrRefCount(*objp);
      *objp = NULL;
    }
  } else if (!(*objp)) {
    *objp = Tcl_NewStringObj(val, -1);
    Tcl_IncrRefCount(*objp);
  } else {
    Tcl_SetStringObj(*objp, val, -1);
  }
}

/*****************************************************************************/

ObjInfo newObject(char *name, int size, int maxDepth,
		  void (*nameProc)(void *, char *), 
		  void (*setProc)(void *, char *), 
		  void (*setStringProc)(void *, char *)) {
  ObjInfo O = (ObjInfo) safeCalloc(1, sizeof *O, "newObject:O");
  O->name = copyString(name);
  O->size = size;
  O->maxDepth = maxDepth;
  O->tail = &(O->members);
  if (!nameProc) fatalError("object %s has no getName procedure", nameProc);
  O->getName = nameProc;
  O->setValue = setProc;
  O->setStringValue = setStringProc;
  return O;
}

void addMember(ObjInfo O, char *name, int type, int offset, 
	       flag writable, int (*rows)(void *), int (*cols)(void *), 
	       ObjInfo info) {
  MemInfo M = (MemInfo) safeMalloc(sizeof *M, "addMember:M");
  M->name = copyString(name);
  M->type = type;
  M->offset = offset;
  M->writable = writable;
  M->rows = rows;
  M->cols = cols;
  M->info = info;
  M->next = NULL;
  *(O->tail) = M;
  O->tail = &(M->next);
}

void addObj(ObjInfo O, char *name, int offset, flag writable, ObjInfo info) {
  addMember(O, name, OBJ, offset, writable, NULL, NULL, info);
}

void addObjP(ObjInfo O, char *name, int offset, flag writable, ObjInfo info) {
  addMember(O, name, OBJP, offset, writable, NULL, NULL, info);
}

void addObjA(ObjInfo O, char *name, int offset, flag writable, 
	     int (*rows)(void *), ObjInfo info) {
  addMember(O, name, OBJA, offset, writable, rows, NULL, info);
}

void addObjPA(ObjInfo O, char *name, int offset, flag writable, 
	     int (*rows)(void *), ObjInfo info) {
  addMember(O, name, OBJPA, offset, writable, rows, NULL, info);
}

void addObjAA(ObjInfo O, char *name, int offset, flag writable, 
	      int (*rows)(void *), int (*cols)(void *), ObjInfo info) {
  addMember(O, name, OBJAA, offset, writable, rows, cols, info);
}

void addObjPAA(ObjInfo O, char *name, int offset, flag writable, 
	       int (*rows)(void *), int (*cols)(void *), ObjInfo info) {
  addMember(O, name, OBJPAA, offset, writable, rows, cols, info);
}

void addSpacer(ObjInfo O) {
  addMember(O, NULL, SPACER, 0, 0, NULL, NULL, NULL);
}

void createObjects(void) {
  RootInfo = newObject("Root", sizeof(struct rootrec), 0, rootName, 
		       NULL, NULL);
  NetInfo = newObject("Network", sizeof(struct network), 0, netName, 
		      NULL, NULL);
  GroupInfo = newObject("Group", sizeof(struct group), 2, groupName, 
			NULL, NULL);
  GroupProcInfo = newObject("GroupProc", sizeof(struct groupProc), 4, 
			    groupProcName, NULL, NULL);
  UnitInfo = newObject("Unit", sizeof(struct unit), 4, unitName, 
		       NULL, NULL);
  BlockInfo = newObject("Block", sizeof(struct block), 6, blockName, 
			NULL, NULL);
  LinkInfo = newObject("Link", sizeof(struct link), 6, linkName, 
		       NULL, NULL);
  Link2Info = newObject("Link2", sizeof(struct link2), 6, link2Name, 
			NULL, NULL);
  ExampleSetInfo = newObject("Example Set", sizeof(struct exampleSet), 2,
			     exampleSetName, NULL, NULL);
  ExampleInfo = newObject("Example", sizeof(struct example), 4, exampleName,
			  NULL, NULL);
  EventInfo = newObject("Event", sizeof(struct event), 6, eventName, 
			NULL, NULL);
  RangeInfo = newObject("Range", sizeof(struct range), 8, rangeName, 
			NULL, NULL);
  GraphInfo = newObject("Graph", sizeof(struct graph), 2, graphName, 
			NULL, NULL);
  TraceInfo = newObject("Trace", sizeof(struct trace), 4, traceName, 
			NULL, NULL);
  PointInfo = newObject("Point", sizeof(struct point), 6, pointName, 
			NULL, NULL);

  IntInfo = newObject("Int", sizeof(int), -1, intName, 
		      setInt, setStringInt);
  RealInfo = newObject("Real", sizeof(real), -1, realName, 
		       setReal, setStringReal);
  DoubleInfo = newObject("Double", sizeof(double), -1, doubleName, 
		       setDouble, setStringDouble);
  FlagInfo = newObject("Flag", sizeof(flag), -1, intName, 
		       setFlag, setStringFlag);
  MaskInfo = newObject("Mask", sizeof(mask), -1, maskName, 
		       setInt, setStringInt);
  StringInfo = newObject("String", sizeof(char *), -1, stringName, 
			 NULL, setString);
  TclObjInfo = newObject("TclObj", sizeof(Tcl_Obj *), -1, tclObjName, 
			 NULL, setTclObj);
}

void initObjects(void) {
  initRootInfo();
  initNetInfo();
  initGroupInfo();
  initGroupProcInfo();
  initUnitInfo();
  initBlockInfo();
  initLinkInfo();
  initLink2Info();
  initExampleSetInfo();
  initExampleInfo();
  initEventInfo();
  initRangeInfo();
  initGraphInfo();
  initTraceInfo();
  initPointInfo();
}

/*****************************************************************************/


static char *parseMemberName(char *path, char *memberName, int *index) {
  int shift = 0;
  memberName[0] = '\0';
  while (*path == '.') path++;
  if (*path == '(' || *path == '[' || *path == ',') {
    if (sscanf(++path, " %d %n", index, &shift) != 1) {
      *index = -1;
      return NULL;
    }
    path += shift;
    if (*path == ')' || *path == ']') path++;
  } else {
    if (sscanf(path, "%[^[(.]%n", memberName, &shift) != 1)
      return NULL;
    path += shift;
  }
  if (*path == '.') path++;
  return path;
}


MemInfo lookupMember(char *name, ObjInfo objectInfo) {
  MemInfo M;
  for (M = objectInfo->members; M; M = M->next)
    if (M->type != SPACER && !strcmp(name, M->name))
      return M;
  return NULL;
}

/* Return a pointer to the object */
void *Obj(char *object, int offset) {
  return (void *) (object + offset);
}

/* Return a pointer to the object */
void *ObjP(char *object, int offset) {
  if (!*(void **)(object + offset))
    return NULL;
  return *((void **) (object + offset));
}

/* Return a pointer to the object */
void *ObjPP(char *object, int offset) {
  if (!*(void ***)(object + offset) || !(**(void ***)(object + offset)))
    return NULL;
  return **((void ***) (object + offset));
}

/* Returns a pointer to the indexed element */
void *ObjA(char *array, int size, int index) {
  if (!array) return NULL;
  return (void *) (array + size * index);
}

/* Returns a pointer to the indexed element */
void *ObjPA(char *array, int index) {
  if (!array) return NULL;
  return ((void **) array)[index];
}

/* Returns a pointer to the indexed element */
void *ObjAA(char *array, int size, int row, int col) {
  if (!array || !((char **) array)[row]) return NULL;
  return (void *) ((((char **) array)[row]) + size * col);
}

/* Returns a pointer to the indexed element */
void *ObjPAA(char *array, int row, int col) {
  if (!array || !((char ***) array)[row]) return NULL;
  return (void *) ((char ***) array)[row][col];
}


/* path is the rest of the lookup path ("unit(4).output")
   object is the current structure pointer (Net->group[3])
   this could point to part of an array if the array is 2D.
   objectInfo is the info about that structure type (GroupInfo)
   if the object is an array, this is the info about the objects in the array
   type is the actual typeof the object.  This may be OBJA if the object is 
   actually part of an OBJAA.
   It returns a pointer to the object and what its actual type is.
   If the object is an array, its dimension(s) are returned
*/
static void *lookupObject(char *path, char *object, ObjInfo O,
			  int type, int rows, int cols, flag writable, 
			  ObjInfo *retObjInfo, int *retType, int *retRows, 
			  int *retCols, flag *retWrit) {
  MemInfo M;
  char memberName[256], *newPath;
  int index = -1;

  if (!object) {
    warning("lookupObject: object does not exist");
    return NULL;
  }
  if (!path[0]) {
    *retObjInfo = O;
    *retType = type;
    *retRows = rows;
    *retCols = cols;
    *retWrit = writable;
    return object;
  }

  newPath = parseMemberName(path, memberName, &index);
  if (!newPath && index == -1) {
    warning("lookupObject: bad path field: \"%s\"", path);
    return NULL;
  }

  if (type == OBJ)
    fatalError("type was OBJ in lookupObject");
  if (type == OBJP) {
    if (!(M = lookupMember(memberName, O))) {
      warning("lookupObject: field \"%s\" not found in object of type %s", 
	      memberName, O->name);
      return NULL;
    }

    if (M->type == OBJ)
      return lookupObject(newPath, Obj(object, M->offset), M->info, 
			  OBJP, -1, -1, M->writable, retObjInfo, retType, 
			  retRows, retCols, retWrit);
    else if (M->type == OBJPP)
      return lookupObject(newPath, ObjPP(object, M->offset), M->info, 
			  OBJP, -1, -1, M->writable, retObjInfo, retType, 
			  retRows, retCols, retWrit);
    else {
      rows = (M->rows) ? M->rows(object) : -1;
      cols = (M->cols) ? M->cols(object) : -1;
      return lookupObject(newPath, ObjP(object, M->offset), M->info, 
			  M->type, rows, cols, M->writable, retObjInfo, 
			  retType, retRows, retCols, retWrit);
    }
  } else { /* it is an array */
    if (memberName[0]) {
      warning("lookupObject: field \"%s\" cannot be a child of an array",
	      memberName);
      return NULL;
    }

    if (index < 0 || index >= rows) {
      warning("lookupObject: index %d out of bounds", index);
      return NULL;
    }
    switch (type) {
    case OBJA:
      return lookupObject(newPath, ObjA(object, O->size, index), O, 
			  OBJP, -1, -1, writable, retObjInfo, retType, 
			  retRows, retCols, retWrit);
    case OBJPA:
      return lookupObject(newPath, ObjPA(object, index), O, 
			  OBJP, -1, -1, writable, retObjInfo, retType, 
			  retRows, retCols, retWrit);
    case OBJAA:
      return lookupObject(newPath, ObjPA(object, index), O, OBJA, 
			  cols, -1, writable, retObjInfo, retType, 
			  retRows, retCols, retWrit);
    case OBJPAA:
      return lookupObject(newPath, ObjPA(object, index), O, OBJPA, 
			  cols, -1, writable, retObjInfo, retType, 
			  retRows, retCols, retWrit);
    default:
      fatalError("lookupObject: undefined field type %d", type);
    }
  }
  return NULL;
}

void *getObject(char *path, ObjInfo *retObjInfo, int *retType, int *retRows, 
		int *retCols, flag *retWrit) {
  void *result = NULL;
  char memberName[256], *newPath;
  int index;
  Network N;
  Group G;
  Unit U;
  ExampleSet S;
  if (Net)  /* Look it up in the network. */
    result = lookupObject(path, (char *) Net, NetInfo, OBJP, -1, -1, FALSE, 
			  retObjInfo, retType, retRows, retCols, retWrit);
  /* If not found, look it up starting at the root. */
  if (!result)
    result = lookupObject(path, (char *) Root, RootInfo, OBJP, -1, -1, FALSE,
                          retObjInfo, retType, retRows, retCols, retWrit);
  if (!result) {
    newPath = parseMemberName(path, memberName, &index);
    if (!newPath && index == -1) {
      warning("lookupObject: bad path field: \"%s\"", path);
      return NULL;
    }
    /* First see if it starts with a network name. */
    if ((N = lookupNet(memberName)))
      return lookupObject(newPath, (char *) N, NetInfo, OBJP, -1, -1, FALSE, 
			  retObjInfo, retType, retRows, retCols, retWrit);
    /* Then try a group name. */
    if (Net && (G = lookupGroup(memberName)))
      return lookupObject(newPath, (char *) G, GroupInfo, OBJP, -1, -1, FALSE, 
			  retObjInfo, retType, retRows, retCols, retWrit);
    /* Then try a unit name. */
    if (Net && (U = lookupUnit(memberName)))
      return lookupObject(newPath, (char *) U, UnitInfo, OBJP, -1, -1, FALSE, 
			  retObjInfo, retType, retRows, retCols, retWrit);
    /* Then try an example set. */
    if ((S = lookupExampleSet(memberName)))
      return lookupObject(newPath, (char *) S, ExampleSetInfo, OBJP, -1, -1, 
			  FALSE, retObjInfo, retType, retRows, retCols, 
			  retWrit);
  }
  return result;
}

flag isObject(char *path) {
  int type, rows, cols;
  flag writable;
  ObjInfo O;

  return (getObject(path, &O, &type, &rows, &cols, &writable)) ? TRUE : FALSE;
}

void printObject(char *object, ObjInfo O, int type, int rows, 
		 int cols, int depth, int initDepth, int maxDepth) {
  MemInfo M;
  int i, t;

  if (!object || depth > maxDepth + 1) return;  

  if (type == OBJ || type == OBJPP)
    fatalError("type was OBJ in printObject");
  if (type == OBJP) {
    if (depth >= maxDepth || depth > O->maxDepth) {
      O->getName(object, Buffer);
      Tcl_AppendResult(Interp, Buffer, NULL);
    } else {
      append("--%s--", O->name);
      for (M = O->members; M; M = M->next) {
	if (M->type == SPACER) continue;
	append("\n");
	for (t = 0; t < depth - initDepth; t++) append("    ");
	append("%-20s: ", M->name);
	if (M->type == OBJ)
	  printObject(Obj(object, M->offset), M->info, OBJP, -1, -1, depth + 1,
		      initDepth, maxDepth);
	else if (M->type == OBJPP)
	  printObject(ObjPP(object, M->offset), M->info, OBJP, -1, -1, 
		      depth + 1, initDepth, maxDepth);
	else {
	  rows = (M->rows) ? M->rows(object) : -1;
	  cols = (M->cols) ? M->cols(object) : -1;
	  printObject(ObjP(object, M->offset), M->info, M->type, rows, cols,
		      depth + 1, initDepth, maxDepth);
	}
      }
    }
  } else { /* It is an array */
    if (type == OBJA || type == OBJPA) { /* 1D array */
      append("--%s(%d)--", O->name, rows);
      if (depth < maxDepth)
	for (i = 0; i < rows; i++) {
	  append("\n");
	  for (t = 0; t < depth - initDepth; t++) append("    ");
	  append("(%d): ", i);
	  if (type == OBJA)
	    printObject(ObjA(object, O->size, i), O, OBJP, -1, -1,
			depth + 1, initDepth, maxDepth);
	  else
	    printObject(ObjPA(object, i), O, OBJP, -1, -1,
			depth + 1, initDepth, maxDepth);
	}
    } else { /* 2D array */
      append("--%s(%d,%d)--", O->name, rows, cols);
      if (depth < maxDepth - 1)
	for (i = 0; i < rows; i++) {
	  append("\n");
	  for (t = 0; t < depth - initDepth; t++) append("    ");
	  append("(%d): ", i);
	  if (type == OBJAA)
	    printObject(ObjPA(object, i), O, OBJA, cols, -1,
			depth + 1, initDepth, maxDepth);
	  else
	    printObject(ObjPA(object, i), O, OBJPA, cols, -1,
			depth + 1, initDepth, maxDepth);
	}
    }
  }
}

flag writeParameters(Tcl_Channel channel, char *path) {
  ObjInfo O;
  MemInfo M;
  char *object;
  int type, rows, cols;
  flag writable;
  char value[512];

  if (!(object = lookupObject(path, (char *) Net, NetInfo, OBJP, -1, -1, FALSE,
			      &O, &type, &rows, &cols, &writable)))
    return warning("writeParameters: unknown object: %s", path);
  if (type != OBJP)
    return warning("writeParameters: can only write fields of "
		   "container objects");
  for (M = O->members; M; M = M->next) {
    if (M->type != OBJ || !M->writable) continue;
    M->info->getName(object + M->offset, value);
    if (path[0])
      cprintf(channel, "setObj %s.%s {%s}\n", path, M->name, value);
    else cprintf(channel, "setObj %s {%s}\n", M->name, value);
  }
  return TCL_OK;
}
