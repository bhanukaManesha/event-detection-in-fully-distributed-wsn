#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <mpi.h>
#include <string.h>
#include <time.h> 
#include <unistd.h>
#include <pthread.h>

// #include "getMACAddress.c"
#include "getIPAddress.c"

#define MAX_RANDOM 3
#define NUMBEROFADJACENT 4

#define ENCRYPT_COMM 1

#define CBC 1
#define CTR 1
#define ECB 1
#include "AES/aes.h"
#include "AES/aes.c"


// global varaible definition
int numtasks, rank;
int baseStation;

int WIDTH;
int HEIGHT;

double simStartTime;

struct AES_ctx ctx;
uint8_t key[] = "sajdaksdnaskbskdjabnsdjokansdljasndlasndalksdnaslkdnalksdnaslkdnaslkdnaslkdnmasslkdandlasndlkasndlkasndlksad\
				jdnakjcasajdaksdnaskjasdlkansdlkamnsdlkasmndlkamasbvduavsbdhjabsdjhabsdjahsbdasdbajkhdbajhdbajsdvbajhdvabsjdh\
				dnakjcasajdaksdnaskjdnakjcasajdaksdnaskjdnakjcasajdaksdnaskjdnakjcasajdaksdnaskjdnakjcasajdakssadnaskjdnakjca";
				
uint8_t iv[16] = { 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff };


int refreshInteval;
int iterations;
int iterationCount = 1;

int totalInterNodeMessageCount;

char* macAddressStorage;
char* ipAddressStorage;

MPI_Comm NODE_COMM;

int userstop = 0;

// --------------------------------------------------------------------------------------------------------------------------------
// Function Declarations

void node();
int recieveTriggerFromAdjacent(int* adjacentNodes, int* recievedNumCurrent, MPI_Request* req, int* nreq);
int sendTrigger(int* adjacentNodes, MPI_Request* req, int* nreq);
int getRandomNumber();
int getAdjacentNodes(int *ajacentNodesArr, int currentRank);
int checkForTrigger(int* recievedNumPast, int* recievedNumCurrent,int* adjacentNodes);

void printBanner();
void initializeSystem();

void initializeBaseStation();
void initializeNodes();
void base();
void listenToEvents();

void * checkStop(void * arg);

void logData(unsigned long startTime, int incomingNode, int triggerValue, int* activeNodes);
void convertToTimeStamp();
// Main program
int main(int argc, char *argv[])
 {
	// Initialize MPI
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);


	initializeSystem();




	// Initalize the node grid
	// unsigned char* nodeGrid = (unsigned char*) malloc(TOTALNODE - 1);

	// Create the new communicator
	// MPI_Comm_split(MPI_COMM_WORLD, rank == 0, 0, &NODE_COMM);


	if (rank == baseStation){
		base();
	}else{
		node();
	}
	MPI_Finalize();
	return 0;

 }
// --------------------------------------------------------------------------------------------------------------------

void initializeSystem(){

	int position = 0;
	int packsize = (sizeof(int) * 2);
	unsigned char packbuf[packsize];
	MPI_Status stat;

	int w;
	int h;

	// Initialize base station
	baseStation = 0;

	if (rank == baseStation){
		printBanner();

		printf("What is the shape of the %i node grid ? (width height) : \n", numtasks - 1);
		fflush(stdin);
		scanf("%d%d", &WIDTH, &HEIGHT);

		printf("Creating node grid of size (%i,%i) and base station using %i nodes\n\n", WIDTH, HEIGHT, numtasks);

		printf("How many iterations does the nodes search for (interger value, -1 for until \"stop\" is entered)? : \n");
		fflush(stdin);
		scanf("%i", &iterations);

		printf("How often does each iteration happen (seconds) ? : \n");
		fflush(stdin);
		scanf("%i", &refreshInteval);


	}

	simStartTime = MPI_Wtime();

	MPI_Bcast( &WIDTH, 1, MPI_INT, 0, MPI_COMM_WORLD );
	MPI_Bcast( &HEIGHT, 1, MPI_INT, 0, MPI_COMM_WORLD );
	MPI_Bcast( &iterations, 1, MPI_INT, 0, MPI_COMM_WORLD );
	MPI_Bcast( &refreshInteval, 1, MPI_INT, 0, MPI_COMM_WORLD );

	// if (rank == 0){
	// 	MPI_Send(&simStartTime, 1, MPI_DOUBLE, baseStation, 0, MPI_COMM_WORLD);
	// }else if (rank == baseStation){
	// 	MPI_Recv(&simStartTime, 1, MPI_DOUBLE, 0 , 0, MPI_COMM_WORLD, &stat);
	// }


	// Create Pthread
	if (iterations == -1 && rank == baseStation){
		pthread_t stopThread;
		pthread_create(&stopThread, NULL, checkStop, NULL);
	}

	
}

void printBanner(){
	printf(" __          _______ _   _   ________      ________ _   _ _______   _____  ______ _______ ______ _____ _______ _____ ____  _   _ \n");
	printf(" \\ \\        / / ____| \\ | | |  ____\\ \\    / /  ____| \\ | |__   __| |  __ \\|  ____|__   __|  ____/ ____|__   __|_   _/ __ \\| \\ | |\n");
	printf("  \\ \\  /\\  / / (___ |  \\| | | |__   \\ \\  / /| |__  |  \\| |  | |    | |  | | |__     | |  | |__ | |       | |    | || |  | |  \\| |\n");
	printf("   \\ \\/  \\/ / \\___ \\| . ` | |  __|   \\ \\/ / |  __| | . ` |  | |    | |  | |  __|    | |  |  __|| |       | |    | || |  | | . ` |\n");
	printf("    \\  /\\  /  ____) | |\\  | | |____   \\  /  | |____| |\\  |  | |    | |__| | |____   | |  | |___| |____   | |   _| || |__| | |\\  |\n");
	printf("     \\/  \\/  |_____/|_| \\_| |______|   \\/   |______|_| \\_|  |_|    |_____/|______|  |_|  |______\\_____|  |_|  |_____\\____/|_| \\_|\n\n");
}


// --------------------------------------------------------------------------------------------------------------------

 void node(){

	initializeNodes();


	// Generate the adjacent nodes
	int adjacentNodes[NUMBEROFADJACENT] = {-1, -1, -1, -1};
	int recievedNumPast[NUMBEROFADJACENT] = {-1, -1, -1, -1};
	int recievedNumCurrent[NUMBEROFADJACENT] = {-1, -1, -1, -1};


	// printf("LENGTH : %lu\n", sizeof(adjacentNodes)/sizeof(int));

	// Get the adjacent nodes
	getAdjacentNodes(adjacentNodes, rank);

	// int j;
	// printf("-----\n");
	// printf("%p", adjacentNodes);
	// printf("Rank : %i\n", rank);
	// for (j=0; j<4; j++)
	// 	printf("array[%d] = %d\n", j, adjacentNodes[j]);



	MPI_Request temp_req;
	MPI_Status temp_stat;

	int usflag = 0;

	MPI_Irecv(&userstop, 1, MPI_INT, baseStation, 3, MPI_COMM_WORLD, &temp_req);


	while (1){
		MPI_Request requests[2 * NUMBEROFADJACENT];
		MPI_Status statuses[2 * NUMBEROFADJACENT];
		int nreq = 0;
		
		sendTrigger(adjacentNodes, requests, &nreq);


		recieveTriggerFromAdjacent(adjacentNodes, recievedNumCurrent, requests, &nreq);

		// printf("rank : %i, nREQ : %i\n", rank, nreq);

		MPI_Waitall(nreq , requests, statuses);

		// for (int j=0; j<4; j++)
		// 	printf("Rank %i : array[%d] = %d\n",rank, j, recievedNumCurrent[j]);

		checkForTrigger(recievedNumPast, recievedNumCurrent, adjacentNodes);

		memcpy(recievedNumPast, recievedNumCurrent, sizeof(int) * 4);

		iterationCount += 1;

		MPI_Test(&temp_req, &usflag, &temp_stat);
		
		// while (usflag == 0){
		// 	sleep(2);
		MPI_Test(&temp_req, &usflag, &temp_stat);

		// }
		

		if (iterations != -1){
			if (iterationCount > iterations){
				break;
			}
		}

		if (userstop == 1){
			break;
		}

		// break;
   		sleep(refreshInteval);


		
		   

	}

	int stop = -1;
	int position = 0;
	int packsize = (sizeof(int) * 5 + sizeof(double) + (80 * sizeof(char)));
	uint8_t packbuf[packsize];

	MPI_Pack(&stop, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
	MPI_Pack(&totalInterNodeMessageCount, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );

	if (ENCRYPT_COMM == 1){
		// Initialize Encyption
		AES_init_ctx_iv(&ctx, key, iv);
		AES_CTR_xcrypt_buffer(&ctx, packbuf, strlen((char*)packbuf));
		// printf("\nENC: %s\n",(char*) packbuf);
		// printf("%i ended.\n", rank);
	}

	MPI_Send(packbuf, position, MPI_PACKED, baseStation, 1, MPI_COMM_WORLD);

}

// void * checkForNodeStop(void * arg){

// 	printf("Rank %i is checking for stop\n", rank);

// 	return NULL;

// }

void initializeNodes(){

	int totalChar = 33;
	int packsize = totalChar * sizeof(unsigned char);
	uint8_t packbuf[packsize];
	int position = 0;
	// unsigned char mac_address[17] = "00:0a:95:9d:68:16";
	// unsigned char ip_address[16] = "192.168.255.255";

	char mac_address[17] = "78:4f:43:5b:c2:c3";
	// unsigned char ip_address[15] = "192.168.255.120";

	char* ip_address = getIPAddress();
	// printf("Host IP: %s\n", ip_address); 

	
	// char* mac_address = getMACAddress();
	// printf("test\n\n");
	

	
	// Pack up the data
	
	// printf("%s\n", mac_address);

	MPI_Pack( &mac_address, 17, MPI_CHAR, packbuf, packsize, &position, MPI_COMM_WORLD );
	MPI_Pack( ip_address, 15, MPI_CHAR, packbuf, packsize, &position, MPI_COMM_WORLD );


	if (ENCRYPT_COMM == 1){
		// printf("Length: %lu",strlen((char*)packbuf));
		AES_init_ctx_iv(&ctx, key, iv);
		AES_CTR_xcrypt_buffer(&ctx, packbuf, strlen((char*)packbuf));
		// printf("\nENC: %s\n",(char*) packbuf); // don't use this string as an input
	}


	MPI_Send(packbuf, position, MPI_PACKED, baseStation, 0, MPI_COMM_WORLD);




	// printf("data sent to %i\n", baseStation);
}

int checkForTrigger(int* recievedNumPast, int* recievedNumCurrent,int* adjacentNodes){


	int sendArrayLevel1[NUMBEROFADJACENT];
	int sendArrayLevel2[NUMBEROFADJACENT];

	int level1Count;
	int level2Count;

	int level1event = 0;
	int level2event = 0;


	int level1match = 0;
	int level2match = 0;

	// printf("----------------\n");
	// for (int j=0; j<4; j++)
		// printf("Rank %i : array[%d] = %d\n",rank, j, recievedNumCurrent[j]);
	// printf("PAST : %i : [%i, %i, %i, %i] -> base\n", rank, recievedNumPast[0], recievedNumPast[1], recievedNumPast[2], recievedNumPast[3]);
	// printf("CURRENT : %i : [%i, %i, %i, %i] -> base\n", rank, recievedNumCurrent[0], recievedNumCurrent[1], recievedNumCurrent[2], recievedNumCurrent[3]);

	for (int i=0; i < NUMBEROFADJACENT / 2;i++){

		memset(sendArrayLevel2,-1,4*sizeof(int));

		int level2Count = 0;

		for (int j = i + 1; j < NUMBEROFADJACENT; j++){

			if (recievedNumCurrent[i] == recievedNumPast[j] && recievedNumCurrent[i] != -1){
				sendArrayLevel2[i] = iterationCount - 1;
				sendArrayLevel2[j] = iterationCount - 1;
				level2match = recievedNumCurrent[i];
				level2Count+=1;
			}else if(recievedNumCurrent[i] == recievedNumCurrent[j] && recievedNumCurrent[i] != -1){
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


			if (recievedNumCurrent[i] == recievedNumCurrent[k] & recievedNumCurrent[i] != -1){
				
				sendArrayLevel1[i] = iterationCount;
				sendArrayLevel1[k] = iterationCount;

				level1match = recievedNumCurrent[i];

				level1Count += 1;
				// printf("L1Count : %i\n", level1Count);
			}
		}
		
		if (level1Count >= 3){
			
			level1event = 1;
			break;
		}

	}
	
	// // printf("Level1 : %i\n",level1Count );

	if (level1event == 1 || level2event == 1){
		
		int packsize = 1000000 * sizeof(double);
		uint8_t packbuf[packsize];
		int position = 0;

		if (level1event == 1){
			// printf("Rank : %i triggered level 1 event\n", rank);
			MPI_Pack( &level1match, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
			MPI_Pack( &sendArrayLevel1, 4, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
			

		}else if (level2event == 1 && level1event == 0 ){
			// printf("Rank : %i triggered level 2 event\n", rank);
			MPI_Pack( &level2match, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
			MPI_Pack( &sendArrayLevel2, 4, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
			

		}

		
		double eventTime = MPI_Wtime();
		char timestamp[80];
		convertToTimeStamp(timestamp, 80);

		MPI_Pack( timestamp, 80, MPI_CHAR, packbuf, packsize, &position, MPI_COMM_WORLD );
		MPI_Pack( &eventTime, 1, MPI_DOUBLE, packbuf, packsize, &position, MPI_COMM_WORLD );
		

		double encyptStartTime = MPI_Wtime();
		if (ENCRYPT_COMM == 1){
			AES_init_ctx_iv(&ctx, key, iv);
			AES_CBC_encrypt_buffer(&ctx, packbuf, packsize);
		}

		double encyptionTime = MPI_Wtime() - encyptStartTime;

		// FILE *fp;
		// char* path;
		// // fprintf(path, "nodes/%i.txt", rank)
		// sprintf(path,"./nodes/%i.txt", rank);
		
		FILE* fp;
		char path[20];
		sprintf(path, "./nodes/%d.txt", rank);
		// fp = fopen(output_path, "a+");

		printf("%s\n", path);
		fp = fopen(path, "a+");
		fprintf (fp, "%s", "------------------------------------------------------\n");
		fprintf(fp, "Encryption Time : %f\n", encyptionTime);
		fclose(fp);
		
		MPI_Send(packbuf, position, MPI_PACKED, baseStation, 1, MPI_COMM_WORLD);


	}
	return 0;
}

int recieveTriggerFromAdjacent(int* adjacentNodes, int* recievedNumCurrent, MPI_Request* req, int* nreq){

	for (int index = 0; index < NUMBEROFADJACENT; index++){
		if (adjacentNodes[index] != -1){
			MPI_Irecv(&recievedNumCurrent[index], 1, MPI_INT, adjacentNodes[index], 0, MPI_COMM_WORLD, &req[*nreq]);

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

			// if (ENCRYPT_COMM == 1){
			// 	// printf("Length: %lu",strlen((char*)packbuf));
			// 	AES_CTR_xcrypt_buffer(&ctx, packbuf, strlen((char*)packbuf));
			// 	// printf("\nENC: %s\n",(char*) packbuf); // don't use this string as an input
			// }

			MPI_Isend(&randNum, 1, MPI_INT, adjacentNodes[index], 0, MPI_COMM_WORLD, &req[*nreq]);
			*nreq = *nreq + 1;
		}
		
	}

	return 0;
}

int getRandomNumber(){
	// Setting the seed
	srand((int) time(NULL) ^ rank);
	srand(rand());
	return rand() % MAX_RANDOM;
	
}


int getAdjacentNodes(int *ajacentNodesArr, int currentRank){

	// for (int j=0; j<4; j++)
	// 	printf("Rank %i : array[%d] = %d\n",rank, j, ajacentNodesArr[j]);

	currentRank -= 1;

	int rowIndex = currentRank / WIDTH;
	int columnIndex = currentRank % WIDTH;
	
	// Left sibling
	int leftSiblingcol = columnIndex - 1;

	if (leftSiblingcol >= 0){
		ajacentNodesArr[0] = rowIndex*WIDTH + leftSiblingcol + 1;
	}

	// Right Sibling
	int rightSiblingcol = columnIndex + 1;

	if (rightSiblingcol < WIDTH) {
		ajacentNodesArr[1] = rowIndex*WIDTH + rightSiblingcol + 1;
	}

	// Top Sibling
	int topSiblingrow = rowIndex - 1;

	if (topSiblingrow >= 0) {
		ajacentNodesArr[2] = topSiblingrow*WIDTH + columnIndex + 1;
	}

	// Bottom Sibling
	int bottomSiblingrow = rowIndex + 1;

	if (bottomSiblingrow < HEIGHT) {
		ajacentNodesArr[3] = bottomSiblingrow*WIDTH + columnIndex + 1;
	}

	return 0;

}


// --------------------------------------------------------------------------------------------------------------------


void base(){

	macAddressStorage = (char*)  malloc(WIDTH * HEIGHT * 17 * sizeof(char));
	ipAddressStorage = (char*) malloc(WIDTH * HEIGHT * 15 * sizeof(char));

	initializeBaseStation();


	listenToEvents();
}

void listenToEvents(){
	MPI_Status stat;
	int packsize = (sizeof(int) * 5 + sizeof(double) + (80 * sizeof(char)));
	uint8_t packbuf[packsize];


	int activatedNodes[4];
	int incomingNode;
	int matchedValue;
	double eventTimeStamp;
	char eventDateTime[80];
	int position;

	int stopCount = 0;

	int adjacentNodes[NUMBEROFADJACENT];


	int totalMessages = 0;
	int totalActivations = 0;


	while (1){

		position = 0;

		MPI_Recv(packbuf, packsize, MPI_PACKED, MPI_ANY_SOURCE , 1, MPI_COMM_WORLD, &stat);

		if (ENCRYPT_COMM == 1){
			// printf("\nDECDEC: %s\n",(char*) packbuf);
			AES_init_ctx_iv(&ctx, key, iv);
			AES_CTR_xcrypt_buffer(&ctx, packbuf, strlen((char*)packbuf));
			// printf("\nDEC: %s\n",(char*) packbuf);
		}

		
		totalMessages += 1;

		incomingNode = stat.MPI_SOURCE;
		MPI_Unpack(packbuf, packsize, &position, &matchedValue, 1, MPI_INT, MPI_COMM_WORLD);


		// Stopping message
		if (matchedValue == -1){
			int temp;
			MPI_Unpack(packbuf, packsize, &position, &temp, 1, MPI_INT, MPI_COMM_WORLD);
			totalMessages += temp;


			// printf("Incoming %i\n", incomingNode);
			if (stopCount == (WIDTH * HEIGHT) - 1){
				break;
			}
			stopCount+= 1;
			continue;
		}


		MPI_Unpack(packbuf, packsize, &position, &activatedNodes, 4, MPI_INT, MPI_COMM_WORLD);
		MPI_Unpack(packbuf, packsize, &position, &eventDateTime, 80, MPI_CHAR, MPI_COMM_WORLD);
		MPI_Unpack(packbuf, packsize, &position, &eventTimeStamp, 1, MPI_DOUBLE, MPI_COMM_WORLD);

		char timeInDateTime[80];
		convertToTimeStamp(timeInDateTime, 80);

		// Get the adjacent nodes
		getAdjacentNodes(adjacentNodes, incomingNode);

		double commTime = MPI_Wtime() - eventTimeStamp;

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
			if (activatedNodes[i] != -1){
				totalActivations+=1;
				
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
	fprintf (fp, "Total Messages though the network (including termination signal): %i\n", totalMessages);
	fprintf (fp, "Total Activations : %d\n", totalActivations);

	fclose(fp);

}

// void logData(double startTime, int incomingNode, int triggerValue, int* activeNodes){

// 	// event time


// 	// 
// }

 void initializeBaseStation(){

	MPI_Status stat;
	int totalChar = 33;
	int packsize = totalChar;
	uint8_t packbuf[packsize];
	int incoming_rank;

	int position = 0;
	int count = 0;

	while (count < (WIDTH * HEIGHT)){

		position = 0;
		MPI_Recv(packbuf, packsize, MPI_PACKED, MPI_ANY_SOURCE , 0, MPI_COMM_WORLD, &stat);

		if (ENCRYPT_COMM == 1){
			// printf("\nDECDEC: %s\n",(char*) packbuf);
			AES_init_ctx_iv(&ctx, key, iv);
			AES_CTR_xcrypt_buffer(&ctx, packbuf, strlen((char*)packbuf));
			// printf("\nDEC: %s\n",(char*) packbuf);
		}


		// printf("PACK : %s\n", &packbuf[0]);

		incoming_rank = stat.MPI_SOURCE;
		// printf("Rank : %i\n", incoming_rank);
		MPI_Unpack(packbuf, packsize, &position, macAddressStorage + incoming_rank*sizeof(unsigned char)*17, 17, MPI_CHAR, MPI_COMM_WORLD);
		MPI_Unpack(packbuf, packsize, &position, ipAddressStorage + incoming_rank*sizeof(unsigned char)*15, 15, MPI_CHAR, MPI_COMM_WORLD);

		count += 1;

	}

 }

void convertToTimeStamp(char* buf, int size){
	struct tm  ts;
	time_t     now;
	time(&now);
    ts = *localtime(&now);
    strftime(buf, size, "%a %Y-%m-%d %H:%M:%S", &ts);
    // year_pr = ts.tm_year;
    // printf("Local Time %s\n", buf);

}



void* checkStop(void * arg){
	char temp[4];

		// printf("Base is Checking\n");
	printf("\n\nPlease enter \"stop\" to end the simulation.....\n");

		// printf("What is the shape of the %i node grid ? (width height) : \n", numtasks - 1);
	fflush(stdin);
	scanf("%s", temp);

		// printf("entered\n");

		// if (strncmp(temp, "stop", 4)){
		// 	printf("nigga\n");
		// 	userstop = 1;
		// }

	userstop = 1;

	MPI_Request temp_r[numtasks - 1];
	MPI_Status temp_s[numtasks - 1];
	int numberOfReq = 0;

	for (int j = 1; j < numtasks; j++  ){
		MPI_Isend(&userstop, 1, MPI_INT, j, 3, MPI_COMM_WORLD, &temp_r[numberOfReq]);
		numberOfReq+=1;

	}
	MPI_Waitall(numberOfReq , temp_r, temp_s);
		// printf("sent\n");
		
	return NULL;
}
