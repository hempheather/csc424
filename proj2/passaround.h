/*
** name: passaround.h
**
** author:
** date:
** last modified:
**
** from template created 31 jan 2015 by bjr
**
*/
 
typedef struct {
	char * first;
	char * rest ;
} StringPair ;

StringPair pa_parse(char * str, int sep) ;

extern int g_verbose ;
