/*
 * This code was adapted from the tkRectOval.c file, version 8.3.4.
 *
 * Copyright (c) 1991-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 */

#include "system.h"
#include "util.h"
#include "type.h"
#include "network.h"
#include "connect.h"
#include "display.h"
#define _TCLINT /* To prevent tclInt.h from being read. */

#include <stdio.h>
#include "tk.h"
#include "tkInt.h"
#include "tkPort.h"
#include "tkCanvas.h"
#include "tkIntXlibDecls.h"
#include "tkIntDecls.h"


/*
 * The structure below defines the record for each rectangle/oval item.
 */

typedef struct CanvRectItem  {
  /* This first part is from a RectOval and cannot change */
    Tk_Item header;		/* Generic stuff that's the same for all
				 * types.  MUST BE FIRST IN STRUCTURE. */
    Tk_Outline outline;		/* Outline structure */
    double bbox[4];		/* Coordinates of bounding box for rectangle
				 * or oval (x1, y1, x2, y2).  Item includes
				 * x1 and x2 but not y1 and y2. */
    Tk_TSOffset tsoffset;
    XColor *fillColor;		/* Color for filling rectangle/oval. */
    XColor *activeFillColor;	/* Color for filling rectangle/oval if state is active. */
    XColor *disabledFillColor;	/* Color for filling rectangle/oval if state is disabled. */
    Pixmap fillStipple;		/* Stipple bitmap for filling item. */
    Pixmap activeFillStipple;	/* Stipple bitmap for filling item if state is active. */
    Pixmap disabledFillStipple;	/* Stipple bitmap for filling item if state is disabled. */
    GC fillGC;			/* Graphics context for filling item. */
  /* Now the CanvRect fields */
  int group;
  int unit;
  int link;  /* If this is -1 it will use the unit value, -2 for targets */
} CanvRectItem;

/*
 * Information used for parsing configuration specs:
 */

static Tk_CustomOption stateOption = {
    (Tk_OptionParseProc *) TkStateParseProc,
    TkStatePrintProc, (ClientData) 2
};
static Tk_CustomOption tagsOption = {
    (Tk_OptionParseProc *) Tk_CanvasTagsParseProc,
    Tk_CanvasTagsPrintProc, (ClientData) NULL
};
static Tk_CustomOption dashOption = {
    (Tk_OptionParseProc *) TkCanvasDashParseProc,
    TkCanvasDashPrintProc, (ClientData) NULL
};
static Tk_CustomOption offsetOption = {
    (Tk_OptionParseProc *) TkOffsetParseProc,
    TkOffsetPrintProc, (ClientData) TK_OFFSET_RELATIVE
};
static Tk_CustomOption pixelOption = {
    (Tk_OptionParseProc *) TkPixelParseProc,
    TkPixelPrintProc, (ClientData) NULL
};

static const Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_CUSTOM, "-activedash", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(CanvRectItem, outline.activeDash),
	TK_CONFIG_NULL_OK, &dashOption},
    {TK_CONFIG_COLOR, "-activefill", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(CanvRectItem, activeFillColor),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_COLOR, "-activeoutline", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(CanvRectItem, outline.activeColor),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_BITMAP, "-activeoutlinestipple", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(CanvRectItem, outline.activeStipple),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_BITMAP, "-activestipple", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(CanvRectItem, activeFillStipple),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-activewidth", (char *) NULL, (char *) NULL,
	"0.0", Tk_Offset(CanvRectItem, outline.activeWidth),
	TK_CONFIG_DONT_SET_DEFAULT, &pixelOption},
    {TK_CONFIG_CUSTOM, "-dash", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(CanvRectItem, outline.dash),
	TK_CONFIG_NULL_OK, &dashOption},
    {TK_CONFIG_PIXELS, "-dashoffset", (char *) NULL, (char *) NULL,
	"0", Tk_Offset(CanvRectItem, outline.offset),
	TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_CUSTOM, "-disableddash", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(CanvRectItem, outline.disabledDash),
	TK_CONFIG_NULL_OK, &dashOption},
    {TK_CONFIG_COLOR, "-disabledfill", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(CanvRectItem, disabledFillColor),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_COLOR, "-disabledoutline", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(CanvRectItem, outline.disabledColor),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_BITMAP, "-disabledoutlinestipple", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(CanvRectItem, outline.disabledStipple),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_BITMAP, "-disabledstipple", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(CanvRectItem, disabledFillStipple),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_PIXELS, "-disabledwidth", (char *) NULL, (char *) NULL,
	"0.0", Tk_Offset(CanvRectItem, outline.disabledWidth),
	TK_CONFIG_DONT_SET_DEFAULT, &pixelOption},
    {TK_CONFIG_COLOR, "-fill", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(CanvRectItem, fillColor), TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-offset", (char *) NULL, (char *) NULL,
	"0,0", Tk_Offset(CanvRectItem, tsoffset),
	TK_CONFIG_DONT_SET_DEFAULT, &offsetOption},
    {TK_CONFIG_COLOR, "-outline", (char *) NULL, (char *) NULL,
	"black", Tk_Offset(CanvRectItem, outline.color), TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-outlineoffset", (char *) NULL, (char *) NULL,
	"0,0", Tk_Offset(CanvRectItem, outline.tsoffset),
	TK_CONFIG_DONT_SET_DEFAULT, &offsetOption},
    {TK_CONFIG_BITMAP, "-outlinestipple", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(CanvRectItem, outline.stipple),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-state", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(Tk_Item, state),TK_CONFIG_NULL_OK,
	&stateOption},
    {TK_CONFIG_BITMAP, "-stipple", (char *) NULL, (char *) NULL,
	(char *) NULL, Tk_Offset(CanvRectItem, fillStipple), TK_CONFIG_NULL_OK},
    {TK_CONFIG_CUSTOM, "-tags", (char *) NULL, (char *) NULL,
	(char *) NULL, 0, TK_CONFIG_NULL_OK, &tagsOption},
    {TK_CONFIG_CUSTOM, "-width", (char *) NULL, (char *) NULL,
	"1.0", Tk_Offset(CanvRectItem, outline.width),
	TK_CONFIG_DONT_SET_DEFAULT, &pixelOption},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

/*
 * Prototypes for procedures defined in this file:
 */

static void		ComputeRectOvalBbox _ANSI_ARGS_((Tk_Canvas canvas,
			    CanvRectItem *canvRectPtr));
static int		ConfigureCanvRect _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int objc,
			    Tcl_Obj *CONST objv[], int flags));
static int		CreateCanvRect _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Canvas canvas, struct Tk_Item *itemPtr,
			    int objc, Tcl_Obj *CONST objv[]));
static void		DeleteCanvRect _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, Display *display));
static void		DisplayRectOval _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, Display *display, Drawable dst,
			    int x, int y, int width, int height));
static int		RectOvalCoords _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int objc,
			    Tcl_Obj *CONST objv[]));
static int		RectOvalToPostscript _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Canvas canvas, Tk_Item *itemPtr, int prepass));
static int		RectToArea _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, double *areaPtr));
static double		RectToPoint _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, double *pointPtr));
static void		ScaleRectOval _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, double originX, double originY,
			    double scaleX, double scaleY));
static void		TranslateRectOval _ANSI_ARGS_((Tk_Canvas canvas,
			    Tk_Item *itemPtr, double deltaX, double deltaY));

/*
 * The structures below defines the rectangle and oval item types
 * by means of procedures that can be invoked by generic item code.
 */

Tk_ItemType tkCanvRectType = {
    "canvrect",			        /* name */
    sizeof(CanvRectItem),		/* itemSize */
    CreateCanvRect,			/* createProc */
    configSpecs,			/* configSpecs */
    ConfigureCanvRect,			/* configureProc */
    RectOvalCoords,			/* coordProc */
    DeleteCanvRect,			/* deleteProc */
    DisplayRectOval,			/* displayProc */
    TK_CONFIG_OBJS,			/* flags */
    RectToPoint,			/* pointProc */
    RectToArea,				/* areaProc */
    RectOvalToPostscript,		/* postscriptProc */
    ScaleRectOval,			/* scaleProc */
    TranslateRectOval,			/* translateProc */
    (Tk_ItemIndexProc *) NULL,		/* indexProc */
    (Tk_ItemCursorProc *) NULL,		/* icursorProc */
    (Tk_ItemSelectionProc *) NULL,	/* selectionProc */
    (Tk_ItemInsertProc *) NULL,		/* insertProc */
    (Tk_ItemDCharsProc *) NULL,		/* dTextProc */
    (Tk_ItemType *) NULL,		/* nextPtr */
};

/*
 *--------------------------------------------------------------
 *
 * CreateCanvRect --
 *
 *	This procedure is invoked to create a new rectangle
 *	or oval item in a canvas.
 *
 * Results:
 *	A standard Tcl return value.  If an error occurred in
 *	creating the item, then an error message is left in
 *	the interp's result;  in this case itemPtr is left uninitialized,
 *	so it can be safely freed by the caller.
 *
 * Side effects:
 *	A new rectangle or oval item is created.
 *
 *--------------------------------------------------------------
 */

static int
CreateCanvRect(interp, canvas, itemPtr, objc, objv)
    Tcl_Interp *interp;			/* For error reporting. */
    Tk_Canvas canvas;			/* Canvas to hold new item. */
    Tk_Item *itemPtr;			/* Record to hold new item;  header
					 * has been initialized by caller. */
    int objc;				/* Number of arguments in objv. */
    Tcl_Obj *CONST objv[];		/* Arguments describing rectangle. */
{
    CanvRectItem *canvRectPtr = (CanvRectItem *) itemPtr;
    int i = 7;

    if (objc == 1) {
	i = 1;
    } else if (objc > 1) {
	char *arg = Tcl_GetString(objv[1]);
	if ((arg[0] == '-') && (arg[1] >= 'a') && (arg[1] <= 'z')) {
	    i = 1;
	}
    }

    if (objc < i) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		Tk_PathName(Tk_CanvasTkwin(canvas)), " create ",
		itemPtr->typePtr->name, " x1 y1 x2 y2 group unit link ?options?\"",
		(char *) NULL);
	return TCL_ERROR;
    }

    /*
     * Carry out initialization that is needed in order to clean
     * up after errors during the the remainder of this procedure.
     */

    Tk_CreateOutline(&(canvRectPtr->outline));
    canvRectPtr->tsoffset.flags = 0;
    canvRectPtr->tsoffset.xoffset = 0;
    canvRectPtr->tsoffset.yoffset = 0;
    canvRectPtr->fillColor = NULL;
    canvRectPtr->activeFillColor = NULL;
    canvRectPtr->disabledFillColor = NULL;
    canvRectPtr->fillStipple = None;
    canvRectPtr->activeFillStipple = None;
    canvRectPtr->disabledFillStipple = None;
    canvRectPtr->fillGC = None;

    /*
     * Process the arguments to fill in the item record.
     */

    if ((RectOvalCoords(interp, canvas, itemPtr, 4, objv) != TCL_OK)) {
	goto error;
    }
    if (Tcl_GetIntFromObj(interp, objv[4], &(canvRectPtr->group)) != TCL_OK)
      return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[5], &(canvRectPtr->unit)) != TCL_OK)
      return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[6], &(canvRectPtr->link)) != TCL_OK)
      return TCL_ERROR;

    if (ConfigureCanvRect(interp, canvas, itemPtr, objc-i, objv+i, 0)
	    == TCL_OK) {
	return TCL_OK;
    }

    error:
    DeleteCanvRect(canvas, itemPtr, Tk_Display(Tk_CanvasTkwin(canvas)));
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * RectOvalCoords --
 *
 *	This procedure is invoked to process the "coords" widget
 *	command on rectangles and ovals.  See the user documentation
 *	for details on what it does.
 *
 * Results:
 *	Returns TCL_OK or TCL_ERROR, and sets the interp's result.
 *
 * Side effects:
 *	The coordinates for the given item may be changed.
 *
 *--------------------------------------------------------------
 */

static int
RectOvalCoords(interp, canvas, itemPtr, objc, objv)
    Tcl_Interp *interp;			/* Used for error reporting. */
    Tk_Canvas canvas;			/* Canvas containing item. */
    Tk_Item *itemPtr;			/* Item whose coordinates are to be
					 * read or modified. */
    int objc;				/* Number of coordinates supplied in
					 * objv. */
    Tcl_Obj *CONST objv[];		/* Array of coordinates: x1, y1,
					 * x2, y2, ... */
{
    CanvRectItem *canvRectPtr = (CanvRectItem *) itemPtr;

    if (objc == 0) {
	Tcl_Obj *obj = Tcl_NewObj();
	Tcl_Obj *subobj = Tcl_NewDoubleObj(canvRectPtr->bbox[0]);
	Tcl_ListObjAppendElement(interp, obj, subobj);
	subobj = Tcl_NewDoubleObj(canvRectPtr->bbox[1]);
	Tcl_ListObjAppendElement(interp, obj, subobj);
	subobj = Tcl_NewDoubleObj(canvRectPtr->bbox[2]);
	Tcl_ListObjAppendElement(interp, obj, subobj);
	subobj = Tcl_NewDoubleObj(canvRectPtr->bbox[3]);
	Tcl_ListObjAppendElement(interp, obj, subobj);
	Tcl_SetObjResult(interp, obj);
    } else if ((objc == 1)||(objc == 4)) {
 	if (objc==1) {
	    if (Tcl_ListObjGetElements(interp, objv[0], &objc,
		    (Tcl_Obj ***) &objv) != TCL_OK) {
		return TCL_ERROR;
	    } else if (objc != 4) {
		char buf[64 + TCL_INTEGER_SPACE];

		sprintf(buf, "wrong # coordinates: expected 0 or 4, got %d", objc);
		Tcl_SetResult(interp, buf, TCL_VOLATILE);
		return TCL_ERROR;
	    }
	}
	if ((Tk_CanvasGetCoordFromObj(interp, canvas, objv[0],
 		    &canvRectPtr->bbox[0]) != TCL_OK)
		|| (Tk_CanvasGetCoordFromObj(interp, canvas, objv[1],
		    &canvRectPtr->bbox[1]) != TCL_OK)
		|| (Tk_CanvasGetCoordFromObj(interp, canvas, objv[2],
			&canvRectPtr->bbox[2]) != TCL_OK)
		|| (Tk_CanvasGetCoordFromObj(interp, canvas, objv[3],
			&canvRectPtr->bbox[3]) != TCL_OK)) {
	    return TCL_ERROR;
	}
	ComputeRectOvalBbox(canvas, canvRectPtr);
    } else {
	char buf[64 + TCL_INTEGER_SPACE];

	sprintf(buf, "wrong # coordinates: expected 0 or 4, got %d", objc);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ConfigureRectOval --
 *
 *	This procedure is invoked to configure various aspects
 *	of a rectangle or oval item, such as its border and
 *	background colors.
 *
 * Results:
 *	A standard Tcl result code.  If an error occurs, then
 *	an error message is left in the interp's result.
 *
 * Side effects:
 *	Configuration information, such as colors and stipple
 *	patterns, may be set for itemPtr.
 *
 *--------------------------------------------------------------
 */

static int
ConfigureRectOval(interp, canvas, itemPtr, objc, objv, flags)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tk_Canvas canvas;		/* Canvas containing itemPtr. */
    Tk_Item *itemPtr;		/* Rectangle item to reconfigure. */
    int objc;			/* Number of elements in objv.  */
    Tcl_Obj *CONST objv[];	/* Arguments describing things to configure. */
    int flags;			/* Flags to pass to Tk_ConfigureWidget. */
{
    CanvRectItem *canvRectPtr = (CanvRectItem *) itemPtr;
    XGCValues gcValues;
    GC newGC;
    unsigned long mask;
    Tk_Window tkwin;
    Tk_TSOffset *tsoffset;
    XColor *color;
    Pixmap stipple;
    Tk_State state;

    tkwin = Tk_CanvasTkwin(canvas);

    if (Tk_ConfigureWidget(interp, tkwin, configSpecs, objc, (CONST84 char **) objv,
	    (char *) canvRectPtr, flags|TK_CONFIG_OBJS) != TCL_OK) {
	return TCL_ERROR;
    }
    state = itemPtr->state;

    /*
     * A few of the options require additional processing, such as
     * graphics contexts.
     */

    if (canvRectPtr->outline.activeWidth > canvRectPtr->outline.width ||
	    canvRectPtr->outline.activeDash.number != 0 ||
	    canvRectPtr->outline.activeColor != NULL ||
	    canvRectPtr->outline.activeStipple != None ||
	    canvRectPtr->activeFillColor != NULL ||
	    canvRectPtr->activeFillStipple != None) {
	itemPtr->redraw_flags |= TK_ITEM_STATE_DEPENDANT;
    } else {
	itemPtr->redraw_flags &= ~TK_ITEM_STATE_DEPENDANT;
    }

    tsoffset = &canvRectPtr->outline.tsoffset;
    flags = tsoffset->flags;
    if (flags & TK_OFFSET_LEFT) {
	tsoffset->xoffset = (int) (canvRectPtr->bbox[0] + 0.5);
    } else if (flags & TK_OFFSET_CENTER) {
	tsoffset->xoffset = (int) ((canvRectPtr->bbox[0]+canvRectPtr->bbox[2]+1)/2);
    } else if (flags & TK_OFFSET_RIGHT) {
	tsoffset->xoffset = (int) (canvRectPtr->bbox[2] + 0.5);
    }
    if (flags & TK_OFFSET_TOP) {
	tsoffset->yoffset = (int) (canvRectPtr->bbox[1] + 0.5);
    } else if (flags & TK_OFFSET_MIDDLE) {
	tsoffset->yoffset = (int) ((canvRectPtr->bbox[1]+canvRectPtr->bbox[3]+1)/2);
    } else if (flags & TK_OFFSET_BOTTOM) {
	tsoffset->yoffset = (int) (canvRectPtr->bbox[2] + 0.5);
    }

    /*
     * Configure the outline graphics context.  If mask is non-zero,
     * the gc has changed and must be reallocated, provided that the
     * new settings specify a valid outline (non-zero width and non-NULL
     * color)
     */

    mask = Tk_ConfigOutlineGC(&gcValues, canvas, itemPtr,
	     &(canvRectPtr->outline));
    if (mask && \
	    canvRectPtr->outline.width != 0 && \
	    canvRectPtr->outline.color != NULL) {
	gcValues.cap_style = CapProjecting;
	mask |= GCCapStyle;
	newGC = Tk_GetGC(tkwin, mask, &gcValues);
    } else {
	newGC = None;
    }
    if (canvRectPtr->outline.gc != None) {
	Tk_FreeGC(Tk_Display(tkwin), canvRectPtr->outline.gc);
    }
    canvRectPtr->outline.gc = newGC;

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }
    if (state==TK_STATE_HIDDEN) {
	ComputeRectOvalBbox(canvas, canvRectPtr);
	return TCL_OK;
    }

    color = canvRectPtr->fillColor;
    stipple = canvRectPtr->fillStipple;
    if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (canvRectPtr->activeFillColor!=NULL) {
	    color = canvRectPtr->activeFillColor;
	}
	if (canvRectPtr->activeFillStipple!=None) {
	    stipple = canvRectPtr->activeFillStipple;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (canvRectPtr->disabledFillColor!=NULL) {
	    color = canvRectPtr->disabledFillColor;
	}
	if (canvRectPtr->disabledFillStipple!=None) {
	    stipple = canvRectPtr->disabledFillStipple;
	}
    }

    if (color == NULL) {
	newGC = None;
    } else {
	gcValues.foreground = color->pixel;
	if (stipple != None) {
	    gcValues.stipple = stipple;
	    gcValues.fill_style = FillStippled;
	    mask = GCForeground|GCStipple|GCFillStyle;
	} else {
	    mask = GCForeground;
	}
	newGC = Tk_GetGC(tkwin, mask, &gcValues);
    }
    if (canvRectPtr->fillGC != None) {
	Tk_FreeGC(Tk_Display(tkwin), canvRectPtr->fillGC);
    }
    canvRectPtr->fillGC = newGC;

    tsoffset = &canvRectPtr->tsoffset;
    flags = tsoffset->flags;
    if (flags & TK_OFFSET_LEFT) {
	tsoffset->xoffset = (int) (canvRectPtr->bbox[0] + 0.5);
    } else if (flags & TK_OFFSET_CENTER) {
	tsoffset->xoffset = (int) ((canvRectPtr->bbox[0]+canvRectPtr->bbox[2]+1)/2);
    } else if (flags & TK_OFFSET_RIGHT) {
	tsoffset->xoffset = (int) (canvRectPtr->bbox[2] + 0.5);
    }
    if (flags & TK_OFFSET_TOP) {
	tsoffset->yoffset = (int) (canvRectPtr->bbox[1] + 0.5);
    } else if (flags & TK_OFFSET_MIDDLE) {
	tsoffset->yoffset = (int) ((canvRectPtr->bbox[1]+canvRectPtr->bbox[3]+1)/2);
    } else if (flags & TK_OFFSET_BOTTOM) {
	tsoffset->yoffset = (int) (canvRectPtr->bbox[3] + 0.5);
    }

    ComputeRectOvalBbox(canvas, canvRectPtr);

    return TCL_OK;
}

void drawHinton(CanvRectItem *canvRectPtr, real value, real temp, char *win,
		int group, int unit, int link) {
  int x = rint(canvRectPtr->bbox[0]);
  int y = rint(canvRectPtr->bbox[1]);
  int a, i, p, w, h, l, s = rint(canvRectPtr->bbox[2]) - x - 1;
  char *color;
  if (canvRectPtr->link == -2) {
    s++;
    eval(".drawHinton %s rect %d %d %d %d -fill %s -outline %s -tag {hint %d:%d:%d}", win,
	 x, y, x + s, y + s, OutlineColor[HINTON][0], OutlineColor[HINTON][0],
	 group, unit, link);
    x--; y--; s++;
  }

  value = normColorValue(value, temp);
  if (isNaN(value)) return;

  /* Scale to range [0,1] and deal with negative values. */
  if (value < 0.5) {
    value = (0.5 - value) * 2;
    color = "black";
  } else {
    value = (value - 0.5) * 2;
    color = "white";
  }
  /* Number of points covered. */
  p = (int) rint(value * s * s);
  if (p == 0) return;
  for (i = 1; i * i < p; i++);
  a = s / 2 - (i + 1) / 2 + 2;
  if (p == i * i) {
    w = h = i;
  } else if (p < i * (i - 1)) {
    w = h = i - 1;
    if ((l = p - (w * h)) > 0) {
      if (i % 2 == 0) {
	eval(".drawHinton %s line %d %d %d %d -fill %s -tag {hint %d:%d:%d}",
	     win, x + a + w, y + s - a + 2, x + a + w,
	     y + s - a + 2 - l, color, group, unit, link);
      } else {
	a++;
	eval(".drawHinton %s line %d %d %d %d -fill %s -tag {hint %d:%d:%d}",
	     win, x + a - 1, y + s + 2 - a - h, x + a - 1,
	     y + s + 2 -a - h + l, color, group, unit, link);
      }
    }
  } else {
    w = i;
    h = i - 1;
    if ((l = p - (w * h)) > 0) {
      if (i % 2 == 0) {
	eval(".drawHinton %s line %d %d %d %d -fill %s -tag {hint %d:%d:%d}",
	     win, x + a + w, y + s - a - h + 1, x + a + w - l,
	     y + s + 1 - a - h, color, group, unit, link);
      } else {
	y--;
	eval(".drawHinton %s line %d %d %d %d -fill %s -tag {hint %d:%d:%d}",
	     win, x + a, y + s - a + 2, x + a + l,
	     y + s - a + 2, color, group, unit, link);
      }
    } else if (i % 2) y--;
  }
  if (h == 1)
    eval(".drawHinton %s line %d %d %d %d -fill %s -tag {hint %d:%d:%d}", win, x + a,
	 y + s - a + 1, x + a + w, y + s - a + 1, color, group, unit, link);
  else
    eval(".drawHinton %s rect %d %d %d %d -fill %s -outline %s -tag {hint %d:%d:%d}", win,
	 x + a, y + s - a + 1, x + a + w - 1,
	 y + s + 2 - a - h, color, color, group, unit, link);
}

static int
ConfigureCanvRect(interp, canvas, itemPtr, objc, objv, flags)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tk_Canvas canvas;		/* Canvas containing itemPtr. */
    Tk_Item *itemPtr;		/* Rectangle item to reconfigure. */
    int objc;			/* Number of elements in argv.  */
    Tcl_Obj *CONST objv[];	/* Arguments describing things to configure. */
    int flags;			/* Flags to pass to Tk_ConfigureWidget. */
{
  int i;
  Group G;
  Unit U;
  Block B;
  Link L;
  real value = 0.0;
  CanvRectItem *canvRectPtr = (CanvRectItem *) itemPtr;
  Tk_Window tkwin = Tk_CanvasTkwin(canvas);

  if (!Net) return warning("ConfigureCanvRect: no current network");

  if (canvRectPtr->group < 0 || canvRectPtr->group >= Net->numGroups)
    return warning("ConfigureCanvRect: group %d out of range",
		   canvRectPtr->group);
  G = Net->group[canvRectPtr->group];
  if (canvRectPtr->unit < 0 || canvRectPtr->unit >= G->numUnits)
    return warning("ConfigureCanvRect: unit %d out of range",
		   canvRectPtr->unit);
  U = G->unit + canvRectPtr->unit;
  if (canvRectPtr->link < 0) {
    switch (Net->unitDisplayValue) {
    case UV_OUT_TARG:
    case UV_OUTPUTS:
    case UV_TARGETS:
      if (!Net->currentExample) {
	value = NaN; break;}
      if (Net->unitDisplayValue == UV_TARGETS ||
	  (Net->unitDisplayValue == UV_OUT_TARG && canvRectPtr->link == -3)) {
	if (!U->targetHistory)
	  value = U->target;
	else if (ViewTick >= Net->ticksOnExample - Net->historyLength)
	  value = GET_HISTORY(U, targetHistory, HISTORY_INDEX(ViewTick));
	else value = NaN;
      } else {
	if (!U->outputHistory)
	  value = U->output;
	else if (ViewTick >= Net->ticksOnExample - Net->historyLength)
	  value = GET_HISTORY(U, outputHistory, HISTORY_INDEX(ViewTick));
	else value = NaN;
      }
      if (!isNaN(value)) {
	if (!isNaN(G->minOutput)) {
	  if (!isNaN(G->maxOutput)) {
	    /* Both min and max bounded */
	    value = (value - G->minOutput) / (G->maxOutput - G->minOutput);
	    value = INV_SIGMOID(value, 1.0);
	  } else {
	    /* Only min bounded */
	    value = LOG(value - G->minOutput);
	  }
	} else if (!isNaN(G->maxOutput)) {
	  /* Only max bounded (may never occur) */
	  value = -LOG(G->maxOutput - value);
	}
      }
      break;
    case UV_INPUTS:
      if (!Net->currentExample) {
	value = NaN; break;}
      if (!U->inputHistory)
	value = U->input;
      else if (ViewTick >= Net->ticksOnExample - Net->historyLength)
	value = GET_HISTORY(U, inputHistory, HISTORY_INDEX(ViewTick));
      else value = NaN;
      break;
    case UV_EXT_INPUTS:
      if (!Net->currentExample || ViewTick < Net->ticksOnExample - 1)
	value = NaN;
      else value = U->externalInput;
      break;
    case UV_OUTPUT_DERIVS:
      if (!Net->currentExample) {
	value = NaN; break;}
      if (!U->outputDerivHistory)
	value = U->outputDeriv;
      else if (ViewTick >= Net->ticksOnExample - Net->historyLength)
	value = GET_HISTORY(U, outputDerivHistory, HISTORY_INDEX(ViewTick));
      else value = NaN;
      break;
    case UV_INPUT_DERIVS:
      if (!Net->currentExample || ViewTick < Net->ticksOnExample - 1)
	value = NaN;
      else value = U->inputDeriv;
      break;
    case UV_GAIN:
      value = U->gain;
      break;
    case UV_LINK_WEIGHTS:
      if ((L = lookupLink(U, Net->unitDisplayUnit, NULL, ALL_LINKS)) ||
	  (L = lookupLink(Net->unitDisplayUnit, U, NULL, ALL_LINKS)))
	value = L->weight;
      else value = NaN; break;
    case UV_LINK_DERIVS:
      if ((L = lookupLink(U, Net->unitDisplayUnit, NULL, ALL_LINKS)) ||
	  (L = lookupLink(Net->unitDisplayUnit, U, NULL, ALL_LINKS)))
	value = L->deriv;
      else value = NaN; break;
    case UV_LINK_DELTAS:
      if ((L = lookupLink(U, Net->unitDisplayUnit, NULL, ALL_LINKS)))
	value = getLink2(Net->unitDisplayUnit, L)->lastWeightDelta;
      else if ((L = lookupLink(Net->unitDisplayUnit, U, NULL, ALL_LINKS)))
	value = getLink2(U, L)->lastWeightDelta;
      else value = NaN; break;
    default:
      fatalError("ConfigureCanvRect: bad Net->unitDisplayValue (%d)",
		 Net->unitDisplayValue);
    }
    if (Net->unitPalette == HINTON) {
      canvRectPtr->fillColor = canvRectPtr->outline.color =
	Tk_GetColor(interp, tkwin, OutlineColor[HINTON][0]);
      if (canvRectPtr->link != -2 || Net->unitDisplayValue == UV_OUT_TARG)
	drawHinton(canvRectPtr, value, Net->unitTemp, "unit",
		   U->group->num, U->num, 0);
    } else {
      canvRectPtr->fillColor = Tk_GetColor(interp, tkwin,
		Tk_GetUid(getColor(value, Net->unitTemp, Net->unitPalette)));
      if (canvRectPtr->link == -2)
	canvRectPtr->outline.color = Tk_GetColor(interp, tkwin,
                Tk_GetUid(getColor(value, Net->unitTemp, Net->unitPalette)));
    }
    /* Set the outline */
    if (canvRectPtr->link != -2) {
      if (U == Net->unitDisplayUnit &&
	  Net->unitDisplayValue >= UV_LINK_WEIGHTS) {
	canvRectPtr->outline.color = Tk_GetColor(interp, tkwin,
			OutlineColor[Net->unitPalette][1]);
      } else if (U == Net->unitLocked) {
        canvRectPtr->outline.color = Tk_GetColor(interp, tkwin,
			OutlineColor[Net->unitPalette][2]);
      } else {
	canvRectPtr->outline.color = Tk_GetColor(interp, tkwin,
			OutlineColor[Net->unitPalette][0]);
      }
    }
  } else {
    if (canvRectPtr->link < 0 || canvRectPtr->link >= U->numIncoming)
      return warning("ConfigureCanvRect: link %d out of range (%s %d)",
		     canvRectPtr->link, U->name, U->numIncoming);
    L = U->incoming + canvRectPtr->link;
    switch (Net->linkDisplayValue) {
    case UV_LINK_WEIGHTS:
      value = L->weight; break;
    case UV_LINK_DERIVS:
      value = L->deriv; break;
    case UV_LINK_DELTAS:
      value = getLink2(U, L)->lastWeightDelta; break;
    default:
      fatalError("ConfigureCanvRect: bad Net->linkDisplayValue (%d)",
		 Net->linkDisplayValue);
    }
    if (Net->linkPalette == HINTON) {
      canvRectPtr->fillColor = canvRectPtr->outline.color =
	Tk_GetColor(interp, tkwin, OutlineColor[HINTON][0]);
      drawHinton(canvRectPtr, value, Net->linkTemp, "link", U->group->num,
		 U->num, canvRectPtr->link);
    } else {
      canvRectPtr->fillColor =
	Tk_GetColor(interp, tkwin,
	   Tk_GetUid(getColor(value, Net->linkTemp, Net->linkPalette)));
    }
    for (i = 0, B = U->block; B->numUnits + i <= canvRectPtr->link;
	 i += B->numUnits, B++);
    if ((Net->type & FROZEN) || (G->type & FROZEN) ||
	(U->type & FROZEN) || (B->type & FROZEN))
      canvRectPtr->outline.color = Tk_GetColor(interp, tkwin,
		      OutlineColor[Net->linkPalette][1]);
    else if (U == Net->linkLockedUnit &&
             canvRectPtr->link == Net->linkLockedNum)
      canvRectPtr->outline.color = Tk_GetColor(interp, tkwin,
	   	      OutlineColor[Net->linkPalette][2]);
    else canvRectPtr->outline.color = Tk_GetColor(interp, tkwin,
	   	      OutlineColor[Net->linkPalette][0]);
  }

  return ConfigureRectOval(interp, canvas, itemPtr, objc, objv, flags);
}

void createCanvRectType(void) {
  Tk_CreateItemType(&tkCanvRectType);
}

/*
 *--------------------------------------------------------------
 *
 * DeleteCanvRect --
 *
 *	This procedure is called to clean up the data structure
 *	associated with a rectangle or oval item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resources associated with itemPtr are released.
 *
 *--------------------------------------------------------------
 */

static void
DeleteCanvRect(canvas, itemPtr, display)
    Tk_Canvas canvas;			/* Info about overall widget. */
    Tk_Item *itemPtr;			/* Item that is being deleted. */
    Display *display;			/* Display containing window for
					 * canvas. */
{
    CanvRectItem *canvRectPtr = (CanvRectItem *) itemPtr;

    Tk_DeleteOutline(display, &(canvRectPtr->outline));
    if (canvRectPtr->fillColor != NULL) {
	Tk_FreeColor(canvRectPtr->fillColor);
    }
    if (canvRectPtr->activeFillColor != NULL) {
	Tk_FreeColor(canvRectPtr->activeFillColor);
    }
    if (canvRectPtr->disabledFillColor != NULL) {
	Tk_FreeColor(canvRectPtr->disabledFillColor);
    }
    if (canvRectPtr->fillStipple != None) {
	Tk_FreeBitmap(display, canvRectPtr->fillStipple);
    }
    if (canvRectPtr->activeFillStipple != None) {
	Tk_FreeBitmap(display, canvRectPtr->activeFillStipple);
    }
    if (canvRectPtr->disabledFillStipple != None) {
	Tk_FreeBitmap(display, canvRectPtr->disabledFillStipple);
    }
    if (canvRectPtr->fillGC != None) {
	Tk_FreeGC(display, canvRectPtr->fillGC);
    }
}

/*
 *--------------------------------------------------------------
 *
 * ComputeRectOvalBbox --
 *
 *	This procedure is invoked to compute the bounding box of
 *	all the pixels that may be drawn as part of a rectangle
 *	or oval.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The fields x1, y1, x2, and y2 are updated in the header
 *	for itemPtr.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static void
ComputeRectOvalBbox(canvas, canvRectPtr)
    Tk_Canvas canvas;			/* Canvas that contains item. */
    CanvRectItem *canvRectPtr;		/* Item whose bbox is to be
					 * recomputed. */
{
    int bloat, tmp;
    double dtmp, width;
    Tk_State state = canvRectPtr->header.state;

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }

    width = canvRectPtr->outline.width;
    if (state==TK_STATE_HIDDEN) {
	canvRectPtr->header.x1 = canvRectPtr->header.y1 =
	canvRectPtr->header.x2 = canvRectPtr->header.y2 = -1;
	return;
    }
    if (((TkCanvas *)canvas)->currentItemPtr == (Tk_Item *)canvRectPtr) {
	if (canvRectPtr->outline.activeWidth>width) {
	    width = canvRectPtr->outline.activeWidth;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (canvRectPtr->outline.disabledWidth>0) {
	    width = canvRectPtr->outline.disabledWidth;
	}
    }

    /*
     * Make sure that the first coordinates are the lowest ones.
     */

    if (canvRectPtr->bbox[1] > canvRectPtr->bbox[3]) {
	double tmp;
	tmp = canvRectPtr->bbox[3];
	canvRectPtr->bbox[3] = canvRectPtr->bbox[1];
	canvRectPtr->bbox[1] = tmp;
    }
    if (canvRectPtr->bbox[0] > canvRectPtr->bbox[2]) {
	double tmp;
	tmp = canvRectPtr->bbox[2];
	canvRectPtr->bbox[2] = canvRectPtr->bbox[0];
	canvRectPtr->bbox[0] = tmp;
    }

    if (canvRectPtr->outline.gc == None) {
	/*
	 * The Win32 switch was added for 8.3 to solve a problem
	 * with ovals leaving traces on bottom and right of 1 pixel.
	 * This may not be the correct place to solve it, but it works.
	 */
#ifdef __WIN32__
	bloat = 1;
#else
	bloat = 0;
#endif
    } else {
	bloat = (int) (width+1)/2;
    }

    /*
     * Special note:  the rectangle is always drawn at least 1x1 in
     * size, so round up the upper coordinates to be at least 1 unit
     * greater than the lower ones.
     */

    tmp = (int) ((canvRectPtr->bbox[0] >= 0) ? canvRectPtr->bbox[0] + .5
	    : canvRectPtr->bbox[0] - .5);
    canvRectPtr->header.x1 = tmp - bloat;
    tmp = (int) ((canvRectPtr->bbox[1] >= 0) ? canvRectPtr->bbox[1] + .5
	    : canvRectPtr->bbox[1] - .5);
    canvRectPtr->header.y1 = tmp - bloat;
    dtmp = canvRectPtr->bbox[2];
    if (dtmp < (canvRectPtr->bbox[0] + 1)) {
	dtmp = canvRectPtr->bbox[0] + 1;
    }
    tmp = (int) ((dtmp >= 0) ? dtmp + .5 : dtmp - .5);
    canvRectPtr->header.x2 = tmp + bloat;
    dtmp = canvRectPtr->bbox[3];
    if (dtmp < (canvRectPtr->bbox[1] + 1)) {
	dtmp = canvRectPtr->bbox[1] + 1;
    }
    tmp = (int) ((dtmp >= 0) ? dtmp + .5 : dtmp - .5);
    canvRectPtr->header.y2 = tmp + bloat;
}

/*
 *--------------------------------------------------------------
 *
 * DisplayRectOval --
 *
 *	This procedure is invoked to draw a rectangle or oval
 *	item in a given drawable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	ItemPtr is drawn in drawable using the transformation
 *	information in canvas.
 *
 *--------------------------------------------------------------
 */

static void
DisplayRectOval(canvas, itemPtr, display, drawable, x, y, width, height)
    Tk_Canvas canvas;			/* Canvas that contains item. */
    Tk_Item *itemPtr;			/* Item to be displayed. */
    Display *display;			/* Display on which to draw item. */
    Drawable drawable;			/* Pixmap or window in which to draw
					 * item. */
    int x, y, width, height;		/* Describes region of canvas that
					 * must be redisplayed (not used). */
{
    CanvRectItem *canvRectPtr = (CanvRectItem *) itemPtr;
    short x1, y1, x2, y2;
    Pixmap fillStipple;
    Tk_State state = itemPtr->state;

    /*
     * Compute the screen coordinates of the bounding box for the item.
     * Make sure that the bbox is at least one pixel large, since some
     * X servers will die if it isn't.
     */

    Tk_CanvasDrawableCoords(canvas, canvRectPtr->bbox[0], canvRectPtr->bbox[1],
	    &x1, &y1);
    Tk_CanvasDrawableCoords(canvas, canvRectPtr->bbox[2], canvRectPtr->bbox[3],
	    &x2, &y2);
    if (x2 <= x1) {
	x2 = x1+1;
    }
    if (y2 <= y1) {
	y2 = y1+1;
    }

    /*
     * Display filled part first (if wanted), then outline.  If we're
     * stippling, then modify the stipple offset in the GC.  Be sure to
     * reset the offset when done, since the GC is supposed to be
     * read-only.
     */

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }
    fillStipple = canvRectPtr->fillStipple;
    if (((TkCanvas *)canvas)->currentItemPtr == (Tk_Item *)canvRectPtr) {
	if (canvRectPtr->activeFillStipple!=None) {
	    fillStipple = canvRectPtr->activeFillStipple;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (canvRectPtr->disabledFillStipple!=None) {
	    fillStipple = canvRectPtr->disabledFillStipple;
	}
    }

    if (canvRectPtr->fillGC != None) {
	if (fillStipple != None) {
	    Tk_TSOffset *tsoffset;
	    int w=0; int h=0;
	    tsoffset = &canvRectPtr->tsoffset;
	    if (tsoffset) {
		int flags = tsoffset->flags;
		if (flags & (TK_OFFSET_CENTER|TK_OFFSET_MIDDLE)) {
		    Tk_SizeOfBitmap(display, fillStipple, &w, &h);
		    if (flags & TK_OFFSET_CENTER) {
			w /= 2;
		    } else {
			w = 0;
		    }
		    if (flags & TK_OFFSET_MIDDLE) {
			h /= 2;
		    } else {
			h = 0;
		    }
		}
		tsoffset->xoffset -= w;
		tsoffset->yoffset -= h;
	    }
	    Tk_CanvasSetOffset(canvas, canvRectPtr->fillGC, tsoffset);
	    if (tsoffset) {
		tsoffset->xoffset += w;
		tsoffset->yoffset += h;
	    }
	}
	if (canvRectPtr->header.typePtr == &tkCanvRectType) {
	    XFillRectangle(display, drawable, canvRectPtr->fillGC,
		    x1, y1, (unsigned int) (x2-x1), (unsigned int) (y2-y1));
	} else {
	    XFillArc(display, drawable, canvRectPtr->fillGC,
		    x1, y1, (unsigned) (x2-x1), (unsigned) (y2-y1),
		    0, 360*64);
	}
	if (fillStipple != None) {
	    XSetTSOrigin(display, canvRectPtr->fillGC, 0, 0);
	}
    }
    if (canvRectPtr->outline.gc != None) {
	Tk_ChangeOutlineGC(canvas, itemPtr, &(canvRectPtr->outline));
	if (canvRectPtr->header.typePtr == &tkCanvRectType) {
	    XDrawRectangle(display, drawable, canvRectPtr->outline.gc,
		    x1, y1, (unsigned) (x2-x1), (unsigned) (y2-y1));
	} else {
	    XDrawArc(display, drawable, canvRectPtr->outline.gc,
		    x1, y1, (unsigned) (x2-x1), (unsigned) (y2-y1), 0, 360*64);
	}
	Tk_ResetOutlineGC(canvas, itemPtr, &(canvRectPtr->outline));
    }
}

/*
 *--------------------------------------------------------------
 *
 * RectToPoint --
 *
 *	Computes the distance from a given point to a given
 *	rectangle, in canvas units.
 *
 * Results:
 *	The return value is 0 if the point whose x and y coordinates
 *	are coordPtr[0] and coordPtr[1] is inside the rectangle.  If the
 *	point isn't inside the rectangle then the return value is the
 *	distance from the point to the rectangle.  If itemPtr is filled,
 *	then anywhere in the interior is considered "inside"; if
 *	itemPtr isn't filled, then "inside" means only the area
 *	occupied by the outline.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static double
RectToPoint(canvas, itemPtr, pointPtr)
    Tk_Canvas canvas;		/* Canvas containing item. */
    Tk_Item *itemPtr;		/* Item to check against point. */
    double *pointPtr;		/* Pointer to x and y coordinates. */
{
    CanvRectItem *rectPtr = (CanvRectItem *) itemPtr;
    double xDiff, yDiff, x1, y1, x2, y2, inc, tmp;
    double width;
    Tk_State state = itemPtr->state;

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }

    width = rectPtr->outline.width;
    if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (rectPtr->outline.activeWidth>width) {
	    width = rectPtr->outline.activeWidth;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (rectPtr->outline.disabledWidth>0) {
	    width = rectPtr->outline.disabledWidth;
	}
    }

    /*
     * Generate a new larger rectangle that includes the border
     * width, if there is one.
     */

    x1 = rectPtr->bbox[0];
    y1 = rectPtr->bbox[1];
    x2 = rectPtr->bbox[2];
    y2 = rectPtr->bbox[3];
    if (rectPtr->outline.gc != None) {
	inc = width/2.0;
	x1 -= inc;
	y1 -= inc;
	x2 += inc;
	y2 += inc;
    }

    /*
     * If the point is inside the rectangle, handle specially:
     * distance is 0 if rectangle is filled, otherwise compute
     * distance to nearest edge of rectangle and subtract width
     * of edge.
     */

    if ((pointPtr[0] >= x1) && (pointPtr[0] < x2)
		&& (pointPtr[1] >= y1) && (pointPtr[1] < y2)) {
	if ((rectPtr->fillGC != None) || (rectPtr->outline.gc == None)) {
	    return 0.0;
	}
	xDiff = pointPtr[0] - x1;
	tmp = x2 - pointPtr[0];
	if (tmp < xDiff) {
	    xDiff = tmp;
	}
	yDiff = pointPtr[1] - y1;
	tmp = y2 - pointPtr[1];
	if (tmp < yDiff) {
	    yDiff = tmp;
	}
	if (yDiff < xDiff) {
	    xDiff = yDiff;
	}
	xDiff -= width;
	if (xDiff < 0.0) {
	    return 0.0;
	}
	return xDiff;
    }

    /*
     * Point is outside rectangle.
     */

    if (pointPtr[0] < x1) {
	xDiff = x1 - pointPtr[0];
    } else if (pointPtr[0] > x2)  {
	xDiff = pointPtr[0] - x2;
    } else {
	xDiff = 0;
    }

    if (pointPtr[1] < y1) {
	yDiff = y1 - pointPtr[1];
    } else if (pointPtr[1] > y2)  {
	yDiff = pointPtr[1] - y2;
    } else {
	yDiff = 0;
    }

    return hypot(xDiff, yDiff);
}

/*
 *--------------------------------------------------------------
 *
 * RectToArea --
 *
 *	This procedure is called to determine whether an item
 *	lies entirely inside, entirely outside, or overlapping
 *	a given rectangle.
 *
 * Results:
 *	-1 is returned if the item is entirely outside the area
 *	given by rectPtr, 0 if it overlaps, and 1 if it is entirely
 *	inside the given area.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
RectToArea(canvas, itemPtr, areaPtr)
    Tk_Canvas canvas;		/* Canvas containing item. */
    Tk_Item *itemPtr;		/* Item to check against rectangle. */
    double *areaPtr;		/* Pointer to array of four coordinates
				 * (x1, y1, x2, y2) describing rectangular
				 * area.  */
{
    CanvRectItem *rectPtr = (CanvRectItem *) itemPtr;
    double halfWidth;
    double width;
    Tk_State state = itemPtr->state;

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }

    width = rectPtr->outline.width;
    if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (rectPtr->outline.activeWidth>width) {
	    width = rectPtr->outline.activeWidth;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (rectPtr->outline.disabledWidth>0) {
	    width = rectPtr->outline.disabledWidth;
	}
    }

    halfWidth = width/2.0;
    if (rectPtr->outline.gc == None) {
	halfWidth = 0.0;
    }

    if ((areaPtr[2] <= (rectPtr->bbox[0] - halfWidth))
	    || (areaPtr[0] >= (rectPtr->bbox[2] + halfWidth))
	    || (areaPtr[3] <= (rectPtr->bbox[1] - halfWidth))
	    || (areaPtr[1] >= (rectPtr->bbox[3] + halfWidth))) {
	return -1;
    }
    if ((rectPtr->fillGC == None) && (rectPtr->outline.gc != None)
	    && (areaPtr[0] >= (rectPtr->bbox[0] + halfWidth))
	    && (areaPtr[1] >= (rectPtr->bbox[1] + halfWidth))
	    && (areaPtr[2] <= (rectPtr->bbox[2] - halfWidth))
	    && (areaPtr[3] <= (rectPtr->bbox[3] - halfWidth))) {
	return -1;
    }
    if ((areaPtr[0] <= (rectPtr->bbox[0] - halfWidth))
	    && (areaPtr[1] <= (rectPtr->bbox[1] - halfWidth))
	    && (areaPtr[2] >= (rectPtr->bbox[2] + halfWidth))
	    && (areaPtr[3] >= (rectPtr->bbox[3] + halfWidth))) {
	return 1;
    }
    return 0;
}

/*
 *--------------------------------------------------------------
 *
 * ScaleRectOval --
 *
 *	This procedure is invoked to rescale a rectangle or oval
 *	item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The rectangle or oval referred to by itemPtr is rescaled
 *	so that the following transformation is applied to all
 *	point coordinates:
 *		x' = originX + scaleX*(x-originX)
 *		y' = originY + scaleY*(y-originY)
 *
 *--------------------------------------------------------------
 */

static void
ScaleRectOval(canvas, itemPtr, originX, originY, scaleX, scaleY)
    Tk_Canvas canvas;			/* Canvas containing rectangle. */
    Tk_Item *itemPtr;			/* Rectangle to be scaled. */
    double originX, originY;		/* Origin about which to scale rect. */
    double scaleX;			/* Amount to scale in X direction. */
    double scaleY;			/* Amount to scale in Y direction. */
{
    CanvRectItem *canvRectPtr = (CanvRectItem *) itemPtr;

    canvRectPtr->bbox[0] = originX + scaleX*(canvRectPtr->bbox[0] - originX);
    canvRectPtr->bbox[1] = originY + scaleY*(canvRectPtr->bbox[1] - originY);
    canvRectPtr->bbox[2] = originX + scaleX*(canvRectPtr->bbox[2] - originX);
    canvRectPtr->bbox[3] = originY + scaleY*(canvRectPtr->bbox[3] - originY);
    ComputeRectOvalBbox(canvas, canvRectPtr);
}

/*
 *--------------------------------------------------------------
 *
 * TranslateRectOval --
 *
 *	This procedure is called to move a rectangle or oval by a
 *	given amount.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The position of the rectangle or oval is offset by
 *	(xDelta, yDelta), and the bounding box is updated in the
 *	generic part of the item structure.
 *
 *--------------------------------------------------------------
 */

static void
TranslateRectOval(canvas, itemPtr, deltaX, deltaY)
    Tk_Canvas canvas;			/* Canvas containing item. */
    Tk_Item *itemPtr;			/* Item that is being moved. */
    double deltaX, deltaY;		/* Amount by which item is to be
					 * moved. */
{
    CanvRectItem *canvRectPtr = (CanvRectItem *) itemPtr;

    canvRectPtr->bbox[0] += deltaX;
    canvRectPtr->bbox[1] += deltaY;
    canvRectPtr->bbox[2] += deltaX;
    canvRectPtr->bbox[3] += deltaY;
    ComputeRectOvalBbox(canvas, canvRectPtr);
}

/*
 *--------------------------------------------------------------
 *
 * RectOvalToPostscript --
 *
 *	This procedure is called to generate Postscript for
 *	rectangle and oval items.
 *
 * Results:
 *	The return value is a standard Tcl result.  If an error
 *	occurs in generating Postscript then an error message is
 *	left in the interp's result, replacing whatever used to be there.
 *	If no error occurs, then Postscript for the rectangle is
 *	appended to the result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
RectOvalToPostscript(interp, canvas, itemPtr, prepass)
    Tcl_Interp *interp;			/* Interpreter for error reporting. */
    Tk_Canvas canvas;			/* Information about overall canvas. */
    Tk_Item *itemPtr;			/* Item for which Postscript is
					 * wanted. */
    int prepass;			/* 1 means this is a prepass to
					 * collect font information;  0 means
					 * final Postscript is being created. */
{
    char pathCmd[500];
    CanvRectItem *canvRectPtr = (CanvRectItem *) itemPtr;
    double y1, y2;
    XColor *color;
    XColor *fillColor;
    Pixmap fillStipple;
    Tk_State state = itemPtr->state;

    y1 = Tk_CanvasPsY(canvas, canvRectPtr->bbox[1]);
    y2 = Tk_CanvasPsY(canvas, canvRectPtr->bbox[3]);

    /*
     * Generate a string that creates a path for the rectangle or oval.
     * This is the only part of the procedure's code that is type-
     * specific.
     */


    if (canvRectPtr->header.typePtr == &tkCanvRectType) {
	sprintf(pathCmd, "%.15g %.15g moveto %.15g 0 rlineto 0 %.15g rlineto %.15g 0 rlineto closepath\n",
		canvRectPtr->bbox[0], y1,
		canvRectPtr->bbox[2]-canvRectPtr->bbox[0], y2-y1,
		canvRectPtr->bbox[0]-canvRectPtr->bbox[2]);
    } else {
	sprintf(pathCmd, "matrix currentmatrix\n%.15g %.15g translate %.15g %.15g scale 1 0 moveto 0 0 1 0 360 arc\nsetmatrix\n",
		(canvRectPtr->bbox[0] + canvRectPtr->bbox[2])/2, (y1 + y2)/2,
		(canvRectPtr->bbox[2] - canvRectPtr->bbox[0])/2, (y1 - y2)/2);
    }

    if(state == TK_STATE_NULL) {
	state = ((TkCanvas *)canvas)->canvas_state;
    }
    color = canvRectPtr->outline.color;
    fillColor = canvRectPtr->fillColor;
    fillStipple = canvRectPtr->fillStipple;
    if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (canvRectPtr->outline.color!=NULL) {
	    color = canvRectPtr->outline.color;
	}
	if (canvRectPtr->activeFillColor!=NULL) {
	    fillColor = canvRectPtr->activeFillColor;
	}
	if (canvRectPtr->activeFillStipple!=None) {
	    fillStipple = canvRectPtr->activeFillStipple;
	}
    } else if (state==TK_STATE_DISABLED) {
	if (canvRectPtr->outline.disabledColor!=NULL) {
	    color = canvRectPtr->outline.disabledColor;
	}
	if (canvRectPtr->disabledFillColor!=NULL) {
	    fillColor = canvRectPtr->disabledFillColor;
	}
	if (canvRectPtr->disabledFillStipple!=None) {
	    fillStipple = canvRectPtr->disabledFillStipple;
	}
    }

    /*
     * First draw the filled area of the rectangle.
     */

    if (fillColor != NULL) {
	Tcl_AppendResult(interp, pathCmd, (char *) NULL);
	if (Tk_CanvasPsColor(interp, canvas, fillColor)
		!= TCL_OK) {
	    return TCL_ERROR;
	}
	if (fillStipple != None) {
	    Tcl_AppendResult(interp, "clip ", (char *) NULL);
	    if (Tk_CanvasPsStipple(interp, canvas, fillStipple)
		    != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (color != NULL) {
		Tcl_AppendResult(interp, "grestore gsave\n", (char *) NULL);
	    }
	} else {
	    Tcl_AppendResult(interp, "fill\n", (char *) NULL);
	}
    }

    /*
     * Now draw the outline, if there is one.
     */

    if (color != NULL) {
	Tcl_AppendResult(interp, pathCmd, "0 setlinejoin 2 setlinecap\n",
		(char *) NULL);
	if (Tk_CanvasPsOutline(canvas, itemPtr,
		&(canvRectPtr->outline))!= TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}
