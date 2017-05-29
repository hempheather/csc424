#
# makefile for packet passarond
# 23 feb 2017
# bjr
#

PORT= 33031
MC= shiloh
MS= antietam

all:
	make passaround

passaround: passaround.c passaround-util.o passaround.h 
	cc -o $@ $< passaround-util.o

passaround-util-test: passaround-util-test.c passaround-util.o passaround.h
	cc -o $@ $<  passaround-util.o

passaround-util.o: passaround-util.c passaround.h
	cc -c -o $@ $<

test2:
	@echo make test2-s on ${MS} then make test2-c on ${MC}
	
test1: passaround
	./passaround -n 3 -m localhost:localhost:localhost ${PORT} > test1.out
	-diff test1.ref test1.out

test2-s: passaround
	@echo Run this on ${MS}
	./passaround -n 2 ${PORT} > test2-s.out
	-diff test2-s.ref test2-s.out
	
test2-c: passaround
	./passaround -n 3 -m ${MS}:${MC}:${MS}:${MC} ${PORT} > test2-c.out
	-diff test2-c.ref test2-c.out
	
test-util: passaround-util-test
	./passaround-util-test "hello"
	./passaround-util-test "hello:world"
	./passaround-util-test "hello:cruel:world"
	
clean:
	-rm passaround-util.o passaround-util-test passaround
	-rm test1.out test2-s.out test2-c.out
