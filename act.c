#include <string.h>
#include "system.h"
#include "util.h"
#include "type.h"
#include "network.h"
#include "act.h"
#include "control.h"
#include "display.h"
#include "graph.h"
#include "train.h"
#include "connect.h"

/**************************** Simple Helper Procedures ***********************/

void clearAllHistories(void) {
  int i, len = Net->historyLength;
  FOR_EACH_GROUP(FOR_EACH_UNIT(G, {
    if (U->inputHistory) for (i = 0; i < len; i++)
      U->inputHistory[i] = NaN;
    if (U->outputHistory) for (i = 0; i < len; i++)
      U->outputHistory[i] = NaN;
    if (U->targetHistory) for (i = 0; i < len; i++)
      U->targetHistory[i] = NaN;
    if (U->outputDerivHistory) for (i = 0; i < len; i++)
      U->outputDerivHistory[i] = NaN;
  }));
}

void storeInputs(Group G, int tick) {
  int index;
  if (!Net->historyLength) return;
  index = HISTORY_INDEX(tick);
  FOR_EACH_UNIT(G, SET_HISTORY(U, inputHistory, index, U->input));
}

void storeOutputs(Group G, int tick) {
  int index;
  if (!Net->historyLength) return;
  index = HISTORY_INDEX(tick);
  FOR_EACH_UNIT(G, SET_HISTORY(U, outputHistory, index, U->output));
}

void storeTargets(Group G, int tick) {
  int index;
  if (!Net->historyLength) return;
  index = HISTORY_INDEX(tick);
  FOR_EACH_UNIT(G, SET_HISTORY(U, targetHistory, index, U->target));
}

void storeOutputsAndTargets(int tick) {
  if (!Net->historyLength) return;
  FOR_EACH_GROUP({
    if (UnitUp || G->type & USE_OUTPUT_HIST)
      storeOutputs(G, tick);
    if ((G->type & ERROR_MASKS) && (UnitUp || G->type & USE_TARGET_HIST))
      storeTargets(G, tick);
  });
}

void storeOutputDerivs(Group G, int tick) {
  int index;
  if (!Net->historyLength) return;
  index = HISTORY_INDEX(tick);
  FOR_EACH_UNIT(G, SET_HISTORY(U, outputDerivHistory, index, U->outputDeriv));
}

void cacheOutputs(Group G) {
  real *O = G->output;
  FOR_EACH_UNIT2(G, O[u] = U->output);
}

/* This resets the cache as well. */
void resetOutputs(Group G) {
  real *O = G->output,
    initOutput = chooseValue(G->initOutput, Net->initOutput);
  FOR_EACH_UNIT2(G, U->output = O[u] = initOutput);
}

/* This caches the restored values as well. */
void restoreOutputs(Group G, int tick) {
  int index = HISTORY_INDEX(tick);
  real *O = G->output;
  FOR_EACH_UNIT2(G, U->output = O[u] = GET_HISTORY(U, outputHistory, index));
}

void restoreInputs(Group G, int tick) {
  int index = HISTORY_INDEX(tick);
  FOR_EACH_UNIT(G, U->input = GET_HISTORY(U, inputHistory, index));
}

void cacheOutputDerivs(Group G) {
  real *D = G->outputDeriv;
  FOR_EACH_UNIT2(G, D[u] = U->outputDeriv);
}

void injectOutputDerivCache(Group G) {
  real *D = G->outputDeriv;
  FOR_EACH_UNIT2(G, U->outputDeriv += D[u]);
}

void resetOutputDerivCache(Group G) {
  memset(G->outputDeriv, 0, G->numUnits * sizeof(real));
}

void resetOutputDerivs(Group G) {
  FOR_EACH_UNIT(G, U->outputDeriv = 0.0);
  memset(G->outputDeriv, 0, G->numUnits * sizeof(real));
}

/* This does not cache the restored values. */
void restoreOutputDerivs(Group G, int tick) {
  int index = HISTORY_INDEX(tick);
  if (G->costType) {
    FOR_EACH_UNIT(G, U->outputDeriv =
		  GET_HISTORY(U, outputDerivHistory, index));
  } else {
    FOR_EACH_UNIT(G, U->outputDeriv = 0.0);
  }
}

void resetForwardIntegrators(Group G) {
  GroupProc P;
  int i;
  if (G->inputType & IN_INTEGR) {
    real val = chooseValue(G->initInput, Net->initInput);
    for (P = G->inputProcs; P; P = P->next)
      if (P->type == IN_INTEGR)
	for (i = 0; i < G->numUnits; i++)
	  P->unitData[i] = val;
  }
  if (G->outputType & (OUT_INTEGR | INTERACT_INTEGR)) {
    real val = chooseValue(G->initOutput, Net->initOutput);
    for (P = G->outputProcs; P; P = P->next)
      if (P->type & (OUT_INTEGR | INTERACT_INTEGR))
	for (i = 0; i < G->numUnits; i++)
	  P->unitData[i] = val;
  }
}

void resetBackwardIntegrators(Group G) {
  GroupProc P;
  if (G->inputType & IN_INTEGR) {
    for (P = G->inputProcs; P; P = P->next)
      if (P->type == IN_INTEGR)
	memset(P->unitData, 0, G->numUnits * sizeof(real));
  }
  if (G->outputType & OUT_INTEGR) {
    for (P = G->outputProcs; P; P = P->next)
      if (P->type == OUT_INTEGR)
	memset(P->unitData, 0, G->numUnits * sizeof(real));
  }
}

void resetLinkDerivs(Group G) {
  FOR_EACH_UNIT(G, U->gainDeriv = 0.0; FOR_EACH_LINK(U, L->deriv = 0.0));
}

void scaleLinkDerivsByDt(Group G) {
  real scale = (real) 1.0 / Net->ticksPerInterval;
  FOR_EACH_UNIT(G, {
    U->gainDeriv *= scale;
    FOR_EACH_LINK(U, L->deriv *= scale);
  });
}

void injectLinkDerivNoise(Group G) {
  if (G->noiseProc) {
    real range = chooseValue(G->noiseRange, Net->noiseRange);
    FOR_EACH_UNIT(G, FOR_EACH_LINK(U, {
      L->deriv = G->noiseProc(L->deriv, range);}));
  }
}


/***************************** Input Procedures ******************************/
/* The backward input procedures cannot assume that U->input is what it was
   following the corresponding forward procedure.  Procedures that
   need this information in the backward step must store it in the forward
   step.
   These procedures will override the previous input value. */

static void dotProductInput(Group G, GroupProc P) {
  FOR_EACH_UNIT(G, {
    real input = 0.0;
    FOR_EACH_LINK_FAST_FORW(U, input += V_OUT * L_WGT, {
      input +=  V_OUT * L_WGT + V_OUT_(1) * L_WGT_(1) +
	V_OUT_(2) * L_WGT_(2) + V_OUT_(3) * L_WGT_(3) +
	V_OUT_(4) * L_WGT_(4) + V_OUT_(5) * L_WGT_(5) +
	V_OUT_(6) * L_WGT_(6) + V_OUT_(7) * L_WGT_(7) +
	V_OUT_(8) * L_WGT_(8) + V_OUT_(9) * L_WGT_(9);
    });
    U->input = input;
  });
}

static void dotProductInputBack(Group G, GroupProc P) {
  FOR_EACH_UNIT(G, {
    real inputDeriv = U->inputDeriv;
    FOR_EACH_LINK_FAST_BACK(U, {
      V_DRV     += inputDeriv * L_WGT;
      L_DRV     += inputDeriv * V_OUT;
    }, {
      V_DRV_(0) += inputDeriv * L_WGT_(0);
      L_DRV_(0) += inputDeriv * V_OUT_(0);
      V_DRV_(1) += inputDeriv * L_WGT_(1);
      L_DRV_(1) += inputDeriv * V_OUT_(1);
      V_DRV_(2) += inputDeriv * L_WGT_(2);
      L_DRV_(2) += inputDeriv * V_OUT_(2);
      V_DRV_(3) += inputDeriv * L_WGT_(3);
      L_DRV_(3) += inputDeriv * V_OUT_(3);
      V_DRV_(4) += inputDeriv * L_WGT_(4);
      L_DRV_(4) += inputDeriv * V_OUT_(4);
      V_DRV_(5) += inputDeriv * L_WGT_(5);
      L_DRV_(5) += inputDeriv * V_OUT_(5);
      V_DRV_(6) += inputDeriv * L_WGT_(6);
      L_DRV_(6) += inputDeriv * V_OUT_(6);
      V_DRV_(7) += inputDeriv * L_WGT_(7);
      L_DRV_(7) += inputDeriv * V_OUT_(7);
      V_DRV_(8) += inputDeriv * L_WGT_(8);
      L_DRV_(8) += inputDeriv * V_OUT_(8);
      V_DRV_(9) += inputDeriv * L_WGT_(9);
      L_DRV_(9) += inputDeriv * V_OUT_(9);
    });
  });
}

static void dotProductInputInit(Group G, GroupProc P) {
  P->forwardProc  = dotProductInput;
  P->backwardProc = dotProductInputBack;
}


static void distanceInput(Group G, GroupProc P) {
  FOR_EACH_UNIT(G, {
    real input = 0.0;
    FOR_EACH_LINK_FORW(U, input += SQUARE(L_WGT - V_OUT));
    U->input = input;
  });
}

static void distanceInputBack(Group G, GroupProc P) {
  real delta;
  FOR_EACH_UNIT(G, {
    real inputDeriv = U->inputDeriv * 2.0;
    if (inputDeriv != 0.0) {
      FOR_EACH_LINK_BACK(U, {
	delta = inputDeriv * (L_WGT - V_OUT);
	V_DRV -= delta;
	L_DRV += delta;
      });
    }
  });
}

static void distanceInputInit(Group G, GroupProc P) {
  P->forwardProc  = distanceInput;
  P->backwardProc = distanceInputBack;
}


static void productInput(Group G, GroupProc P) {
  real *inputStore = P->unitHistoryData[HISTORY_INDEX(Net->currentTick)];
  FOR_EACH_UNIT2(G, {
    real input = 1.0;
    FOR_EACH_LINK_FORW(U, input *= V_OUT * L_WGT);
    U->input = inputStore[u] = input;
  });
}

static void productInputBack(Group G, GroupProc P) {
  real *inputStore = P->unitHistoryData[HISTORY_INDEX(Net->currentTick)];
  FOR_EACH_UNIT2(G, {
    real v; real p = U->inputDeriv * inputStore[u];
    FOR_EACH_LINK_BACK(U, {
      v = p / (V_OUT * L_WGT);
      V_DRV += v * L_WGT;
      L_DRV += v * V_OUT;
    });
  });
}

static void productInputInit(Group G, GroupProc P) {
  P->forwardProc  = productInput;
  P->backwardProc = productInputBack;
  P->unitHistoryData = realMatrix(Net->historyLength, G->numUnits,
				  "PRODUCT:unitHistoryData");
}


static void boltzmannInput(Group G, GroupProc P) {
  real input;
  FOR_EACH_UNIT(G, {
    input = 0.0;
    if (isNaN(U->externalInput) && (isNaN(U->target) || !Net->inGracePeriod)) {
      FOR_EACH_LINK_FORW(U, input += V_OUT * L_WGT);
    }
    U->input = input;
  });
}

static void boltzmannInputBack(Group G, GroupProc P) {
  FOR_EACH_UNIT(G, {
    real output = U->output; real deriv = U->outputDeriv;
    FOR_EACH_LINK_BACK(U, L_DRV += output * V_OUT - deriv * V_DRV);
  });
}

static void boltzmannInputInit(Group G, GroupProc P) {
  P->forwardProc  = boltzmannInput;
  P->backwardProc = boltzmannInputBack;
}


static void softClampInput(Group G, GroupProc P) {
  real initOutput = chooseValue(G->initOutput, Net->initOutput),
    gain = chooseValue(G->gain, Net->gain),
    strength = chooseValue(G->clampStrength, Net->clampStrength), val;
  FOR_EACH_UNIT(G, {
    if (!isNaN(U->externalInput)) {
      val = initOutput + strength * (U->externalInput - initOutput);
      U->input += INV_SIGMOID(val, gain);
    }
  });
}

/* There is no softClampInputBack */

static void softClampInputInit(Group G, GroupProc P) {
  P->forwardProc = softClampInput;
}


static void integrateInput(Group G, GroupProc P) {
  real dt = Net->dt * G->dtScale, *lastInput = P->unitData;
  FOR_EACH_UNIT2(G, {
    lastInput[u] += dt * U->dtScale * (U->input - lastInput[u]);
    U->input = lastInput[u];
  });
}

static void integrateInputBack(Group G, GroupProc P) {
  real dt = Net->dt * G->dtScale, *lastInputDeriv = P->unitData;
  FOR_EACH_UNIT2(G, {
    real d = dt * U->dtScale * lastInputDeriv[u];
    lastInputDeriv[u] += U->inputDeriv - d;
    U->inputDeriv = d;
  });
}

static void integrateInputInit(Group G, GroupProc P) {
  P->forwardProc  = integrateInput;
  P->backwardProc = integrateInputBack;
  P->unitData = realArray(G->numUnits, "IN_INTEGR:unitData");
}


static void normalizeInput(Group G, GroupProc P) {
  int tick = HISTORY_INDEX(Net->currentTick);
  real *normedInput = P->unitHistoryData[tick];
  double scale = 0.0;
  FOR_EACH_UNIT(G, scale += U->input);
  if (scale != 0.0) {
    scale = (double) 1.0 / scale;
    FOR_EACH_UNIT2(G, U->input *= scale; normedInput[u] = U->input);
  } else FOR_EACH_UNIT2(G, normedInput[u] = U->input);
  P->groupHistoryData[tick] = scale;
}

static void normalizeInputBack(Group G, GroupProc P) {
  int tick = HISTORY_INDEX(Net->currentTick);
  real shift = 0.0, scale = P->groupHistoryData[tick],
    *normedInput = P->unitHistoryData[tick];
  FOR_EACH_UNIT2(G, shift += U->inputDeriv * normedInput[u]);
  FOR_EACH_UNIT(G, U->inputDeriv = scale * (U->inputDeriv - shift););
}

static void normalizeInputInit(Group G, GroupProc P) {
  P->forwardProc  = normalizeInput;
  P->backwardProc = normalizeInputBack;
  P->groupHistoryData = realArray(Net->historyLength,
				  "IN_NORM:groupHistoryData");
  P->unitHistoryData = realMatrix(Net->historyLength, G->numUnits,
				  "IN_NORM:unitHistoryData");
}


static void noisyInput(Group G, GroupProc P) {
  if (G->noiseProc) {
    real range = chooseValue(G->noiseRange, Net->noiseRange);
    FOR_EACH_UNIT(G, U->input = G->noiseProc(U->input, range));
  }
}

/* There is no noisyInputBack because there is no need to restore the input
   to the clean value as there is with the output. */

static void noisyInputInit(Group G, GroupProc P) {
  P->forwardProc  = noisyInput;
}


/* There is no noisyInputDeriv forward */

static void noisyInputDerivBack(Group G, GroupProc P) {
  if (G->noiseProc) {
    real range = chooseValue(G->noiseRange, Net->noiseRange);
    FOR_EACH_UNIT(G, U->inputDeriv = G->noiseProc(U->inputDeriv, range));
  }
}

static void noisyInputDerivInit(Group G, GroupProc P) {
  P->backwardProc = noisyInputDerivBack;
}


/* No error is backpropagated. */
static void copyInputs(Group G, GroupProc P) {
  Group source;
  int offset;
  CopyData D = (CopyData) P->otherData;
  if (!D) return;
  source = D->source;
  offset = D->offset;
  FOR_EACH_UNIT2(G, {
    U->input = *((real *)((char *)(source->unit + u) + offset));
  });
}

static void copyInputsInit(Group G, GroupProc P) {
  P->forwardProc = copyInputs;
}


static void incrementClampInput(Group G, GroupProc P) {
  real strength = chooseValue(G->clampStrength, Net->clampStrength);
  FOR_EACH_UNIT(G, {
    if (!isNaN(U->externalInput)) U->input += strength * U->externalInput;
  });
}

/* There is no incrementClampInputBack */

static void incrementClampInputInit(Group G, GroupProc P) {
  P->forwardProc  = incrementClampInput;
}


/***************************** Output Procedures *****************************/
/* The backward output procedures may assume that U->output and U->input are
   what they were following the corresponding forward procedure.  Procedures
   that alter the unit output in the forward step must undo the alteration in
   the backward step.
   These procedures will override the previous output value. */

static void linearOutput(Group G, GroupProc P) {
  FOR_EACH_UNIT(G, U->output = U->input);
}

static void linearOutputBack(Group G, GroupProc P) {
  FOR_EACH_UNIT(G, U->inputDeriv = U->outputDeriv);
}

static void linearOutputInit(Group G, GroupProc P) {
  P->forwardProc  = linearOutput;
  P->backwardProc = linearOutputBack;
}


static void logisticOutput(Group G, GroupProc P) {
  if (G->type & ADAPTIVE_GAIN) {
    FOR_EACH_UNIT(G, {
      U->output = fastSigmoid(U->input * U->gain);
    });
  } else {
    real gain = chooseValue(G->gain, Net->gain);
    FOR_EACH_UNIT(G, U->output = fastSigmoid(U->input * gain));
  }
}

static void logisticOutputBack(Group G, GroupProc P) {
  if (G->type & ADAPTIVE_GAIN) {
    real x;
    FOR_EACH_UNIT(G, {
      x = U->outputDeriv * U->output * (1.0 - U->output);
      U->inputDeriv = x * U->gain;
      U->gainDeriv += x * U->input;
    });
  } else {
    real gain = chooseValue(G->gain, Net->gain);
    FOR_EACH_UNIT(G, U->inputDeriv = U->outputDeriv *
		  U->output * (1.0 - U->output) * gain);
  }
}

static void logisticOutputInit(Group G, GroupProc P) {
  P->forwardProc  = logisticOutput;
  P->backwardProc = logisticOutputBack;
}


static void ternaryOutput(Group G, GroupProc P) {
  real x, z, gain = chooseValue(G->gain, Net->gain),
    y = EXP(gain * chooseValue(G->ternaryShift, Net->ternaryShift));
  FOR_EACH_UNIT(G, {
    x = EXP(gain * U->input);
    z = x * y;
    U->output = (x * z - y) / ((x + y) * (z + 1.0));
  });
}

static void ternaryOutputBack(Group G, GroupProc P) {
  real x, z, v, w, gain = chooseValue(G->gain, Net->gain),
    y = EXP(gain * chooseValue(G->ternaryShift, Net->ternaryShift));
  FOR_EACH_UNIT(G, {
    x = EXP(gain * U->input);
    z = x * y; v = SQUARE(x + y); w = SQUARE(z + 1.0);
    U->inputDeriv = U->outputDeriv * gain * z * (v + w) / (v * w);
  });
}

static void ternaryOutputInit(Group G, GroupProc P) {
  P->forwardProc  = ternaryOutput;
  P->backwardProc = ternaryOutputBack;
}


static void tanhOutput(Group G, GroupProc P) {
  if (G->type & ADAPTIVE_GAIN) {
    FOR_EACH_UNIT(G, U->output = CALC_TANH(U->gain * U->input));
  } else {
    real gain = chooseValue(G->gain, Net->gain);
    FOR_EACH_UNIT(G, U->output = CALC_TANH(gain * U->input));
  }
}

static void tanhOutputBack(Group G, GroupProc P) {
  if (G->type & ADAPTIVE_GAIN) {
    real x;
    FOR_EACH_UNIT(G, {
      x = U->outputDeriv * TANH_DERIV(U->input);
      U->inputDeriv = x * U->gain;
      U->gainDeriv += x * U->input;
    });
  } else {
    real gain = chooseValue(G->gain, Net->gain);
    FOR_EACH_UNIT(G, U->inputDeriv = U->outputDeriv * gain *
                  TANH_DERIV(U->input));
  }
}

static void tanhOutputInit(Group G, GroupProc P) {
  P->forwardProc  = tanhOutput;
  P->backwardProc = tanhOutputBack;
}


static void exponentialOutput(Group G, GroupProc P) {
  FOR_EACH_UNIT(G, U->output = EXP(U->input));
}

static void exponentialOutputBack(Group G, GroupProc P) {
  FOR_EACH_UNIT(G, U->inputDeriv = U->output * U->outputDeriv);
}

static void exponentialOutputInit(Group G, GroupProc P) {
  P->forwardProc  = exponentialOutput;
  P->backwardProc = exponentialOutputBack;
}


static void gaussianOutput(Group G, GroupProc P) {
  real x;
  if (G->type & ADAPTIVE_GAIN) {
    FOR_EACH_UNIT(G, {
      x = U->input * U->gain;
      U->output = EXP(-x * x);
    });
  } else {
    real gain = chooseValue(G->gain, Net->gain);
    FOR_EACH_UNIT(G, {
      x = U->input * gain;
      U->output = EXP(-x * x);
    });
  }
}

static void gaussianOutputBack(Group G, GroupProc P) {
  real scale;
  if (G->type & ADAPTIVE_GAIN) {
    FOR_EACH_UNIT(G, {
      scale = -2.0 * U->input * U->gain * U->output * U->outputDeriv;
      U->inputDeriv = scale * U->gain;
      U->gainDeriv += scale * U->input;
    });
  } else {
    real gain = chooseValue(G->gain, Net->gain);
    scale = -2.0 * gain * gain;
    FOR_EACH_UNIT(G, U->inputDeriv = U->outputDeriv * U->output *
		  U->input * scale);
  }
}

static void gaussianOutputInit(Group G, GroupProc P) {
  P->forwardProc  = gaussianOutput;
  P->backwardProc = gaussianOutputBack;
}


/* This differs from an exponential and then a normalization because it does
   the shift to avoid overflow. */
static void softMaxOutput(Group G, GroupProc P) {
  double maxInput = 0.0, outputSum = 0.0, scale;
  FOR_EACH_UNIT(G, if (U->input > maxInput) maxInput = U->input);
  FOR_EACH_UNIT(G, U->output = EXP(U->input - maxInput);
	       outputSum += U->output);
  scale = (real) 1.0 / outputSum;
  FOR_EACH_UNIT(G, U->output *= scale);
}

static void softMaxOutputBack(Group G, GroupProc P) {
  double outputDerivSum = 0.0;
  FOR_EACH_UNIT(G, outputDerivSum += U->outputDeriv * U->output);
  FOR_EACH_UNIT(G, U->inputDeriv = U->output *
		(U->outputDeriv - outputDerivSum));
}

static void softMaxOutputInit(Group G, GroupProc P) {
  P->forwardProc  = softMaxOutput;
  P->backwardProc = softMaxOutputBack;
}


static real distanceSquared(real ai, real aj, real bi, real bj,
			    int cols, int rows, flag periodic) {
  real di, dj;
  di = ABS(ai - bi); dj = ABS(aj - bj);
  if (periodic) {
    if (2 * di > cols) di = cols - di;
    if (2 * dj > rows) dj = rows - dj;
  }
  return SQUARE(di) + SQUARE(dj);
}

static void kohonenOutput(Group G, GroupProc P) {
  real max = 0.0, min = LARGE_VAL, scale, neigh;
  int i, j, mu = 0, mi, mj, cols = G->numColumns, rows = G->numUnits / cols;
  flag periodic = G->periodicBoundary;

  FOR_EACH_UNIT(G, {
    if (U->input > max) max = U->input;
    if (U->input < min) {min = U->input; mu = U->num;}
  });
  scale = (real) 1.0 / max;
  neigh = SQUARE(G->neighborhood);
  mi = mu % cols;  mj = mu / cols;
  FOR_EACH_UNIT(G, {
    i = U->num % cols; j = U->num / cols;
    if (distanceSquared(i, j, mi, mj, cols, rows, periodic) <= neigh)
      U->output = 1.0 - U->input * scale;
    else U->output = 0.0;
  });
}

/* The gain is just used as some kind of scaling factor. */
static void kohonenOutputBack(Group G, GroupProc P) {
  FOR_EACH_UNIT(G, U->inputDeriv = (U->output > 0) ? 1.0 : 0.0);
}

static void kohonenOutputInit(Group G, GroupProc P) {
  P->forwardProc  = kohonenOutput;
  P->backwardProc = kohonenOutputBack;
}


static void boltzmannOutput(Group G, GroupProc P) {
  real gain = chooseValue(G->gain, Net->gain),
    dt = Net->dt * G->dtScale, *lastOutput = P->unitData;
  FOR_EACH_UNIT2(G, {
    lastOutput[u] = U->output;
    if (!isNaN(U->externalInput))
      U->output = U->externalInput;
    else if (!isNaN(U->target) && Net->inGracePeriod)
      U->output = U->target;
    else U->output += dt * U->dtScale * (SIGMOID(U->input, gain) - U->output);
  });
}

/* There is no boltzmannOutputBack */

static void boltzmannOutputInit(Group G, GroupProc P) {
  P->forwardProc  = boltzmannOutput;
  P->unitData = realArray(G->numUnits, "OUT_BOLTZ:unitData");
}


static void hardClampOutput(Group G, GroupProc P) {
  int tick = HISTORY_INDEX(Net->currentTick);
  real *externalInputHistory = P->unitHistoryData[tick];
  FOR_EACH_UNIT2(G, {
    if (!isNaN(U->externalInput))
      U->output = U->externalInput;
    externalInputHistory[u] = U->externalInput;
  });
}

static void hardClampOutputBack(Group G, GroupProc P) {
  int tick = HISTORY_INDEX(Net->currentTick);
  real *externalInputHistory = P->unitHistoryData[tick];
  FOR_EACH_UNIT2(G, {
    if (!isNaN(externalInputHistory[u]))
      U->inputDeriv = 0.0;
  });
}

static void hardClampOutputInit(Group G, GroupProc P) {
  P->forwardProc  = hardClampOutput;
  P->backwardProc = hardClampOutputBack;
  P->unitHistoryData = realMatrix(Net->historyLength, G->numUnits,
				  "HARD_CLAMP:unitHistoryData");
}


static void biasClampOutput(Group G, GroupProc P) {
  FOR_EACH_UNIT(G, U->output = 1.0);
}

static void biasClampOutputBack(Group G, GroupProc P) {
  FOR_EACH_UNIT(G, U->inputDeriv = 0.0);
}

static void biasClampOutputInit(Group G, GroupProc P) {
  P->forwardProc  = biasClampOutput;
  P->backwardProc = biasClampOutputBack;
}


/* This just adds in the source unit's output so it could be used to sum the
   outputs of several groups. */
static void elmanClampOutput(Group G, GroupProc P) {
  real *elmanInput;
  if (!(P->otherData)) return;
  elmanInput = ((Group) P->otherData)->output;
  FOR_EACH_UNIT2(G, U->output += elmanInput[u]);
}

/* For this to work the source group's output array must be the same as it was
   when the forward was run. */
static void elmanClampOutputBack(Group G, GroupProc P) {
  real *elmanInput, *elmanDeriv;
  if (!(P->otherData)) return;
  elmanInput = ((Group) P->otherData)->output;
  elmanDeriv = ((Group) P->otherData)->outputDeriv;
  FOR_EACH_UNIT2(G, {
    elmanDeriv[u] += U->outputDeriv;
    U->output -= elmanInput[u];
  });
}

static void elmanClampOutputInit(Group G, GroupProc P) {
  P->forwardProc  = elmanClampOutput;
  P->backwardProc = elmanClampOutputBack;
}


static void weakClampOutput(Group G, GroupProc P) {
  real *originalOutput = P->unitHistoryData[HISTORY_INDEX(Net->currentTick)],
    strength = chooseValue(G->clampStrength, Net->clampStrength);
  FOR_EACH_UNIT2(G, {
    originalOutput[u] = U->output;
    if (!isNaN(U->externalInput))
      U->output += strength * (U->externalInput - U->output);
  });
}

static void weakClampOutputBack(Group G, GroupProc P) {
  real *originalOutput = P->unitHistoryData[HISTORY_INDEX(Net->currentTick)],
    scale = 1.0 - chooseValue(G->clampStrength, Net->clampStrength);
  FOR_EACH_UNIT2(G, {
    if (U->output != originalOutput[u]) {
      U->output = originalOutput[u];
      U->outputDeriv *= scale;
    }
  });
}

static void weakClampOutputInit(Group G, GroupProc P) {
  P->forwardProc  = weakClampOutput;
  P->backwardProc = weakClampOutputBack;
  P->unitHistoryData = realMatrix(Net->historyLength, G->numUnits,
				  "WEAK_CLAMP:unitHistoryData");
}


static void integrateOutput(Group G, GroupProc P) {
  real dt = Net->dt * G->dtScale;
  real *lastOutput = P->unitData;
  real *instantOutput = P->unitHistoryData[HISTORY_INDEX(Net->currentTick)];
  FOR_EACH_UNIT2(G, {
    instantOutput[u] = U->output;
    lastOutput[u] += dt * U->dtScale * (U->output - lastOutput[u]);
    U->output = lastOutput[u];
  });
}

static void integrateOutputBack(Group G, GroupProc P) {
  real dt = Net->dt * G->dtScale;
  real *lastOutputDeriv = P->unitData;
  real *instantOutput = P->unitHistoryData[HISTORY_INDEX(Net->currentTick)];
  FOR_EACH_UNIT2(G, {
    real d = dt * U->dtScale * lastOutputDeriv[u];
    U->output = instantOutput[u];
    lastOutputDeriv[u] += U->outputDeriv - d;
    U->outputDeriv = d;
  });
}

static void integrateOutputInit(Group G, GroupProc P) {
  P->forwardProc  = integrateOutput;
  P->backwardProc = integrateOutputBack;
  P->unitData = realArray(G->numUnits, "OUT_INTEGR:unitData");
  P->unitHistoryData = realMatrix(Net->historyLength, G->numUnits,
				  "OUT_INTEGR:unitHistoryData");
}


static void normalizeOutput(Group G, GroupProc P) {
  int tick = HISTORY_INDEX(Net->currentTick);
  real scale = 0.0, *originalOutput = P->unitHistoryData[tick];
  FOR_EACH_UNIT(G, scale += U->output);
  if (scale != 0.0) {
    scale = (real) 1.0 / scale;
    FOR_EACH_UNIT2(G, originalOutput[u] = U->output; U->output *= scale);
  } else FOR_EACH_UNIT2(G, originalOutput[u] = U->output);
  P->groupHistoryData[tick] = scale;
}

static void normalizeOutputBack(Group G, GroupProc P) {
  int tick = HISTORY_INDEX(Net->currentTick);
  real shift = 0.0, scale = P->groupHistoryData[tick],
    *originalOutput = P->unitHistoryData[tick];
  FOR_EACH_UNIT(G, shift += U->outputDeriv * U->output);
  FOR_EACH_UNIT2(G, {
    U->outputDeriv = scale * (U->outputDeriv - shift);
    U->output = originalOutput[u];
  });
}

static void normalizeOutputInit(Group G, GroupProc P) {
  P->forwardProc  = normalizeOutput;
  P->backwardProc = normalizeOutputBack;
  P->groupHistoryData = realArray(Net->historyLength,
				  "OUT_NORM:groupHistoryData");
  P->unitHistoryData = realMatrix(Net->historyLength, G->numUnits,
				  "OUT_NORM:unitHistoryData");
}


static void noisyOutput(Group G, GroupProc P) {
  real *cleanOutput = P->unitHistoryData[HISTORY_INDEX(Net->currentTick)],
    range = chooseValue(G->noiseRange, Net->noiseRange);
  FOR_EACH_UNIT2(G, {
    cleanOutput[u] = U->output;
    if (G->noiseProc) U->output = G->noiseProc(U->output, range);
  });
}

static void noisyOutputBack(Group G, GroupProc P) {
  real *cleanOutput = P->unitHistoryData[HISTORY_INDEX(Net->currentTick)];
  FOR_EACH_UNIT2(G, U->output = cleanOutput[u]);
}

static void noisyOutputInit(Group G, GroupProc P) {
  P->forwardProc  = noisyOutput;
  P->backwardProc = noisyOutputBack;
  P->unitHistoryData = realMatrix(Net->historyLength, G->numUnits,
				  "OUT_NOISE:unitHistoryData");
}


/* There is no noiseOutputDeriv forward */

static void noisyOutputDerivBack(Group G, GroupProc P) {
  if (G->noiseProc) {
    real range = chooseValue(G->noiseRange, Net->noiseRange);
    FOR_EACH_UNIT(G, U->outputDeriv =
		  G->noiseProc(U->outputDeriv, range));
  }
}

static void noisyOutputDerivInit(Group G, GroupProc P) {
  P->backwardProc = noisyOutputDerivBack;
}


static void croppedOutput(Group G, GroupProc P) {
  real *originalOutput = P->unitHistoryData[HISTORY_INDEX(Net->currentTick)];
  real max = G->maxOutput, min = G->minOutput;
  FOR_EACH_UNIT2(G, {
    originalOutput[u] = U->output;
    if (U->output > max) U->output = max;
    else if (U->output < min) U->output = min;
  });
}

static void croppedOutputBack(Group G, GroupProc P) {
  real *originalOutput = P->unitHistoryData[HISTORY_INDEX(Net->currentTick)];
  FOR_EACH_UNIT2(G, U->output = originalOutput[u]);
}

static void croppedOutputInit(Group G, GroupProc P) {
  P->forwardProc  = croppedOutput;
  P->backwardProc = croppedOutputBack;
  P->unitHistoryData = realMatrix(Net->historyLength, G->numUnits,
				  "OUT_CROPPED:unitHistoryData");
}


/* No error is backpropagated. */
static void copyOutputs(Group G, GroupProc P) {
  Group source;
  int offset;
  CopyData D = (CopyData) P->otherData;
  if (!D) return;
  source = D->source;
  offset = D->offset;
  FOR_EACH_UNIT2(G, {
    U->output = *((real *)((char *)(source->unit + u) + offset));
  });
}

static void copyOutputsInit(Group G, GroupProc P) {
  P->forwardProc = copyOutputs;
}


static void winnerTakeAllOutput(Group G, GroupProc P) {
  real *originalOutput = P->unitHistoryData[HISTORY_INDEX(Net->currentTick)];
  real max = G->minOutput;
  Unit winner = NULL;
  FOR_EACH_UNIT2(G, {
    originalOutput[u] = U->output;
    if (U->output > max) {max = U->output; winner = U;}
  });
  FOR_EACH_UNIT(G, {
    if (U != winner) U->output = G->minOutput;
  });
}

static void winnerTakeAllOutputBack(Group G, GroupProc P) {
  real *originalOutput = P->unitHistoryData[HISTORY_INDEX(Net->currentTick)];
  FOR_EACH_UNIT2(G, U->output = originalOutput[u]);
}

static void winnerTakeAllOutputInit(Group G, GroupProc P) {
  P->forwardProc  = winnerTakeAllOutput;
  P->backwardProc = winnerTakeAllOutputBack;
  P->unitHistoryData = realMatrix(Net->historyLength, G->numUnits,
				  "OUT_WINNER:unitHistoryData");
}


static void interactiveActivation(Group G, GroupProc P) {
  real *lastOutput = P->unitData;
  real dt   = Net->dt * G->dtScale;
  real max  = G->maxOutput;
  real min  = G->minOutput;
  real rest = chooseValue(G->initOutput, Net->initOutput);
  real in, out;
  FOR_EACH_UNIT2(G, {
    in  = U->input;
    out = lastOutput[u];
    if (in > 0.0) out += dt * U->dtScale * ((max - out) * in - (out - rest));
    else out += dt * U->dtScale * ((out - min) * in - (out - rest));
    if (out > max) out = max;
    else if (out < 0.0) out = 0.0;
    U->output = lastOutput[u] = out;
  });
}

/* There is no interactiveActivationInputBack */

static void interactiveActivationInit(Group G, GroupProc P) {
  P->forwardProc  = interactiveActivation;
  P->backwardProc = NULL;
  P->unitData = realArray(G->numUnits, "INTERACT_INTEGR:unitData");
}


/***************************** Error Procedures ******************************/

static void squaredError(Group G, GroupProc P) {
  real output, target, error = 0.0,
    targetRadius = chooseValue(G->targetRadius, Net->targetRadius),
    zeroErrorRadius = chooseValue(G->zeroErrorRadius, Net->zeroErrorRadius);
  if (targetRadius != 0.0 || zeroErrorRadius != 0.0) {
    FOR_EACH_UNIT(G, {
      if (!isNaN(U->target)) {
	output = U->output;  target = U->target;
	target = U->adjustedTarget =
	  ADJUSTED_TARGET(output, target, targetRadius, zeroErrorRadius);
	error += SQUARE(output - target);
      }
    });
  } else {
    FOR_EACH_UNIT(G, {
      if (!isNaN(U->target)) {
	output = U->output;
	target = U->adjustedTarget = U->target;
	error += SQUARE(output - target);
      }
    });
  }
  error *= G->errorScale / Net->ticksPerInterval *
    ((Net->pseudoExampleFreq) ? Net->currentExample->frequency : 1.0);
  G->error += error;
  Net->error += error;
}

static void squaredErrorBack(Group G, GroupProc P) {
  real scale = ((Net->pseudoExampleFreq) ?
		Net->currentExample->frequency : 1.0) * G->errorScale * 2.0;
  FOR_EACH_UNIT(G, if (!isNaN(U->target)) {
    U->outputDeriv += scale * (U->output - U->adjustedTarget);
  });
}

static void squaredErrorInit(Group G, GroupProc P) {
  P->forwardProc  = squaredError;
  P->backwardProc = squaredErrorBack;
}


static void crossEntropyError(Group G, GroupProc P) {
  real output, target, error = 0.0,
    targetRadius = chooseValue(G->targetRadius, Net->targetRadius),
    zeroErrorRadius = chooseValue(G->zeroErrorRadius, Net->zeroErrorRadius);
  if (targetRadius != 0.0 || zeroErrorRadius != 0.0) {
    FOR_EACH_UNIT(G, {
      if (!isNaN(U->target)) {
	output = U->output;  target = U->target;
	target = U->adjustedTarget =
	  ADJUSTED_TARGET(output, target, targetRadius, zeroErrorRadius);
	error += CROSS_ENTROPY_ERROR(output, target);
      }
    });
  } else {
    FOR_EACH_UNIT(G, {
      if (!isNaN(U->target)) {
	output = U->output;
	target = U->adjustedTarget = U->target;
	error += CROSS_ENTROPY_ERROR(output, target);
      }
    });
  }
  error *= G->errorScale / Net->ticksPerInterval *
    ((Net->pseudoExampleFreq) ? Net->currentExample->frequency : 1.0);
  G->error += error;
  Net->error += error;
}

static void crossEntropyErrorBack(Group G, GroupProc P) {
  real scale = ((Net->pseudoExampleFreq) ? Net->currentExample->frequency :
		1.0) * G->errorScale, output, target;
  FOR_EACH_UNIT(G, {
    if (!isNaN(U->target)) {
      output = U->output; target = U->adjustedTarget;
      U->outputDeriv += scale * CROSS_ENTROPY_DERIV(output, target);
    }
  });
}

static void crossEntropyErrorInit(Group G, GroupProc P) {
  P->forwardProc  = crossEntropyError;
  P->backwardProc = crossEntropyErrorBack;
}


static void divergenceError(Group G, GroupProc P) {
  real output, target, error = 0.0,
    targetRadius = chooseValue(G->targetRadius, Net->targetRadius),
    zeroErrorRadius = chooseValue(G->zeroErrorRadius, Net->zeroErrorRadius);
  if (targetRadius != 0.0 || zeroErrorRadius != 0.0) {
    FOR_EACH_UNIT(G, {
      if (!isNaN(U->target)) {
	output = U->output; target = U->target;
	target = U->adjustedTarget =
	  ADJUSTED_TARGET(output, target, targetRadius, zeroErrorRadius);
	error += DIVERGENCE_ERROR(output, target);
      }
    });
  } else {
    FOR_EACH_UNIT(G, {
      if (!isNaN(U->target)) {
	output = U->output;
	target = U->adjustedTarget = U->target;
	error += DIVERGENCE_ERROR(output, target);
      }
    });
  }
  error *= G->errorScale / Net->ticksPerInterval *
    ((Net->pseudoExampleFreq) ? Net->currentExample->frequency : 1.0);
  G->error += error;
  Net->error += error;
}

static void divergenceErrorBack(Group G, GroupProc P) {
  real scale = ((Net->pseudoExampleFreq) ? Net->currentExample->frequency :
		1.0) * G->errorScale, output, target;
  FOR_EACH_UNIT(G, {
    if (!isNaN(U->target)) {
      output = U->output; target = U->adjustedTarget;
      U->outputDeriv += scale * DIVERGENCE_DERIV(output, target);
    }
  });
}

static void divergenceErrorInit(Group G, GroupProc P) {
  P->forwardProc  = divergenceError;
  P->backwardProc = divergenceErrorBack;
}


typedef struct cosineData {
  real cosine;
  real invdotprod;
  real invsqoutlen;
} *CosineData;

static void cosineError(Group G, GroupProc P) {
  real output, target, error = 0.0, dotprod = 0.0, sqoutlen = 0.0,
    sqtarlen = 0.0,
    targetRadius = chooseValue(G->targetRadius, Net->targetRadius),
    zeroErrorRadius = chooseValue(G->zeroErrorRadius, Net->zeroErrorRadius);
  CosineData data = (CosineData) P->otherData;

  if (targetRadius != 0.0 || zeroErrorRadius != 0.0) {
    FOR_EACH_UNIT(G, {
      if (!isNaN(U->target)) {
	output = U->output;  target = U->target;
	target = U->adjustedTarget =
	  ADJUSTED_TARGET(output, target, targetRadius, zeroErrorRadius);
        sqoutlen += output * output;
        sqtarlen += target * target;
        dotprod  += output * target;
      }
    });
  } else {
    FOR_EACH_UNIT(G, {
      if (!isNaN(U->target)) {
	output = U->output;
	target = U->adjustedTarget = U->target;
        sqoutlen += output * output;
        sqtarlen += target * target;
        dotprod  += output * target;
      }
    });
  }
  if (sqoutlen * sqtarlen == 0.0) data->cosine = 0.0;
  else data->cosine = dotprod / SQRT(sqoutlen * sqtarlen);
  data->cosine *= ((Net->pseudoExampleFreq) ?
		   Net->currentExample->frequency : 1.0) * G->errorScale;
  data->invdotprod  = 1.0 / dotprod;
  data->invsqoutlen = 1.0 / sqoutlen;

  error = (1.0 - data->cosine) * G->errorScale / Net->ticksPerInterval *
    ((Net->pseudoExampleFreq) ? Net->currentExample->frequency : 1.0);
  G->error += error;
  Net->error += error;
}

static void cosineErrorBack(Group G, GroupProc P) {
  CosineData data = (CosineData) P->otherData;
  real cosine = data->cosine, invdotprod = data->invdotprod,
    invsqoutlen = data->invsqoutlen;
  if (cosine == 0.0) return;
  FOR_EACH_UNIT(G, if (!isNaN(U->target)) {
    U->outputDeriv += cosine * (U->output * invsqoutlen -
				U->adjustedTarget * invdotprod);
  });
}

static void cosineErrorInit(Group G, GroupProc P) {
  P->forwardProc  = cosineError;
  P->backwardProc = cosineErrorBack;
  P->otherData    = (CosineData) safeMalloc(sizeof(struct cosineData),
					    "cosineErrorInit:P->otherData");
}


/* This one isn't an error procedure, but it is considered a cost function. */
static void copyTargets(Group G, GroupProc P) {
  Group source;
  int offset;
  CopyData D = (CopyData) P->otherData;
  if (!D) return;
  source = D->source;
  offset = D->offset;
  FOR_EACH_UNIT2(G, {
    U->target = *((real *)((char *)(source->unit + u) + offset));
  });
}

static void copyTargetsInit(Group G, GroupProc P) {
  P->forwardProc = copyTargets;
}

/**************************** Output Cost Procedures *************************/

static void linearCost(Group G, GroupProc P) {
  real cost = 0.0, min = G->minOutput, max = G->maxOutput;
  if (isNaN(min) || isNaN(max)) {
    FOR_EACH_UNIT(G, cost += ABS(U->output));
  } else {
    real p = chooseValue(G->outputCostPeak, Net->outputCostPeak),
      invP = 1.0 / (p - min), inv1mP = 1.0 / (max - p);
    FOR_EACH_UNIT(G, cost += (U->output <= p) ? (U->output - min) * invP :
		  (max - U->output) * inv1mP);
  }
  cost *= G->outputCostScale / Net->ticksPerInterval;
  G->outputCost += cost;
  Net->outputCost += cost;
}

static void linearCostBack(Group G, GroupProc P) {
  real strength = Net->outputCostStrength * G->outputCostScale /
    Net->ticksPerInterval, min = G->minOutput, max = G->maxOutput;
  if (isNaN(min) || isNaN(max)) {
    FOR_EACH_UNIT(G, U->outputDeriv += (U->output > 0) ? strength :
		  (U->output < 0) ? -strength : 0.0);
  } else {
    real p = chooseValue(G->outputCostPeak, Net->outputCostPeak),
      leftDeriv = strength / (p - min),
      rightDeriv = strength / (p - max);
    FOR_EACH_UNIT(G, {
      U->outputDeriv += (U->output < p) ? leftDeriv :
	(U->output > p) ? rightDeriv : 0.0;
    });
  }
}

static void linearCostInit(Group G, GroupProc P) {
  P->forwardProc  = linearCost;
  P->backwardProc = linearCostBack;
}


static void quadraticCost(Group G, GroupProc P) {
  real cost = 0.0, min = G->minOutput, max = G->maxOutput;
  if (isNaN(min) || isNaN(max)) {
    FOR_EACH_UNIT(G, cost += SQUARE(U->output));
  } else {
    real p = chooseValue(G->outputCostPeak, Net->outputCostPeak),
      invP = 1.0 / (p - min), inv1mP = 1.0 / (max - p), v;
    FOR_EACH_UNIT(G, {
      v = (U->output <= p) ? ((U->output - min) * invP) :
	(max - U->output) * inv1mP;
      cost += SQUARE(v);
    });
  }
  cost *= G->outputCostScale / Net->ticksPerInterval;
  G->outputCost += cost;
  Net->outputCost += cost;
}

static void quadraticCostBack(Group G, GroupProc P) {
  real strength = Net->outputCostStrength * G->outputCostScale * 2.0 /
    Net->ticksPerInterval, min = G->minOutput, max = G->maxOutput;
  if (isNaN(min) || isNaN(max)) {
    FOR_EACH_UNIT(G, U->outputDeriv += strength * U->output);
  } else {
    real p = chooseValue(G->outputCostPeak, Net->outputCostPeak),
      leftScale = strength / SQUARE(p - min),
      rightScale = -strength / SQUARE(max - p);
    FOR_EACH_UNIT(G, {
      U->outputDeriv += (U->output < p) ? leftScale * (U->output - min) :
	(U->output > p) ? rightScale * (max - U->output) : 0.0;
    });
  }
}

static void quadraticCostInit(Group G, GroupProc P) {
  P->forwardProc  = quadraticCost;
  P->backwardProc = quadraticCostBack;
}


/* Must be a bounded unit */
static void convexQuadraticCost(Group G, GroupProc P) {
  real cost = 0.0,
    p = chooseValue(G->outputCostPeak, Net->outputCostPeak),
    min = (p < 0.5) ? p * 2 - 1 : 0.0,
    scale = 1.0 / (p * p - min);
  FOR_EACH_UNIT(G, cost += (U->output * (p * 2 - U->output) - min) * scale);
  cost *= G->outputCostScale / Net->ticksPerInterval;
  G->outputCost += cost;
  Net->outputCost += cost;
}

static void convexQuadraticCostBack(Group G, GroupProc P) {
  real p = chooseValue(G->outputCostPeak, Net->outputCostPeak),
    min = (p < 0.5) ? 2 * p - 1 : 0.0,
    scale = Net->outputCostStrength * G->outputCostScale * 2.0 /
    (Net->ticksPerInterval * (p * p - min));
  FOR_EACH_UNIT(G, U->outputDeriv += (p - U->output) * scale);
}

static void convexQuadraticCostInit(Group G, GroupProc P) {
  P->forwardProc  = convexQuadraticCost;
  P->backwardProc = convexQuadraticCostBack;
}


/* Must be a bounded unit */
static void logisticCost(Group G, GroupProc P) {
  real cost = 0.0,
    p = chooseValue(G->outputCostPeak, Net->outputCostPeak),
    logp = LOG(p), log1mp = LOG(1.0 - p),
    scale = (p <= 0.5) ? (real) 1.0 / logp : (real) 1.0 / log1mp, x;
  FOR_EACH_UNIT(G, {
    x = U->output;
    if (x <= 0.0) cost += -log1mp * scale + 1.0;
    else if (x >= 1.0) cost += -logp * scale + 1.0;
    else cost += (x*(LOG(x)-logp) + (1-x)*(LOG(1-x)-log1mp)) * scale + 1.0;
  });
  cost *= G->outputCostScale / Net->ticksPerInterval;
  G->outputCost += cost;
  Net->outputCost += cost;
}

static void logisticCostBack(Group G, GroupProc P) {
  real strength = Net->outputCostStrength * G->outputCostScale /
    Net->ticksPerInterval,
    p = chooseValue(G->outputCostPeak, Net->outputCostPeak),
    logp = LOG(p), log1mp = LOG(1.0-p),
    scale = (p <= 0.5) ? strength / logp : strength / log1mp, x;
  FOR_EACH_UNIT(G, {
    x = U->output;
    if (x < 1e-6) x = 1e-6;
    else if (x > (1.0 - 1e-6)) x = 1.0 - 1e-6;
    U->outputDeriv += (LOG(x) - logp - LOG(1-x) + log1mp) * scale;
  });
}

static void logisticCostInit(Group G, GroupProc P) {
  P->forwardProc  = logisticCost;
  P->backwardProc = logisticCostBack;
}


/* Must be a bounded unit */
static void cosineCost(Group G, GroupProc P) {
  real cost = 0.0,
    p = chooseValue(G->outputCostPeak, Net->outputCostPeak),
    invp = 1.0 / p,
    inv1mp = 1.0 / (1.0 - p);
  FOR_EACH_UNIT(G, {
    cost += (U->output <= p) ? (1 - cos(PI * invp * U->output)) :
      (1 - cos(PI * inv1mp * (U->output - 2 * p + 1)));
  });
  cost *= G->outputCostScale / Net->ticksPerInterval;
  G->outputCost += cost;
  Net->outputCost += cost;
}

static void cosineCostBack(Group G, GroupProc P) {
  real strength = Net->outputCostStrength * G->outputCostScale *
    PI * 0.5 / Net->ticksPerInterval,
    p = chooseValue(G->outputCostPeak, Net->outputCostPeak),
    invp = 1.0 / p,
    inv1mp = 1.0 / (1.0 - p);
  FOR_EACH_UNIT(G, {
    U->outputDeriv += (U->output <= p) ?
      strength * invp * sin(PI * invp * U->output) :
      strength * inv1mp * sin(PI * inv1mp * (U->output - 2 * p + 1));
  });
}

static void cosineCostInit(Group G, GroupProc P) {
  P->forwardProc  = cosineCost;
  P->backwardProc = cosineCostBack;
}


/* This penalizes units for changing their output. */
static void deltaCost(Group G, GroupProc P) {
  real *lastOutput = P->unitData, *thisOutput = P->unitData + G->numUnits,
    delta, cost = 0.0;
  if (G->type & RESET_ON_EXAMPLE &&
      GET_HISTORY(Net, resetHistory, HISTORY_INDEX(Net->currentTick)))
    return;
  FOR_EACH_UNIT2(G, {
    lastOutput[u] = thisOutput[u];
    thisOutput[u] = U->output;
    delta = thisOutput[u] - lastOutput[u];
    cost += delta * delta;
  });
  cost *= G->outputCostScale / Net->ticksPerInterval;
  G->outputCost += cost;
  Net->outputCost += cost;
}

static void deltaCostBack(Group G, GroupProc P) {
  real *lastOutput = P->unitData, *thisOutput = P->unitData + G->numUnits,
    strength;
  if (G->type & RESET_ON_EXAMPLE &&
      GET_HISTORY(Net, resetHistory, HISTORY_INDEX(Net->currentTick))) {
    FOR_EACH_UNIT2(G, lastOutput[u] = U->output);
  } else {
    strength = Net->outputCostStrength * G->outputCostScale * 2.0 /
      Net->ticksPerInterval;
    FOR_EACH_UNIT2(G, {
      U->outputDeriv += strength * (thisOutput[u] - lastOutput[u]);
    });
  }
}

static void deltaCostInit(Group G, GroupProc P) {
  P->forwardProc  = deltaCost;
  P->backwardProc = deltaCostBack;
  P->unitData = realArray(2 * G->numUnits, "DELTA_COST:unitData");
}

/*************************** Group Criterion Functions **********************/

/* Each unit's output must be within the criterion of its target.
   TRUE if all targets are NaN. */
static flag standardGroupCriterion(Group G, real criterion) {
  FOR_EACH_UNIT(G, {
    if (!isNaN(U->target) && (ABS(U->output - U->target) >= criterion))
      return FALSE;
  });
  return TRUE;
}

static void standardGroupCriterionInit(Group G, GroupProc P) {
  G->groupCriterionReached = standardGroupCriterion;
}


/* The most active unit must be the unit with the highest target and
   difference between output and target must be less than the criterion.
   If all targets are NaN, it is TRUE. */
static flag maxGroupCriterion(Group G, real criterion) {
  Unit maxOutUnit = NULL, maxTargUnit = NULL;
  real maxOut = -LARGE_VAL, maxTarg = -LARGE_VAL;
  flag allTargetsNaN = TRUE;
  FOR_EACH_UNIT(G, {
    if (isNaN(U->target)) continue;
    allTargetsNaN = FALSE;
    if (U->output > maxOut)  {maxOut  = U->output; maxOutUnit  = U;}
    if (U->target > maxTarg) {maxTarg = U->target; maxTargUnit = U;}
  });
  if (allTargetsNaN || (maxOutUnit == maxTargUnit &&
			ABS(maxOut - maxTarg) < criterion)) return TRUE;
  return FALSE;
}

static void maxGroupCriterionInit(Group G, GroupProc P) {
  G->groupCriterionReached = maxGroupCriterion;
}


flag groupCriteriaReached(flag training) {
  flag reached = TRUE, atLeastOne = FALSE;
  real criterion;

  FOR_EACH_GROUP(if (G->groupCriterionReached) {
    atLeastOne = TRUE;
    criterion = (training) ?
      chooseValue(G->trainGroupCrit, Net->trainGroupCrit) :
      chooseValue(G->testGroupCrit, Net->testGroupCrit);
    if (!(reached = G->groupCriterionReached(G, criterion))) break;
  });
  return (atLeastOne && reached);
}


/**************************** Main Group Procedures **************************/

/* This is true if every unit in the group has an externalInput. */
static flag fullyClamped(Group G) {
  FOR_EACH_UNIT(G, if (isNaN(U->externalInput)) return FALSE);
  return TRUE;
}

/* This computes the input to each unit. */
void computeInput(Group G, flag alwaysStore) {
  GroupProc P;
  flag clamped = (!G->inputProcs || ((G->outputType & HARD_CLAMP) &&
				     fullyClamped(G)));

  /* Clear the input if it won't be over-written. */
  if (clamped || !(G->inputType & BASIC_INPUT_TYPES)) {
    FOR_EACH_UNIT(G, U->input = 0.0);
  }
  if (!clamped) {
    for (P = G->inputProcs; P; P = P->next)
      if (P->forwardProc) P->forwardProc(G, P);
  }
  /* Record the inputs in the history */
  if (UnitUp || alwaysStore || G->type & USE_INPUT_HIST)
    storeInputs(G, Net->currentTick);
}

/* This takes the units' inputDerivs and increments the sending units'
   outputDerivs. */
void computeInputBack(Group G) {
  GroupProc P;
  if (!G->inputProcs || ((G->outputType & HARD_CLAMP) &&
			 (G->outputType == HARD_CLAMP || fullyClamped(G))))
    return;

  /* Do the backward procedures in reverse order. */
  if (G->inputProcs) {
    if ((P = G->inputProcs->prev)) do {
      if (P->backwardProc) P->backwardProc(G, P);
      P = P->prev;
    } while (P != G->inputProcs->prev);
  }
}


/* This computes the output for each unit and caches and stores them. */
void computeOutput(Group G, flag alwaysStore) {
  GroupProc P;
  /* Clear the output if it won't be over-written. */
  if (!(G->outputType & (BASIC_OUTPUT_TYPES | BIAS_CLAMP)))
    FOR_EACH_UNIT(G, U->output = 0.0);
  /* Do the forward procedures in order. */
  for (P = G->outputProcs; P; P = P->next)
    if (P->forwardProc) P->forwardProc(G, P);
  cacheOutputs(G);
  /* Record the outputs in the history and cache. */
  if (UnitUp || alwaysStore || G->type & USE_OUTPUT_HIST)
    storeOutputs(G, Net->currentTick);
}

/* This takes the units' cached and regular outputDerivs and computes their
   inputDerivs. */
void computeOutputBack(Group G) {
  GroupProc P;
  injectOutputDerivCache(G);
  resetOutputDerivCache(G);
  /* Do the backward procedures in reverse order. */
  if (G->outputProcs) {
    if ((P = G->outputProcs->prev)) do {
      if (P->backwardProc) P->backwardProc(G, P);
      P = P->prev;
    } while (P != G->outputProcs->prev);
  }
}


/* This increments the network's error and outputCost. */
void computeCost(Group G, flag alwaysStore) {
  GroupProc P;
  if (Net->inGracePeriod) {
    if ((UnitUp || alwaysStore || G->type & USE_TARGET_HIST) &&
	G->costType & ERROR_MASKS)
      FOR_EACH_UNIT(G, SET_HISTORY(U, targetHistory,
				   HISTORY_INDEX(Net->currentTick), NaN));
  } else {
    /* Do the forward procedures in order. */
    for (P = G->costProcs; P; P = P->next)
      if (P->forwardProc) P->forwardProc(G, P);
    /* Record the targets in the history. */
    if ((UnitUp || alwaysStore || G->type & USE_TARGET_HIST) &&
	G->costType & ERROR_MASKS)
      storeTargets(G, Net->currentTick);
  }
}

/* This sets and stores the outputDerivs of output units. It overrides any
   previous values in U->outputDeriv. */
void computeCostBack(Group G, flag alwaysStore) {
  GroupProc P;
  FOR_EACH_UNIT(G, U->outputDeriv = 0.0);
  /* Do the backward procedures in reverse order. */
  if (G->costProcs) {
    if (!Net->inGracePeriod && (P = G->costProcs->prev)) do {
      if (P->backwardProc) P->backwardProc(G, P);
      P = P->prev;
    } while (P != G->costProcs->prev);
    /* Only stored if there was an errorProc. */
    if (UnitUp || alwaysStore || G->type & USE_OUT_DERIV_HIST)
      storeOutputDerivs(G, Net->currentTick);
  }
}


/**************** Standard (Non-continuous) Network Procedures ***************/

flag netForward(Event V) {
  FOR_EACH_GROUP({
    computeInput(G,  FALSE);
    computeOutput(G, FALSE);
    computeCost(G,   FALSE);
  });
  return TCL_OK;
}

flag netForwardBackward(Event V) {
  int i, lastSource = -1;

  FOR_EACH_GROUP({
    computeInput(G,  Net->backpropTicks > 1);
    computeOutput(G, Net->backpropTicks > 1);
    computeCost(G,   FALSE);
    resetBackwardIntegrators(G);
    computeCostBack(G, FALSE);
    resetOutputDerivCache(G);
  });
  RUN_PROC(preTickBackProc);
  FOR_EACH_GROUP_BACK(BACKPROP(G));

  /* This will extend the backprop backward in time, which may be used in
     simple recurrent networks.  lastSource is the last group that is an
     Elman layer or a source for one.  This is used to avoid unnecessary
     work. */
  if (Net->backpropTicks > 1) {
    GroupProc P;
    flag wasReset;
    /* Find the last group that we need to consider. */
    FOR_EACH_GROUP({
      if (G->type & ELMAN) {
	if (g > lastSource) lastSource = g;
	for (P = G->outputProcs; P; P = P->next)
	  if (P->type == ELMAN_CLAMP && ((Group) P->otherData) &&
	      ((Group) P->otherData)->num > lastSource)
	    lastSource = ((Group) P->otherData)->num;
      }});

    for (i = 1; i < Net->backpropTicks; i++) {
      wasReset = GET_HISTORY(Net,resetHistory,HISTORY_INDEX(Net->currentTick));
      Net->currentTick--;
      /* Set the outputs to the way they were at the end of t-i. */
      FOR_EACH_GROUP_IN_RANGE_BACK(lastSource, 0, {
	restoreOutputs(G, Net->currentTick);
	restoreInputs(G, Net->currentTick);
	if (wasReset && G->type & RESET_ON_EXAMPLE) {
	  resetOutputDerivCache(G);
	  resetBackwardIntegrators(G);
	}
      });
      /* Backpropagate. */
      FOR_EACH_GROUP_IN_RANGE_BACK(lastSource, 0, {
	FOR_EACH_UNIT(G, U->outputDeriv = 0.0);
	BACKPROP(G);
      });
    }

    /* Restore the network to the original state. */
    Net->currentTick += Net->backpropTicks - 1;
    FOR_EACH_GROUP_IN_RANGE(0, lastSource, {
      restoreOutputs(G, Net->currentTick);
    });
  }
  return TCL_OK;
}


/******************** Simple Recurrent Backprop Through Time *****************/

flag srbpttNetForward(Event V) {
  FOR_EACH_GROUP({
    computeInput(G, TRUE);
    computeOutput(G, TRUE);
    computeCost(G, TRUE);
    computeCostBack(G, TRUE);
  });
  return TCL_OK;
}

flag srbpttNetExampleBack(Example E) {
  /* Clear the output deriv caches. */
  /* If integrating, reset the deriv accumulators. */
  FOR_EACH_GROUP(resetOutputDerivCache(G); resetBackwardIntegrators(G));

  FOR_EACH_GROUP_BACK(BACKPROP(G));

  for (Net->currentTick = Net->ticksOnExample - 2;
       Net->currentTick >= 0; Net->currentTick--) {
    /* Set outputDerivs to the stored instant error derivatives. */
    FOR_EACH_GROUP({
      restoreOutputDerivs(G, Net->currentTick);
      restoreOutputs(G, Net->currentTick);
      restoreInputs(G, Net->currentTick);
    });
    FOR_EACH_GROUP_BACK(BACKPROP(G));
  }
  /* Restore the final outputs for continuity with the next example */
  FOR_EACH_GROUP(restoreOutputs(G, Net->ticksOnExample - 1));
  return TCL_OK;
}


/*********************** Continuous Network Procedures ***********************/

flag continuousNetTickForward(Event V) {
  /* Do the main procedures for each group. */
  FOR_EACH_GROUP(computeInput(G, TRUE));
  FOR_EACH_GROUP(computeOutput(G, TRUE));
  FOR_EACH_GROUP(computeCost(G, TRUE));
  return TCL_OK;
}

flag continuousNetTrainTickForward(Event V) {
  continuousNetTickForward(V);
  /* Calculate and store the outputDerivs. */
  FOR_EACH_GROUP(computeCostBack(G, TRUE));
  return TCL_OK;
}

flag continuousNetExampleBack(Example E) {
  /* Clear the output deriv caches. */
  /* If integrating, reset the deriv accumulators. */
  FOR_EACH_GROUP(resetOutputDerivCache(G); resetBackwardIntegrators(G));

  for (Net->currentTick = Net->ticksOnExample - 1;
       Net->currentTick > 0; Net->currentTick--) {
    /* When you get here, the outputs are the outputs from Net->currentTick and
       the outputDeriv caches contain the backpropagated error from the next
       tick. */
    /* Set outputDerivs to the stored instant error derivatives. */
    FOR_EACH_GROUP(restoreOutputDerivs(G, Net->currentTick));
    /* Compute the instant inputDerivs, first injecting the cache. */
    FOR_EACH_GROUP(computeOutputBack(G));
    /* Restore the outputs from the previous tick. */
    FOR_EACH_GROUP({
      restoreOutputs(G, Net->currentTick - 1);
      restoreInputs(G, Net->currentTick - 1);
    });
    /* Backpropagate across the incoming links. */
    FOR_EACH_GROUP(computeInputBack(G));
  }
  /* Restore the final outputs for continuity with the next example */
  FOR_EACH_GROUP(restoreOutputs(G, Net->ticksOnExample - 1));
  return TCL_OK;
}


/*************************** Generic Example Processing **********************/

flag standardNetRunExample(Example E, flag (*tickProc)(Event), flag *correct) {
  int tick = 0, ticksOnEvent, event = 0;
  flag eventDone, critReached, training = (tickProc == Net->netTrainTick);
  real timeOnEvent, minTime, maxTime, graceTime;
  Event V;

  if (E->set->loadEvent(V = E->event)) return TCL_ERROR;
  ticksOnEvent = 0;
  maxTime = chooseValue(V->maxTime, E->set->maxTime);
  minTime = chooseValue(V->minTime, E->set->minTime);
  graceTime = chooseValue(V->graceTime, E->set->graceTime);
  Net->inGracePeriod = (graceTime > 0.0);

  /* Reset the outputs and integrators if requested. */
  FOR_EACH_GROUP(if (G->type & RESET_ON_EXAMPLE) {
    resetOutputs(G);
    resetForwardIntegrators(G);
  });

  if (Net->type & CONTINUOUS) {
    storeOutputsAndTargets(0);
    Net->exampleHistoryStart = 0;
    Net->eventHistory[tick] = -1;
    tick = 1;
    updateDisplays(ON_TICK);
  } else {
    Net->exampleHistoryStart =
      (Net->exampleHistoryStart + Net->ticksOnExample) % Net->historyLength;
  }
  Net->resetHistory[HISTORY_INDEX(0)] = TRUE;

  RUN_PROC(preEventProc);
  *correct = TRUE;
  for (; tick < Net->maxTicks && event < E->numEvents; tick++) {
    Net->currentTick = tick;
    Net->eventHistory[tick] = event;
    if (tick > 0) Net->resetHistory[HISTORY_INDEX(tick)] = FALSE;

    RUN_PROC(preTickProc);
    if (tickProc(V)) return TCL_ERROR;   /* This is the important call */
    RUN_PROC(postTickProc);

    updateDisplays(ON_TICK);

    eventDone = FALSE;
    critReached = 2;
    ticksOnEvent++;
    /* The SMALL_VAL is to correct for possible floating point inaccuracy */
    timeOnEvent = ((real) ticksOnEvent / Net->ticksPerInterval) + SMALL_VAL;
    /* If we have done enough time on this event, go on */
    if (timeOnEvent >= maxTime || tick >= Net->maxTicks - 1)
      eventDone = TRUE;
    /* If we have gone long enough and reached criterion, go on */
    else if (timeOnEvent >= minTime &&
	     (critReached = groupCriteriaReached(training)))
      eventDone = TRUE;
    /* See if the grace period is over */
    else if (Net->inGracePeriod && timeOnEvent >= graceTime)
      Net->inGracePeriod = FALSE;

    if (eventDone) {
      if (critReached == 2) critReached = groupCriteriaReached(training);
      if (!critReached) *correct = FALSE;
      RUN_PROC(postEventProc);
      updateDisplays(ON_EVENT);

      if (Net->groupCritRequired && !critReached) break;

      if (++event < E->numEvents && ((V = E->event + event))) {
	if (E->set->loadEvent(V)) return TCL_ERROR;
	ticksOnEvent = 0;
	maxTime = chooseValue(V->maxTime, E->set->maxTime);
	minTime = chooseValue(V->minTime, E->set->minTime);
	graceTime = chooseValue(V->graceTime, E->set->graceTime);
	Net->inGracePeriod = (graceTime > 0.0);
	RUN_PROC(preEventProc);
      }
    }
  }
  Net->ticksOnExample = Net->currentTick + 1;

  updateDisplays(ON_EXAMPLE);
  return TCL_OK;
}


/****************************** Major Network Functions **********************/

/* This accumulates link error derivs on a batch of training examples */
flag standardNetTrainBatch(flag *allCorrect) {
  int i, numExamples;
  ExampleSet S;
  Example E;
  flag criterionReached, value = TCL_OK;

  if (!(S = Net->trainingSet))
    return warning("There is no training example set");
  /* Reset the net error */
  Net->error = Net->outputCost = 0.0;

  /* Reset the group error and all the link derivs */
  FOR_EACH_GROUP({
    G->error = G->outputCost = 0.0;
    resetLinkDerivs(G);
  });

  *allCorrect = TRUE;
  /* If batchSize == 0, run through the whole example set */
  numExamples = (Net->batchSize > 0) ? Net->batchSize : S->numExamples;
  if (numExamples == 0 && S->mode == PIPE)
    return warning("You must specify a non-zero batch size when reading from "
		   "a pipe.");

  for (i = 0; i < numExamples; i++) {
    if (loadNextExample(S)) return TCL_ERROR;
    E = S->currentExample;

    RUN_PROC(preExampleProc);
    if (Net->netTrainExample(E, Net->netTrainTick, &criterionReached))
      return TCL_ERROR;
    if (Net->netTrainExampleBack) {
      RUN_PROC(preExampleBackProc);
      if (Net->netTrainExampleBack(E)) return TCL_ERROR;
    }
    RUN_PROC(postExampleProc);

    if (!criterionReached) *allCorrect = FALSE;

    if (Net->outputFile && Net->writeExample) Net->writeExample(E);
    updateDisplays(ON_EXAMPLE);
    if (smartUpdate(FALSE)) {
      value = TCL_ERROR;
      break;
    }
  }

  if (Net->type & CONTINUOUS)
    FOR_EACH_GROUP(scaleLinkDerivsByDt(G));
  FOR_EACH_GROUP(if (G->type & DERIV_NOISE) injectLinkDerivNoise(G));

  return value;
}

static void smartAppendVal(String S, real x, int width) {
  char buf[64];
  smartFormatReal(buf, x, width);
  stringAppend(S, buf);
}

/* If print is true, the results are printed to the terminal, otherwise they
   are returned. */
flag reportError(int examples, int ticks, int reached, flag printResults) {
  int numOutputGroups = 0;
  String S = newString(256);

  FOR_EACH_GROUP(if (G->type & OUTPUT) numOutputGroups++);

  stringAppendV(S, "Test results on %d examples, %d ticks:\n",
                examples, ticks);
  eval("set Test(numExamples) %d", examples);
  eval("set Test(numTicks) %d", ticks);

  stringAppend(S, "                     Network     ");
  if (numOutputGroups > 1) {
    FOR_EACH_GROUP(if (G->type & OUTPUT) {
      stringAppendV(S, " %-12s", G->name);
    });
  }
  stringAppend(S, "\nError total:        ");
  smartAppendVal(S, Net->error, 8);
  eval("set Test(totalError) %g", Net->error);
  if (numOutputGroups > 1) {
    FOR_EACH_GROUP(if (G->type & OUTPUT) {
      stringAppend(S, "     ");
      smartAppendVal(S, G->error, 8);
      eval("set Test(%s.totalError) %d", G->name, ticks);
    });
  }
  stringAppend(S, "\nError per example:  ");
  smartAppendVal(S, Net->error / examples, 8);
  eval("set Test(exampleError) %g", Net->error / examples);
  if (numOutputGroups > 1) {
    FOR_EACH_GROUP(if (G->type & OUTPUT) {
      stringAppend(S, "     ");
      smartAppendVal(S, G->error / examples, 8);
      eval("set Test(%s.exampleError) %g", G->name, Net->error / examples);
    });
  }
  stringAppend(S, "\nError per tick:     ");
  smartAppendVal(S, Net->error / ticks, 8);
  eval("set Test(tickError) %g", Net->error / ticks);
  if (numOutputGroups > 1) {
    FOR_EACH_GROUP(if (G->type & OUTPUT) {
      stringAppend(S, "     ");
      smartAppendVal(S, G->error / ticks, 8);
      eval("set Test(%s.tickError) %g", G->name, Net->error / ticks);
    });
  }
  stringAppend(S, "\nUnit cost per tick: ");
  smartAppendVal(S, Net->outputCost / ticks, 8);
  eval("set Test(totalCost) %g", Net->outputCost);
  eval("set Test(tickCost) %g", Net->outputCost / ticks);
  stringAppendV(S, "\nOutput unit criterion reached on %d examples (%.2f%%)",
	 reached, (real) (reached * 100) / examples);
  eval("set Test(examplesCorrect) %d", reached);
  eval("set Test(percentCorrect) %g", (real) (reached * 100) / examples);

  if (printResults) {
    print(1, "%s\n", S->s);
    result("");
  } else result(S->s);
  freeString(S);
  return TCL_OK;
}


/* This sums up the total error on one pass through the testing set.
   Basically, it "tests" the network.  If printResults is true, the results
   are printed to the terminal.  Otherwise they are returned. */
flag standardNetTestBatch(int numExamples, flag resetError,
                          flag printResults) {
  int i, totalTicks, reached = 0;
  // Does not appear to be used.
  // int set;
  ExampleSet S;
  Example E;
  flag criterionReached, value;

  if (!(S = Net->testingSet) && !(S = Net->trainingSet))
    return warning("there are no current example sets");
  // Does not appear to be used.
  // set = (S == Net->testingSet) ? TEST_SET : TRAIN_SET;

  if (resetError) {
    Net->error = Net->outputCost = 0.0;
    FOR_EACH_GROUP(G->error = G->outputCost = 0.0);
  }

  if (numExamples == 0) {
    numExamples = S->numExamples;
    if (numExamples == 0 && S->mode == PIPE && S->pipeLoop)
      print(1, "Warning: Testing on an infinite pipe\n");
    resetExampleSet(S);
  }

  for (i = 0, totalTicks = 0; numExamples == 0 || i < numExamples; i++) {
    if (loadNextExample(S)) {
      if (S->mode != PIPE) return TCL_ERROR;
      else break;
    }
    E = S->currentExample;

    RUN_PROC(preExampleProc);
    if (Net->netTestExample(E, Net->netTestTick, &criterionReached))
      return TCL_ERROR;
    RUN_PROC(postExampleProc);

    totalTicks += Net->ticksOnExample;
    if (criterionReached) reached++;
    if (Net->outputFile && Net->writeExample) Net->writeExample(E);
    updateDisplays(ON_EXAMPLE);

    if (smartUpdate(FALSE)) break;
  }

  updateDisplays(ON_TRAINING);
  result("");
  if (i != numExamples) {
    append("Testing halted prematurely after %d examples\n", i);
    value = TCL_ERROR;
  } else value = TCL_OK;
  reportError(i, totalTicks, reached, printResults);

  return value;
}


/* This is run when you click on a single example in the Unit Viewer */
flag standardNetRunExampleNum(int num, ExampleSet S) {
  Example E;
  flag criterionReached;
  if (!S) return warning("standardNetRunExampleNum: no example set");
  if (!S->example || num < 0) {
    if (loadNextExample(S)) return TCL_ERROR;
  } else {
    if (num >= S->numExamples)
      return warning("standardNetRunExample: example %d out of range", num);
    if (S->loadExample(S->example[num])) return TCL_ERROR;
  }
  E = S->currentExample;

  RUN_PROC(preExampleProc);
  if (Net->netRunExample(E, Net->netRunTick, &criterionReached))
    return TCL_ERROR;
  RUN_PROC(postExampleProc);

  if (Net->outputFile && Net->writeExample) Net->writeExample(E);
  return TCL_OK;
}


flag initStandardNet(Network N) {
  N->netTrain            = standardNetTrain;
  N->netTrainBatch       = standardNetTrainBatch;
  N->netTrainExample     = standardNetRunExample;
  N->netTrainExampleBack = NULL;
  N->netTrainTick        = netForwardBackward;
  N->netTestBatch        = standardNetTestBatch;
  N->netTestExample      = standardNetRunExample;
  N->netTestTick         = netForward;
  N->netRunExampleNum    = standardNetRunExampleNum;
  N->netRunExample       = standardNetRunExample;
  N->netRunTick          = netForward;
  N->resetNet            = standardResetNet;
  N->saveWeights         = standardSaveWeights;
  N->loadWeights         = standardLoadWeights;
  return TCL_OK;
}

static flag initSRBPTT(Network N) {
  N->netTrainExampleBack = srbpttNetExampleBack;
  N->netTrainTick        = srbpttNetForward;
  return TCL_OK;
}

static flag initContinuousNet(Network N) {
  N->netTrainExampleBack = continuousNetExampleBack;
  N->netTrainTick        = continuousNetTrainTickForward;
  N->netTestTick         = continuousNetTickForward;
  N->netRunTick          = continuousNetTickForward;
  return TCL_OK;
}


/************************* Deterministic Boltzmann Machines ******************/

flag boltzmannUpdate(Event V) {
  /* Do the main procedures for each group, but don't computeCostBack. */
  FOR_EACH_GROUP(computeInput(G, TRUE));
  FOR_EACH_GROUP(computeOutput(G, TRUE));
  FOR_EACH_GROUP({
    if ((UnitUp || G->type & USE_TARGET_HIST) && (G->costType & ERROR_MASKS))
      storeTargets(G, Net->currentTick);
  });
  return TCL_OK;
}

static flag boltzmannSettled(flag training) {
  FOR_EACH_GROUP({
    if (G->outputType & OUT_BOLTZ) {
      GroupProc P;
      real criterion = (training) ?
	chooseValue(G->trainGroupCrit, Net->trainGroupCrit) :
	chooseValue(G->testGroupCrit, Net->testGroupCrit);
      for (P = G->outputProcs; P; P = P->next)
	if (P->type == OUT_BOLTZ) {
	  real *lastOutput = P->unitData;
	  int u;
	  for (u = 0; u < G->numUnits; u++)
	    if (ABS(G->output[u] - lastOutput[u]) > criterion)
	      return FALSE;
	}
    }
  });
  return TRUE;
}

static void initializeBoltzmannOutputs(Group G) {
  real initOutput = chooseValue(G->initOutput, Net->initOutput);

  FOR_EACH_UNIT(G, {
    if (!isNaN(U->externalInput)) U->output = U->externalInput;
    else if (!isNaN(U->target)) U->output = U->target;
    else U->output = initOutput;
  });
  cacheOutputs(G);
}

/* This is very similar to a weakClamp function. */
static void resetBoltzmannOutputs(Group G) {
  real clampStrength = chooseValue(G->clampStrength, Net->clampStrength),
    retain = 1.0 - clampStrength,
    initOutput = chooseValue(G->initOutput, Net->initOutput) * clampStrength;
  FOR_EACH_UNIT(G, {
    if (isNaN(U->externalInput))
      U->output = initOutput + U->output * retain;
  });
  cacheOutputs(G);
}

enum boltzstate {NEW_EVENT, POSITIVE, NEGATIVE};

flag boltzmannNetTrainExample(Example E, flag (*tickProc)(Event),
			      flag *correct) {
  int tick, ticksOnPhase = 0, ticksOnEvent = 0, event = 0, phase;
  flag phaseDone;
  real timeOnPhase, minTime = 0.0, maxTime = 0.0, graceTime = 0.0;
  //real timeOnEvent;
  Event V = NULL;

  *correct = FALSE;
  phase = NEW_EVENT;
  for (tick = 0, event = 0; tick < Net->maxTicks && event < E->numEvents;
       tick++) {
    Net->currentTick = tick;
    Net->eventHistory[tick] = event;

    if (phase == NEW_EVENT) {
      V = E->event + event;
      if (E->set->loadEvent(V)) return TCL_ERROR;
      FOR_EACH_GROUP(if (!(G->type & BIAS)) initializeBoltzmannOutputs(G));

      graceTime = chooseValue(V->graceTime, E->set->graceTime);
      maxTime = chooseValue(V->maxTime, E->set->maxTime) - graceTime;
      minTime = chooseValue(V->minTime, E->set->minTime);
      ticksOnPhase = ticksOnEvent = 0;
      Net->inGracePeriod = TRUE;
      phase = POSITIVE;
    }

    Net->gain = Net->finalGain + (Net->initGain - Net->finalGain) *
      POW(0.5, (real) ticksOnPhase / (Net->annealTime*Net->ticksPerInterval));

    tickProc(V);

    updateDisplays(ON_TICK);

    ticksOnPhase++;
    ticksOnEvent++;
    /* The SMALL_VAL is to correct for possible floating point inaccuracy */
    timeOnPhase = ((real) ticksOnPhase / Net->ticksPerInterval) + SMALL_VAL;
    //timeOnEvent = ((real) ticksOnEvent / Net->ticksPerInterval) + SMALL_VAL;
    if (tick == Net->maxTicks - 1)
      phaseDone = TRUE;
    else if (timeOnPhase < minTime)
      phaseDone = FALSE;
    else if (phase == POSITIVE && timeOnPhase >= graceTime)
      phaseDone = TRUE;
    else if (phase == NEGATIVE && timeOnPhase >= maxTime)
      phaseDone = TRUE;
    else if (boltzmannSettled(TRUE))
      phaseDone = TRUE;
    else phaseDone = FALSE;

    if (phaseDone) {
      if (phase == POSITIVE) {
	FOR_EACH_GROUP({
	  FOR_EACH_UNIT(G, U->outputDeriv = U->output);
	  cacheOutputDerivs(G);
	  if (G->type & RESET_ON_EXAMPLE) resetBoltzmannOutputs(G);
	});
	ticksOnPhase = 0;
	Net->inGracePeriod = FALSE;
	phase = NEGATIVE;
	Net->currentTick = ++tick;
	Net->eventHistory[tick] = event;
	storeOutputsAndTargets(tick);
	updateDisplays(ON_TICK);
      } else {
	FOR_EACH_GROUP(computeCost(G, TRUE); computeInputBack(G));
	event++;
	phase = NEW_EVENT;
      }
    }
  }

  Net->ticksOnExample = Net->currentTick + 1;
  updateDisplays(ON_EXAMPLE);
  return TCL_OK;
}

flag boltzmannNetTestExample(Example E, flag (*tickProc)(Event),
			     flag *correct) {
  int tick, ticksOnEvent = 0, event = 0, phase;
  flag eventDone;
  real timeOnEvent, minTime = 0.0, maxTime = 0.0;
  Event V = NULL;

  *correct = FALSE;
  phase = NEW_EVENT;
  for (tick = 0, event = 0; tick < Net->maxTicks && event < E->numEvents;
       tick++) {
    Net->currentTick = tick;
    Net->eventHistory[tick] = event;

    if (phase == NEW_EVENT) {
      V = E->event + event;
      if (E->set->loadEvent(V)) return TCL_ERROR;
      FOR_EACH_GROUP({
	initializeBoltzmannOutputs(G);
	FOR_EACH_UNIT(G, U->outputDeriv = U->output);
	cacheOutputDerivs(G);
	if (G->type & RESET_ON_EXAMPLE) resetBoltzmannOutputs(G);
      });

      maxTime = chooseValue(V->maxTime, E->set->maxTime) -
	chooseValue(V->graceTime, E->set->graceTime);
      minTime = chooseValue(V->minTime, E->set->minTime);
      ticksOnEvent = 0;
      Net->inGracePeriod = FALSE;
      phase = NEGATIVE;
      storeOutputsAndTargets(tick++);
      Net->currentTick = tick;
      Net->eventHistory[tick] = event;
      updateDisplays(ON_TICK);
    }

    Net->gain = Net->finalGain + (Net->initGain - Net->finalGain) *
      POW(0.5, (real) ticksOnEvent / (Net->annealTime*Net->ticksPerInterval));

    tickProc(V);

    updateDisplays(ON_TICK);

    ticksOnEvent++;
    /* The SMALL_VAL is to correct for possible floating point inaccuracy */
    timeOnEvent = ((real) ticksOnEvent / Net->ticksPerInterval) + SMALL_VAL;
    if (tick == Net->maxTicks - 1)
      eventDone = TRUE;
    else if (timeOnEvent < minTime)
      eventDone = FALSE;
    else if (phase == NEGATIVE && timeOnEvent >= maxTime)
      eventDone = TRUE;
    else if (boltzmannSettled(FALSE))
      eventDone = TRUE;
    else eventDone = FALSE;

    if (eventDone) {
      FOR_EACH_GROUP(computeCost(G, TRUE));
      event++;
      phase = NEW_EVENT;
    }
  }

  Net->ticksOnExample = Net->currentTick + 1;
  updateDisplays(ON_EXAMPLE);
  return TCL_OK;
}


static flag initBoltzmannNet(Network N) {
  N->netTrainExample  = boltzmannNetTrainExample;
  N->netTestExample   = boltzmannNetTestExample;
  N->netRunExample    = boltzmannNetTrainExample;
  N->netTrainTick     = boltzmannUpdate;
  N->netTestTick      = boltzmannUpdate;
  N->netRunTick       = boltzmannUpdate;
  return TCL_OK;
}


/******************************* Type Registration ***************************/

void registerNetAndGroupTypes(void) {
  registerNetType("CONTINUOUS", CONTINUOUS, initContinuousNet);
  registerNetType("SRBPTT", SRBPTT, initSRBPTT);
  registerNetType("BOLTZMANN", BOLTZMANN, initBoltzmannNet);

  registerGroupType("STANDARD_CRIT", STANDARD_CRIT, GROUP,
		    standardGroupCriterionInit);
  registerGroupType("MAX_CRIT", MAX_CRIT, GROUP,
		    maxGroupCriterionInit);

  registerGroupType("DOT_PRODUCT", DOT_PRODUCT, GROUP_INPUT,
		    dotProductInputInit);
  registerGroupType("DISTANCE", DISTANCE, GROUP_INPUT,
		    distanceInputInit);
  registerGroupType("PRODUCT", PRODUCT, GROUP_INPUT,
		    productInputInit);
  registerGroupType("IN_BOLTZ", IN_BOLTZ, GROUP_INPUT,
		    boltzmannInputInit);
  registerGroupType("SOFT_CLAMP", SOFT_CLAMP, GROUP_INPUT,
		    softClampInputInit);
  registerGroupType("IN_INTEGR", IN_INTEGR, GROUP_INPUT,
		    integrateInputInit);
  registerGroupType("IN_NORM", IN_NORM, GROUP_INPUT,
		    normalizeInputInit);
  registerGroupType("IN_NOISE", IN_NOISE, GROUP_INPUT,
		    noisyInputInit);
  registerGroupType("IN_DERIV_NOISE", IN_DERIV_NOISE, GROUP_INPUT,
		    noisyInputDerivInit);
  registerGroupType("IN_COPY", IN_COPY, GROUP_INPUT,
		    copyInputsInit);
  registerGroupType("INCR_CLAMP", INCR_CLAMP, GROUP_INPUT,
		    incrementClampInputInit);

  registerGroupType("LINEAR", LINEAR, GROUP_OUTPUT,
		    linearOutputInit);
  registerGroupType("LOGISTIC", LOGISTIC, GROUP_OUTPUT,
		    logisticOutputInit);
  registerGroupType("TERNARY", TERNARY, GROUP_OUTPUT,
		    ternaryOutputInit);
  registerGroupType("TANH", TANH, GROUP_OUTPUT,
		    tanhOutputInit);
  registerGroupType("EXPONENTIAL", EXPONENTIAL, GROUP_OUTPUT,
		    exponentialOutputInit);
  registerGroupType("GAUSSIAN", GAUSSIAN, GROUP_OUTPUT,
		    gaussianOutputInit);
  registerGroupType("SOFT_MAX", SOFT_MAX, GROUP_OUTPUT,
		    softMaxOutputInit);
  registerGroupType("KOHONEN", KOHONEN, GROUP_OUTPUT,
		    kohonenOutputInit);
  registerGroupType("OUT_BOLTZ", OUT_BOLTZ, GROUP_OUTPUT,
		    boltzmannOutputInit);
  registerGroupType("HARD_CLAMP", HARD_CLAMP, GROUP_OUTPUT,
		    hardClampOutputInit);
  registerGroupType("BIAS_CLAMP", BIAS_CLAMP, GROUP_OUTPUT,
		    biasClampOutputInit);
  registerGroupType("ELMAN_CLAMP", ELMAN_CLAMP, GROUP_OUTPUT,
		    elmanClampOutputInit);
  registerGroupType("WEAK_CLAMP", WEAK_CLAMP, GROUP_OUTPUT,
		    weakClampOutputInit);
  registerGroupType("OUT_INTEGR", OUT_INTEGR, GROUP_OUTPUT,
		    integrateOutputInit);
  registerGroupType("OUT_NORM", OUT_NORM, GROUP_OUTPUT,
		    normalizeOutputInit);
  registerGroupType("OUT_NOISE", OUT_NOISE, GROUP_OUTPUT,
		    noisyOutputInit);
  registerGroupType("OUT_DERIV_NOISE", OUT_DERIV_NOISE, GROUP_OUTPUT,
		    noisyOutputDerivInit);
  registerGroupType("OUT_CROPPED", OUT_CROPPED, GROUP_OUTPUT,
		    croppedOutputInit);
  registerGroupType("OUT_COPY", OUT_COPY, GROUP_OUTPUT,
		    copyOutputsInit);
  registerGroupType("OUT_WINNER", OUT_WINNER, GROUP_OUTPUT,
		    winnerTakeAllOutputInit);
  registerGroupType("INTERACT_INTEGR", INTERACT_INTEGR, GROUP_OUTPUT,
		    interactiveActivationInit);

  registerGroupType("SUM_SQUARED", SUM_SQUARED, GROUP_COST,
		    squaredErrorInit);
  registerGroupType("CROSS_ENTROPY", CROSS_ENTROPY, GROUP_COST,
		    crossEntropyErrorInit);
  registerGroupType("DIVERGENCE", DIVERGENCE, GROUP_COST,
		    divergenceErrorInit);
  registerGroupType("COSINE", COSINE, GROUP_COST,
		    cosineErrorInit);
  registerGroupType("TARGET_COPY", TARGET_COPY, GROUP_COST,
		    copyTargetsInit);

  registerGroupType("LINEAR_COST", LINEAR_COST, GROUP_COST,
		    linearCostInit);
  registerGroupType("QUADRATIC_COST", QUADRATIC_COST, GROUP_COST,
		    quadraticCostInit);
  registerGroupType("CONV_QUAD_COST", CONV_QUAD_COST, GROUP_COST,
		    convexQuadraticCostInit);
  registerGroupType("LOGISTIC_COST", LOGISTIC_COST, GROUP_COST,
		    logisticCostInit);
  registerGroupType("COSINE_COST", COSINE_COST, GROUP_COST,
		    cosineCostInit);
  registerGroupType("DELTA_COST", DELTA_COST, GROUP_COST,
		    deltaCostInit);
}
