/*
** name: mradius
**
** author: Andrew Shields
** created: 30 mar 2017
** last modified: 19 apr 2017
**
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
#include<openssl/md5.h>

#include "mradius.h"

int g_verbose = 0 ;
int g_norandomness = 0 ; 

int main(int argc, char * argv[]) {
	int ch ;
	char * shared_key = DEFAULT_SHARED_KEY ;
	int port = DEFAULT_PORT ;
	char * hostname = NULL ;
	struct Params params ;

	while ((ch = getopt(argc, argv, "vRk:p:h:")) != -1) {
		switch(ch) {
		case 'v':
			g_verbose ++ ;
			break ;
		case 'R':
			g_norandomness ++ ;
			break ;
		case 'h':
			hostname = strdup(optarg) ;
			break ;
		case 'p':
			port = atoi(optarg) ;
			break ;
		case '?':
		default:
			printf("%s\n",USAGE_MESSAGE) ;
			return 0 ;
		}
	}
	argc -= optind;
	argv += optind;

	if ( (argc!=1 && !hostname ) ||	(argc!=2 && hostname ) ) {
		fprintf(stderr,"%s\n",USAGE_MESSAGE) ;
		exit(0) ;
	}

	// sanity check inputs
	params.no_randomness = g_norandomness ;
	params.host = hostname ;
	params.port = port ;
	params.shared_key = shared_key;

	/* your code */
    if ( !hostname ) {
    	// server
		Node * n ;
		if ( g_verbose ) {
			printf("%s:%d: mradius_server, (|%s|)\n", __FILE__, __LINE__, argv[0] ) ;
		}
	
		n = parse_pwfile(argv[0]) ;
		if ( !n ) {
			perror(argv[0]) ;
			return 0 ; 
		}
		if ( g_verbose ) print_nodes(n) ; 
    	mradius_server( &params, n ) ;	
	}
	else {
		params.username = argv[0];
		params.password = argv[1];
		mradius_client(&params) ;
	}
	return 0 ;
}

