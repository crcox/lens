#ifndef CONNECT_H
#define CONNECT_H

extern void initLinkTypes(void);
extern mask lookupLinkType(char *typeName);
extern flag registerLinkType(char *typeName, mask *type);
extern flag unregisterLinkType(char *typeName);
extern flag listLinkTypes(void);
extern Link lookupLink(Unit preUnit, Unit postUnit, int *num, mask linkType);
extern Unit lookupPreUnit(Unit postUnit, int num);
extern int  getLinkType(Unit U, Link L);
extern Link2 getLink2(Unit U, Link L);

extern void randomizeUnitWeights(Unit U, real range, real mean, real reqRange,
				 real reqMean, mask linkType, flag doFrozen);
extern void randomizeGroupWeights(Group G, real range, real mean,
				  real reqRange, real reqMean, mask linkType,
				  flag doFrozen);
extern void randomizeWeights(real range, real mean, mask linkType,
			     flag doFrozen);

extern void setUnitBlockValues(Unit U, flag ext, MemInfo M, char *value,
			       mask linkType);
extern void setGroupBlockValues(Group G, flag ext, MemInfo M, char *value,
				mask linkType);
extern void setBlockValues(flag ext, MemInfo M, char *value, mask linkType);
extern void initLinkValues(Link L, Link2 M);

extern flag connectUnits(Unit preUnit, Unit postUnit, mask linkType,
			 real range, real mean, flag frozen);
extern flag connectGroupToUnit(Group preGroup, Unit postUnit, mask linkType,
			       real range, real mean, flag frozen);
extern flag fullConnectGroups(Group preGroup, Group postGroup, mask linkType,
			      real strength, real range, real mean);
extern void registerProjectionTypes(void);

extern flag elmanConnect(Group source, Group elman);

extern flag disconnectUnits(Unit preUnit, Unit postUnit, mask linkType);
extern flag disconnectGroupFromUnit(Group preGroup, Unit postUnit,
				    mask linkType);
extern flag disconnectGroups(Group preGroup, Group postGroup, mask linkType);

extern flag deleteUnitInputs(Unit U, mask linkType);
extern flag deleteGroupInputs(Group G, mask linkType);
extern flag deleteUnitOutputs(Unit U, mask linkType);
extern flag deleteGroupOutputs(Group G, mask linkType);
extern flag deleteAllLinks(mask linkType);

extern void freezeUnitInputs(Unit U, mask linkType);
extern void freezeGroupInputs(Group G, mask linkType);
extern void freezeAllLinks(mask linkType);
extern void thawUnitInputs(Unit U, mask linkType);
extern void thawGroupInputs(Group G, mask linkType);
extern void thawAllLinks(mask linkType);

extern flag lesionLinks(Group G, real proportion, int mode, real value,
		real (*noiseProc)(real value, real range), mask linkType);

extern flag standardSaveWeights(Tcl_Obj *fileNameObj, flag binary, int numValues,
				flag thawed, flag frozen);
extern flag standardLoadWeights(Tcl_Obj *fileNameObj, flag thawed, flag frozen);
extern flag loadXerionWeights(Tcl_Obj *fileNameObj);

extern char **LinkTypeName;

#endif /* CONNECT_H */
