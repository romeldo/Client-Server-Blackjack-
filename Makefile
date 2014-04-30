all:		client server dealer

client:		client.c common.h
			cc client.c -o client

server:		server.c common.h
			cc server.c -o server

dealer:		dealer.c common.h
			cc dealer.c -o dealer -lpthread

clean:
			rm -f client server dealer
