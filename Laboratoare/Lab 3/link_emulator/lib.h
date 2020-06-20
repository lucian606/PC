#ifndef LIB
#define LIB

#define MSGSIZE		1400

typedef struct {
  int len;
  char payload[MSGSIZE];
  char parity;
} msg;

typedef struct {
  int type;
  char payload[1396];
} pkt;

void init(char* remote,int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);

#endif

