
#include "common.h"

#include <sys/types.h>
#include <sys/wait.h>


#include <errno.h>


#define BACKLOGS 5

int main(int argc, char** argv)
{
	int serverSockFd;
	struct sockaddr_in serverAddress;
	socklen_t serverLen;
	int port;

	if (argc != 2)
	{
		printf("Usage: ./server port_no\n");
		printf("Example ./server 2001\n");
		exit(0);
	}

	port = atoi(argv[1]);

	if (0 > (serverSockFd = socket(AF_INET, SOCK_STREAM, 0)))
		error("Error creating a socket");

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(port);
	serverLen = sizeof (serverAddress);
	if (0 > bind(serverSockFd, (struct sockaddr *) &serverAddress, serverLen))
		error("Error biding to socket");

	listen(serverSockFd, BACKLOGS);

	while (TRUE)
	{
		struct sockaddr_in clientAddress;
		socklen_t clientLen = sizeof (clientAddress);
		int clientSockFd[MAX_PLAYERS];

		printf("\nServer waiting to begin a new game...\n\n");

		if (0 > (setsockopt(serverSockFd, SOL_SOCKET, SO_RCVTIMEO, &tv_wait_inf, sizeof (tv_wait_inf))))
			error("Error setting serverSockFd with setsockopt before first accept");

		if (0 > (clientSockFd[0] = accept(serverSockFd, (struct sockaddr *) &clientAddress, &clientLen)))
			error("Error accepting first client");
		else
		{
			char buffer[BUFFER_SIZE];
			struct timeval tv_started;
			gettimeofday(&tv_started, NULL);

			printf("First player connected on socket %d\n", clientSockFd[0]);

			if (read_with_timeout(clientSockFd[0], buffer, tv_wait_rcv))
			{
				if (0 == strncmp(buffer, LETS_PLAY, sizeof (LETS_PLAY)))
				{
					int pid;
					int nclients = 1;

					printf("First player on socket %d confirmed the magic word to play\n", clientSockFd[0]);

					while (nclients < MAX_PLAYERS)
					{
						struct timeval tv_left = difftimeval(tv_wait_game, elapsed_since(tv_started));

						if (0 > (setsockopt(serverSockFd, SOL_SOCKET, SO_RCVTIMEO, &tv_left, sizeof (tv_left))))
							error("Error setting serverSockFd with setsockopt before accept");

						printf("Waiting for next player to connect...\n\n");

						if (0 > (clientSockFd[nclients] = accept(serverSockFd, (struct sockaddr *) &clientAddress, &clientLen)))
							if (errno == EAGAIN)
							{
								printf("Time expired for accepting players\n");
								break;
							}
							else
								error("Error accepting next client");
						else
							printf("Next player connected on socket %d\n", clientSockFd[nclients]);


						if (read_with_timeout(clientSockFd[nclients], buffer, tv_wait_rcv))
							if (0 == strncmp(buffer, LETS_PLAY, sizeof (LETS_PLAY)))
							{
								printf("Player on socket %d confirmed the magic word to play\n\n", clientSockFd[nclients]);
								++nclients;
							}
					}

					/* play game */
					printf("Now we're playing...\n");

					switch (pid = fork())
					{
						case -1:
							error("Error! Unable to fork.");

						case 0: /* child */
						{
							int i;
							char sock_args[MAX_PLAYERS][100];
							char* psock_args[MAX_PLAYERS + 2];

							strcpy(sock_args[0], "./dealer");
							psock_args[0] = sock_args[0];
							for (i = 0; i < nclients; ++i)
							{
								sprintf(sock_args[i + 1], "%d", clientSockFd[i]);
								psock_args[i + 1] = sock_args[i + 1];
							}
							for (i = nclients + 1; i <= MAX_PLAYERS + 1; ++i)
								psock_args[i] = 0;

							execv("./dealer", psock_args);
							error("execv didn't succceed");
						}

						default: /* parent */
						{
							int status;
							wait(&status);
							break;
						}
					}
				}
				else
				{
					printf("First player of socket %d didn't send the magic word to begin a game\n\n", clientSockFd[0]);
					close(clientSockFd[0]);
				}
			}
			else
			{
				printf("First player of socket %d disconnected\n\n", clientSockFd[0]);
				close(clientSockFd[0]);
			}
		}
	}

	return (EXIT_SUCCESS);
}

