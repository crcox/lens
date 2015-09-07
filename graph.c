#include <string.h>
#include "system.h"
#include "util.h"
#include "type.h"
#include "network.h"
#include "object.h"
#include "graph.h"
#include "parallel.h"

#define Y_MARGIN 20
#define X_MARGIN 40

/*****************************************************************************/

/* HSV should be in the range [0, 1] */
void colorName(char *color, real H, real S, real V) {
  int R = 0, G = 0, B = 0, p, q, t, v, h;
  real f;
  v = (int) (V * 255);
  if (S == 0) {
    R = G = B = v;
  } else {
    H *= 6.0;
    if (H >= 6.0) H = 0.0;
    h = (int) H;
    f = H - h;
    p = (int) (V * (1 - S) * 255);
    q = (int) (V * (1 - S * f) * 255);
    t = (int) (V * (1 - S * (1 - f)) * 255);
    switch (h) {
    case 0: R = v; G = t; B = p; break;
    case 1: R = q; G = v; B = p; break;
    case 2: R = p; G = v; B = t; break;
    case 3: R = p; G = q; B = v; break;
    case 4: R = t; G = p; B = v; break;
    case 5: R = v; G = p; B = q; break;
    }
  }
  sprintf(color, "#%02x%02x%02x", R, G, B);
}

char *nextColor(void) {
  static int last = 0;
  char *color = (char *) safeMalloc(8, "color");
  colorName(color, (real) last / 16, 1.0, 0.75);
  last = (last + 3) % 16;
  return color;
}

/*****************************************************************************/

void drawTrace(Trace T, int from, int to);
flag drawGraph(Graph G);

Graph lookupGraph(int g) {
  if (g < Root->numGraphs && Root->graph[g])
    return Root->graph[g];
  return NULL;
}

Trace lookupTrace(Graph G, int t) {
  if (t < G->numTraces && G->trace[t])
    return G->trace[t];
  return NULL;
}

Trace createTrace(Graph G, char *object) {
  int t;
  Trace T = (Trace) safeCalloc(1, sizeof *T, "createTrace:T");
  for (t = 0; t < G->numTraces && G->trace[t]; t++);
  if (t >= G->numTraces) {
    int n = imax(t + 2, 2 * G->numTraces);
    G->trace = safeRealloc(G->trace, n * sizeof(Trace), "G->trace");
    memset(G->trace + G->numTraces, 0, (n - G->numTraces) * sizeof(Trace));
    G->numTraces = n;
  }
  G->trace[t] = T;
  T->num = t;

  T->graph = G;
  if (T->num == 0) T->color = copyString("black");
  else T->color = nextColor();
  T->maxVals = DEF_GR_values;
  T->active = TRUE;
  T->transient = FALSE;
  T->visible = TRUE;
  T->val = (point *) safeMalloc(T->maxVals * sizeof(point), "T->val");
  T->object = copyString(object);
  G->tracesChanged = TRUE;
  refreshPropsLater(G);
  return T;
}

Graph createGraph(void) {
  int g;
  Graph G = (Graph) safeCalloc(1, sizeof *G, "createGraph:G");

  for (g = 0; g < Root->numGraphs && Root->graph[g]; g++);
  if (g >= Root->numGraphs) {
    int n = imax(g + 2, 2 * Root->numGraphs);
    Root->graph = safeRealloc(Root->graph, n * sizeof(Graph), "Root->graph");
    memset(Root->graph + Root->numGraphs, 0,
           (n - Root->numGraphs) * sizeof(Graph));
    Root->numGraphs = n;
  }
  Root->graph[g] = G;
  G->num      = g;

  G->updateOn = ON_REPORT;
  G->updateEvery = 1;

  G->width    = 0;
  G->height   = 0;
  G->cols     = DEF_GR_columns;

  G->max = G->min = 0.0;
  G->fixMax   = FALSE;
  G->fixMin   = TRUE;
  G->maxVal   = -LARGE_VAL;
  G->minVal   = LARGE_VAL;
  G->scaleY   = 0;

  G->clearOnReset = FALSE;
  G->storeOnReset = TRUE;

  G->maxX     = 0;
  G->hidden   = TRUE;
  if (!Batch) showGraph(G);
  return G;
}

flag showGraph(Graph G) {
  char var[32];
  if (!G->hidden || Batch) return TCL_OK;
  G->hidden = FALSE;
  G->needsRedraw = FALSE;
  G->needsPropRefresh = FALSE;
  eval(".drawGraph %d", G->num);
  sprintf(var, ".graphPropUp_%d", G->num);
  Tcl_LinkVar(Interp, var, (char *) &(G->propertiesUp), TCL_LINK_INT);
  drawLater(G);
  return TCL_OK;
}

flag hideGraph(Graph G) {
  if (G->hidden) return TCL_OK;
  G->hidden = TRUE;
  G->needsRedraw = FALSE;
  G->needsPropRefresh = FALSE;
  eval("destroy .graph%d", G->num);
  eval("destroy .grProp%d", G->num);
  eval("destroy .graphPropUp_%d", G->num);
  return TCL_OK;
}

flag deleteTrace(Trace T) {
  Graph G = T->graph;
  G->trace[T->num] = NULL;
  FREE(T->label);
  FREE(T->object);
  if (T->proc) Tcl_DecrRefCount(T->proc);
  FREE(T->val);
  FREE(T->color);
  FREE(T);
  G->tracesChanged = TRUE;
  drawLater(G);
  refreshPropsLater(G);
  return TCL_OK;
}

flag deleteGraph(Graph G) {
  hideGraph(G);
  Root->graph[G->num] = NULL;
  /* FREE(G->title); */
  FOR_EACH_TRACE(G, deleteTrace(T));
  FREE(G->trace);
  FREE(G);
  return TCL_OK;
}

flag clearTrace(Trace T) {
  FREE(T->val);
  T->numVals = 0;
  T->maxVals = DEF_GR_values;
  T->val = (point *) safeMalloc(T->maxVals * sizeof(point), "T->val");
  drawLater(T->graph);
  refreshPropsLater(T->graph);
  return TCL_OK;
}

flag clearGraph(Graph G) {
  FOR_EACH_TRACE(G, {
    if (T->transient) deleteTrace(T);
    else if (T->active) clearTrace(T);
  });
  G->x = G->maxX = 0;
  return TCL_OK;
}

flag storeTrace(Trace T) {
  Graph G = T->graph;
  Trace S;
  if (T->numVals == 0) return TCL_OK;
  S = createTrace(G, T->object);
  S->object    = copyString(T->object);
  S->numVals   = T->numVals;
  S->maxVals   = T->numVals;
  S->val       = (point *) safeMalloc(S->numVals * sizeof(point), "S->val");
  memcpy(S->val, T->val, S->numVals * sizeof(point));
  S->color     = nextColor();
  S->active    = FALSE;
  S->transient = TRUE;
  S->visible   = TRUE;
  clearTrace(T);
  return TCL_OK;
}

flag storeGraph(Graph G) {
  FOR_EACH_TRACE(G, if (T->active) storeTrace(T););
  G->x = 0;
  if (!G->hidden) eval(".graph%d.g xview moveto 0", G->num);
  return TCL_OK;
}

/* Called when network is reset. */
void resetGraphs(void) {
  FOR_EACH_GRAPH({
    if (G->clearOnReset) clearGraph(G);
    else if (G->storeOnReset) storeGraph(G);
  });
}

static void updateTrace(Trace T, int x) {
  int i;
  real v = 0.0;
  Graph G = T->graph;
  if (!T->object) return;
  if (T->object != T->lastObject) T->value = NULL;
  if (!T->proc) {
    if (!T->value) {
      void *object;
      ObjInfo O;
      int type, rows, cols;
      flag writable;
      object = getObject(T->object, &O,&type,&rows,&cols, &writable);
      if (!object || O != RealInfo) {
	T->proc = Tcl_NewStringObj(T->object, strlen(T->object));
	Tcl_IncrRefCount(T->proc);
      }
      else T->value = (real *) object;
    }
    if (T->value) v = *T->value;
  }
  if (T->proc) {
    double dv;
    if (Tcl_EvalObjEx(Interp, T->proc, TCL_EVAL_GLOBAL) ||
	Tcl_GetDoubleFromObj(Interp, Tcl_GetObjResult(Interp), &dv)) {
      error(Tcl_GetStringResult(Interp));
      Tcl_DecrRefCount(T->proc);
      T->proc = NULL;
      T->active = FALSE;
      refreshPropsLater(G);
      return;
    }
    v = (real) dv;
  }
  T->lastObject = T->object;

  /* Remove everything after x. */
  for (i = T->numVals - 1; i >= 0 && T->val[i].x >= x; i--);
  if (T->numVals != i + 1) {
    T->numVals = i + 1;
    drawLater(G);
  }

  if (T->numVals == T->maxVals) {
    T->maxVals *= 2;
    T->val = (point *) safeRealloc(T->val, T->maxVals * sizeof(point),
				   "T->val");
  }
  T->val[T->numVals].x = x;
  T->val[T->numVals].y = v;
  if (x > G->maxX) G->maxX = x;
  T->numVals++;
  if ((!G->fixMax && v > G->maxVal) || (!G->fixMin && v < G->minVal))
    drawLater(G);
  else drawTrace(T, T->numVals - 2, T->numVals - 1);
}

void fixGraphScrolling(Graph G) {
  int g = G->num, xMax;
  Tcl_Obj *obj;
  double visA, visB;
  real v, a, b;
  if (G->hidden) return;
  if (G->maxX == 0) {
    v = a = b = 0;
  } else {
    v = (real) G->x / G->maxX;
    a = (real) (G->x - 2 * G->updateEvery) / G->maxX;
    b = (real) (G->x + G->updateEvery) / G->maxX;
  }
  eval("lindex [.graph%d.g xview] 0", g);
  obj = Tcl_GetObjResult(Interp);
  Tcl_GetDoubleFromObj(Interp, obj, &visA);
  eval("lindex [.graph%d.g xview] 1", g);
  obj = Tcl_GetObjResult(Interp);
  Tcl_GetDoubleFromObj(Interp, obj, &visB);

  xMax = (int) (G->maxX * G->scaleX + 2);
  if (xMax < G->width - 2) xMax = G->width;
  eval(".graph%d.g configure -scrollregion {0 0 %d %d}", g, xMax, G->height);
  if (visB >= a && visB <= b)
    eval(".graph%d.g xview moveto %g", g, v - (visB - visA));
}

flag updateGraph(Graph G) {
  int t;
  Trace T;
  if ((G->x % G->updateEvery) == 0)
    for (t = G->numTraces - 1; t >= 0; t--)
      if ((T = G->trace[t]) && T->active) updateTrace(T, G->x);
  G->x++;
  fixGraphScrolling(G);
  if (++G->updatesSinceRedraw >= 100) drawLater(G);
  return TCL_OK;
}

flag updateGraphs(mask update) {
  FOR_EACH_GRAPH(if (G->updateOn & update) updateGraph(G););
  return TCL_OK;
}

/* Causes values to be looked up next time they are accessed. */
void reacquireGraphObjects(void) {
  FOR_EACH_GRAPH({
    FOR_EACH_TRACE(G, T->value = NULL;);
  });
}

flag drawNow(ClientData data) {
  Graph G;
  int g = *(int *) data;
  if (!(G = lookupGraph(g))) return TCL_OK;
  return drawGraph(G);
}

flag drawLater(Graph G) {
  if (!G->needsRedraw && !G->hidden) {
    G->needsRedraw = TRUE;
    Tcl_DoWhenIdle((Tcl_IdleProc *) drawNow, (ClientData) &G->num);
  }
  return TCL_OK;
}

void drawTrace(Trace T, int from, int to) {
  int i;
  Graph G = T->graph;
  static String cmd = NULL;
  char buf[40];
  if (G->hidden) return;
  if (from < 0) from = 0;
  if (!T->visible || from >= to) return;
  if (!cmd) cmd = newString(60 + (to - from) * 10);
  else stringSize(cmd, 60 + (to - from) * 10);
  clearString(cmd);
  sprintf(buf, ".graph%d.g create line", G->num);
  stringAppend(cmd, buf);
  for (i = from; i <= to; i++) {
    int y = G->height - Y_MARGIN - (int) ((T->val[i].y - G->min) * G->scaleY);
    sprintf(buf, " %d %d", (int) (T->val[i].x * G->scaleX) + 1, y);
    stringAppend(cmd, buf);
  }
  sprintf(buf, " -fill %s -width %f", T->color, DEF_GR_lineWidth);
  stringAppend(cmd, buf);
  Tcl_EvalEx(Interp, cmd->s, -1, TCL_EVAL_GLOBAL);
}

real nearestInterval(real v) {
  int base = (int) floor(log10((double) v));
  real scale = POW(10,base);
  v /= scale;
  if (v >= 5) v = 5;
  else if (v >= 2) v = 2;
  else v = 1;
  return v * scale;
}

flag drawGraph(Graph G) {
  int t, i, g = G->num, xint;
  Trace T;
  real yint;
  if (!G->needsRedraw || G->hidden) return TCL_OK;
  G->needsRedraw = FALSE;
  G->updatesSinceRedraw = 0;
  if (!G->fixMax || !G->fixMin) {
    G->maxVal = -LARGE_VAL;
    G->minVal =  LARGE_VAL;
    FOR_EACH_TRACE(G, {
      if (T->visible) {
        for (i = 0; i < T->numVals; i++) {
          if (T->val[i].y > G->maxVal) G->maxVal = T->val[i].y;
          if (T->val[i].y < G->minVal) G->minVal = T->val[i].y;
        }
      }
    });
    if (!G->fixMax) G->max = G->maxVal;
    if (!G->fixMin) G->min = G->minVal;
    if (G->max < G->min) G->max = G->min = 0;
  }
  G->scaleX = (G->cols > 0) ? (real) (G->width - 2) / G->cols : 0.0;
  G->scaleY = (G->max > G->min) ? (real) (G->height - Y_MARGIN) /
    (G->max - G->min) : 0.0;

  eval(".grProp%d.t.e2 delete 0 end; .grProp%d.t.e2 insert 0 %d",
       g, g, G->cols);
  eval(".grProp%d.t.e3 delete 0 end; .grProp%d.t.e3 insert 0 %g",
       g, g, G->max);
  eval(".grProp%d.t.e4 delete 0 end; .grProp%d.t.e4 insert 0 %g",
       g, g, G->min);
  if (eval(".graph%d.g delete all; .graph%d.y delete all", g, g))
    return TCL_ERROR;
  /* draw the axes */
  eval(".graph%d.g create rect 0 0 10000 %d -fill white -outline grey50",
       g, G->height - Y_MARGIN);
  /*  eval(".graph%d.g create line 0 %d 10000 %d -fg gray50",
      g, G->height - Y_MARGIN, G->height - Y_MARGIN);*/
  xint = (int) nearestInterval((real) G->cols / 3.0);
  for (i = 1; i * xint < G->maxX * 2 || i < 10; i++) {
    int x = (int) (((real) i - 0.5) * xint * G->scaleX);
    eval(".graph%d.g create line %d %d %d %d -fill grey50", g,
	 x, (int) (G->height - Y_MARGIN),
	 x, (int) (G->height - Y_MARGIN + 5));
    x = (int) (i * xint * G->scaleX);
    eval(".graph%d.g create line %d %d %d %d -fill grey50", g,
	 x, (int) (G->height - Y_MARGIN),
	 x, (int) (G->height - Y_MARGIN + 5));
    eval(".graph%d.g create text %d %d -text %d -anchor n", g,
	 x, (int) (G->height - Y_MARGIN + 6), (int) (xint * i));
  }
  yint = nearestInterval((G->max - G->min) / 2.5);
  i = (int) (ceil(G->min / yint));

  if (yint > 0) {
    while (yint * ((real) i - 0.5) < G->max) {
      int y;
      if (((real) i - 0.5) * yint >= G->min) {
        y = G->height - Y_MARGIN - (int) ((((real) i - 0.5) * yint - G->min)
					* G->scaleY);
        eval(".graph%d.y create line %d %d %d %d -fill grey50", g,
             X_MARGIN, y, X_MARGIN - 5, y);
      }
      y = G->height - Y_MARGIN - (int) ((i * yint - G->min) * G->scaleY);
      eval(".graph%d.y create line %d %d %d %d -fill grey50", g,
           X_MARGIN, y, X_MARGIN - 5, y);
      if (y > 6)
        eval(".graph%d.y create text %d %d -text %g -anchor e", g,
             X_MARGIN - 6, y, yint * i);
      i++;
    }
  }

  /* Traces drawn in reverse order because stored ones are at the end. */
  for (t = G->numTraces - 1; t >= 0; t--)
    if ((T = G->trace[t]))
      drawTrace(T, 0, T->numVals - 1);

  /* Deal with scrolling. */
  fixGraphScrolling(G);
  return TCL_OK;
}

flag drawTraceProps(Graph G) {
  if (G->hidden) return TCL_OK;
  if (eval(".deleteTraceProps %d", G->num)) return TCL_ERROR;
  FOR_EACH_TRACE(G, {
    if (eval(".drawTraceProps %d %d", G->num, t)) return TCL_ERROR;
  });
  G->tracesChanged = FALSE;
  return TCL_OK;
}

flag refreshTraceProps(Graph G) {
  if (G->hidden) return TCL_OK;
  FOR_EACH_TRACE(G, {
    if (eval(".refreshTraceProps %d %d", G->num, t)) return TCL_ERROR;
  });
  return TCL_OK;
}

flag refreshProps(Graph G) {
  if (G->hidden) return TCL_OK;
  G->needsPropRefresh = FALSE;
  eval(".refreshGraphProps %d", G->num);
  if (G->tracesChanged) return drawTraceProps(G);
  else return refreshTraceProps(G);
}

flag refreshPropsNow(ClientData data) {
  Graph G;
  int g = *(int *) data;
  if (!(G = lookupGraph(g))) return TCL_OK;
  if (!G->needsPropRefresh || !G->propertiesUp) return TCL_OK;
  return refreshProps(G);
}

flag refreshPropsLater(Graph G) {
  if (G->hidden) return TCL_OK;
  if (G->propertiesUp && !G->needsPropRefresh) {
    G->needsPropRefresh = TRUE;
    Tcl_DoWhenIdle((Tcl_IdleProc *) refreshPropsNow, (ClientData) &G->num);
  }
  return TCL_OK;
}

flag exportGraphData(Graph G, Tcl_Obj *fileNameObj, flag labels, flag gnuplot) {
  Tcl_Channel file;
  int i, minX, first = TRUE;
  const char *fileName = Tcl_GetStringFromObj(fileNameObj, NULL);
  if (!(file = writeChannel(fileNameObj, FALSE)))
    return warning("Couldn't write file \"%s\"\n", fileName);

  if (gnuplot) {
    FOR_EACH_TRACE(G, {
      if (T->visible) {
        if (!first) cprintf(file, "\n\n");
        first = FALSE;
        if (labels)
          cprintf(file, "# %s\n", (T->label) ? T->label : "");
        for (i = 0; i < T->numVals; i++)
          cprintf(file, "%d\t%f\n", T->val[i].x, T->val[i].y);
      }
    });
  } else {
    FOR_EACH_TRACE(G, T->export = 0;);
    if (labels) {
      cprintf(file, "#");
      FOR_EACH_TRACE(G, {
        if (T->visible) cprintf(file, "\t%s", (T->label) ? T->label : "");
      });
      cprintf(file, "\n");
    }
    while (1) {
      minX = 1000000;
      FOR_EACH_TRACE(G, {
        if (T->visible && T->export < T->numVals && T->val[T->export].x < minX)
	  minX = T->val[T->export].x;
      });
      if (minX == 1000000) break;
      cprintf(file, "%d", minX);
      FOR_EACH_TRACE(G, {
        if (T->visible) {
          if (T->export < T->numVals && T->val[T->export].x == minX)
            cprintf(file, "\t%g", T->val[T->export++].y);
          else
            cprintf(file, "\t-");
        }
      });
      cprintf(file, "\n");
    }
  }
  closeChannel(file);
  return TCL_OK;
}
