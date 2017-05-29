/*
 * passaround-util.h
 *
 * created: 09 feb 2017
 * author: bjr
 * last-update:
 *
 */
 
typedef struct {
	char * first;
	char * rest ;
} StringPair ;

StringPair pa_parse(char * str, int sep) ;
