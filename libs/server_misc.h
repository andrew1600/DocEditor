#ifndef SERVER_MISC_H
#define SERVER_MISC_H
#include "markdown.h"



//for tracking msgs that are sent
struct msg {
    time_t time_recieved;
    char *text; //max length of a msg is 256.
    char *username;
    char perms;
    struct msg* next;
};

struct msg_list {
    struct msg *messages;
    int size;
};


struct client_list {
    struct client * clients;
    int size;
};

struct client {
    pid_t client_pid;
    struct client * next;
};

//client stuff
void add_client(struct client_list* cl, pid_t *pid);
void remove_client(struct client_list* cl, pid_t*pid);
void write_to_clients(struct client_list* cl, char* log);

//miscellaneous stuff
int check_username(char *username);


//msg cleaning
struct msg* msg_init(char *text, char *username, time_t time_rec, char perm);
void add_msg(struct msg_list * msl, struct msg* m);
void remove_msg(struct msg_list* msl, struct msg* m, struct msg* m_prev);
void free_msg_list(struct msg_list* msl);

//for log
void append_char(char** dest, const char* src);



//for incrementing
char *run_on_doc(document* global_doc, struct msg* min);


#endif 