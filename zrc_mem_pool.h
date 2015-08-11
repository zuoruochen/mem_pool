//
//  zrc_mem_pool.h
//  mem_pool
//
//  Created by ruochenzuo on 8/10/15.
//  Copyright (c) 2015 ruochenzuo. All rights reserved.
//

#ifndef _ZRC_MEM_POOL_
#define _ZRC_MEM_POOL_

#include <sys/types.h>

typedef unsigned char u_char;

typedef struct zrc_mem_pool_s zrc_mem_pool_t;
typedef struct zrc_mem_data_s zrc_mem_data_t;
typedef struct zrc_mem_large_s  zrc_mem_large_t;
typedef void (*zrc_mem_cleanup_handle)(void *);
typedef struct zrc_mem_cleanup_s zrc_mem_cleanup_t;


#define ALIGNMENT 16
#define MAX_POOL_SIZE 2048
#define zrc_align(x) (u_char *)(((size_t)(x) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

struct zrc_mem_data_s {
    u_char *last;
    u_char *end;
    zrc_mem_pool_t *next;
    size_t fail;

};

struct zrc_mem_large_s {
    void *data;
    zrc_mem_large_t *next;
};

struct zrc_mem_cleanup_s {
    zrc_mem_cleanup_handle handle;
    void *data;
    zrc_mem_cleanup_t *next;
};

struct zrc_mem_pool_s {
    zrc_mem_data_t data;
    size_t max;
    zrc_mem_pool_t *current;
    zrc_mem_large_t *large;
    zrc_mem_cleanup_t *cleanup;
    int fd;
};

zrc_mem_pool_t *zrc_mem_pool_create(size_t size, int fd);
void zrc_mem_pool_destory(zrc_mem_pool_t *pool);
void zrc_mem_pool_reset(zrc_mem_pool_t *pool);

void *zrc_mem_palloc(zrc_mem_pool_t *pool, size_t size);
int zrc_mem_pfree(zrc_mem_pool_t *pool, void *p);
int zrc_add_cleanup_handle(zrc_mem_pool_t *pool,zrc_mem_cleanup_handle handle, void *data);
int zrc_remove_cleanup_handle(zrc_mem_pool_t *pool,zrc_mem_cleanup_handle handle);
#endif
