#ifndef TRAIN_H
#define TRAIN_H

typedef struct algorithm *Algorithm;
struct algorithm {
  mask       code;
  char      *shortName;
  char      *longName;
  void     (*updateWeights)(flag doStats);
  Algorithm  next;
};

extern Algorithm AlgorithmTable;

extern void registerAlgorithm(mask code, char *shortName, char *longName,
			      void (*updateWeights)(flag));
extern void registerAlgorithms(void);
extern Algorithm getAlgorithm(mask code);

extern void printReportHeader(void);
extern flag printReport(int lastReport, int update, unsigned long startTime);
extern flag standardNetTrain(void);
extern void updateAdaptiveGain(Group G);

extern void steepestUpdateWeights(flag doStats);
extern void momentumUpdateWeights(flag doStats);
extern void dougsMomentumUpdateWeights(flag doStats);
extern void deltabardeltaUpdateWeights(flag doStats);
extern void quickpropUpdateWeights(flag doStats);

#define UPDATE_WEIGHTS(proc) {\
  Link L, sL; Link2 M;\
  real learningRate, momentum, lastWeightDelta, deriv, weightDecay,\
    gradLin = 0.0, lastDeltaLen = 0.0, derivLen = 0.0, weightCost = 0.0, w;\
  Net->gradientLinearity = 1.0;\
  Net->weightCost = 0.0;\
  if (Net->type & FROZEN) return;\
  FOR_EACH_GROUP({if (G->type & FROZEN) continue;\
    if (G->type & ADAPTIVE_GAIN) updateAdaptiveGain(G);\
    FOR_EACH_UNIT(G, {if (U->type & FROZEN) continue;\
      L = U->incoming; M = U->incoming2;\
      FOR_EACH_BLOCK(U, {\
	if (B->type & FROZEN) {\
	  L += B->numUnits; M += B->numUnits;\
	} else {\
  	  learningRate =\
	    chooseValue3(B->learningRate, G->learningRate, Net->learningRate);\
	  momentum =\
	    chooseValue3(B->momentum, G->momentum, Net->momentum);\
  	  weightDecay =\
	    chooseValue3(B->weightDecay, G->weightDecay, Net->weightDecay);\
	  if (!doStats)\
	    for (sL = L + B->numUnits; L < sL; L++, M++) {proc}\
	  else\
            for (sL = L + B->numUnits; L < sL; L++, M++) {\
              lastWeightDelta = M->lastWeightDelta; \
              deriv = L->deriv; \
              gradLin -= lastWeightDelta * deriv; \
              lastDeltaLen += SQUARE(lastWeightDelta); \
              derivLen += SQUARE(deriv);\
              {proc}\
              weightCost += SQUARE(L->weight);\
            }\
        }\
      });\
    });\
  });\
  if (doStats) {\
    Net->gradientLinearity = (lastDeltaLen * derivLen == 0.0) ? NaN :\
      gradLin / SQRT(lastDeltaLen * derivLen);\
    Net->weightCost = weightCost / 2;\
  }}

#endif /* TRAIN_H */
