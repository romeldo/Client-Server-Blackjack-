
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <pthread.h>


int card_values[52];
int card_suits[52];
int ncard;
pthread_mutex_t card_mutex;

int players_hand_values[MAX_PLAYERS][20], dealers_hand_values[MAX_PLAYERS][20];
int players_hand_suits[MAX_PLAYERS][20], dealers_hand_suits[MAX_PLAYERS][20];
int nplayers[MAX_PLAYERS], ndealers[MAX_PLAYERS];
int clientSockFds[MAX_PLAYERS];

void init_cards()
{
	int i;

	for (i = 0; i < 52; ++i)
	{
		card_values[i] = (i % 13) + 1;
		card_suits[i] = i / 13;
	}

	srand(time(NULL));
	/* srand(1); */
	for (i = 0; i < 52; ++i)
	{
		int cv, cs;
		int j = rand() % 52;

		cv = card_values[i];
		card_values[i] = card_values[j];
		card_values[j] = cv;

		cs = card_suits[i];
		card_suits[i] = card_suits[j];
		card_suits[j] = cs;
	}

	ncard = 0;
}

void* play_game_one(void *data)
{
	int id = (int) data;
	char buffer[BUFFER_SIZE];
	int nwritten;
	int *player_hand_values = players_hand_values[id], *dealer_hand_values = dealers_hand_values[id];
	int *player_hand_suits = players_hand_suits[id], *dealer_hand_suits = dealers_hand_suits[id];
	int i;

	nplayers[id] = 0;
	ndealers[id] = 0;
	pthread_mutex_lock(&card_mutex);
	{
		for (i = 0; i < 2; ++i)
		{
			player_hand_values[nplayers[id]] = card_values[ncard];
			player_hand_suits[nplayers[id]] = card_suits[ncard];
			++ncard;
			++nplayers[id];
		}

		dealer_hand_values[ndealers[id]] = card_values[ncard];
		dealer_hand_suits[ndealers[id]] = card_suits[ncard];
		++ncard;
		++ndealers[id];
	}
	pthread_mutex_unlock(&card_mutex);

	buffer[0] = value_codes[player_hand_values[0]];
	buffer[1] = suit_codes[player_hand_suits[0]];
	buffer[2] = value_codes[player_hand_values[1]];
	buffer[3] = suit_codes[player_hand_suits[1]];
	buffer[4] = value_codes[dealer_hand_values[0]];
	buffer[5] = suit_codes[dealer_hand_suits[0]];
	buffer[6] = 0;

	if (BUFFER_SIZE != (nwritten = write(clientSockFds[id], buffer, BUFFER_SIZE)))
	{
		printf("Error! Couldn't write to client %d. Client will be disconnected.\n", clientSockFds[id]);
		close(clientSockFds[id]);
		clientSockFds[id] = 0;
		return NULL;
	}

	/* while player wishes to receive more cards */
	while (TRUE)
	{
		if (read_with_timeout(clientSockFds[id], buffer, tv_wait_rcv))
		{
			printf("\n");
			printf("Player %d Hand: ", clientSockFds[id]);
			display_state(player_hand_values, player_hand_suits, nplayers[id]);
			printf("Dealer Hand with player %d: ", clientSockFds[id]);
			display_state(dealer_hand_values, dealer_hand_suits, ndealers[id]);

			printf("I received from player %d: %s\n", clientSockFds[id], buffer);

			if (strcmp(buffer, HIT) == 0)
			{
				pthread_mutex_lock(&card_mutex);
				{
					player_hand_values[nplayers[id]] = card_values[ncard];
					player_hand_suits[nplayers[id]] = card_suits[ncard];

					buffer[0] = value_codes[player_hand_values[nplayers[id]]];
					buffer[1] = suit_codes[player_hand_suits[nplayers[id]]];
					buffer[2] = 0;

					++ncard;
					++nplayers[id];
				}
				pthread_mutex_unlock(&card_mutex);

				if (BUFFER_SIZE != (nwritten = write(clientSockFds[id], buffer, BUFFER_SIZE)))
				{
					printf("Error! Couldn't write to player %d.\n", clientSockFds[id]);
					close(clientSockFds[id]);
					clientSockFds[id] = 0;
					return NULL;
				}

				if (calc_sum(player_hand_values, nplayers[id]) > 21)
				{
					printf("Player %d busted. Dealer wins.\n", clientSockFds[id]);
					close(clientSockFds[id]);
					clientSockFds[id] = 0;
					return NULL;
				}
				else if (calc_sum(player_hand_values, nplayers[id]) == 21)
					break;
			}
			else if (strcmp(buffer, STAND) == 0)
				break;
			else
			{
				printf("Player %d didn't respond with a known command. Player will be disconnected.\n", clientSockFds[id]);
				close(clientSockFds[id]);
				clientSockFds[id] = 0;
				return NULL;
			}
		}
		else
		{
			printf("Player %d didn't respond on time. Player will be disconnected.\n", clientSockFds[id]);
			close(clientSockFds[id]);
			clientSockFds[id] = 0;
			return NULL;
		}
	}

	return NULL;
}

void play_game_all(int nclients)
{
	int id;
	pthread_t threads[MAX_PLAYERS];

	init_cards();

	for (id = 0; id < nclients; ++id)
		pthread_create(&threads[id], NULL, play_game_one, (void*) id);

	for (id = 0; id < nclients; ++id)
		pthread_join(threads[id], NULL);

	for (id = 0; id < MAX_PLAYERS; ++id)
	{
		if (clientSockFds[id] > 0)
		{
			int i = 0;
			int nwritten;
			char buffer[BUFFER_SIZE];
			int player_sum, dealer_sum;
			int *player_hand_values = players_hand_values[id], *dealer_hand_values = dealers_hand_values[id];
			int *player_hand_suits = players_hand_suits[id], *dealer_hand_suits = dealers_hand_suits[id];

			while (calc_sum(dealer_hand_values, ndealers[id]) < 17)
			{
				dealer_hand_values[ndealers[id]] = card_values[ncard];
				dealer_hand_suits[ndealers[id]] = card_suits[ncard];
				buffer[i++] = value_codes[dealer_hand_values[ndealers[id]]];
				buffer[i++] = suit_codes[dealer_hand_suits[ndealers[id]]];
				++ncard;
				++ndealers[id];
			}
			buffer[i] = 0;

			printf("\n");
			printf("Player %d Hand: ", clientSockFds[id]);
			display_state(player_hand_values, player_hand_suits, nplayers[id]);
			printf("Dealer %d Hand: ", clientSockFds[id]);
			display_state(dealer_hand_values, dealer_hand_suits, ndealers[id]);

			if (BUFFER_SIZE != (nwritten = write(clientSockFds[id], buffer, BUFFER_SIZE)))
			{
				printf("Error! Couldn't write to player %d.\n", clientSockFds[id]);
				close(clientSockFds[id]);
				return;
			}

			player_sum = calc_sum(player_hand_values, nplayers[id]);
			dealer_sum = calc_sum(dealer_hand_values, ndealers[id]);

			if (dealer_sum > 21)
				printf("\nDealer busted! Player %d wins!\n", clientSockFds[id]);
			else if (player_sum == dealer_sum)
				printf("\nPlayer %d and dealer have the same score. It's a push!\n", clientSockFds[id]);
			else if (player_sum < dealer_sum)
				printf("\nDealer has a higher score than player %d. Dealer wins!\n", clientSockFds[id]);
			else
				printf("\nPlayer %d has a higher score. Player wins!\n", clientSockFds[id]);
		}
	}
}

int main(int argc, char** argv)
{
	int i;

	for (i = 0; i < MAX_PLAYERS; ++i)
		clientSockFds[i] = 0;

	for (i = 0; i < argc - 1; ++i)
		clientSockFds[i] = atoi(argv[i + 1]);

	play_game_all(argc - 1);

	return (EXIT_SUCCESS);
}

