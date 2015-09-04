#include <string.h>
#include "system.h"
#include "util.h"
#include "type.h"
#include "network.h"
#include "object.h"
#include "command.h"
#include "control.h"

#define MAX_ARRAY  60
#define MAX_FIELDS 100

int C_getObject(TCL_CMDARGS) {
  int type, rows, cols, arg = 1;
  flag writable;
  ObjInfo O;
  char *object, *objName = "";
  int depth, additionalDepth = 1;

  const char *usage = "getObject [<object>] [-depth <max-depth>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc > 1 && Tcl_GetStringFromObj(objv[1], NULL)[0] != '-') {
    objName = Tcl_GetStringFromObj(objv[1], NULL);
    arg = 2;
  }
  if (arg < objc) {
    if (arg != objc - 2) return usageError(commandName, usage);
    if (subString(Tcl_GetStringFromObj(objv[arg], NULL), "-depth", 2))
      Tcl_GetIntFromObj(interp, objv[++arg], &additionalDepth);
    else return usageError(commandName, usage);
  }

  if (!(object = getObject(objName, &O, &type, &rows, &cols, &writable)))
    return TCL_ERROR;

  depth = MAX(O->maxDepth, 0);
  Tcl_ResetResult(Interp);
  printObject(object, O, type, rows, cols, depth, depth,
	      O->maxDepth + additionalDepth);

  return TCL_OK;
}

int C_setObject(TCL_CMDARGS) {
  int type, valoffset, rows, cols;
  flag writable;
  char *object;
  ObjInfo O;

  const char *usage = "setObject <object-member> [<value>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2 && objc != 3)
    return usageError(commandName, usage);
  if (objc == 2)
    return C_getObject(data, interp, objc, objv);

  if (!(object = getObject(Tcl_GetStringFromObj(objv[1], NULL), &O, &type, &rows, &cols, &writable)))
    return TCL_ERROR;

  if (!O->setStringValue)
    return warning("%s: \"%s\" is not a basic type and cannot be set", commandName,
		   Tcl_GetStringFromObj(objv[1], NULL));
  if (!writable)
    return warning("%s is write-protected", Tcl_GetStringFromObj(objv[1], NULL));
  O->setStringValue(object, Tcl_GetStringFromObj(objv[2],NULL));
  sprintf(Buffer, "uplevel #0 set \".%s\" ", Tcl_GetStringFromObj(objv[1], NULL));
  valoffset = strlen(Buffer);
  O->getName(object, Buffer + valoffset);

  Tcl_Eval(interp, Buffer);
  Tcl_SetResult(interp, Buffer + valoffset, TCL_VOLATILE);

  return TCL_OK;
}

int C_path(TCL_CMDARGS) {
  const char *usage = "path (-network <network> | -group <group> | -unit <unit> |\n"
    "\t-set <example-set>)";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 3) return usageError(commandName, usage);
  if (Tcl_GetStringFromObj(objv[1], NULL)[0] != '-') return usageError(commandName, usage);
  switch (Tcl_GetStringFromObj(objv[1], NULL)[1]) {
  case 'n': {
    Network N;
    if (!(N = lookupNet(Tcl_GetStringFromObj(objv[2], NULL))))
      return warning("%s: network \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[2], NULL));
    return result("root.net(%d)", N->num);
  }
  case 'g': {
    Group G;
    if (!(G = lookupGroup(Tcl_GetStringFromObj(objv[2], NULL))))
      return warning("%s: group \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[2], NULL));
    return result("group(%d)", G->num);
  }
  case 'u': {
    Unit U;
    if (!(U = lookupUnit(Tcl_GetStringFromObj(objv[2], NULL))))
      return warning("%s: unit \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[2], NULL));
    return result("group(%d).unit(%d)", U->group->num, U->num);
  }
  case 's': {
    ExampleSet S;
    if (!(S = lookupExampleSet(Tcl_GetStringFromObj(objv[2], NULL))))
      return warning("%s: example set \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[2], NULL));
    return result("root.set(%d)", S->num);
  }}
  return usageError(commandName, usage);
}

#ifdef JUNK
int C_netPath(TCL_CMDARGS) {
  Network N;
  const char *usage = "netPath <network>";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2)
    return usageError(commandName, usage);

  if (!(N = lookupNet(Tcl_GetStringFromObj(objv[1], NULL))))
    return warning("%s: network \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[1], NULL));
  return result("root.net(%d)", N->num);
}

int C_groupPath(TCL_CMDARGS) {
  Group G;
  const char *usage = "groupPath <group>";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2)
    return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  if (!(G = lookupGroup(Tcl_GetStringFromObj(objv[1], NULL))))
    return warning("%s: group \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[1], NULL));
  return result("group(%d)", G->num);
}

int C_unitPath(TCL_CMDARGS) {
  Unit U;
  const char *usage = "unitPath <unit>";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2)
    return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  if (!(U = lookupUnit(Tcl_GetStringFromObj(objv[1], NULL))))
    return warning("%s: unit \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[1], NULL));
  return result("group(%d).unit(%d)", U->group->num, U->num);
}

int C_setPath(TCL_CMDARGS) {
  ExampleSet S;
  const char *usage = "setPath <example-set>";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2)
    return usageError(commandName, usage);

  if (!(S = lookupExampleSet(Tcl_GetStringFromObj(objv[1], NULL))))
    return warning("%s: example set \"%s\" doesn't exist", commandName, Tcl_GetStringFromObj(objv[1], NULL));
  return result("root.set(%d)", S->num);
}
#endif

static flag addField(char *win, char *name, void *object, ObjInfo O, int type,
		     int rows, int cols, flag writable, flag array) {
  char value[512], dest[128];
  if (array)
    sprintf(dest, "(%s)", name);
  else
    sprintf(dest, ".%s", name);

  switch (type) {
  case OBJ:
  case OBJP:
    if (object)
      O->getName(object, value);
    else *value = '\0';
    if (!O->members && O->maxDepth < 0) {
      sprintf(Buffer, ".addMember %s {%s} {%s} %d", win, name,
	      value, writable);
    } else {
      sprintf(Buffer, ".addObject %s {%s} {%s} {%s} %d",
	      win, name, value, dest, (object) ? 1 : 0);
    }
    break;
  case OBJA:
  case OBJPA:
    sprintf(Buffer, ".addObjectArray %s {%s} {%s} {%s} %d", win, name,
	    O->name, dest, (object) ? 1 : 0);
    break;
  case OBJAA:
  case OBJPAA:
    sprintf(Buffer, ".addObjectArrayArray %s {%s} {%s} {%s} %d %d", win,
	    name, O->name, dest, rows, (object) ? 1 : 0);
    break;
  default:
    fatalError("Bad case in addField");
  }
  return Tcl_Eval(Interp, Buffer);
}

int C_sendObjectArray(TCL_CMDARGS) {
  char value[512];
  ObjInfo O;
  int i, type, rows, cols;
  flag writable;
  char *win, *menu, *object, *name, *dest;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 6)
    return warning("%s was called with the wrong # of arguments", commandName);
  win = Tcl_GetStringFromObj(objv[1], NULL);
  menu = Tcl_GetStringFromObj(objv[2], NULL);
  name = Tcl_GetStringFromObj(objv[4], NULL);
  dest = Tcl_GetStringFromObj(objv[5], NULL);

  if (!(object = getObject(Tcl_GetStringFromObj(objv[3], NULL), &O, &type, &rows, &cols, &writable)))
    return TCL_ERROR;
  sprintf(Buffer, ".buildObjectArray %s %s {%s} {%s} %d {", win, menu, name,
	  dest, rows);
  for (i = 0; i < rows && i < MAX_ARRAY; i++) {
    if (type == OBJA)
      O->getName(ObjA(object, O->size, i), value);
    else
      O->getName(ObjPA(object, i), value);
    sprintf(Buffer + strlen(Buffer), "{%s} ", value);
  }
  strcat(Buffer, "}");
  return Tcl_Eval(Interp, Buffer);
}

/* Assumes the path is cleaned up.  If the last thing on the path is an index
   into an array, this returns the index, else -1 */
static int arrayElement(char *path) {
  int index;
  char *s = path + strlen(path) - 1;
  if (*s != ')') return -1;
  for (s--; s >= path && *s != ',' && *s != '('; s--);
  if (s++ < path) return -1;
  sscanf(s, "%d", &index);
  return index;
}

static char *nextElement(char *path) {
  int index;
  char *new, *s = path + strlen(path) - 1;
  if (*s != ')') return NULL;
  for (s--; s >= path && *s != ',' && *s != '('; s--);
  if (s++ < path) return NULL;
  sscanf(s, "%d", &index);
  new = (char *) safeMalloc(strlen(path) + 1, "nextElement:new");
  strcpy(new, path);
  s = new + (s - path);
  sprintf(s, "%d)", index + 1);
  return new;
}

static char *prevElement(char *path) {
  int index;
  char *new, *s = path + strlen(path) - 1;
  if (*s != ')') return NULL;
  for (s--; s >= path && *s != ',' && *s != '('; s--);
  if (s++ < path) return NULL;
  sscanf(s, "%d", &index);
  new = (char *) safeMalloc(strlen(path) + 1, "nextElement:new");
  strcpy(new, path);
  s = new + (s - path);
  sprintf(s, "%d)", index - 1);
  return new;
}

/* This is not a user accessible function */
int C_loadObject(TCL_CMDARGS) {
  int i, type, rows, cols;
  flag writable;
  ObjInfo O;
  MemInfo M;
  char *object, *win, label[32];
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);

  if (objc != 3)
    return warning("%s was called with the wrong # of arguments", commandName);
  win = Tcl_GetStringFromObj(objv[1], NULL);

  if (!(object = getObject(Tcl_GetStringFromObj(objv[2], NULL), &O, &type, &rows, &cols, &writable)))
    return TCL_ERROR;

  else if (type == OBJ || type == OBJPP)
    fatalError("got OBJ type in loadObject");
  else if (type == OBJP && !O->members) {
    if (addField(win, "", object, O, OBJP, -1, -1, writable, FALSE))
      return TCL_ERROR;
  }
  else if (type == OBJP) {
    for (M = O->members; M; M = M->next) {
      switch (M->type) {
      case SPACER:
	if (eval(".addSpacer %s", win)) return TCL_ERROR;
	break;
      case OBJ:
	if (addField(win, M->name, Obj(object, M->offset), M->info, OBJP,
		     -1, -1, M->writable, FALSE)) return TCL_ERROR;
	break;
      case OBJP:
	if (addField(win, M->name, ObjP(object, M->offset), M->info, OBJP,
		     -1, -1, M->writable, FALSE)) return TCL_ERROR;
	break;
      case OBJPP:
	if (addField(win, M->name, ObjPP(object, M->offset), M->info, OBJP,
		     -1, -1, M->writable, FALSE)) return TCL_ERROR;
	break;
      case OBJA:
      case OBJPA:
	if (addField(win, M->name, ObjP(object, M->offset), M->info, M->type,
		     M->rows(object), -1, M->writable,
		     FALSE)) return TCL_ERROR;
	break;
      case OBJAA:
      case OBJPAA:
	if (addField(win, M->name, ObjP(object, M->offset), M->info, M->type,
		     M->rows(object), M->cols(object), M->writable,
		     FALSE)) return TCL_ERROR;
	break;
      }
    }
  }
  else if (type == OBJA || type == OBJPA) {
    for (i = 0; i < rows && i < MAX_FIELDS; i++) {
      sprintf(label, "%d", i);
      if (type == OBJA) {
	if (addField(win, label, ObjA(object, O->size, i), O, OBJP,
		     -1, -1, writable, TRUE)) return TCL_ERROR;
      } else {
	if (addField(win, label, ObjPA(object, i), O, OBJP,
		     -1, -1, writable, TRUE)) return TCL_ERROR;
      }
    }
    if (i >= MAX_FIELDS)
      eval(".addLabel %s toobig \"Array(%d) too large to display\"",
	   win, rows);
  }
  else if (type == OBJAA || type == OBJPAA) {
    for (i = 0; i < rows && i < MAX_FIELDS; i++) {
      sprintf(label, "%d", i);
      if (type == OBJAA) {
	if (addField(win, label, ObjPA(object, i), O, OBJA,
		     cols, -1, writable, TRUE)) return TCL_ERROR;
      } else {
	if (addField(win, label, ObjPA(object, i), O, OBJPA,
		     cols, -1, writable, TRUE)) return TCL_ERROR;
      }
    }
    if (i >= MAX_FIELDS)
      eval(".addLabel %s toobig \"Array(%d) too large to display\"",
	   win, rows);
  }
  if ((i = arrayElement(Tcl_GetStringFromObj(objv[2], NULL))) >= 0) {
    char *newpath = nextElement(Tcl_GetStringFromObj(objv[2], NULL));
    if (!newpath) return warning("nextElement failed on %s", Tcl_GetStringFromObj(objv[2], NULL));
    if (i > 0) eval(".setArrayCanGoLeft %s", win);

    if (getObject(newpath, &O, &type, &rows, &cols, &writable))
      eval(".setArrayCanGoRight %s", win);
    FREE(newpath);
  }

  return TCL_OK;
}

/* This is not a user accessible function */
int C_cleanPath(TCL_CMDARGS) {
  int count;
  char *r, *w;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 2)
    return warning("%s was called with the wrong # of arguments", commandName);
  for (r = Tcl_GetStringFromObj(objv[1], NULL),
       w = Tcl_GetStringFromObj(objv[1], NULL),
       count = 0; *r; r++) {
    if (*r == '[') *r = '(';
    else if (*r == ']') *r = ')';
    if (*r == '(') count++;
    else if (*r == ')') count--;
  }

  for (r = Tcl_GetStringFromObj(objv[1], NULL),
       w = Tcl_GetStringFromObj(objv[1], NULL); *r; r++) {
    if (*r == ')' && r[1] == '(') {
      *(w++) = ',';
      r++;
    }
    else if (*r == '.' && r[1] == '(') {
      *(w++) = '(';
      r++;
    }
    else if (!r[1] && (*r == '.' || *r == '(')) {
      if (*r == '(') count--;
      *w = '\0';
    }
    else if (!(r == Tcl_GetStringFromObj(objv[1], NULL) && *r == '.'))
      *(w++) = *r;
  }
  *w = '\0';
  result(Tcl_GetStringFromObj(objv[1], NULL));
  if (count > 0) append(")");
  return TCL_OK;
}

/* This is not a user accessible function */
int C_pathParent(TCL_CMDARGS) {
  char *r;
  int n;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 2)
    return warning("%s was called with the wrong # of arguments", commandName);
  r = Tcl_GetStringFromObj(objv[1], &n);
  for (r = r + (n-1); r >= Tcl_GetStringFromObj(objv[1], NULL) && *r != '.'; r--) {
    if (r >= Tcl_GetStringFromObj(objv[1], NULL))
      *r = '\0';
    else
      Tcl_GetStringFromObj(objv[1], NULL)[0] = '\0';
  }
  return result(Tcl_GetStringFromObj(objv[1], NULL));
}

/* This is not a user accessible function */
int C_pathUp(TCL_CMDARGS) {
  char *r,*path;
  int n;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 2)
    return warning("%s was called with the wrong # of arguments", commandName);
  r = Tcl_GetStringFromObj(objv[1], &n);
  path = Tcl_GetStringFromObj(objv[1], &n);
  for (r = r + (n-1);
       r >= Tcl_GetStringFromObj(objv[1], NULL) && *r != '.' && *r != '(' && *r != ',';
       r--) {
    if (r >= path) {
      if (*r == ',') {
        *(r++) = ')';
      }
      *r = '\0';
    } else path[0] = '\0';
  return result(path);
  }
  return 0; // just to prevent compiler warning---should never get here.
}

/* This is not a user accessible function */
int C_pathRight(TCL_CMDARGS) {
  char *newpath;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 2)
    return warning("%s was called with the wrong # of arguments", commandName);
  if (!(newpath = nextElement(Tcl_GetStringFromObj(objv[1], NULL))))
    return warning(".pathRight failed on %s", objv[1]);
  result(newpath);
  FREE(newpath);
  return TCL_OK;
}

/* This is not a user accessible function */
int C_pathLeft(TCL_CMDARGS) {
  char *newpath;
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc != 2)
    return warning("%s was called with the wrong # of arguments", commandName);
  if (!(newpath = prevElement(Tcl_GetStringFromObj(objv[1], NULL))))
    return warning(".pathLeft failed on %s", objv[1]);
  result(newpath);
  FREE(newpath);
  return TCL_OK;
}

int C_saveParameters(TCL_CMDARGS) {
  Tcl_Channel channel;
  int arg = 2;
  char groupName[32];
  flag append = FALSE;
  Tcl_Obj *fileNameObj;
  const char *fileName;
  const char *usage = "saveParameters <file-name> [-append] [<object> ...]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc < 2)
    return usageError(commandName, usage);
  if (!Net) return warning("%s: no current network", commandName);

  if (objc > arg && subString(Tcl_GetStringFromObj(objv[arg], NULL), "-append", 2)) {
    append = TRUE;
    arg++;
  }
  fileNameObj = objv[1];
  fileName = Tcl_GetStringFromObj(objv[1], NULL);
  if (!(channel = writeChannel(fileNameObj, append)))
    return warning("saveParameters: couldn't open the file \"%s\"", fileName);

  if (objc == arg) {
    if (writeParameters(channel, "")) {
      closeChannel(channel);
      return TCL_ERROR;
    }
    FOR_EACH_GROUP({
      sprintf(groupName, "group(%d)", g);
      if (writeParameters(channel, groupName)) {
        closeChannel(channel);
        return TCL_ERROR;
      }
    });
  } else for (; arg < objc; arg++)
    if (writeParameters(channel, Tcl_GetStringFromObj(objv[arg], NULL))) {
      closeChannel(channel);
      return TCL_ERROR;
    }

  closeChannel(channel);
  return TCL_OK;
}

void registerObjectCommands(void) {
  registerCommand((Tcl_ObjCmdProc *)C_setObject, "setObject",
		  "sets the value of an object field");
  registerCommand((Tcl_ObjCmdProc *)C_getObject, "getObject",
		  "prints the contents of an object");
  registerCommand((Tcl_ObjCmdProc *)C_path, "path",
		  "returns the full object path name of a major object");
  /*
  registerCommand((Tcl_ObjCmdProc *)C_groupPath, "groupPath",
		  "returns the full object path name of the group");
  registerCommand((Tcl_ObjCmdProc *)C_unitPath, "unitPath",
		  "returns the full object path name of the unit");
  registerCommand((Tcl_ObjCmdProc *)C_setPath, "setPath",
		  "returns the full object path name of the example set");
  */
  registerCommand((Tcl_ObjCmdProc *)C_saveParameters, "saveParameters",
		  "writes network or group parameters to a script file");

  createCommand((Tcl_ObjCmdProc *)C_loadObject, ".loadObject");
  createCommand((Tcl_ObjCmdProc *)C_sendObjectArray, ".sendObjectArray");
  createCommand((Tcl_ObjCmdProc *)C_cleanPath, ".cleanPath");
  createCommand((Tcl_ObjCmdProc *)C_pathParent, ".pathParent");
  createCommand((Tcl_ObjCmdProc *)C_pathUp, ".pathUp");
  createCommand((Tcl_ObjCmdProc *)C_pathRight, ".pathRight");
  createCommand((Tcl_ObjCmdProc *)C_pathLeft, ".pathLeft");
}
