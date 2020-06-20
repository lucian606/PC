#include <stdio.h>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <arpa/inet.h>
#include "helpers.h"
#include <unordered_map>
#include <iostream>
#include <map>

using namespace std;

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno;
	int first_client;
	char buffer[BUFLEN];
	char word_to_guess[50];
	char guessed_letters[30];
	unordered_map <int, int> users; //HashMap file_descriptor-vieti;
	strcpy(word_to_guess, "temp");
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret, connected_clients = 0;
	socklen_t clilen;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret = listen(sockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd, &read_fds);
	FD_SET(0, & read_fds);
	fdmax = sockfd;

	while (1) {
		tmp_fds = read_fds; 
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {

				if (i == sockfd) {

					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");
					
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}

					if (connected_clients == 0) {
						first_client = newsockfd;
					} //Se retine primul client conectat

					users[newsockfd] = 3; //Numar de vieti
					connected_clients++;
					printf("S-a conectat un nou client pe socketul %d\n", newsockfd);

					if(strcmp(word_to_guess, "temp") != 0) {
						char notificare[100];
						string word_status = "";
						for(int j = 0; j < strlen(word_to_guess); j++) {
							if(guessed_letters[j] == 0) {
								word_status += "_";
							} else {
								word_status += word_to_guess[j];
							}
						}
						sprintf(notificare, "Welcome, here is the word: len: %ld\n%s", strlen(word_to_guess),word_status.c_str());
						n = send(newsockfd, notificare, strlen(notificare), 0);
						DIE(n < 0, "SENDING GAME");
					}
				} //SE FACE O CERERE DE CONECTARE DE LA CLIENT TCP

				else if (i == 0) {
					char command[30];
					scanf("%s", command);
					if(strcmp(command, "status") == 0) {
						printf ("Connected clients: %d\n", connected_clients); 
						continue;
					}
					else if(strcmp(command, "quit") == 0) {
						goto END;
					}
				}

				else { //SE PRIMESC DATE DE LA UN CLIENT TCP
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze

					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					if (n == 0) {
						// conexiunea s-a inchis
						connected_clients--;
						close(i);
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);
						if(i == first_client)
							goto END;
					} 
					else {                     

						cout << buffer << endl;

						if(strncmp(buffer, "set", 3) == 0) {
							if(i != first_client) {
								char eroare[50];
								strcpy(eroare, "YOU ARE NOT ALLOWED TO DO THAT");
								n = send(i, eroare, strlen(eroare), 0);
								DIE(n < 0, "SENDING ERROR");
								continue;
							}
							else {
								char tmp[20], word[50];
								sscanf(buffer, "%s%s", tmp, word);
								if(strlen(word) < 10 || strlen(word) > 25) {
									char eroare[50];
									strcpy(eroare, "WORD MUST HAVE BETWEEN 10-25 CHARS");
									n = send(i, eroare, strlen(eroare), 0);
									DIE(n < 0, "SENDING ERROR");
									continue;
								}
								if(strcmp(word_to_guess, "temp") != 0) {
									char eroare[50];
									strcpy(eroare, "YOU HAVE ALREADY CHOSEN A WORD");
									n = send(i, eroare, strlen(eroare), 0);
									DIE(n < 0, "SENDING ERROR");
									continue;
								}
								cout << "The word is: " << word << endl;
								strcpy(word_to_guess, word);
								for(int i = 0; i < strlen(word_to_guess); i++) {
									guessed_letters[i] = 0;
								}
							}
						}
						
						else if(strncmp(buffer, "guess", 5) == 0) {
							if(i == first_client) {
								char eroare[50];
								strcpy(eroare, "YOU ARE NOT ALLOWED TO DO THAT");
								n = send(i, eroare, strlen(eroare), 0);
								DIE(n < 0, "SENDING ERROR");
								continue;
							}
							else {
								char tmp[20], letter[50];
								sscanf(buffer, "%s%s", tmp, letter);
								if(strlen(letter) > 1) {
									char eroare[50];
									strcpy(eroare, "INVALID LETTER TO GUESS");
									n = send(i, eroare, strlen(eroare), 0);
									DIE(n < 0, "SENDING ERROR");
									continue;
								}
								else if(strcmp(word_to_guess, "temp") == 0) {
									char eroare[50];
									strcpy(eroare, "NO WORD WAS CHOSEN");
									n = send(i, eroare, strlen(eroare), 0);
									DIE(n < 0, "SENDING ERROR");
									continue;
								}
								int ok = 0;
								string letters_found = "Letter " + string(letter) + " is at: ";
								for(int i = 0; i < strlen(word_to_guess); i++) {
									if(word_to_guess[i] == letter[0]) {
										ok = 1; //LITERA GHICITA PRIMA OARA
										if(guessed_letters[i] == 1)
											ok = 2; //LITERA DEJA GHICITA
										guessed_letters[i] = 1;
										string pos = to_string(i);
										letters_found = letters_found + pos + " ";
									}
								}
								if(ok == 0) {
									char notificare[50];
									strcpy(notificare, "Wrong letter, you lose a life\n");
									users[i] = users[i] - 1;
									n = send(i, notificare, strlen(notificare), 0);
									DIE(n < 0, "SENDING NOTIFICATION");
									if(users[i] == 0) {
										strcpy(notificare, "YOU LOST ALL LIVES\n");
										n = send(i, notificare, strlen(notificare), 0);
										DIE(n < 0, "SENDING LOSE");
										connected_clients--;
										FD_CLR(i, &read_fds);
										close(i);
									}
									continue;
								}
								else if(ok == 1){
									char notificare[100];
									sprintf(notificare, "%s", letters_found.c_str());
									if(ok > 0) {
										for(int l = 4; l <= fdmax; l++) {
											if(FD_ISSET(l, &read_fds)) {
												n = send(l, notificare, strlen(notificare), 0);
												DIE(n < 0, "GAME OVER");
											}
										}
									}
									int is_game_over = 1;
									for(int k = 0; k < strlen(word_to_guess); k++) {
										if(guessed_letters[k] == 0) {
											is_game_over = 0;
											break;
										}
									}
									if(is_game_over) {
										char notificare[100];
										sprintf(notificare, "GAME OVER! The word was %s\n", word_to_guess);
										for(int l = 4; l <= fdmax; l++) {
											if(FD_ISSET(l, &read_fds)) {
												n = send(l, notificare, strlen(notificare), 0);
												DIE(n < 0, "GAME OVER");
											}
										}
										goto END;
									}
								}
								else if(ok == 2) {
									char notificare[100];
									sprintf(notificare, "LETTER ALREADY GUESSED\n");
									n = send(i, notificare, strlen(notificare), 0);
									DIE(n < 0, "ALREADY GUESS");
								}
							}
						}

						else if (strncmp(buffer, "status", 6) == 0) {
							if(i == first_client) {
								char eroare[50];
								strcpy(eroare, "YOU ARE NOT ALLOWED TO DO THAT");
								n = send(i, eroare, strlen(eroare), 0);
								DIE(n < 0, "SENDING ERROR");
								continue;
							}
							int lives = users[i];
							char notificare[50];
							sprintf(notificare, "Remaining lives: %d", lives);
							n = send(i , notificare, strlen(notificare), 0);
							DIE(n < 0, "SENDING LIVES");
							continue;
						}

					} //S-a primit o cerere
				}
			}
		}
	}
END:
	close(sockfd);
	return 0;
}