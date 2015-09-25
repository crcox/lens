/* This defines the network and group type flags */
#ifndef TYPE_H
#define TYPE_H

#include "network.h"
#include "example.h"

/* TYPE CLASSES */
enum typeClass {NETWORK, GROUP, GROUP_INPUT, GROUP_OUTPUT, GROUP_COST, UNIT, 
		LINK, PROJECTION, EXAMPLE_MODE, ALGORITHM, TASK, UPDATE_SIGNAL,
		MAX_TYPE_CLASS};
extern char *typeClassName[];

/* SHARED TYPES */
#define ALL                ~0
#define LESIONED           ((mask) 1 << 30)
#define FROZEN             ((mask) 1 << 31)


/* NETWORK TYPES */
#define SRBPTT             ((mask) 1 << 0)
#define CONTINUOUS         ((mask) 1 << 1)
#define BOLTZMANN          ((mask) 1 << 2)

#define OPTIMIZED          ((mask) 1 << 20)


/* GROUP TYPES */
/* Fixed Types */
#define BIAS               ((mask) 1 << 0)
#define INPUT              ((mask) 1 << 1)
#define OUTPUT             ((mask) 1 << 2)
#define ELMAN              ((mask) 1 << 3)
#define FIXED_MASKS        (BIAS | INPUT | OUTPUT | ELMAN)

/* Criterion Functions */
#define STANDARD_CRIT      ((mask) 1 << 12)
#define MAX_CRIT           ((mask) 1 << 13)
#define CRITERION_MASKS    (STANDARD_CRIT | MAX_CRIT)

/* History Control Params */
#define USE_INPUT_HIST     ((mask) 1 << 20)
#define USE_OUTPUT_HIST    ((mask) 1 << 21)
#define USE_TARGET_HIST    ((mask) 1 << 22)
#define USE_OUT_DERIV_HIST ((mask) 1 << 23)
#define USE_HIST_MASKS     (USE_INPUT_HIST | USE_OUTPUT_HIST | \
			    USE_TARGET_HIST | USE_OUT_DERIV_HIST)

/* Other Stuff */
#define BIASED             ((mask) 1 << 24)
#define RESET_ON_EXAMPLE   ((mask) 1 << 25)
#define ADAPTIVE_GAIN      ((mask) 1 << 26)
#define DERIV_NOISE        ((mask) 1 << 27)
#define EXT_INPUT_NOISE    ((mask) 1 << 28)
#define WRITE_OUTPUTS      ((mask) 1 << 29)


/* GROUP_INPUT TYPES */
#define DOT_PRODUCT        ((mask) 1 << 0)
#define DISTANCE           ((mask) 1 << 1)
#define PRODUCT            ((mask) 1 << 2)
#define IN_BOLTZ           ((mask) 1 << 3)
#define SOFT_CLAMP         ((mask) 1 << 4)
#define IN_INTEGR          ((mask) 1 << 5)
#define IN_NORM            ((mask) 1 << 6)
#define IN_NOISE           ((mask) 1 << 7)
#define IN_DERIV_NOISE     ((mask) 1 << 8)
#define IN_COPY            ((mask) 1 << 9)
#define INCR_CLAMP         ((mask) 1 << 10)
#define BASIC_INPUT_TYPES  (DOT_PRODUCT | DISTANCE | PRODUCT | IN_BOLTZ | \
			    IN_COPY)
#define IN_INTEGR_TYPES (IN_INTEGR)


/* GROUP_OUTPUT TYPES */
#define LINEAR             ((mask) 1 << 0)
#define LOGISTIC           ((mask) 1 << 1)
#define TERNARY            ((mask) 1 << 2)
#define TANH               ((mask) 1 << 3)
#define EXPONENTIAL        ((mask) 1 << 4)
#define GAUSSIAN           ((mask) 1 << 5)
#define SOFT_MAX           ((mask) 1 << 6)
#define KOHONEN            ((mask) 1 << 7)
#define OUT_BOLTZ          ((mask) 1 << 8)
#define HARD_CLAMP         ((mask) 1 << 9)
#define BIAS_CLAMP         ((mask) 1 << 10)
#define ELMAN_CLAMP        ((mask) 1 << 11)
#define WEAK_CLAMP         ((mask) 1 << 12)
#define OUT_INTEGR         ((mask) 1 << 13)
#define OUT_NORM           ((mask) 1 << 14)
#define OUT_NOISE          ((mask) 1 << 15)
#define OUT_DERIV_NOISE    ((mask) 1 << 16)
#define OUT_CROPPED        ((mask) 1 << 17)
#define OUT_COPY           ((mask) 1 << 18)
#define INTERACT_INTEGR    ((mask) 1 << 19)
#define OUT_WINNER         ((mask) 1 << 20)
#define BASIC_OUTPUT_TYPES (LINEAR | LOGISTIC | TERNARY | TANH | GAUSSIAN | \
                            EXPONENTIAL | SOFT_MAX | KOHONEN | OUT_BOLTZ | \
                            OUT_COPY | INTERACT_INTEGR) 
#define CLAMPING_OUTPUT_TYPES (HARD_CLAMP | BIAS_CLAMP | ELMAN_CLAMP | \
			       WEAK_CLAMP)
#define OUT_INTEGR_TYPES (OUT_INTEGR | INTERACT_INTEGR)


/* GROUP_COST TYPES */
/* Error Functions */
#define SUM_SQUARED        ((mask) 1 << 0)
#define CROSS_ENTROPY      ((mask) 1 << 1)
#define DIVERGENCE         ((mask) 1 << 2)
#define COSINE             ((mask) 1 << 3)
#define ERROR_MASKS        (SUM_SQUARED | CROSS_ENTROPY | DIVERGENCE | COSINE)
/* Target Setting Functions */
#define TARGET_COPY        ((mask) 1 << 10)

/* Unit Output Cost Types */
#define LINEAR_COST        ((mask) 1 << 16)
#define QUADRATIC_COST     ((mask) 1 << 17)
#define CONV_QUAD_COST     ((mask) 1 << 18)
#define LOGISTIC_COST      ((mask) 1 << 19)
#define COSINE_COST        ((mask) 1 << 20)
#define DELTA_COST         ((mask) 1 << 21)
#define UNIT_COST_MASKS     (LINEAR_COST | QUADRATIC_COST | CONV_QUAD_COST | \
			     LOGISTIC_COST | COSINE_COST | DELTA_COST)

#define GROUP_HAS_TYPE(G, class, type) \
     ((class == GROUP && G->type & type) ||\
      (class == GROUP_INPUT && G->inputType & type) ||\
      (class == GROUP_OUTPUT && G->outputType & type) ||\
      (class == GROUP_COST && G->costType & type))


/* LINK TYPES */
#define ALL_LINKS          ((mask) 0)

/* Other link types are defined with registerLinkType() */

/* The following are for user-defined link types */
#define LINK_TYPE_BITS 8
#define LINK_NUM_TYPES ((1 << LINK_TYPE_BITS) - 1)

/* The only link state flag */
#define SET_LINK_TYPE(t,B)   ((B)->type = (((B)->type & ~LINK_NUM_TYPES)|(t)))
#define GET_LINK_TYPE(B)     ((B)->type & LINK_NUM_TYPES)
#define LINK_TYPE_MATCH(t,B) (((t) == ALL_LINKS) || ((t) == GET_LINK_TYPE(B)))


/* PROJECTION TYPES */
#define ONE_TO_ONE         ((mask) 1 << 0)
#define FULL               ((mask) 1 << 1)
#define RANDOM             ((mask) 1 << 2)
#define FIXED_IN           ((mask) 1 << 3)
#define FIXED_OUT          ((mask) 1 << 4)
#define FAIR               ((mask) 1 << 5)
#define FAN                ((mask) 1 << 6)


/* EXAMPLE SELECTION TYPES */
#define ORDERED            ((mask) 1 << 0)
#define PERMUTED           ((mask) 1 << 1)
#define RANDOMIZED         ((mask) 1 << 2)
#define PROBABILISTIC      ((mask) 1 << 3)
#define PIPE               ((mask) 1 << 4)
#define CUSTOM             ((mask) 1 << 5)


/* ALGORITHM TYPES */
#define STEEPEST           ((mask) 1 << 0)
#define MOMENTUM           ((mask) 1 << 1)
#define DOUGS_MOMENTUM     ((mask) 1 << 2)
#define DELTA_BAR_DELTA    ((mask) 1 << 3)


/* TASK TYPES */
#define WAITING            ((mask) 1 << 0)
#define TRAINING           ((mask) 1 << 1)
#define TESTING            ((mask) 1 << 2)
#define PARA_TRAINING      ((mask) 1 << 3)
#define LOADING_EXAMPLES   ((mask) 1 << 4)
#define SAVING_EXAMPLES    ((mask) 1 << 5)
#define BUILDING_LINKS     ((mask) 1 << 6)
#define NETWORK_TASKS      (TRAINING | TESTING | PARA_TRAINING)


/* DISPLAY-RELATED TYPES */
enum exampleSets {NO_SET, TRAIN_SET, TEST_SET};
#define ON_NEVER           ((mask) 0)
#define ON_TICK            ((mask) 1 << 0)
#define ON_EVENT           ((mask) 1 << 1)
#define ON_EXAMPLE         ((mask) 1 << 2)
#define ON_UPDATE          ((mask) 1 << 3)
#define ON_REPORT          ((mask) 1 << 4)
#define ON_TRAINING        ((mask) 1 << 5)
#define ON_CUSTOM          ((mask) 1 << 6)


/* PALETTE TYPES */
enum palettes {BLUE_BLACK_RED, BLUE_RED_YELLOW, BLACK_GRAY_WHITE, HINTON,
               NUM_PALETTES};


/* OTHER STUFF */
enum typeModes {TOGGLE_TYPE, ADD_TYPE, REMOVE_TYPE};


extern flag lookupTypeMask(char *typeName, unsigned int typeClass, 
			   mask *result);
extern flag lookupType(char *typeName, unsigned int *typeClass, mask *type);
extern flag lookupGroupType(char *typeName, unsigned int *typeClass, 
			    mask *type);
extern const char *lookupTypeName(mask typeMask, unsigned int typeClass);
extern flag registerType(char *typeName, mask typeMask, 
			 unsigned int typeClass);
extern void printTypes(char *label, unsigned int class, mask filter);

extern void registerNetType(char *name, mask type, 
			    flag (*initNetType)(Network N));
extern flag initNetTypes(Network N);

extern void registerGroupType(char *name, mask type, unsigned int class, 
			      void (*initGroupType)(Group G, GroupProc P));
extern flag initGroupTypes(Group G);
extern void printGroupType(void *G);
extern void printGroupTypes(void);

extern void registerProjectionType(char *name, mask type,
			    flag (*connectGroups)(Group, Group, mask, 
						  real, real, real));
extern flag (*projectionProc(mask type))(Group, Group, mask, real, real, real);

extern void registerExampleModeType(char *name, mask type,
			     flag (*loadNextExample)(ExampleSet));
extern flag (*nextExample(mask type))(ExampleSet);

extern void initTypes(void);
extern void estimateOutputBounds(void *G);
extern void cleanupGroupType(void *G);

#endif /* TYPE_H */
