#include "my_cache.h"

// set cache size; set all cache units to expiration; return cache.
struct MyCache initializeMyCache(int cacheSize, int keySize, int valueSize)
{
    struct MyCache myCache;
    int i;
    long currentTime = time(NULL);
    myCache.cacheSize = cacheSize;
    myCache.keySize = keySize;
    myCache.valueSize = valueSize;
    myCache.cacheUnits = (struct CacheUnit *)malloc(cacheSize * sizeof(struct CacheUnit));
    for (i = 0; i < cacheSize; i++)
    {
        myCache.cacheUnits[i].key = (char *)malloc(myCache.keySize * sizeof(char));
        myCache.cacheUnits[i].value = (char *)malloc(myCache.valueSize * sizeof(char));
        myCache.cacheUnits[i].enteringTime = currentTime;
        myCache.cacheUnits[i].lastUsedTime = currentTime;
        myCache.cacheUnits[i].maxAge = -1;
    }
    return myCache;
}

// save into cache
int putIntoMyCache(char *key, char *value, int maxAge, int valueLength, struct MyCache *myCache)
{
    int i;
    int keyExist = 0, availablePosition = 0; // 0: false; 1: true.
    int leastUsedPosition = 0, leastUsedTime = 0;
    long currentTime = time(NULL);

    // if key exists, update cache unit
    for (i = 0; i < myCache->cacheSize; i++)
    {
        if (!strcmp(key, myCache->cacheUnits[i].key))
        {
            keyExist = 1;
            // update cache
            bzero(myCache->cacheUnits[i].value, myCache->valueSize);
            memcpy(myCache->cacheUnits[i].value, value, valueLength);
            myCache->cacheUnits[i].maxAge = maxAge;
            myCache->cacheUnits[i].enteringTime = currentTime;
            myCache->cacheUnits[i].lastUsedTime = currentTime;
            myCache->cacheUnits[i].valueLength = valueLength;
            return 0;
        }
    }

    // if key doesn't exist and there's available position, put key//value into that position
    if (!keyExist)
    {
        for (i = 0; i < myCache->cacheSize; i++)
        {
            if (currentTime - myCache->cacheUnits[i].enteringTime > myCache->cacheUnits[i].maxAge)
            {
                availablePosition = 1;
                // put into cache
                strcpy(myCache->cacheUnits[i].key, key);
                bzero(myCache->cacheUnits[i].value, myCache->valueSize);
                memcpy(myCache->cacheUnits[i].value, value, valueLength);
                myCache->cacheUnits[i].maxAge = maxAge;
                myCache->cacheUnits[i].enteringTime = currentTime;
                myCache->cacheUnits[i].lastUsedTime = currentTime;
                myCache->cacheUnits[i].valueLength = valueLength;
                return 0;
            }
        }
    }

    // if key doesn't exist and there isn't available position,
    // find the least used cache unit and replace it/
    if (!keyExist && !availablePosition)
    {
        // find least used cache unit
        for (i = 0; i < myCache->cacheSize; i++)
        {
            if (currentTime - myCache->cacheUnits[i].lastUsedTime > leastUsedTime)
            {
                leastUsedTime = currentTime - myCache->cacheUnits[i].lastUsedTime;
                leastUsedPosition = i;
            }
        }
        // replace least used cache unit with new one
        strcpy(myCache->cacheUnits[leastUsedPosition].key, key);
        bzero(myCache->cacheUnits[leastUsedPosition].value, myCache->valueSize);
        memcpy(myCache->cacheUnits[leastUsedPosition].value, value, valueLength);
        myCache->cacheUnits[leastUsedPosition].maxAge = maxAge;
        myCache->cacheUnits[leastUsedPosition].enteringTime = currentTime;
        myCache->cacheUnits[leastUsedPosition].lastUsedTime = currentTime;
        myCache->cacheUnits[leastUsedPosition].valueLength = valueLength;
        return 0;
    }

    return -1;
}

// find the valid value (in valid time) of the key; update the last used time;
// return 0 if cached; -1 otherwise.
int getFromMyCache(char *key, char *value, int *valueLength, struct MyCache *myCache)
{
    // printf("key:%s, value:%s", key, value);
    // strcpy(value, "NA");
    int i;
    long currentTime = time(NULL);
    for (i = 0; i < myCache->cacheSize; i++)
    {
        if (!strcmp(key, myCache->cacheUnits[i].key) &&
            (currentTime - myCache->cacheUnits[i].enteringTime <= myCache->cacheUnits[i].maxAge))
        {
            // printf("Age: %d\n", (currentTime - myCache.cacheUnits[i].enteringTime));
            *valueLength = myCache->cacheUnits[i].valueLength;
            memcpy(value, myCache->cacheUnits[i].value, *valueLength);
            myCache->cacheUnits[i].lastUsedTime = currentTime;
            return 0;
        }
    }
    return -1;
}

int deleteFromMyCache(char *key, struct MyCache* myCache){
    int i;
    long currentTime = time(NULL);
    for(i = 0; i < myCache->cacheSize; i++){
        if(strcmp(key, myCache->cacheUnits[i].key) == 0){
                strcpy(myCache->cacheUnits[i].key, "\0");
                bzero(myCache->cacheUnits[i].value, myCache -> valueSize);
                myCache->cacheUnits[i].enteringTime = currentTime;
                myCache->cacheUnits[i].lastUsedTime = currentTime;
                myCache->cacheUnits[i].maxAge = -1;
                myCache->cacheUnits[i].valueLength = 0;
                return 0;
            }
    }
    return -1;
}

// age = lastUsedTime - enteringTime
int getAge(char *key, struct MyCache myCache)
{
    // printf("key:%s, value:%s", key, value);
    int i;
    long currentTime = time(NULL);
    for (i = 0; i < myCache.cacheSize; i++)
    {
        // printf("key: %s, myCache.cacheUnits[i].key: %s\n", key, myCache.cacheUnits[i].key);
        if (strcmp(key, myCache.cacheUnits[i].key) == 0)
        {
            myCache.cacheUnits[i].lastUsedTime = currentTime;
            return myCache.cacheUnits[i].lastUsedTime - myCache.cacheUnits[i].enteringTime;
        }
    }
    return -1;
}

// return number of vaild used cache (not include stale and deleted)
int numUsed(struct MyCache myCache){
    int num = 0, i = 0;
    long currentTime = time(NULL);
    for(i = 0; i < myCache.cacheSize; i++){
        if (currentTime - myCache.cacheUnits[i].enteringTime < myCache.cacheUnits[i].maxAge){
            num++;
        }
    }
    return num;
}