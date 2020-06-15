#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#include "helpers.h"

using namespace std;

void usage(char *file) {
  fprintf(stderr, "Usage: %s <id_client> <server_address> <server_port>\n",
          file);
  exit(0);
}

int number_of_words(char *str) {
  if (str == NULL) return 0;
  int words_count = 0;
  int len = 0;
  for (long unsigned int i = 0; i < strlen(str); i++) {
    if (str[i] == ' ')
      len = 0;
    else if (++len == 1)
      words_count++;
  }
  return words_count;
}  // Functie care numara cuvintele/argumentele dintr-un sir//

int main(int argc, char *argv[]) {
  int sockfd, n, ret, fdmax;
  int flag = 1;
  struct sockaddr_in serv_addr;
  char buffer[BUFLEN];
  char exist[] = "USER ALREADY EXISTS";
  char *recv_buff;

  if (argc <= 3) {
    usage(argv[0]);
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);  // Creez socketul clientului
  DIE(sockfd < 0, "Error creating tcp_client socket");

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(atoi(argv[3]));
  ret = inet_aton(argv[2], &serv_addr.sin_addr);
  DIE(ret == 0, "inet_aton");

  // Realizez conexiunea clinet server
  ret = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(ret < 0, "Error connecting to server");

  // Trimit id-ul userului pentru autentificarea la server
  strcpy(buffer, argv[1]);
  DIE(strlen(argv[1]) > 10, "ID must be maximum 10 characters long");
  argv[1][strlen(argv[1])] = '\0';
  n = send(sockfd, argv[1], strlen(argv[1]), 0);
  DIE(n < 0, "Error sending ID");

  // Dezactivez Nagle
  setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

  fd_set read_fds;
  fd_set tmp_fds;

  FD_ZERO(&read_fds);
  FD_ZERO(&tmp_fds);

  FD_SET(sockfd, &read_fds);
  FD_SET(0, &read_fds);
  fdmax = sockfd;

  while (1) {
    tmp_fds = read_fds;

    ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
    DIE(ret < 0, "select");

    if (FD_ISSET(0, &tmp_fds)) {
      // Se primeste o comanda de la tastatura
      memset(buffer, 0, BUFLEN);
      fgets(buffer, BUFLEN - 1, stdin);

      if (strncmp(buffer, "exit", 4) == 0) {
        break;
      }  // Daca e comanda de iesire

      if (number_of_words(buffer) > 3) {
        perror(
            "Invalid comand, try: subscribe <topic> <SF(1/0)>, unsubscribe "
            "<topic>, exit");
        continue;
      }  // Nu exista comenzi valide cu mai mult de trei argumente

      if (number_of_words(buffer) == 3) {
        // Daca are 3 argumente e posibil sa fie o comanda de subscribe
        char arg1[100], arg2[100], arg3[100];
        sscanf(buffer, "%s%s%s", arg1, arg2, arg3);  // Separ argumentele
        if (strcmp(arg1, "subscribe") == 0) {
          // In cazul in care e comanda subscribe verific restul
          // de argumente
          if (strlen(arg2) > 50) {
            perror("Topic name must be maximum 50 characters long.");
            continue;
          }  // Topic invalid

          if (strlen(arg3) == 1 && (arg3[0] >= '0' && arg3[0] <= '1')) {
            // SF valid, comanda e valida
            request_msg *sub_request =
                (request_msg *)malloc(sizeof(request_msg));
            sub_request->request_type = SUBSCRIBE;
            arg2[strlen(arg2)] = '\0';
            strcpy(sub_request->topic, arg2);
            sub_request->sf_type = arg3[0];
            // Trimit request de subscribe la server
            n = send(sockfd, (char *)sub_request, sizeof(request_msg), 0);
            DIE(n < 0, "send");
            free(sub_request);
            memset(buffer, 0, BUFLEN);
            // Primesc raspunsul de la server
            n = recv(sockfd, buffer, sizeof(tcp_msg), 0);
            sub_reply *reply = (sub_reply *)buffer;
            if (reply->reply_type == FAIL) {
              perror("You are already subscribed to this topic");
            } else {
              printf("Subscribed %s\n", arg2);
            }  // Afisam feedback-ul comenzii in functie de raspuns
            continue;
          } else {
            perror(
                "Invalid use of subscribe, try: subscribe <topic> <SF>, SF "
                "must be 0 or 1");
            continue;
          }  // SF introdus > 1
        } else {
          perror(
              "Invalid comand, try: subscribe <topic> <SF>, unsubscribe "
              "<topic>, exit");
          continue;
        }  // Nu e comanda de subscribe deci nu e valida
      }

      else if (number_of_words(buffer) == 2) {
        // Daca are doua argumente e posibil sa fie o comanda de unsubscribe
        char arg1[100], arg2[51];
        // Separ argumentele
        sscanf(buffer, "%s%s", arg1, arg2);
        if (strcmp(arg1, "unsubscribe") == 0) {
          // In cazul in care e comanda de unsubscribe
          request_msg *unsub_request =
              (request_msg *)malloc(sizeof(request_msg));
          unsub_request->request_type = UNSUBSCRIBE;
          strcpy(unsub_request->topic, arg2);
          unsub_request->sf_type = '1';
          // Trimit request de unsubscribe la server
          n = send(sockfd, (char *)unsub_request, sizeof(request_msg), 0);
          DIE(n < 0, "send");
          free(unsub_request);
          memset(buffer, 0, BUFLEN);
          // Primesc raspunsul de la server
          n = recv(sockfd, buffer, sizeof(tcp_msg), 0);
          sub_reply *reply = (sub_reply *)buffer;
          if (reply->reply_type == FAIL) {
            perror("You must be subscribed to this topic to unsubscribe");
          }  // Nu eram abonat la topic
          else {
            printf("Unsubscribed %s\n", arg2);
          }
          continue;
        } else {
          perror(
              "Invalid comand, try: subscribe <topic> <SF>, unsubscribe "
              "<topic>, exit");
          continue;
        }  // Nu e comanda de unsubscribe deci nu e valida
      } else {
        perror(
            "Invalid comand, try: subscribe <topic> <SF>, unsubscribe <topic>, "
            "exit");
        continue;
      }  // Comanda invalida
    } else if (FD_ISSET(sockfd, &tmp_fds)) {
      recv_buff = (char *)malloc(sizeof(tcp_msg));
      memset(recv_buff, 0, sizeof(tcp_msg));
      n = recv(sockfd, recv_buff, sizeof(tcp_msg), 0);

      if (n == 0) {
        perror("Server connection closed.");
        goto END;
      }  // Daca s-a inchis conexiunea cu serverul inchid clientul

      if (strcmp(recv_buff, exist) == 0) {
        perror("ID already taken, please pick another ID.");
        goto END;
      }  // Daca userul deja e conectat la server inchid clientul

      else {
        tcp_msg *received = (tcp_msg *)recv_buff;
        printf("%s:%hu - %s - %s - %s\n", received->ip, received->port,
               received->topic, data_types[received->data_type],
               received->content);
      }  // Primesc mesajul de la server si il afisez
    }
  }
END:
  free(recv_buff);
  close(sockfd);
  return 0;
}