#ifndef DEFAULTS_H
#define DEFAULTS_H

/* NETWORK FIELDS */
#define DEF_N_numTimeIntervals    1
#define DEF_N_numTicksPerInterval 1
#define DEF_N_backpropTicks       1

#define DEF_N_numUpdates          100
#define DEF_N_batchSize           0    /* This means full batch mode */
#define DEF_N_reportInterval      10
#define DEF_N_criterion           0.0
#define DEF_N_trainGroupCrit      0.0
#define DEF_N_testGroupCrit       0.5
#define DEF_N_groupCritRequired   FALSE
#define DEF_N_minCritBatches      1
#define DEF_N_pseudoExampleFreq   FALSE

#define DEF_N_algorithm           DOUGS_MOMENTUM
#define DEF_N_learningRate        0.1
#ifdef ADVANCED
# define DEF_N_rateIncrement      0.1
# define DEF_N_rateDecrement      0.9
#endif /* ADVANCED */
#define DEF_N_momentum            0.9
#define DEF_N_adaptiveGainRate    0.001
#define DEF_N_weightDecay         0.0
#define DEF_N_gainDecay           0.0
#define DEF_N_outputCostStrength  0.01
#define DEF_N_outputCostPeak      0.5
#define DEF_N_targetRadius        0.0
#define DEF_N_zeroErrorRadius     0.0

#define DEF_N_gain                1.0
#define DEF_N_ternaryShift        5.0
#define DEF_N_clampStrength       0.5
#define DEF_N_initOutput          0.5
#define DEF_N_initInput           0.0
#define DEF_N_initGain            1.0
#define DEF_N_finalGain           1.0
#define DEF_N_annealTime          1.0

#define DEF_N_randMean            0.0
#define DEF_N_randRange           1.0
#define DEF_N_noiseRange          0.1

#define DEF_N_autoPlot            TRUE
#define DEF_N_plotCols            10
#define DEF_N_unitCellSize        14
#define DEF_N_unitCellSpacing     2
#define DEF_N_unitDisplayValue    UV_OUT_TARG
#define DEF_N_unitDisplaySet      TRAIN_SET
#define DEF_N_unitUpdates         ON_REPORT
#define DEF_N_unitTemp            1.0
#define DEF_N_unitPalette         BLUE_BLACK_RED
#define DEF_N_unitExampleProc      0
#define DEF_N_boltzUnitExampleProc 1

#define DEF_N_linkCellSize        8
#define DEF_N_linkCellSpacing     0
#define DEF_N_linkDisplayValue    UV_LINK_WEIGHTS
#define DEF_N_linkUpdates         ON_REPORT
#define DEF_N_linkTemp            1.0
#define DEF_N_linkPalette         BLUE_BLACK_RED

#define DEF_N_writeExample        standardWriteExample

/* GROUP FIELDS */
#define DEF_G_standardReset       TRUE  /* RESET_ON_EXAMPLE for SRNs */
#define DEF_G_continuousReset     TRUE  /* for continuous networks */
#define DEF_G_trainGroupCrit      NaN
#define DEF_G_testGroupCrit       NaN

#define DEF_G_learningRate        NaN
#define DEF_G_momentum            NaN
#define DEF_G_weightDecay         NaN
#define DEF_G_gainDecay           NaN
#define DEF_G_outputCostScale     1.0
#define DEF_G_outputCostPeak      NaN
#define DEF_G_targetRadius        NaN
#define DEF_G_zeroErrorRadius     NaN
#define DEF_G_errorScale          1.0

#define DEF_G_dtScale             1.0
#define DEF_G_gain                NaN
#define DEF_G_ternaryShift        NaN
#define DEF_G_clampStrength       NaN
#define DEF_G_initInput           0.0

#define DEF_G_randMean            NaN
#define DEF_G_randRange           NaN
#define DEF_G_noiseRange          NaN
#define DEF_G_noiseProc           addGaussianNoise

#define DEF_G_showIncoming        TRUE
#define DEF_G_showOutgoing        TRUE
#define DEF_G_numColumns          0
#define DEF_G_neighborhood        4
#define DEF_G_periodicBoundary    FALSE


/* UNIT FIELDS */
#define DEF_U_target              NaN
#define DEF_U_externalInput       NaN
#define DEF_U_dtScale             1.0

/* BLOCK FIELDS */
#define DEF_B_learningRate        NaN
#define DEF_B_momentum            NaN
#define DEF_B_weightDecay         NaN
#define DEF_B_randMean            NaN
#define DEF_B_randRange           NaN
#define DEF_B_min                 NaN
#define DEF_B_max                 NaN

/* LINK FIELDS */
#define DEF_L_deriv               0.0
#define DEF_L_lastWeightDelta     0.0
#define DEF_L_lastValue           1.0


/* EXAMPLE SET FIELDS */
#define DEF_S_mode                ORDERED
#define DEF_S_pipeLoop            TRUE
#define DEF_S_maxTime             1.0
#define DEF_S_minTime             0.0
#define DEF_S_graceTime           0.0
#define DEF_S_defaultInput        0.0
#define DEF_S_activeInput         1.0
#define DEF_S_defaultTarget       0.0
#define DEF_S_activeTarget        1.0
#define DEF_S_loadEvent           standardLoadEvent
#define DEF_S_loadExample         standardLoadExample
#define DEF_S_loadNextExample     standardLoadNextExample


/* EXAMPLE FIELDS */
#define DEF_E_frequency           1.0


/* EVENT FIELDS */
#define DEF_V_maxTime             NaN
#define DEF_V_minTime             NaN
#define DEF_V_graceTime           NaN

/* GRAPH PARAMETERS */
#define DEF_GR_columns            100
#define DEF_GR_values             100
#define DEF_GR_lineWidth          1.0

/* OTHER STUFF */
#define DEF_GUI          TRUE   /* Whether the Main Window opens by default. */
#define DEF_BATCH        FALSE  /* Whether its in Batch mode by default. */
#define DEF_CONSOLE      FALSE  /* Whether the Console opens by default. */
#define DEF_VERBOSITY    1      /* The initial verbosity level. */

/* Buffer Limits */
#define BUFFER_SIZE      4096   /* Size of the text Buffer. */
#define PAR_BUF_SIZE     8192   /* Buffer size for parallel weight transfers.
				   Must be at least 3 */
#define MAX_TYPES        32     /* Max types modifying a single group */

/* Fast Sigmoid */
#define SIGMOID_RANGE    16     /* Sigmoid defined for up to +- this value */
#define SIGMOID_SUB      1024   /* Granularity of the table */

/* Magic Cookies */
#define BINARY_EXAMPLE_COOKIE 0xaaaaaaaa /* 10101010... */
#define OLD_BINARY_WEIGHT_COOKIE  0x55555555 /* 01010101... */
#define BINARY_WEIGHT_COOKIE  0x55555556

#define BIAS_NAME        "bias"   /* The name of the bias group */
#define NUM_COLORS       101      /* Colors in blue-red colormap */
#define UPDATE_INTERVAL  100      /* Interval between screen updates, 
				     in milliseconds */
#define AUTO_NICE_VALUE  10       /* Long-running process renice value */
#define AUTO_NICE_DELAY  10       /* Minutes before auto-renicing */

#endif
