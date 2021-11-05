#ifndef MY_CACHE_   /* Include guard */
#define MY_CACHE_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MaxKeyLength 100
#define MaxValueLength 10485760

struct CacheUnit{
    char *key;
    char *value;
    int maxAge; // in seconds
    long enteringTime; // in seconds
    long lastUsedTime; // in seconds
    int headerLength;
    int contentLength;
    int valueLength;
};

struct MyCache{
    int cacheSize;
    struct CacheUnit *cacheUnits;
};

struct MyCache initializeMyCache(int cacheSize);
struct MyCache putIntoMyCache(char *key, char *value, int maxAge, int valueLength, struct MyCache myCache);
struct MyCache getFromMyCache(char *key, char *value, int *valueLength, struct MyCache myCache);
int getAge(char *key, struct MyCache myCache);

#endif // MY_CACHE_