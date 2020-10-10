////////////////////////////////////////////////////////////////////////////////
//
//  File           : lcloud_cache.c
//  Description    : This is the cache implementation for the LionCloud
//                   assignment for CMPSC311.
//
//   Author        : Patrick McDaniel
//   Last Modified : Thu 19 Mar 2020 09:27:55 AM EDT
//

// Includes 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cmpsc311_log.h>
#include <lcloud_cache.h>


typedef struct {
	int uon;
	uint16_t bl;
	uint16_t sc;
	LcDeviceId dv;
	char *cache;
	int time;
} caches;
double hit = 0;
double miss = 0;
caches *LRU;
uint16_t size;
uint64_t systime;
//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_getcache
// Description  : Search the cache for a block 
//
// Inputs       : did - device number of block to find
//                sec - sector number of block to find
//                blk - block number of block to find
// Outputs      : cache block if found (pointer), NULL if not or failure

char * lcloud_getcache( LcDeviceId did, uint16_t sec, uint16_t blk ) {
	uint64_t *tempbuf;
	tempbuf = (uint64_t *)malloc(sizeof(uint64_t));
	for (int i = 0; i < LC_CACHE_MAXBLOCKS; i++){
		if (LRU[i].uon == 1 && LRU[i].dv == did && LRU[i].sc == sec && LRU[i].bl == blk){
			strcpy(tempbuf, &(LRU[i].cache));
			LRU[i].time = systime;
			systime = systime + 1;
			hit = hit + 1;
			return (void*)tempbuf;
		}
	}
	miss = miss + 1;
    /* Return not found */
    return( NULL );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_putcache
// Description  : Put a value in the cache 
//
// Inputs       : did - device number of block to insert
//                sec - sector number of block to insert
//                blk - block number of block to insert
// Outputs      : 0 if succesfully inserted, -1 if failure

int lcloud_putcache( LcDeviceId did, uint16_t sec, uint16_t blk, char *block ) {
    /* Return successfully */
	int unused = -1;
	int min = -1;
	int index;
	int justused;
	int touse = -1;
	for (int i=0; i<size; i++){
		if (LRU[i].uon == 0){
			//unused = i;
			touse = i;
			//LRU[i].time = systime;
			//min = systime;
		}else if (LRU[i].dv == did && LRU[i].sc == sec && LRU[i].bl == blk){
			touse = i;
			break;
		}else if(min == -1){
			min = LRU[i].time;
			touse = i;
		}else if(min >= LRU[i].time){
			min = LRU[i].time;
			touse = i;
		}
	}
	LRU[touse].time = systime;
	strcpy(&(LRU[touse].cache),block);
	LRU[touse].dv = did;
	LRU[touse].sc = sec;
	LRU[touse].bl = blk;
	systime = systime + 1;
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_initcache
// Description  : Initialze the cache by setting up metadata a cache elements.
//
// Inputs       : maxblocks - the max number number of blocks 
// Outputs      : 0 if successful, -1 if failure

int lcloud_initcache( int maxblocks ) {
	LRU = (caches *)malloc(maxblocks*(sizeof(caches)));
	for (int i=0; i < maxblocks; i++){
		LRU[i].uon = 0;
	}
	size = maxblocks;
	systime = 0;
    /* Return successfully */
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_closecache
// Description  : Clean up the cache when program is closing
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int lcloud_closecache( void ) {
	free(LRU);
	logMessage(LOG_INFO_LEVEL,"%f",hit/(hit+miss));
    /* Return successfully */
    return( 0 );
}
