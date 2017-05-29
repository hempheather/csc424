/*
** name: mradius-hashchain
**
** author: 
** created: 30 mar 2017
** last modified:
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
#include<openssl/sha.h>

#include "mradius.h"


int mradius_hashchain( struct Params * params ) {
	unsigned char buf[SHA_DIGEST_LENGTH] ;
	unsigned char * sha ;
	assert(params->hashchain_len>0) ;
	assert(params->upass) ;
	SHA1((unsigned char *) params->upass, strlen(params->upass), buf) ;
	while (--params->hashchain_len) {
		sha = SHA1( buf, SHA_DIGEST_LENGTH, NULL ) ;
		memcpy( buf, sha, SHA_DIGEST_LENGTH ) ;
	}
	print_hex((char *) buf, SHA_DIGEST_LENGTH) ;
	printf("\n") ;
	return 0 ; 
}
