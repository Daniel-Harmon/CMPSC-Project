#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "net.h"
#include "jbod.h"



/* the client socket descriptor for the connection to the server */
int cli_sd = -1;

void *packet_header;
void *packet;

uint8_t* packet_construct(int op, uint8_t *block){
	
	uint16_t length;
	uint16_t return_code = 0;
	
	uint32_t server_op = htonl(op);
	uint16_t server_length;
	uint16_t server_return_code = htons(return_code);	
		
	uint8_t cmd = (uint8_t) ((op >> 26) & 0xFF);

	if(cmd == JBOD_WRITE_BLOCK){
		length = 264;
		server_length = htons(length);

		*(uint16_t*)(packet) = server_length;
		
		*(uint32_t*)(packet + 2) = server_op;
		
		*(uint16_t*)(packet + 6) = server_return_code;
		
		for(int b = 0; b < 256; b++){
			*(uint8_t*)(packet + 8 + b) = block[b];
		}		
		
		return packet;
	}else{
		length = 8;
		server_length = htons(length);
		
		*(uint16_t*)(packet_header) = server_length;
		
		*(uint32_t*)(packet_header + 2) = server_op;
		
		*(uint16_t*)(packet_header + 6) = server_return_code;
		
		return packet_header;
	}	
}



/* attempts to read n bytes from fd; returns true on success and false on
 * failure */
static bool nread(int fd, int len, uint8_t *buf) {
  
  int num_read = 0;

  while(num_read < len){
	int success = read(fd, (buf + num_read), (len - num_read));
  
  	if(success == -1){
  		//printf("Error reading socket");
  		return false;
  	}else if(success == 0){
  		//printf("Nothing sent to socket");
  		return false;
  	}
  	
  	num_read = num_read + success;
  }
  
  return true;
  
}

/* attempts to write n bytes to fd; returns true on success and false on
 * failure */
static bool nwrite(int fd, int len, uint8_t *buf) {
	
  int num_write = 0;
  
  while(num_write < len){	
  	int success = write(fd, (buf + num_write), (len - num_write));
  
  	if(success == -1){
  		//printf("Error writing to socket");
  		return false;
  	}else if(success == 0){
  		//printf("Nothing written to socket");
  		return false;
  	}
  	
  	num_write = num_write + success;
  }
  return true;
}

/* attempts to receive a packet from fd; returns true on success and false on
 * failure */
 
static bool recv_packet(int fd, uint32_t op, uint8_t *ret, uint8_t *block) {
	bool packet_read = false;
	uint8_t cmd = (uint8_t) ((op >> 26) & 0xFF);
	
	if(cmd == JBOD_READ_BLOCK || cmd == JBOD_SIGN_BLOCK){
		packet_read = nread(fd, 264, ret);
		if(packet_read){
			for(int a = 0; a < 256; a++){
				block[a] = ret[8 + a];
			}
		}
	}else{
		packet_read = nread(fd, 8, ret);
	}
	
	return packet_read;
}

/* attempts to send a packet to sd; returns true on success and false on
 * failure */
static bool send_packet(int sd, uint32_t op, uint8_t *block) {
	
	uint8_t *pkt = packet_construct(op, block);
	size_t size;
	
	if(pkt[0] == 0){
		size = 8;
	}else{
		size = 264;
	}
	
	return nwrite(sd, size, pkt);
}

/* attempts to connect to server and set the global cli_sd variable to the
 * socket; returns true if successful and false if not. */
bool jbod_connect(const char *ip, uint16_t port) {	
	
	packet = calloc(264, 1);
	packet_header = calloc(8, 1);		
		
		//create a socket
			int sockfd;
			sockfd = socket(AF_INET, SOCK_STREAM, 0);
			if(sockfd == -1){
				//printf("Error on socket creation");
				return false;
			}
		//convert ip to binary
			struct sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_port = htons(JBOD_PORT);
			if(inet_aton(JBOD_SERVER, &addr.sin_addr) == 0){
				//printf("Error with server ip format");
				return false;
			}
		//call connect function
			int connection = connect(sockfd, (const struct sockaddr *) &addr, sizeof(addr));
			if(connection == -1){
				//printf("Error connecting to server");
				return false;
			}
			
			cli_sd = sockfd;
			return true;
}

/* disconnects from the server and resets cli_sd */
void jbod_disconnect(void) {
	
	free(packet);
	free(packet_header);
	packet = NULL;
	packet_header = NULL;

		//int connection = 
	close(cli_sd);
		//if(connection == -1){
			//printf("Error disconnecting from server");
		//}
		cli_sd = -1;
}

/* sends the JBOD operation to the server and receives and processes the
 * response. */

//***** ALSO NOT SURE IF THE WHILE LOOPS ARE GOOD *****
int jbod_client_operation(uint32_t op, uint8_t *block) {
	
	//bool packet_sent = false;
	//bool packet_recv = false;
	uint8_t cmd = (uint8_t) ((op >> 26) & 0xFF);

	//while(!packet_sent){
		//packet_sent = 
		send_packet(cli_sd, op, block);
	//}
	
	//while(!packet_recv){
		if(cmd == JBOD_READ_BLOCK || cmd == JBOD_SIGN_BLOCK){
			//packet_recv = 
			recv_packet(cli_sd, op, packet, block);
		}else{
			//packet_recv = 
			recv_packet(cli_sd, op, packet_header, block);
		}
	//}
	
	return 1;
}
