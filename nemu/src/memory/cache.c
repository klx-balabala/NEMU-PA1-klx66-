#include "common.h"
#include "memory/cache.h"
#include <stdio.h>
#include <stdlib.h>
#include "burst.h"
#include <time.h>

void ddr3_read_replace(hwaddr_t addr, void *data);
void dram_write(hwaddr_t addr, size_t len, uint32_t data);
void ddr3_write_replace(hwaddr_t addr, void *data, uint8_t *mask);

void init_cache(){
    int i;
    for(i = 0; i < Cache_L1_Size / Cache_Block_L1_Size; i ++)
        cache1[i].valid = 0;

    for(i = 0; i < Cache_L2_Size / Cache_Block_L2_Size; i ++){
        cache2[i].valid = 0;
        cache2[i].dirty = 0;
    }
    test_time = 0;
}

int read_cache1(hwaddr_t addr){
    uint32_t group_idx = (addr >> Cache_L1_Block_Bit) & (Cache_L1_Group_Size - 1);
    uint32_t tag = (addr >> (Cache_L1_Group_Bit + Cache_L1_Block_Bit));
    //uint32_t block_start = (addr >> Cache_L1_Block_Bit) << Cache_L1_Block_Bit;
    int i, group = group_idx * Cache_L1_Way_Size;
    for(i = group; i < group + Cache_L1_Way_Size; i ++){
        if (cache1[i].valid == 1 && cache1[i].tag == tag){
            #ifdef Test
                test_time += 2;//HIT in Cache1
            #endif
            return i;
        }
    }

     // Find in Cache2
    int pl = read_cache2(addr);   
    srand(time(0));
    i = group + rand() % Cache_L1_Way_Size;
    memcpy(cache1[i].data, cache2[pl].data, Cache_Block_L1_Size);

    // #ifdef Test
    //     test_time += 200;
    // #endif
    // srand(time(0));
    // i = group + rand() % Cache_L1_Way_Size;
    //     /*new content*/
    // int j;
    // for (j = 0;j < Cache_Block_L1_Size / BURST_LEN;j ++)
    //     ddr3_read_replace(block_start + BURST_LEN * j, cache1[i].data + BURST_LEN * j);
    
    cache1[i].valid = 1;
    cache1[i].tag = tag;
    //printf("%d\n", test_time);
    return i;
}

int read_cache2(hwaddr_t addr){
    uint32_t group_idx = (addr >> Cache_L2_Block_Bit) & (Cache_L2_Group_Size - 1);
    uint32_t tag = (addr >> (Cache_L2_Group_Bit + Cache_L2_Block_Bit));
    uint32_t block_start = (addr >> Cache_L2_Block_Bit) << Cache_L2_Block_Bit;

    int i,group = group_idx * Cache_L2_Way_Size;
    for (i = group; i < group + Cache_L2_Way_Size; i ++){
        if (cache2[i].valid == 1 && cache2[i].tag == tag){  // READ HIT
            #ifdef Test
            test_time += 10;    //HIT in Cache2
            #endif
            return i;
        }
    }
            #ifdef Test
            test_time += 200;//NOT HIT in Cache2
            #endif
    srand(time(0));
    i = group + rand() % Cache_L2_Way_Size;
    /*write back*/
    if (cache2[i].valid == 1 && cache2[i].dirty == 1){
        uint8_t ret[BURST_LEN << 1];
        uint32_t block_st = (cache2[i].tag << (Cache_L2_Group_Bit + Cache_L2_Block_Bit)) | (group_idx << Cache_L2_Block_Bit);
        int w;
        memset(ret,1,sizeof ret);
        for (w = 0;w < Cache_Block_L2_Size / BURST_LEN; w++)
            ddr3_write_replace(block_st + BURST_LEN * w, cache2[i].data + BURST_LEN * w,ret);
        
    }

    int j;
    for (j = 0;j < Cache_Block_L2_Size / BURST_LEN;j ++)
        ddr3_read_replace(block_start + BURST_LEN * j, cache2[i].data + BURST_LEN * j);

    cache2[i].dirty = 0;
    cache2[i].valid = 1;
    cache2[i].tag = tag;
    return i;
}

void write_cache1(hwaddr_t addr, size_t len, uint32_t data){
    uint32_t group_idx = (addr >> Cache_L1_Block_Bit) & (Cache_L1_Group_Size - 1);
    uint32_t tag = (addr >> (Cache_L1_Group_Bit + Cache_L1_Block_Bit));
    uint32_t offset = addr & (Cache_Block_L1_Size - 1);

    int i,group = group_idx * Cache_L1_Way_Size;
    for (i = group;i < group + Cache_L1_Way_Size;i ++){
        if (cache1[i].valid == 1 && cache1[i].tag == tag){// WRITE HIT
            /*write through*/
            if (offset + len > Cache_Block_L1_Size)
            {
                dram_write(addr,Cache_Block_L1_Size - offset,data);
                memcpy(cache1[i].data + offset, &data, Cache_Block_L1_Size - offset);
                write_cache2(addr,Cache_Block_L1_Size - offset,data);
                write_cache1(addr + Cache_Block_L1_Size - offset, len -  (Cache_Block_L1_Size - offset), data >>  (Cache_Block_L1_Size - offset));
            }
            else
            {
                dram_write(addr,len,data);
                memcpy(cache1[i].data + offset, &data, len);
                write_cache2(addr,len,data);
            }
            //printf("%d\n", test_time);
            return;
        }
    }
    //dram_write(addr,len,data);
    write_cache2(addr,len,data);
}

void write_cache2(hwaddr_t addr, size_t len, uint32_t data){
    uint32_t group_idx = (addr >> Cache_L2_Block_Bit) & (Cache_L2_Group_Size - 1);
    uint32_t tag = (addr >> (Cache_L2_Group_Bit + Cache_L2_Block_Bit));
    uint32_t offset = addr & (Cache_Block_L2_Size - 1);

    int i,group = group_idx * Cache_L2_Way_Size;
    for (i = group + 0;i < group + Cache_L2_Way_Size;i ++){
        if (cache2[i].valid == 1 && cache2[i].tag == tag)
        {   // WRITE HIT
            cache2[i].dirty = 1;
            if (offset + len > Cache_Block_L2_Size)
            {
                memcpy(cache2[i].data + offset, &data, Cache_Block_L2_Size - offset);
                write_cache2(addr + Cache_Block_L2_Size - offset,len - (Cache_Block_L2_Size - offset),data >> (Cache_Block_L2_Size - offset));
            }
            else
                memcpy(cache2[i].data + offset, &data, len);
            return;
        }
    }
    /*write allocate*/
    i = read_cache2(addr);
    cache2[i].dirty = 1;
    memcpy(cache2[i].data + offset,&data,len);
}