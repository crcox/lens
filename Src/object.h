#ifndef OBJECT_H
#define OBJECT_H

enum memberTypes{SPACER, OBJ, OBJP, OBJPP, OBJA, OBJPA, OBJAA, OBJPAA};

typedef struct objInfo *ObjInfo;
typedef struct memInfo *MemInfo;

struct objInfo {
  char *name;        /* Name of this type of object, like "Network" */
  int size;          /* Only used for OBJAs (arrays of these objects) */
  int maxDepth;      /* For printing recursive object structures.  If -1,
			object is terminal */
  /* Function to produce name of instance */
  void (*getName)(void *object, char *name); 
  void (*setValue)(void *object, char *value);
  void (*setStringValue)(void *object, char *value);
  MemInfo members;   /* List of members */
  MemInfo *tail;
};

struct memInfo {
  char *name;        /* The actual name used in the structure definition */
  int type;          /* OBJ, OBJP, OBJA, OBJPA, OBJAA, or OBJPAA */
  flag writable;     /* Is this not write-protected */
  int offset;        /* Byte offset from start of structure */
  int (*rows)(void *);
  int (*cols)(void *);
  ObjInfo info;      /* The info about the type of object this member is */

  MemInfo next;
};

extern void *Obj(char *object, int offset);
extern void *ObjP(char *object, int offset);
extern void *ObjPP(char *object, int offset);
extern void *ObjA(char *array, int size, int index);
extern void *ObjPA(char *array, int index);
extern void *ObjAA(char *array, int size, int row, int col);
extern void *ObjPAA(char *array, int row, int col);

extern void createObjects(void);
extern void initObjects(void);
/* Remove this */
extern void addMember(ObjInfo O, char *name, int type, int offset, 
		      flag writable, int (*rows)(void *), int (*cols)(void *), 
		      ObjInfo info);
extern void addObj(ObjInfo O, char *name, int offset, flag writable, 
		   ObjInfo info);
extern void addObjP(ObjInfo O, char *name, int offset, flag writable, 
		    ObjInfo info);
extern void addObjPA(ObjInfo O, char *name, int offset, flag writable, 
		     int (*rows)(void *), ObjInfo info);
extern void addObjAA(ObjInfo O, char *name, int offset, flag writable, 
		     int (*rows)(void *), int (*cols)(void *), ObjInfo info);
extern void addObjPAA(ObjInfo O, char *name, int offset, flag writable, 
		      int (*rows)(void *), int (*cols)(void *), ObjInfo info);
extern void addSpacer(ObjInfo O);

extern MemInfo lookupMember(char *name, ObjInfo objectInfo);

extern ObjInfo newObject(char *name, int size, int maxDepth,
			 void (*nameProc)(void *, char *), 
			 void (*setProc)(void *, char *),
			 void (*setStringProc)(void *, char *));
extern void *getObject(char *path, ObjInfo *retObjInfo, int *retType, 
		       int *retRows, int *retCols, flag *retWrit);
extern flag isObject(char *path);
extern void printObject(char *object, ObjInfo O, int type, int rows, 
			int cols, int depth, int initDepth, int maxDepth);
extern flag writeParameters(Tcl_Channel channel, char *path);

extern ObjInfo RootInfo;
extern ObjInfo NetInfo;
extern ObjInfo GroupInfo;
extern ObjInfo UnitInfo;
extern ObjInfo BlockInfo;
extern ObjInfo LinkInfo;
extern ObjInfo ExampleSetInfo;
extern ObjInfo ExampleInfo;
extern ObjInfo EventInfo;
extern ObjInfo SparseLotInfo;

extern ObjInfo IntInfo;             /* int */
extern ObjInfo RealInfo;            /* real */
extern ObjInfo FlagInfo;            /* flag */
extern ObjInfo MaskInfo;            /* mask */
extern ObjInfo StringInfo;          /* char * */
extern ObjInfo TclObjInfo;          /* TclObj */

#endif /* OBJECT_H */
