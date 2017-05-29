/*
** name: passaround.c
**
** author: Andrew Shields
** date: 2/19/17
** last modified: 2/19/17
**
** from template created 31 jan 2015 by bjr
**
*/


#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<assert.h>

#include "passaround.h"
//#include "passaround-util.h"

#define LOCALHOST "localhost"
#define MAXMSGLEN 2048
#define N_REPEAT_DEFAULT 1
#define DELIM ':'

#define USAGE_MESSAGE "usage: passaround [-v] [-n num] [-m message] port"
#define PROG_NAME "passaround" 

int g_verbose = 0 ;

int main( int argc, char * argv[] ) {

    int sock1, sockfd ;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr ;
	struct hostent *he ;
	int numbytes ;
	unsigned int addr_len ;
	int the_port = 0 ;
	char * host = NULL ;
	char * msg = NULL ;
	int ch ;
	char buf[MAXMSGLEN];
	int n_repeat = N_REPEAT_DEFAULT ;
	int is_forever = 0 ;
	StringPair sp;

	assert(sizeof(short)==2) ; 

	while ((ch = getopt(argc, argv, "vm:n:")) != -1) {
	  switch (ch) {
		case 'n':
			n_repeat = atoi(optarg) ;
			break ;
		case 'v':
			g_verbose = 1 ;
			break ;
		case 'm':
			msg = strdup(optarg) ;
			break ;
		case '?':
		default:
			printf(USAGE_MESSAGE) ;
			return 0 ;
	  }
	}
	
	argc -= optind;
	argv += optind;

	if ( argc!= 1 ) {
		fprintf(stderr,"%s\n",USAGE_MESSAGE) ;
		exit(0) ;
	}

	the_port = atoi(*argv) ;
	assert(the_port) ;

	is_forever = (n_repeat == 0) ;

	//create a UDP socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 ) {
		perror("socket") ;
		exit(1) ;
	}

	// create my address block
	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons((short)the_port) ;
	my_addr.sin_addr.s_addr = INADDR_ANY ;
	memset(&(my_addr.sin_zero),'\0',8) ;

	//bind address block
	if (bind(sockfd, (struct sockaddr *)&my_addr, 
		sizeof(struct sockaddr)) == -1 ) {
		perror("bind") ;
		exit(1) ;
	}

	if ( msg ) {

		//parse and send
		sp = pa_parse(msg, DELIM);
		if(sp.rest == NULL) {
		  sp.rest = "";
		}

		if ((he=gethostbyname(sp.first))==NULL) {
			perror("gethostbyname") ;
			exit(1) ;
		}

		//create address block for first message
		their_addr.sin_family = AF_INET ;
		their_addr.sin_port = htons((short)the_port) ;
		their_addr.sin_addr = *((struct in_addr *)he->h_addr) ;
		memset(&(their_addr.sin_zero), '\0', 8 ) ;

		if ((numbytes=sendto(sockfd, sp.rest, strlen(sp.rest),0,
				(struct sockaddr *)&their_addr, sizeof(struct sockaddr)) ) == -1 ) {
			perror("sendto") ;
			exit(1) ;
		}

    	printf("S: %s:%d |%s|\n", inet_ntoa(their_addr.sin_addr), ntohs(their_addr.sin_port), sp.rest); 

		if (g_verbose) {
			printf("send %d bytes to %s\n", numbytes, inet_ntoa(their_addr.sin_addr)) ;
		}

		free(msg) ;
		n_repeat-- ; // a packet sent
	}



	while( is_forever || n_repeat ) {

		// receive packet
		addr_len = sizeof(struct sockaddr) ;
		if ((numbytes=recvfrom(sockfd, buf, MAXMSGLEN-1, 0,
				(struct sockaddr *)&their_addr, &addr_len)) == -1 ) {
			perror("recvfrom") ;
			exit(1) ;
		}
		// assume can be a string, and terminate
		buf[numbytes] = '\0' ;
	
		printf("R: %s:%d |%s|\n", inet_ntoa(their_addr.sin_addr), ntohs(their_addr.sin_port), buf);
		
		if ( g_verbose ) {
			/* added to get source port number */
			printf("got packet from %s, port %d\n", inet_ntoa(their_addr.sin_addr), 
					ntohs(their_addr.sin_port)) ;
			printf("packet is %d bytes long\n", numbytes ) ;
			
			/* Mild bug: if the incoming data was binary, the "string" might be less than numbytes long */
			printf("packet contains \"%s\"\n", buf ) ;
		}
	
		//more to pass around
		if (numbytes > 0) {
		   /* return the packet to sender */

			sp = pa_parse(buf, DELIM);
			if(sp.rest == NULL) {
				sp.rest = "";
			}

			if ((he=gethostbyname(sp.first))==NULL) {
				perror("gethostbyname") ;
				exit(1) ;
			}

			their_addr.sin_family = AF_INET ;
			their_addr.sin_port = htons((short)the_port) ;
			their_addr.sin_addr = *((struct in_addr *)he->h_addr) ;
			memset(&(their_addr.sin_zero), '\0', 8 ) ;
			
			if ((numbytes=sendto(sockfd, sp.rest, strlen(sp.rest),0,
					(struct sockaddr *)&their_addr, sizeof(struct sockaddr)) ) == -1 ) {
				perror("sendto") ;
				exit(1) ;
			}
			printf("S: %s:%d |%s|\n", inet_ntoa(their_addr.sin_addr), ntohs(their_addr.sin_port), sp.rest);
			if (g_verbose) {
				printf("send %d bytes to %s\n", numbytes, inet_ntoa(their_addr.sin_addr)) ;
			}
		}	
		n_repeat-- ;
	}
	close(sockfd) ;
	return 0 ;

}
