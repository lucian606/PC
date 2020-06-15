#include <arpa/inet.h>
#include <float.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <climits>
#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "helpers.h"
#include "math.h"

using namespace std;

void usage(char *file) {
  fprintf(stderr, "Usage: %s server_port\n", file);
  exit(0);
}

typedef struct {
  char id[10];                         // Id-ul userului
  int file_descriptor;                 // File descriptorul userului
  queue<tcp_msg> unreceived_messages;  // Mesajele pe care le-a primit offline
  bool online;                         // Statusul utilizatorului
} user;                                // Structura pentru un user

typedef struct {
  char topic_name[50];           // Numele topicului
  vector<string> sf_users;       // Useri cu store&forward activ
  vector<string> regular_users;  // Useri cu store&forward inactiv
} topic;                         // Structura unui topic

int number_of_words(char *str) {
  if (str == NULL) return 0;
  int words_count = 0;
  int len = 0;
  long unsigned int i;
  for (i = 0; i < strlen(str); i++) {
    if (str[i] == ' ')
      len = 0;
    else if (++len == 1)
      words_count++;
  }
  return words_count;
}  // Functie care numara cuvintele/argumentele dintr-un sir

int main(int argc, char *argv[]) {
  int tcp_sockfd, udp_sockfd, newsockfd, portno, n, i, ret;
  int flag = 1;
  char buffer[CLIENT_BUFLEN];
  struct sockaddr_in tcp_serv_addr, udp_serv_addr, cli_addr;
  socklen_t udp_len = sizeof(sockaddr);
  socklen_t clilen;

  fd_set read_fds;  // Multimea de citire folosita in select()
  fd_set tmp_fds;   // Multime folosita temporar
  int fdmax;        // Valoare maxima fd din multimea read_fds

  unordered_map<string, user> users;   // Useri conectati la server
  unordered_map<int, string> ids_map;  // Id-urile pentru fiecare filedescriptor
  unordered_map<string, topic> topics;  // Topicurile active

  if (argc < 2) {
    usage(argv[0]);
  }

  // Se goleste multimea de descriptori de citire (read_fds) si multimea
  // Temporara (tmp_fds)
  FD_ZERO(&read_fds);
  FD_ZERO(&tmp_fds);

  udp_sockfd = socket(PF_INET, SOCK_DGRAM, 0);  // Creare udp socket
  DIE(udp_sockfd < 0, "Error creating UDP socket");

  tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);  // Creare tcp socket
  DIE(tcp_sockfd < 0, "Error creating TCP socket");

  portno = atoi(argv[1]);  // Parsez portul
  DIE(portno == 0, "Atoi failed");

  udp_msg *received_msg;

  memset((char *)&udp_serv_addr, 0,
         sizeof(udp_serv_addr));  // Completez headerul udp
  udp_serv_addr.sin_family = AF_INET;
  udp_serv_addr.sin_port = htons(portno);
  udp_serv_addr.sin_addr.s_addr = INADDR_ANY;

  memset((char *)&tcp_serv_addr, 0,
         sizeof(tcp_serv_addr));  // Completez headerul tcp
  tcp_serv_addr.sin_family = AF_INET;
  tcp_serv_addr.sin_port = htons(portno);
  tcp_serv_addr.sin_addr.s_addr = INADDR_ANY;

  // Fac legatura intre udp si server
  ret = bind(udp_sockfd, (struct sockaddr *)&udp_serv_addr,
             sizeof(struct sockaddr));
  DIE(ret < 0, "Error binding UDP socket");

  // Fac legatura intre tcp si server
  ret = bind(tcp_sockfd, (struct sockaddr *)&tcp_serv_addr,
             sizeof(struct sockaddr));
  DIE(ret < 0, "Error binding TCP socket");

  // Setez socketul serverului pentru a fi pasiv
  ret = listen(tcp_sockfd, MAX_CLIENTS);
  DIE(ret < 0, "Error listening TCP socket");

  // Se adauga noul file descriptor (socketul pe care se asculta conexiuni) in
  // multimea read_fds
  FD_SET(tcp_sockfd, &read_fds);
  FD_SET(udp_sockfd, &read_fds);
  FD_SET(0, &read_fds);
  fdmax = tcp_sockfd;

  while (1) {
    tmp_fds = read_fds;

    ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
    DIE(ret < 0, "select");

    for (i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &tmp_fds)) {
        if (i == 0) {
          memset(buffer, 0, BUFLEN);
          fgets(buffer, BUFLEN - 1, stdin);

          if (strncmp(buffer, "exit", 4) == 0) {
            goto CLOSE_CONNECTIONS;
          } else {
            fprintf(stderr, "Invalid comand, you can only use exit.\n");
            break;
          }
        }  // S-a trimis o comanda serverului de la tastatura.

        else if (i == udp_sockfd) {

           // S-a primit un mesaj de la un client UDP
          memset(buffer, 0, BUFLEN);  // Golesc bufferul
          char topic_name[50], ip[30];

          // Primesc mesajul
          n = recvfrom(udp_sockfd, buffer, BUFLEN, 0,
                       (struct sockaddr *)&udp_serv_addr, &udp_len);
          DIE(n < 0, "No message from UDP");

          tcp_msg *msg_to_send = (tcp_msg *)malloc(sizeof(tcp_msg));
          inet_ntop(AF_INET, &udp_serv_addr.sin_addr.s_addr, ip, 30);
          received_msg = (udp_msg *)buffer;
          strcpy(topic_name, received_msg->topic);
          strcpy(msg_to_send->topic, received_msg->topic);
          strcpy(msg_to_send->ip, ip);
          msg_to_send->port = htons(udp_serv_addr.sin_port);
          msg_to_send->data_type = received_msg->data_type;

          if (received_msg->data_type == 0) {
            int32_t value = (*(int32_t *)(received_msg->content + 1));
            value = ntohl(value);
            if (received_msg->content[0] == 1) {
              value = value * (-1);
            }
            sprintf(msg_to_send->content, "%d", value);
          }  // Daca am primit un int

          else if (received_msg->data_type == 1) {
            float value = ntohs(*(uint16_t *)received_msg->content);
            value = value / 100;
            sprintf(msg_to_send->content, "%.2f", value);
          }  // Daca am primit un short_real

          else if (received_msg->data_type == 2) {
            uint32_t merged_value = (*(uint32_t *)(received_msg->content + 1));
            merged_value = ntohl(merged_value);
            double value;
            int exponent = received_msg->content[5];
            double divisor = pow(10, exponent);
            value = (float)merged_value / (float)divisor;
            if (received_msg->content[0] == 1) value *= -1;
            sprintf(msg_to_send->content, "%lf", value);
          }  // Daca am primit un float

          else if (received_msg->data_type == 3) {
            strcpy(msg_to_send->content, received_msg->content);
          }  // Daca am primit un string

          // Daca nu am gasit topicul mesajului, adaug topicul in hash map
          if (topics.find(topic_name) == topics.end()) {
            topic new_topic;
            strcpy(new_topic.topic_name, topic_name);
            topics[topic_name] = new_topic;
          } else {
            topic messaged_topic = topics[topic_name];
            for (long unsigned int j = 0;
                 j < messaged_topic.regular_users.size(); j++) {
              int fd = users[(messaged_topic.regular_users[j])].file_descriptor;
              if (users[messaged_topic.regular_users[j]].online == true) {
                n = send(fd, (char *)msg_to_send, sizeof(tcp_msg), 0);
                DIE(n < 0, "ERROR SENDING MESSAGE");
              } // Trimit mesajul userilor online
            }

            // Trimit mesajul userilor cu S&F activ
            for (long unsigned int j = 0; j < messaged_topic.sf_users.size();
                 j++) {
              int fd = users[(messaged_topic.sf_users[j])].file_descriptor;
              if (users[messaged_topic.sf_users[j]].online == true) {
                n = send(fd, (char *)msg_to_send, sizeof(tcp_msg), 0);
                DIE(n < 0, "ERROR SENDING MESSAGE");
              } else {
                tcp_msg m = *msg_to_send;
                users[messaged_topic.sf_users[j]].unreceived_messages.push(m);
              } // Daca userul cu S&F nu e activ, ii pun mesajul in coada
            }
          }
          free(msg_to_send);
        }

        else if (i == tcp_sockfd) {
          // A venit o cerere de conexiune pe socketul inactiv (cel cu listen),
          // pe care serverul o accepta
          clilen = sizeof(cli_addr);
          newsockfd = accept(tcp_sockfd, (struct sockaddr *)&cli_addr, &clilen);
          DIE(newsockfd < 0, "accept");

          setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag,
                     sizeof(int)); // Dezactivez Nagle

          memset(buffer, 0, strlen(buffer));
          n = recv(newsockfd, buffer, sizeof(buffer), 0); // Primesc id-ul
          buffer[10] = '\0';

          bool already_connected = false;

          if (users.find(buffer) ==
              users.end()) {
            user new_user;
            strcpy(new_user.id, buffer);
            new_user.file_descriptor = newsockfd;
            new_user.online = true;
            users[buffer] = new_user;
            ids_map[newsockfd] = buffer;
          } // Daca nu exista userul, creez unul nou pe care il adaug in hash map

          // Daca este un user care se reconecteaza
          else if (users[buffer].online == false) {
            users[buffer].online = true;
            int old_fd = users[buffer].file_descriptor;
            users[buffer].file_descriptor = newsockfd;
            ids_map.erase(old_fd);
            ids_map[newsockfd] = buffer; // Updatez file descriptorul
            while (users[buffer].unreceived_messages.empty() == false) {
              tcp_msg *to_send = (tcp_msg *)malloc(sizeof(tcp_msg));
              char *buffer_to_send = (char *)malloc(sizeof(tcp_msg));
              memcpy(to_send, &users[buffer].unreceived_messages.front(),
                     sizeof(tcp_msg));
              memcpy(buffer_to_send, to_send, sizeof(tcp_msg));
              buffer_to_send[BUFLEN] = '\0';
              n = send(newsockfd, buffer_to_send, sizeof(tcp_msg), 0);
              DIE(n < 0, "Error sending message from client queue");
              users[buffer].unreceived_messages.pop();
              for(int x = 0; x < 255; x++)
                for(int y = 0; y < 255; y++)
                  for(int z= 0; z < 2; z++);
              free(buffer_to_send);
              free(to_send);
            } // Ii trimit mesajele din coada
          }

          else if (users[buffer].online == true) {
            already_connected = true;
          }  // Daca userul este deja online -> ID duplicat

          FD_SET(newsockfd, &read_fds);
          if (newsockfd > fdmax) {
            fdmax = newsockfd;
          } // Actualizez file descriptorul maxim

          if (already_connected == false) // Daca s-a conectat userul cu succes
            printf("New client %s connected from %s:%d.\n", buffer,
                   inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
          else {
            char exist[] = "USER ALREADY EXISTS";
            n = send(newsockfd, exist, strlen(exist), 0);
            DIE(n < 0, "Error sending existed notification");
            FD_CLR(newsockfd, &read_fds);
            close(newsockfd);
          } // Daca e un cu id duplicat, il notific si inchid conexiunea cu el
        } else {
          // S-au primit date pe unul din socketii de client,
          // Asa ca serverul trebuie sa le receptioneze
          memset(buffer, 0, sizeof(tcp_msg));
          n = recv(i, buffer, sizeof(buffer), 0);
          DIE(n < 0, "recv");

          if (n == 0) {
            // Conexiunea s-a inchis
            cout << "Client " << ids_map.at(i) << " disconnected.\n";
            users[ids_map.at(i)].online = false;
            close(i);

            // Se scoate din multimea de citire socketul inchis
            FD_CLR(i, &read_fds);
          } else {
            // S-a primit o comanda de subscribe/unsubscribe de la un client TCP
            request_msg *request = (request_msg *)buffer;
            
            if (request->request_type == SUBSCRIBE) {
              // Daca e o cerere de subscribe
              char subscribed_topic[50];
              sub_reply *reply = (sub_reply *)malloc(sizeof(sub_reply));
              reply->reply_type = SUCCESS;
              strcpy(subscribed_topic, request->topic);
              if (topics.find(subscribed_topic) == topics.end()) {
                // Creez topicul daca acesta nu exista
                topic new_topic;
                strcpy(new_topic.topic_name, subscribed_topic);
                if (request->sf_type == '1') {
                  new_topic.sf_users.push_back(ids_map[i]);
                } else {
                  new_topic.regular_users.push_back(ids_map[i]);
                } // Adaug userul in multimea corespunzatoare S&F-ului ales
                topics[subscribed_topic] = new_topic;
              } else {
                // Daca topicul exista deja
                topic subbed = topics[subscribed_topic];

                // Verific daca userul este deja abonat
                for (long unsigned int j = 0; j < subbed.regular_users.size();
                     j++) {
                  if (subbed.regular_users[j].compare(ids_map.at(i)) == 0) {
                    reply->reply_type = FAIL;
                    break;
                  }
                }

                for (long unsigned int j = 0; j < subbed.sf_users.size(); j++) {
                  if (subbed.sf_users[j].compare(ids_map.at(i)) == 0) {
                    reply->reply_type = FAIL;
                    break;
                  }
                }

                if (reply->reply_type == SUCCESS) {
                // Daca abonarea se realizeaza cu succes
                  if (request->sf_type == '1') {
                    topics[subscribed_topic].sf_users.push_back(ids_map[i]);
                  } else {
                    topics[subscribed_topic].regular_users.push_back(
                        ids_map[i]);
                  }
                } // Adaug utilizatorul in multimea corespunzatoare optiunii de S&F
              }
              
              // Trimit raspuns la client sa afle cum a decurs abonarea
              n = send(i, (char *)reply, sizeof(sub_reply), 0);
              DIE(n < 0, "Error sending subscribe reply");
              free(reply);
            }

            else if (request->request_type == UNSUBSCRIBE) {
              // Daca este o crere de unsubscribe
              char unsubscribed_topic[50];
              sub_reply *reply = (sub_reply *)malloc(sizeof(sub_reply));
              reply->reply_type = FAIL; // Momentan nu am gasit utilizatorul
              strcpy(unsubscribed_topic, request->topic);
              topic unsubbed = topics[unsubscribed_topic];

              // Caut utilizatorul in multimea de subscriberi a topicului
              for (long unsigned int j = 0; j < unsubbed.regular_users.size();
                   j++) {
                if (unsubbed.regular_users[j].compare(ids_map.at(i)) == 0) {
                  topics[unsubscribed_topic].regular_users.erase(
                      topics[unsubscribed_topic].regular_users.begin() + j);
                  reply->reply_type = SUCCESS;
                  break;
                }
              }
              
              for (long unsigned int j = 0; j < unsubbed.sf_users.size(); j++) {
                if (unsubbed.sf_users[j].compare(ids_map.at(i)) == 0) {
                  topics[unsubscribed_topic].sf_users.erase(
                      topics[unsubscribed_topic].sf_users.begin() + j);
                  reply->reply_type = SUCCESS;
                  break;
                }
              }

              // Daca utilizatorul nu a fost gasit inseamna ca el nu era abonat
              n = send(i, (char *)reply, sizeof(sub_reply), 0);
              DIE(n < 0, "Error sending unsubscribe reply");
              free(reply);
            }
          }
        }
      }
    }
  }
CLOSE_CONNECTIONS:
  for (int i = fdmax; i >= 0; i--) {
    if (FD_ISSET(i, &read_fds)) {
      n = close(i);
      DIE(n < 0, "error closing socket");
    }
  } // S-a primit comanda de exit, deci inchid toate conexiunile
  return 0;
}