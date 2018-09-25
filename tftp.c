#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#define BUFSIZE 1024
#define MAX_RETRIES 5

//TODO: TIMOUTS AND SIGNAL HANDLING IMPLEMENTATION, GOTTA WAIT FOR THOSE KIDDOS TO DIE 

//credits to https://codeforwin.org/2018/03/c-program-check-file-or-directory-exists-not.html
/**
 * Function to check whether a file exists or not using
 * stat() function. It returns 1 if file exists at 
 * given path otherwise returns 0. Modified to use lstat instead
 */
int fileExists(const char *path)
{
	struct stat stats;

	lstat(path, &stats);

    // Check for file existence
	if (stats.st_mode & F_OK)
		return 1;

	return 0;
}

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

void ERROR(int sock, int err, ssize_t len, struct sockaddr_in *cli_sock, socklen_t *cli_len){
	message msg; 	

	msg.opcode = 05;
	char* err_msg; 
	if(err == 0){
		err_msg = "Not defined, see error message.\n\0"; 
	}
	else if(err == 1){
		err_msg = "File not found.\n\0"; 
	}
	else if(err == 2){
		err_msg = "Access violation.\n\0"; 
	}
	else if(err == 3){
		err_msg = "Disk full or allocation exceeded.\n\0"; 
	}
	else if(err == 4){
		err_msg = "Illegal FTP Operation.\n\0"; 
	}
	else if(err == 5){
		err_msg = "Unknown transfer ID.\n\0"; 
	}
	else if(err == 6){
		err_msg = "File already exists.\n\0"; 
	}
	else if(err == 7){
		err_msg = "No such user.\n\0";
	}
	strcpy((char*)msg.error.errmsg, err_msg); 
	msg.error.error_code = err;
	msg.opcode = htons(05);

	if(sendto(sock, &msg, strlen(err_msg) + 5, 0, (struct sockaddr *) cli_sock, *cli_len) <0){
		perror("sendto() failed"); 
	}
}
int ACK(int sock, uint16_t block_num, struct sockaddr_in *cli_sock, socklen_t *cli_len){

	//this function returns 1 upon sending, 0 otherwise
	message msg; 
	msg.ack.block_num = block_num; 
	msg.opcode = htons(block_num); 

	if(sendto(sock, &msg, sizeof(&msg) + 1, 0, (struct sockaddr*) cli_sock, *cli_len) < 0){
		perror("sendto failed!\n"); 
		return 0;
	}

	return 1; 
}

int DATA(int sock, ssize_t len, uint16_t block_num,  uint8_t *data_body, struct sockaddr_in *cli_sock, socklen_t *cli_len){
	message msg; 

	memcpy(msg.data.data, data_body, sizeof(&data_body));
	msg.data.block_num = block_num; 
	msg.opcode = htons(03); 
	
	if(sendto(sock, &msg, len, 0, (struct sockaddr *) cli_sock, *cli_len) <0){
		perror("sendto() failed\n"); 
		return 0; 
	}

	return 1;
}

void RRQ(FILE* file, int newsockfd, message *msg, ssize_t len, struct sockaddr_in *cli_sock, socklen_t *cli_len){
	uint16_t block_number = 0;
	int countdown;
	int handle = 1;
	int state;
	uint8_t* byte_stream;

	state = ACK(newsockfd, block_number, cli_sock, cli_len);

	if (state == 0) {
		printf("%s.%u: transfer killed\n",
			inet_ntoa(cli_sock->sin_addr), ntohs(cli_sock->sin_port));
		exit(1);
	}

	for (int i =0; i < block_number; i++){
		ssize_t pkt = recvfrom(newsockfd,msg,sizeof(*msg),0,(struct sockaddr *) cli_sock,cli_len);

		if (pkt >= 0 && pkt < 4) {
			printf("%s.%u: invalid message received\n",
				inet_ntoa(cli_sock->sin_addr), ntohs(cli_sock->sin_port));
			ERROR(newsockfd,0,len,cli_sock,cli_len);
			exit(1);
		}

		if (ntohs(msg->opcode) == 05)  {
			printf("%s.%u: error message: %u %s\n",
				inet_ntoa(cli_sock->sin_addr), ntohs(cli_sock->sin_port),
				ntohs(msg->error.error_code), msg->error.errmsg);
			exit(1);
		}

		if (ntohs(msg->opcode) != 03)  {
			printf("%s.%u: invalid message during transfer\n",
				inet_ntoa(cli_sock->sin_addr), ntohs(cli_sock->sin_port));
			ERROR(newsockfd,0,len,cli_sock,cli_len);
			exit(1);
		}

		if (ntohs(msg->ack.block_num) != block_number) {
			printf("%s.%u: invalid block number\n", 
				inet_ntoa(cli_sock->sin_addr), ntohs(cli_sock->sin_port));
			ERROR(newsockfd,0,len,cli_sock,cli_len);
			exit(1);
		}

		char i;
		int j = 0;
		while((i = fgetc(file)) != EOF){
			if (j >= 512 || i == feof(file)){
				pkt = DATA(newsockfd, len, block_number, byte_stream, cli_sock, cli_len);
				memset(byte_stream, 0, 512);
			} else {		
				byte_stream[j] = i;
				j++;
			}
		}

		if (pkt < 0) {
			perror("server: DATA()");
			exit(1);
		}

		pkt = ACK(newsockfd, block_number, cli_sock, cli_len);

		if (pkt < 0) {
			printf("%s.%u: transfer killed\n",
				inet_ntoa(cli_sock->sin_addr), ntohs(cli_sock->sin_port));
			exit(1);
		}
		block_number++; 
	}

}


void WRQ(FILE* file, int newsockfd, message *msg, ssize_t len, struct sockaddr_in *cli_sock, socklen_t *cli_len){
//file is the bytestream of the file 
	//starting our variables 
	uint16_t block_number; block_number = 0; 
	int countdown; 
	int handle; handle = 1; 
	int retries; retries = 0; 
	int i; 
	int state; 

	//first, send an ACK that we got the WRQ and are going to start writing 
	state = ACK(newsockfd, block_number, cli_sock, cli_len); 
	if(state == 0){
		ERROR(newsockfd, 0, len, cli_sock, cli_len); 
		printf("%s.%u: transfer killed\n",
			inet_ntoa(cli_sock->sin_addr), ntohs(cli_sock->sin_port));
		exit(1);
	}
}


void chld_handler(int newsockfd, message *msg, ssize_t len, struct sockaddr_in *cli_sock, socklen_t *cli_len){
	char* filename;
	strcpy(filename,(char *)msg->request.filename_mode);
	char* end;
	end = &filename[len - 2 - 1];

	if (*end != '\0') {
		printf("%s.%u: invalid filename\n",
			inet_ntoa(cli_sock->sin_addr), ntohs(cli_sock->sin_port));
		ERROR(newsockfd,0,len,cli_sock,cli_len);
		exit(1);
    } //check filename

     //Use lstat to implement this
    //TA in office hours said we only need to care about filenames, not whole directories
    if(!fileExists(filename)) {
    	printf("%s.%u: filename outside base directory\n",
    		inet_ntoa(cli_sock->sin_addr), ntohs(cli_sock->sin_port));
    	ERROR(newsockfd,0,len,cli_sock,cli_len);
    	exit(1);

     } // file not in directory 

     printf("%s.%u: request ready: %s '%s' %s\n", 
     	inet_ntoa(cli_sock->sin_addr), ntohs(cli_sock->sin_port),
     	ntohs(msg->opcode) == 01 ? "get" : "put", filename, "octet");

     FILE* file = fopen(filename,"r+");

     if (msg->opcode == 01){
     	RRQ(file, newsockfd, msg, len, cli_sock, cli_len); 
     } else if(msg->opcode == 02){
     	WRQ(file, newsockfd, msg, len, cli_sock, cli_len); 
     }
 }

 int main(int argc, char const *argv[]){

 	int sockfd, newsockfd;
 	struct sockaddr_in server_sock;
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
 	printf("Port Number: %d\n",(int) ntohs(server_sock.sin_port));


 	while (1) {
 		struct sockaddr_in client_sock;
 		socklen_t cli_len = sizeof(client_sock);
 		ssize_t len;

 		message client;
 		uint16_t opcode; 

 		if ((len = recvfrom(sockfd, &client, sizeof(client), 0, (struct sockaddr *) &client_sock, &cli_len)) < 0) {
 			perror("Connection failed.");
 		}

 		opcode = ntohs(client.opcode); 
 		printf("Opcode is: %d\n", opcode);

 		if(opcode == 01 || opcode == 02){
 			if(fork() == 0){
 				if ( (newsockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
 					perror("socket creation failed"); 
 					close(newsockfd); 
 				} 

 				memset(&server_sock, 0, sizeof server_sock);
 				server_sock.sin_family = AF_INET;
 				server_sock.sin_addr.s_addr = INADDR_ANY;
 				server_sock.sin_port = 0;

 				if ((bind(newsockfd, (struct sockaddr *) &client_sock, sizeof(client_sock)) == -1)){ 
 					close(newsockfd);
 					perror("Error on binding");
 				}

 				printf("Connection successful\n");
 				chld_handler(newsockfd, &client, len, &client_sock, &cli_len);

 			}

        	else { //parent

        		continue;

        	}

        } else {

        	perror("Error in the code!\n");

        }

    }

    return 0;
}