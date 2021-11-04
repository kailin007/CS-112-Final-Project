#include "my_cache.h"

// set cache size; set all cache units to expiration; return cache.
struct MyCache initializeMyCache(int cacheSize){
    struct MyCache myCache;
    int i;
    long currentTime = time(NULL);
    myCache.cacheSize = cacheSize;
    myCache.cacheUnits = (struct CacheUnit *) malloc(cacheSize * sizeof(struct CacheUnit));
    for(i = 0; i < cacheSize; i++){
        myCache.cacheUnits[i].key = (char *) malloc(MaxKeyLength * sizeof(char));
        myCache.cacheUnits[i].value = (char *) malloc(MaxValueLength * sizeof(char));
        myCache.cacheUnits[i].enteringTime = currentTime;
        myCache.cacheUnits[i].lastUsedTime = currentTime;
        myCache.cacheUnits[i].maxAge = -1;
    }
    return myCache;
}

// save into cache
struct MyCache putIntoMyCache(char *key, char *value, int maxAge, int valueLength, struct MyCache myCache){
    int i;
    int keyExist = 0, availablePosition = 0; // 0: false; 1: true.
    int leastUsedPosition = 0, leastUsedTime = 0;
    long currentTime = time(NULL);

    // if key exists, update cache unit
    for(i = 0; i < myCache.cacheSize; i++){
        if(!strcmp(key, myCache.cacheUnits[i].key)){
            keyExist = 1;
            // update cache
            memcpy(myCache.cacheUnits[i].value, value, MaxValueLength);
            myCache.cacheUnits[i].maxAge = maxAge;
            myCache.cacheUnits[i].enteringTime = currentTime;
            myCache.cacheUnits[i].lastUsedTime = currentTime;
            myCache.cacheUnits[i].valueLength = valueLength;
            break;
        }
    }

    // if key doesn't exist and there's available position, put key//value into that position
    if(!keyExist){
        for(i = 0; i < myCache.cacheSize; i++){
            if(currentTime - myCache.cacheUnits[i].enteringTime > myCache.cacheUnits[i].maxAge){
                availablePosition = 1;
                // put into cache
                strcpy(myCache.cacheUnits[i].key, key);
                memcpy(myCache.cacheUnits[i].value, value, MaxValueLength);
                myCache.cacheUnits[i].maxAge = maxAge;
                myCache.cacheUnits[i].enteringTime = currentTime;
                myCache.cacheUnits[i].lastUsedTime = currentTime;
                myCache.cacheUnits[i].valueLength = valueLength;
                break;
            }
        }
    }

    // if key doesn't exist and there isn't available position, 
    // find the least used cache unit and replace it/
    if(!keyExist && !availablePosition){
        // find least used cache unit
        for(i = 0; i < myCache.cacheSize; i++){
            if (currentTime - myCache.cacheUnits[i].lastUsedTime > leastUsedTime){
                leastUsedTime = currentTime - myCache.cacheUnits[i].lastUsedTime;
                leastUsedPosition = i;
            }
        }
        // replace least used cache unit with new one
        strcpy(myCache.cacheUnits[leastUsedPosition].key, key);
        memcpy(myCache.cacheUnits[leastUsedPosition].value, value, MaxValueLength);
        myCache.cacheUnits[leastUsedPosition].maxAge = maxAge;
        myCache.cacheUnits[leastUsedPosition].enteringTime = currentTime;
        myCache.cacheUnits[leastUsedPosition].lastUsedTime = currentTime;
        myCache.cacheUnits[i].valueLength = valueLength;
    }

    return myCache;
}

// find the valid value (in valid time) of the key; update the last used time;
// return the value if found, "NA" otherwise.
struct MyCache getFromMyCache(char *key, char value[], int *valueLength, struct MyCache myCache){
    // printf("key:%s, value:%s", key, value);
    strcpy(value, "NA");
    int i;
    long currentTime = time(NULL);
    for(i = 0; i < myCache.cacheSize; i++){
        if(!strcmp(key, myCache.cacheUnits[i].key) &&
            (currentTime - myCache.cacheUnits[i].enteringTime <= myCache.cacheUnits[i].maxAge)){
                // printf("Age: %d\n", (currentTime - myCache.cacheUnits[i].enteringTime));
                memcpy(value, myCache.cacheUnits[i].value, MaxValueLength);
                *valueLength = myCache.cacheUnits[i].valueLength;
                myCache.cacheUnits[i].lastUsedTime = currentTime;
                break;
            }
    }
    return myCache;
}

// age = lastUsedTime - enteringTime
int getAge(char *key, struct MyCache myCache){
    // printf("key:%s, value:%s", key, value);
    int i;
    long currentTime = time(NULL);
    for(i = 0; i < myCache.cacheSize; i++){
        if(!strcmp(key, myCache.cacheUnits[i].key)){
                myCache.cacheUnits[i].lastUsedTime = currentTime;
                return myCache.cacheUnits[i].lastUsedTime - myCache.cacheUnits[i].enteringTime;
            }
    }
    return -1;
}