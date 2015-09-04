#ifndef EXTENSION_H
#define EXTENSION_H
#include "object.h"

extern char *VersionExt;

typedef struct netExt *NetExt;
typedef struct groupExt *GroupExt;
typedef struct unitExt *UnitExt;
typedef struct blockExt *BlockExt;
typedef struct exSetExt *ExSetExt;
typedef struct exampleExt *ExampleExt;
typedef struct eventExt *EventExt;

extern flag userInit(void);

extern flag initNetworkExtension(Network N);
extern flag resetNetworkExtension(Network N);
extern flag freeNetworkExtension(Network N);
extern flag copyNetworkExtension(Network from, Network to);

extern flag initGroupExtension(Group G);
extern flag freeGroupExtension(Group G);
extern flag copyGroupExtension(Group from, Group to);

extern flag initUnitExtension(Unit U);
extern flag freeUnitExtension(Unit U);
extern flag copyUnitExtension(Unit from, Unit to);

extern flag initBlockExtension(Block B);
extern flag freeBlockExtension(Block B);
extern flag copyBlockExtension(Block from, Block to);

extern flag initExSetExtension(ExampleSet S);
extern flag freeExSetExtension(ExampleSet S);
extern flag copyExSetExtension(ExampleSet from, ExampleSet to);

extern flag initExampleExtension(Example E);
extern flag freeExampleExtension(Example E);
extern flag copyExampleExtension(Example from, Example to);

extern flag initEventExtension(Event V);
extern flag freeEventExtension(Event V);
extern flag copyEventExtension(Event from, Event to);

extern ObjInfo NetExtInfo;
extern ObjInfo GroupExtInfo;
extern ObjInfo UnitExtInfo;
extern ObjInfo BlockExtInfo;
extern ObjInfo ExSetExtInfo;
extern ObjInfo ExampleExtInfo;
extern ObjInfo EventExtInfo;

#endif /* EXTENSION_H */
