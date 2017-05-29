/*
 * passaround-util-test.c
 *
 * created: 09 feb 2017
 * author: bjr
 * last-update:
 *
 */

#include<stdio.h>
#include "passaround.h"

#define SEP ':'

int g_verbose ;

int main(int argc, char * argv[]) {

	StringPair sp ;
	if (argc!=2) {
		printf("error: incorrect usage.\n") ;
		return 0 ;
	}
	sp = pa_parse(argv[1], SEP) ;
	printf("first: |%s|\n", sp.first ) ;
	if ( sp.rest ) {
		printf("rest: |%s|\n", sp.rest ) ;
	}
	else printf("rest: NULL\n") ;
	return 0 ;
}
