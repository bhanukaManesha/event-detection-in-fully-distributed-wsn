/* 
File: base.c
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
#include "./base.h"

// Import the global variables
extern int numtasks;
extern int rank;
extern int baseStation;

extern int WIDTH;
extern int HEIGHT;

extern char* macAddressStorage;
extern char* ipAddressStorage;

extern int userstop;
extern double simStartTime;

extern struct AES_ctx ctx;
extern uint8_t key[];
extern uint8_t iv[];

// Initialize global variables
char* macAddressStorage;
char* ipAddressStorage;


// --------------------------------------------------------------------------------------------------------------------
void base(){
	/*
	 * Method executed by the base station in the WSN
	 */ 

	// Create two dynamic arrays to store the mac and ip addresses
	macAddressStorage = (char*)  malloc(WIDTH * HEIGHT * 17 * sizeof(char));
	ipAddressStorage = (char*) malloc(WIDTH * HEIGHT * 15 * sizeof(char));

	// Initialize the base station
	initializeBaseStation();

	// Start listening to events
	listenToEvents();

}

void listenToEvents(){
	/*
	 * Method used to listen to events and log them to a file
	 */

	// Initialize the status object
	MPI_Status stat;
	
	// Initialize the pack buffer with zeros
	uint8_t packbuf[packsize];
	memset(packbuf, 0, packsize);

	// Initialize the local variables
	int activatedNodes[4];
	int incomingNode;
	int matchedValue;
	
	char eventDateTime[100];
	int position;
	double eventT;
	int stopCount = 0;

	int adjacentNodes[NUMBEROFADJACENT];

	int totalMessages = 0;
	int totalActivations = 0;
	int totalInnerMessages = 0;

	int incomingNodeCount[(WIDTH * HEIGHT) + 1];
	memset(incomingNodeCount, 0, sizeof(int) * ((WIDTH * HEIGHT) + 1));

	// Loop until break
	while (1){

		// Set the position to zero
		position = 0;

		// Recieve the event information buffer from node
		MPI_Recv(packbuf, packsize, MPI_PACKED, MPI_ANY_SOURCE , 1, MPI_COMM_WORLD, &stat);
		
		// Get the decryption time
		double decryptStartTime = MPI_Wtime();

		// Decrypt the message
		// if (ENCRYPT_COMM == 1){
		// 	AES_init_ctx_iv(&ctx, key, iv);
		// 	AES_CTR_xcrypt_buffer(&ctx, packbuf, packsize);
		// }

		if (ENCRYPT_COMM == 1){
			encrypt_decrypt(packbuf,packsize);
		}

		

		// Calculate the decryption time
		double decryptionTime = MPI_Wtime() - decryptStartTime;

		// Increment the number of total messages
		totalMessages += 1;

		// Get the rank of the recieved node
		incomingNode = stat.MPI_SOURCE;

		// Unpack the first value of the message
		MPI_Unpack(packbuf, packsize, &position, &matchedValue, 1, MPI_INT, MPI_COMM_WORLD);
		
		// if the first value is greater than the MAX_RANDOM number
		if (matchedValue > MAX_RANDOM){

			// Initialize a temp variable
			int tempInnerMessageCount;

			// Unpack the second value
			MPI_Unpack(packbuf, packsize, &position, &tempInnerMessageCount, 1, MPI_INT, MPI_COMM_WORLD);
			
			// Increment the total message count
			totalInnerMessages += tempInnerMessageCount;

			// If all the nodes have sent stop messages
			if (stopCount == (WIDTH * HEIGHT) - 1){
				// break out of the loop
				break;
			}
			
			// Increment the stop count
			stopCount+= 1;

			// Else continue and wait for other nodes
			continue;
		}

		// Unpack the event information
		MPI_Unpack(packbuf, packsize, &position, &activatedNodes, 4, MPI_INT, MPI_COMM_WORLD);
		MPI_Unpack(packbuf, packsize, &position, &eventT, 1, MPI_DOUBLE, MPI_COMM_WORLD);
		MPI_Unpack(packbuf, packsize, &position, &eventDateTime, 100, MPI_CHAR, MPI_COMM_WORLD);
		
		// Get the current time as a date time string
		char timeInDateTime[100];
		convertToTimeStamp(timeInDateTime, 100);

		// Get the adjacent nodes of the recieved node
		getAdjacentNodes(adjacentNodes, incomingNode);

		// Calcualte the communication time
		double commTime =  MPI_Wtime() - eventT;

		// Set the iteration number to any value which is not equal to zero
		int iterationNumber;
		for (int k = 0; k < 4; k++){
			if (activatedNodes[k] != -1){
				iterationNumber = activatedNodes[k];
			}
		} 

		// Set the highest value as the current iteration
		for (int k = 0; k < 4; k++){
			if (activatedNodes[k] > iterationNumber && activatedNodes[k] != -1){
				iterationNumber = activatedNodes[k];
			}
		} 
		
		// Increment the node activation count
		incomingNodeCount[incomingNode] += 1;

		// Uncomment to write statistics to csv
		FILE *xp;
		xp = fopen("stats.csv", "a+");
		
		// Open file for logging
		FILE *fp;
		fp = fopen("log.txt", "a+");

		// Logging the data of each triggered event
		fprintf (fp, "%s", "------------------------------------------------------\n");
		fprintf(fp, "Iteration : %i\n", iterationNumber);
		fprintf (fp, "Logged Time : \t\t\t\t%s\n", timeInDateTime);
		fprintf (fp, "Event Occured Time : \t\t%s\n", eventDateTime);
		fprintf (fp, "%s", "\n");
		fprintf (fp, "%s", "Activated Node\n");
		fprintf (fp, "%i", incomingNode);
		fprintf (fp, "%s", "\t\t");
		fwrite(macAddressStorage + incomingNode*sizeof(unsigned char)*17 , 17 , sizeof(unsigned char) , fp );
		// fprintf (fp, "%s",macAddressStorage + incomingNode*sizeof(unsigned char)*17);
		fprintf (fp, "%s", "\t\t");
		// fwrite(ipAddressStorage + incomingNode*sizeof(unsigned char)*15 , 15 , sizeof(unsigned char) , fp );
		fprintf (fp, "%s", ipAddressStorage + incomingNode*sizeof(char)*15);
		fprintf (fp, "%s", "\n\n");
		int labelFlag = 0;
		int totalActivationPerMessage = 0;
		for (int i = 0; i < 4; i++){
			if (activatedNodes[i] != -1){
				totalActivations+=1;
				totalActivationPerMessage+=1;
				if (labelFlag == 0){
					fprintf (fp, "%s", "Adjacent Nodes\n");
					labelFlag = 1;
				}
				fprintf (fp, "%i", adjacentNodes[i]);
				fprintf (fp, "%s", "\t\t");
				fwrite(macAddressStorage + adjacentNodes[i]*sizeof(unsigned char)*17 , 17 , sizeof(unsigned char) , fp );
				// fprintf (fp, "%s", macAddressStorage + adjacentNodes[i]*sizeof(char)*17);
				fprintf (fp, "%s", "\t\t");
				// fwrite(ipAddressStorage + activatedNodes[i]*sizeof(unsigned char)*15 , 15 , sizeof(unsigned char) , fp );
				fprintf (fp, "%s", ipAddressStorage + adjacentNodes[i]*sizeof(char)*15);
				fprintf (fp, "%s", "\t\t");
				fprintf (fp, "%i", activatedNodes[i]);
				fprintf (fp, "%s", "\n");
				}
			}
		fprintf (fp, "%s", "\n\n");
		fprintf (fp, "Triggered Value : %i\n", matchedValue);
		fprintf (fp, "Communication Time (seconds) : %f\n", commTime);
		fprintf(fp, "Decryption Time (seconds) : %f\n", decryptionTime);
		fprintf (fp, "Total Messages with server: %i\n", totalMessages);
		fprintf (fp, "Total Activations per Message: %d\n", totalActivationPerMessage);
		fprintf (fp, "Total Activations : %d\n", totalActivations);
		// Uncomment to write statistics to csv
		fprintf (xp, "%d,%d,%d,%d,%f,%f\n", iterationNumber,totalActivations,totalActivationPerMessage,totalMessages,commTime,decryptionTime);
		fclose(xp);
		fclose(fp);
	}
	
	// Log the total statistics of of the simulation
	FILE *fp;
	fp = fopen("log.txt", "a+");
	fprintf (fp, "%s", "\n\n");
	fprintf (fp, "%s", "------------------------------------------------------\n");
	fprintf (fp, "%s", "------------------------------------------------------\n");
	fprintf (fp, "Total Simulation Time (seconds) : %f\n", MPI_Wtime() - simStartTime);
	fprintf (fp, "Total Events detected : %i\n", totalMessages - (numtasks - 1));
	fprintf (fp, "Total Messages with the base station (including termination signal): %i\n", totalMessages);
	fprintf (fp, "Total Messages between sensor nodes : %i\n", totalInnerMessages);
	fprintf (fp, "Total Messages though the network (including termination signal): %i\n", (totalMessages + totalInnerMessages));
	fprintf (fp, "Total Activations : %d\n", totalActivations);
	fprintf (fp, "%s", "\n");
	fprintf (fp, "Total Activations per Node: \n");
	for (int b = 1; b < (WIDTH * HEIGHT) + 1; b++){
			fprintf (fp, "\tRank %i : %d\n", b, incomingNodeCount[b]);
	} 
	fclose(fp);

	// Free the dynamic array
	free(ipAddressStorage);
	free(macAddressStorage);
}


 void initializeBaseStation(){
	 /*
	  * This method is used to initialize the base station by collecting the MAC and IP Addresses
	  */

	// Initialize the status object 
	MPI_Status stat;

	// Initialize a pack bufffer with zeros
	uint8_t packbuf[packsize];
	memset(packbuf, 0, packsize);

	int incoming_rank;
	int position = 0;
	int count = 0;

	// Loop tough all the nodes
	while (count < (WIDTH * HEIGHT)){
		// Initialize the position to 0
		position = 0;

		// Recieve the buffer with ip and mac from the nodes
		MPI_Recv(packbuf, packsize, MPI_PACKED, MPI_ANY_SOURCE , 0, MPI_COMM_WORLD, &stat);

		// Decrypt the buffer
		if (ENCRYPT_COMM == 1){
			encrypt_decrypt(packbuf,packsize);
		}

		// Get the rank of the incoming node
		incoming_rank = stat.MPI_SOURCE;

		// Unpack the buffer straight into the dynamic array
		MPI_Unpack(packbuf, packsize, &position, macAddressStorage + incoming_rank*sizeof(unsigned char)*17, 17, MPI_CHAR, MPI_COMM_WORLD);
		MPI_Unpack(packbuf, packsize, &position, ipAddressStorage + incoming_rank*sizeof(unsigned char)*15, 15, MPI_CHAR, MPI_COMM_WORLD);

		// Increase the count 
		count += 1;

	}

 }


