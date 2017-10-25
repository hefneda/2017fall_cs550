CC = clang

all: server client 
	@echo build complete

server: server.c
	$(CC) $(CFLAGS) server.c -o server -lpthread

client: client.c 
	$(CC) $(CFLAGS) client.c -o client -lpthread

clean: 
	@rm -f ./server ./client
	@rm -fr ./server.dSYM ./client.dSYM
