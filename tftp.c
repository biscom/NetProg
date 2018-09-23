#include "../unpv13e/lib/unp.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


int main(int argc, char const *argv[]){
	
	int sockfd, n;
	struct sockaddr_in addr, newsockfd;

	// Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 

	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = 0;
	if ((bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1)){ 
		close(sockfd);
    	perror("Error on binding");
	}

    /*
    while (1) {
    	
	}
	*/
	unsigned int len = sizeof(addr);
	int sock_name = getsockname(sockfd, (struct sockaddr *) &addr, &len);
	printf("Port Number: %d\n", sock_name);
}