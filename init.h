#define MAX_RANDOM 10
#define NUMBEROFADJACENT 4
#define packsize 16000
#define ENCRYPT_COMM 1

#define CBC 1
#define CTR 1
#define ECB 1

void convertToTimeStamp(char* buf, int size);
int getAdjacentNodes(int *ajacentNodesArr, int currentRank);
void* checkStop(void * arg);

#include "AES/aes.h"