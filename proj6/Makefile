# 
# Note: Makefiles begin with a captal M
#   Indentation MUST be with a tab
#
# tagets build, test, clean and submit are required.
# 

COPTS= 
#COPTS= -Wall

PASSFILE= mypasswords.txt
PASSWORD= paSSw0rd1

build:
	make mradius

mradius: mradius.c mradius.h utils.o mradius-client.o mradius-server.o mradius-hashchain.o
	cc ${COPTS} -o $@ -lcrypto $< utils.o mradius-client.o mradius-server.o mradius-hashchain.o

mradius-client.o: mradius-client.c mradius.h
	cc ${COPTS} -c -o $@ $< 
	
mradius-server.o: mradius-server.c mradius.h
	cc ${COPTS} -c -o $@ $< 

mradius-hashchain.o: mradius-hashchain.c mradius.h
	cc ${COPTS} -c -o $@ $< 

utils.o: utils.c mradius.h
	cc ${COPTS} -c -o $@ $< 

test-hashchain: mradius
	./mradius -n 100 ${PASSWORD}

run-server-nr: mradius
	./mradius -vLR ${PASSFILE}
	./mradius -vLR ${PASSFILE}
	
run-server: mradius
	./mradius -v -W ${PASSFILE}
	
run-client-nr: mradius
	./mradius -vR -h localhost burt passwrds
	sleep 1
	./mradius -vR -h localhost burt wrong

run-client: mradius
	./mradius -v -h localhost burt passwrds
	./mradius -v -h localhost burt wrong

run-client-custom: mradius
	@echo test otp
	./mradius -vW -h localhost burt ${PASSWORD}
	./mradius -vW -h localhost burt wrong
	@echo test standard
	./mradius -v -h localhost pika ${PASSWORD}
	./mradius -v -h localhost pika wrong

clean:
	-rm mradius utils.o mradius-client.o mradius-server.o mradius-hashchain.o

submit:
	@echo svn add your work
	svn commit -m "submitted for grading"


