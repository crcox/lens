#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "util.h"
#include "type.h"
#include "network.h"
#include "train.h"
#include "act.h"
#include "display.h"
#include "graph.h"
#include "control.h"
#include "parallel.h"

enum messages      {EMPTY, DEATH, EVAL, WEIGHTS, TRAIN, TEST, HALT, START, 
		    STOP};
char *messName[] = {"EMPTY", "DEATH", "EVAL", "WEIGHTS", "TRAIN", "TEST", 
		    "HALT", "START", "STOP"};


enum clientState   {IS_IDLE, IS_WAITING, IS_TRAINING, IS_TESTING, IS_DEAD,
		    CLIENT_STATES};
enum haltConditions{HALT_REQUESTED, SERVER_STOPPING, ENOUGH_UPDATES, 
		    ERROR_CRIT, UNIT_CRIT, NO_MORE_CLIENTS};

typedef struct client *Client;

struct client {
  int num;
  Tcl_Channel channel;
  char *hostName;
  int port;
  int state;

  int batchSize;
  int batchTime;
  real proportion;

  Client next;
};

#define FOR_EACH_CLIENT(proc)\
  for (C = ClientList; C; C = C->next) {\
    if (C->state == IS_DEAD) continue;\
    proc;}
#define FOR_EACH_WAITING_CLIENT(proc)\
  for (C = ClientList; C; C = C->next) {\
    if (C->state == IS_WAITING) {proc;}}


/*************************** CLIENT AND SERVER VARIABLES *********************/
/* NONE, CLIENT, or SERVER */
int         ParallelState = NONE; 
/* If the server, this is where it listens for clients.
   If a client, this is how it communicates with the server. */
Tcl_Channel ServerChannel = NULL;
/* True if parallel training is going on. */
flag        IsTraining = FALSE;
/* StartTime is when training started and working time keeps track of how 
   long it was busy. */
unsigned long StartTime, WorkingTime;


/******************************** SERVER VARIABLES ***************************/
int         NumClients = 0;
Client      ClientList;
/* Why parallel training halted. */
int         HaltCondition;
flag        Synchronous;
int         TestInterval;
/* Counts the number of clients in each state. */
int         Clients[CLIENT_STATES];
int         UpdatesDone;
Algorithm   Alg;
int         LastReport;
int         WaitClients;
int         ClientsWaiting;
char       *WaitCommand;


/******************************** MUTUAL CODE ********************************/

static flag sendServerEval(char *message);
static flag sendClientEval(Client C, char *message);
static Client lookupClient(int num);

static flag sendDeath(Tcl_Channel channel) {
  return sendChar(channel, DEATH);
}

static flag sendEval(Tcl_Channel channel, const char *message) {
  return (sendChar(channel, EVAL) ||
	  sendString(channel, message));
}

flag sendEvals(char *message, int num) {
  if (ParallelState == CLIENT)
    return sendServerEval(message);
  if (ParallelState == SERVER) {
    Client C;
    if (num == 0) {
      FOR_EACH_CLIENT(sendClientEval(C, message));
    } else {
      if (!(C = lookupClient(num)))
	return warning("sendEvals: there is no client with number %d", num);
      sendClientEval(C, message);
    }
    return TCL_OK;
  }
  return warning("sendEvals: not a client or a server");
}

static flag receiveEval(Tcl_Channel channel) {
  if (receiveString(channel, Buffer)) return TCL_ERROR;
  if (Tcl_Eval(Interp, Buffer)) error(Tcl_GetStringResult(Interp));
  else print(1, Tcl_GetStringResult(Interp));
  return TCL_OK;
}

static flag receiveLinkValues(Tcl_Channel channel, int offset, 
			      flag increment) {
  int links, i, buffer[PAR_BUF_SIZE], num, val, partial, slack;
  char junk;
  float v;
  
  if (!Net) return TCL_ERROR;
  if (receiveInt(channel, &links)) return TCL_ERROR;
  if (links != Net->numLinks) {
    Tcl_SetChannelOption(Interp, channel, "-blocking", "0");
    /* fcntl(fd, F_SETFL, O_NONBLOCK); */
    while (receiveChar(channel, &junk) == TCL_OK);
    return TCL_ERROR;
  }
  num = i = slack = partial = val = 0;
  FOR_EACH_GROUP({
    FOR_EACH_UNIT(G, {
      FOR_EACH_LINK(U, {
	while (i >= num) {
	  num = MIN(links, PAR_BUF_SIZE) * sizeof(int);
	  if (slack) {
	    num -= slack;
	    buffer[0] = partial;
	  }
	  do {
	    if ((val = Tcl_Read(channel, ((char *) buffer) + slack, num)) 
		== -1) {
	      Tcl_SetChannelOption(Interp, channel, "-blocking", "0");
	      /* fcntl(fd, F_SETFL, O_NONBLOCK); */
	      while (receiveChar(channel, &junk) == TCL_OK);
	      perror("Error in receiveLinkValues");
	      return TCL_ERROR;
	    }
	    if (val == 0) {
	      print(1, "waiting...\n");
	      eval("after 50");
	      smartUpdate(TRUE);
	    }
	  } while (val == 0);
	  val += slack;
	  num = val / sizeof(int);
	  slack = val % sizeof(int);
	  partial = buffer[num];
	  if ((links -= num) < 0) 
	    fatalError("receiveLinkValues got to negative links");
	  i = 0;
	}

	val = NTOHL(buffer[i++]);
	v = *((float *) &val);
	if (increment)
	  *((real *) ((char *) L + offset)) += (isNaNf(v)) ? NaN : (real) v;
	else
	  *((real *) ((char *) L + offset)) =  (isNaNf(v)) ? NaN : (real) v;
      });
    });
  });
  if (links != 0) return TCL_ERROR;
  return TCL_OK;
}


/******************************** SERVER CODE ********************************/

static flag deleteClient(Client C);

static Client lookupClient(int num) {
  Client C;
  FOR_EACH_CLIENT(if (C->num == num) return C);
  return NULL;
}

static void changeState(Client C, int state) {
  Clients[C->state]--;
  C->state = state;
  Clients[state]++;
}

static flag sendClientDeath(Client C) {
  if (sendDeath(C->channel)) return deleteClient(C);
  return TCL_OK;
}

static flag sendClientEval(Client C, char *message) {
  if (sendEval(C->channel, message)) return deleteClient(C);
  return TCL_OK;
}

static flag sendClientStart(Client C) {
  if (sendChar(C->channel, START)) return deleteClient(C);
  return TCL_OK;
}

static flag sendClientStop(Client C) {
  if (sendChar(C->channel, STOP)) return deleteClient(C);
  return TCL_OK;
}

static flag sendClientWeights(Client C) {
  int i = 1, buffer[PAR_BUF_SIZE], blockSize = PAR_BUF_SIZE * sizeof(int);
  Tcl_Channel channel = C->channel;
  float v;

  if (!Net) return TCL_ERROR;
  if (sendChar(channel, WEIGHTS))
    return deleteClient(C);
  buffer[0] = HTONL(Net->numLinks);

  FOR_EACH_GROUP({
    FOR_EACH_UNIT(G, {
      FOR_EACH_LINK(U, {
	v = (isNaN(L->weight)) ? NaNf : (float) L->weight;
	buffer[i++] = HTONL(*(int *) &v);
	if (i == PAR_BUF_SIZE) {
	  if (Tcl_WriteChars(channel, (char *) buffer, blockSize)
	      != blockSize) return deleteClient(C);
	  i = 0;
	}
      });
    });
  });
  if (i && (Tcl_WriteChars(channel, (char *) buffer, i * sizeof(int)) 
	    != i * sizeof(int)))
    return deleteClient(C);
  
  return TCL_OK;
}

static flag sendClientTrain(Client C, char randExample, int batchSize) {
  if (sendChar(C->channel, TRAIN) ||
      sendInt(C->channel, Net->numLinks) ||
      sendInt(C->channel, batchSize) ||
      sendChar(C->channel, randExample)) return deleteClient(C);
  changeState(C, IS_TRAINING);
  C->batchSize = batchSize;
  return TCL_OK;
}

static flag sendClientTest(Client C) {
  if (sendChar(C->channel, TEST) ||
      sendInt(C->channel, UpdatesDone)) return deleteClient(C);
  changeState(C, IS_TESTING);
  return TCL_OK;
}

static flag receiveClientEval(Client C) {
  if (receiveEval(C->channel)) return deleteClient(C);
  return TCL_OK;
}


static void performTest(void) {
  startTask(TESTING);
  Net->netTestBatch(0, TRUE, TRUE);
  stopTask(TESTING);
  print(1, "Performance after %d updates:\n%s\n", UpdatesDone, 
	 Tcl_GetStringResult(Interp));
}


static void performUpdate(Client C, flag groupCritReached) {
  flag willReport;
  UpdatesDone++;
  Net->totalUpdates++;
  willReport = (Net->reportInterval && 
		(!(UpdatesDone % Net->reportInterval) || 
		 UpdatesDone == 1 ||
		 UpdatesDone == Net->numUpdates ||
		 groupCritReached ||
		 Net->error <= Net->criterion)) ? 
    TRUE : FALSE;
  
  /* Change the algorithm if requested. */
  if (Alg->code != Net->algorithm) {
    Alg = getAlgorithm(Net->algorithm);
    print(1, "Changing algorithm to %s...\n", Alg->longName);
  }
  Alg->updateWeights(willReport); /* Here's the important thing */
  updateDisplays(ON_UPDATE);

  if (willReport) {
    printReport(LastReport, UpdatesDone, StartTime);
    LastReport = UpdatesDone;
  }
}

static void synchronousContinue(Client C, flag groupCritReached) {
  static flag groupCritsReached = TRUE;
  int active, totalBatch, desiredBatch;
  real totalExamplePerTime = 0.0;
  real avgExamplePerTime;
  Client firstClient = NULL;

  if (!groupCritReached) groupCritsReached = FALSE;

  if (Clients[IS_TRAINING] == 0) {
    performUpdate(C, groupCritsReached);
    if ((UpdatesDone < Net->numUpdates) && !groupCritsReached && 
	Net->error > Net->criterion) {
      desiredBatch = (Net->batchSize) ? Net->batchSize : 
	Net->trainingSet->numExamples;

      /* Figure out examples/time for all that did the last batch */
      active = 0;
      FOR_EACH_CLIENT({
	if (C->state == IS_IDLE) continue;
	if (C->state != IS_WAITING)
	  error("Got a non-idle client in synchronous mode");
	print(2, "%d %d %d\n", C->num, C->batchSize, C->batchTime);
	if (C->batchSize != 0) {
	  C->proportion = (real) C->batchSize / C->batchTime;
	  totalExamplePerTime += C->proportion;
	  active++;
	}
      });
      /* Figure out examples/time for new clients */
      avgExamplePerTime = totalExamplePerTime / active;
      FOR_EACH_CLIENT({
	if (C->state == IS_IDLE) continue;
	if (C->batchSize == 0) {
	  C->proportion = avgExamplePerTime;
	  totalExamplePerTime += avgExamplePerTime;
	}
      });
      /* Set the batch sizes */
      totalExamplePerTime = 1.0 / totalExamplePerTime;
      totalBatch = 0;
      FOR_EACH_CLIENT({
	if (C->state == IS_IDLE) continue;
	if (!firstClient) firstClient = C;
	C->batchSize = roundr(desiredBatch * C->proportion * 
			     totalExamplePerTime);
	totalBatch += C->batchSize;
      });
      /* Make sure we haven't lost any */
      firstClient->batchSize += desiredBatch - totalBatch;

      FOR_EACH_WAITING_CLIENT({
	if (C->batchSize == 0) continue;
	if (sendClientWeights(C)) continue;
	if (sendClientTrain(C, 0, C->batchSize)) continue;
      });
      if (TestInterval && (UpdatesDone % TestInterval) == 0)
	performTest();
      groupCritsReached = TRUE;
      Net->error = 0.0;
      FOR_EACH_GROUP(resetLinkDerivs(G));
    }
    else {
      if (UpdatesDone >= Net->numUpdates) 
	HaltCondition = ENOUGH_UPDATES;
      else if (Net->error <= Net->criterion || isNaN(Net->error))
	HaltCondition = ERROR_CRIT;
      else if (groupCritsReached) 
	HaltCondition = UNIT_CRIT;
      else error("Stopped parallel training for no reason");
    }
  }
}

static void asynchronousContinue(Client C, flag groupCritReached) {
  performUpdate(C, groupCritReached);
  if ((UpdatesDone < Net->numUpdates) && !groupCritReached && 
      Net->error > Net->criterion) {
    if (sendClientWeights(C)) return;
    if (TestInterval && (UpdatesDone % TestInterval) == 0) {
      if (sendClientTest(C)) return;
    } else if (sendClientTrain(C, 0, Net->batchSize)) return;
  } else {
    if (UpdatesDone >= Net->numUpdates) 
      HaltCondition = ENOUGH_UPDATES;
    else if (Net->error <= Net->criterion) 
      HaltCondition = ERROR_CRIT;
    else if (groupCritReached) 
      HaltCondition = UNIT_CRIT;
    else fatalError("Stopped parallel training for no reason");
  }
}

static flag receiveClientDerivs(Client C) {
  real err;
  flag groupCritReached;
  Link L;

  if (receiveReal(C->channel, &err) ||
      receiveFlag(C->channel, &groupCritReached) ||
      receiveInt(C->channel, &C->batchTime)) return deleteClient(C);
  if (Synchronous)
    Net->error += err;
  else Net->error = err;
  
  if (receiveLinkValues(C->channel, OFFSET(L, deriv), Synchronous))
    return deleteClient(C);
  
  changeState(C, IS_WAITING);

  /* Ignore the derivs if we are done */
  if (!IsTraining) 
    return TCL_OK;

  if (Synchronous)
    synchronousContinue(C, groupCritReached);
  else
    asynchronousContinue(C, groupCritReached);
  return TCL_OK;
}

/* Only happens in asynchronous mode. */
static flag receiveClientTestResults(Client C) {
  int updates;

  if (receiveInt(C->channel, &updates) ||
      receiveString(C->channel, Buffer)) return deleteClient(C);
  changeState(C, IS_WAITING);
  if (IsTraining) {
    print(1, "Performance after %d updates:\n%s\n", updates, Buffer);
    if (sendClientWeights(C) || sendClientTrain(C, 0, Net->batchSize))
      return TCL_ERROR;
  }
  return TCL_OK;
}


static void stopParallelTrain(void) {
  Client C;
  IsTraining = FALSE;
  result("Performed %d updates\n", UpdatesDone);
  switch (HaltCondition) {
  case ENOUGH_UPDATES:
    break;
  case ERROR_CRIT:
    append("Network reached overall error criterion of %f\n", Net->criterion);
    break;
  case UNIT_CRIT:
    append("Network reached unit output criterion\n");
    break;
  case HALT_REQUESTED:
    append("Training was halted early\n");
    break;
  case SERVER_STOPPING:
    append("Server has been killed\n");
    break;
  case NO_MORE_CLIENTS:
    append("No clients left to continue training\n");
    break;
  }
  StartTime = getTime() - StartTime;
  append("Server was working %.2f%%%% of the time (%.2f of %.2f seconds)\n",
	 (real) (WorkingTime * 100) / StartTime, (real) WorkingTime * 1e-3,
	 (real) StartTime * 1e-3);

  FOR_EACH_CLIENT({
    if (C->state != IS_IDLE) {
      if (sendClientStop(C)) continue;
      changeState(C, IS_IDLE);
    }
  });
  
  if (TestInterval) performTest();
}


/* Now this won't return until training is done */
flag trainParallel(flag synch, int testInt) {
  Client C;
  int batchSize, size;
  
  if (NumClients == 0)
    return warning("trainParallel: no clients");
  if (IsTraining)
    return warning("trainParallel: already training in parallel");

  Alg = getAlgorithm(Net->algorithm);

  if (Net->learningRate < 0.0)
    return warning("trainParallel: learningRate (%f) cannot be negative",
		   Net->learningRate);
  if (Net->momentum < 0.0 && Net->momentum >= 1.0)
    return warning("trainParallel: momentum (%f) is out of range [0,1)", 
		   Net->momentum);
  if (Net->weightDecay < 0.0 || Net->weightDecay > 1.0)
    return warning("trainParallel: weight decay (%f) must be in the range "
		   "[0,1]", Net->weightDecay);
  if (Net->numUpdates <= 0)
    return warning("trainParallel: numUpdates (%d) must be positive", 
		   Net->numUpdates);
  if (Net->reportInterval < 0)
    return warning("trainParallel: reportInterval (%d) cannot be negative", 
		   Net->reportInterval);

  StartTime = getTime();
  WorkingTime = 0;
  LastReport = 0;
  Synchronous = synch;
  TestInterval = testInt;
  UpdatesDone = 0;
  IsTraining = TRUE;
  HaltCondition = HALT_REQUESTED;

  startTask(PARA_TRAINING);
  
  print(1, "Performing %d %s parallel updates using %s...\n", Net->numUpdates,
	 (synch) ? "synchronous" : "asynchronous", Alg->longName);
  if (Net->reportInterval) printReportHeader();

  FOR_EACH_CLIENT(changeState(C, IS_WAITING));

  if (Synchronous) {
    batchSize = (Net->batchSize) ? Net->batchSize : 
      Net->trainingSet->numExamples;
    size = batchSize / NumClients;

    FOR_EACH_CLIENT({
      if (sendClientStart(C)) continue;
      if (sendClientWeights(C)) continue;
      if (C == ClientList)
	sendClientTrain(C, 1, batchSize - (size * (NumClients - 1)));
      else sendClientTrain(C, 1, size);
    });
    if (testInt) performTest();
    Net->error = 0.0;
    FOR_EACH_GROUP(resetLinkDerivs(G));
  } else {
    FOR_EACH_CLIENT({
      if (sendClientStart(C)) continue;
      if (sendClientWeights(C)) continue;
      if (TestInterval && C == ClientList) sendClientTest(C);
      else sendClientTrain(C, 1, Net->batchSize);
    });
  }
  
  /* Now we wait until it is halted */
  while (!smartUpdate(TRUE) && HaltCondition == HALT_REQUESTED)
    eval("after 50");
  stopParallelTrain();
  stopTask(PARA_TRAINING);
  return TCL_OK;
}

static void removeDeadClients(void) {
  Client C, N;

  while ((C = ClientList) && C->state == IS_DEAD) {
    ClientList = C->next;
    FREE(C);
  }
  while (C && (N = C->next)) {
    if (N->state == IS_DEAD) {
      C->next = N->next;
      FREE(N);
    } else C = N;
  }
}

static void receiveClientHalt(Client C) {
  flag synchCont = FALSE;
  if (C->state == IS_TRAINING || C->state == IS_TESTING)
    if (sendClientStop(C)) return;
  if (C->state == IS_TRAINING)
    if (Synchronous) synchCont = TRUE;
  changeState(C, IS_IDLE);
  if (IsTraining) {
    error("Client %d has halted", C->num);
    if (UpdatesDone < Net->numUpdates && Clients[IS_IDLE] == NumClients)
      HaltCondition = NO_MORE_CLIENTS;
    else if (synchCont) synchronousContinue(C, FALSE);
  }
}

static void processClientMessage(ClientData clientData, int event) {
  static flag pending = 0;
  static flag working = FALSE;
  static int  pendEvent[256];
  char type = 0;
  Client C = (Client) clientData;
  unsigned long sTime;

  if (working) {
    pendEvent[pending++] = event;
    print(1, "%d pending\n", pending);
    return;
  }
  sTime = getTime();
  working = TRUE;
  if (pending) event = pendEvent[--pending];

  if (event == TCL_EXCEPTION) {
    error("An exception occurred for client %d", C->num);
    working = FALSE;
    return;
  }

  if (receiveChar(C->channel, &type) || type == EMPTY) {
    deleteClient(C);
    working = FALSE;
    return;
  }

  /* print(1, "Received %s\n", messName[(int) type]); */

  switch (type) {
  case DEATH:
    deleteClient(C);
    break;
  case EVAL:
    receiveClientEval(C);
    break;
  case HALT:
    receiveClientHalt(C);
    break;
  case TRAIN:
    receiveClientDerivs(C);
    break;
  case TEST:
    if (Synchronous) fatalError("Got a net error message in synchronous mode");
    if (receiveClientTestResults(C)) deleteClient(C);
    break;
  default:
    error("Got a bad message code (%d) from client %d", type, C->num);
  }
  removeDeadClients();
  working = FALSE;
  WorkingTime += getTime() - sTime;
  while (pending) processClientMessage(clientData, pendEvent[pending]);
}

static void printClient(Client C) {
  char *state;
  switch (C->state) {
  case IS_IDLE     : state = "idle"; break;
  case IS_WAITING  : state = "waiting"; break;
  case IS_TRAINING : state = "training"; break;
  case IS_TESTING  : state = "testing"; break;
  default: state = "error";
  }
  append("%-2d %-13s %-5s %-4d %s", C->num, C->hostName, 
	 Tcl_GetChannelName(C->channel), C->port, state);
}

flag clientInfo(int num) {
  Client C;
  if (ParallelState != SERVER)
    return warning("clientInfo: not running as a server");
  if (num == 0) {
    append("%d clients  %d training  %d testing  %d waiting  %d idle",
	   NumClients, Clients[IS_TRAINING], Clients[IS_TESTING], 
	   Clients[IS_WAITING], Clients[IS_IDLE]);
    FOR_EACH_CLIENT({
      append("\n");
      printClient(C);
    });
  }
  else {
    if (!(C = lookupClient(num)))
      return warning("clientInfo: client %d doesn't exist", num);
    printClient(C);
  }
  return TCL_OK;
}

flag waitForClients(int clients, char *command) {
  if (ParallelState == CLIENT) {
    startTask(WAITING);
    sendEvals("incr .ClientsWaiting", 0);
    while (!smartUpdate(TRUE)) eval("after 50");
    stopTask(WAITING);
    return TCL_OK;
  }
  
  if (ParallelState != SERVER)
    return warning("waitForClients: not running as a server");
  /* This is just the barrier wait case */
  if (clients == 0) {
    startTask(WAITING);
    while (!smartUpdate(TRUE) && ClientsWaiting < NumClients)
      eval("after 50");
    sendEvals("wait 0", 0);
    ClientsWaiting = 0;
    stopTask(WAITING);
    return TCL_OK;
  }
  if (NumClients >= clients) {
    if (command) return eval(command);
    else return TCL_OK;
  }
  if (command) { /* Set up the wait command and return immediately */
    WaitClients = clients;
    FREE(WaitCommand);
    WaitCommand = copyString(command);
  } else {       /* Wait for the clients and then return if no command */
    startTask(WAITING);
    while (!smartUpdate(TRUE) && NumClients < clients) eval("after 50");
    stopTask(WAITING);
  }
  return TCL_OK;
}

static flag deleteClient(Client C) {
  if (C->state == IS_DEAD) return TCL_ERROR;
  receiveClientHalt(C);
  changeState(C, IS_DEAD);
  sendClientDeath(C);
  Tcl_DeleteChannelHandler(C->channel, processClientMessage, C);
  Tcl_UnregisterChannel(Interp, C->channel);
  print(1, "Deleting client %d\n", C->num);
  NumClients--;
  return TCL_ERROR;
}

static void acceptClient(ClientData junk, Tcl_Channel channel, char *hostName, 
			 int port) {
  int num;
  Client C, N;
  char *com;
  C = (Client) safeCalloc(1, sizeof *C, "acceptClient:C");
  C->channel = channel;
  C->hostName = copyString(hostName);
  C->port = port;
  for (num = 1; lookupClient(num); num++);
  C->num = num;
  if (IsTraining) {
    C->state = IS_WAITING;
    Clients[IS_WAITING]++;
  }
  else {
    C->state = IS_IDLE;
    Clients[IS_IDLE]++;
  }
  /* C->fd = getFD(channel); */
  if (!ClientList) 
    ClientList = C;
  else {
    for (N = ClientList; N->next; N = N->next);
    N->next = C;
  }
  NumClients++;

  Tcl_SetChannelOption(Interp, channel, "-blocking", "1");
  Tcl_SetChannelOption(Interp, channel, "-buffering", "none");
  Tcl_SetChannelOption(Interp, channel, "-buffersize", "0");
  Tcl_SetChannelOption(Interp, channel, "-translation", "binary binary");
  Tcl_CreateChannelHandler(channel, TCL_READABLE | TCL_EXCEPTION, 
			   processClientMessage, C);
  Tcl_RegisterChannel(Interp, channel);
  result("");
  printClient(C);
  print(1, "The following client just connected:\n%s\n", 
	 Tcl_GetStringResult(Interp));

  if (IsTraining) {
    if (sendClientStart(C)) return;
    if (!Synchronous) {
      if (sendClientWeights(C) ||
	  sendClientTrain(C, 1, Net->batchSize)) return;
    }
  }
  if (WaitCommand && NumClients >= WaitClients) {
    WaitClients = 0;
    /* All this weirdness is in case the command does something that tries to
       free itself.  In that case, we don't want to free it twice. */
    com = WaitCommand;
    WaitCommand = NULL;
    if (eval(com)) error(Tcl_GetStringResult(Interp));
    FREE(com);
  }
}

flag startServer(int port) {
  Tcl_DString buf;
  if (ParallelState == SERVER)
    return warning("startServer: already acting as a server");
  if (ParallelState == CLIENT)
    return warning("startServer: already acting as a client");


  if (!(ServerChannel = Tcl_OpenTcpServer(Interp, port, NULL, acceptClient, 
					  NULL)))
    return warning("startServer failed");

  ParallelState = SERVER;
  NumClients = 0;
  ClientsWaiting = 0;
  Clients[IS_TRAINING] = Clients[IS_TESTING] = Clients[IS_IDLE] = 
    Clients[IS_WAITING] = 0;
  Tcl_DStringInit(&buf);
  Tcl_GetChannelOption(Interp, ServerChannel, "-sockname", &buf);
  /*  result("%s", Tcl_DStringValue(&buf));*/
  eval("lindex {%s} 2", Tcl_DStringValue(&buf));
  Tcl_DStringFree(&buf);
  return TCL_OK;
}

flag stopServer(void) {
  Client C;
  if (ParallelState != SERVER)
    return warning("stopServer: not running as a server");

  if (IsTraining) HaltCondition = SERVER_STOPPING;
  FOR_EACH_CLIENT(deleteClient(C));
  removeDeadClients();

  if (NumClients) return warning("stopServer: lost track of some clients");
  if (Tcl_Close(Interp, ServerChannel))
    return warning("stopServer: error closing server channel");
  ServerChannel = NULL;
  ParallelState = NONE;
  WaitClients = 0;
  FREE(WaitCommand);
  return result("");
}


/******************************** CLIENT CODE ********************************/

static flag sendServerEval(char *message) {
  if (sendEval(ServerChannel, message)) return stopClient();
  return TCL_OK;
}

static flag sendServerHalt(void) {
  if (sendChar(ServerChannel, HALT)) return stopClient();
  return TCL_OK;
}

static flag sendServerTestResults(void) {
  if (sendChar(ServerChannel, TEST) ||
      sendInt(ServerChannel, UpdatesDone) ||
      sendString(ServerChannel, Tcl_GetStringResult(Interp)))
    return stopClient();
  return TCL_OK;
}

static flag haltClient(void) {
  if (sendServerHalt()) return TCL_ERROR;
  return TCL_OK;
}

static flag sendServerDerivs(flag groupCritReached, unsigned int time) { 
  int i, buffer[PAR_BUF_SIZE], blockSize = PAR_BUF_SIZE * sizeof(int);
  float v;

  if (!Net) return haltClient();

  if (sendChar(ServerChannel, TRAIN) ||
      sendReal(ServerChannel, Net->error) ||
      sendFlag(ServerChannel, groupCritReached) ||
      sendInt(ServerChannel, time) ||
      sendInt(ServerChannel, Net->numLinks)) return stopClient();
  
  i = 0;
  FOR_EACH_GROUP({
    FOR_EACH_UNIT(G, {
      FOR_EACH_LINK(U, {
	v = (isNaN(L->deriv)) ? NaNf : (float) L->deriv;
	buffer[i++] = HTONL(*(int *) &v);
	if (i == PAR_BUF_SIZE) {
	  if (Tcl_WriteChars(ServerChannel, (char *) buffer, blockSize) 
	      != blockSize) return stopClient();
	  i = 0;
	}
      });
    });
  });
  if (i && (Tcl_WriteChars(ServerChannel, (char *) buffer, i * sizeof(int)) 
	    != i * sizeof(int))) return stopClient();
  
  return TCL_OK;
}

static flag receiveServerEval(void) {
  if (receiveEval(ServerChannel)) return stopClient();
  return TCL_OK;
}

static flag receiveServerStart(void) {
  StartTime = getTime();
  WorkingTime = 0;
  IsTraining = TRUE;
  return TCL_OK;
}

static flag receiveServerStop(void) {
  if (!IsTraining) return TCL_ERROR;
  
  print(1, "\nParallel training done\n");
  StartTime = getTime() - StartTime;
  print(1, "Client was working %.2f%% of the time (%.2f of %.2f seconds)\n",
	((real) (WorkingTime * 100) / StartTime), 
	(real) WorkingTime * 1e-3, (real) StartTime * 1e-3);
  IsTraining = FALSE;
  updateDisplays(ON_REPORT);
  return TCL_OK;
}

static flag receiveServerWeights(void) {
  Link L;

  if (receiveLinkValues(ServerChannel, OFFSET(L, weight), FALSE))
    return stopClient();
  
  updateDisplays(ON_UPDATE);
  return TCL_OK;
}

static flag receiveServerTrain(void) {
  int links, batchSize;
  flag groupCritReached, result;
  char randExample;
  unsigned int time;

  if (receiveInt(ServerChannel, &links) ||
      receiveInt(ServerChannel, &batchSize) ||
      receiveChar(ServerChannel, &randExample)) return stopClient();
  
  time = getTime();
  if (!Net || links != Net->numLinks || !Net->trainingSet)
    return haltClient();

  if (randExample) {
    Net->trainingSet->currentExampleNum = 
      randInt(Net->trainingSet->numExamples) - 1;
    Net->totalUpdates = 0;
  }
  Net->batchSize = batchSize;

  startTask(TRAINING);
  result = Net->netTrainBatch(&groupCritReached);
  stopTask(TRAINING);
  if (result == TCL_ERROR) {
    haltClient();
  } else if (IsTraining) {
    print(1, ".");
    time = getTime() - time;
    if (sendServerDerivs(groupCritReached, time)) 
      result = TCL_ERROR;
  }
  return result;
}

static flag receiveServerTest(void) {
  flag result;
  if (receiveInt(ServerChannel, &UpdatesDone)) return TCL_ERROR;

  if (!Net) return warning("receiveTest: no current network");

  startTask(TESTING);
  result = Net->netTestBatch(0, TRUE, TRUE);
  stopTask(TESTING);

  if (result == TCL_ERROR) {
    haltClient();
    return TCL_ERROR;
  }
  /* If testing was done with the training set, put it in a random state 
     so all the clients aren't synchronized */
  if (!Net->testingSet)
    Net->trainingSet->currentExampleNum = 
      randInt(Net->trainingSet->numExamples) - 1;
  if (IsTraining) {
    print(1, "\n%s\n", Tcl_GetStringResult(Interp));
    if (sendServerTestResults()) return TCL_ERROR;
  }
  return TCL_OK;
}

static void processServerMessage(ClientData clientData, int event) {
  char type;
  static flag working = FALSE;

  unsigned long sTime = getTime();
  /* if (working) error("Got a message while working"); */
  working = TRUE;
  if (event == TCL_EXCEPTION) {
    error("An exception occurred for the channel");
    working = FALSE;
    return;
  }

  if (receiveChar(ServerChannel, &type) || type == EMPTY) {
    stopClient();
    working = FALSE;
    return;
  }

  /* print(1, "Received %s\n", messName[(int) type]); */

  switch (type) {
  case DEATH:
    stopClient();
    break;
  case EVAL:
    receiveServerEval();
    break;
  case WEIGHTS:
    receiveServerWeights();
    break;
  case TRAIN:
    receiveServerTrain();
    break;
  case TEST:
    receiveServerTest();
    break;
  case START:
    receiveServerStart();
    break;
  case STOP:
    working = FALSE; /* Because receive stop does an update */
    receiveServerStop();
    break;
  default:
    error("Got a bad message code (%d) from the server", type);
    stopClient();
  }
  working = FALSE;
  WorkingTime += getTime() - sTime;
}

flag startClient(char *server, int port, char *myname) {
  if (ParallelState == SERVER)
    return warning("startClient: already acting as a server");
  if (ParallelState == CLIENT)
    return warning("startClient: already acting as a client");

  if (!(ServerChannel = Tcl_OpenTcpClient(Interp, port, server, myname, 0, 0)))
    return warning("startClient failed");
  ParallelState = CLIENT;
  Tcl_SetChannelOption(Interp, ServerChannel, "-blocking", "1");
  Tcl_SetChannelOption(Interp, ServerChannel, "-buffering", "none");
  Tcl_SetChannelOption(Interp, ServerChannel, "-buffersize", "0");
  Tcl_SetChannelOption(Interp, ServerChannel, "-translation", "binary binary");
  Tcl_RegisterChannel(Interp, ServerChannel);
  Tcl_CreateChannelHandler(ServerChannel, TCL_READABLE | TCL_EXCEPTION, 
			   processServerMessage, NULL);
  print(1, "Connected to server on channel \"%s\"\n", 
	Tcl_GetChannelName(ServerChannel));
  return TCL_OK;
}

flag stopClient(void) {
  /* send stop signal */
  if (ParallelState != CLIENT)
    return warning("stopClient: not running as a client");
  IsTraining = FALSE;
  ParallelState = NONE;
  sendDeath(ServerChannel);
  Tcl_DeleteChannelHandler(ServerChannel, processServerMessage, NULL);
  if (Tcl_UnregisterChannel(Interp, ServerChannel))
    return warning("stopClient: error closing client channel");
  ServerChannel = NULL;
  print(1, "Client stopped\n");
  return TCL_ERROR;
}
