#ifndef GRAPH_H
#define GRAPH_H

typedef struct graph *Graph;
typedef struct trace *Trace;
typedef struct point *Point;

struct graph {
  int        num;
  mask       updateOn;
  int        updateEvery; /* If 2 it updates every other event. */
  flag       clearOnReset;
  flag       storeOnReset;
  flag       hidden;

  int        x;
  int        maxX;
  int        updatesSinceRedraw;

  int        width;
  int        height;
  int        cols;

  real       max;
  real       min;
  flag       fixMax;
  flag       fixMin;
  real       maxVal;
  real       minVal;

  real       scaleX;
  real       scaleY;

  flag       needsRedraw;
  flag       needsPropRefresh;
  flag       tracesChanged;
  flag       propertiesUp;

  int        numTraces;
  Trace     *trace;
};

typedef struct point {
  int        x;
  real       y;
} point;

struct trace {
  int        num;
  Graph      graph;
  char      *label;
  char      *object;
  char      *lastObject;
  real      *value;
  Tcl_Obj   *proc;

  int        numVals;
  int        maxVals;
  point     *val;

  char      *color;
  flag       active;
  flag       transient;
  flag       visible;

  int        export;
};


extern void colorName(char *color, real H, real S, real V);
extern Graph lookupGraph(int g);
extern Trace lookupTrace(Graph G, int t);
extern Trace createTrace(Graph G, char *object);
extern Graph createGraph(void);

extern flag showGraph(Graph G);
extern flag hideGraph(Graph G);

extern flag deleteTrace(Trace T);
extern flag deleteGraph(Graph G);

extern flag clearTrace(Trace T);
extern flag clearGraph(Graph G);

extern flag storeTrace(Trace T);
extern flag storeGraph(Graph G);

extern flag updateGraph(Graph G);
extern flag updateGraphs(mask update);
extern void resetGraphs(void);

extern void reacquireGraphObjects(void);

extern flag drawTraceProps(Graph G);
extern flag drawLater(Graph G);
extern flag refreshPropsLater(Graph G);

extern flag exportGraphData(Graph G, Tcl_Obj *fileNameObj,
			    flag labels, flag gnuplot);

#endif /* GRAPH_H */
