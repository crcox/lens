#ifndef CONTROL_H
#define CONTROL_H

#include <tcl.h>

extern flag queryUser(char *fmt, ...);
extern flag createCommand(Tcl_ObjCmdProc *proc, char *name);
extern flag registerSynopsis(char *name, char *synopsis);
extern flag registerCommand(Tcl_ObjCmdProc *proc, char *name, char *synopsis);
extern flag printSynopsis(char *name);

extern flag configureDisplay(void);
extern flag signalCurrentNetChanged(void);
extern flag signalSetsChanged(int set);
extern flag signalNetListChanged(void);
extern flag signalSetListsChanged(void);
extern flag signalNetStructureChanged(void);

extern void startTask(mask type);
extern void stopTask(mask type);
extern flag unsafeToRun(const char *command, mask badTasks);
extern flag smartUpdate(flag forceUpdate);
extern flag haltWaiting(void);
extern void signalHalt(void);

extern void signalHandler(int sig);

extern void initializeSimulator(void);

#endif /* CONTROL_H */
