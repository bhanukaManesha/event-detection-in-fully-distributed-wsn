/* 
File: node.c
Author: Bhanuka Gamage
Date: 14th October 2019
StudentID - 28993373
Assignment 2
*/

// Including the libraries
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <mpi.h>
#include <string.h>
#include <time.h> 
#include <unistd.h>
#include <pthread.h>

#include "./init.h"
#include "./node.h"
#include "getIPAddress.c"
#include "getMACAddressUbuntu.c"


// Import the global variables
extern int numtasks;
extern int rank;
extern int baseStation;
extern int userstop;

extern int iterationMax;
extern int refreshInteval;

extern int WIDTH;
extern int HEIGHT;

extern struct AES_ctx ctx;
extern uint8_t key[];
extern uint8_t iv[];

// Initialize global variables
int iterationCount = 1;
int totalInterNodeMessageCount;

// --------------------------------------------------------------------------------------------------------------------

int getRandomNumber(){
	/*
	 * Method use to generate the random number
	*/

	// Setting the seed
	srand((int) time(NULL) ^ rank);
	srand(rand());

	// Generate the random numbers
	return rand() % MAX_RANDOM;
}


 void node(){
	 /*
	  * Method executed by each node in the WSN
	 */

	// Method to initialize the nodes
	initializeNodes();

	// Initialize the array to store the adjacent nodes
	int adjacentNodes[NUMBEROFADJACENT] = {-1,-1, -1, -1};

	// Initialize the arrays to store the recieved the random number
	int recievedNumPast[NUMBEROFADJACENT] = {MAX_RANDOM, MAX_RANDOM, MAX_RANDOM, MAX_RANDOM};
	int recievedNumCurrent[NUMBEROFADJACENT] = {MAX_RANDOM, MAX_RANDOM, MAX_RANDOM, MAX_RANDOM};

	// Initialize array to store the pack buffers used in non blocking communication
	uint8_t recievePackBuffer[packsize * NUMBEROFADJACENT];
	memset(recievePackBuffer, 0, sizeof(uint8_t) * packsize * NUMBEROFADJACENT);

	// Get the adjacent nodes
	getAdjacentNodes(adjacentNodes, rank);

	// Initialize the local variables
	MPI_Request temp_req;
	MPI_Status temp_stat;

	// Set the user flag to zero
	int usflag = 0;

	// Get the non blocking recieve from the POSIX thread
	MPI_Irecv(&userstop, 1, MPI_INT, baseStation, 3, MPI_COMM_WORLD, &temp_req);

	// Loop until break
	while (1){

		// Initialize request and status arrays for non blocking send
		MPI_Request requests[2 * NUMBEROFADJACENT];
		MPI_Status statuses[2 * NUMBEROFADJACENT];

		// Initialize count for number of requests
		int nreq = 0;
		
		// Generate random numbers and send to adjacent nodes
		sendTrigger(adjacentNodes, requests, &nreq);

		// Recieve packed buffer with random numbers from adjacent nodes
		recieveTriggerFromAdjacent(adjacentNodes, recievePackBuffer, requests, &nreq);

		// Wait for all the non blocking communication to end
		MPI_Waitall(nreq , requests, statuses);

		// After receiveing packed buffers with random numbers, loop though them
		for (int index = 0; index < NUMBEROFADJACENT; index++){

			// Initialize the position to zero
			int position = 0;

			// If there is an adjacent node
			if (adjacentNodes[index] != -1){
				
				// Decrypt the packed buffer with the random number
				if (ENCRYPT_COMM == 1){
					encrypt_decrypt(recievePackBuffer + (packsize * index),packsize);
				}
			
				// Unpack the random number
				MPI_Unpack((recievePackBuffer + (packsize * index)), packsize, &position, &recievedNumCurrent[index], 1, MPI_INT, MPI_COMM_WORLD);
			}
			
		}

		// Check if there is an event and inform base station
		checkForTrigger(recievedNumPast, recievedNumCurrent);

		// Copy the random numbers from current iteration and store it in the past iteration array
		memcpy(recievedNumPast, recievedNumCurrent, sizeof(int) * 4);

		// Increase the iterationCount
		iterationCount += 1;

		// Check if the user wants to stop the iterations
		MPI_Test(&temp_req, &usflag, &temp_stat);
				
		// If the iterationMax is reached, then break from the loop
		if (iterationMax != -1){
			if (iterationCount > iterationMax){
				break;
			}
		}

		// If user wants to stop the iterations, break from the loop
		if (userstop == 1){
			break;
		}

		// Sleep to have an interval between each iteration
   		sleep(refreshInteval);

	}

	// If the iterations are ended
	
	// Initialize the stop variable
	int stop = MAX_RANDOM + 2;

	// Set the position to zero
	int position = 0;
	
	// Initialize the pack buffer with zeros
	uint8_t packbuf[packsize];
	memset(packbuf, 0, sizeof(uint8_t) * packsize);

	// Pack the buffer with the stop variable and the total inter node message count
	MPI_Pack(&stop, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
	MPI_Pack(&totalInterNodeMessageCount, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
	
	// Encrypt the buffer
	if (ENCRYPT_COMM == 1){
		encrypt_decrypt(packbuf,packsize);
	}

	// Send the data to the base station to exit gracefully
	MPI_Send(packbuf, packsize, MPI_PACKED, baseStation, 1, MPI_COMM_WORLD);

}

void initializeNodes(){
	/*
	 * Method used to initialize nodes
	 */

	// Initialize the pack buffer with zeros
	uint8_t packbuf[packsize];
	memset(packbuf, 0, sizeof(uint8_t) * packsize);

	// Set the position to zero
	int position = 0;

	// Create pointer to store the ip address
	unsigned char ip_address[15];
	memset(ip_address, 0, sizeof(unsigned char) * 15);
	getIPAddress(ip_address);

	// Hard code the mac address, since MAC doesnot support accessing the mac address
	// unsigned char mac_address[17] = "78:4f:43:5b:c2:c3";
	unsigned char mac_address[17];
	memset(mac_address, 0, sizeof(unsigned char) * 17);
	getMACAddress(mac_address);
	
	// Pack the mac address and the ipaddress to the buffer
	MPI_Pack( mac_address, 17, MPI_UNSIGNED_CHAR, packbuf, packsize, &position, MPI_COMM_WORLD );
	MPI_Pack( ip_address, 15, MPI_UNSIGNED_CHAR, packbuf, packsize, &position, MPI_COMM_WORLD );

	// Encrypt the buffer
	if (ENCRYPT_COMM == 1){
		encrypt_decrypt(packbuf,packsize);
	}
	
	// Send the ip and mac to the base station to be stored
	MPI_Send(packbuf, packsize, MPI_PACKED, baseStation, 0, MPI_COMM_WORLD);

}

int checkForTrigger(int* recievedNumPast, int* recievedNumCurrent){
	/*
	 * Method used to check if there is an event triggered
	 */ 

	// Initialize two arrays to store the level 1 and level 2 iterations
	int sendArrayLevel1[NUMBEROFADJACENT];
	int sendArrayLevel2[NUMBEROFADJACENT];

	// Initialize a variable to count level 1 and level 2 events
	int level1Count;
	int level2Count;

	// Flags for level 1 and 2 events
	int level1event = 0;
	int level2event = 0;

	// Variable to store the level 1 and 2 random number which matched
	int level1match = 0;
	int level2match = 0;

	// Detecting level 2 events
	// Loop from 0 to 4
	for (int i=0; i < NUMBEROFADJACENT;i++){

		// Set the send array to -1
		memset(sendArrayLevel2,-1, sizeof(int) *4);

		// Initialize the count to 1
		int level2Count = 1;

		// Loop from 0 to 4
		for (int j = i + 1; j < NUMBEROFADJACENT; j++){

			// Check if there is any trigger value matching with the past iteration
			if (recievedNumCurrent[i] == recievedNumPast[j] && recievedNumCurrent[i] != MAX_RANDOM && i != j){
				// Store the iteration number
				sendArrayLevel2[i] = iterationCount - 1;
				sendArrayLevel2[j] = iterationCount - 1;

				// Store the trigger value
				level2match = recievedNumCurrent[i];

				// Incease the level count
				level2Count+=1;
			
			// Check if any trigger value matches with the current iteration
			}else if(recievedNumCurrent[i] == recievedNumCurrent[j] && recievedNumCurrent[i] != MAX_RANDOM && i != j){
				// Store the iteration number
				sendArrayLevel2[i] = iterationCount;
				sendArrayLevel2[j] = iterationCount;

				// Store the trigger value
				level2match = recievedNumCurrent[i];

				// Incease the level count
				level2Count+=1;
			}

		}

		// If there are more than three similar numbers
		if (level2Count >= 3){
			// Mark as event and break
			level2event = 1;
			break;
		}
	
	}


	// Loop from 0 to 2
	for (int i = 0; i < NUMBEROFADJACENT / 2;i++){
		
		// Set the send array to -1
		memset(sendArrayLevel1,-1,sizeof(int) *4);

		// Initialize the count to 1
		int level1Count = 1;

		// Loop from 0 to 4
		for (int k = i + 1; k < NUMBEROFADJACENT; k++){

			// Check if there is a matching random number in the same iteration
			if (recievedNumCurrent[i] == recievedNumCurrent[k] & recievedNumCurrent[i] != MAX_RANDOM){
				// Store the iteration number
				sendArrayLevel1[i] = iterationCount;
				sendArrayLevel1[k] = iterationCount;

				// Store the trigger value
				level1match = recievedNumCurrent[i];

				// Incease the level count
				level1Count += 1;
			}
		}
		
		// If there are more than three similar numbers
		if (level1Count >= 3){
			// Mark as event and break
			level1event = 1;
			break;
		}

	}
	
	// Check if there is both a level 1 and level 2 event
	if (level1event == 1 || level2event == 1 ){

		// Open the file and print the seperator
		FILE* fp;
		char path[20];
		sprintf(path, "./nodes/%d.txt", rank);
		fp = fopen(path, "a+");
		fprintf (fp, "%s", "\n\n------------------------------------------------------\n");
		
		// Uncomment to write node data to csv
		// FILE *dp;
		// sprintf(path, "./nodes/%d.csv", rank);
		// dp = fopen(path, "a+");

		// Initialize the pack buffer with zeros
		uint8_t packbuf[packsize];
		memset(packbuf, 0, sizeof(uint8_t) * packsize);

		// Initialize the position
		int position = 0;

		// Set the send flag to 0
		int sendFlag = 0;

		// If there is a level 1 event
		if (level1event == 1){

			// Set the send flag to 1
			sendFlag = 1;

			// Pack the trigger value and the iteration numbers to be sent
			MPI_Pack( &level1match, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
			MPI_Pack( &sendArrayLevel1, 4, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
			
		// If it is only a level 2 event
		}else if (level2event == 1 && level1event == 0 ){

			// Set the send flag to 1
			sendFlag = 1;

			// Pack the trigger value and the iteration numbers to be sent
			MPI_Pack( &level2match, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
			MPI_Pack( &sendArrayLevel2, 4, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );

		}

		// If the send flag is true
		if (sendFlag) {

			// Get the event time to calculate communication time
			double eventTime = MPI_Wtime();
			
			// Pack the event time
			MPI_Pack( &eventTime, 1, MPI_DOUBLE, packbuf, packsize, &position, MPI_COMM_WORLD );

			// Get the current time to be used for logginf
			char timestamp[128];
			convertToTimeStamp(timestamp, 128);
			MPI_Pack( timestamp, 128, MPI_CHAR, packbuf, packsize, &position, MPI_COMM_WORLD );
			
			// Write the original message to the node file
			fprintf(fp, "\nOriginal Message : \n");
			fwrite(&packbuf , packsize , sizeof(char) , fp );

			// Get the time to calculate the encryption time
			double encyptStartTime = MPI_Wtime();

			// Encrypt the message
			if (ENCRYPT_COMM == 1){
				encrypt_decrypt(packbuf,packsize);
			}


			// Get the encryption time
			double encyptionTime = MPI_Wtime() - encyptStartTime;

			// Send the data to the base station
			MPI_Send(packbuf, packsize, MPI_PACKED, baseStation, 1, MPI_COMM_WORLD);

			// Write the encryption time on the file
			fprintf(fp, "\n\nEncryption Time : %f\n", encyptionTime);
			
			// Uncomment to write node data to csv
			// fprintf (dp, "%f\n", encyptionTime);
		
			// Write the encrypted message on the file
			fprintf(fp, "\nEncrypted Message : \n");
			fwrite(&packbuf , packsize , sizeof(char) , fp );

			// Close the file
			fclose(fp);

			// Uncomment to write node data to csv
			// fclose(dp);

		}

	}
	return 0;
}

int recieveTriggerFromAdjacent(int* adjacentNodes, uint8_t* recievePackBuffer, MPI_Request* req, int* nreq){
	/*
	 * Method to receive the random numbers from the adjacent nodes using non blocking send
	 */

	// Loop though all the adjacent nodes
	for (int index = 0; index < NUMBEROFADJACENT; index++){
		if (adjacentNodes[index] != -1){
			
			
			// Recieve the packed buffer from the adjacent nodes which contain the random number
			MPI_Irecv((recievePackBuffer + (packsize * index)), packsize, MPI_PACKED, adjacentNodes[index], 0, MPI_COMM_WORLD, &req[*nreq]);

			// Increase the request count
			*nreq = *nreq + 1;
		}
		
	}

	return 0;

}



int sendTrigger(int* adjacentNodes, MPI_Request* req, int* nreq){
	/*
	 * Method used to generate a random number and send to adjacent nodes 
	 */ 

	// Initialize the variable to store the random number
	int randNum;

	// Generate the random number
	randNum = getRandomNumber(rank);

	// Loop though all the adjacent nodes
	for (int index = 0; index < NUMBEROFADJACENT; index++){

		// Increment the total inner node message count
		totalInterNodeMessageCount +=1;
		
		// If there exist an adjacent node
		if (adjacentNodes[index] != -1){
			
			// Initialize a buffer with zeros
			uint8_t packbuf[packsize];
			memset(packbuf, 0, sizeof(uint8_t) * packsize);

			// Initialize the position to be zero
			int position = 0;

			// Pack the random number to the buffer
			MPI_Pack( &randNum, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );

			// Encrypt the buffer
			if (ENCRYPT_COMM == 1){
				encrypt_decrypt(packbuf,packsize);
			}

			// Send the data to the adjacent nodes using non blocking send
			MPI_Isend(packbuf, packsize, MPI_PACKED, adjacentNodes[index], 0, MPI_COMM_WORLD, &req[*nreq]);

			// Increament the request count
			*nreq = *nreq + 1;
		}
		
	}

	return 0;
}