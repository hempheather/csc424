/*
** name: ttftp-client.c
**
** author: Andrew Shields
** created: 31 jan 2015 by bjr
** last modified: 8 mar 2017
**		 
**
** from template created 31 jan 2015 by bjr
**
*/

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<assert.h>
#include<unistd.h>

#include <time.h>

#include "ttftp.h"

#define h_addr h_addr_list[0]


void delay(double dly){
    /* save start time */
    const time_t start = time(NULL);

    time_t current;
    do{
        /* get current time */
        time(&current);

        /* break loop when the requested number of seconds have elapsed */
    }while(difftime(current, start) < dly);
}


int ttftp_client( char * to_host, int to_port, char * file ) {
	int block_count ; 
	int sockfd ;
	int numbytes ;
	int bytes_sent ;
	int tid_s ;
	short opcode ;
	struct TftpReq* req ;
	struct sockaddr_in my_addr ;
	struct sockaddr_in their_addr ;
	struct hostent *he ;


	// create a socket to send
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 ) {
		perror("socket") ;
		exit(1) ;
	}

	// get hostname
	if ((he=gethostbyname(to_host))==NULL) {
		perror("gethostbyname") ;
		exit(1) ;
	}

	//create address block
	their_addr.sin_family = AF_INET ;
	their_addr.sin_port = htons((short)to_port) ;
	their_addr.sin_addr = *((struct in_addr *)he->h_addr) ;
	memset(&(their_addr.sin_zero), '\0', 8 ) ;


	opcode = htons(TFTP_RRQ);

	numbytes = strlen(file) + strlen(OCTET_STRING) + 4; // add 2 for nulls & 2 for short
	req = malloc(numbytes);
	memcpy(&(req->opcode),  &opcode, 2);
	memcpy(&(req->filename_and_mode),  file, strlen(file) + 1);
	memcpy(&(req->filename_and_mode)[strlen(file) + 1],  OCTET_STRING, strlen(OCTET_STRING) + 1);

	// send RRQ
	if ((bytes_sent=sendto(sockfd, req, numbytes, 0,
			(struct sockaddr *)&their_addr, sizeof(struct sockaddr)) ) == -1 ) {
		perror("sendto") ;
		exit(1) ;
	}
	free(req);

	tid_s = 0;
	block_count = 1 ; /* value expected */
	while ( block_count ) {
		struct TftpData* data;
		struct TftpAck* ack;
		struct TftpError* error;
		unsigned int addr_len;
		char buf[MAXMSGLEN + 1];
		int i;
	
		// read at DAT packet
		addr_len = sizeof(struct sockaddr) ;

		do {
			if ((numbytes=recvfrom(sockfd, buf, MAXMSGLEN-1, 0,
					(struct sockaddr *)&their_addr, &addr_len)) == -1 ) {
				perror("recvfrom") ;
				exit(1) ;
			}

			if(tid_s == 0) {
				tid_s = ntohs(their_addr.sin_port);
			}

			if(tid_s != ntohs(their_addr.sin_port)) {
				serialize_error( 5, "Unknown transfer ID", sockfd, their_addr);
			}
		
		} while (tid_s != ntohs(their_addr.sin_port));
 
		// write bytes to stdout
		opcode = htons(*((short*)buf));
		if(opcode == TFTP_DATA) {
			data = (struct TftpData*) buf;
			for(i = 0 ; i < numbytes - 4 ; i++) {
      			putchar(data->data[i]);
   			}
   		} else if(opcode == TFTP_ERR) {
			// currently assumes all errors cause termination
			error = (struct TftpError*) buf;
			puts(error->error_msg);
			break;
		} else {
			puts("unknown opcode. ending file transfer");
			break;
		}
				 
		//send an ACK
		short opcode = htons(TFTP_ACK);
		short block = htons((short) block_count);
		ack = malloc(4);

		memcpy(&(ack->opcode),  &opcode, sizeof(short));
		memcpy(&(ack->block_num), &block, sizeof(short));

		if ((bytes_sent=sendto(sockfd, ack, 4, 0,
			(struct sockaddr *)&their_addr, sizeof(struct sockaddr)) ) == -1 ) {
			perror("sendto") ;
			exit(1) ;
		}
		free(ack);

		block_count ++ ;
		
		// check if more blocks expected, else set block_count = 0 ;
		if(numbytes < TFTP_DATALEN + 4) {
			block_count = 0;
		}
	}
	close(sockfd);
	return 0 ;
}
