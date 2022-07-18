#ifndef __COMMANDS_H__
#define __COMMANDS_H__


#define MAX_CONNECTIONS 100
#define BUF_SIZE 1024

struct client_sock {
    int sock_fd;
    int state;
    char *username;
    char buf[1024];
    int inbuf;
    struct client_sock *next;
};

struct listen_sock {
    struct sockaddr_in *addr;
    int sock_fd;
};


void start_server(long port);
void start_client(long port, char *hostname, char* message, int send);
size_t check_connect(char *input_buf);
size_t check_client(char *input_buf);
size_t check_send(char *input_buf);
size_t check_close(char *input_buf);
#endif