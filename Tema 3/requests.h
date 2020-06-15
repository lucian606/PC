#ifndef _REQUESTS_
#define _REQUESTS_

// computes and returns a GET request string (query_params
// and cookies can be set to NULL if not needed)
char *compute_get_request(char *host, char *url, char *query_params,
							const char *cookie);

// computes and returns a POST request string (cookies can be NULL if not needed)
char *compute_post_request(char *host, char *url, char* content_type, const char *body_data,
							char** cookies, int cookies_count);


char *compute_get_request_with_token(char *host, char *url, char *query_params,
                                       const char *token);


char *compute_delete_request_with_token(char *host, char *url, char *query_params,
                                        const char *token);

char *compute_post_request_with_token(char *host, char *url, char* content_type, const char *body_data, const char *token);

#endif

/*string get_json_from_message(string message) {
			string start_delim = "{";
			string end_delim ="}";
			int first_delim_pos = message.find(start_delim);
			int end_first_delim_pos = first_delim_pos + start_delim.length();
			int last_delim_pos = message.find_first_of(end_delim, end_first_delim_pos);
			string payload = message.substr(first_delim_pos, last_delim_pos);
			return payload;
}*/