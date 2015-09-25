#include <string.h>
#include "system.h"
#include "util.h"
#include "type.h"
#include "network.h"
#include "command.h"
#include "control.h"
#include "graph.h"

int C_graph(TCL_CMDARGS) {
  Graph G;
  const char *usage = "graph [create | list] | [delete | refresh | update | store | clear |\n\thide | show] <graph-list>";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  /* if (Batch) return result("%s is not available in batch mode", commandName); */
  if (objc < 2) return usageError(commandName, usage);
  if (subString(Tcl_GetStringFromObj(objv[1], NULL), "create", 2)) {
    if (objc != 2) return usageError(commandName, usage);
    if (!(G = createGraph())) return TCL_ERROR;
    return result("%d", G->num);
  } else if (subString(Tcl_GetStringFromObj(objv[1], NULL), "list", 1)) {
    if (objc != 2) return usageError(commandName, usage);
    {
      char gname[16];
      String graphs = newString(64);
      FOR_EACH_GRAPH({
        sprintf(gname, "%d ", G->num);
        stringAppend(graphs, gname);
      });
      if (graphs->numChars) graphs->s[--graphs->numChars] = '\0';
      result(graphs->s);
      freeString(graphs);
     }
  } else {
    if (objc != 3) return usageError(commandName, usage);
    FOR_EACH_GRAPH_IN_LIST(Tcl_GetStringFromObj(objv[2], NULL), {
      if (subString(Tcl_GetStringFromObj(objv[1], NULL), "delete", 1)) {
        deleteGraph(G);
      } else if (subString(Tcl_GetStringFromObj(objv[1], NULL), "refresh", 1)) {
        drawLater(G);
        refreshPropsLater(G);
      } else if (subString(Tcl_GetStringFromObj(objv[1], NULL), "update", 1)) {
        if (G->updateOn) updateGraph(G);
      } else if (subString(Tcl_GetStringFromObj(objv[1], NULL), "store", 2)) {
        storeGraph(G);
      } else if (subString(Tcl_GetStringFromObj(objv[1], NULL), "clear", 1)) {
        clearGraph(G);
      } else if (subString(Tcl_GetStringFromObj(objv[1], NULL), "hide", 1)) {
        hideGraph(G);
      } else if (subString(Tcl_GetStringFromObj(objv[1], NULL), "show", 2)) {
        showGraph(G);
      }
    });
  }
  return TCL_OK;
}

int C_trace(TCL_CMDARGS) {
  Graph G;
  Trace T;
  int g;
  const char *usage = "trace create <graph> [<object>] | list <graph> | [delete | store | clear] <graph> <trace-list>";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  /* if (Batch) return result("%s is not available in batch mode", commandName); */
  if (objc < 2) return usageError(commandName, usage);
  Tcl_GetIntFromObj(interp, objv[2], &g);
  if (!(G = lookupGraph(g)))
    return warning("%s: graph \"%s\" doesn't exist", commandName, objv[2]);
  if (subString(Tcl_GetStringFromObj(objv[1], NULL), "create", 2)) {
    if (objc != 3 && objc != 4) return usageError(commandName, usage);
    if (objc == 4) {
      if (!(T = createTrace(G, Tcl_GetStringFromObj(objv[3], NULL)))) return TCL_ERROR;
    } else {
      if (!(T = createTrace(G, ""))) return TCL_ERROR;
    }
    return result("%d", T->num);
  } else if (subString(Tcl_GetStringFromObj(objv[1], NULL), "list", 1)) {
    if (objc != 3) return usageError(commandName, usage);
    {
      char tname[16];
      String traces = newString(64);
      FOR_EACH_TRACE(G, {
        sprintf(tname, "%d ", T->num);
        stringAppend(traces, tname);
      });
      if (traces->numChars) traces->s[--traces->numChars] = '\0';
      result(traces->s);
      freeString(traces);
    }
  } else {
    if (objc != 4) return usageError(commandName, usage);
    FOR_EACH_TRACE_IN_LIST(G, Tcl_GetStringFromObj(objv[3], NULL), {
      if (subString(Tcl_GetStringFromObj(objv[1], NULL), "delete", 1)) {
        deleteTrace(T);
      } else if (subString(Tcl_GetStringFromObj(objv[1], NULL), "store", 1)) {
        storeTrace(T);
      } else if (subString(Tcl_GetStringFromObj(objv[1], NULL), "clear", 2)) {
        clearTrace(T);
      }
    });
  }
  return TCL_OK;
}

int C_graphObject(TCL_CMDARGS) {
  Graph G;
  int arg = 2;
  mask updates = ON_REPORT;
  char *objects;
  const char *usage = "graphObject [<object-list> [-updates <update-rate>]]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  /* if (Batch) return result("%s is not available in batch mode", commandName); */
  for (; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    switch (Tcl_GetStringFromObj(objv[arg], NULL)[1]) {
    case 'u':
      if (++arg >= objc) return usageError(commandName, usage);
      if (lookupTypeMask(Tcl_GetStringFromObj(objv[arg], NULL), UPDATE_SIGNAL, &updates))
	return warning("%s: unknown update signal: %s\n", commandName, Tcl_GetStringFromObj(objv[arg], NULL));
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg < objc) return usageError(commandName, usage);
  if (!(G = createGraph())) return TCL_ERROR;
  G->updateOn = updates;
  objects = (objc > 1) ? Tcl_GetStringFromObj(objv[1], NULL) : "error";
  FOR_EACH_STRING_IN_LIST(objects, {
    if (!createTrace(G, S->s)) return TCL_ERROR;
  });
  return result("%d", G->num);
}

int C_exportGraph(TCL_CMDARGS) {
  Graph G;
  int arg,g;
  flag labels = FALSE, gnuplot = FALSE;
  const char *usage = "exportGraph <graph> <file-name> [-labels | -gnuplot]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  //const char *fileName;
  Tcl_Obj *fileNameObj;
  if (objc < 3) return usageError(commandName, usage);
  Tcl_GetIntFromObj(interp, objv[1], &g);
  if (!(G = lookupGraph(g)))
    return warning("Graph %s doesn't exist", objv[1]);
  for (arg = 3; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    switch (Tcl_GetStringFromObj(objv[arg], NULL)[1]) {
    case 'l':
      labels = TRUE;
      break;
    case 'g':
      gnuplot = TRUE;
      break;
    default: return usageError(commandName, usage);
    }
  }
  fileNameObj = objv[2];
  //fileName = Tcl_GetStringFromObj(objv[2], NULL);
  if (arg < objc) return usageError(commandName, usage);
  return exportGraphData(G, fileNameObj, labels, gnuplot);
}

int C_drawAllTraceProps(TCL_CMDARGS) {
  Graph G;
  int g;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 2) return warning("%s was called improperly", commandName);
  Tcl_GetIntFromObj(interp, objv[1], &g);
  if (!(G = lookupGraph(g)))
    return warning("Graph %d doesn't exist", g);
  return drawTraceProps(G);
}

int C_setGraphSize(TCL_CMDARGS) {
  Graph G;
  int w, h, g;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 4) return warning("%s was called improperly", commandName);
  Tcl_GetIntFromObj(interp, objv[1], &g);
  if (!(G = lookupGraph(g)))
    return warning("Graph %s doesn't exist", objv[1]);
  Tcl_GetIntFromObj(interp, objv[2], &w);
  Tcl_GetIntFromObj(interp, objv[3], &h);
  if (h == G->height && w == G->width) return TCL_OK;
  if (h == 1 && w == 1) return TCL_OK;
  G->height = h;
  G->width = w;
  return drawLater(G);
}

int C_colorName(TCL_CMDARGS) {
  real H, S, B;
  double tempDbl;
  char color[32];
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 4) return warning("%s was called improperly", commandName);
  Tcl_GetDoubleFromObj(interp, objv[1], &tempDbl);
  H = (real)tempDbl;
  Tcl_GetDoubleFromObj(interp, objv[2], &tempDbl);
  S = (real)tempDbl;
  Tcl_GetDoubleFromObj(interp, objv[3], &tempDbl);
  B = (real)tempDbl;
  colorName(color, H, S, B);
  return result(color);
}

void registerGraphCommands(void) {

  registerCommand((Tcl_ObjCmdProc *)C_graphObject, "graphObject",
		  "opens a graph of the value of an object or a command");
  registerCommand((Tcl_ObjCmdProc *)C_graph, "graph",
		  "creates, deletes, or manipulates graphs");
  registerCommand((Tcl_ObjCmdProc *)C_exportGraph, "exportGraph",
                  "export the data in a graph to a file");
  registerCommand((Tcl_ObjCmdProc *)C_trace, "trace",
                  "creates, deletes, or manipulates traces within a graph");

  createCommand((Tcl_ObjCmdProc *)C_drawAllTraceProps, ".drawAllTraceProps");
  createCommand((Tcl_ObjCmdProc *)C_setGraphSize, ".setGraphSize");
  createCommand((Tcl_ObjCmdProc *)C_colorName, ".colorName");
}
