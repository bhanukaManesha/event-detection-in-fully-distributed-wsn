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
// #include "./base.h"


// #include "AES/aes.h"
// #include "AES/aes.c"

#include "getIPAddress.c"

extern int numtasks;
extern int rank;
extern int baseStation;
extern int userstop;

extern int iterations;

extern int WIDTH;
extern int HEIGHT;



extern int refreshInteval;

int iterationCount = 1;
int totalInterNodeMessageCount;

extern struct AES_ctx ctx;
extern uint8_t key[];
extern uint8_t iv[];


// void node();
// int recieveTriggerFromAdjacent(int* adjacentNodes, uint8_t* recievePackBuffer, MPI_Request* req, int* nreq);
// int sendTrigger(int* adjacentNodes, MPI_Request* req, int* nreq);
// int getRandomNumber();
// int getAdjacentNodes(int *ajacentNodesArr, int currentRank);
// int checkForTrigger(int* recievedNumPast, int* recievedNumCurrent);
// void initializeNodes();


// --------------------------------------------------------------------------------------------------------------------

int getRandomNumber(){
	// Setting the seed
	srand((int) time(NULL) ^ rank);
	srand(rand());
	return rand() % MAX_RANDOM;
	
}


 void node(){

	initializeNodes();


	// Generate the adjacent nodes
	int adjacentNodes[NUMBEROFADJACENT] = {-1,-1, -1, -1};
	int recievedNumPast[NUMBEROFADJACENT] = {MAX_RANDOM, MAX_RANDOM, MAX_RANDOM, MAX_RANDOM};
	int recievedNumCurrent[NUMBEROFADJACENT] = {MAX_RANDOM, MAX_RANDOM, MAX_RANDOM, MAX_RANDOM};

	uint8_t recievePackBuffer[packsize * NUMBEROFADJACENT];
	memset(recievePackBuffer, 0, packsize * NUMBEROFADJACENT);

	// Get the adjacent nodes
	getAdjacentNodes(adjacentNodes, rank);


	MPI_Request temp_req;
	MPI_Status temp_stat;

	int usflag = 0;

	MPI_Irecv(&userstop, 1, MPI_INT, baseStation, 3, MPI_COMM_WORLD, &temp_req);


	while (1){
		MPI_Request requests[2 * NUMBEROFADJACENT];
		MPI_Status statuses[2 * NUMBEROFADJACENT];

		int nreq = 0;
		
		sendTrigger(adjacentNodes, requests, &nreq);


		recieveTriggerFromAdjacent(adjacentNodes, recievePackBuffer, requests, &nreq);


		MPI_Waitall(nreq , requests, statuses);


		
		for (int index = 0; index < NUMBEROFADJACENT; index++){
			int position = 0;
			if (adjacentNodes[index] != -1){

			if (ENCRYPT_COMM == 1){
				AES_init_ctx_iv(&ctx, key, iv);
				AES_CTR_xcrypt_buffer(&ctx, recievePackBuffer + (packsize * index), packsize);
			}


				MPI_Unpack((recievePackBuffer + (packsize * index)), packsize, &position, &recievedNumCurrent[index], 1, MPI_INT, MPI_COMM_WORLD);
			}
			
			// printf("Recieving %i; Value : %i \n",rank,recievedNumCurrent[index] );
		}

		checkForTrigger(recievedNumPast, recievedNumCurrent);

		memcpy(recievedNumPast, recievedNumCurrent, sizeof(int) * 4);

		iterationCount += 1;

		MPI_Test(&temp_req, &usflag, &temp_stat);
		
		// MPI_Test(&temp_req, &usflag, &temp_stat);		

		if (iterations != -1){
			if (iterationCount > iterations){
				break;
			}
		}

		if (userstop == 1){
			break;
		}

   		sleep(refreshInteval);

	}

	int stop = MAX_RANDOM + 2;
	int position = 0;

	// if (ENCRYPT_DEMO == 0){
	// 	packsize = (sizeof(int) * 5 + sizeof(double) + (100 * sizeof(char)));
	// }
	
	uint8_t packbuf[packsize];
	memset(packbuf, 0, packsize);

	MPI_Pack(&stop, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
	MPI_Pack(&totalInterNodeMessageCount, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
	
	
	if (ENCRYPT_COMM == 1){
		// Initialize Encyption
		AES_init_ctx_iv(&ctx, key, iv);
		AES_CTR_xcrypt_buffer(&ctx, packbuf, packsize);
	}

	MPI_Send(packbuf, position, MPI_PACKED, baseStation, 1, MPI_COMM_WORLD);

}


void initializeNodes(){

	// if (ENCRYPT_DEMO == 0){
	// 	packsize = 32 * sizeof(unsigned char);
	// }

	uint8_t packbuf[packsize];
	memset(packbuf, 0, packsize);

	int position = 0;

	char* ip_address = getIPAddress();

	char mac_address[17] = "78:4f:43:5b:c2:c3";
	// char* mac_address = getMACAddress();
	

	MPI_Pack( &mac_address, 17, MPI_CHAR, packbuf, packsize, &position, MPI_COMM_WORLD );
	MPI_Pack( ip_address, 15, MPI_CHAR, packbuf, packsize, &position, MPI_COMM_WORLD );


	if (ENCRYPT_COMM == 1){
		AES_init_ctx_iv(&ctx, key, iv);
		AES_CTR_xcrypt_buffer(&ctx, packbuf, packsize);
	}


	MPI_Send(packbuf, position, MPI_PACKED, baseStation, 0, MPI_COMM_WORLD);

}

int checkForTrigger(int* recievedNumPast, int* recievedNumCurrent){


	int sendArrayLevel1[NUMBEROFADJACENT];
	int sendArrayLevel2[NUMBEROFADJACENT];

	int level1Count;
	int level2Count;

	int level1event = 0;
	int level2event = 0;

	int level1match = 0;
	int level2match = 0;

	for (int i=0; i < NUMBEROFADJACENT;i++){

		memset(sendArrayLevel2,-1,4*sizeof(int));

		int level2Count = 1;

		for (int j = i + 1; j < NUMBEROFADJACENT; j++){

			if (recievedNumCurrent[i] == recievedNumPast[j] && recievedNumCurrent[i] != MAX_RANDOM && i != j){
				sendArrayLevel2[i] = iterationCount - 1;
				sendArrayLevel2[j] = iterationCount - 1;
				level2match = recievedNumCurrent[i];
				level2Count+=1;
			}else if(recievedNumCurrent[i] == recievedNumCurrent[j] && recievedNumCurrent[i] != MAX_RANDOM && i != j){
				sendArrayLevel2[i] = iterationCount;
				sendArrayLevel2[j] = iterationCount;
				level2match = recievedNumCurrent[i];
				level2Count+=1;
			}

		}

		if (level2Count >= 3){
			level2event = 1;
			break;
		}
	
	}

	for (int i = 0; i < NUMBEROFADJACENT / 2;i++){
		
		memset(sendArrayLevel1,-1,4*sizeof(int));

		int level1Count = 1;

		int k;

		
		for (k = i + 1; k < NUMBEROFADJACENT; k++){


			if (recievedNumCurrent[i] == recievedNumCurrent[k] & recievedNumCurrent[i] != MAX_RANDOM){
				
				sendArrayLevel1[i] = iterationCount;
				sendArrayLevel1[k] = iterationCount;

				level1match = recievedNumCurrent[i];

				level1Count += 1;
			}
		}
		
		if (level1Count >= 3){
			
			level1event = 1;
			break;
		}

	}
	

	if (level1event == 1 || level2event == 1 ){

		FILE* fp;
		char path[20];
		sprintf(path, "./nodes/%d.txt", rank);
		// printf("%s\n", path);
		fp = fopen(path, "a+");
		fprintf (fp, "%s", "------------------------------------------------------\n");
		

		uint8_t packbuf[packsize];
		memset(packbuf, 0, packsize);


		int position = 0;

		int sendFlag = 0;

		if (level1event == 1){
			sendFlag = 1;
			MPI_Pack( &level1match, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
			MPI_Pack( &sendArrayLevel1, 4, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
			

		}else if (level2event == 1 && level1event == 0 ){
			sendFlag = 1;
			MPI_Pack( &level2match, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
			MPI_Pack( &sendArrayLevel2, 4, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
		}


		if (sendFlag) {
			double eventTime = MPI_Wtime();
			
			MPI_Pack( &eventTime, 1, MPI_DOUBLE, packbuf, packsize, &position, MPI_COMM_WORLD );

			char timestamp[100];
			convertToTimeStamp(timestamp, 100);
			MPI_Pack( timestamp, 100, MPI_CHAR, packbuf, packsize, &position, MPI_COMM_WORLD );
			
			// fprintf(fp, "Original Message : \n");
			// fwrite(&packbuf , packsize , sizeof(char) , fp );


			double encyptStartTime = MPI_Wtime();

			if (ENCRYPT_COMM == 1){
				AES_init_ctx_iv(&ctx, key, iv);
				AES_CTR_xcrypt_buffer(&ctx, packbuf, packsize);
			}

			double encyptionTime = MPI_Wtime() - encyptStartTime;

			MPI_Send(packbuf, position, MPI_PACKED, baseStation, 1, MPI_COMM_WORLD);
			

			fprintf(fp, "Encryption Time : %f\n", encyptionTime);

			
			// fprintf(fp, "Encrypted Message : \n");
			// fwrite(&packbuf , packsize , sizeof(char) , fp );

			fclose(fp);


		}

	}
	return 0;
}

int recieveTriggerFromAdjacent(int* adjacentNodes, uint8_t* recievePackBuffer, MPI_Request* req, int* nreq){

	for (int index = 0; index < NUMBEROFADJACENT; index++){
		if (adjacentNodes[index] != -1){

			
			int position = 0;
			
			// MPI_Recv(packbuf, packsize, MPI_PACKED, 0 , 0, MPI_COMM_WORLD, &stat);
			MPI_Irecv((recievePackBuffer + (packsize * index)), packsize, MPI_PACKED, adjacentNodes[index], 0, MPI_COMM_WORLD, &req[*nreq]);

			// printf("Recieving %i -> %i; Value : %i \n",adjacentNodes[index],rank,recievedNumCurrent[index] );
			*nreq = *nreq + 1;
		}
		
	}

	return 0;

}



int sendTrigger(int* adjacentNodes, MPI_Request* req, int* nreq){
	int randNum;
	randNum = getRandomNumber(rank);


	for (int index = 0; index < NUMBEROFADJACENT; index++){

		totalInterNodeMessageCount +=1;
		
		if (adjacentNodes[index] != -1){
			// printf("Sending %i -> %i : Value : %i; \n", rank,adjacentNodes[index], randNum);

			uint8_t packbuf[packsize];
			memset(packbuf, 0, packsize);

			int position = 0;
			MPI_Pack( &randNum, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );

			if (ENCRYPT_COMM == 1){
				AES_init_ctx_iv(&ctx, key, iv);
				AES_CTR_xcrypt_buffer(&ctx, packbuf, packsize);
			}

			
			MPI_Isend(packbuf, packsize, MPI_PACKED, adjacentNodes[index], 0, MPI_COMM_WORLD, &req[*nreq]);
			*nreq = *nreq + 1;
		}
		
	}

	return 0;
}