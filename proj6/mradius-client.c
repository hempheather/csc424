/*
** name: mradius-client
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
	

	if(strlen(params->upass) == 2 * SHA_DIGEST_LENGTH) {
		printf("Passwords cannot be 40 characters long\n");
		exit(EXIT_SUCCESS);
	}

	user_len = 4 + strlen(params->uname);
 
	if(params->use_otp) {
		pass_len = 0;
	} else {
		pass_len = 4 + MD5_DIGEST_LENGTH;
	}

	numbytes = 24 + user_len + pass_len;
	
	rp = malloc(numbytes);

	//create main part of Radius Packet
	*((short*)rp->code) = htons(RFC2865_ACC_REQ);
	*((int*)rp->length) = htonl(numbytes);

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
	*((short*)rp->attributes) = htons(RFC2865_ATT_U_NAME);
	*((short*)(rp->attributes + 2)) = htons(strlen(params->uname));
	strncpy((rp->attributes + 4), params->uname, strlen(params->uname));

	if(!params->use_otp) {
		// create and copy password attribute
		*((short*)(rp->attributes + user_len)) = htons(RFC2865_ATT_U_PASS);
		*((short*)(rp->attributes + user_len + 2)) = htons(MD5_DIGEST_LENGTH);

		pass_loc = rp->attributes + user_len + 4;
		MD5((unsigned char *)params->upass, strlen(params->upass), (unsigned char *)pass_loc);

		MD5_Init(&context);
		MD5_Update(&context, params->shared_key, strlen(params->shared_key));
		MD5_Update(&context, rp->auth, MD5_DIGEST_LENGTH);
		MD5_Final(md5hash, &context);

	
		for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
			pass_loc[i] ^= md5hash[i];
		}
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

	MD5_Init(&context);
	MD5_Update(&context, rp, bytes_sent);
	MD5_Update(&context, params->shared_key, strlen(params->shared_key));
	MD5_Final(md5hash, &context);
	free(rp);

	// wait for response
	if ((numbytes=recvfrom(sockfd, buf, MAXMSGLEN-1, 0,
			(struct sockaddr *)&their_addr, &addr_len)) == -1 ) {
		perror("recvfrom") ;
		exit(1) ;
	}

	buf[MAXMSGLEN - 1] = '\0';
	rp = (RadiusPacket *)buf;


	if (memcmp(rp->auth, md5hash, MD5_DIGEST_LENGTH) != 0) {
		if ( g_verbose ) printf("mismatch in auth\n");
		printf("NO\n");
		exit(EXIT_SUCCESS);
	}

	if(ntohs(*(short *)rp->code) == RFC2865_ACC_ACC) {
		printf("YES\n");
		exit(EXIT_SUCCESS);
	}
	if(ntohs(*(short *)rp->code) == RFC2865_ACC_REJ) {
		printf("NO\n");
		exit(EXIT_SUCCESS);
	}

	{

		int offset, max;
		char *reply;
		unsigned char old_password[SHA_DIGEST_LENGTH];
		unsigned char sha_buf[SHA_DIGEST_LENGTH] ;
		unsigned char *sha;
		RadiusPacket *reply_packet;

		reply_packet = malloc(numbytes + 4 + 2 * MD5_DIGEST_LENGTH);
		memcpy(reply_packet, rp, numbytes);


		MD5_Init(&context);
		MD5_Update(&context, rp, bytes_sent);
		MD5_Update(&context, params->shared_key, strlen(params->shared_key));
		MD5_Final((unsigned char *)reply_packet->auth, &context);

		*((short *)reply_packet->code) = htons(RFC2865_ACC_REQ);
		*((int *)reply_packet->length) = htonl(numbytes + 4 + 2 * MD5_DIGEST_LENGTH);

		// do send stuff
		reply = find_attribute(rp->attributes, RFC2865_ATT_U_REPL, numbytes);
		if(reply == NULL) {
			if ( g_verbose ) printf("no reply message provided\n");
			printf("NO\n");
			exit(EXIT_SUCCESS);
		}

		for(i = 0; i < SHA_DIGEST_LENGTH; i++) {
			sscanf(reply + (i * 2), "%02x", (unsigned int *)&old_password[i]) ;
		}

		max = MAXSSHALP;
		sha = SHA1((unsigned char *) params->upass, strlen(params->upass), sha_buf) ;
		memcpy( sha_buf, sha, SHA_DIGEST_LENGTH ) ;
		sha = SHA1( sha_buf, SHA_DIGEST_LENGTH, NULL ) ;
		while(memcmp(sha, old_password, SHA_DIGEST_LENGTH) != 0 && max > 0) {
			memcpy( sha_buf, sha, SHA_DIGEST_LENGTH ) ;
			sha = SHA1( sha_buf, SHA_DIGEST_LENGTH, NULL ) ;
			max--;
		}

		// sha_buf now contains i - 1 password
		*((short *)((char *)reply_packet + numbytes)) = htons(RFC2865_ATT_U_PASS);
		*((short *)((char *)reply_packet + numbytes + 2)) = htons(2 * MD5_DIGEST_LENGTH);
		encrypt_password((char *)reply_packet + numbytes + 4, (char *)sha_buf, reply_packet->auth, params->shared_key);

		if ((bytes_sent=sendto(sockfd, reply_packet, numbytes + 4 + 2 * MD5_DIGEST_LENGTH, 0,
				(struct sockaddr *)&their_addr, sizeof(struct sockaddr)) ) == -1 ) {
			perror("sendto") ;
			exit(1) ;
		}

		MD5_Init(&context);
		MD5_Update(&context, reply_packet, bytes_sent);
		MD5_Update(&context, params->shared_key, strlen(params->shared_key));
		MD5_Final(md5hash, &context);


		// wait for response
		if ((numbytes=recvfrom(sockfd, buf, MAXMSGLEN-1, 0,
				(struct sockaddr *)&their_addr, &addr_len)) == -1 ) {
			perror("recvfrom") ;
			exit(1) ;
		}


		buf[numbytes] = '\0';
		rp = (RadiusPacket *)buf;


		if (memcmp(rp->auth, md5hash, MD5_DIGEST_LENGTH) != 0) {
			if ( g_verbose ) printf("mismatch in auth\n");
			printf("NO\n");
			exit(EXIT_SUCCESS);
		}

		if(ntohs(*(short *)rp->code) == RFC2865_ACC_ACC) {
			printf("YES\n");
			exit(EXIT_SUCCESS);
		}
		if(ntohs(*(short *)rp->code) == RFC2865_ACC_REJ) {
			printf("NO\n");
			exit(EXIT_SUCCESS);
		}

	}
	
	return 0 ; 
}
