/* 
File: init.h
Author: Bhanuka Gamage
Date: 14th October 2019
StudentID - 28993373
Assignment 2
*/

// Define the macros
#define MAX_RANDOM 12
#define NUMBEROFADJACENT 4
#define packsize 256
#define ENCRYPT_COMM 1

// Define the Encryption type
#define CBC 1
#define CTR 1
#define ECB 1


// Include standard interger types
#include "stdint.h"
#include "inttypes.h"

// Functions Definitions for the node.c file
void convertToTimeStamp(char* buf, int size);
int getAdjacentNodes(int *ajacentNodesArr, int currentRank);
void* checkStop(void * arg);
void printBanner();
void initializeSystem();
void encrypt_decrypt(uint8_t* buffer, uint32_t size);

// Include the AES encyption file
#include "aes.h"

