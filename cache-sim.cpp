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


struct Instruction** parse(char* argv[], int size);
int getSize(char* argv[]);
int* directMappedCache(struct Instruction** instructions, int size);
int setAssociativeCache(struct Instruction** instructions, int size, int numWays);
vector<vector<int>> updateLRU(vector<vector<int>> LRU, int index, int way);
void printVector(vector<int> LRU);

int main(int argc, char* argv[])
{
	int size = getSize(argv);
	printf("There are %d lines\n", size);
	struct Instruction **instructions = parse(argv, size);
	/*printf("Here are the first 20 instructions\n");
	for(int i = 0; i < 20; i++)
	{
		if(instructions[i]->operation == 0)
			printf("Operation: Load \t");
		else if(instructions[i]->operation == 1)
			printf("Operations: Store\t");
		printf("Address: %lx\n", instructions[i]->address);
	}*/
	int* directMapped = directMappedCache(instructions, size);
	for(int i = 0; i < 4; i++)
	{
		//printf("directMapped[%d]: %d\n", i, directMapped[i]);
	}

	int* setAssociative = (int *)malloc(sizeof(int) * 4);
	setAssociative[0] = setAssociativeCache(instructions, size, 2);
	cout << "Done with 2 way." << endl;
	/*setAssociative[1] = setAssociativeCache(instructions, size, 4);
	cout << "Done with 4 way." << endl;
	setAssociative[2] = setAssociativeCache(instructions, size, 8);
	cout << "Done with 8 way." << endl;
	setAssociative[3] = setAssociativeCache(instructions, size, 16);
	cout << "Done with 16 way." << endl;
	*/

	for(int i = 0; i < 1; i++)
	{
		printf("setAssocativeCache[%d]: %d\n", i, setAssociative[i]);
	}

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

	printf("instructions initialized\n");

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
	int numSets = 512 / numWays, numHits = 0, hitSwitch = 0, index, tag, mostRecentlyUsed = 0;
	int cacheTable[numSets][numWays];
	vector<vector<int>> LRU(numSets, vector<int>(numWays));

	for(int i = 0; i < numSets; i++)
	{
		for(int j = 0; j < numWays; j++)
		{
			LRU[i][j] = j;
		}
	}
	cout << "LRU is initialized." << endl;
	

	for(int i = 0; i < numSets; i++)
	{
		for(int j = 0; j < numWays; j++)
		{
			cacheTable[i][j] = 0;
		}
	}
	cout << "Cache table is initialized." << endl;

	for(int i = 0; i < size; i++)
	{

		index = (instructions[i]->address / 32) % numSets;
		tag = instructions[i]->address >>(int)(log2(numSets) + 5);
		for(int n = 0; n < numWays; n++)
		{
			if(cacheTable[index][n] == tag)
			{
				numHits++;
				hitSwitch = 1;
				mostRecentlyUsed = n;
				LRU = updateLRU(LRU, index, mostRecentlyUsed);
				break; //end the loop
			}
			else if(n == (numWays - 1) && hitSwitch == 0)//there is a miss
			{
				LRU = updateLRU(LRU, index, mostRecentlyUsed);
				cacheTable[index][LRU[index].at(0)] = tag;

			}
		}
		hitSwitch = 0;
	}
	return numHits;
}

vector<vector<int>> updateLRU(vector<vector<int>> LRU, int index, int way)
{
	int temp = LRU[index][way];
	LRU[index].erase(LRU[index].begin() + way);
	LRU[index].push_back(temp);
	return LRU;
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









	
