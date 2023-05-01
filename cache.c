#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cache.h"

static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int cache_entries = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;
static bool cache_running = false;

//Dont know what the valid cache entry thing is supposed to mean
//maybe want to add return values to lookup, update, and insert so that once they find their blocks they dont keep iterating
//still not 100% sure if my implementation of cache_enabled is right

int cache_check(int disk_num, int block_num) {
	if(!cache_running || cache_entries == 0){
		return -1;
	}else{
		for(int a = 0; a < cache_entries; a++){
			if(cache[a].disk_num == disk_num && cache[a].block_num == block_num){
				return 1;
			}
		}
	}
	return -1;
}

int cache_create(int num_entries){
	if(cache_running || num_entries < 2 || num_entries > 4096){
		return -1;
	}
	
	else{
		cache = malloc(num_entries * sizeof(cache_entry_t));
  		cache_size = num_entries;
  		cache_running = true;
  		return 1;	
	}
}

int cache_destroy(void) {
  	if(cache_running){
  		free(cache);
  		cache = NULL;
  		cache_size = 0;
  		cache_entries = 0;
  		cache_running = false;
  		return 1;
  	}else{
  		return -1;
  	}
}

int cache_lookup(int disk_num, int block_num, uint8_t *buf) {
	if(!cache_running || cache_entries == 0 || buf == NULL){
		return -1;
	}else{
		num_queries = num_queries + 1;
		clock = clock + 1;
	
		for(int a = 0; a < cache_entries; a++){
			if(cache[a].disk_num == disk_num && cache[a].block_num == block_num){
				num_hits = num_hits + 1;
				cache[a].access_time = clock;
			
				for(int b = 0; b < JBOD_BLOCK_SIZE; b++){
					buf[b] = cache[a].block[b];
				}
				
				return 1;
			}
		}
	}
	return -1;
}

void cache_update(int disk_num, int block_num, const uint8_t *buf) {
	clock = clock + 1;
	for(int a = 0; a < cache_entries; a++){
		if(cache[a].disk_num == disk_num && cache[a].block_num == block_num){
			cache[a].access_time = clock;
			for(int b = 0; b < JBOD_BLOCK_SIZE; b++){
				cache[a].block[b] = buf[b];
			}
		}
	}
}

int cache_insert(int disk_num, int block_num, const uint8_t *buf) {
	if(!cache_running || buf == NULL || disk_num < 0 || disk_num > 15 || block_num < 0 || block_num > 255 || cache_check(disk_num, block_num) == 1){
		return -1;
	}else{
		int LRU = 0;
		clock = clock + 1;
		if(cache_entries < cache_size){
			cache[cache_entries].disk_num = disk_num;
			cache[cache_entries].block_num = block_num;
			for(int a = 0; a < JBOD_BLOCK_SIZE; a++){
				cache[cache_entries].block[a] = buf[a];
			}	
			cache[cache_entries].access_time = clock;
			cache_entries = cache_entries + 1;
		}else{
			for(int a = 1; a < cache_entries; a++){
				if(cache[a].access_time < cache[LRU].access_time){
					LRU = a;
				}
			}
			
			cache[LRU].disk_num = disk_num;
			cache[LRU].block_num = block_num;
			for(int a = 0; a < JBOD_BLOCK_SIZE; a++){
				cache[LRU].block[a] = buf[a];
			}	
			cache[LRU].access_time = clock;
		}
		
		return 1;
	}
}

bool cache_enabled(void) {
  return cache_running;
}

void cache_print_hit_rate(void) {
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);
}
