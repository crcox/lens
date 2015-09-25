#ifndef PARALLEL_H
#define PARALLEL_H

enum pState {NONE, SERVER, CLIENT};

extern flag startServer(int port);
extern flag stopServer(void);
extern flag startClient(char *server, int port, char *myname);
extern flag stopClient(void);
extern flag sendEvals(char *message, int num);
extern flag clientInfo(int num);
extern flag trainParallel(flag synch, int testInt);
extern flag waitForClients(int clients, char *command);

extern int  ParallelState;
extern int  ClientsWaiting;

#endif /* PARALLEL_H */
