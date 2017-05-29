/*
** name: mradius-client
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


int mradius_client( struct Params * params ) {
	RadiusPacket *rp;
	MD5_CTX context;
	int sockfd, i ;
	int numbytes, bytes_sent ;
	int user_len, pass_len;
	struct sockaddr_in their_addr ;
	struct hostent *he ;
	char buf[MAXMSGLEN];
	unsigned char md5hash[MD5_DIGEST_LENGTH];
	unsigned int addr_len;
	char * pass_loc;
	

	user_len = strlen(params->username) + 2 * sizeof(short);
	pass_len = 2 * sizeof(short) + 16 * sizeof(char);

	numbytes = 24 + user_len + pass_len;
	
	rp = malloc(numbytes);

	//create main part of Radius Packet
	*((short*)rp->code) = htons(ACCESS_REQUEST);
	*((long*)rp->length) = htonl(numbytes);

	// random parts
	if(params->no_randomness == 0) {
		FILE* fp;
		fp = fopen("/dev/urandom", "r");
		fread(rp->ident, sizeof(short), 1, fp);
		fread(rp->auth, sizeof(char), MD5_DIGEST_LENGTH, fp);
		fclose(fp);
	} else {
		*((short*)rp->ident) = htons(1);
		for (i = 0; i < MD5_DIGEST_LENGTH; i++) {
			rp->auth[i] = i + 1;
		}
	}

	// copy user attribute
	*((short*)rp->attributes) = htons(ATT_USER_NAME);
	*((short*)(rp->attributes + 2)) = htons(strlen(params->username));
	strncpy((rp->attributes + 4), params->username, strlen(params->username));

	// create and copy password attribute
	pass_loc = rp->attributes + user_len + 4;
	MD5((unsigned char *)params->password, strlen(params->password), (unsigned char *)pass_loc);

	MD5_Init(&context);
	MD5_Update(&context, params->shared_key, strlen(params->shared_key));
	MD5_Update(&context, rp->auth, MD5_DIGEST_LENGTH);
	MD5_Final(md5hash, &context);

	*((short*)(rp->attributes + user_len)) = htons(ATT_USER_PASSWORD);
	*((short*)(rp->attributes + user_len + 2)) = htons(MD5_DIGEST_LENGTH);
	
	for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
		pass_loc[i] ^= md5hash[i];
	}

	// create a socket to send
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 ) {
		perror("socket") ;
		exit(1) ;
	}

	// get hostname
	if ((he=gethostbyname(params->host))==NULL) {
		perror("gethostbyname") ;
		exit(1) ;
	}

	//create address block
	their_addr.sin_family = AF_INET ;
	their_addr.sin_port = htons((short)params->port) ;
	their_addr.sin_addr = *((struct in_addr *)he->h_addr) ;
	memset(&(their_addr.sin_zero), '\0', 8 ) ;

	// send RadiusPacket
	if ((bytes_sent=sendto(sockfd, rp, numbytes, 0,
			(struct sockaddr *)&their_addr, sizeof(struct sockaddr)) ) == -1 ) {
		perror("sendto") ;
		exit(1) ;
	}

	// wait for response
	if ((numbytes=recvfrom(sockfd, buf, MAXMSGLEN-1, 0,
			(struct sockaddr *)&their_addr, &addr_len)) == -1 ) {
		perror("recvfrom") ;
		exit(1) ;
	}

	buf[MAXMSGLEN - 1] = '\0';

	MD5_Init(&context);
	MD5_Update(&context, rp, numbytes);
	MD5_Update(&context, params->shared_key, strlen(params->shared_key));
	MD5_Final(md5hash, &context);

	free(rp);

	rp = (RadiusPacket *)buf;

	if (memcmp(rp->auth, md5hash, MD5_DIGEST_LENGTH) != 0) {
		if ( g_verbose ) printf("mismatch in auth\n");
		printf("NO\n");
	} else {
		if(ntohs(*(short *)rp->code) == ACCESS_ACCEPT) {
			printf("YES\n");
		} else {
			printf("NO\n");
		}
	}
	return 0 ; 
}
