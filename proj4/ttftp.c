/*
** name: ttftp.c
**
** author: Andrew Shields
** created: 31 jan 2015 by bjr
** last modified: 8 mar 2017
**		
**
** from template created 31 jan 2015 by bjr
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

#define USAGE_MESSAGE "usage: ttftp [-vL] [-h hostname -f filename] port"

int g_verbose = 0 ;  // global declaration; extern definition in header 


int serialize_error( short code, char* message, int sockfd, struct sockaddr_in their_addr) {
	struct TftpError* err = malloc(strlen(message) + 5);
	short opcode = htons(TFTP_ERR);
	short error_code = htons(code);
	int bytes_sent ;

	memcpy(&(err->opcode),  &opcode, sizeof(short));
	memcpy(&(err->error_code),  &error_code, sizeof(short));
	memcpy(&(err->error_msg),  &message, strlen(message) + 1);

	if ((bytes_sent=sendto(sockfd, err, strlen(message) + 5, 0,
		(struct sockaddr *)&their_addr, sizeof(struct sockaddr)) ) == -1 ) {
		perror("sendto") ;
		exit(1) ;
	}
	free(err);

	return bytes_sent;
}



int main(int argc, char * argv[]) {
	int ch ;
	int is_server = 0 ;
	int port = 0 ; 
	int is_noloop = 0 ; 
	char * hostname = NULL ;
	char * filename = NULL ;

	// check whether we can use short as the data type for 2 byte int's
	assert(sizeof(short)==2) ;

	while ((ch = getopt(argc, argv, "vLf:h:")) != -1) {
		switch(ch) {
		case 'v':
			g_verbose ++ ;
			break ;
		case 'h':
			hostname = strdup(optarg) ;
			break ;
		case 'f':
			filename = strdup(optarg) ;
			break ;
		case 'L':
			is_noloop = 1 ;
			break ;
		case '?':
		default:
			printf("%s\n",USAGE_MESSAGE) ;
			return 0 ;
		}
	}
	argc -= optind;
	argv += optind;

	if ( argc!= 1 ) {
			fprintf(stderr,"%s\n",USAGE_MESSAGE) ;
		exit(0) ;
	}
	port = atoi(*argv) ;

	// sanity check inputs

	assert(port);
	/* your code */

	is_server = (hostname == NULL || filename == NULL); // host and filenames required for client

	if (!is_server ) {
		/* is client */
		return ttftp_client( hostname, port, filename ) ;
	}
	else {
		/* is server */
		return ttftp_server( port, is_noloop ) ;
	}
	
	assert(1==0) ;
	return 0 ;
}

