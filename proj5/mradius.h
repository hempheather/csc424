/*
** name: mradius.h
**
** author: Andrew Shields
** created: 30 mar 2017
** last modified: 19 apr 2017
**
*/

#define USAGE_MESSAGE "usage: mradius [-vR -k key -p port] ( -h host user pwd | pwd-file )" 
#define DEFAULT_PORT  1812
#define DEFAULT_SHARED_KEY "pa55word0" 

#define ACCESS_REQUEST 1
#define ACCESS_ACCEPT 2
#define ACCESS_REJECT 3

#define ATT_USER_NAME 1
#define ATT_USER_PASSWORD 2

#define MAXMSGLEN 2048

extern int g_verbose ;
extern int g_norandomness ;

typedef struct {
        char * first;
        char * rest ;
} StringPair ;

typedef struct Node {
	char * user ;
	char * pass ;
	struct Node * next ;
} Node ;

struct Params {
	char * host ;
	int port ;
	int no_randomness ;
	/* add more parameters here, if needed */
	char * username;
	char * password;
	char * shared_key;
} ;

typedef struct {
	char code[2];
	char ident[2];
    char length[4];
	char auth[16];
	char attributes[];
} RadiusPacket ;

StringPair pa_parse(char * str, int sep) ;

Node * new_node( char * user, char * pass , Node * next ) ; 
Node * find_node( Node * root, char * user ) ;
Node * parse_pwfile( char * filename ) ;
void print_nodes( Node * n ) ;

int mradius_client( struct Params * params ) ;
int mradius_server( struct Params * params, Node * ll_pwds ) ;
