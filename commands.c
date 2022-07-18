#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>  
#include <sys/socket.h>  
#include <errno.h>  
#include <signal.h>  
#include <arpa/inet.h>      
#include <netdb.h>          
#include <netinet/in.h>      
  
  
  
#include "commands.h"  
#include "io_helpers.h"  
#include "builtins.h"  
#include "variables.h" 
  
  
  
// --code from lab9 -------------start.
int sigint_received = 0;  
  
pid_t server_pid;  
char *token_arr[MAX_STR_LEN] = {NULL};  
  
  
void sigint_handler(int code) {  
    sigint_received = 1;  
}  
  
  
void setup_server_socket(struct listen_sock *s, long port) {  
    if(!(s->addr = malloc(sizeof(struct sockaddr_in)))) {  
        perror("malloc");  
        exit(1);  
    }  
    // Allow sockets across machines.  
    s->addr->sin_family = AF_INET;  
    // The port the process will listen on.  
    s->addr->sin_port = htons(port);  
    // Clear this field; sin_zero is used for padding for the struct.  
    memset(&(s->addr->sin_zero), 0, 8);  
    // Listen on all network interfaces.  
    s->addr->sin_addr.s_addr = INADDR_ANY;  
  
    s->sock_fd = socket(AF_INET, SOCK_STREAM, 0);  
    if (s->sock_fd < 0) {  
        perror("server socket");  
        exit(1);  
    }  
  
    // Make sure we can reuse the port immediately after the  
    // server terminates. Avoids the "address in use" error  
    int on = 1;  
    int status = setsockopt(s->sock_fd, SOL_SOCKET, SO_REUSEADDR,  
        (const char *) &on, sizeof(on));  
    if (status < 0) {  
        perror("setsockopt");  
        exit(1);  
    }  
  
    // Bind the selected port to the socket.  
    if (bind(s->sock_fd, (struct sockaddr *)s->addr, sizeof(*(s->addr))) < 0) {  
        display_error("ERROR: Address already in use","");  
        display_error("ERROR: Builtin failed: start-server","");  
        close(s->sock_fd);  
        exit(1);  
    }  
  
    // Announce willingness to accept connections on this socket.  
    if (listen(s->sock_fd, 10) < 0) {  
        perror("server: listen");  
        close(s->sock_fd);  
        exit(1);  
    }  
}  
  
int accept_connection(int fd, struct client_sock **clients) {  
    struct sockaddr_in peer;  
    unsigned int peer_len = sizeof(peer);  
    peer.sin_family = AF_INET;  
  
    int num_clients = 0;  
    struct client_sock *curr = *clients;  
    while (curr != NULL && num_clients < MAX_CONNECTIONS && curr->next != NULL) {  
        curr = curr->next;  
        num_clients++;  
    }  
  
    int client_fd = accept(fd, (struct sockaddr *)&peer, &peer_len);  
      
    if (client_fd < 0) {  
        perror("server: accept");  
        close(fd);  
        exit(1);  
    }  
  
    if (num_clients == MAX_CONNECTIONS) {  
        close(client_fd);  
        return -1;  
    }  
  
    struct client_sock *newclient = malloc(sizeof(struct client_sock));  
    newclient->sock_fd = client_fd;  
    newclient->inbuf = newclient->state = 0;  
    newclient->username = NULL;  
    newclient->next = NULL;  
    memset(newclient->buf, 0, BUF_SIZE);  
    if (*clients == NULL) {  
        *clients = newclient;  
    }  
    else {  
        curr->next = newclient;  
    }  
  
    return client_fd;  
}  
  
/* 
 * Close all sockets, free memory, and exit with specified exit status. 
 */  
void clean_exit(struct listen_sock s, struct client_sock *clients, int exit_status) {  
    struct client_sock *tmp;  
    while (clients) {  
        tmp = clients;  
        close(tmp->sock_fd);  
        clients = clients->next;  
        free(tmp->username);  
        free(tmp);  
    }  
    close(s.sock_fd);  
    free(s.addr);  
    exit(exit_status);  
}  
  
  
int find_network_newline(const char *buf, int inbuf) {  
    for (int i = 0; i < inbuf -1; i++)  
        if (buf[i] == '\r' && buf[i+1] == '\n')  
            return i + 2;  
    return -1;  
}  
  
int read_from_socket(int sock_fd, char *buf, int *inbuf) {  
    int num_read = read(sock_fd, buf + *inbuf, BUF_SIZE - *inbuf);  
      
    // if client disconnected...  
    if (num_read == 0) return 1;  
  
    if (num_read == -1) return -1;  
  
    *inbuf += num_read;  
    for (int i = 0; i <= *inbuf-2; i++)  
        if (buf[i] == '\r' && buf[i+1] == '\n')  
            return 0;  
      
    if (*inbuf == BUF_SIZE) return -1;  
  
    return 2;  
}  
  
int read_from_client(struct client_sock *curr) {  
    return read_from_socket(curr->sock_fd, curr->buf, &(curr->inbuf));  
}  
  
int get_message(char **dst, char *src, int *inbuf) {  
    int msg_len = find_network_newline(src, *inbuf);  
    if (msg_len == -1) return 1;  
    *dst = malloc(BUF_SIZE);  
    if (*dst == NULL) {  
        perror("malloc");  
        return 1;  
    }  
  
    memmove(*dst, src, msg_len - 2);  
    (*dst)[msg_len - 2] = '\0';  
    memmove(src, src + msg_len, BUF_SIZE - msg_len);  
    *inbuf -= msg_len;  
    return 0;  
}  
  
  
int remove_client(struct client_sock **curr, struct client_sock **clients) {  
    struct client_sock *temp = *clients;  
    struct client_sock *prev = NULL;  
  
    if(temp != NULL && temp == *curr){  
        temp = *clients;  
        *clients = temp->next;  
        *curr = *clients;  
        free(temp->username);  
        free(temp);  
        return 0;  
    }  
    while(temp != NULL && temp != *curr){  
        prev = temp;  
        temp = temp->next;  
    }  
    if(temp){  
        prev->next = temp->next;  
        *curr = prev->next;  
        free(temp->username);  
        free(temp);  
        return 0;  
    }  
    free(temp);  
    return 1;  
}  
  
void start_server(long port) {  
  
    // Linked list of clients  
    struct client_sock *clients = NULL;  
      
    struct listen_sock s;  
    setup_server_socket(&s, port);  
  
  
    // Set up SIGINT handler  
    struct sigaction sa_sigint;  
    memset (&sa_sigint, 0, sizeof (sa_sigint));  
    sa_sigint.sa_handler = sigint_handler;  
    sa_sigint.sa_flags = 0;  
    sigemptyset(&sa_sigint.sa_mask);  
    sigaction(SIGINT, &sa_sigint, NULL);  
  
    int exit_status = 0;  
    int max_fd = s.sock_fd;  
  
    fd_set all_fds, listen_fds;  
      
    FD_ZERO(&all_fds);  
    FD_SET(s.sock_fd, &all_fds);  
  
    do {  
  
        listen_fds = all_fds;  
        int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);  
        if (sigint_received) break;  
        if (nready == -1) {  
            if (errno == EINTR) continue;  
            perror("server: select");  
            exit_status = 1;  
            break;  
        }  
  
        /*  
         * If a new client is connecting, create new 
         * client_sock struct and add to clients linked list. 
         */  
        if (FD_ISSET(s.sock_fd, &listen_fds)) {  
            int client_fd = accept_connection(s.sock_fd, &clients);  
            if (client_fd < 0) {  
                printf("Failed to accept incoming connection.\n");  
                continue;  
            }  
            if (client_fd > max_fd) {  
                max_fd = client_fd;  
            }  
            FD_SET(client_fd, &all_fds);  
        }  
  
        if (sigint_received) break;  
  
  
        /* 
         * Accept incoming messages from clients, 
         * and send to all other connected clients. 
         */  
        struct client_sock *curr = clients;  
        while (curr) {  
            if (!FD_ISSET(curr->sock_fd, &listen_fds)) {  
                curr = curr->next;  
                continue;  
            }  
            int client_closed = read_from_client(curr);  
              
            // If error encountered when receiving data  
            if (client_closed == -1) {  
                client_closed = 1; // Disconnect the client  
            }  
              
            char *msg;  
            // Loop through buffer to get complete message(s)  
            while (client_closed == 0 && !get_message(&msg, curr->buf, &(curr->inbuf))) {  
  
                display_message("\n");
                display_message(msg);  
                display_message("\n");  
                display_message("mysh$ ");  
  
                free(msg);  
              
            }  
              
            if (client_closed == 1) { // Client disconnected  
                FD_CLR(curr->sock_fd, &all_fds);  
                close(curr->sock_fd);  
                remove_client(&curr, &clients);  
            }  
            else {  
                curr = curr->next;  
            }  
        }  
    } while(!sigint_received);  
    clean_exit(s, clients, exit_status);  
}  
  
  
  
  
  
int write_to_socket(int sock_fd, char *buf, int len) {  
    int num_write;  
    if ((num_write = write(sock_fd, buf, len)) == -1) {  
        perror("write");  
        return 1;  
    }  
    if (num_write == 0)   
        return 2;  
    return 0;  
}  
  
  
void start_client(long port, char *hostname, char* message, int send) {  
     
    // Create the socket FD.  
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);  
    if (sock_fd < 0) {  
        perror("client: socket");  
        return;  
    }  
  
    // Set the IP and port of the server to connect to.  
    struct sockaddr_in server;  
    server.sin_family = AF_INET;  
    server.sin_port = htons(port);  
    if (inet_pton(AF_INET, hostname, &server.sin_addr) < 1) {  
        perror("client: inet_pton");  
        close(sock_fd);  
        return;  
    }  
  
    // Connect to the server.  
    if (connect(sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1) {  
        perror("client: connect");  
        close(sock_fd);  
        return;  
    }  
  
    char buf[BUF_SIZE];  
    int buf_len;  
  
    if (send) {  
        strcpy(buf, message);  
        buf_len = strlen(buf) + 1;  
      
  
        buf[buf_len-1] = '\r';  
        buf[buf_len] = '\n';  
        buf_len += 1;  
        if (write_to_socket(sock_fd, buf, buf_len)) {  
            fprintf(stderr, "Error sending msg.\n");  
            return;  
        }  
  
    } else {  
          
        while (fgets(buf, BUF_SIZE-2, stdin)) {  
            buf_len = strlen(buf);  
            if (buf_len == 1) break;  
  
            buf[buf_len-1] = '\r';  
            buf[buf_len] = '\n';  
            buf_len += 1;  
              
            if (write_to_socket(sock_fd, buf, buf_len)) {  
                fprintf(stderr, "Error sending msg.\n");  
                return;  
            }  
        }  
    }  
  
    close(sock_fd);  
}  
  
// --code from lab9 -------------end.

  
size_t check_connect(char *input_buf){  
    if (strncmp(input_buf, "start-server", 12) == 0) {  
        int i = tokenize_input(input_buf, token_arr);  
        if (i < 2) {  
            display_error("ERROR: No port provided", "");  
            return 0;  
        }  
          
        if ((server_pid = fork()) == -1) {  
            display_error("fork", "");  
            return 0;  
        }  
        if (server_pid > 0) return 0;  
  
        
        if (server_pid == 0) {  
            start_server(strtol(token_arr[1], NULL, 10));  
            return 0;  
        }  
    } return 0;  
}  
  
size_t check_client(char *input_buf){  
    if (strncmp(input_buf, "start-client", 12) == 0) {  
        int i = tokenize_input(input_buf, token_arr);  
        if (i < 2) {  
            display_error("ERROR: No port provided", "");  
            return 0;  
        }  
        if (i < 3) {  
            display_error("ERROR: No hostname provided", "");  
            return 0;  
        }  
        long port = strtol(token_arr[1], NULL, 10);  
        char *hostname = token_arr[2];  
  
      
        start_client(port, hostname, "", 0);  
        return 0;  
    } return 0;    
}  
  
  
  
size_t check_send(char *input_buf){  
    if (strncmp(input_buf, "send ", 5) == 0) {  
        int i = tokenize_input(input_buf, token_arr);  
        if (i < 2) {  
            display_error("ERROR: No port provided", "");  
            return 0;  
        }  
        if (i < 3) {  
            display_error("ERROR: No hostname provided", "");  
            return 0;  
        }  
        if (i < 4) {  
            display_error("ERROR: No message", "");  
            return 0;  
        }  
        long port = strtol(token_arr[1], NULL, 10);  
        char *hostname = token_arr[2];  
        char message[128];  
        message[0] = '\0';  
        strcat(message, token_arr[3]);  
        for(int n = 4; n < i; n++){  
            strcat(message, " ");  
            strcat(message, token_arr[n]);  
        }  
        start_client(port, hostname, message, 1);  
        return 0;  
    } return 0;  
}  
  
  
size_t check_close(char *input_buf){  
    if (strncmp(input_buf, "close-server", 12) == 0) {  
        if (server_pid > 0) {   
            kill(server_pid, SIGINT);  
            return 0;  
        }  
    } return 0;  
}  