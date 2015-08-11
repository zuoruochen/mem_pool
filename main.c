//
//  main.c
//  mem_pool
//
//  Created by ruochenzuo on 8/10/15.
//  Copyright (c) 2015 ruochenzuo. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include "zrc_mem_pool.h"


void z1(void *data){
    printf("%s\n",data);
}

void z2(void *data){
    printf("%s\n",data);
}

void z3(void *data){
    printf("%s\n",data);
}


int main(int argc, const char * argv[]) {
    // insert code here...
    
    int *a,*b;
    char *c;
    char data1[] = "cleanup handle test1!";
    char data2[] = "cleanup handle test2!";
    char data3[] = "cleanup handle test3!";
    zrc_mem_pool_t *pool;
    pool = zrc_mem_pool_create(1024, 2);
    a = (int *)zrc_mem_palloc(pool, 128);
    b = (int *)zrc_mem_palloc(pool, 128);
    a[32] = 999;
    b[0] = 123;
    printf("%d\n",a[32]);
    c = zrc_mem_palloc(pool, 1025);
    zrc_add_cleanup_handle(pool, z1, data1);
    zrc_add_cleanup_handle(pool, z2, data2);
    zrc_add_cleanup_handle(pool, z3, data3);
    zrc_remove_cleanup_handle(pool,z2);
    zrc_mem_pool_destory(pool);
    printf("Hello, World!\n");
    return 0;
}
