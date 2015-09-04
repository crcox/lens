#ifndef DISPLAY_H
#define DISPLAY_H

enum unitValues {UV_OUT_TARG, UV_OUTPUTS, UV_TARGETS, UV_INPUTS, UV_EXT_INPUTS,
		 UV_OUTPUT_DERIVS, UV_INPUT_DERIVS, UV_GAIN, UV_LINK_WEIGHTS,
		 UV_LINK_DERIVS, UV_LINK_DELTAS};

extern void fillPalette(int palette);
extern real normColorValue(real x, real temp);
extern char *getColor(real x, real temp, int palette);

extern flag setUnitTemp(real val);
extern flag clearUnitPlot(Unit U, void *junk);
extern flag plotRow(int argc, char *argv[], int *unitsPlotted);
extern flag plotRowObj(Tcl_Interp *interp, int objc, Tcl_Obj *objv[], int *unitsPlotted);
extern flag drawUnitsLater(void);
extern flag autoPlot(int numColumns);
extern flag setUnitValue(int val);
extern flag updateUnitDisplay(void);
extern flag chooseUnitSet(void);
extern flag loadExampleDisplay(void);

extern flag setLinkTemp(real val);
extern flag setLinkValue(int val);
extern flag drawLinksLater(void);
extern flag updateLinkDisplay(void);
extern flag linksChanged(void);
extern flag buildLinkGroupMenus(void);
extern void updateDisplays(mask update);

extern char *OutlineColor[NUM_PALETTES][3];
extern char *NaNColor;
extern flag UnitUp;
extern flag LinkUp;

extern int  ViewTick;

#endif /* DISPLAY_H */
