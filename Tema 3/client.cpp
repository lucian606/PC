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
#include <iostream>
#include "nlohmann/json.hpp"
#include <string>
#include <string.h>

#define SERVER_ADDRESS "3.8.116.10"
#define HOST_NAME "ec2-3-8-116-10.eu-west-2.compute.amazonaws.com"
#define BAD_REQUEST "HTTP/1.1 400 Bad Request"
#define OK "HTTP/1.1 200 OK"
#define CREATED_OK "HTTP/1.1 201 Created"
#define TOO_MANY_REQUESTS "HTTP/1.1 429 Too Many Requests"

using namespace std;
using json = nlohmann::json;

string get_json_from_message(string message) {
			string start_delim = "{";
			string end_delim ="}";
			if(message.find('{') == std::string::npos || message.find('}') == std::string::npos)
				return "";
			int first_delim_pos = message.find(start_delim);
			int end_first_delim_pos = first_delim_pos + start_delim.length();
			int last_delim_pos = message.find_first_of(end_delim, end_first_delim_pos);
			string payload = message.substr(first_delim_pos, last_delim_pos);
			return payload;
}	//Extracts the json from the server response

string get_booklist_from_message(string message) {
			string start_delim = "[";
			string end_delim ="]";
			int first_delim_pos = message.find(start_delim);
			int end_first_delim_pos = first_delim_pos + start_delim.length();
			int last_delim_pos = message.find_first_of(end_delim, end_first_delim_pos);
			string payload = message.substr(first_delim_pos, last_delim_pos);
			return payload;
}	//Extracts the book list from the response for the get_books command

int hasLetters(char s[]) {
    for(unsigned int i = 0; i < strlen(s); i++) {
        if(s[i] < '0' || s[i] > '9')
            return 1;
    }
    return 0;
}	//Checks if the string has a letter

int main(int argc, char *argv[])
{
    char *message;
    char *response;
    char ip[30], command[30], host[100];
	string cookie_str = "Empty";
	string token_str = "Empty";
    strcpy(ip, SERVER_ADDRESS);
    strcpy(host, HOST_NAME);

    int sockfd;

    while (1) {
        
		scanf("%s", command);

		if (strcmp(command, "register") == 0) {
			sockfd = open_connection(ip, 8080, AF_INET, SOCK_STREAM, 0);
    		DIE(sockfd < 0, "Error opening connection");

			//If there is a cookie it means we are already logged in
			if (cookie_str.compare("Empty") != 0) {
				cout << "Can't register while logged in." << endl;
				close(sockfd);
				continue;
			}

			char username[100], pass[100], url[100], type[100];
			
			//Displaying the interface and getting the credidentials
			printf("register\n");
			printf("username=");
			getchar();
			cin.getline(username, sizeof(username));
			printf("password=");
			scanf("%s",pass);
			
			if(strlen(username) == 0 || strlen(pass) == 0) {
				cout << "Error empty username/pass" << endl;
				close(sockfd);
				continue;
			}


			strcpy(url, "/api/v1/tema/auth/register");
			strcpy(type, "application/json");

			//Creating the json with credidentials and passing it as a string
			//to the post request.
			json j;
			j["username"] = username;
			j["password"] = pass;

			string j_as_str = j.dump();

            message = compute_post_request(host, url, type, j_as_str.c_str(), NULL, 0);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			string response_as_string(response);

			//Getting the response type
			string first_token = response_as_string.substr(0, response_as_string.find("\r\n"));

			//Check if too many requests
			if (strcmp(first_token.c_str(), TOO_MANY_REQUESTS) == 0) {
				cout << "Too many requests, please try again later." << endl;
				free(message);
				free(response);
				close(sockfd);
				continue;
			}

			//Check if failure
			if (strcmp(first_token.c_str(), CREATED_OK) != 0) {
				string json_as_string = get_json_from_message(response_as_string);
				
				if(json_as_string.length() == 0) {
					cout << "An error has occurred, here is the server response:" << endl;
					cout << response << endl;
					free(message);
					free(response);
					close(sockfd);
					continue;
				} //If the response from server doesn't have a json

				json json_token = json::parse(json_as_string);
				string received_token = json_token["error"].dump();
				
				//Displaying the error message and the cause for it
				cout << "Register failed: " << received_token << endl;
				free(message);
				free(response);
				close(sockfd);
				continue;
			}

			//If success
			else {
				//Displaying the success message
				cout << "Register done." << endl;
				free(message);
				free(response);
				close(sockfd);
				continue;
			}
		}

    	else if (strcmp(command, "login") == 0) {
			sockfd = open_connection(ip, 8080, AF_INET, SOCK_STREAM, 0);
    		DIE(sockfd < 0, "Error opening connection");

			//If there is a cookie it means we are already logged in
			if(cookie_str.compare("Empty") != 0) {
				cout << "You are already logged in." << endl;
				close(sockfd);
				continue;
			}

        	char username[100], pass[100], url[100], type[100];

			//Displaying the interface and getting the credidentials
			printf("login\n");
			getchar();
        	printf("username=");
			cin.getline(username, sizeof(username));
        	printf("password=");
        	scanf("%s",pass);
            strcpy(url, "/api/v1/tema/auth/login");
            strcpy(type, "application/json");

			//Creating the json with credidentials and passing it as a string
			//to the post request.			
			json j;
			j["username"] = username;
			j["password"] = pass;

			string j_as_str = j.dump();

            message = compute_post_request(host, url, type, j_as_str.c_str(), NULL, 0);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
			string response_as_string(response);

			//Getting the cookie from the response if the login was done with success.
			string start_delim = "Set-Cookie: ";
			string end_delim =";";
			int first_delim_pos = response_as_string.find(start_delim);
			int end_first_delim_pos = first_delim_pos + start_delim.length();
			int last_delim_pos = response_as_string.find_first_of(end_delim, end_first_delim_pos);

			//Getting the response type
			auto first_token = response_as_string.substr(0, response_as_string.find("\r\n"));
			
			//Check if too many requests
			if (strcmp(first_token.c_str(), TOO_MANY_REQUESTS) == 0) {
				cout << "Too many requests, please try again later." << endl;
				free(message);
				free(response);
				close(sockfd);
				continue;
			}

			//Checking if failure and displaying the error message if so
			if(strcmp(first_token.c_str(), OK) != 0) {
				string json_as_string = get_json_from_message(response_as_string);

				if(json_as_string.length() == 0) {
					cout << "An error has occurred, here is the server response:" << endl;
					cout << response << endl;
					free(message);
					free(response);
					close(sockfd);
					continue;
				} //If the response from server doesn't have a json

				json json_token = json::parse(json_as_string);
				string received_token = json_token["error"].dump();
				cout << "Login failed: " << received_token << endl;
				free(message);
				free(response);
				close(sockfd);
			}

			//Checking if success and storing the cookie
			else {
				cout << "Login done with success." << endl;
				cookie_str = response_as_string.substr(end_first_delim_pos, last_delim_pos-end_first_delim_pos);
				free(message);
				free(response);
				close(sockfd);
			}
		}

		else if (strcmp(command, "enter_library") == 0) {
			
			sockfd = open_connection(ip, 8080, AF_INET, SOCK_STREAM, 0);
    		DIE(sockfd < 0, "Error opening connection");

			//If there is a cookie it means we are already logged in
			if(token_str.compare("Empty") != 0) {
				cout << "You have already entered the library." << endl;
				close(sockfd);
				continue;
			}

			char url[100];
			strcpy(url, "/api/v1/tema/library/access");

			message = compute_get_request(host, url, NULL, cookie_str.c_str());
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			string response_as_string(response);

			//Getting de response type
			string first_token = response_as_string.substr(0, response_as_string.find("\r\n"));

			
			//Check if too many requests
			if (strcmp(first_token.c_str(), TOO_MANY_REQUESTS) == 0) {
				cout << "Too many requests, please try again later." << endl;
				free(message);
				free(response);
				close(sockfd);
				continue;
			}

			//Check if failure (invalid/nonexistent cookie)
			if(strcmp(first_token.c_str(), OK) != 0) {
				string json_as_string = get_json_from_message(response_as_string);

				if(json_as_string.length() == 0) {
					cout << "An error has occurred, here is the server response:" << endl;
					cout << response << endl;
					free(message);
					free(response);
					close(sockfd);
					continue;
				} //If the response from server doesn't have a json

				json json_token = json::parse(json_as_string);
				string received_token = json_token["error"].dump();
				cout << "Failed to enter library. " << received_token << endl;
				free(message);
				free(response);
				close(sockfd);
				continue;
			}

			//Getting the json from message (cookie)
			string payload = get_json_from_message(response_as_string);

			//Converting the cookie to a json format and getting the JWT token
			json json_token = json::parse(payload);
			string received_token = json_token["token"].dump();
			

			//Removing the "" from the JWT token and storing it
			string start_delim = "\"";
			string end_delim = "\"";
			int first_delim_pos = received_token.find(start_delim);
			int end_first_delim_pos = first_delim_pos + start_delim.length();
			int last_delim_pos = received_token.find_first_of(end_delim, end_first_delim_pos);
			token_str = received_token.substr(end_first_delim_pos, last_delim_pos - end_delim.length());

			//Displaying the success message
			cout << "Done entering the library." << endl;
			close(sockfd);
		}

		else if (strcmp(command, "get_books") == 0) {

			sockfd = open_connection(ip, 8080, AF_INET, SOCK_STREAM, 0);
    		DIE(sockfd < 0, "Error opening connection");

			char url[100];
			strcpy(url, "/api/v1/tema/library/books");

			message = compute_get_request_with_token(host, url, NULL, token_str.c_str());
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			string response_as_string(response);

			//Getting the response type
			string first_token = response_as_string.substr(0, response_as_string.find("\r\n"));
			
			//Check if too many requests
			if (strcmp(first_token.c_str(), TOO_MANY_REQUESTS) == 0) {
				cout << "Too many requests, please try again later." << endl;
				free(message);
				free(response);
				close(sockfd);
				continue;
			}

			//Check if failure(invalid/nonexistent JWT)
			if(strcmp(first_token.c_str(), OK) != 0) {
				string json_as_string = get_json_from_message(response_as_string);

				if(json_as_string.length() == 0) {
					cout << "An error has occurred, here is the server response:" << endl;
					cout << response << endl;
					free(message);
					free(response);
					close(sockfd);
					continue;
				} //If the response from server doesn't have a json

				json json_token = json::parse(json_as_string);
				string received_token = json_token["error"].dump();
				cout << "Failed to get books. " << received_token << endl;
				free(message);
				free(response);
				close(sockfd);
				continue;
			}

			//Displaying success message
			cout << "Here is your booklist:\n" << get_booklist_from_message(response_as_string) << endl;
			close(sockfd);
		}

		else if (strcmp(command, "add_book") == 0) {

			sockfd = open_connection(ip, 8080, AF_INET, SOCK_STREAM, 0);
    		DIE(sockfd < 0, "Error opening connection");

			char url[100];
			strcpy(url, "/api/v1/tema/library/books");

			char type[30];
			strcpy(type, "application/json");

			char title[100], author[100], genre[100], publisher[100], page_number[100];
			int page_count;

			json j;
			
			//Displaying the interface and getting the book info
			printf("add_book\n");
			printf("title=");
			getchar();
			cin.getline(title, sizeof(title));
			printf("author=");
			cin.getline(author, sizeof(author));
			printf("genre=");
			cin.getline(genre, sizeof(genre));
			printf("publisher=");
			cin.getline(publisher, sizeof(publisher));
			printf("page_count=");
			
			//Checking if page_count is a number
			scanf("%s", page_number);
			if(hasLetters(page_number)) {
				cout << "Invalid page number." << endl;
				close(sockfd);
				continue;
			}

			page_count = atoi(page_number);

			if(page_count < 0) {
				cout << "Page count must be possitive." << endl;
				close(sockfd);
				continue;
			}

			//Creating the json with the book details
			j["title"] = title;
			j["author"] = author;
			j["genre"] = genre;
			j["page_count"] = page_count;
			j["publisher"] = publisher;

			//Converting the json as string and sending it in the request
			string j_as_str = j.dump();
			message = compute_post_request_with_token(host, url, type, j_as_str.c_str(), token_str.c_str());
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			string response_as_string(response);

			//Getting the response type
			string first_token = response_as_string.substr(0, response_as_string.find("\r\n"));

			//Check if too many requests
			if (strcmp(first_token.c_str(), TOO_MANY_REQUESTS) == 0) {
				cout << "Too many requests, please try again later." << endl;
				free(message);
				free(response);
				close(sockfd);
				continue;
			}

			//If failure (nonexistent/invalid JWT)
			if(strcmp(first_token.c_str(), OK) != 0) {
				string json_as_string = get_json_from_message(response_as_string);

				if(json_as_string.length() == 0) {
					cout << "An error has occurred, here is the server response:" << endl;
					cout << response << endl;
					free(message);
					free(response);
					close(sockfd);
					continue;
				} //If the response from server doesn't have a json

				json json_token = json::parse(json_as_string);
				string received_token = json_token["error"].dump();
				cout << "Failed to add book." << received_token << endl;
				free(message);
				free(response);
				close(sockfd);
				continue;
			}
			
			//If success display message
			cout << "Book added successfully." << endl;
			close(sockfd);
		}

		else if (strcmp(command, "get_book") == 0) {

			sockfd = open_connection(ip, 8080, AF_INET, SOCK_STREAM, 0);
    		DIE(sockfd < 0, "Error opening connection");

			char url[100];
			strcpy(url, "/api/v1/tema/library/books/");

			char type[30];
			strcpy(type, "application/json");

			char id_read[100];
			int id;
			
			//Displaying the interface and getting the id
			printf("get_book\n");
			printf("id=");


			scanf("%s", id_read);
			if(hasLetters(id_read)) {
				cout << "Invalid book ID." << endl;
				close(sockfd);
				continue;
			}

			id = atoi(id_read);

			if(id < 0) {
				cout << "Id must be possitive." << endl;
				close(sockfd);
				continue;
			}

			//Converting the id to string and concatenating it to url
			char id_str[30];
			strcpy(id_str,to_string(id).c_str());
			strcat(url,id_str);
			
			message = compute_get_request_with_token(host, url, NULL, token_str.c_str());
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			string response_as_string(response);

			//Getting the response type
			string first_token = response_as_string.substr(0, response_as_string.find("\r\n"));
			
			//Check if too many requests
			if (strcmp(first_token.c_str(), TOO_MANY_REQUESTS) == 0) {
				cout << "Too many requests, please try again later." << endl;
				free(message);
				free(response);
				close(sockfd);
				continue;
			}
			
			//If failure (nonexistent/invalid JWT or the book doesn't exist)
			if(strcmp(first_token.c_str(), OK) != 0) {
				string json_as_string = get_json_from_message(response_as_string);

				if(json_as_string.length() == 0) {
					cout << "An error has occurred, here is the server response:" << endl;
					cout << response << endl;
					free(message);
					free(response);
					close(sockfd);
					continue;
				} //If the response from server doesn't have a json

				json json_token = json::parse(json_as_string);
				string received_token = json_token["error"].dump();
				cout << "Failed to get book." << received_token << endl;
				free(message);
				free(response);
				close(sockfd);
				continue;
			}

			//Displaying success message
			cout << "Here is your book:\n" << get_booklist_from_message(response_as_string) << endl;
			close(sockfd);
		}

		else if (strcmp(command, "delete_book") == 0) {
			
			sockfd = open_connection(ip, 8080, AF_INET, SOCK_STREAM, 0);
    		DIE(sockfd < 0, "Error opening connection");

			char url[100];
			strcpy(url, "/api/v1/tema/library/books/");

			//Displaying the interface and getting the id
			char id_read[100];
			int id;
			getchar();
			printf("delete_book\n");
			printf("id=");
			
			scanf("%s", id_read);
			if(hasLetters(id_read)) {
				cout << "Invalid book ID." << endl;
				close(sockfd);
				continue;
			}

			id = atoi(id_read);

			if(id < 0) {
				cout << "Id must be possitive." << endl;
				close(sockfd);
				continue;
			}

			if(id < 0) {
				cout << "Id must be possitive." << endl;
				close(sockfd);
				continue;
			}

			//Converting the id to string and concatenating it to the url
			char id_str[30];
			strcpy(id_str,to_string(id).c_str());
			strcat(url,id_str);

			
			message = compute_delete_request_with_token(host, url, NULL, token_str.c_str());
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			string response_as_string(response);

			//Getting the response type
			string first_token = response_as_string.substr(0, response_as_string.find("\r\n"));
			
			//Check if too many requests
			if (strcmp(first_token.c_str(), TOO_MANY_REQUESTS) == 0) {
				cout << "Too many requests, please try again later." << endl;
				free(message);
				free(response);
				close(sockfd);
				continue;
			}
			
			//If failure (nonexistent/invalid JWT or the book doesn't exist)
			if(strcmp(first_token.c_str(), OK) != 0) {
				string json_as_string = get_json_from_message(response_as_string);

				if(json_as_string.length() == 0) {
					cout << "An error has occurred, here is the server response:" << endl;
					cout << response << endl;
					free(message);
					free(response);
					close(sockfd);
					continue;
				} //If the response from server doesn't have a json

				json json_token = json::parse(json_as_string);
				string received_token = json_token["error"].dump();
				cout << "Failed to delete book." << received_token << endl;
				free(message);
				free(response);
				close(sockfd);
				continue;
			}

			//Displaying success message
			cout << "Book deleted." << endl;
			close(sockfd);
		}

		else if (strcmp(command, "logout") == 0) {
			
			sockfd = open_connection(ip, 8080, AF_INET, SOCK_STREAM, 0);
    		DIE(sockfd < 0, "Error opening connection");
			
			char url[100];
			strcpy(url, "/api/v1/tema/auth/logout");
			
			message = compute_get_request(host, url, NULL, cookie_str.c_str());
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			string response_as_string(response);

			//Getting the response type
			string first_token = response_as_string.substr(0, response_as_string.find("\r\n"));
			
			//Check if too many requests
			if (strcmp(first_token.c_str(), TOO_MANY_REQUESTS) == 0) {
				cout << "Too many requests, please try again later." << endl;
				free(message);
				free(response);
				close(sockfd);
				continue;
			}

			//If failure (nonexistent/invalid cookie)
			if(strcmp(first_token.c_str(), OK) != 0) {
				string json_as_string = get_json_from_message(response_as_string);

				if(json_as_string.length() == 0) {
					cout << "An error has occurred, here is the server response:" << endl;
					cout << response << endl;
					free(message);
					free(response);
					close(sockfd);
					continue;
				} //If the response from server doesn't have a json

				json json_token = json::parse(json_as_string);
				string received_token = json_token["error"].dump();
				cout << "Failed to log out." << received_token << endl;
				free(message);
				free(response);
				close(sockfd);
				continue;
			}

			//Clearing the cookie and the JWT, displaying success message
			cookie_str = "Empty";
			token_str = "Empty";
			cout << "Logout done." << endl;
			close(sockfd);
		}

		else if (strcmp(command, "exit") == 0) {
			break;
		}
    }
    return 0;
}