/* 
File: init.c
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
#include "omp.h"

#include "./init.h"
#include "./node.h"
#include "./base.h"

// #include "getMACAddress.c"

// initialize the global varaibles
int numtasks, rank;
int baseStation;

int WIDTH;
int HEIGHT;

double simStartTime;
int refreshInteval;
int iterationMax;

int userstop = 0;


// initialize the variables needed for encryption
struct AES_ctx ctx;
// uint8_t key[] = "6Ghen2kCseQms8t3";
uint8_t key[16] = { (uint8_t) 0x2b, (uint8_t) 0x7e, (uint8_t) 0x15, (uint8_t) 0x16, (uint8_t) 0x28, (uint8_t) 0xae, (uint8_t) 0xd2, (uint8_t) 0xa6, (uint8_t) 0xab, (uint8_t) 0xf7, (uint8_t) 0x15, (uint8_t) 0x88, (uint8_t) 0x09, (uint8_t) 0xcf, (uint8_t) 0x4f, (uint8_t) 0x3c };
uint8_t iv[]  = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

// --------------------------------------------------------------------------------------------------------------------------------

// Main program
int main(int argc, char *argv[])
 {

	// Initialize MPI
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	// Initialize the system
	initializeSystem();

	// Running the specific methods based on the rank
	if (rank == baseStation){
		base();
	}else{
		node();
	}
	
	// Finalize the MPI program
	MPI_Finalize();
	return 0;

 }
// --------------------------------------------------------------------------------------------------------------------

void initializeSystem(){
	/*
	 * This method is used to initialize the system, by getting the user inputs and broadcasting it to all the nodes
	 * Starts the timer for the simulation
	 * It also creates a POSIX thread if the user enters -1 as input
	*/

	// Initialize the local variables
	int position = 0;
	MPI_Status stat;

	// Initialize base station
	baseStation = 0;

	// Base station get the input from the user
	// if (rank == baseStation){

	// 	// Print the banner
	// 	printBanner();

	// 	// Get grid width and height
	// 	printf("What is the shape of the %i node grid ? (width height) : \n", numtasks - 1);
	// 	fflush(stdin);
	// 	scanf("%d%d", &WIDTH, &HEIGHT);

	// 	printf("Creating node grid of size (%i,%i) and base station using %i nodes\n\n", WIDTH, HEIGHT, numtasks);

	// 	// Get the iteration count
	// 	printf("How many iterations does the nodes search for (integer value, -1 for until \"stop\" is entered)? : \n");
	// 	fflush(stdin);
	// 	scanf("%i", &iterationMax);

	// 	// Get the interval
	// 	printf("How often does each iteration happen (seconds) ? : \n");
	// 	fflush(stdin);
	// 	scanf("%i", &refreshInteval);


	// }

	WIDTH = 3;
	HEIGHT = 3;
	iterationMax = 10;
	refreshInteval = 1;
	
	// Get the current time
	simStartTime = MPI_Wtime();

	// Base station sends the data to the nodes
	if (rank == baseStation){

		// Initialize the packbuffer to zeros
		uint8_t packbuf[packsize];
		memset(packbuf, 0, packsize);

		// Pack the data to be encypted and sent
		int position = 0;
		MPI_Pack( &WIDTH, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
		MPI_Pack( &HEIGHT, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
		MPI_Pack( &iterationMax, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
		MPI_Pack( &refreshInteval, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );


		// Encrypt the data
		if (ENCRYPT_COMM == 1){
			AES_init_ctx_iv(&ctx, key, iv);
			AES_CTR_xcrypt_buffer(&ctx, packbuf, packsize);
		}

		// Send the data to all the nodes
		for (int i = 1; i < numtasks; i++){
			MPI_Send(packbuf, position, MPI_PACKED, i, 0, MPI_COMM_WORLD);
		}
		
	}

	// If it is a sensor node
	if (rank != baseStation){

		// Initialize the packbuffer
		uint8_t packbuf[packsize];
		memset(packbuf, 0, packsize);

		// Initialize the position
		int position = 0;

		// Recieve the user inputs from the base station
		MPI_Recv(packbuf, packsize, MPI_PACKED, 0 , 0, MPI_COMM_WORLD, &stat);

		// Decrypt the pack buffer
		if (ENCRYPT_COMM == 1){
			AES_init_ctx_iv(&ctx, key, iv);
			AES_CTR_xcrypt_buffer(&ctx, packbuf, packsize);
		}

		// Unpack the user inputs and assign them
		MPI_Unpack(packbuf, packsize, &position, &WIDTH, 1, MPI_INT, MPI_COMM_WORLD);
		MPI_Unpack(packbuf, packsize, &position, &HEIGHT, 1, MPI_INT, MPI_COMM_WORLD);
		MPI_Unpack(packbuf, packsize, &position, &iterationMax, 1, MPI_INT, MPI_COMM_WORLD);
		MPI_Unpack(packbuf, packsize, &position, &refreshInteval, 1, MPI_INT, MPI_COMM_WORLD);
	}

	// Base Station creates a POSIX thread if the user enters -1 for the iteration
	if (iterationMax == -1 && rank == baseStation){
		pthread_t stopThread;
		pthread_create(&stopThread, NULL, checkStop, NULL);

	}else if (rank == baseStation){
		// If the user doesnt enter -1, display on the console
		printf("Running %i iterations at %i second intervals\n", iterationMax, refreshInteval);
	}
	
}

void printBanner(){
	/*
	 * Method used to print the banner on the screen
	*/
	printf(" __          _______ _   _   ________      ________ _   _ _______   _____  ______ _______ ______ _____ _______ _____ ____  _   _ \n");
	printf(" \\ \\        / / ____| \\ | | |  ____\\ \\    / /  ____| \\ | |__   __| |  __ \\|  ____|__   __|  ____/ ____|__   __|_   _/ __ \\| \\ | |\n");
	printf("  \\ \\  /\\  / / (___ |  \\| | | |__   \\ \\  / /| |__  |  \\| |  | |    | |  | | |__     | |  | |__ | |       | |    | || |  | |  \\| |\n");
	printf("   \\ \\/  \\/ / \\___ \\| . ` | |  __|   \\ \\/ / |  __| | . ` |  | |    | |  | |  __|    | |  |  __|| |       | |    | || |  | | . ` |\n");
	printf("    \\  /\\  /  ____) | |\\  | | |____   \\  /  | |____| |\\  |  | |    | |__| | |____   | |  | |___| |____   | |   _| || |__| | |\\  |\n");
	printf("     \\/  \\/  |_____/|_| \\_| |______|   \\/   |______|_| \\_|  |_|    |_____/|______|  |_|  |______\\_____|  |_|  |_____\\____/|_| \\_|\n\n");
}

void convertToTimeStamp(char* buf, int size){
	/*
	 * Method used to get the current time as a date time string
	*/
	struct tm  ts;
	time_t     now;

	// Get the time
	time(&now);

	// Create a tm object
    ts = *localtime(&now);

	// Convert the time to date time string
    strftime(buf, size, "%a %Y-%m-%d %H:%M:%S", &ts);
}


int getAdjacentNodes(int *ajacentNodesArr, int currentRank){
	/*
	 * Method used to get the adjacent node
	*/

	// Reduce the rank by one to account for base station being zero
	currentRank -= 1;

	// Get the row and column of the given rank
	int rowIndex = currentRank / WIDTH;
	int columnIndex = currentRank % WIDTH;
	
	// Calculate left sibling
	int leftSiblingcol = columnIndex - 1;

	if (leftSiblingcol >= 0){
		ajacentNodesArr[0] = rowIndex*WIDTH + leftSiblingcol + 1;
	}

	// Calcualte right sibling
	int rightSiblingcol = columnIndex + 1;

	if (rightSiblingcol < WIDTH) {
		ajacentNodesArr[1] = rowIndex*WIDTH + rightSiblingcol + 1;
	}

	// Calculate top Sibling
	int topSiblingrow = rowIndex - 1;

	if (topSiblingrow >= 0) {
		ajacentNodesArr[2] = topSiblingrow*WIDTH + columnIndex + 1;
	}

	// Calcualte bottom sibling
	int bottomSiblingrow = rowIndex + 1;

	if (bottomSiblingrow < HEIGHT) {
		ajacentNodesArr[3] = bottomSiblingrow*WIDTH + columnIndex + 1;
	}

	return 0;

}


void* checkStop(void * arg){
	/*
	 * Method which checks whether the user enters stop
	*/

	// Creates an array to store the user input
	char temp[4];

	// Display the message
	printf("\n\nPlease enter \"stop\" to end the simulation.....\n");
	fflush(stdin);
	scanf("%s", temp);

	// Check of the entered keyword is stop
	if (strncmp(temp, "stop",4) == 0){
		// if yes stop the simulation
		printf("\nStopping the simulation.\n");
	}else{
		// if not display the message
		printf("\nCorrect keyword not entered, but still stopping the simulation.\n");
	}
	
	// Create the local varaibles
	userstop = 1;
	MPI_Request temp_r[numtasks - 1];
	MPI_Status temp_s[numtasks - 1];
	int numberOfReq = 0;

	// Send a message to each node to end the iteration
	for (int j = 1; j < numtasks; j++  ){
		MPI_Isend(&userstop, 1, MPI_INT, j, 3, MPI_COMM_WORLD, &temp_r[numberOfReq]);
		numberOfReq+=1;

	}

	// Wait until all messeges are sents
	MPI_Waitall(numberOfReq , temp_r, temp_s);
	
	return NULL;
}

void encrypt_decrypt(uint8_t* buffer, uint32_t size){

	uint32_t block_size = size/16;

	int i;

	#pragma omp for schedule(static) private(i)
	for (i = 0; i < block_size; i++){
		AES_init_ctx_iv(&ctx, key, iv);
		for (int j = 0; j < 1000; j++){
			AES_CTR_xcrypt_buffer(&ctx, buffer + (block_size * i), block_size);
		}
	}

}


