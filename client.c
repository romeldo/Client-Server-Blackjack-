
#include "common.h"

void play_game(int clientSockFd)
{
	int my_hand_values[20], dealer_hand_values[20];
	int my_hand_suits[20], dealer_hand_suits[20];
	int nmy = 0, ndealer = 0;
	char buffer[BUFFER_SIZE];
	int nwritten;

	printf("Waiting for game to begin...\n");
	if (read_with_timeout(clientSockFd, buffer, tv_wait_game3))
	{
		int my_sum;
		int dealer_sum;

		my_hand_values[0] = get_value_id(buffer[0]);
		my_hand_suits[0] = get_suit_id(buffer[1]);
		my_hand_values[1] = get_value_id(buffer[2]);
		my_hand_suits[1] = get_suit_id(buffer[3]);
		dealer_hand_values[0] = get_value_id(buffer[4]);
		dealer_hand_suits[0] = get_suit_id(buffer[5]);
		nmy = 2;
		ndealer = 1;

		/* while the player HITs */
		while (TRUE)
		{
			int choice;
			my_sum = calc_sum(my_hand_values, nmy);

			printf("\n");
			printf("My Hand: ");
			display_state(my_hand_values, my_hand_suits, nmy);
			printf("Dealer Hand: ");
			display_state(dealer_hand_values, dealer_hand_suits, ndealer);

			if (my_sum > 21)
			{
				printf("\nI'm busted! I lose!\n");
				return;
			}

			printf("\n");
			printf("1. Hit\n");
			printf("2. Stand\n");
			printf("Please choose 1 or 2: ");
			fflush(stdout);

			scanf("%d", &choice);
			if (choice == 1)
			{
				strcpy(buffer, HIT);
				printf("Sending: %s\n", buffer);
				if (BUFFER_SIZE != (nwritten = write(clientSockFd, buffer, BUFFER_SIZE)))
					error("Error! Couldn't write to server");

				if (read_with_timeout(clientSockFd, buffer, tv_wait_rcv))
				{
					printf("I received: %s\n", buffer);
					my_hand_values[nmy] = get_value_id(buffer[0]);
					my_hand_suits[nmy] = get_suit_id(buffer[1]);
					++nmy;
				}
				else
					error("Error! Server didn't respond on time. Game aborted.\n");
			}
			else if (choice == 2)
			{
				strcpy(buffer, STAND);
				printf("Sending: %s\n", buffer);
				if (BUFFER_SIZE != (nwritten = write(clientSockFd, buffer, BUFFER_SIZE)))
					error("Error! Couldn't write to server");
				break;
			}
			else
				printf("Unrecognized choice. Choose again.\n");
		}

		if (read_with_timeout(clientSockFd, buffer, tv_wait_inf))
		{
			unsigned i;
			printf("I received: %s\n", buffer);
			for (i = 0; i < strlen(buffer); i += 2)
			{
				dealer_hand_values[ndealer] = get_value_id(buffer[i]);
				dealer_hand_suits[ndealer] = get_suit_id(buffer[i + 1]);
				++ndealer;
			}
		}
		else
			error("Error! Server didn't respond.\n");

		printf("\n");
		printf("My Hand: ");
		display_state(my_hand_values, my_hand_suits, nmy);
		printf("Dealer Hand: ");
		display_state(dealer_hand_values, dealer_hand_suits, ndealer);

		my_sum = calc_sum(my_hand_values, nmy);
		dealer_sum = calc_sum(dealer_hand_values, ndealer);

		if (dealer_sum > 21)
			printf("\nDealer busted! I win!\n");
		else if (my_sum == dealer_sum)
			printf("\nMe and the dealer have the same score. It's a push!\n");
		else if (my_sum < dealer_sum)
			printf("\nDealer has a higher score. I lose!\n");
		else
			printf("\nI have a higher score. I win!\n");
	}
	else
		printf("Server didn't respond on time. No game will be played\n");
}

int main(int argc, char** argv)
{
	char *ip_addr;
	int port;
	int clientSockFd;
	struct sockaddr_in serverAddress;

	if (argc != 3)
	{
		printf("Usage: ./server ip_address port_no\n");
		printf("Example ./server 127.0.0.1 2001\n");
		exit(0);
	}

	ip_addr = argv[1];
	port = atoi(argv[2]);

	clientSockFd = socket(AF_INET, SOCK_STREAM, 0);

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(ip_addr);
	serverAddress.sin_port = htons(port);
	memset(serverAddress.sin_zero, '\0', sizeof (serverAddress.sin_zero));

	if (0 > connect(clientSockFd, (const struct sockaddr *) &serverAddress, sizeof (serverAddress)))
		error("Error. Cannot connect to server.");
	else
	{
		char buffer[BUFFER_SIZE];
		int nwritten;

		printf("Connected to server %s:%d\n", ip_addr, port);
		strcpy(buffer, LETS_PLAY);
		printf("Sending: %s\n", buffer);
		if (BUFFER_SIZE != (nwritten = write(clientSockFd, buffer, BUFFER_SIZE)))
			error("Error! Couldn't write to server");

		play_game(clientSockFd);
		close(clientSockFd);
	}
	printf("\nGoodbye...\n");

	return (EXIT_SUCCESS);
}

