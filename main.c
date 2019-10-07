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

#define MAX_RANDOM 3
#define NUMBEROFADJACENT 4


// global varaible definition
int numtasks, rank;
int baseStation;

int WIDTH;
int HEIGHT;

int refreshInteval;
int iterations;

unsigned char* macAddressStorage;
unsigned char* ipAddressStorage;

MPI_Comm NODE_COMM;
// --------------------------------------------------------------------------------------------------------------------------------
// Function Declarations

void node();
int recieveTriggerFromAdjacent(int* adjacentNodes, int* recievedNumCurrent, MPI_Request* req, int* nreq);
int sendTrigger(int* adjacentNodes, MPI_Request* req, int* nreq);
int getRandomNumber();
int getAdjacentNodes(int *ajacentNodesArr, int currentRank);
int checkForTrigger(int* recievedNumPast, int* recievedNumCurrent,int* adjacentNodes);
void getIPAddress();

void printBanner();
void initializeSystem();

void initializeBaseStation();
void initializeNodes();
void base();
void listenToEvents();

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
	MPI_Comm_split(MPI_COMM_WORLD, rank == numtasks - 1, 0, &NODE_COMM);


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
	baseStation = numtasks - 1;

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

		// printf("\n");


	}

	MPI_Bcast( &WIDTH, 1, MPI_INT, baseStation, MPI_COMM_WORLD );
	MPI_Bcast( &HEIGHT, 1, MPI_INT, baseStation, MPI_COMM_WORLD );

	MPI_Bcast( &iterations, 1, MPI_INT, baseStation, MPI_COMM_WORLD );
	MPI_Bcast( &refreshInteval, 1, MPI_INT, baseStation, MPI_COMM_WORLD );

	
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


	while (iterations >= 0){
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
		
		// break;
   		sleep(refreshInteval);

		iterations -= 1;
		
		   

	}

	int stop = -1;
	int position = 0;
	int totalChar = 33;
	int packsize = totalChar * sizeof(unsigned char);
	unsigned char packbuf[packsize];

	MPI_Pack(&stop, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
	MPI_Send(packbuf, position, MPI_PACKED, baseStation, 1, MPI_COMM_WORLD);

 }



void initializeNodes(){

	int totalChar = 33;
	int packsize = totalChar * sizeof(unsigned char);
	unsigned char packbuf[packsize];
	int position = 0;
	// unsigned char mac_address[17] = "00:0a:95:9d:68:16";
	// unsigned char ip_address[16] = "192.168.255.255";

	unsigned char mac_address[17] = "abcdefghijklmnopq";
	unsigned char ip_address[15] = "192.168.255.120";

	// Get ip address
	// getIPAddress();


	// Pack up the data

	MPI_Pack( &mac_address, 17, MPI_UNSIGNED_CHAR, packbuf, packsize, &position, MPI_COMM_WORLD );
	MPI_Pack( &ip_address, 15, MPI_UNSIGNED_CHAR, packbuf, packsize, &position, MPI_COMM_WORLD );

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
				sendArrayLevel2[i] = 9;
				sendArrayLevel2[j] = 9;
				level2match = recievedNumCurrent[i];
				level2Count+=1;
			}else if(recievedNumCurrent[i] == recievedNumCurrent[j] && recievedNumCurrent[i] != -1){
				sendArrayLevel2[i] = 8;
				sendArrayLevel2[j] = 8;
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
				
				sendArrayLevel1[i] = 1;
				sendArrayLevel1[k] = 1;

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
		
		int packsize = (sizeof(int) * 5 + sizeof(unsigned long));
		unsigned char packbuf[packsize];

		
		int position = 0;

		if (level1event == 1){
			// printf("Rank : %i triggered level 1 event\n", rank);
			MPI_Pack( &level1match, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
			MPI_Pack( &sendArrayLevel1, 4, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
			

		}else if (level2event == 1){
			// printf("Rank : %i triggered level 2 event\n", rank);
			MPI_Pack( &level2match, 1, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
			MPI_Pack( &sendArrayLevel2, 4, MPI_INT, packbuf, packsize, &position, MPI_COMM_WORLD );
			

		}
		

		double timestamp = MPI_Wtime();

		// Send to base station
		// printf("%lu : %i : [%i, %i, %i, %i] -> base\n", timestamp, rank, sendArrayLevel1[0], sendArrayLevel1[1], sendArrayLevel1[2], sendArrayLevel1[3]);

		MPI_Pack( &timestamp, 1, MPI_DOUBLE, packbuf, packsize, &position, MPI_COMM_WORLD );
		
		// printf("Send %i : %s\n", rank, &packbuf);
		
		MPI_Send(packbuf, position, MPI_PACKED, baseStation, 1, MPI_COMM_WORLD);


	}
	return 0;
}

int recieveTriggerFromAdjacent(int* adjacentNodes, int* recievedNumCurrent, MPI_Request* req, int* nreq){

	for (int index = 0; index < NUMBEROFADJACENT; index++){
		if (adjacentNodes[index] != -1){
			MPI_Irecv(&recievedNumCurrent[index], 1, MPI_INT, adjacentNodes[index], 0, NODE_COMM, &req[*nreq]);

			// printf("Recieving %i -> %i; Value : %i \n",adjacentNodes[index],rank,recievedNumCurrent[index] );
			*nreq = *nreq + 1;
		}
		
	}

	return 0;

}



int sendTrigger(int* adjacentNodes, MPI_Request* req, int* nreq){
	int randNum;
	randNum = getRandomNumber(rank);


	// for (int j=0; j<4; j++)
	// 	printf("Rank %i : array[%d] = %d\n",rank, j, adjacentNodes[j]);


	for (int index = 0; index < NUMBEROFADJACENT; index++){
		
		if (adjacentNodes[index] != -1){
			// printf("Sending %i -> %i : Value : %i; \n", rank,adjacentNodes[index], randNum);
			MPI_Isend(&randNum, 1, MPI_INT, adjacentNodes[index], 0, NODE_COMM, &req[*nreq]);
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

	int rowIndex = currentRank / WIDTH;
	int columnIndex = currentRank % WIDTH;
	
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


void base(){

	macAddressStorage = (unsigned char*)  malloc(WIDTH * HEIGHT * 17 * sizeof(unsigned char));
	ipAddressStorage = (unsigned char*) malloc(WIDTH * HEIGHT * 15 * sizeof(unsigned char));

	initializeBaseStation();


	listenToEvents();

	// printf("MACCC : %s\n", &macAddressStorage[0]);
	// for (int i = 0; i < 20; i++){
		// printf("Rank : %i, MAC : %s IP: %s\n", i, macAddressStorage + i*sizeof(unsigned char)*17, ipAddressStorage + i*sizeof(unsigned char)*16);
		// printf("Rank : %i, MAC : %s IP: %s\n", i, macAddressStorage[i], ipAddressStorage[i]);
	// }

	// while (1){

	// 	MPI_Recv(packbuf, packsize, MPI_PACKED, MPI_ANY_SOURCE , 0, MPI_COMM_WORLD, &stat);

	// }

}

void listenToEvents(){
	MPI_Status stat;
	int packsize = (sizeof(int) * 5 + sizeof(unsigned long));
	unsigned char packbuf[packsize];


	int activatedNodes[4];
	int incomingNode;
	int matchedValue;
	double eventTimeStamp;
	int position;

	int stopCount = 0;

	int adjacentNodes[NUMBEROFADJACENT];

	while (1){

		position = 0;

		MPI_Recv(packbuf, packsize, MPI_PACKED, MPI_ANY_SOURCE , 1, MPI_COMM_WORLD, &stat);
		
		incomingNode = stat.MPI_SOURCE;
		MPI_Unpack(packbuf, packsize, &position, &matchedValue, 1, MPI_INT, MPI_COMM_WORLD);

		// Stopping message
		if (matchedValue == -1){

			// printf("Incoming %i\n", incomingNode);
			if (stopCount == (WIDTH * HEIGHT) - 1){
				break;
			}
			stopCount+= 1;
			continue;
		}


		MPI_Unpack(packbuf, packsize, &position, &activatedNodes, 4, MPI_INT, MPI_COMM_WORLD);
		MPI_Unpack(packbuf, packsize, &position, &eventTimeStamp, 1, MPI_DOUBLE, MPI_COMM_WORLD);

		char timeInDateTime[80];
		convertToTimeStamp(timeInDateTime, 80);

		// Get the adjacent nodes
		getAdjacentNodes(adjacentNodes, incomingNode);

		double commTime = MPI_Wtime() - eventTimeStamp;

		FILE *fp;

        fp = fopen("log.txt", "a+");
		
		fprintf (fp, "%s", "------------------------------------------------------\n");
		fprintf (fp, "Time : %s\n", timeInDateTime);

		fprintf (fp, "%s", "\n");

		fprintf (fp, "%s", "Activated Node\n");
		fprintf (fp, "%i", incomingNode);
		fprintf (fp, "%s", "\t\t");
		fwrite(macAddressStorage + incomingNode*sizeof(unsigned char)*17 , 17 , sizeof(unsigned char) , fp );
		fprintf (fp, "%s", "\t\t");
		fwrite(ipAddressStorage + incomingNode*sizeof(unsigned char)*15 , 15 , sizeof(unsigned char) , fp );

		fprintf (fp, "%s", "\n\n");

		int labelFlag = 0;

		for (int i = 0; i < 4; i++){
			if (activatedNodes[i] != -1){
				
				if (labelFlag == 0 && activatedNodes[i] == 1){
					fprintf (fp, "%s", "Event Type : First Level Event\n\n");
					fprintf (fp, "%s", "Adjacent Nodes\n");
					labelFlag = 1;
				}else if (labelFlag == 0 && activatedNodes[i] > 1){
					fprintf (fp, "%s", "Event Type : Second Level Event\n\n");
					fprintf (fp, "%s", "Adjacent Nodes\n");
					labelFlag = 1;
				}

				fprintf (fp, "%i", adjacentNodes[i]);
				fprintf (fp, "%s", "\t\t");
				fwrite(macAddressStorage + activatedNodes[i]*sizeof(unsigned char)*17 , 17 , sizeof(unsigned char) , fp );
				fprintf (fp, "%s", "\t\t");
				fwrite(ipAddressStorage + activatedNodes[i]*sizeof(unsigned char)*15 , 15 , sizeof(unsigned char) , fp );
				fprintf (fp, "%s", "\t\t");

				if (activatedNodes[i] == 1 || activatedNodes[i] == 8){
					fprintf (fp, "%i", 1);
				}else{
					fprintf (fp, "%i", 2);
				}
				fprintf (fp, "%s", "\n");
				}
			}

		fprintf (fp, "%s", "\n\n");

		fprintf (fp, "Triggered Value : %i\n", matchedValue);
		fprintf (fp, "Communication Time : %f\n", commTime);

        fclose(fp);

	}


}

// void logData(double startTime, int incomingNode, int triggerValue, int* activeNodes){

// 	// event time


// 	// 
// }

 void initializeBaseStation(){

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
		MPI_Unpack(packbuf, packsize, &position, macAddressStorage + incoming_rank*sizeof(unsigned char)*17, 17, MPI_UNSIGNED_CHAR, MPI_COMM_WORLD);
		MPI_Unpack(packbuf, packsize, &position, ipAddressStorage + incoming_rank*sizeof(unsigned char)*15, 15, MPI_UNSIGNED_CHAR, MPI_COMM_WORLD);

		count += 1;

	}

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


void convertToTimeStamp(char* buf, int size){
	struct tm  ts;
	time_t     now;
	time(&now);
    ts = *localtime(&now);
    strftime(buf, size, "%a %Y-%m-%d %H:%M:%S", &ts);
    // year_pr = ts.tm_year;
    // printf("Local Time %s\n", buf);

}