/*
** name: mradius
**
** author: bjr
** created: 30 mar 2017
** last modified: 16 apr 2017
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
#include<openssl/sha.h>


#include "mradius.h"

int g_verbose = 0 ;
int g_debug = 0 ;

char* find_attribute(char* attributes, short attr, int rp_len) {
	int offset = 0;
	while(offset < rp_len - 24) {
		if( attr == ntohs(*((short *)(attributes + offset)))) {
			return strndup(attributes + offset + 4, *((short*)(attributes + offset + 2)));
		}
		offset += 4 + ntohs(*((short *)(attributes + offset + 2)));
	}
	if ( g_verbose ) printf("attribute (%d) not found\n", attr);
	return NULL;
}


void encrypt_password(char dest[32], char* pw, char* ra, char* shared_key) {
	MD5_CTX context;
	char src[32];
	int i;

	memset(&src, 0, 32);
	memcpy(src, pw, 20);

	MD5_Init(&context);
	MD5_Update(&context, shared_key, strlen(shared_key));
	MD5_Update(&context, ra, MD5_DIGEST_LENGTH);
	MD5_Final((unsigned char *)dest, &context);

	for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
		dest[i] ^= src[i]; 
	}

	MD5_Init(&context);
	MD5_Update(&context, shared_key, strlen(shared_key));
	MD5_Update(&context, dest, MD5_DIGEST_LENGTH);
	MD5_Final((unsigned char *)dest + 16, &context);

	for(i = MD5_DIGEST_LENGTH; i <  2 * MD5_DIGEST_LENGTH; i++) {
		dest[i] ^= src[i]; 
	}
}

void decrypt_password(char dest[20], char* pw, char* ra, char* shared_key) {
	MD5_CTX context;
	char buf[32];
	int i;

	MD5_Init(&context);
	MD5_Update(&context, shared_key, strlen(shared_key));
	MD5_Update(&context, pw, MD5_DIGEST_LENGTH);
	MD5_Final((unsigned char *)buf + 16, &context);

	for(i = MD5_DIGEST_LENGTH; i <  2 * MD5_DIGEST_LENGTH; i++) {
		buf[i] ^= pw[i]; 
	}

	MD5_Init(&context);
	MD5_Update(&context, shared_key, strlen(shared_key));
	MD5_Update(&context, ra, MD5_DIGEST_LENGTH);
	MD5_Final((unsigned char *)buf, &context);

	for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
		buf[i] ^= pw[i]; 
	}

	memcpy(dest, buf, SHA_DIGEST_LENGTH);
}

int main(int argc, char * argv[]) {
	int ch ;
	struct Params params ;

	memset( &params, 0, sizeof(struct Params)) ; 
	params.port = DEFAULT_PORT ;
	params.shared_key = DEFAULT_SHARED_KEY ;
	
	while ((ch = getopt(argc, argv, "vRWLk:p:h:D:n:")) != -1) {
		switch(ch) {
		case 'v':
			g_verbose ++ ;
			g_debug |= DEBUGFLAG_VERBOSE ;
			break ;
		case 'D':
			g_debug |= atoi(optarg) ;
			break ;
		case 'R':
			params.no_randomness ++ ;
			g_debug |= DEBUGFLAG_NORANDOM ;
			break ;
		case 'L':
			params.no_loop = 1 ;
			break ;
		case 'h':
			params.host = strdup(optarg) ;
			break ;
		case 'p':
			params.port = atoi(optarg) ;
			break ;
		case 'k':
			params.shared_key = strdup(optarg) ;
			break ;
		case 'n':
			params.is_hashchain = 1 ;
			params.hashchain_len = atoi(optarg) ;
			break ;
		case 'W':
			params.use_otp = 1 ;
			break ;
		case '?':
		default:
			printf("%s\n",USAGE_MESSAGE) ;
			return 0 ;
		}
	}
	argc -= optind;
	argv += optind;

	if ( (argc!=1 && !params.host ) ||	(argc!=2 && params.host ) ) {
		fprintf(stderr,"%s\n",USAGE_MESSAGE) ;
		exit(0) ;
	}

	if ( params.is_hashchain ) {
		params.upass = strdup(argv[0]) ;
		mradius_hashchain( &params ) ;
	}
    else if ( !params.host ) {
    	params.pwfile = strdup(argv[0]) ;
    	mradius_server( &params ) ;	
	}
	else {
		params.uname = strdup(argv[0]) ;
		params.upass = strdup(argv[1]) ;
		mradius_client( &params ) ;
	}
	return 0 ;
}

