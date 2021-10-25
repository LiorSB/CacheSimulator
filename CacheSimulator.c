#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define FULLY_ASSOCIATIVE 0
#define ONE_WAY 1
#define TWO_WAY 2
#define ADDRESS_SIZE 32

typedef struct 
{
	int tag;
	int set;
} AddressFields;

typedef struct 
{
	AddressFields* fields;
	int* lru;
	int capacity;
} CacheInformation;

float getMissRate(FILE* fp, int cacheSize, int associativity, int blockSize);
void initializeCache(CacheInformation* cache, int cacheSize, int associativity, int blockSize);
void convertAddressToBinary(AddressFields* fields, char* line, int setSize, int tagSize);
void associateAddress(CacheInformation* cache, AddressFields fields, int associativity, int* missCounter);
void fullyAssociative(CacheInformation* cache, AddressFields fields, int* missCounter);
void oneWay(CacheInformation* cache, AddressFields fields, int* missCounter);
void twoWay(CacheInformation* cache, AddressFields fields, int* missCounter);

int main(int argc, char* argv[])
{
	FILE* traceFile = fopen(argv[4], "r");

	if (traceFile == NULL)
	{
		printf("Error in opening file!\n");
		exit(EXIT_FAILURE);
	}

	printf("Miss Rate: %g%%\n", getMissRate(traceFile, atoi(argv[1]), atoi(argv[2]), atoi(argv[3])));

	fclose(traceFile);
	return 0;
}

float getMissRate(FILE* fp, int cacheSize, int associativity, int blockSize)
{
	CacheInformation cache;

	initializeCache(&cache, cacheSize, associativity, blockSize);

	int offsetSize = (int)log2(blockSize);
	int setSize = 0;
	int tagSize = 0;

	if (associativity == TWO_WAY)
	{
		setSize = (int)log2(cache.capacity / 2);
	}
	else
	{
		setSize = (int)log2(cache.capacity);
	}

	if (associativity == FULLY_ASSOCIATIVE)
	{
		tagSize = ADDRESS_SIZE - offsetSize;
	}
	else
	{
		tagSize = ADDRESS_SIZE - offsetSize - setSize;
	}

	char data[12];
	int dataCounter = 0;
	int missCounter = 0;

	while (fscanf(fp, "%s", data) != EOF)
	{
		dataCounter++;

		AddressFields fields = { 0, 0 };

		convertAddressToBinary(&fields, data, setSize, tagSize);
		associateAddress(&cache, fields, associativity, &missCounter);
	}

	free(cache.lru);
	free(cache.fields);

	return (((float)missCounter / dataCounter) * 100);
}

void initializeCache(CacheInformation* cache, int cacheSize, int associativity, int blockSize)
{
	cache->capacity = (int)(cacheSize / blockSize);
	cache->fields = (AddressFields*)malloc(cache->capacity * sizeof(AddressFields));

	if (associativity == TWO_WAY)
	{
		cache->lru = (int*)calloc( cache->capacity / 2, sizeof(int));
	}
	else
	{
		cache->lru = (int*)calloc(cache->capacity, sizeof(int));
	}

	if (cache->fields == NULL || cache->lru == NULL)
	{
		printf("Error in allocating memory!\n");
		exit(EXIT_FAILURE);
	}
}

void convertAddressToBinary(AddressFields* fields, char* data, int setSize, int tagSize)
{
	int binaryNumber[ADDRESS_SIZE] = { 0 };
	long long dataAddress = strtoll(data, NULL, 10); // Converts a string to a long long
	int i = ADDRESS_SIZE - 1;

	while (dataAddress > 0 || i == 0)
	{
		binaryNumber[i] = dataAddress % 2;
		dataAddress = dataAddress / 2;
		i--;
	}

	for (int i = 0; i < tagSize; i++)
	{
		fields->tag += binaryNumber[i] * pow(2, tagSize - (i + 1));
	}

	int addressSize = tagSize + setSize;

	for (int i = tagSize; i < addressSize; i++)
	{
		fields->set += binaryNumber[i] * pow(2, addressSize - (i + 1));
	}
}

void associateAddress(CacheInformation* cache, AddressFields fields, int associativity, int* missCounter)
{
	switch (associativity)
	{
		case FULLY_ASSOCIATIVE:
		{
			fullyAssociative(cache, fields, missCounter);
			break;
		}
		case ONE_WAY:
		{
			oneWay(cache, fields, missCounter);
			break;
		}
		case TWO_WAY:
		{
			twoWay(cache, fields, missCounter);
			break;
		}
		default:
		{
			exit(EXIT_FAILURE);
		}
	}
}

void fullyAssociative(CacheInformation* cache, AddressFields fields, int* missCounter)
{
	int isHit = 0;

	for (int i = 0; i < cache->capacity; i++)
	{
		if (cache->fields[i].tag == fields.tag)
		{
			cache->lru[i]++;
			isHit = 1;
			break;
		}
	}

	if (!isHit)
	{
		(*missCounter)++;
		int lruMinimum = cache->lru[0];
		int lruIndex = 0;

		for (int i = 1; i < cache->capacity; i++)
		{
			if (cache->lru[i] < lruMinimum)
			{
				lruMinimum = cache->lru[i];
				lruIndex = i;
			}
		}

		cache->fields[lruIndex].tag = fields.tag;
		cache->lru[lruIndex]++;
	}
}

void oneWay(CacheInformation* cache, AddressFields fields, int* missCounter)
{
	if (cache->fields[fields.set].tag == fields.tag)
	{
		return;
	}

	(*missCounter)++;
	cache->fields[fields.set].tag = fields.tag;
}

void twoWay(CacheInformation* cache, AddressFields fields, int* missCounter)
{
	if (cache->fields[fields.set].tag == fields.tag)
	{
		cache->lru[fields.set] = 1;
		return;
	}

	if (cache->fields[fields.set + ADDRESS_SIZE].tag == fields.tag)
	{
		cache->lru[fields.set] = 2;
		return;
	}
	
	(*missCounter)++;

	if (cache->lru[fields.set] == ONE_WAY)
	{
		cache->fields[fields.set].tag = fields.tag;
		return;
	}
	
	if (cache->lru[fields.set] == TWO_WAY)
	{
		cache->fields[fields.set + ADDRESS_SIZE].tag = fields.tag;
		return;
	}

	cache->fields[fields.set].tag = fields.tag;
	cache->lru[fields.set] = 1;
}