#include <string.h>
#include "system.h"
#include "util.h"
#include "type.h"
#include "network.h"
#include "connect.h"
#include "control.h"
#include "example.h"
#include "display.h"
#include "parallel.h"
#include "graph.h"

#define OUTPUT_SCALE 0.7

flag UnitUp;
flag LinkUp;

char *OutlineColor[NUM_PALETTES][3];
char *ColorMap[NUM_PALETTES][NUM_COLORS];
char *NaNColor = "grey50";
int  PlotWidth, Space, ViewTick;
flag UnitsNeedRedraw = FALSE, LinksNeedRedraw = FALSE;

flag drawUnits(ClientData junk);
flag drawLinks(ClientData junk);

void fillPalette(int palette) {
  static char done[4] = {FALSE, FALSE, FALSE, FALSE};
  int i, ival;
  real rval, red, green, blue;
  char buf[32];
  if (done[palette]) return;
  done[palette] = TRUE;
  switch (palette) {
  case BLUE_BLACK_RED:
    for (i = 0; i < NUM_COLORS; i++) {
      red = ((real) i / (NUM_COLORS - 1)) * 2 - 1;
      if (red < 0.0) {
	blue = -red;
	red = 0.0;
      } else blue = 0.0;
      sprintf(buf, "#%03X000%03X", (int)(red * 4095), (int)(blue * 4095));
      ColorMap[BLUE_BLACK_RED][i] = copyString(buf);
    }
    OutlineColor[BLUE_BLACK_RED][0] = "black";
    OutlineColor[BLUE_BLACK_RED][1] = "yellow";
    OutlineColor[BLUE_BLACK_RED][2] = "magenta";
    break;
  case BLUE_RED_YELLOW:
    for (i = 0; i < NUM_COLORS; i++) {
      rval = ((real) i / (NUM_COLORS - 1));
      if (rval <= 0.5) {
	green = 0.0;
	red = rval * 2;
	blue = 1.0 - red;
      } else {
	red = 1.0;
	blue = 0.0;
	green = rval * 2 - 1.0;
      }
      sprintf(buf, "#%03X%03X%03X", (int)(red * 4095), (int)(green * 4095),
	      (int)(blue * 4095));
      ColorMap[BLUE_RED_YELLOW][i] = copyString(buf);
    }
    OutlineColor[BLUE_RED_YELLOW][0] = "black";
    OutlineColor[BLUE_RED_YELLOW][1] = "white";
    OutlineColor[BLUE_RED_YELLOW][2] = "magenta";
    break;
  case BLACK_GRAY_WHITE:
    for (i = 0; i < NUM_COLORS; i++) {
      ival = (int) ((real) i / (NUM_COLORS - 1) * 4095);
      sprintf(buf, "#%03X%03X%03X", ival, ival, ival);
      ColorMap[BLACK_GRAY_WHITE][i] = copyString(buf);
    }
    OutlineColor[BLACK_GRAY_WHITE][0] = "grey50";
    OutlineColor[BLACK_GRAY_WHITE][1] = "black";
    OutlineColor[BLACK_GRAY_WHITE][2] = "white";
    break;
  case HINTON:
    OutlineColor[HINTON][0] = "grey50";
    OutlineColor[HINTON][1] = "black";
    OutlineColor[HINTON][2] = "white";
    break;
  }
}

real normColorValue(real x, real temp) {
  if (isNaN(x)) return x;
  if (x * temp > 10) return 1.0;
  else if (x * temp < -10) return 0.0;
  else return SIGMOID(x, temp);
}

char *getColor(real x, real temp, int palette) {
  x = normColorValue(x, temp);
  if (isNaN(x)) return NaNColor;
  return ColorMap[palette][(int)(x * (NUM_COLORS - 1))];
}

flag setUnitTemp(real val) {
  if (Net) {
    Net->unitTemp = POW(2.0, val);
    updateUnitDisplay();
  }
  return TCL_OK;
}

enum plotfieldtypes {NEXT, LNEXT, CNEXT, RNEXT, SPAN, UNITF, BLANK, FILL};

typedef struct plotField *PlotField;
struct plotField {
  int type;
  int num;
  int width;
  int numUnits;
  Group group;
  Unit unit;
  PlotField next;
};

static void freePlotFieldList(PlotField PF) {
  PlotField next;
  for (; PF; PF = next) {
    next = PF->next;
    free(PF);
  }
}

static int numUnplotted(PlotField PF, int needed) {
  int unused = 0;
  Group G = PF->group;
  FOR_EVERY_UNIT(G, {
    if (unused >= needed) break;
    if (U->plotCol == 0) {
      U->plotCol = -PF->num;
      unused++;
    }
  });
  return unused;
}

static int spanUnplotted(PlotField PF) {
  int u, unused = 0;
  Group G = PF->group;
  for (u = PF->unit->num; u < G->numUnits && u < PF->unit->num + PF->width;
       u++) {
    if (G->unit[u].plotCol == 0) {
      G->unit[u].plotCol = -PF->num;
      unused++;
    }
  }
  return unused;
}

static int plotNext(PlotField PF, int offset) {
  int plotted = 0;
  Group G = PF->group;
  FOR_EVERY_UNIT(G, {
    if (plotted >= PF->numUnits) break;
    if (U->plotCol == -PF->num) {
      U->plotCol = offset + plotted++;
      U->plotRow = Net->plotRows;
    }
  });
  return plotted;
}

static int plotSpan(PlotField PF, int offset) {
  int u, plotted = 0;
  Group G = PF->group;
  Unit U;
  for (u = PF->unit->num; u < G->numUnits && u < PF->width + PF->unit->num;
       u++) {
    U = G->unit + u;
    if (U->plotCol == -PF->num) {
      U->plotCol = offset + u - PF->unit->num;
      U->plotRow = Net->plotRows;
      plotted++;
    }
  }
  return plotted;
}

flag plotRow(int argc, char *argv[], int *unitsPlotted) {
  int arg, numFills = 0, numUnits, totalWidth = 0, fillSpace, numShort,
    numFields = 0;
  PlotField PF, first = NULL, last = NULL;

  Net->plotRows++;
  arg = 0;
  while (arg < argc) {
    numFields++;
    PF = (PlotField) safeCalloc(1, sizeof *PF, "plotRow:PF");
    PF->num = numFields;
    if (last) last->next = PF;
    else first = PF;
    last = PF;

    switch (argv[arg++][0]) {
    case 'n':
      if (arg >= argc - 1) return warning("plotRow: missing fields");
      PF->type = NEXT;
      if (!(PF->group = lookupGroup(argv[arg++]))) {
	freePlotFieldList(first);
	return warning("plotRow: group \"%s\" doesn't exist", argv[--arg]);
      }
      PF->width = PF->numUnits = numUnplotted(PF, atoi(argv[arg++]));
      break;
    case 'l':
      if (arg >= argc - 1) return warning("plotRow: missing fields");
      PF->type = LNEXT;
      if (!(PF->group = lookupGroup(argv[arg++]))) {
	freePlotFieldList(first);
	return warning("plotRow: group \"%s\" doesn't exist", argv[--arg]);
      }
      if ((PF->width = atoi(argv[arg++])) < 0) {
	freePlotFieldList(first);
	return warning("plotRow: numUnits (%d) must be non-negative "
		       "for group \"%s\"", PF->width, PF->group->name);
      }
      PF->numUnits = numUnplotted(PF, PF->width);
      break;
    case 'c':
      if (arg >= argc - 1) return warning("plotRow: missing fields");
      PF->type = CNEXT;
      if (!(PF->group = lookupGroup(argv[arg++]))) {
	freePlotFieldList(first);
	return warning("plotRow: group \"%s\" doesn't exist", argv[--arg]);
      }
      if ((PF->width = atoi(argv[arg++])) < 0) {
	freePlotFieldList(first);
	return warning("plotRow: numUnits (%d) must be non-negative "
		       "for group \"%s\"", PF->width, PF->group->name);
      }
      PF->numUnits = numUnplotted(PF, PF->width);
      break;
    case 'r':
      if (arg >= argc - 1) return warning("plotRow: missing fields");
      PF->type = RNEXT;
      if (!(PF->group = lookupGroup(argv[arg++]))) {
	freePlotFieldList(first);
	return warning("plotRow: group \"%s\" doesn't exist", argv[--arg]);
      }
      if ((PF->width = atoi(argv[arg++])) < 0) {
	freePlotFieldList(first);
	return warning("plotRow: numUnits (%d) must be non-negative "
		       "for group \"%s\"", PF->width, PF->group->name);
      }
      PF->numUnits = numUnplotted(PF, PF->width);
      break;
    case 's':
      if (arg >= argc - 2) return warning("plotRow: missing fields");
      PF->type = SPAN;
      if (!(PF->group = lookupGroup(argv[arg++]))) {
	freePlotFieldList(first);
	return warning("plotRow: group \"%s\" doesn't exist", argv[--arg]);
      }
      PF->width = atoi(argv[arg++]);
      if (PF->width < 0 || PF->width >= PF->group->numUnits) {
	freePlotFieldList(first);
	return warning("plotRow: unit index %d of range in group \"%s\"",
		       PF->width, PF->group->name);
      }
      PF->unit = PF->group->unit + PF->width;
      if ((PF->width = atoi(argv[arg++])) < 0) {
	freePlotFieldList(first);
	return warning("plotRow: numUnits (%d) must be non-negative "
		       "for group \"%s\"", PF->width, PF->group->name);
      }
      PF->numUnits = spanUnplotted(PF);
      break;
    case 'u':
      if (arg >= argc) return warning("plotRow: missing fields");
      PF->type = UNITF;
      if (!(PF->unit = lookupUnit(argv[arg++]))) {
	freePlotFieldList(first);
	return warning("plotRow: group \"%s\" doesn't exist", argv[--arg]);
      }
      PF->width = 1;
      PF->unit->plotCol = -PF->num;
      break;
    case 'b':
      if (arg >= argc) return warning("plotRow: missing fields");
      PF->type = BLANK;
      if ((PF->width = atoi(argv[arg++])) < 0) {
	freePlotFieldList(first);
	return warning("plotRow: blank width (%d) must be non-negative",
		       PF->width);
      }
      break;
    case 'f':
      PF->type = FILL;
      numFills++;
      break;
    default:
      return warning("plotRow: unrecognized command: \"%s\"", argv[--arg]);
    }
    totalWidth += PF->width;
  }

  if (totalWidth < Net->plotCols && numFills) {
    fillSpace = (Net->plotCols - totalWidth) / numFills;
    numShort = numFills - ((Net->plotCols - totalWidth) % numFills);
    for (PF = first; PF; PF = PF->next)
      if (PF->type == FILL) {
	if (numShort) {
	  PF->width = fillSpace;
	  numShort--;
	}
	else
	  PF->width = fillSpace + 1;
      }
  }

  /* I now use totalWidth for the offset of each field */
  for (PF = first, totalWidth = 1, numUnits = 0; PF; PF = PF->next) {
    switch (PF->type) {
    case NEXT:
    case LNEXT:
      numUnits += plotNext(PF, totalWidth);
      break;
    case CNEXT:
      numUnits += plotNext(PF, totalWidth + ((PF->width - PF->numUnits) / 2));
      break;
    case RNEXT:
      numUnits += plotNext(PF, totalWidth + PF->width - PF->numUnits);
      break;
    case SPAN:
      numUnits += plotSpan(PF, totalWidth);
      break;
    case UNITF:
      /* It is important that this doesn't add 1 to numUnits */
      PF->unit->plotCol = totalWidth;
      PF->unit->plotRow = Net->plotRows;
      break;
    case BLANK:
    case FILL:
      break;
    default: fatalError("plotRow: bad plotField type: %d", PF->type);
    }
    totalWidth += PF->width;
  }

  freePlotFieldList(first);
  *unitsPlotted = numUnits;
  return TCL_OK;
}

flag plotRowObj(Tcl_Interp *interp, int objc, Tcl_Obj *objv[], int *unitsPlotted) {
  int arg, numFills = 0, numUnits, totalWidth = 0, fillSpace, numShort,
    numFields = 0, needed;
  PlotField PF, first = NULL, last = NULL;

  Net->plotRows++;
  arg = 0;
  while (arg < objc) {
    numFields++;
    PF = (PlotField) safeCalloc(1, sizeof *PF, "plotRow:PF");
    PF->num = numFields;
    if (last) last->next = PF;
    else first = PF;
    last = PF;

    switch (Tcl_GetStringFromObj(objv[arg++], NULL)[0]) {
    case 'n':
      if (arg >= objc - 1) return warning("plotRow: missing fields");
      PF->type = NEXT;
      if (!(PF->group = lookupGroup(Tcl_GetStringFromObj(objv[arg++], NULL)))) {
        freePlotFieldList(first);
        return warning("plotRow: group \"%s\" doesn't exist", Tcl_GetStringFromObj(objv[--arg], NULL));
      }
      Tcl_GetIntFromObj(interp, objv[arg++], &needed);
      PF->width = PF->numUnits = numUnplotted(PF, needed);
      break;
    case 'l':
      if (arg >= objc - 1) return warning("plotRow: missing fields");
      PF->type = LNEXT;
      if (!(PF->group = lookupGroup(Tcl_GetStringFromObj(objv[arg++], NULL)))) {
        freePlotFieldList(first);
        return warning("plotRow: group \"%s\" doesn't exist", Tcl_GetStringFromObj(objv[--arg], NULL));
      }
      Tcl_GetIntFromObj(interp, objv[arg++], &PF->width);
      if (PF->width < 0) {
        freePlotFieldList(first);
        return warning("plotRow: numUnits (%d) must be non-negative "
                 "for group \"%s\"", PF->width, PF->group->name);
      }
      PF->numUnits = numUnplotted(PF, PF->width);
      break;
    case 'c':
      if (arg >= objc - 1) return warning("plotRow: missing fields");
      PF->type = CNEXT;
      if (!(PF->group = lookupGroup(Tcl_GetStringFromObj(objv[arg++], NULL)))) {
        freePlotFieldList(first);
        return warning("plotRow: group \"%s\" doesn't exist", Tcl_GetStringFromObj(objv[--arg], NULL));
      }
      Tcl_GetIntFromObj(interp, objv[arg++], &PF->width);
      if (PF->width < 0) {
        freePlotFieldList(first);
        return warning("plotRow: numUnits (%d) must be non-negative "
                 "for group \"%s\"", PF->width, PF->group->name);
      }
      PF->numUnits = numUnplotted(PF, PF->width);
      break;
    case 'r':
      if (arg >= objc - 1) return warning("plotRow: missing fields");
      PF->type = RNEXT;
      if (!(PF->group = lookupGroup(Tcl_GetStringFromObj(objv[arg++], NULL)))) {
        freePlotFieldList(first);
        return warning("plotRow: group \"%s\" doesn't exist", Tcl_GetStringFromObj(objv[--arg], NULL));
      }
      Tcl_GetIntFromObj(interp, objv[arg++], &PF->width);
      if (PF->width < 0) {
        freePlotFieldList(first);
        return warning("plotRow: numUnits (%d) must be non-negative "
                 "for group \"%s\"", PF->width, PF->group->name);
      }
      PF->numUnits = numUnplotted(PF, PF->width);
      break;
    case 's':
      if (arg >= objc - 2) return warning("plotRow: missing fields");
      PF->type = SPAN;
      if (!(PF->group = lookupGroup(Tcl_GetStringFromObj(objv[arg++], NULL)))) {
        freePlotFieldList(first);
        return warning("plotRow: group \"%s\" doesn't exist", Tcl_GetStringFromObj(objv[--arg], NULL));
      }
      Tcl_GetIntFromObj(interp, objv[arg++], &PF->width);
      if (PF->width < 0 || PF->width >= PF->group->numUnits) {
        freePlotFieldList(first);
        return warning("plotRow: unit index %d of range in group \"%s\"",
                 PF->width, PF->group->name);
      }
      PF->unit = PF->group->unit + PF->width;
      Tcl_GetIntFromObj(interp, objv[arg++], &PF->width);
      if (PF->width < 0) {
        freePlotFieldList(first);
        return warning("plotRow: numUnits (%d) must be non-negative "
                 "for group \"%s\"", PF->width, PF->group->name);
      }
      PF->numUnits = spanUnplotted(PF);
      break;
    case 'u':
      if (arg >= objc) return warning("plotRow: missing fields");
      PF->type = UNITF;
      if (!(PF->unit = lookupUnit(Tcl_GetStringFromObj(objv[arg++], NULL)))) {
        freePlotFieldList(first);
        return warning("plotRow: group \"%s\" doesn't exist", objv[--arg]);
      }
      PF->width = 1;
      PF->unit->plotCol = -PF->num;
      break;
    case 'b':
      if (arg >= objc) return warning("plotRow: missing fields");
      PF->type = BLANK;
      Tcl_GetIntFromObj(interp, objv[arg++], &PF->width);
      if (PF->width < 0) {
        freePlotFieldList(first);
        return warning("plotRow: blank width (%d) must be non-negative",
                 PF->width);
      }
      break;
    case 'f':
      PF->type = FILL;
      numFills++;
      break;
    default:
      return warning("plotRow: unrecognized command: \"%s\"", Tcl_GetStringFromObj(objv[--arg], NULL));
    }
    totalWidth += PF->width;
  }

  if (totalWidth < Net->plotCols && numFills) {
    fillSpace = (Net->plotCols - totalWidth) / numFills;
    numShort = numFills - ((Net->plotCols - totalWidth) % numFills);
    for (PF = first; PF; PF = PF->next)
      if (PF->type == FILL) {
	if (numShort) {
	  PF->width = fillSpace;
	  numShort--;
	}
	else
	  PF->width = fillSpace + 1;
      }
  }

  /* I now use totalWidth for the offset of each field */
  for (PF = first, totalWidth = 1, numUnits = 0; PF; PF = PF->next) {
    switch (PF->type) {
    case NEXT:
    case LNEXT:
      numUnits += plotNext(PF, totalWidth);
      break;
    case CNEXT:
      numUnits += plotNext(PF, totalWidth + ((PF->width - PF->numUnits) / 2));
      break;
    case RNEXT:
      numUnits += plotNext(PF, totalWidth + PF->width - PF->numUnits);
      break;
    case SPAN:
      numUnits += plotSpan(PF, totalWidth);
      break;
    case UNITF:
      /* It is important that this doesn't add 1 to numUnits */
      PF->unit->plotCol = totalWidth;
      PF->unit->plotRow = Net->plotRows;
      break;
    case BLANK:
    case FILL:
      break;
    default: fatalError("plotRow: bad plotField type: %d", PF->type);
    }
    totalWidth += PF->width;
  }

  freePlotFieldList(first);
  *unitsPlotted = numUnits;
  return TCL_OK;
}

flag drawUnitsLater(void) {
  if (!UnitsNeedRedraw) {
    UnitsNeedRedraw = TRUE;
    Tcl_DoWhenIdle((Tcl_IdleProc *) drawUnits, NULL);
  }
  return TCL_OK;
}

flag drawUnits(ClientData junk) {
  int x, y;
  if (!UnitsNeedRedraw) return TCL_OK;
  UnitsNeedRedraw = FALSE;
  if (!UnitUp) return TCL_OK;
  if (!Net) {
    if (Tcl_Eval(Interp, ".unit.c.c delete all")) return TCL_ERROR;
    return updateUnitDisplay();
  }
  fillPalette(Net->unitPalette);
  if (Net->unitCellSize < 5 || Net->unitCellSize > 100)
    return warning("drawUnits: unitCellSize (%d) must be in the range [5,100]",
		   Net->unitCellSize);
  if (Net->unitCellSpacing < 0 || Net->unitCellSpacing > 100)
    return warning("drawUnits: unitCellSpacing (%d) must be in the range "
		   "[0,100]", Net->unitCellSpacing);
  Space = Net->unitCellSize + Net->unitCellSpacing;

  if (Tcl_Eval(Interp, ".unit.c.c delete all"))
    return TCL_ERROR;

  PlotWidth = 0;
  FOR_EACH_GROUP(FOR_EVERY_UNIT(G, {
    if (U->plotCol > PlotWidth)
      PlotWidth = U->plotCol;
  }));
  if (!PlotWidth) return updateUnitDisplay();
  /* Now it's the width of one epoch */
  PlotWidth = (PlotWidth + 2) * Space;

  eval(".unit.c.c configure -scrollregion {0 0 %d %d}",
       PlotWidth, (Net->plotRows + 2) * Space);
  eval(".unit.c.c xview moveto 0; .unit.c.c yview moveto 0");

  FOR_EACH_GROUP({
    if (G->costType & ERROR_MASKS) {
      int outputShift; int outputSize;
      if (Net->unitCellSize <= 7) outputSize = Net->unitCellSize - 4;
      else if (Net->unitCellSize <= 14) outputSize = Net->unitCellSize - 6;
      else outputSize = Net->unitCellSize * OUTPUT_SCALE;
      if ((outputSize & 1) != (Net->unitCellSize & 1)) outputSize--;
      outputShift = (Net->unitCellSize - outputSize) / 2;

      FOR_EVERY_UNIT(G, {
	if (U->plotCol) {
	  x = U->plotCol * Space;
	  y = U->plotRow * Space;

	  if (eval(".unit.c.c create canvrect %d %d %d %d  %d %d -3 "
		   "-fill %s -tag {cr %d:%d:t};"
		   ".setUnitBindings %d:%d:t %d %d 1",
		   x, y, x + Net->unitCellSize, y + Net->unitCellSize, G->num,
		   U->num, NaNColor, G->num, U->num, G->num, U->num, G->num,
		   U->num)
	      != TCL_OK) return TCL_ERROR;
	  x += outputShift;
	  y += outputShift;
	  if (eval(".unit.c.c create canvrect %d %d %d %d  %d %d -2 "
		   "-fill %s -tag {cr %d:%d:0};"
		   ".setUnitBindings %d:%d:0 %d %d 0",
		   x, y, x + outputSize, y + outputSize, G->num, U->num,
		   NaNColor, G->num, U->num, G->num, U->num, G->num, U->num)
	      != TCL_OK) return TCL_ERROR;
	}
      });
    }
    else FOR_EVERY_UNIT(G, {
      if (U->plotCol) {
	x = U->plotCol * Space;
	y = U->plotRow * Space;

	if (eval(".unit.c.c create canvrect %d %d %d %d  %d %d -1 "
		 "-fill %s -width 1 -tag {cr %d:%d:0};"
		 ".setUnitBindings %d:%d:0 %d %d 0",
		 x, y, x + Net->unitCellSize, y + Net->unitCellSize, G->num,
		 U->num, NaNColor, G->num, U->num, G->num, U->num, G->num,
		 U->num) != TCL_OK) return TCL_ERROR;
      }
    });
  });

  if (eval(".setViewerSize %d %d", PlotWidth, Space * (Net->plotRows + 2)))
    return TCL_ERROR;

  if (eval(".unit.c.c configure -xscrollincrement %d -yscrollincrement %d",
	   Space, Space))
    return TCL_ERROR;
  return setUnitValue(Net->unitDisplayValue);
}

/* If number of columns is 0, it figures out a nice value */
flag autoPlot(int numCols) {
  int maxUnits, argc, unitsPlotted;
  char *argv[7];

  if (numCols <= 0) {
    maxUnits = 0;
    numCols = 10;
    FOR_EACH_GROUP({
      if (G->numColumns) {
	if (G->numColumns > numCols) numCols = G->numColumns;
      } else if (G->numUnits > maxUnits) maxUnits = G->numUnits;
    });
    numCols = imax(numCols, CEIL(SQRT(5 * maxUnits)));
  }
  Net->plotCols = numCols;
  Net->plotRows = 0;
  FOR_EACH_GROUP(FOR_EVERY_UNIT(G, U->plotCol = U->plotRow = 0));

  maxUnits = 0;
  FOR_EACH_GROUP_BACK({
    if (G->type & BIAS) {
      argv[0] = "l";
      argv[1] = G->name;
      sprintf(Buffer, "%d", numCols);
      argv[2] = Buffer;
      argc = 3;
    }
    else if (G->type & ELMAN) {
      Net->plotRows--;
      argv[0] = "b";
      argv[1] = "2";
      argv[2] = "f";
      argv[3] = "c";
      argv[4] = G->name;
      sprintf(Buffer, "%d", (G->numColumns > 0) ? G->numColumns : numCols);
      argv[5] = Buffer;
      argv[6] = "f";
      argc = 7;
    }
    else {
      argv[0] = "f";
      argv[1] = "c";
      argv[2] = G->name;
      sprintf(Buffer, "%d", (G->numColumns > 0) ? G->numColumns : numCols);
      argv[3] = Buffer;
      argv[4] = "f";
      argc = 5;
    }
    do {
      if (plotRow(argc, argv, &unitsPlotted))
	return TCL_ERROR;
    } while (unitsPlotted > 0);
  });
  Net->plotRows--;

  return drawUnitsLater();
}

flag setUnitValue(int val) {
  /* Set the label */
  if (eval("global _valueLabel; .unit.c.label configure "
	   "-text $_valueLabel(%d)", val))
    return TCL_ERROR;
  Net->unitDisplayValue = val;
  Tcl_UpdateLinkedVar(Interp, ".unitDisplayValue");
  return updateUnitDisplay();
}

static flag displayPipeExample(ExampleSet S) {
  eval(".unit.l.list delete 0 end");
  if (!S->pipeExample)
    return warning("example set %s has no pipe example", S->name);
  if (S->pipeExample->name)
    eval(".unit.l.list insert end {%s}", S->pipeExample->name);
  else eval(".unit.l.list insert end {Pipe Example}");
  return TCL_OK;
}

flag updateUnitDisplay(void) {
  int tpi, event, eventStart, eventStop;
  ExampleSet S;
  if (!UnitUp || UnitsNeedRedraw) return TCL_OK;
  if (!Net || !Net->currentExample) {
    eval(".unit.t.left3 configure -state disabled;"
	 ".unit.t.left2 configure -state disabled;"
	 ".unit.t.left  configure -state disabled;"
	 ".unit.t.left0 configure -state disabled;"
	 ".unit.t.right0 configure -state disabled;"
	 ".unit.t.right  configure -state disabled;"
	 ".unit.t.right2 configure -state disabled;"
	 ".unit.t.right3 configure -state disabled;"
	 ".unit.t.step2 configure -state disabled;"
	 ".unit.t.step3 configure -state disabled;"
	 ".unit.t.event configure -state disabled;"
	 ".unit.t.tick configure -state disabled;"
	 ".unit.t.etick configure -state disabled;");
    eval("set .viewEvent {};"
	 "set .viewEvents {};"
	 "set .viewTime {};"
	 "set .viewMaxTime {};"
	 "set .viewEventTime {};"
	 "set .viewMaxEventTime {};"
	 ".setUnitEntries;");

    return eval(".unit.c.c itemconfigure all -stipple {}");
  }
  S = Net->currentExample->set;
  tpi = Net->ticksPerInterval;
  ViewTick = imax(ViewTick, 0);
  ViewTick = imin(ViewTick, Net->ticksOnExample - 1);
  event = Net->eventHistory[ViewTick];

  if (eval(".unit.c.c delete hint") != TCL_OK)
    return TCL_ERROR;
  if (eval(".unit.c.c itemconfigure cr -stipple {}") != TCL_OK)
    return TCL_ERROR;
  if ((Net->unitDisplaySet == TRAIN_SET && S == Net->trainingSet) ||
      (Net->unitDisplaySet == TEST_SET && S == Net->testingSet)) {
    if (S->mode == PIPE && displayPipeExample(S)) return TCL_ERROR;
    if (eval(".unit.l.list selection clear 0 end; .unit.l.list see %d; "
	     ".unit.l.list activate %d; .unit.l.list selection set active;",
	     Net->currentExample->num, Net->currentExample->num))
      return TCL_ERROR;
  } else eval(".unit.l.list selection clear 0 end");

  for (eventStart = 0; Net->eventHistory[eventStart] < event;
       eventStart++);
  for (eventStop = eventStart; eventStop < Net->ticksOnExample &&
	 Net->eventHistory[eventStop] == event; eventStop++);
  if (event >= 0) eval("set .viewEvent %d", event);
  else eval("set .viewEvent {}");
  eval("set .viewEvents %d", Net->currentExample->numEvents);
  eval("set .viewTime %d:%d", (ViewTick + 1) / tpi, (ViewTick + 1) % tpi);
  eval("set .viewMaxTime %d:%d", Net->ticksOnExample / tpi,
       Net->ticksOnExample % tpi);
  eval("set .viewEventTime %d:%d", (ViewTick - eventStart + 1) / tpi,
       (ViewTick - eventStart + 1) % tpi);
  eval("set .viewMaxEventTime %d:%d", (eventStop - eventStart) / tpi,
       (eventStop - eventStart) % tpi);

  eval(".setUnitEntries");

  eval(".unit.t.event configure -state normal;"
       ".unit.t.tick configure -state normal;"
       ".unit.t.etick configure -state normal;");

  if (ViewTick <= 0 || !Net->currentExample)
    eval(".unit.t.left3 configure -state disabled;"
	 ".unit.t.left2 configure -state disabled;"
	 ".unit.t.left  configure -state disabled;");
  else
    eval(".unit.t.left3 configure -state normal;"
	 ".unit.t.left2 configure -state normal;"
	 ".unit.t.left  configure -state normal");
  if (ViewTick >= Net->ticksOnExample - 1 || !Net->currentExample)
    eval(".unit.t.step3 configure -state disabled;"
	 ".unit.t.step2 configure -state disabled;"
	 ".unit.t.right3 configure -state disabled;"
	 ".unit.t.right2 configure -state disabled;"
	 ".unit.t.right  configure -state disabled;");
  else
    eval(".unit.t.step3 configure -state normal;"
	 ".unit.t.step2 configure -state normal;"
	 ".unit.t.right3 configure -state normal;"
	 ".unit.t.right2 configure -state normal;"
	 ".unit.t.right  configure -state normal");

  eval(".updateUnitInfo");
  eval("update idletasks");
  return TCL_OK;
}

flag chooseUnitSet(void) {
  if (!Net) return loadExampleDisplay();

  if (Net->unitDisplaySet == TRAIN_SET && !Net->trainingSet)
    Net->unitDisplaySet = NO_SET;
  else if (Net->unitDisplaySet == TEST_SET && !Net->testingSet)
    Net->unitDisplaySet = NO_SET;
  if (Net->unitDisplaySet == NO_SET) {
    if (Net->trainingSet)
      Net->unitDisplaySet = TRAIN_SET;
    else if (Net->testingSet)
      Net->unitDisplaySet = TEST_SET;
  }

  if (Net->unitDisplaySet == NO_SET)
    eval(".unit.l.label configure -text {}");
  else
    eval(".unit.l.label configure -text $_unitSetLabel(%d)",
	 Net->unitDisplaySet);
  return loadExampleDisplay();
}

flag loadExampleDisplay(void) {
  int i;
  ExampleSet S = NULL;

  if (!UnitUp) return TCL_OK;
  eval(".unit.l.list delete 0 end");
  if (!Net) return TCL_OK;
  if (Net->unitDisplaySet == TRAIN_SET)
    S = Net->trainingSet;
  else if (Net->unitDisplaySet == TEST_SET)
    S = Net->testingSet;
  else S = NULL;
  if (!S) return TCL_OK;

  for (i = 0; i < S->numExamples; i++) {
    if (S->example[i]->name)
      eval(".unit.l.list insert end {%s}", S->example[i]->name);
    else eval(".unit.l.list insert end %d", S->example[i]->num);
  }
  return TCL_OK;
}

/*****************************************************************************/

flag setLinkTemp(real val) {
  if (Net) {
    Net->linkTemp = POW(2.0, val);
    updateLinkDisplay();
  }
  return TCL_OK;
}

flag setLinkValue(int val) {
  /* Set the label */
  if (eval("global _valueLabel; .link.c.label configure "
	   "-text $_valueLabel(%d)", val))
    return TCL_ERROR;
  Net->linkDisplayValue = val;
  Tcl_UpdateLinkedVar(Interp, ".linkDisplayValue");
  return updateLinkDisplay();
}

flag drawLinksLater(void) {
  if (!LinksNeedRedraw) {
    LinksNeedRedraw = TRUE;
    Tcl_DoWhenIdle((Tcl_IdleProc *) drawLinks, NULL);
  }
  return TCL_OK;
}

flag drawLinks(ClientData junk) {
  int l, topmargin, leftmargin, fontsize, longestname = 0,
    inPos, outPos, Space, x, y;
  flag halted;
  Unit U, sU, P, sP;

  if (!LinksNeedRedraw) return TCL_OK;
  LinksNeedRedraw = FALSE;
  if (!LinkUp) return TCL_OK;
  if (!Net) return Tcl_Eval(Interp, ".link.c.c delete all");
  fillPalette(Net->linkPalette);
  if (Net->linkCellSize < 2 || Net->linkCellSize > 100)
    return warning("drawLinks: linkCellSize (%d) must be in the range [2,100]",
		   Net->linkCellSize);
  if (Net->linkCellSpacing < 0 || Net->linkCellSpacing > 100)
    return warning("drawLinks: linkCellSpacing (%d) must be in the range "
		   "[0,100]", Net->linkCellSpacing);
  /* This should make another call to drawLinks */
  if (Tcl_Eval(Interp, ".link.c.c delete all"))
    return TCL_ERROR;

  startTask(BUILDING_LINKS);

  eval(".buildLinkGroupMenus");
  /* First go through and mark inPositions and mark groups that will have
     outgoing links to plot */
  FOR_EACH_GROUP(G->outPosition = G->inPosition = -1);

  inPos = 0;
  FOR_EACH_GROUP_BACK({
    if (!G->showIncoming || !G->numIncoming) continue;
    FOR_EVERY_UNIT(G, {
      FOR_EACH_BLOCK(U, {
	for (P = B->unit, sP = P + B->numUnits; P < sP; P++) {
	  if (P->group->showOutgoing) {
	    if (P->group->outPosition == -1) {
	      P->group->outPosition = 0;
	      if (strlen(P->group->name) > longestname)
		longestname = strlen(P->group->name);
	    }
	    if (G->inPosition == -1) {
	      G->inPosition = inPos;
	      inPos += G->numUnits + 1;
	    }
	  }
	}
      });
    });
  });

  outPos = 0;
  FOR_EACH_GROUP_BACK({
    if (G->outPosition != -1) {
      G->outPosition = outPos;
      outPos += G->numUnits + 1;
    }
  });

  fontsize = imax(imin(Net->linkCellSize + 2, 18), 8);
  leftmargin = (fontsize * (longestname + 5)) * .7;
  topmargin = fontsize * 5;
  Space = Net->linkCellSize + Net->linkCellSpacing;
  eval("catch {font delete .link.font}; font create .link.font "
       "-family helvetica -size -%d -weight bold", fontsize);
  eval(".link.c.c configure -scrollregion {0 0 %d %d}\n",
       leftmargin + inPos * Space, topmargin + outPos * Space);
  eval(".link.c.c xview moveto 0; .link.c.c yview moveto 0");

  halted = FALSE;
  FOR_EACH_GROUP({
    if (halted) break;
    if (G->outPosition != -1) {
      eval(".link.c.c create text %d %d -text {%s} -anchor e "
	   "-font .link.font", leftmargin - fontsize * 3,
	   topmargin + (int)(((real) G->numUnits / 2 + G->outPosition) *
			     Space), G->name);
      for (U = G->unit, sU = U + G->numUnits; U < sU; U += 5)
	eval(".link.c.c create text %d %d -text %d -anchor ne "
	     "-font .link.font", leftmargin - 4,
	     topmargin + (G->outPosition + U->num) * Space, U->num);
    }
    if (G->inPosition != -1) {
      eval(".link.c.c create text %d %d -text {%s} -anchor s "
	   "-font .link.font", leftmargin + (int)
	   (((real) G->numUnits / 2 + G->inPosition) * Space),
	   topmargin - (int) (fontsize * 2), G->name);
      FOR_EVERY_UNIT(G, {
	l = 0;
	FOR_EACH_BLOCK(U, {
	  for (P = B->unit, sP = P + B->numUnits; P < sP; P++, l++) {
	    if (P->group->outPosition != -1) {
	      x = leftmargin + (G->inPosition + U->num) * Space;
	      y = topmargin + (P->group->outPosition + P->num) * Space;
	      if (eval("set .r [.link.c.c create canvrect %d %d %d %d  "
		       "%d %d %d -fill %s -tag {cr %d:%d:%d}];"
		       ".setLinkBindings %d:%d:%d %d %d %d %d %d",
		       x, y, x + Net->linkCellSize, y + Net->linkCellSize,
		       G->num, U->num, l, NaNColor, G->num, U->num, l,
		       G->num, U->num, l, P->group->num, P->num,
		       U->group->num, U->num, l) != TCL_OK) {
		stopTask(BUILDING_LINKS);
		return TCL_ERROR;
	      }
	    }
	  }
	});
	if ((U->num % 5) == 0) {
	  eval(".link.c.c create text %d %d -text %d -anchor sw "
	       "-font .link.font", leftmargin + (G->inPosition + U->num)
	       * Space, topmargin - 2, U->num);
	}
	if ((halted = smartUpdate(FALSE))) break;
      });
    }
  });

  stopTask(BUILDING_LINKS);
  setLinkValue(Net->linkDisplayValue);
  return updateLinkDisplay();
}

flag updateLinkDisplay(void) {
  int value = Net->linkDisplayValue, i, si, n = 0;
  Link L; Link2 M;
  real v, av;
  real mean = 0.0;
  real meanAbs = 0.0;
  real var = 0.0;
  /*  real varAbs = 0.0;*/
  real max = 0.0;
  real maxAbs = 0.0;
  real avgRange = 0.0;

  if (!LinkUp || LinksNeedRedraw) return TCL_OK;
  if (unsafeToRun("", BUILDING_LINKS)) return TCL_OK;

  if (eval(".link.c.c delete hint") != TCL_OK)
    return TCL_ERROR;
  if (eval(".link.c.c itemconfigure cr -stipple {}") != TCL_OK)
    return TCL_ERROR;
  FOR_EACH_GROUP({
    if (G->inPosition != -1) {
      FOR_EVERY_UNIT(G, {
	L = U->incoming;
	M = U->incoming2;
	i = 0;
	FOR_EACH_BLOCK(U, {
	  if (B->unit[0].group->outPosition != -1) {
	    for (si = i + B->numUnits; i < si; i++) {
	      switch (value) {
	      case UV_LINK_WEIGHTS: v = L[i].weight; break;
	      case UV_LINK_DERIVS:  v = L[i].deriv;  break;
	      case UV_LINK_DELTAS:  v = M[i].lastWeightDelta; break;
	      default: return warning("updateLinkDisplay called with bad "
				      "Net->linkDisplayValue");
	      }
	      mean += v;
	      av = (real) ABS(v);
	      meanAbs += av;
	      if (av > maxAbs) {
		maxAbs = av;
		max = v;
	      }
	      n++;
	    }
	  } else i += B->numUnits;
	});
      });
    }
  });

  if (n) {
    mean /= n;
    meanAbs /= n;
  }

  FOR_EACH_GROUP({
    if (G->inPosition != -1) {
      FOR_EVERY_UNIT(G, {
	L = U->incoming;
	M = U->incoming2;
	i = 0;
	FOR_EACH_BLOCK(U, {
	  if (B->unit[0].group->outPosition != -1) {
	    for (si = i + B->numUnits; i < si; i++) {
	      switch (value) {
	      case UV_LINK_WEIGHTS: v = L[i].weight; break;
	      case UV_LINK_DERIVS:  v = L[i].deriv;  break;
	      case UV_LINK_DELTAS:  v = M[i].lastWeightDelta; break;
	      default: return warning("updateLinkDisplay called with bad "
				      "Net->linkDisplayValue");
	      }
	      av = (real) ABS(v);
	      avgRange += ABS(v - mean);
	      var += SQUARE(v - mean);
	      /* varAbs += SQUARE(v - meanAbs);*/
	    }
	  } else i += B->numUnits;
	});
      });
    }
  });

  if (n > 1) {
    avgRange /= n;
    var /= (n - 1);
    /* varAbs /= (n - 1);*/
  } else {
    var = 0.0;
    /* varAbs = 0.0; */
  }

  eval("set .linkMean %f", mean);
  eval("set .linkVar %f", var);
  eval("set .linkMAbs %f", meanAbs);
  eval("set .linkMDist %f", avgRange);
  eval("set .linkMax %f", max);

  eval(".updateLinkInfo");
  eval("update idletasks");
  return TCL_OK;
}

flag linksChanged(void) {
  if (updateUnitDisplay()) return TCL_ERROR;
  return drawLinksLater();
}

flag buildLinkGroupMenus(void) {
  return eval(".buildLinkGroupMenus");
}

void updateDisplays(mask update) {
  if (!Net) return;
  if (UnitUp && Net->unitUpdates & update) updateUnitDisplay();
  if (LinkUp && Net->linkUpdates & update) updateLinkDisplay();
  updateGraphs(update);
}
