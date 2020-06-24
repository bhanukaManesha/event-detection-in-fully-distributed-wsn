/* 
File: node.h
Author: Bhanuka Gamage
Date: 14th October 2019
StudentID - 28993373
Assignment 2
*/

// Functions Definitions for the node.c file
void node();
int recieveTriggerFromAdjacent(int* adjacentNodes, uint8_t* recievePackBuffer, MPI_Request* req, int* nreq);
int sendTrigger(int* adjacentNodes, MPI_Request* req, int* nreq);
int getRandomNumber();
int checkForTrigger(int* recievedNumPast, int* recievedNumCurrent);
void initializeNodes();