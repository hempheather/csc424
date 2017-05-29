#
# example networking program from
# "computer networks a systems apprroach" by Peterson and Davie 
#
# makefile author: bjr
# created: 23 jan 2017
# last-update: 23 jan 2017
#

HOST= 172.19.1.43
MESSAGE= "hello world" 

all: simplex-talk simplex-talk-s
	./simplex-talk-s &
	echo ${MESSAGE} | ./simplex-talk ${HOST}
	killall simplex-talk-s
	make clean

run-s: simplex-talk-s
	./simplex-talk-s 

run: simplex-talk-s simplex-talk
	echo ${MESSAGE} | ./simplex-talk ${HOST}

simplex-talk: simplex-talk.c
	cc -o $@ $<

simplex-talk-s: simplex-talk-s.c
	cc -o $@ $<

clean:
	-rm simplex-talk simplex-talk-s
