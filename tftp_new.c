
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
#include <fcntl.h>

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

<<<<<<< HEAD
void RRQ(int childfd, struct sockaddr_in *child_sock, char *buf, unsigned int buf_size){
	int fd;
	char childBuf[516];
	unsigned short int *opcode_ptr = (unsigned short int*) childBuf;
    //Get file
	char* filename;
	strcpy(filename,buf+2);
	ssize_t len;
=======
void sig_child(int signo){
	pid_t pid; 
	int stat; 

	while( (pid = waitpid(-1, &stat, WHOHANG)) > 0){
		printf("child %d terminated\n", pid); 
	}

	return;
}

/*typedef union {

	char* opcode[];

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
	
>>>>>>> 2375c1f05f34aa2b7fda3b886e5e1e17f4619281

	//open file
    if((fd = open(filename, O_RDONLY)) < 0){
        //file not found, add error to send buffer
        *opcode_ptr = htons(5);
        *(opcode_ptr + 1) = htons(1);
        *(childBuf + 4) = 0;
        printf("FILE NOT FOUND\n");
    file_send:
        len = sendto(childfd, childBuf, 5, 0, (struct sockaddr *) &child_sock, sizeof(child_sock));
        //error sending
        if(len < 0){
            if(errno == EINTR){
            	goto file_send;
            }
            perror("sendto error");
            exit(-1);
        }
        exit(-1);
    }

	}     
}

void WRQ(int child_sock, struct sockaddr_in *server_sock, char *buf, unsigned int buf_size){

	//Get file
	char* filename;
	strcpy(filename,buf+2);

}

void child_handler(unsigned short int opcode, struct sockaddr_in *server_sock, char *buf, unsigned int buf_size){
	int childfd;
	socklen_t child_len;
	struct sockaddr_in child_sock;
	ssize_t len;

	child_len = sizeof(child_sock);

	// Creating socket file descriptor 
	if ( (childfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
		perror("child creation failed"); 
		exit(EXIT_FAILURE); 
	} 

	memset(&child_sock, 0, sizeof child_sock);
	child_sock.sin_family = AF_INET;
	child_sock.sin_addr.s_addr = htonl(INADDR_ANY);
	child_sock.sin_port = htons(0);


	if ((childfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket error");
		exit(-1);
	}

	if (bind(childfd, (struct sockaddr*) &child_sock, child_len) < 0) {
		perror("failed to bind");
		exit(-1);
	}

	printf("In child\n");

	if (opcode == 1){
		RRQ(childfd, &child_sock, buf, buf_size);
	}

	if (opcode == 2){
		WRQ(childfd, &child_sock, buf, buf_size);
	}
}

int main(int argc, char const *argv[]){
	int sockfd;
	socklen_t server_len;
	struct sockaddr_in server_sock;

	char buf[BUFSIZE]; /* message buf */
	unsigned short int opcode;
	ssize_t len;

	server_len = sizeof(server_sock);

	// Creating socket file descriptor 
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		exit(EXIT_FAILURE); 
	} 

	memset(&server_sock, 0, server_len);
	server_sock.sin_family = AF_INET;
	server_sock.sin_addr.s_addr = INADDR_ANY;
	server_sock.sin_port = htons(6500);


	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket error");
		exit(-1);
	}

	if (bind(sockfd, (struct sockaddr*) &server_sock, server_len) < 0) {
		perror("failed to bind");
		exit(-1);
	}

	getsockname(sockfd, (struct sockaddr *) &server_sock, &server_len);
	printf("%d\n", ntohs(server_sock.sin_port));

	signal(SIGCHLD, sig_child); 
	while (1) {
		receive:
		len = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &server_sock, &server_len);
		printf("Len: %s\n", buf);
		if (len < 0) {
			if (errno == EINTR) {
				goto receive;
			}
			perror("recvfrom");
			exit(-1);
		}

 		/* check the opcode */
		unsigned short int * opcode_ptr = (unsigned short int *) buf;
		opcode = ntohs(*opcode_ptr);
		char op_char = opcode;
		char str[10];
		sprintf(str, "%c", opcode);
		printf("Opcode: %d\n", atoi(str));
		opcode = atoi(str);
		if(opcode == 1 || opcode == 2){
			if(fork() == 0){
				close(sockfd);
				break;

			}

        	else { //parent

        	}
        }
    }
    child_handler(opcode,&server_sock, buf, BUFSIZE);

    return 0;
}