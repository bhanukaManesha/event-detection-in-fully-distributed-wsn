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

// #include "getMACAddress.c"
#include "./node.h"
#include "./base.h"

// #include "AES/aes.c"

// global varaible definition
int numtasks, rank;
int baseStation;

int WIDTH;
int HEIGHT;

double simStartTime;

struct AES_ctx ctx;
uint8_t key[] = "hanhrithuqwedjkl";
uint8_t iv[]  = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

int refreshInteval;
int iterations;

int userstop = 0;





// --------------------------------------------------------------------------------------------------------------------------------
// Function Declarations

// void node();
// int recieveTriggerFromAdjacent(int* adjacentNodes, uint8_t* recievePackBuffer, MPI_Request* req, int* nreq);
// int sendTrigger(int* adjacentNodes, MPI_Request* req, int* nreq);
// int getRandomNumber();
// int getAdjacentNodes(int *ajacentNodesArr, int currentRank);
// int checkForTrigger(int* recievedNumPast, int* recievedNumCurrent);

void printBanner();
void initializeSystem();

// void initializeBaseStation();
// void initializeNodes();
// void base();
// void listenToEvents();

// void * checkStop(void * arg);

// void logData(unsigned long startTime, int incomingNode, int triggerValue, int* activeNodes);
// void convertToTimeStamp();

// --------------------------------------------------------------------------------------------------------------------------------

// Main program
int main(int argc, char *argv[])
 {

	// Initialize MPI
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	initializeSystem();

	if (rank == baseStation){
		base();
	}else{
		node();
	}
	
	// MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
	return 0;

 }
// --------------------------------------------------------------------------------------------------------------------

void initializeSystem(){

	int position = 0;
	// int packsize = (sizeof(int) * 2);
	// unsigned char packbuf[packsize];
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


	if (rank == baseStation){

		uint8_t packbuf[packsize];
		memset(packbuf, 0, packsize);

		int position = 0;
		MPI_Pack( &WIDTH, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
		MPI_Pack( &HEIGHT, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
		MPI_Pack( &iterations, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
		MPI_Pack( &refreshInteval, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );


		if (ENCRYPT_COMM == 1){
			AES_init_ctx_iv(&ctx, key, iv);
			AES_CTR_xcrypt_buffer(&ctx, packbuf, packsize);
		}

		for (int i = 1; i < numtasks; i++){
			MPI_Send(packbuf, position, MPI_PACKED, i, 0, MPI_COMM_WORLD);
		}
		
	}


	if (rank != baseStation){
		uint8_t packbuf[packsize];
		memset(packbuf, 0, packsize);
		int position = 0;
		MPI_Recv(packbuf, packsize, MPI_PACKED, 0 , 0, MPI_COMM_WORLD, &stat);

		if (ENCRYPT_COMM == 1){
			AES_init_ctx_iv(&ctx, key, iv);
			AES_CTR_xcrypt_buffer(&ctx, packbuf, packsize);
		}

		MPI_Unpack(packbuf, packsize, &position, &WIDTH, 1, MPI_INT, MPI_COMM_WORLD);
		MPI_Unpack(packbuf, packsize, &position, &HEIGHT, 1, MPI_INT, MPI_COMM_WORLD);
		MPI_Unpack(packbuf, packsize, &position, &iterations, 1, MPI_INT, MPI_COMM_WORLD);
		MPI_Unpack(packbuf, packsize, &position, &refreshInteval, 1, MPI_INT, MPI_COMM_WORLD);
	}

	// Create Pthread
	if (iterations == -1 && rank == baseStation){
		pthread_t stopThread;
		pthread_create(&stopThread, NULL, checkStop, NULL);
	}else if (rank == baseStation){
		printf("Running %i iterations at %i second intervals\n", iterations, refreshInteval);
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

void convertToTimeStamp(char* buf, int size){
	struct tm  ts;
	time_t     now;
	time(&now);
    ts = *localtime(&now);
    strftime(buf, size, "%a %Y-%m-%d %H:%M:%S", &ts);

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


void* checkStop(void * arg){
	char temp[4];

	printf("\n\nPlease enter \"stop\" to end the simulation.....\n");
	fflush(stdin);
	scanf("%s", temp);

	userstop = 1;

	MPI_Request temp_r[numtasks - 1];
	MPI_Status temp_s[numtasks - 1];
	int numberOfReq = 0;

	for (int j = 1; j < numtasks; j++  ){
		MPI_Isend(&userstop, 1, MPI_INT, j, 3, MPI_COMM_WORLD, &temp_r[numberOfReq]);
		numberOfReq+=1;

	}

	MPI_Waitall(numberOfReq , temp_r, temp_s);
		
	return NULL;
}


