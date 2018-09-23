#include "../unpv13e/lib/unp.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


int main(int argc, char const *argv[]){
	
	int sockfd, n;
	struct sockaddr_in server_sock, newsockfd;

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
        socklen_t slen = sizeof(client_sock);
        ssize_t len;

        if ((len = recvfrom(sockfd, &message, &client_sock, &slen)) < 0) {
               continue;
          }
	}
}