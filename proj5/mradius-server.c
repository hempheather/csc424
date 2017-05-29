/*
** name: mradius-server
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

int findAttribute(char* ptr, short attr, int rp_len) {
	int offset = 0;
	while(offset < rp_len - 24) {
		if( attr == ntohs(*((short *)(ptr + offset)))) {
			return 4 + offset;	
		}
		offset = offset + 4 + ntohs(*((short *)(ptr + offset + 2)));
	}
	return -1;
}

int mradius_server( struct Params * params, Node * ll_pwds ) {
	int sockfd;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	int is_noloop = 0;
	
	// create a socket to listen for RRQ
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 ) {
		perror("socket") ;
		exit(1) ;
	}

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons((short)params->port) ;
	my_addr.sin_addr.s_addr = INADDR_ANY ;
	memset(&(my_addr.sin_zero),'\0',8) ;

	if (bind(sockfd, (struct sockaddr *)&my_addr, 
		sizeof(struct sockaddr)) == -1 ) {
		perror("bind") ;
		exit(1) ;
	}

	do {
		MD5_CTX context;
		Node *user;
		RadiusPacket *rp;
		char buf[MAXMSGLEN];
		char* ptr;
		char *username;
		char *password;
		int numbytes;
		int offset;
		short user_len;
		short code = ACCESS_ACCEPT;
		unsigned int addr_len;


		addr_len = sizeof(struct sockaddr) ;
		if ((numbytes=recvfrom(sockfd, buf, MAXMSGLEN-1, 0,
				(struct sockaddr *)&their_addr, &addr_len)) == -1 ) {
			perror("recvfrom") ;
			exit(1) ;
		}
		buf[numbytes] = '\0';

		// parse request
		rp = (RadiusPacket *) buf;

		offset = findAttribute(rp->attributes, ATT_USER_NAME, numbytes);
		if(offset < 0) {
			username = NULL;
			code = ACCESS_REJECT;
			if ( g_verbose ) printf("no username provided\n");
		} else {
			username = strndup(rp->attributes + offset, *((short*)(rp->attributes + offset - 2)));
			user = find_node(ll_pwds, username);

		}

		offset = findAttribute(rp->attributes, ATT_USER_PASSWORD, numbytes);
		if(offset < 0) {
			password = NULL;
			code = ACCESS_REJECT;
			if ( g_verbose ) printf("no password provided\n");
		} else {
			password = strndup(rp->attributes + offset, 16);
		}

		if(user == NULL) {
			// user not in file
			code = ACCESS_REJECT;
			if ( g_verbose ) printf("user not found\n");
		} else {
			//verify password
			unsigned char md5hash[MD5_DIGEST_LENGTH];
			unsigned char md5pass[MD5_DIGEST_LENGTH];
			int i;

			MD5((unsigned char *)user->pass, strlen(user->pass), md5pass);

			MD5_Init(&context);
			MD5_Update(&context, params->shared_key, strlen(params->shared_key));
			MD5_Update(&context, rp->auth, MD5_DIGEST_LENGTH);
			MD5_Final(md5hash, &context);

			for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
				md5hash[i] ^= md5pass[i];
			}

			if(memcmp(md5hash, password, MD5_DIGEST_LENGTH) != 0) {
				code = ACCESS_REJECT;
				if ( g_verbose ) printf("incorrect password\n");
			}
		}
		   
		// calculate new hash 
		// ResponseAuth = MD5(Code+ID+Length+rpuestAuth+Attributes+Secret)
		MD5_Init(&context);
		MD5_Update(&context, rp, numbytes);
		MD5_Update(&context, params->shared_key, strlen(params->shared_key));
		MD5_Final((unsigned char *)rp->auth, &context);

		*((short*)rp->code) = htons(code);
	
		// send response
		if ((numbytes=sendto(sockfd, rp, numbytes, 0,
			(struct sockaddr *)&their_addr, sizeof(struct sockaddr)) ) == -1 ) {
			perror("sendto") ;
			exit(1) ;
		}

	} while (!is_noloop) ;
	close(sockfd);
	return 0 ; 
}
