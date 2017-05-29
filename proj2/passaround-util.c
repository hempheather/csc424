/*
 * passaround-util.c
 *
 * created: 09 feb 2017
 * author: bjr
 * last-update:
 *
 */

#include<stdio.h>
#include<string.h>

#include "passaround-util.h"


StringPair pa_parse(char * str, int sep) {
	StringPair sp ;
	char * p ;
	sp.first = str ;
	sp.rest = NULL ;
	p = strchr(str,sep) ;
	if (p) {
		*p = '\0' ;
		sp.rest = p+1 ;
	}
	return sp ;
}

