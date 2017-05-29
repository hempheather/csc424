/*
** name: mradius-server
**
** author: Andrew Shields
** created: 30 mar 2017
** last modified: 1 may 2017
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

int mradius_server( struct Params * params ) {
	int sockfd;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	int is_noloop = 0;
	Node * ll_pwds ;
	ll_pwds = parse_pwfile(params->pwfile) ;
	
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
		MD5_CTX md5_context;
		SHA_CTX sha_context;
		Node *user = NULL;
		RadiusPacket *rp;
		char buf[MAXMSGLEN];
		char* ptr;
		char *username;
		char *password;
		int numbytes;
		int offset;
		short user_len;
		short code = RFC2865_ACC_ACC;
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
		char old_ra[MD5_DIGEST_LENGTH];

		memcpy(old_ra, rp->auth, MD5_DIGEST_LENGTH); 

		MD5_Init(&md5_context);
		MD5_Update(&md5_context, rp, numbytes);
		MD5_Update(&md5_context, params->shared_key, strlen(params->shared_key));
		MD5_Final((unsigned char *)rp->auth, &md5_context);


		password = find_attribute(rp->attributes, RFC2865_ATT_U_PASS, numbytes);
		username = find_attribute(rp->attributes, RFC2865_ATT_U_NAME, numbytes);
		if(username != NULL) {
			user = find_node(ll_pwds, username);

		}

		if(user && !password) {
			RadiusPacket *new_rp;
			if ( g_verbose ) printf("use otp, send challenge response\n");
			
			// add challenge attribute
			new_rp = malloc(numbytes + 44);
			memcpy(new_rp, rp, numbytes);
			rp = new_rp;

			*((short *)((char *)rp + numbytes)) = htons(RFC2865_ATT_U_REPL);
			*((short *)((char *)rp + numbytes + 2)) = htons(2 * SHA_DIGEST_LENGTH);
			if(strlen(user->pass) == 2 * SHA_DIGEST_LENGTH) {
				strncpy((char *)rp + numbytes + 4, user->pass, 2 * SHA_DIGEST_LENGTH);
			} else {
				memset((char *)rp + numbytes + 4,'\0', 2 * SHA_DIGEST_LENGTH) ;
			}		
			// update code and length
			code = RFC2865_ACC_CHA;
			numbytes += 44;

		} else if(user && password && strlen(password) > MD5_DIGEST_LENGTH) {
			if ( g_verbose ) printf("use otp, check challenge response\n");
			unsigned char new_pass[SHA_DIGEST_LENGTH];
			unsigned char sha_pass[SHA_DIGEST_LENGTH];
			unsigned char old_pass[SHA_DIGEST_LENGTH];
			int i;

			decrypt_password((char *)new_pass, password, old_ra, params->shared_key);

        	SHA1(new_pass, SHA_DIGEST_LENGTH, sha_pass);
			for(i = 0; i < SHA_DIGEST_LENGTH; i++) {
				sscanf((const char *)user->pass + (i * 2), "%02x", (unsigned int *)&old_pass[i]);
			}

			if(memcmp(sha_pass, old_pass, SHA_DIGEST_LENGTH) != 0) {
				code = RFC2865_ACC_REJ;
				if ( g_verbose ) printf("incorrect password\n");
			} else {
				unsigned char buf[40];
				code = RFC2865_ACC_ACC;
				for(i = 0; i < SHA_DIGEST_LENGTH; i++) {
					sprintf ((char *)buf + (i * 2), "%02x", new_pass[i]);
				}
				user->pass = strndup((const char *)buf, 2 * SHA_DIGEST_LENGTH);
			}
		} else if(user && password && strlen(password) == MD5_DIGEST_LENGTH) {
			if ( g_verbose ) printf("dont use otp\n");
			unsigned char md5hash[MD5_DIGEST_LENGTH];
			unsigned char md5pass[MD5_DIGEST_LENGTH];
			int i;

			MD5((unsigned char *)user->pass, strlen(user->pass), md5pass);

			MD5_Init(&md5_context);
			MD5_Update(&md5_context, params->shared_key, strlen(params->shared_key));
			MD5_Update(&md5_context, old_ra, MD5_DIGEST_LENGTH);
			MD5_Final(md5hash, &md5_context);

			for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
				md5hash[i] ^= md5pass[i];
			}

			if(memcmp(md5hash, password, MD5_DIGEST_LENGTH) != 0) {
				code = RFC2865_ACC_REJ;
				if ( g_verbose ) printf("incorrect password\n");
			}

		} else {
			code = RFC2865_ACC_REJ;
		}


		*((short*)rp->code) = htons(code);
		*((int *)(rp->length)) = htonl(numbytes);
	
		// send response
		if ((numbytes=sendto(sockfd, rp, numbytes, 0,
			(struct sockaddr *)&their_addr, sizeof(struct sockaddr)) ) == -1 ) {
			perror("sendto") ;
			exit(1) ;
		}

	} while (!params->no_loop) ;
	close(sockfd);
	return 0 ; 
}
