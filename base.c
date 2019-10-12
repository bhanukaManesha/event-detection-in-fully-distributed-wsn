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

extern int numtasks;
extern int rank;
extern int baseStation;

extern int WIDTH;
extern int HEIGHT;

extern char* macAddressStorage;
extern char* ipAddressStorage;

extern int userstop;
extern double simStartTime;

char* macAddressStorage;
char* ipAddressStorage;

extern struct AES_ctx ctx;
extern uint8_t key[];
extern uint8_t iv[];

// --------------------------------------------------------------------------------------------------------------------

// void initializeBaseStation();

// void base();
// void listenToEvents();

// void convertToTimeStamp(char* buf, int size);

// --------------------------------------------------------------------------------------------------------------------


void base(){

	macAddressStorage = (char*)  malloc(WIDTH * HEIGHT * 17 * sizeof(char));
	ipAddressStorage = (char*) malloc(WIDTH * HEIGHT * 15 * sizeof(char));

	initializeBaseStation();


	listenToEvents();
}

void listenToEvents(){
	MPI_Status stat;

	// uint32_t packsize;

	// if (ENCRYPT_DEMO == 0){
	// 	packsize = (sizeof(int) * 5 + sizeof(double) + (100 * sizeof(char)));
	// }
	
	uint8_t packbuf[packsize];


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


	while (1){
		position = 0;

		MPI_Recv(packbuf, packsize, MPI_PACKED, MPI_ANY_SOURCE , 1, MPI_COMM_WORLD, &stat);
		
		double decryptStartTime = MPI_Wtime();

		if (ENCRYPT_COMM == 1){
			AES_init_ctx_iv(&ctx, key, iv);
			AES_CTR_xcrypt_buffer(&ctx, packbuf, packsize);
		}

		double decryptionTime = MPI_Wtime() - decryptStartTime;


		totalMessages += 1;

		incomingNode = stat.MPI_SOURCE;
		MPI_Unpack(packbuf, packsize, &position, &matchedValue, 1, MPI_INT, MPI_COMM_WORLD);
		
		// Stopping message
		if (matchedValue == MAX_RANDOM + 2){
			int temp;
			MPI_Unpack(packbuf, packsize, &position, &temp, 1, MPI_INT, MPI_COMM_WORLD);
			totalInnerMessages += temp;

			if (stopCount == (WIDTH * HEIGHT) - 1){
				// printf("Incoming %i\n", incomingNode);
				break;
			}
			stopCount+= 1;
			continue;
		}


		MPI_Unpack(packbuf, packsize, &position, &activatedNodes, 4, MPI_INT, MPI_COMM_WORLD);
		MPI_Unpack(packbuf, packsize, &position, &eventT, 1, MPI_DOUBLE, MPI_COMM_WORLD);
		MPI_Unpack(packbuf, packsize, &position, &eventDateTime, 100, MPI_CHAR, MPI_COMM_WORLD);
		

		char timeInDateTime[100];
		convertToTimeStamp(timeInDateTime, 100);

		// Get the adjacent nodes
		getAdjacentNodes(adjacentNodes, incomingNode);

		double commTime =  MPI_Wtime() - eventT;

		int iterationNumber = 0;

		for (int k = 0; k < 4; k++){
			if (iterationNumber < activatedNodes[k]){
				iterationNumber = activatedNodes[k];
			}
		} 
		

		
		FILE *fp;
		fp = fopen("log.txt", "a+");

		fprintf (fp, "%s", "------------------------------------------------------\n");
		fprintf(fp, "Iteration : %i\n", iterationNumber);
		fprintf (fp, "Logged Time : \t\t\t\t%s\n", timeInDateTime);

		fprintf (fp, "Event Occured Time : \t\t%s\n", eventDateTime);
		// fwrite(eventDateTime , 80 , sizeof(char) , fp );

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

		for (int i = 0; i < 4; i++){
			// printf("activated Node : %i\n", activatedNodes[i]);
			if (activatedNodes[i] != -1){
				totalActivations+=1;
				// printf("test\n\n");
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
		fprintf(fp, "Decyption Time (seconds) : %f\n", decryptionTime);
		fprintf (fp, "Total Messages with server: %i\n", totalMessages);
		fprintf (fp, "Total Activations : %d\n", totalActivations);

		fclose(fp);


	}
	

	FILE *fp;
	fp = fopen("log.txt", "a+");

	fprintf (fp, "%s", "\n\n");
	fprintf (fp, "%s", "------------------------------------------------------\n");
	fprintf (fp, "%s", "------------------------------------------------------\n");
	fprintf (fp, "Total Simulation Time (seconds) : %f\n", MPI_Wtime() - simStartTime);
	fprintf (fp, "Total Messages though the network (including termination signal): %i\n", (totalMessages + totalInnerMessages));
	fprintf (fp, "Total Activations : %d\n", totalActivations);

	fclose(fp);

	free(ipAddressStorage);
	free(macAddressStorage);

}


 void initializeBaseStation(){

	MPI_Status stat;
	// uint32_t packsize;

	// if (ENCRYPT_DEMO == 0){
	// 	packsize = 32 * sizeof(unsigned char);
	// }


	uint8_t packbuf[packsize];
	int incoming_rank;

	int position = 0;
	int count = 0;

	while (count < (WIDTH * HEIGHT)){

		position = 0;
		MPI_Recv(packbuf, packsize, MPI_PACKED, MPI_ANY_SOURCE , 0, MPI_COMM_WORLD, &stat);

		if (ENCRYPT_COMM == 1){
			AES_init_ctx_iv(&ctx, key, iv);
			AES_CTR_xcrypt_buffer(&ctx, packbuf, packsize);
		}

		incoming_rank = stat.MPI_SOURCE;
		MPI_Unpack(packbuf, packsize, &position, macAddressStorage + incoming_rank*sizeof(unsigned char)*17, 17, MPI_CHAR, MPI_COMM_WORLD);
		MPI_Unpack(packbuf, packsize, &position, ipAddressStorage + incoming_rank*sizeof(unsigned char)*15, 15, MPI_CHAR, MPI_COMM_WORLD);

		count += 1;

	}

 }


