#ifndef ACT_H
#define ACT_H

extern void clearAllHistories(void);
extern void storeInputs(Group G, int tick);
extern void storeOutputs(Group G, int tick);
extern void storeTargets(Group G, int tick) ;
extern void storeOutputDerivs(Group G, int tick);
extern void cacheOutputs(Group G);
extern void resetOutputs(Group G);
extern void restoreOutputs(Group G, int tick);
extern void restoreInputs(Group G, int tick);
extern void cacheOutputDerivs(Group G);
extern void injectOutputDerivCache(Group G);
extern void resetOutputDerivCache(Group G);
extern void resetOutputDerivs(Group G);
extern void restoreOutputDerivs(Group G, int tick);
extern void resetForwardIntegrators(Group G);
extern void resetBackwardIntegrators(Group G);
extern void resetLinkDerivs(Group G);
extern void scaleLinkDerivsByDt(Group G);
extern void injectLinkDerivNoise(Group G);

extern flag groupCriteriaReached(flag training);

extern void computeInput(Group G, flag alwaysStore);
extern void computeInputBack(Group G);
extern void computeOutput(Group G, flag alwaysStore);
extern void computeOutputBack(Group G);
extern void computeCost(Group G, flag alwaysStore);
extern void computeCostBack(Group G, flag alwaysStore);

extern flag netForward(Event V);
extern flag netForwardBackward(Event V);
extern flag srbpttNetExampleBack(Example E);
extern flag continuousNetTickForward(Event V);
extern flag continuousNetExampleBack(Example E);
extern flag standardNetRunExample(Example E, flag (*tickProc)(Event), 
				  flag *correct);
extern flag standardNetTrainBatch(flag *allCorrect);
extern flag reportError(int examples, int ticks, int reached, flag print);
extern flag standardNetTestBatch(int numExamples, flag resetError,
                                 flag printResults);
extern flag standardNetRunExampleNum(int num, ExampleSet S);
extern flag initStandardNet(Network N);

extern flag boltzmannUpdate(Event V);
extern flag boltzmannNetTrainExample(Example E, flag (*tickProc)(Event),
				     flag *correct);
extern flag boltzmannNetTestExample(Example E, flag (*tickProc)(Event), 
				    flag *correct);

extern void registerNetAndGroupTypes(void);

/* If the output is within zr or tr of the target, the target is set to the 
   output. Otherwise, the target is adjusted tr towards the output */
#define ADJUSTED_TARGET(o,t,tr,zr) ((((o)-(t)) < (zr)) && ((o)-(t)) > (-zr))\
                              ? o : (((o)-(t)) > (tr)) ? ((t)+(tr)) :\
				(((o)-(t)) < (-tr)) ? ((t)-(tr)) : o;

#define SIMPLE_TANH(x)\
     ((real) tanh((double)(x)))
#define CALC_TANH(x)\
     ((x) >= 17 ? (1.0) :\
      ((x) <= -17 ? (-1.0) : SIMPLE_TANH(x)))
#define SIMPLE_TANH_DERIV(x)\
     ((real) 1.0 / SQUARE((real) cosh((double)(x))))
#define TANH_DERIV(x)\
     (((x) >= 17 || (x) <= -17) ? SMALL_VAL : SIMPLE_TANH_DERIV(x))

#define SIMPLE_CROSS_ENTROPY(y, d)\
     (((y) <= 0.0 || (y) >= 1.0) ? LARGE_VAL :\
     (LOG((d)/(y))*(d) +\
     LOG((1.0-(d))/(1.0-(y)))*(1.0-(d))))
#define CROSS_ENTROPY_ZERO_TARGET(y)\
     ((y) == 1.0 ? LARGE_VAL : -LOG(1.0-(y)))
#define CROSS_ENTROPY_ONE_TARGET(y)\
     ((y) == 0.0 ? LARGE_VAL : -LOG(y))
#define CROSS_ENTROPY_ERROR(y, d)\
     (((d) == 0.0) ? CROSS_ENTROPY_ZERO_TARGET(y) :\
       (((d) == 1.0) ? CROSS_ENTROPY_ONE_TARGET(y) :\
	 SIMPLE_CROSS_ENTROPY(y, d)))

#define SIMPLE_CED(y, d)\
     ((((real) (y)*(1.0-(y))) <= SMALL_VAL) ?\
     (((real) (y)-(d)) * LARGE_VAL) :\
       (((real) (y)-(d))/((real) (y)*(1.0-(y)))))
#define CED_ZERO_TARGET(y)\
     (((real) (1.0-(y)) <= SMALL_VAL) ? LARGE_VAL :\
     ((real) 1.0/(1.0-(y))))
#define CED_ONE_TARGET(y)\
     (((real) (y) <= SMALL_VAL) ? -LARGE_VAL :\
     ((real) -1.0/(y)))
#define CROSS_ENTROPY_DERIV(y, d)\
     (((real) (d) == 0.0) ? CED_ZERO_TARGET(y) :\
       (((real) (d) == 1.0) ? CED_ONE_TARGET(y) :\
	 SIMPLE_CED(y, d)))

#define DIVERGENCE_ERROR(y, d)\
     (((real) (d) == 0.0) ? 0.0 :\
      ((real) (y) <= 0.0) ? ((d) * LOG((d) * LARGE_VAL)) :\
      (LOG((d)/(y)) * (d)))

#define DIVERGENCE_DERIV(y, d)\
     (((real) (d) == 0.0) ? 0.0 :\
     ((real) (y) <= SMALL_VAL) ? ((real) -(d) * LARGE_VAL) :\
       ((real) -(d)/(y)))


#define FOR_EACH_LINK_FORW(U, proc) {\
  Link L, sL; real *O; Block B, sB;\
  for (B = U->block, sB = B + U->numBlocks, L = U->incoming; B < sB; B++)\
    for (O = B->output, sL = L + B->numUnits; L < sL; O++, L++)\
      {proc;}}

#define FOR_EACH_LINK_FAST_FORW(U, proc, unrollproc) {\
 Link L, sL; real *O; Block B, sB;\
 for (B = U->block, sB = B + U->numBlocks, L = U->incoming; B < sB; B++) {\
   for (O = B->output, sL = L + B->numUnits; L + 10 < sL;\
	O += 10, L += 10)   {unrollproc;}\
   for (; L < sL; O++, L++) {proc;}}}

#define FOR_EACH_LINK_BACK(U, proc) {\
  Link L, sL; real *O; Block B, sB; int nU;\
  for (B = U->block, sB = B + U->numBlocks, L = U->incoming; B < sB; B++)\
    for (O = B->output, nU = B->groupUnits, sL = L + B->numUnits; L < sL;\
         O++, L++)\
      {proc;}}

#define FOR_EACH_LINK_FAST_BACK(U, proc, unrollproc) {\
 Link L, sL; real *O, *D; Block B, sB; int nU;\
 for (B = U->block, sB = B + U->numBlocks, L = U->incoming; B < sB; B++) {\
   for (O = B->output, nU = B->groupUnits, D = O + nU, sL = L + B->numUnits;\
	L + 10 < sL; O += 10, L += 10, D += 10) {unrollproc;}\
   for (; L < sL; O++, L++)                     {proc;}}}

#define L_WGT     L->weight
#define L_WGT_(i) L[i].weight
#define L_DRV     L->deriv
#define L_DRV_(i) L[i].deriv
#define V_OUT     O[0]
#define V_OUT_(i) O[i]
#define V_DRV     O[nU]  /* output and outputDeriv arrays consecutive */
#define V_DRV_(i) D[i]


#define UPDATE(G)   {computeInput(G,  TRUE);\
                     computeOutput(G, TRUE);\
                     computeCost(G,   TRUE);}
#define BACKPROP(G) {computeOutputBack(G);\
                     computeInputBack(G);}

#endif /* ACT_H */
