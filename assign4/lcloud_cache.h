#ifndef LCLOUD_CACHE_INCLUDED
#define LCLOUD_CACHE_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : lcloud_cache.c
//  Description    : This is the cache API for the LionCloud
//                   assignment for CMPSC311.
//
//   Author        : Patrick McDaniel
//   Last Modified : Thu 19 Mar 2020 09:27:55 AM EDT
//

// Includes 
#include <stdint.h>
#include <lcloud_controller.h>

// Defines 
#define LC_CACHE_MAXBLOCKS 64

//
// Functional Prototypes

char * lcloud_getcache( LcDeviceId did, uint16_t sec, uint16_t blk );
    // Search the cache for a block 

int lcloud_putcache( LcDeviceId did, uint16_t sec, uint16_t blk, char *block );
    // Put a value in the cache 

int lcloud_initcache( int maxblocks );
    // Initialze the cache by setting up metadata a cache elements.

int lcloud_closecache( void );
    // Clean up the cache when program is closing.

#endif
