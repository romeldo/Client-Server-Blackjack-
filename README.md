Client-Server-Blackjack-
========================
This project allows up to 4 users to play Blackjack. The users can
be on different machines, only the IP address and port number of
the running server program needs to be known.

To compile everything type at the command prompt:

>make


To start the server with a port number 2001 type:

>./server 2001


To start a client to connect to the server above type:

>./client 127.0.0.1 2001


Up to 4 clients can be started. If the time the server allows for
input from the users is too short, it can be increased by modifying
the following variables in the file 'common.h':

#define MAX_WAIT_RCV 5
#define MAX_WAIT_GAME 30

The first one allows maximum 5 seconds for client input, and the
second one for maximum 30 seconds for the server to wait for
players to connect.

After that the server spawns a new process 'dealer' which plays
against the players, and when it's done the server continues to
listen for new connections for a new game.

The project implements everything that's in the requirements, I
believe not a single thing is missing. One small limitation is that
if a player waits for too long and then chooses "STAND", his client
program will hang forever because he's been disconnected by the dealer,
so now he needs to hit Ctrl-C to kill his client program. In all other
cases the sockets use a timeout, and if they don't receive the
required data they return with an error which the program handles
gracefully. The "STAND" feature could also be made to wait for limited
time only, but in this case the dealer would need to send periodical
"WAIT" messages to all clients which are in "STAND" mode, until all
clients are finished HIT-ting.
