
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
#include <signal.h>

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

void sig_child(int signo){
	pid_t pid; 
	int stat; 

	while( (pid = waitpid(-1, &stat, WNOHANG)) > 0){
		printf("child %d terminated\n", pid); 
	}

	return;
}

void RRQ(int childfd, struct sockaddr_in *child_sock, char buf[], unsigned int buf_size, struct sockaddr_in *server_sock){
	printf("%s",buf);
	printf("In rrq\n");
	int fd, block_num, len;
	char childBuf[516];
	unsigned short int *buf_ptr = (unsigned short int*) childBuf;
    //Get file
	char* filename = buf+2;
	ssize_t val;
	int timeout = 0;
	printf("filename: %s", filename);
	//open file
	if((fd = open(filename, O_RDONLY)) < 0){
		printf("%d\n", fd);
        //file not found, add error to send buffer
		*buf_ptr = htons(5);
		*(buf_ptr + 1) = htons(1);
		*(childBuf + 4) = 0;
		printf("FILE NOT FOUND\n");
		file_send:
		val = sendto(childfd, childBuf, 5, 0, (struct sockaddr *) &server_sock, sizeof(server_sock));
        //error sending
		if(val < 0){
			if(errno == EINTR){
				goto file_send;
			}
			perror("sendto error");
			exit(-1);
		}
		exit(-1);
	}
	
	block_num = 1;
	while((len = read(childfd, buf_ptr+2 , 512)) != 0){
		*buf_ptr = htons(3);
		*(buf_ptr + 1) = htons(block_num);
		send_rrq:
		val = sendto(childfd,childBuf,len+4,0, (struct sockaddr *) &server_sock, sizeof(server_sock));
		if (val < 0){
			if (errno == EINTR){
				goto send_rrq;
			}
			perror("read packet failed to send");
			exit(-1);
		}
	}

	retrieve_ack:

	val = recvfrom(childfd, childBuf, sizeof(childBuf), 0, (struct sockaddr *) server_sock, &buf_size);

	if (val < 0) {
		if (errno == EINTR){
			if (timeout < 10){
				goto send_rrq;
			} else {
				printf("Timed out\n");
				close(childfd);
				exit(-1);
			}
			perror("ERROR: recvfrom");
			exit(-1);
		}
	} else {
		char temp[4];
		unsigned short int *temp_ptr = (unsigned short int*) temp;
        if(*temp_ptr == htons(4) && *(temp_ptr + 1) == htons(block_num)){
            block_num++;
        } else {
            printf("Unexpected packet, exiting\n");
            exit(-1);
        }
	}
	printf("success\n");
	close(childfd);
}     

void WRQ(int childfd, struct sockaddr_in *child_sock, struct sockaddr_in *server_sock, char *buf, unsigned int buf_size){

	//Get file
	char* filename;
	strcpy(filename,buf+2);
	int block; 
	int block_len;
	ssize_t len; 
	char* childBuf[516]; 
	unsigned short int *opcode_ptr = (unsigned short int*) childBuf;
	// so I am assuming the client is going to send data and I have 
	// to receive it. Need to open a new file in w mode and write the buffer
	// to the file
	// If the file exists, make sure nothing is written.
	if(fileExists(filename)){
		//send a ERROR packet
		*opcode_ptr = htons(5);
		*(opcode_ptr + 1) = htons(1);
		*(childBuf + 4) = 0;
		printf("FILE EXISTS\n");
		file_exists:
		len = sendto(childfd, childBuf, 5, 0, (struct sockaddr *) &child_sock, sizeof(child_sock));
        //error sending
		if(len < 0){
			if(errno == EINTR){
				goto file_exists;
			}
			perror("sendto error");
			exit(-1);
		}
		exit(-1);
	}
	//open the file since it does not exist 
	memset(&childBuf, 0, sizeof(childBuf));
	FILE *fptr = fopen(filename, "w"); 
	int kid_fd = fileno(*fptr);
	//send ACK that it's about to happen! 
	*opcode_ptr = htons(3);
	*(opcode_ptr + 1) = htons(0);
	//now go through and send "DATA" things to the file 
	while(1){
		memset(&childBuf, 0, sizeof(childBuf));
		len = recvfrom(child_sock, childBuf, 516, 0, (struct sockaddr*) server_sock, sizeof(*child_sock)); 
		if(len < 0){
			perror("put more stuff related to timeouts here"); 
			exit(-1); 
		}
		//TODO: check it's a DATA block 
		if (*(unsigned short *) childBuf != htons(03)) {
			printf("ERROR: DID NOT RECIEVE DATA PACKET\n");
			continue; 
		}
		//TODO: check it's the right block_num / INTEGRATE BLOCK SIZES WHOOPS 
		buffer[len] = '\0'
		write(kid_fd, childBuf + 4, n-4); 
		if(len < 516){
			break;
		}
	}
	//SEND one last ACK not sure if it's the way too but whatevs 
	*opcode_ptr = htons(3);
	*(opcode_ptr + 1) = htons(0);
	//close stuff now! 
	close(kid_fd); 
	close(childfd);
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

	//printf("In child\n");

	if (opcode == 1){
		printf("pre read\nbuffer: %s", buf);
		RRQ(childfd, &child_sock, buf, buf_size, server_sock);
	}

	if (opcode == 2){
		WRQ(childfd, &child_sock, &server_sock, buf, buf_size);
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
	server_sock.sin_port = htons(6500); //TODO: set to 0 when all done


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

	//signal(SIGCHLD, sig_child); 
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
