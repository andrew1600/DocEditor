//TODO: client code that can send instructions to server.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "../libs/markdown.h"
#include "../libs/server_misc.h"

document *global_doc; //client list.
pthread_mutex_t doc_mutex = PTHREAD_MUTEX_INITIALIZER;

static char* global_log = NULL;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

volatile sig_atomic_t shutdown_flag = 0;


void* read_listening_thread(void* arg) {
    int fd_read = *((int *) arg);

     while(!shutdown_flag) {
       char buf[256];
       ssize_t n;
       int i = 0;
        while((n = read(fd_read, buf + i, 1)) > 0 && !shutdown_flag) {
            if(buf[i] == '\n') {
                buf[i+1] = '\0';
                //append the line to global_log
                pthread_mutex_lock(&log_mutex);
                append_char(&global_log, buf);
                pthread_mutex_unlock(&log_mutex);

                if(strcmp(buf, "END\n") == 0) {
                    pthread_mutex_lock(&doc_mutex);
                    markdown_increment_version(global_doc);
                    pthread_mutex_unlock(&doc_mutex);
                    memset(buf, 0, sizeof(buf));
                    i = 0;
                    continue;
                }

                //now see if its a valid command.
                //skip the EDIT.
                char* s1 = strchr(buf, ' ');
                if(s1 == NULL) {
                    //if we hit the end, this will happen.
                    memset(buf, 0, sizeof(buf));
                    i = 0;
                    continue;
                }

                //skip username

                char* s2 = strchr(s1 + 1, ' ');
                if(s2 == NULL) {
                    memset(buf, 0, sizeof(buf));
                    i = 0;
                    continue;
                }


                //if we find a success in this line, we know that we should run the command.
                char* s3 = strstr(s2 + 1, "SUCCESS");
                if(s3 == NULL) {
                    //if theres no success, we failed go to next line.
                    memset(buf, 0, sizeof(buf));
                    i = 0;
                    continue;
                }

                //place null terminator before success so that s1 +1 is jsut the command.
                *(s3) = '\0';
                //put it in a message.

                char* bufa = malloc(strlen(s2+1) + 1);
                strcpy(bufa, s2+1);
                time_t t = time(NULL);
                char *name = "andrew";
                char perm = 'w';


                struct msg* msg = msg_init(bufa, name, t, perm);

                //run it on the global doc.
                pthread_mutex_lock(&doc_mutex);
                char* r = run_on_doc(global_doc, msg);
                free(r);
                pthread_mutex_unlock(&doc_mutex);

                free(msg->text);
                free(msg->username);
                free(msg);
                free(bufa);

                memset(buf, 0, sizeof(buf));
                i = 0;



            }else {
                i++;
            }
        }
        
    }
    return NULL;
}

int main(int argc, char** argv) {
    //error handling
    if(argc != 3) {
        printf("Incorrect amt of args.");
        return -1;
    }


    sigset_t set;
    int sig;

    sigemptyset(&set);
    sigaddset(&set, SIGRTMIN+1);  // Add SIGRTMIN+1 to the set

    // Block SIGRTMIN+1 so sigwait can catch it
    sigprocmask(SIG_BLOCK, &set, NULL);

    //grab arguments and name them so we know that they are.
    pid_t server_pid = (pid_t)atoi(argv[1]);
    pid_t client_pid = getpid();
    int name_size = strlen(argv[2]);
    char* name = malloc(name_size + 2);
    strcpy(name, argv[2]);
    name[name_size] = '\n';
    name[name_size + 1] = '\0';

    //setup sigqueue and value
    union sigval val = {0};
    val.sival_int = (int)client_pid;
    sigqueue(server_pid, SIGRTMIN, val);

    if (sigwait(&set, &sig) == 0) {
    } else {
        perror("sigwait");
    }

    char s2c[18];
    char c2s[18];
    snprintf(s2c, sizeof(s2c), "FIFO_S2C_%d", getpid());
    snprintf(c2s, sizeof(c2s), "FIFO_C2S_%d", getpid());



    // Open the write pipe for writing (blocks until there is a reader)
    int fd_write = open(c2s, O_WRONLY);
    if (fd_write == -1) {
        perror("open fifo_write");
    }

   // Open the read pipe for reading (non-blocking so it doesn't hang if no writer)
     int fd_read = open(s2c, O_RDONLY);
     if (fd_read == -1) {
         perror("open fifo_read");
         close(fd_write);
     }

     write(fd_write, name, strlen(name));

     free(name);

     //get perms
     char perms[22];
     char perm;
     int i = 0;
     while(1) {
        read(fd_read, perms + i, 1);
        if(perms[i] == '\n') {
            perms[i] = '\0';
            break;
        }
        i += 1;
     }

 

     if(strcmp(perms, "write") == 0) {
        perm = 'w';
     }else if(strcmp(perms, "read") == 0 ) {
        perm = 'r';
     }else if(strcmp(perms, "Reject UNAUTHORISED") == 0) {
        fflush(stdout);
        close(fd_write);
        close(fd_read);
        printf("PERMS: REJECT UNAUTHORIZED\n");
        printf("Shutting down...\n");
        return -1;
     }

     //get version.
     char vers_buf[32];
     i = 0;
     while(1) {
        read(fd_read, vers_buf + i, 1);
        if(vers_buf[i] == '\n') {
            vers_buf[i] = '\0';
            break;
        }
        i += 1;
    }
     
     int version = atoi(vers_buf);

    //get size
     char size_buf[32];
     i = 0;
     while(1) {
        read(fd_read, size_buf + i, 1);
        if(size_buf[i] == '\n') {
            size_buf[i] = '\0';
            break;
        }
        i += 1;
    }
     

     int size_of_doc = atoi(size_buf);

    //set up global doc
    global_doc = markdown_init();

    int build_size = 0;
    int len = 0;
    char * build_string = malloc(size_of_doc + 1);
    memset(build_string, 0, size_of_doc);

    while((len + build_size) != size_of_doc) {
        read(fd_read, build_string + build_size, 1);
        if(build_string[build_size] == '\n') {
            build_string[build_size] = '\0';
            //insert the first line.
            markdown_newline(global_doc, global_doc->version, len);
            markdown_insert(global_doc, global_doc->version, len, build_string);

            //increment to next line.
            markdown_increment_version(global_doc);

            len += build_size + 1;
            build_size = 0;
            memset(build_string, 0, size_of_doc);

        }else {
            build_size++;
        }
    }
    if(build_size > 0) {
        build_string[build_size] = '\0';
        markdown_insert(global_doc, global_doc->version, len, build_string);
        markdown_increment_version(global_doc);
        
    }
    global_doc->version = version;

    free(build_string);

    //set up global log
    global_log = malloc(1);
    global_log[0] = '\0';


     //create dedicated reading pipe thread
    pthread_t read_thread;
    if (pthread_create(&read_thread, NULL, read_listening_thread, &fd_read) != 0) {
        perror("pthread_create stdin listener");
        exit(EXIT_FAILURE);
    }




     char buf[256];

     while(fgets(buf, sizeof(buf), stdin) != NULL) {
        if(strcmp(buf, "DISCONNECT\n") ==0 || strcmp(buf, "DISCONNECT") ==0) {
            write(fd_write, buf, strlen(buf));
            shutdown_flag = 1;
            close(fd_write);
            close(fd_read);
            break;
        }else if(strcmp(buf, "DOC?\n") == 0) {
            pthread_mutex_lock(&doc_mutex);
            markdown_print(global_doc, stdout);
            pthread_mutex_unlock(&doc_mutex);
            printf("\n");

        }else if(strcmp(buf, "LOG?\n") == 0) {
            pthread_mutex_lock(&log_mutex);
            printf("%s", global_log);
            pthread_mutex_unlock(&log_mutex);

        }else if(strcmp(buf, "PERM?\n") == 0) {
            if(perm == 'w') {
                printf("write\n");
            }else if(perm == 'r'){
                printf("read\n");
            }

        }else {
            write(fd_write, buf, strlen(buf));
        }
    }

    pthread_join(read_thread, NULL);
    printf("read_thread joined.\n");

    //free doc.
    pthread_mutex_lock(&doc_mutex);
    pthread_mutex_lock(&log_mutex);

    printf("doc freed.\n");
    markdown_free(global_doc);

    printf("log freed.\n");
    free(global_log);

    pthread_mutex_unlock(&doc_mutex);
    pthread_mutex_unlock(&log_mutex);

    printf("mutexes destroyed!\n");
    pthread_mutex_destroy(&doc_mutex);
    pthread_mutex_destroy(&log_mutex);


    return 0;




}