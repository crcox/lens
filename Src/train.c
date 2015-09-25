#include "system.h"
#include "util.h"
#include "type.h"
#include "network.h"
#include "act.h"
#include "train.h"
#include "control.h"
#include "display.h"
#include "graph.h"

Algorithm AlgorithmTable = NULL;

void registerAlgorithm(mask code, char *shortName, char *longName,
		       void (*updateWeights)(flag)) {
  Algorithm A = (Algorithm) safeMalloc(sizeof(struct algorithm),
				       "registerAlgorithm:A");
  registerType(shortName, code, ALGORITHM);
  A->code = code;
  A->shortName = copyString(shortName);
  A->longName = copyString(longName);
  A->updateWeights = updateWeights;
  A->next = AlgorithmTable;
  AlgorithmTable = A;
  if (eval(".registerAlgorithm %s \"%s\" %d", shortName, longName, code))
    fatalError(Tcl_GetStringResult(Interp));
}

void registerAlgorithms(void) {
  registerAlgorithm(STEEPEST, "steepest", "Steepest Descent",
		    steepestUpdateWeights);
  registerAlgorithm(MOMENTUM, "momentum", "Momentum Descent",
		    momentumUpdateWeights);
  registerAlgorithm(DOUGS_MOMENTUM, "dougsMomentum", "Doug's Momentum",
		    dougsMomentumUpdateWeights);
#ifdef ADVANCED
  registerAlgorithm(DELTA_BAR_DELTA, "deltaBarDelta", "Delta-Bar-Delta",
		    deltabardeltaUpdateWeights);
  /*
  registerAlgorithm(QUICK_PROP, "quickProp", "Quick-Prop",
		    quickpropUpdateWeights);
  */
#endif /* ADVANCED */
}

Algorithm getAlgorithm(mask code) {
  Algorithm A;
  for (A = AlgorithmTable; A && A->code != code; A = A->next);
  if (!A) fatalError("unknown algorithm code: %d", code);
  return A;
}

void printReportHeader(void) {
  print(1, "__Update____Error___UnitCost__Wgt.Cost__Grad.Lin__TimeUsed__"
	"TimeLeft__\n");
}

flag printReport(int lastReport, int update, unsigned long startTime) {
  unsigned long now, remaining;
  char buf[128];
  static unsigned int reportNum;
  static unsigned long last;
  static real perUpdate = 0.0;

  if (!lastReport) reportNum = 1;
  else reportNum++;

  if (Verbosity < 1) return TCL_OK;
  print(1, "%7d)  ", Net->totalUpdates);
  smartPrintReal(Net->error, 8, FALSE);
  print(1, "  ");
  smartPrintReal(Net->outputCost, 8, FALSE);
  print(1, "  ");
  smartPrintReal(Net->weightCost, 8, FALSE);
  print(1, "  ");
  smartPrintReal(Net->gradientLinearity, 8, FALSE);
  print(1, "  ");

  now = getTime();

  printTime((now - startTime) * 1e-3, buf);
  print(1, " %s   ", buf);
  if (Net->numUpdates > update && update != 0) {
    if (reportNum < 5)
      perUpdate = (real) (now - startTime) / update;
    else
      perUpdate = 0.5 * perUpdate + 0.5 *
	((real) (now - last) / (update - lastReport));
    last = now;
    remaining = (unsigned long) (perUpdate * 1e-3 *
				 (Net->numUpdates - update));
    printTime(remaining, buf);
  } else printTime(0, buf);
  print(1, "%s\n", buf);

  updateDisplays(ON_REPORT);
  return TCL_OK;
}

/* Assumes there is a net */
flag standardNetTrain(void) {
  int i, lastReport, batchesAtCriterion;
  flag willReport, groupCritReached, value = TCL_OK, done;
  unsigned long startTime;
  Algorithm A;

  if (Net->numUpdates < 0)
    return warning("numUpdates (%d) must be positive.", Net->numUpdates);
  if (Net->numUpdates == 0) return result("");
  if (!Net->trainingSet)
    return warning("There is no training set.");
  if (Net->learningRate < 0.0)
    return warning("learningRate (%f) cannot be negative.", Net->learningRate);
  if (Net->momentum < 0.0 && Net->momentum >= 1.0)
    return warning("momentum (%f) is out of range [0,1).", Net->momentum);
  if (Net->weightDecay < 0.0 || Net->weightDecay > 1.0)
    return warning("weightDecay (%f) must be in the range [0,1].",
		   Net->weightDecay);
  if (Net->reportInterval < 0)
    return warning("reportInterval (%d) cannot be negative.",
		   Net->reportInterval);

  A = getAlgorithm(Net->algorithm);

  print(1, "Performing %d updates using %s...\n",
	Net->numUpdates, A->longName);
  if (Net->reportInterval) printReportHeader();

  startTime = getTime();
  lastReport = batchesAtCriterion = 0;
  groupCritReached = FALSE;
  done = FALSE;
  /* It always does at least one update. */
  for (i = 1; !done; i++) {
    RUN_PROC(preEpochProc);

    if ((value = Net->netTrainBatch(&groupCritReached))) break;

    if (Net->error < Net->criterion || groupCritReached)
      batchesAtCriterion++;
    else batchesAtCriterion = 0;
    if ((Net->minCritBatches > 0 && batchesAtCriterion >= Net->minCritBatches)
	|| i >= Net->numUpdates) done = TRUE;

    willReport = (Net->reportInterval &&
		  ((i % Net->reportInterval == 0) || done))
      ? TRUE : FALSE;

    RUN_PROC(postEpochProc);

    /* Here's the weight update (one epoch). */
    A->updateWeights(willReport);

    RUN_PROC(postUpdateProc);

    updateDisplays(ON_UPDATE);

    Net->totalUpdates++;

    if (willReport) {
      printReport(lastReport, i, startTime);
      lastReport = i;
    }
    /* Stop if requested. */
    if (smartUpdate(FALSE)) break;
    /* Change the algorithm if requested. */
    if (A->code != Net->algorithm) {
      A = getAlgorithm(Net->algorithm);
      print(1, "Changing algorithm to %s...\n", A->longName);
    }
  }
  startTime = (getTime() - startTime);

  updateDisplays(ON_TRAINING);

  if (value == TCL_ERROR) return TCL_ERROR;
  result("Performed %d updates\n", i - 1);
  if (!done) {
    append("Training halted prematurely\n", i);
    value = TCL_ERROR;
  }
  if (Net->error <= Net->criterion &&
      batchesAtCriterion >= Net->minCritBatches)
    append("Network reached overall error criterion of %f\n",
	   Net->criterion);
  if (groupCritReached && batchesAtCriterion >= Net->minCritBatches)
    append("Network reached group output criterion\n");
  append("Total time elapsed: %.3f seconds", ((real) startTime * 1e-3));

  return value;
}

void updateAdaptiveGain(Group G) {
  real learningRate = Net->adaptiveGainRate;
  real gainDecay = chooseValue(G->gainDecay, Net->gainDecay);
  FOR_EACH_UNIT(G, {
    U->gain -= (learningRate * U->gainDeriv + gainDecay * U->gain);
    if (U->gain < SMALL_VAL) U->gain = SMALL_VAL;
  });
}


/* This splits the processing of entire block into doStats and no doStats
   cases so the no stats inner loop doesn't need to do extra tests */
void steepestUpdateWeights(flag doStats) {
  UPDATE_WEIGHTS({
    w = L->weight;
    lastWeightDelta = -learningRate * L->deriv;
    if (weightDecay > 0.0) lastWeightDelta -= weightDecay * w;
    w += lastWeightDelta;
    if (!isNaN(B->min) && w < B->min) w = B->min;
    else if (!isNaN(B->max) && w > B->max) w = B->max;
    M->lastWeightDelta = w - L->weight;
    L->weight = w;
  });
}

/* This is like steepest except that it uses the momentum. */
void momentumUpdateWeights(flag doStats) {
  UPDATE_WEIGHTS({
    w = L->weight;
    lastWeightDelta = -learningRate * L->deriv +
      momentum * M->lastWeightDelta;
    if (weightDecay > 0.0) lastWeightDelta -= weightDecay * w;
    w += lastWeightDelta;
    if (!isNaN(B->min) && w < B->min) w = B->min;
    else if (!isNaN(B->max) && w > B->max) w = B->max;
    M->lastWeightDelta = w - L->weight;
    L->weight = w;
  });
}

/* This is exactly like momentum but the length of the weight delta vector
   (before momentum) is always exactly the learning rate. */
void dougsMomentumUpdateWeights(flag doStats) {
  double scale, sum = 0.0;
  FOR_EACH_GROUP({if (G->type & FROZEN) continue;
    FOR_EACH_UNIT(G, {
      Link L; Link sL;
      if (U->type & FROZEN) continue;
      L = U->incoming;
      FOR_EACH_BLOCK(U, {
	    if (B->type & FROZEN) {L += B->numUnits; continue;}
	    for (sL = L + B->numUnits; L < sL; L++)
	      sum += SQUARE(L->deriv);
      });
    });
  });
  scale = (sum > 1.0) ? 1.0 / SQRT(sum) : 1.0;
  UPDATE_WEIGHTS({
    w = L->weight;
    lastWeightDelta = -learningRate * scale * L->deriv +
      momentum * M->lastWeightDelta;
    if (weightDecay > 0.0) lastWeightDelta -= weightDecay * w;
    w += lastWeightDelta;
    if (!isNaN(B->min) && w < B->min) w = B->min;
    else if (!isNaN(B->max) && w > B->max) w = B->max;
    M->lastWeightDelta = w - L->weight;
    L->weight = w;
  });
}


#ifdef ADVANCED
/* The link learning rate is stored in the lastValue field. */
/* The link momentum is stored in the lastWeightDelta field. */

#define OPPOSITE_SIGN(x,y) (IS_NEGATIVE(x) ^ IS_NEGATIVE(y))

void deltabardeltaUpdateWeights(flag doStats) {
  real rateIncrement = Net->rateIncrement, rateDecrement = Net->rateDecrement,
    linkLearningRate;
  UPDATE_WEIGHTS({
    lastWeightDelta = M->lastWeightDelta;
    deriv = L->deriv;
    linkLearningRate = M->lastValue;

    if (OPPOSITE_SIGN(deriv, lastWeightDelta))
      linkLearningRate += rateIncrement;
    else linkLearningRate *= rateDecrement;
    lastWeightDelta = -linkLearningRate * learningRate * deriv +
      momentum * lastWeightDelta;
    w = L->weight;
    if (weightDecay > 0.0) lastWeightDelta -= weightDecay * w;
    w += lastWeightDelta;
    if (!isNaN(B->min) && w < B->min) w = B->min;
    else if (!isNaN(B->max) && w > B->max) w = B->max;
    M->lastWeightDelta = w - L->weight;
    L->weight = w;
    M->lastValue = linkLearningRate;
  });
}

#ifdef JUNK
/* If lastWeightDelta is 0, I use steepest.  Is this best? */
/* lastValue stores the lastDeriv */
void quickpropUpdateWeights(flag doStats) {
  UPDATE_WEIGHTS({
    deriv = L->deriv;
    w = L->weight;
    lastWeightDelta = M->lastWeightDelta;
    if (lastWeightDelta == 0.0)
      lastWeightDelta = -learningRate * 0.1 * deriv;
    else /* I multiply by the learning rate here to stabilize it */
      lastWeightDelta = learningRate *
	(deriv * lastWeightDelta) / (M->lastValue - deriv);
    M->lastValue = deriv;
    if (weightDecay > 0.0) lastWeightDelta -= weightDecay * w;
    w += lastWeightDelta;
    if (!isNaN(B->min) && w < B->min) w = B->min;
    else if (!isNaN(B->max) && w > B->max) w = B->max;
    M->lastWeightDelta = w - L->weight;
    L->weight = w;
  }, {
    deriv = L->deriv;
    w = L->weight;
    lastWeightDelta = M->lastWeightDelta;
    gradLin -= lastWeightDelta * deriv;
    lastDeltaLen += SQUARE(lastWeightDelta);
    derivLen += SQUARE(deriv);
    if (lastWeightDelta == 0.0)
      lastWeightDelta = -learningRate * 0.1 * deriv;
    else /* I multiply by the learning rate here to stabilize it */
      lastWeightDelta = learningRate *
	(deriv * lastWeightDelta) / (M->lastValue - deriv);
    M->lastValue = deriv;
    if (weightDecay > 0.0) lastWeightDelta -= weightDecay * w;
    w += lastWeightDelta;
    if (!isNaN(B->min) && w < B->min) w = B->min;
    else if (!isNaN(B->max) && w > B->max) w = B->max;
    M->lastWeightDelta = w - L->weight;
    L->weight = w;
    weightCost += SQUARE(w);
  });
}
#endif /* JUNK */
#endif /* ADVANCED */
