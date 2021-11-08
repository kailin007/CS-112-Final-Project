#ifndef MY_CACHE_ /* Include guard */
#define MY_CACHE_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct CacheUnit
{
    char *key;
    char *value;
    int maxAge;        // in seconds
    long enteringTime; // in seconds
    long lastUsedTime; // in seconds
    int valueLength;
};

struct MyCache
{
    int cacheSize;
    int keySize;
    int valueSize;
    struct CacheUnit *cacheUnits;
};

struct MyCache initializeMyCache(int cacheSize, int keySize, int valueSize);
int putIntoMyCache(char *key, char *value, int maxAge, int valueLength, struct MyCache *myCache); // return 0 if saved; otherwise return -1
int getFromMyCache(char *key, char *value, int *valueLength, struct MyCache *myCache);            // return 0 if found; otherwise return -1 and value = "NA"
int getAge(char *key, struct MyCache myCache);

#endif // MY_CACHE_