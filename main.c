#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <mpi.h>
#include <string.h>
#include <time.h> 
#include <unistd.h>


#include <sys/ioctl.h>
#include <net/if.h> 
#include <unistd.h>
#include <netinet/in.h>

#define MAX_RANDOM 2
#define WIDTH 4
#define HEIGHT 5
#define TOTALNODE WIDTH*HEIGHT
#define NUMBEROFADJACENT 4

// --------------------------------------------------------------------------------------------------------------------------------
// Function Declarations

void node(int rank, int baseStation);
int recieveTriggerFromAdjacent(int* adjacentNodes,int rank, int* recievedNumCurrent, MPI_Request* req, int* nreq);
int sendTrigger(int rank, int* adjacentNodes, MPI_Request* req, int* nreq);
int getRandomNumber(int rank);
int getAdjacentNodes(int rank, int *ajacentNodesArr);
int checkForTrigger(int* recievedNumPast, int* recievedNumCurrent,int* adjacentNodes,int rank);
void getIPAddress();


void initializeBaseStation(unsigned char* macaddressstorage, unsigned char* ipaddressstorage);
void initializeNodes(int rank, int baseStation);
void base(int rank);


// Main program
int main(int argc, char *argv[])
 {
	
	// Defining the variables to store the number of tasks and rank
	int numtasks, rank;


	int baseStation;

	// Initialize MPI
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Status stat;
	MPI_Comm NODE_COMM;

	// Initalize the node grid
	// unsigned char* nodeGrid = (unsigned char*) malloc(TOTALNODE - 1);

	// Initialize base station
	baseStation = numtasks - 1;

	// Create the new communicator
	// MPI_Comm_split(MPI_COMM_WORLD, rank == numtasks - 1, 0, &NODE_COMM);

	if (rank == baseStation){
		base(rank);
	}else{
		node(rank, baseStation);
	}
	MPI_Finalize();
	return 0;

 }


// --------------------------------------------------------------------------------------------------------------------

 void node(int rank, int baseStation){

	initializeNodes(rank, baseStation);


	// Generate the adjacent nodes
	int adjacentNodes[NUMBEROFADJACENT] = {-1, -1, -1, -1};
	int recievedNumPast[NUMBEROFADJACENT] = {-1, -1, -1, -1};
	int recievedNumCurrent[NUMBEROFADJACENT] = {-1, -1, -1, -1};


	// printf("LENGTH : %lu\n", sizeof(adjacentNodes)/sizeof(int));

	// Get the adjacent nodes
	getAdjacentNodes(rank, adjacentNodes);

	// int j;
	// printf("-----\n");
	// printf("%p", adjacentNodes);
	// printf("Rank : %i\n", rank);
	// for (j=0; j<4; j++)
	// 	printf("array[%d] = %d\n", j, adjacentNodes[j]);


	while (1){
		MPI_Request requests[2 * NUMBEROFADJACENT];
		MPI_Status statuses[2 * NUMBEROFADJACENT];
		int nreq = 0;
		
		sendTrigger(rank, adjacentNodes, requests, &nreq);


		recieveTriggerFromAdjacent(adjacentNodes,rank, recievedNumCurrent, requests, &nreq);

		// printf("rank : %i, nREQ : %i\n", rank, nreq);

		MPI_Waitall(nreq , requests, statuses);

		// for (int j=0; j<4; j++)
		// 	printf("Rank %i : array[%d] = %d\n",rank, j, recievedNumCurrent[j]);

		checkForTrigger(recievedNumPast, recievedNumCurrent, adjacentNodes, rank);

		memcpy(recievedNumPast, recievedNumCurrent, sizeof(int) * 4); 
		
		// break;
		// printf("Sleeping for 5 second.\n");
   		sleep(2);
		
		   

	}

 }



void initializeNodes(int rank, int baseStation){

	int totalChar = 33;
	int packsize = totalChar * sizeof(unsigned char);
	unsigned char packbuf[packsize];
	int position = 0;
	// unsigned char mac_address[17] = "00:0a:95:9d:68:16";
	// unsigned char ip_address[16] = "192.168.255.255";

	unsigned char mac_address[17] = "abcdefghijklmnopq";
	unsigned char ip_address[16] = "192.168.255.120";

	// Get ip address
	// getIPAddress();


	// Pack up the data

	MPI_Pack( &mac_address, 17, MPI_UNSIGNED_CHAR, packbuf, packsize, &position, MPI_COMM_WORLD );
	MPI_Pack( &ip_address, 16, MPI_UNSIGNED_CHAR, packbuf, packsize, &position, MPI_COMM_WORLD );

	MPI_Send(packbuf, position, MPI_PACKED, baseStation, 0, MPI_COMM_WORLD);

	// printf("data sent to %i\n", baseStation);
}

int checkForTrigger(int* recievedNumPast, int* recievedNumCurrent,int* adjacentNodes,int rank){


	int sendArrayLevel1[NUMBEROFADJACENT];
	int sendArrayLevel2[NUMBEROFADJACENT];

	int level1Count;
	int level2Count;

	int level1event = 0;
	int level2event = 0;

	// printf("----------------\n");
	// for (int j=0; j<4; j++)
		// printf("Rank %i : array[%d] = %d\n",rank, j, recievedNumCurrent[j]);
	printf("PAST : %i : [%i, %i, %i, %i] -> base\n", rank, recievedNumPast[0], recievedNumPast[1], recievedNumPast[2], recievedNumPast[3]);
	printf("CURRENT : %i : [%i, %i, %i, %i] -> base\n", rank, recievedNumCurrent[0], recievedNumCurrent[1], recievedNumCurrent[2], recievedNumCurrent[3]);

	for (int i=0; i < NUMBEROFADJACENT / 2;i++){

		memset(sendArrayLevel2,-1,4*sizeof(int));

		int level2Count = 0;

		for (int j = i + 1; j < NUMBEROFADJACENT; j++){

			if ((recievedNumCurrent[i] == recievedNumPast[j] | recievedNumCurrent[i] == recievedNumCurrent[j]) && recievedNumCurrent[i] != -1){
					sendArrayLevel2[i] = 2;
					sendArrayLevel2[j] = 2;
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
				
				sendArrayLevel1[i] = 1;
				sendArrayLevel1[k] = 1;

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

		if (level1event == 1){
			printf("Rank : %i triggered level 1 event\n", rank);
		}

		if (level2event == 1){
			printf("Rank : %i triggered level 2 event\n", rank);
		}

		unsigned long timestamp = time(NULL);

		// Send to base station
		printf("%lu : %i : [%i, %i, %i, %i] -> base\n", timestamp, rank, sendArrayLevel1[0], sendArrayLevel1[1], sendArrayLevel1[2], sendArrayLevel1[3]);

	}
	return 0;
}

int recieveTriggerFromAdjacent(int* adjacentNodes,int rank, int* recievedNumCurrent, MPI_Request* req, int* nreq){

	for (int index = 0; index < NUMBEROFADJACENT; index++){
		if (adjacentNodes[index] != -1){
			MPI_Irecv(&recievedNumCurrent[index], 1, MPI_INT, adjacentNodes[index], 0, MPI_COMM_WORLD, &req[*nreq]);

			// printf("Recieving %i -> %i; Value : %i \n",adjacentNodes[index],rank,recievedNumCurrent[index] );
			*nreq = *nreq + 1;
		}
		
	}

	return 0;

}



int sendTrigger(int rank, int* adjacentNodes, MPI_Request* req, int* nreq){
	int randNum;
	randNum = getRandomNumber(rank);


	// for (int j=0; j<4; j++)
	// 	printf("Rank %i : array[%d] = %d\n",rank, j, adjacentNodes[j]);


	for (int index = 0; index < NUMBEROFADJACENT; index++){
		
		if (adjacentNodes[index] != -1){
			// printf("Sending %i -> %i : Value : %i; \n", rank,adjacentNodes[index], randNum);
			MPI_Isend(&randNum, 1, MPI_INT, adjacentNodes[index], 0, MPI_COMM_WORLD, &req[*nreq]);
			*nreq = *nreq + 1;
		}
		
	}

	return 0;
}

int getRandomNumber(int rank){
	// Setting the seed
	srand((int) time(NULL) ^ rank);
	srand(rand());
	return rand() % MAX_RANDOM;
	
}


int getAdjacentNodes(int rank, int *ajacentNodesArr){

	// for (int j=0; j<4; j++)
	// 	printf("Rank %i : array[%d] = %d\n",rank, j, ajacentNodesArr[j]);

	int rowIndex = rank / WIDTH;
	int columnIndex = rank % WIDTH;
	
	// Left sibling
	int leftSiblingcol = columnIndex - 1;

	if (leftSiblingcol >= 0){
		ajacentNodesArr[0] = rowIndex*WIDTH + leftSiblingcol;
	}

	// Right Sibling
	int rightSiblingcol = columnIndex + 1;

	if (rightSiblingcol < WIDTH) {
		ajacentNodesArr[1] = rowIndex*WIDTH + rightSiblingcol;
	}

	// Top Sibling
	int topSiblingrow = rowIndex - 1;

	if (topSiblingrow >= 0) {
		ajacentNodesArr[2] = topSiblingrow*WIDTH + columnIndex;
	}

	
	// Bottom Sibling
	int bottomSiblingrow = rowIndex + 1;

	if (bottomSiblingrow < HEIGHT) {
		ajacentNodesArr[3] = bottomSiblingrow*WIDTH + columnIndex;
	}

	return 0;

}


// --------------------------------------------------------------------------------------------------------------------


void base(int rank){

	unsigned char* macAddressStorage = (unsigned char*)  malloc(WIDTH * HEIGHT * 17 * sizeof(unsigned char));
	unsigned char* ipAddressStorage = (unsigned char*) malloc(WIDTH * HEIGHT * 16 * sizeof(unsigned char));

	initializeBaseStation(macAddressStorage, ipAddressStorage);

	// printf("MACCC : %s\n", &macAddressStorage[0]);
	// for (int i = 0; i < 20; i++){
		// printf("Rank : %i, MAC : %s IP: %s\n", i, macAddressStorage + i*sizeof(unsigned char)*17, ipAddressStorage + i*sizeof(unsigned char)*16);
		// printf("Rank : %i, MAC : %s IP: %s\n", i, macAddressStorage[i], ipAddressStorage[i]);
	// }

	// while (1){

	// 	MPI_Recv(packbuf, packsize, MPI_PACKED, MPI_ANY_SOURCE , 0, MPI_COMM_WORLD, &stat);

	// }

}


 void initializeBaseStation(unsigned char* macaddressstorage, unsigned char* ipaddressstorage){

	MPI_Status stat;
	int totalChar = 33;
	int packsize = totalChar;
	unsigned char packbuf[packsize];
	int incoming_rank;

	int position = 0;
	int count = 0;

	while (count < (WIDTH * HEIGHT)){

		position = 0;
		MPI_Recv(packbuf, packsize, MPI_PACKED, MPI_ANY_SOURCE , 0, MPI_COMM_WORLD, &stat);

		// printf("PACK : %s\n", &packbuf[0]);

		incoming_rank = stat.MPI_SOURCE;
		// printf("Rank : %i\n", incoming_rank);
		MPI_Unpack(packbuf, packsize, &position, macaddressstorage + incoming_rank*sizeof(unsigned char)*17, 17, MPI_UNSIGNED_CHAR, MPI_COMM_WORLD);
		MPI_Unpack(packbuf, packsize, &position, ipaddressstorage + incoming_rank*sizeof(unsigned char)*16, 16, MPI_UNSIGNED_CHAR, MPI_COMM_WORLD);

		count += 1;
		

		


	}


	// for (j=0; j<4; j++)
	// 	printf("array[%d] = %d\n", j, adjacentNodes[j]);
	



	//  printf("Initialized Base\n");



 }

// void getIPAddress(){
// 	// http://www.geekpage.jp/en/programming/linux-network/get-ipaddr.php
// 	struct ifreq ifr;
//     struct ifconf ifc;
//     char buf[1024];
//     int success = 0;

//     int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
//     if (sock == -1) { /* handle error*/ };

//     ifc.ifc_len = sizeof(buf);
//     ifc.ifc_buf = buf;
//     if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) { /* handle error */ }

//     struct ifreq* it = ifc.ifc_req;
//     const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

//     for (; it != end; ++it) {
//         strcpy(ifr.ifr_name, it->ifr_name);
//         if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
//             if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
//                 if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
//                     success = 1;
//                     break;
//                 }
//             }
//         }
//         else { /* handle error */ }
//     }

//     unsigned char mac_address[6];

//     if (success) memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
// }
