/*
** name: ttftp-server.c
**
** author: Andrew Shields
** created: 14 feb 2016 by bjr
** last modified: 8 mar 2017
**
** from extracted from ttftp.c
*/

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<assert.h>
#include<unistd.h>

#include "ttftp.h"

int ttftp_server( int listen_port, int is_noloop ) {
	int sockfd_l;
	int sockfd_s ;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	int block_count ;
	
	// create a socket to listen for RRQ
	if ((sockfd_l = socket(AF_INET, SOCK_DGRAM, 0)) == -1 ) {
		perror("socket") ;
		exit(1) ;
	}

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons((short)listen_port) ;
	my_addr.sin_addr.s_addr = INADDR_ANY ;
	memset(&(my_addr.sin_zero),'\0',8) ;

	if (bind(sockfd_l, (struct sockaddr *)&my_addr, 
		sizeof(struct sockaddr)) == -1 ) {
		perror("bind") ;
		exit(1) ;
	}

	do {
		unsigned int addr_len;
		struct TftpReq* req ;
		char buf[MAXMSGLEN];
		int numbytes;
		FILE *fp;
		short opcode ;
		int tid_c ;

		/*
		 * for each RRQ 
		 */

		addr_len = sizeof(struct sockaddr) ;
		if ((numbytes=recvfrom(sockfd_l, buf, MAXMSGLEN-1, 0,
				(struct sockaddr *)&their_addr, &addr_len)) == -1 ) {
			perror("recvfrom") ;
			exit(1) ;
		}
		buf[numbytes-1] = '\0';

		tid_c = ntohs(their_addr.sin_port);

		// parse request and open file
		req = (struct TftpReq*)buf;
		opcode = ntohs(*(short*)req->opcode);

		if(opcode != TFTP_RRQ) {
			serialize_error( 4, "Illegal TFTP operation.", sockfd_s, their_addr);
   			continue;
		}

		fp = fopen(req->filename_and_mode, "r");
		if(fp == NULL) {
			serialize_error( 1, "File not found.", sockfd_s, their_addr);
			perror("Error opening file");
			continue;
   		}

		// create a sock for the data packets
		if ((sockfd_s = socket(AF_INET, SOCK_DGRAM, 0)) == -1 ) {
			perror("socket") ;
			exit(1) ;
		}
		
		block_count = 1 ;
		while (block_count) {
			struct TftpData* data;
			struct TftpAck* ack;
			int data_len;
			short opcode = htons(TFTP_DATA) ;
			short block = htons((short) block_count) ;

			// read from file
			data = malloc(TFTP_DATALEN + 4) ;
			
			memcpy(&(data->opcode),  &opcode, sizeof(short));
			memcpy(&(data->block_num),  &block, sizeof(short));
			data_len = fread (data->data, 1, TFTP_DATALEN, fp);

			// send data packet
   			if ((numbytes=sendto(sockfd_s, data, 4 + data_len, 0,
				(struct sockaddr *)&their_addr, sizeof(struct sockaddr)) ) == -1 ) {
				perror("sendto") ;
				exit(1) ;
			}
			free(data);

			// wait for acknowledgement
			addr_len = sizeof(struct sockaddr) ;

			if ((numbytes=recvfrom(sockfd_s, buf, MAXMSGLEN-1, 0,
				(struct sockaddr *)&their_addr, &addr_len)) == -1 ) {
				perror("recvfrom") ;
				exit(1) ;
			}
			ack = (struct TftpAck*)buf;

			if(tid_c != ntohs(their_addr.sin_port)) {
				// TODO - need to wait for another ack
				serialize_error( 5, "Unknown transfer ID", sockfd_s, their_addr);
			}
			if(TFTP_ACK != ntohs(*(short*)ack->opcode)) {
				serialize_error( 4, "Illegal TFTP operation.", sockfd_s, their_addr);
				break;
			}
			if(block_count != ntohs(*(short*)ack->block_num)) {
				serialize_error( 0, "Incorrect block_num received.", sockfd_s, their_addr);
				break;
			}

			// if end of file
			if(feof(fp)) {
	           break; 
			}
	 
			block_count++ ;

		}

		fclose(fp);
	
	} while (!is_noloop) ;
	return 0 ;
}
