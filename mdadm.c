#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "mdadm.h"
#include "jbod.h"
#include "net.h"

bool mounted = false;


int command_construct(int cmd, int diskID, int blockID){
	uint32_t command = 0;
	command = command + cmd;
	command = command << 4;
	command = command + diskID;
	command =  command << 14;
	command = command << 8;
	command = command + blockID;
	return command;
}

int mdadm_mount(void) {
	uint32_t cmd;
	if (!mounted){
		cmd = command_construct(JBOD_MOUNT, 0, 0);
		jbod_client_operation(cmd, NULL);
		mounted = true;
		return 1;
	}
	else{
		return -1;
	}
}

int mdadm_unmount(void) {
	uint32_t cmd;
	if (mounted){
		cmd = command_construct(JBOD_UNMOUNT, 0, 0);
		jbod_client_operation(cmd, NULL);
		mounted = false;
		return 1;
	}
	else{
		return -1;
	}
}

int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf) {
  int start_disk;
  int start_block;
  int byte_by_block;
  uint32_t cmd;
  uint8_t read_block[256];
  uint8_t cache_block[256];
  int i;
  int a = 0;
  int bytes_to_block_end;
  int true_len = len;
  
  
  if (!mounted){
  	return -1;
  }

  if (len > 1024){
  	return -1;
  }

  if (addr < 0){
  	return -1;
  }

  if (buf == NULL && len != 0){
  	return -1;
  }
  
  if ((addr + len) >  1048576){
  	return -1;
  }
  
  start_disk = addr / 65536;
  start_block = (addr % 65536) / 256;
  byte_by_block = addr % 256;
  
  cmd = command_construct(JBOD_SEEK_TO_DISK, start_disk, 0);
  jbod_client_operation(cmd, NULL);

  cmd = command_construct(JBOD_SEEK_TO_BLOCK, 0, start_block);
  jbod_client_operation(cmd, NULL);
 
  while(len < 1025){
  	
  	bytes_to_block_end = (256 - byte_by_block);
  	
  	if(cache_lookup(start_disk, start_block, read_block) == -1){
  		cmd = command_construct(JBOD_READ_BLOCK, 0, 0);
  		jbod_client_operation(cmd, read_block);
  		
  		cmd = command_construct(JBOD_SEEK_TO_BLOCK, 0, start_block);
  		jbod_client_operation(cmd, NULL);
  		cmd = command_construct(JBOD_READ_BLOCK, 0, 0);
  		jbod_client_operation(cmd, cache_block);
  		
  		cache_insert(start_disk, start_block, cache_block);
  	}
  	
  	start_block = start_block + 1;
  	
  	
  	if(len > bytes_to_block_end){
  		for(i = 0; i < bytes_to_block_end; i++){
  			buf[a] = read_block[byte_by_block + i];
  			a = a + 1;
  		}
  	}
  	else{
  		for(i = 0; i < len; i++){
  			buf[a] = read_block[byte_by_block + i];
  			a = a + 1;
  		}
  	}
  	
  	len = (len - bytes_to_block_end);
  	byte_by_block = 0;
  	
  	if (start_block == 256){
  		
  		start_disk = start_disk + 1;
  		cmd = command_construct(JBOD_SEEK_TO_DISK, start_disk, 0);
  		jbod_client_operation(cmd, NULL);
  		
  		start_block = 0;
  		cmd = command_construct(JBOD_SEEK_TO_BLOCK, 0, start_block);
  		jbod_client_operation(cmd, NULL);
  
  	}
  }
  
  return true_len;
}


int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) {
  int start_disk;
  int start_block;
  int byte_by_block;
  uint32_t cmd;
  uint8_t write_block[256];
  int i;
  int a = 0;
  int bytes_to_block_end;
  int true_len = len;

  if (!mounted){
  	return -1;
  }

  if (len > 1024){
  	return -1;
  }

  if (addr < 0){
  	return -1;
  }

  if (buf == NULL && len != 0){
  	return -1;
  }
  
  if ((addr + len) >  1048576){
  	return -1;
  }
  
  start_disk = addr / 65536;
  start_block = (addr % 65536) / 256;
  byte_by_block = addr % 256;
  
  cmd = command_construct(JBOD_SEEK_TO_DISK, start_disk, 0);
  jbod_client_operation(cmd, NULL);

  cmd = command_construct(JBOD_SEEK_TO_BLOCK, 0, start_block);
  jbod_client_operation(cmd, NULL);
  
 
  while(len < 1025){
  	
  	bytes_to_block_end = (256 - byte_by_block);
	
	if(cache_lookup(start_disk, start_block, write_block) == -1){
  		cmd = command_construct(JBOD_READ_BLOCK, 0, 0);
  		jbod_client_operation(cmd, write_block);
  	
  		cmd = command_construct(JBOD_SEEK_TO_BLOCK, 0, start_block);
  		jbod_client_operation(cmd, NULL);
	}
  	
  	if(len > bytes_to_block_end){
  		for(i = 0; i < bytes_to_block_end; i++){
  			write_block[byte_by_block + i] = buf[a];
  			a = a + 1;
  		}
  	}
  	else{
  		for(i = 0; i < len; i++){
  			write_block[byte_by_block + i] = buf[a];
  			a = a + 1;
  		}
  	}
  	
  	len = (len - bytes_to_block_end);
  	byte_by_block = 0;
  	
  	if(cache_insert(start_disk, start_block, write_block) == -1){
  		cache_update(start_disk, start_block, write_block);
  	}
  	
  	cmd = command_construct(JBOD_WRITE_BLOCK, 0, 0);
  	jbod_client_operation(cmd, write_block);
  	start_block = start_block + 1;
  	
  	if (start_block == 256){
  		
  		start_disk = start_disk + 1;
  		cmd = command_construct(JBOD_SEEK_TO_DISK, start_disk, 0);
  		jbod_client_operation(cmd, NULL);
  		
  		start_block = 0;
  		cmd = command_construct(JBOD_SEEK_TO_BLOCK, 0, start_block);
  		jbod_client_operation(cmd, NULL);
  
  	}
  }
  
  return true_len;
 }
