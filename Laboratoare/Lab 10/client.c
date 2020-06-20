#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

int main(int argc, char *argv[])
{
    char *message;
    char *response;
    int sockfd;


    sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
        
    // Ex 1.1: GET dummy from main server
    
    message = compute_get_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/dummy", NULL, NULL, 0);
    puts(message);
    
    // Ex 1.2: POST dummy and print response from main server
    
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    puts(response);
    
    message = compute_post_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/dummy", "application/x-www-form-urlencoded", NULL, 0, NULL, 0);
    puts(message);
    // Ex 2: Login into main server
    
    char **credidentials = malloc(2 * sizeof(char *));
    credidentials[0] = malloc(sizeof(char) * 30);
    strcpy(credidentials[0], "username=student");
    credidentials[1] = malloc(sizeof(char) * 30);
    strcpy(credidentials[1], "password=student");
    message = compute_post_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/auth/login", "application/x-www-form-urlencoded", credidentials, 2, NULL, 0);
    
    puts(message);
    // Ex 3: GET weather key from main server
    
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    puts(response);
    char** keys = malloc(sizeof(char *));
    keys[0] = malloc(sizeof(char) * 100);
    strcpy(keys[0], "connect.sid=s%3Arz-SMT4e4tuada2au5r1JA3u80nT5gvt.xQ3s2gMHiYl%2B6H1lzafxisibM2Hl5j%2FD6spFyFXNax0");

    message = compute_get_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/weather/key", NULL, keys, 1);
    puts(message);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    puts(response);    

    // Ex 4: GET weather data from OpenWeather API
    
    // Ex 5: POST weather data for verification to main server
    
    // Ex 6: Logout from main server

    message = compute_get_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/auth/logout", NULL, NULL, 0);
    puts(message);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    puts(response);

    // BONUS: make the main server return "Already logged in!"

    // free the allocated data at the end!

    free(message);
    free(response);

    return 0;
}
