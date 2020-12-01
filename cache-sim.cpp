#include<stddef.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<vector>
#include<math.h>
#include<iostream>

typedef struct Instruction
{
	int operation; // 0 if load, 1 if store
	long address; //the address for the instruction
} Instruction;

typedef struct Node
{
	int way;
	struct Node* next;
} Node;


using namespace std;

int getSize(char* argv[]);
struct Instruction** parse(char* argv[], int size);
int* directMappedCache(struct Instruction** instructions, int size);
int setAssociativeCache(struct Instruction** instructions, int size, int numWays);
int fullyAssociativeLRU(struct Instruction** instructions, int size);
vector<vector<int>> updateLRU(vector<vector<int>> LRU, int index, int way);
vector<int> updateLRU(vector<int> LRU, int way);
int storeMissNoWrite(struct Instruction** instructions, int size, int numWays);
int setAssociativeWithPrefetching(struct Instruction** instructions, int size, int numWays);
int prefetchOnMiss(struct Instruction** instructions, int size, int numWays);
void writeToOutput(char* argv[], int* results, int size);
void printVector(vector<int> LRU);

int main(int argc, char* argv[])
{
	int size = getSize(argv);
	struct Instruction **instructions = parse(argv, size);
	int* answers = (int *)malloc(sizeof(int) * 22);

	//part 1
	int* direct = directMappedCache(instructions, size);
	for(int i = 0; i < 4; i++)
	{
		answers[i] = direct[i];
	}

	//part 2
	answers[4] = setAssociativeCache(instructions, size, 2);
	answers[5] = setAssociativeCache(instructions, size, 4);
	answers[6] = setAssociativeCache(instructions, size, 8);
	answers[7] = setAssociativeCache(instructions, size, 16);

	//part 3
	answers[8] = fullyAssociativeLRU(instructions, size);
	answers[9] = size - 123456; //if by some miracle this returns the right answer I'll be shocked to my core.
	
	//part 4
	answers[10] = storeMissNoWrite(instructions, size, 2);
	answers[11] = storeMissNoWrite(instructions, size, 4);
	answers[12] = storeMissNoWrite(instructions, size, 8);
	answers[13] = storeMissNoWrite(instructions, size, 16);

	//part 5
	answers[14] = setAssociativeWithPrefetching(instructions, size, 2);
	answers[15] = setAssociativeWithPrefetching(instructions, size, 4);
	answers[16] = setAssociativeWithPrefetching(instructions, size, 8);
	answers[17] = setAssociativeWithPrefetching(instructions, size, 16);

	//part 6
	answers[18] = prefetchOnMiss(instructions, size, 2);
	answers[19] = prefetchOnMiss(instructions, size, 4);
	answers[20] = prefetchOnMiss(instructions, size, 8);
	answers[21] = prefetchOnMiss(instructions, size, 16);
	
	writeToOutput(argv, answers, size);

	return 0;


}

int getSize(char* argv[])
{
	FILE *input = fopen(argv[1], "r");
	if(input == NULL)
	{
		printf("Error opening file\n");
		exit(1);
	}
	int numLines = 0;
	char c;
	while((c = getc(input)) != EOF)
	{
		if(c == '\n')
			numLines++;
	}
	return numLines;
}


struct Instruction** parse(char* argv[], int size)
{

	// Temporary variables
	char operation;
	// Open file for reading
	FILE *input = fopen(argv[1], "r");
	if(input == NULL)
	{
		printf("Error opening file\n");
		exit(1);
	}
	struct Instruction** instructions = (struct Instruction**)malloc(size*sizeof(struct Instruction*));
	for(int i = 0; i < size; i++)
	{
		instructions[i] = (struct Instruction*)malloc(sizeof(struct Instruction));
	}

	int instructionIterator = 0, tempAddressIterator, operationSwitch = 0, whitespaceSwitch = 0, addressSwitch = 0;
	char c, tempAddress[8], *eptr;

	for(int i = 0; i < 8; i++)
	{
		tempAddress[i] = ' ';
	}

	while((c = fgetc(input)) != EOF) 
	{	
		if(operationSwitch == 0)
		{
			if(c == 'L')
			{
				instructions[instructionIterator]->operation = 0;
			}
			else if(c == 'S')
			{
				
				instructions[instructionIterator]->operation = 1;
			}
			else
			{
				printf("Unknown operation!\n");
				exit(1);
			}
			operationSwitch++;
		}
		else if(operationSwitch == 1 && whitespaceSwitch == 0)
		{	
			whitespaceSwitch++;
			c = fgetc(input);
			continue;
		}
		else if(operationSwitch == whitespaceSwitch && addressSwitch == 0)
		{
			if(c == 'x') //ignore these characters
			{
				continue;
			}
			else if(c == '\n')	//this is the end of the address, convert it to a string and assign it to the instruction
			{
				instructions[instructionIterator]->address = strtol(tempAddress, &eptr, 16);
				operationSwitch = 0;
				whitespaceSwitch = 0;
				addressSwitch = 0;
				for (int i = 0; i < 8; i++)
					tempAddress[i] = ' ';
				tempAddressIterator = 0;
				instructionIterator++;
			}
			else //have this build toward the address
			{
				if(tempAddressIterator < 8)
				{
					tempAddress[tempAddressIterator] = c;
					tempAddressIterator++;
				}
			}
				
  		}
	}
	return instructions;
}

int* directMappedCache(struct Instruction** instructions, int size)
{
	int  tagBits[] = {5,7,9,10}, indexModular[] = {32, 128, 512, 1024};
	int* numHits = (int*)malloc(4 * sizeof(int));
	for(int i = 0; i < 4; i++)
	{
		numHits[i] = 0;
	}
	long *cacheTable = (long*) malloc(sizeof(long) * 32768);
	for(int i = 0; i < 1024; i++)
		cacheTable[i] = -1;
	long index, tag;
	for(int i = 0; i < 4; i++)
	{
		for(int j = 0; j < size; j++)
		{
			//determine index and tag values
			index = (instructions[j]->address / 32) % indexModular[i];
			tag = instructions[j]->address >> tagBits[i];

			//determine whether hit or miss
			if(cacheTable[index] == -1) //miss, junk value
			{
				cacheTable[index] = tag;
			}
			else if(cacheTable[index] == tag) //hit, tag matches tag of current instruction
			{
				numHits[i]++;
			}
			else //miss, tag does not match tag of current instruction
			{
				cacheTable[index] = tag;
			}
			
		}
		for(int i = 0; i < 32768; i++)
		{
			cacheTable[i] = -1;
		}
	}
	return numHits;
}

int setAssociativeCache(struct Instruction** instructions, int size, int numWays)
{
	int numSets = 512 / numWays, numHits = 0, index, tag;
	int cacheTable[numSets][numWays];
	vector<vector<int>> LRU(numSets, vector<int>(numWays));

	for(int i = 0; i < numSets; i++)
	{
		for(int j = 0; j < numWays; j++)
		{
			LRU[i][j] = j;
		}
	}


	for(int i = 0; i < numSets; i++)
	{
		for(int j = 0; j < numWays; j++)
		{
			cacheTable[i][j] = -1;
		}
	}
	for(int i = 0; i < size; i++)
	{

		index = (instructions[i]->address / 32) % numSets;
		tag = instructions[i]->address >>(int)(log2(numSets) + 5);
		for(int n = 0; n < numWays; n++)
		{
			if(cacheTable[index][n] == tag)
			{
				numHits++;
				LRU = updateLRU(LRU, index, n);
				break; //end the loop
			}
			else if(n == numWays - 1)//there is a miss
			{
				int j = 0, junkFlag = 0;
				for(j = 0; j < numWays; j++)
				{
					if(cacheTable[index][j] == -1)
					{
						cacheTable[index][j] = tag;
						junkFlag = 1;
						LRU = updateLRU(LRU, index, LRU[index][j]);
						break;
					}
				}
				if(junkFlag == 0)
				{
					cacheTable[index][LRU[index][0]] = tag;
					LRU = updateLRU(LRU,index, LRU[index][0]);
				}
			}
		}
	}
	return numHits;
}

vector<vector<int>> updateLRU(vector<vector<int>> LRU, int index, int way)
{
	int tempIndex;
	for(int i = 0; i < LRU[index].size(); i++)
	{
		if(LRU[index][i] == way)
		{
			tempIndex = i;
			break;
		}
	}
	LRU[index].erase(LRU[index].begin() + tempIndex);
	LRU[index].push_back(way);
	return LRU;
}

int fullyAssociativeLRU(struct Instruction** instructions, int size)
{
	int cacheTable[512][1]; //doing it this way so easier to visualize
	long tag;
	vector<int> LRU;
	for(int i = 0; i < 512; i++)
	{
		cacheTable[i][0] = -1;
		LRU.push_back(i);
	}
	int numHits = 0, j, k, l, tempValue; 
	for(int i = 0; i < size; i++)
	{
		tag = instructions[i]->address >> 5;
		for(j = 0; j < 512; j++)
		{
			if(cacheTable[j][0] == tag)
			{
				numHits++;
				LRU = updateLRU(LRU, j);
				break;
			}
		}
		if(j == 512)
		{
			for(l = 0; l < 512; l++)
			{
				if(cacheTable[l][0] == -1) //junk value present
				{
					cacheTable[l][0] = tag;
					LRU = updateLRU(LRU, l);
					break;
				}
			}
			if(l == 512)
			{
				cacheTable[LRU[0]][0] = tag;
				LRU = updateLRU(LRU, LRU[0]);
			}
		}
			
	}
				
	return numHits; 
}

vector<int> updateLRU(vector<int> LRU, int way)
{
	int tempIndex;
	for(int i = 0; i < 512; i++)
	{
		if(LRU[i] == way)
		{
			tempIndex = i;
			break;
		}
	}
	LRU.erase(LRU.begin() + tempIndex);
	LRU.push_back(way);
	return LRU;
}

int storeMissNoWrite(struct Instruction** instructions, int size, int numWays)
{
	int numSets = 512 / numWays, numHits = 0, index, tag;
	int cacheTable[numSets][numWays];
	vector<vector<int>> LRU(numSets, vector<int>(numWays));

	for(int i = 0; i < numSets; i++)
	{
		for(int j = 0; j < numWays; j++)
		{
			LRU[i][j] = j;
		}
	}

	for(int i = 0; i < numSets; i++)
	{
		for(int j = 0; j < numWays; j++)
		{
			cacheTable[i][j] = -1;
		}
	}
	for(int i = 0; i < size; i++)
	{

		index = (instructions[i]->address / 32) % numSets;
		tag = instructions[i]->address >>(int)(log2(numSets) + 5);
		for(int n = 0; n < numWays; n++)
		{
			if(cacheTable[index][n] == tag)
			{
				numHits++;
				LRU = updateLRU(LRU, index, n);
				break; //end the loop
			}
			else if(n == numWays - 1 && instructions[i]->operation == 0)//there is a miss
			{
				int j = 0, junkFlag = 0;
				for(j = 0; j < numWays; j++)
				{
					if(cacheTable[index][j] == -1)
					{
						cacheTable[index][j] = tag;
						junkFlag = 1;
						LRU = updateLRU(LRU, index, LRU[index][j]);
						break;
					}
				}
				if(junkFlag == 0)
				{
					cacheTable[index][LRU[index][0]] = tag;
					LRU = updateLRU(LRU,index, LRU[index][0]);
				}
			}
		}
	}
	return numHits;
}

int setAssociativeWithPrefetching(struct Instruction** instructions, int size, int numWays)
{
	int numSets = 512 / numWays, numHits = 0, index, tag, secondAddress, secondIndex, secondTag;
	int cacheTable[numSets][numWays];
	vector<vector<int>> LRU(numSets, vector<int>(numWays));

	for(int i = 0; i < numSets; i++)
	{
		for(int j = 0; j < numWays; j++)
		{
			LRU[i][j] = j;
		}
	}

	for(int i = 0; i < numSets; i++)
	{
		for(int j = 0; j < numWays; j++)
		{
			cacheTable[i][j] = -1;
		}
	}
	for(int i = 0; i < size; i++)
	{
		index = (instructions[i]->address / 32) % numSets;
		tag = instructions[i]->address >>(int)(log2(numSets) + 5);
		secondAddress = instructions[i]->address + 32;
		secondIndex = (secondAddress / 32) % numSets;
		secondTag = secondAddress >> (int)(log2(numSets) + 5);
		for(int n = 0; n < numWays; n++)
		{
			if(cacheTable[index][n] == tag)
			{
				numHits++;
				LRU = updateLRU(LRU, index, n);
				break; //end the loop
			}
			else if(n == numWays - 1)//there is a miss
			{
				int j = 0, junkFlag = 0;
				for(j = 0; j < numWays; j++)
				{
					if(cacheTable[index][j] == -1)
					{
						cacheTable[index][j] = tag;
						junkFlag = 1;
						LRU = updateLRU(LRU, index, LRU[index][j]);
						break;
					}
				}
				if(junkFlag == 0)
				{
					cacheTable[index][LRU[index][0]] = tag;
					LRU = updateLRU(LRU,index, LRU[index][0]);
				}
			}
		}
		for(int n = 0; n < numWays; n++)
		{
			if(cacheTable[secondIndex][n] == secondTag)
			{
				LRU = updateLRU(LRU, secondIndex, n);
				break;
			}
			else if(n == numWays - 1)
			{
				int j = 0, junkFlag = 0;
				for(j = 0; j < numWays; j++)
				{
					if(cacheTable[secondIndex][j] == -1)
					{
						cacheTable[secondIndex][j] = secondTag;
						junkFlag = 1;
						LRU = updateLRU(LRU, secondIndex, LRU[secondIndex][j]);
						break;
					}
				}
				if(junkFlag == 0)
				{
					cacheTable[secondIndex][LRU[secondIndex][0]] = secondTag;
					LRU = updateLRU(LRU, secondIndex, LRU[secondIndex][0]);
				}
			}
		}
	}
	return numHits;
}

int prefetchOnMiss(struct Instruction** instructions, int size, int numWays)
{
	int numSets = 512 / numWays, numHits = 0, index, tag, secondAddress, secondIndex, secondTag, preFetchFlag;
	int cacheTable[numSets][numWays];
	vector<vector<int>> LRU(numSets, vector<int>(numWays));

	for(int i = 0; i < numSets; i++)
	{
		for(int j = 0; j < numWays; j++)
		{
			LRU[i][j] = j;
		}
	}


	for(int i = 0; i < numSets; i++)
	{
		for(int j = 0; j < numWays; j++)
		{
			cacheTable[i][j] = -1;
		}
	}

	for(int i = 0; i < size; i++)
	{
		index = (instructions[i]->address / 32) % numSets;
		tag = instructions[i]->address >>(int)(log2(numSets) + 5);
		secondAddress = instructions[i]->address + 32;
		secondIndex = (secondAddress / 32) % numSets;
		secondTag = secondAddress >> (int)(log2(numSets) + 5);
		for(int n = 0; n < numWays; n++)
		{
			if(cacheTable[index][n] == tag)
			{
				preFetchFlag = 0;
				numHits++;
				LRU = updateLRU(LRU, index, n);
				break; //end the loop
			}
			else if(n == numWays - 1)//there is a miss
			{
				preFetchFlag = 1;
				int j = 0, junkFlag = 0;
				for(j = 0; j < numWays; j++)
				{
					if(cacheTable[index][j] == -1)
					{
						cacheTable[index][j] = tag;
						junkFlag = 1;
						LRU = updateLRU(LRU, index, LRU[index][j]);
						break;
					}
				}
				if(junkFlag == 0)
				{
					cacheTable[index][LRU[index][0]] = tag;
					LRU = updateLRU(LRU,index, LRU[index][0]);
				}
			}
		}
		if(preFetchFlag == 1)
		{
			for(int n = 0; n < numWays; n++)
			{
				if(cacheTable[secondIndex][n] == secondTag)
				{
					LRU = updateLRU(LRU, secondIndex, n);
					break;
				}
				else if(n == numWays - 1)
				{
					int j = 0, junkFlag = 0;
					for(j = 0; j < numWays; j++)
					{
						if(cacheTable[secondIndex][j] == -1)
						{
							cacheTable[secondIndex][j] = secondTag;
							junkFlag = 1;
							LRU = updateLRU(LRU, secondIndex, LRU[secondIndex][j]);
							break;
						}
					}
					if(junkFlag == 0)
					{
						cacheTable[secondIndex][LRU[secondIndex][0]] = secondTag;
						LRU = updateLRU(LRU, secondIndex, LRU[secondIndex][0]);
					}
				}
			}
		}
	}
	return numHits;
}

void writeToOutput(char* argv[], int* results, int size)
{
	FILE *fp;
	fp = fopen(argv[2], "w");
	if(fp == NULL)
	{
		printf("ERROR: Cannot open file.\n");
		exit(1);
	}

	for(int i = 0; i < 22; i++)
	{
		fprintf(fp, "%d,%d; ", results[i], size);
		if(i == 3 || i == 7 || i == 8 || i == 9 || i == 13 || i == 17 || i == 21)
		{
			fprintf(fp, "\n");
		}
	}
	fprintf(fp, "\n");
	return;
}

	

void printVector(vector<int> LRU)
{
	for(int i = 0; i < LRU.size(); i++)
	{
		printf("%d, ", LRU[i]);
	}
	printf("\n");
	return;
}









	
