#include "unpv13e/lib/unp.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFSIZE 1024

typedef union {

	uint16_t opcode;

	struct{
		uint16_t opcode;
		uint8_t filename_mode[512];
	} request;

	struct{
		uint16_t opcode;
		uint16_t block_num;
		uint8_t data;
	} data;

	struct{
		uint16_t opcode;
		uint16_t block_num;
	} ack;

	struct{
		uint16_t opcode;
		uint16_t error_code;
		uint8_t errmsg[512];
	} error;

} message;

void RRQ(message *m, ssize_t len, struct sockaddr_in *cli_sock, socklen_t *cli_len){
	 
}

void WRQ(message *m, ssize_t len, struct sockaddr_in *cli_sock, socklen_t *cli_len){
	
}

void DATA(message *m, ssize_t len, struct sockaddr_in *cli_sock, socklen_t *cli_len){
	
}

void ACK(message *m, ssize_t len, struct sockaddr_in *cli_sock, socklen_t *cli_len){
	
}

void ERROR(message *m, ssize_t len, struct sockaddr_in *cli_sock, socklen_t *cli_len){
	
}

int main(int argc, char const *argv[]){
	
	int sockfd, n;
	struct sockaddr_in server_sock, newsockfd;
	char buf[BUFSIZE]; /* message buf */

	// Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 

	memset(&server_sock, 0, sizeof server_sock);
	server_sock.sin_family = AF_INET;
	server_sock.sin_addr.s_addr = INADDR_ANY;
	server_sock.sin_port = 0;
	if ((bind(sockfd, (struct sockaddr *) &server_sock, sizeof(server_sock)) == -1)){ 
		close(sockfd);
    	perror("Error on binding");
	}

	unsigned int server_len = sizeof(server_sock);
	int sock_name = getsockname(sockfd, (struct sockaddr *) &server_sock, &server_len);
	printf("Port Number: %d\n", sock_name);


    while (1) {
    	struct sockaddr_in client_sock;
        socklen_t cli_len = sizeof(client_sock);
        ssize_t len;

        message client;
        uint16_t opcode; 

        if ((len = recvfrom(sockfd, &client, sizeof(&client), 0, (struct sockaddr *) &client_sock, &cli_len)) < 0) {
               perror("Connection failed.");
        }

        opcode = ntohs(client.opcode); 

        if(client.opcode == 01 || client.opcode == 02){
        	if(fork() == 0){
        		if (client.opcode == 01){
        			RRQ(&client, len, &client_sock, &cli_len); 
        		}
        		else if(client.opcode == 02){
        			WRQ(&client, len, &client_sock, &cli_len); 
        		}
        	}
        	else{ //parent
        		continue;
        	}
        }
        else{
        	perror("Error in the code!\n");
        }

	}

	return 0;
}