CC = gcc
DEBUG = -g
CFLAGS = -Wall -lpthread -c $(DEBUG)
LFLAGS = -Wall -lpthread $(DEBUG)

all: client server

client: client.o chatroom_utils.o
ifeq ($(OS),Windows_NT)
	$(CC) $(LFLAGS) client.o chatroom_utils.o -lws2_32 -o client
else 
	$(CC) $(LFLAGS) client.o chatroom_utils.o -o client
endif

server: server.o chatroom_utils.o
ifeq ($(OS),Windows_NT)
	$(CC) $(LFLAGS) server.o chatroom_utils.o -lws2_32 -o server
else
	$(CC) $(LFLAGS) server.o chatroom_utils.o -o server
endif

client.o: client.c chatroom_utils.h
	$(CC) $(CFLAGS) client.c

server.o: server.c chatroom_utils.h
	$(CC) $(CFLAGS) server.c

chatroom_utils.o: chatroom_utils.h chatroom_utils.c
	$(CC) $(CFLAGS) chatroom_utils.c

clean:
	rm -rf *.o *~ client server
