#
# Name: Andrew Shields
# Date: 14 feb 2016
# 
# Note: Makefiles begin with a captal M
#   Indentation MUST be with a tab
#
# tagets build, test, clean and submit are required.
# 

COPTS= 
#COPTS= -Wall

# adjust these macro values accordingly.
# you may have to adjust PORT= to claim an available port
PORT= 33031
FILE_S = file-on-server.txt
FILE_C = file-on-client.txt
SERVER_HOST = localhost

all:
	@echo "Targets: build, test, clean, submit"

build:
	make ttftp

ttftp-server.o: ttftp-server.c ttftp.h
	cc ${COPTS} -c -o $@ $<

ttftp-client.o: ttftp-client.c ttftp.h
	cc ${COPTS} -c -o $@ $<

ttftp: ttftp.c ttftp-server.o ttftp-client.o ttftp.h
	cc ${COPTS} -o $@ $< ttftp-client.o ttftp-server.o 

run-server: ttftp
	echo `date` >> ${FILE_S}
	./ttftp ${PORT}
	
test: ttftp
	echo `date` >> ${FILE_S}
	./ttftp -L ${PORT} &
	@echo "server started"	
	./ttftp -h ${SERVER_HOST} -f ${FILE_S} ${PORT} > ${FILE_C}
	diff ${FILE_S} ${FILE_C}

clean:
	-rm ttftp ttftp-server.o ttftp-client.o ${FILE_S} ${FILE_C}

submit:
	@echo svn add your work
	svn commit -m "submitted for grading"


