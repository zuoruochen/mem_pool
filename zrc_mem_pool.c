//
//  zrc_mem_pool.c
//  mem_pool
//
//  Created by ruochenzuo on 8/10/15.
//  Copyright (c) 2015 ruochenzuo. All rights reserved.
//

#include "zrc_mem_pool.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

void *zrc_alloc(size_t size, int fd){
    void *ret;
    ret= malloc(size);
    if (ret == NULL) {
        char buf[1024];
        sprintf(buf,"%s %s\n",__func__,"malloc failed");
        write(fd,buf,strlen(buf)+1);
    }
    return ret;
}



void *zrc_calloc(size_t size,int fd){
    void *ret;
    ret = zrc_alloc(size, fd);
    if (ret != NULL) {
        memset(ret, 0, size);
    }
    return ret;
}

void *zrc_memalign(size_t size, size_t alignment, int fd){
    void *p;
    int ret;
    ret = posix_memalign(&p, alignment, size);
    if(ret ){
        char buf[1024];
        sprintf(buf,"%s %s\n",__func__,"memalign failed");
        write(fd,buf,strlen(buf)+1);
        p = NULL;
    }
    return p;
}


zrc_mem_pool_t *zrc_mem_pool_create(size_t size, int fd){
    zrc_mem_pool_t *p;
    p = zrc_memalign(size, ALIGNMENT, fd);
    if(p != NULL){
        p->data.last = (u_char *)p + sizeof(zrc_mem_pool_t);
        p->data.end = (u_char *)p + size;
        p->data.next = NULL;
        p->data.fail = 0;
        
        p->max = (size < MAX_POOL_SIZE )? size : MAX_POOL_SIZE;
        p->current = p;
        p->large = NULL;
        p->cleanup = NULL;
        p->fd = fd;
    }
    return p;
}

void *zrc_mem_block(zrc_mem_pool_t *pool,size_t size){
    void *p;
    zrc_mem_pool_t *new,*m;
    write(pool->fd,"call zrc_mem_block\n",sizeof("call zrc_mem_block\n"));
    new = zrc_memalign(pool->data.end - (u_char *)pool, ALIGNMENT, pool->fd);
    if (new == NULL) {
        return NULL;
    }
    
    for(m = pool->current; m->data.next != NULL; m = m->data.next){
        if (m->data.fail++ > 5) {
            pool->current =m->data.next;
        }
    }
    m->data.next = new;
    new->data.last = (u_char *)new + sizeof(zrc_mem_data_t);
    
    new->data.last = zrc_align(new->data.last);
    p = new->data.last;
    new->data.last = p + size;
    
    new->data.end = (u_char *)new + (pool->data.end - (u_char *)pool);
    new->data.next = NULL;
    new->data.fail = 0;
    pool->current = pool->current ? pool->current : new;
    return p;
}

void *zrc_mem_large(zrc_mem_pool_t *pool, size_t size){
    void *p;
    write(pool->fd,"alloc from large\n",sizeof("alloc from large\n"));
    p = zrc_alloc(size, pool->fd);
    if(p == NULL)
        return NULL;
    int n= 0;
    zrc_mem_large_t *large;
    for (large = pool->large; large != NULL; large = large->next) {
        if(large->data == NULL){
            large->data = p;
            return p;
        }
        if(n++ > 3){
            break;
        }
    }
    large = zrc_mem_palloc(pool, sizeof(zrc_mem_large_t));
    if (large == NULL) {
        return NULL;
    }
    large->data = p;
    large->next = pool->large;
    pool->large = large;
    return p;
}


void *zrc_mem_palloc(zrc_mem_pool_t *pool, size_t size){
    zrc_mem_pool_t *p;
    if (size <= pool->max ) {
        u_char *m;
        for(p = pool->current; p != NULL; p = p->data.next){
            m = zrc_align(p->data.last);
            if (p->data.end - p->data.last >= size) {
                p->data.last = m + size;
                write(pool->fd,"alloc from current block\n",sizeof("alloc from current block\n"));
                return m;
            }
        }
        return zrc_mem_block(pool, size);
    }
    return zrc_mem_large(pool,size);
}

int zrc_mem_pfree(zrc_mem_pool_t *pool, void *p){
    zrc_mem_large_t *l;
    for (l = pool->large; l != NULL; l = l->next) {
        if(l->data == p){
            free(p);
            l->data = NULL;
            return 1;
        }
    }
    return 0;
}

//todo : change method
int zrc_add_cleanup_handle(zrc_mem_pool_t *pool,zrc_mem_cleanup_handle handle, void *data){
    zrc_mem_cleanup_t *p;

    p = zrc_mem_palloc(pool, sizeof(zrc_mem_cleanup_t));
    if (p == NULL) {
        return -1;
    }
    p->handle = handle;
    p->data = data;
    p->next = pool->cleanup;
    pool->cleanup = p;
    
    return 1;
}


int zrc_remove_cleanup_handle(zrc_mem_pool_t *pool,zrc_mem_cleanup_handle handle){
    zrc_mem_cleanup_t * cleanup,*pre;
    if(pool->cleanup->handle == handle){
        pool->cleanup = pool->cleanup->next;
        return 1;
    }
    for(pre = pool->cleanup,cleanup = pre->next; cleanup != NULL ; pre = pre->next,cleanup = cleanup->next){
        if (cleanup->handle == handle) {
            pre->next = cleanup->next;
            return 1;
        }
    }
    return 0;
}



void zrc_mem_pool_reset(zrc_mem_pool_t *pool){
    zrc_mem_large_t *large;
    zrc_mem_pool_t *p;
    for(large = pool->large; large != NULL ;large = large->next){
        if(large->data != NULL){
            free(large->data);
        }
    }
    pool->data.last = (u_char *)pool + sizeof(zrc_mem_pool_t);
    for(p = pool->data.next; p != NULL ; p = p->data.next){
        p->data.last = (u_char *)p + sizeof(zrc_mem_data_t);
    }
    
}


void zrc_mem_pool_destory(zrc_mem_pool_t *pool){
    zrc_mem_large_t *large;
    zrc_mem_cleanup_t *cleanup;
    for(cleanup = pool->cleanup; cleanup != NULL; cleanup = cleanup->next){
        if (cleanup->handle) {
            cleanup->handle(cleanup->data);
        }
    }
    for(large = pool->large; large != NULL ;large = large->next){
        if(large->data != NULL){
            free(large->data);
        }
    }
    zrc_mem_pool_t *p,*tmp;
    for (p = pool; p != NULL;) {
        tmp = p;
        p = p->data.next;
        free(tmp);
    }
}














