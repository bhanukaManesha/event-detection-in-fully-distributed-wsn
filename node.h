void node();
int recieveTriggerFromAdjacent(int* adjacentNodes, uint8_t* recievePackBuffer, MPI_Request* req, int* nreq);
int sendTrigger(int* adjacentNodes, MPI_Request* req, int* nreq);
int getRandomNumber();
int checkForTrigger(int* recievedNumPast, int* recievedNumCurrent);
void initializeNodes();