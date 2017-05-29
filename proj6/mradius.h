/*
** name: mradius.h
**
** author: bjr
** created: 30 mar 2017
** last modified: 16 apr 2017
**
*/

#define USAGE_MESSAGE "usage: mradius [-vLR -k key -p port -D debugflags] ( -h host user pwd | pwd-file |  -n num pwd_seed )" 
#define DEFAULT_PORT  1812
#define DEFAULT_SHARED_KEY "pa55word0" 

#define AUTHEN_LEN 16

#define RFC2865_ACC_REQ 1
#define RFC2865_ACC_ACC 2
#define RFC2865_ACC_REJ 3
#define RFC2865_ACC_CHA 11
#define RFC2865_ATT_U_NAME 1
#define RFC2865_ATT_U_PASS 2
#define RFC2865_ATT_U_REPL 18

#define DEBUGFLAG_NOCRYPTO  01
#define DEBUGFLAG_NORANDOM  02
#define DEBUGFLAG_VERBOSE 04

#define MAXMSGLEN 2048
#define MAXSSHALP 1000


extern int g_verbose ;
extern int g_debug ;

struct Params {
	char * host ;
	char * pwfile ;
	int port ;
	char * shared_key ;
	int no_randomness ;
	int no_loop ;
	char * upass ;
	char * uname ;
	int is_hashchain ;
	int hashchain_len ;
	/* add more parameters here, if needed */
	int use_otp;

} ;

typedef struct {
        char * first;
        char * rest ;
} StringPair ;

typedef struct Node {
	char * user ;
	char * pass ;
	struct Node * next ;
} Node ;

typedef struct Pair {
	char * dat ;
	int len ;
} Pair ;

typedef struct {
	char code[2];
	char ident[2];
    char length[4];
	char auth[16];
	char attributes[];
} RadiusPacket ;

char next_random(void) ;

StringPair pa_parse(char * str, int sep) ;

Node * new_node( char * user, char * pass , Node * next ) ; 
Node * find_node( Node * root, char * user ) ;
Node * parse_pwfile( char * filename ) ;
void print_nodes( Node * n ) ;

void print_hex( char * b, int len ) ;

char* find_attribute(char* attributes, short attr, int rp_len);
void encrypt_password(char dest[32], char* pw, char* ra, char* shared_key);
void decrypt_password(char dest[20], char* pw, char* ra, char* shared_key);


int mradius_client( struct Params * params ) ;
int mradius_server( struct Params * params ) ;
int mradius_hashchain( struct Params * params ) ;

