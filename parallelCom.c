#include <string.h>
#include "system.h"
#include "util.h"
#include "type.h"
#include "network.h"
#include "command.h"
#include "control.h"
#include "train.h"
#include "parallel.h"

/* Make the port optional */
int C_startServer(TCL_CMDARGS) {
  int port = 0;
  char *usage = "startServer [<port>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc > 2)
    return usageError(commandName, usage);
  if (objc == 2)
    Tcl_GetIntFromObj(interp, objv[1], &port);
  return startServer(port);
}

int C_stopServer(TCL_CMDARGS) {
  char *usage = "stopServer";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 1)
    return usageError(commandName, usage);
  return stopServer();
}

int C_startClient(TCL_CMDARGS) {
  char *myaddr;
  int port;
  char *usage = "startClient <hostname> <port> [<my-address>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 3 && objc != 4)
    return usageError(commandName, usage);

  myaddr = (objc > 3) ? Tcl_GetStringFromObj(objv[3], NULL) : NULL;
  Tcl_GetIntFromObj(interp, objv[2], &port);
  return startClient(Tcl_GetStringFromObj(objv[1], NULL), port, myaddr);
}

int C_stopClient(TCL_CMDARGS) {
  char *usage = "stopClient";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 1)
    return usageError(commandName, usage);
  return stopClient();
}

int C_sendEval(TCL_CMDARGS) {
  int client = 0;
  char *usage = "sendEval <command> [<client>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 2 && objc != 3)
    return usageError(commandName, usage);
  if (objc == 3)
    Tcl_GetIntFromObj(interp, objv[2], &client);
  return sendEvals(Tcl_GetStringFromObj(objv[1], NULL), client);
}

int C_clientInfo(TCL_CMDARGS) {
  int client = 0;
  char *usage = "clientInfo [<client>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc != 1 && objc != 2)
    return usageError(commandName, usage);
  if (objc == 2)
    Tcl_GetIntFromObj(interp, objv[1], &client);
  return clientInfo(client);
}

int C_waitForClients(TCL_CMDARGS) {
  int clients = 0;
  char *usage = "waitForClients [<num-clients> [<command>]]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h"))
    return commandHelp(commandName);
  if (objc > 3)
    return usageError(commandName, usage);
  if (objc > 1) {
    Tcl_GetIntFromObj(interp, objv[1], &clients);
    if (clients <= 0)
      return warning("%s: number of clients must be positive",
		     commandName, clients);
  }
  if (objc > 2)
    return waitForClients(clients, Tcl_GetStringFromObj(objv[2], NULL));
  else return waitForClients(clients, NULL);
}

int C_trainParallel(TCL_CMDARGS) {
  int arg = 1, testInterval = 0;
  flag synchronous = TRUE;
  char *usage = "trainParallel [<num-updates>] [-nonsynchronous |\n"
    "\t-report <report-interval> | -algorithm <algorithm> |\n"
    "\t-test <test-interval>]";
  const char *commandName = Tcl_GetStringFromObj(objv[0], NULL);
  if (objc == 2 && !strcmp(Tcl_GetStringFromObj(objv[1], NULL), "-h")) return commandHelp("trainParallel");
  if (objc > 6) return usageError("trainParallel", usage);
  if (!Net) return warning("%s: no current network", commandName);
  if (ParallelState != SERVER)
    return warning("trainParallel: must be the server to train in parallel");

  if (objc > 1 && isInteger(Tcl_GetStringFromObj(objv[1], NULL))) {
    Tcl_GetIntFromObj(interp, objv[1], &Net->numUpdates);
    arg = 2;
  }

  for (; arg < objc && Tcl_GetStringFromObj(objv[arg], NULL)[0] == '-'; arg++) {
    switch (Tcl_GetStringFromObj(objv[arg], NULL)[1]) {
    case 'n':
      synchronous = FALSE;
      break;
    case 'r':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetIntFromObj(interp, objv[arg], &Net->reportInterval);
      break;
    case 'a':
      if (++arg >= objc) return usageError(commandName, usage);
      if (lookupTypeMask(Tcl_GetStringFromObj(objv[arg], NULL), ALGORITHM, &(Net->algorithm)))
	return warning("%s: unrecognized algorithm: %s", commandName, objv[arg]);
      break;
    case 't':
      if (++arg >= objc) return usageError(commandName, usage);
      Tcl_GetIntFromObj(interp, objv[arg], &testInterval);
      break;
    default: return usageError(commandName, usage);
    }
  }
  if (arg != objc) return usageError(commandName, usage);

  return trainParallel(synchronous, testInterval);
}

void registerParallelCommands(void) {
  registerCommand((Tcl_ObjCmdProc *)C_startServer, "startServer",
                  "makes the current process a parallel training server");
  registerCommand((Tcl_ObjCmdProc *)C_stopServer, "stopServer",
                  "disengages all clients and stops running as a server");
  registerCommand((Tcl_ObjCmdProc *)C_startClient, "startClient",
                  "makes the current process a parallel training client");
  registerCommand((Tcl_ObjCmdProc *)C_stopClient, "stopClient",
                  "closes the connection with the server");
  registerCommand((Tcl_ObjCmdProc *)C_sendEval, "sendEval",
                  "executes a command on the server or on one or all clients");
  registerSynopsis("sendObject",
		   "a shortcut for doing a local and remote setObject");
  registerCommand((Tcl_ObjCmdProc *)C_clientInfo, "clientInfo",
		  "returns information about one or all clients");
  registerCommand((Tcl_ObjCmdProc *)C_waitForClients, "waitForClients",
		  "executes a command when enough clients have connected");
  registerCommand((Tcl_ObjCmdProc *)C_trainParallel, "trainParallel",
		  "trains the network in parallel on the clients");
  Tcl_LinkVar(Interp, ".ClientsWaiting", (char *) &ClientsWaiting,
	      TCL_LINK_INT);
}
