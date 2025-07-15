
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

#include "../libs/server_misc.h"

int pipe_fds[2];  // Pipe for communication between signal handler and dispatcher

volatile sig_atomic_t shutdown_flag = 0; //for shutdown

static struct client_list global_client_list = { .clients = NULL, .size = 0 }; //client list.
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct msg_list global_msg_list = { .messages = NULL, .size = 0 }; //client list.
pthread_mutex_t message_mutex = PTHREAD_MUTEX_INITIALIZER;

document *global_doc;
pthread_mutex_t doc_mutex = PTHREAD_MUTEX_INITIALIZER;

static char* global_log = NULL;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;


// Signal handler writes sender's PID into pipe (async-signal-safe)
void signal_handler(int sig, siginfo_t *si, void *context) {
    if (sig == SIGRTMIN) {
        pid_t pid = si->si_pid;
        ssize_t ret = write(pipe_fds[1], &pid, sizeof(pid));
        (void)ret;  // Ignore errors in handler
    }
}
//empty sig handler for sending ending.
void sig_handler(int, siginfo_t *, void *) {
}

// Worker thread processes the PID received
static void* worker_thread(void* arg) {
    pid_t* pid_ptr = (pid_t*)arg;

    //set up fifo names;
    char s2c[25];
    char c2s[25];
    snprintf(s2c, sizeof(s2c), "FIFO_S2C_%d", *pid_ptr);
    snprintf(c2s, sizeof(c2s), "FIFO_C2S_%d", *pid_ptr);

    // If FIFO files already exist, remove them first
    unlink(s2c);  // Ignore return code — it's fine if they didn't exist
    unlink(c2s);


    // Create the FIFOs if they don't exist
    if (mkfifo(c2s, 0666) == -1) {
        perror("mkfifo c2s");

    }


    if (mkfifo(s2c, 0666) == -1) {
        perror("mkfifo s2c");
    }


    kill(*pid_ptr, SIGRTMIN+1);


    // Open the read pipe for reading (non-blocking so it doesn't hang if no writer)
    int fd_read = open(c2s, O_RDONLY);
    if (fd_read == -1) {
        perror("open fifo_read");
    }


    // Open the write pipe for writing (blocks until there is a reader)
    int fd_write = open(s2c, O_WRONLY);
    if (fd_write == -1) {
        perror("open fifo_write");
        close(fd_read);
    }
    //get username
    char username[30];
    ssize_t n = read(fd_read, username, 30);
    username[n -1] = '\0';
    
    int perms = check_username(username);
    char perm = '0';

    if(perms == 0) {
        write(fd_write, "Reject UNAUTHORISED\n", 20);
        sleep(1);
        close(fd_read);
        close(fd_write);
        unlink(s2c);
        unlink(c2s);
        free(pid_ptr);
        return NULL;
    } else if(perms == 1) {
        write(fd_write, "write\n", 6);
        perm = 'w';
    } else if(perms == 2) {
        write(fd_write, "read\n", 5);
        perm = 'r';
    }

    printf("Client Registered: %d\n", *pid_ptr);

    pthread_mutex_lock(&counter_mutex);
    add_client(&global_client_list, pid_ptr);
    pthread_mutex_unlock(&counter_mutex);

    pthread_mutex_lock(&doc_mutex);
    //write version
    uint64_t version = global_doc->version;  // or however you're storing it
    char vers_buf[32];
    snprintf(vers_buf, sizeof(vers_buf), "%lu\n", version);

    write(fd_write, vers_buf, strlen(vers_buf));

    //write size
    uint64_t size_n = global_doc->size;  // or however you're storing it
    char size_buf[32];
    snprintf(size_buf, sizeof(size_buf), "%lu\n", size_n);

    write(fd_write, size_buf, strlen(size_buf));


    //write document contents.
    char* doc_contents = markdown_flatten(global_doc);
    write(fd_write, doc_contents, strlen(doc_contents));
    free(doc_contents);

    


    pthread_mutex_unlock(&doc_mutex);

    


    char buf[256];

    while(1) {
       ssize_t n = read(fd_read, buf, sizeof(buf) - 1);
       time_t now = time(NULL);
       if(n > 0) {
            buf[n] = '\0';
            //if we gonna disconnect then disconnect.
            if(strcmp(buf, "DISCONNECT\n") == 0 || strcmp(buf, "DISCONNECT") == 0 ) {
                printf("Client disconnected: %d\n", *pid_ptr);
                pthread_mutex_lock(&counter_mutex);
                remove_client(&global_client_list, pid_ptr);
                pthread_mutex_unlock(&counter_mutex);
                close(fd_read);
                close(fd_write);
                unlink(s2c);
                unlink(c2s);
                break;
            }else {
                //if not send it off to the queue.
                struct msg* message = msg_init(buf, username, now, perm);
                pthread_mutex_lock(&message_mutex);
                add_msg(&global_msg_list, message);
                pthread_mutex_unlock(&message_mutex);
            }
        }
    }
    free(pid_ptr);
    return NULL;
}

// Dispatcher thread reads from pipe and spawns worker threads
void* signal_dispatcher_thread(void* arg) {
    (void)arg;
    while (!shutdown_flag) {
        pid_t pid;
        ssize_t bytes = read(pipe_fds[0], &pid, sizeof(pid));
        if (bytes == sizeof(pid)) {
            pid_t* pid_copy = malloc(sizeof(pid));
            if (!pid_copy) {
                fprintf(stderr, "Failed to allocate memory\n");
                continue;
            }
            *pid_copy = pid;

            pthread_t tid;
            if (pthread_create(&tid, NULL, worker_thread, pid_copy) == 0) {
                pthread_detach(tid);
            } else {
                fprintf(stderr, "Failed to create worker thread\n");
                free(pid_copy);
            }
        } else if (bytes == -1) {
            perror("read");
        } else {
            fprintf(stderr, "Partial or unexpected read size: %zd\n", bytes);
        }
    }
    return NULL;
}

// Dedicated thread that unblocks SIGRTMIN and waits for signals using pause()
void* signal_handling_thread(void* arg) {
    (void)arg;

    // Unblock SIGRTMIN in this thread only
    sigset_t unblock_set;
    sigemptyset(&unblock_set);
    sigaddset(&unblock_set, SIGRTMIN);
    sigaddset(&unblock_set, SIGUSR1);
    if (pthread_sigmask(SIG_UNBLOCK, &unblock_set, NULL) != 0) {
        perror("pthread_sigmask unblock");
        pthread_exit(NULL);
    }

    // Wait for signals indefinitely
    while (!shutdown_flag) {
        pause();  // Wait for signal; signal handler will run on signal arrival
    }
    return NULL;
}

void* stdin_listening_thread(void* arg) {
    (void)arg;

    char buf[256];


    while(fgets(buf, sizeof(buf), stdin) != NULL) {
    
        //grab client list size;
        pthread_mutex_lock(&counter_mutex);
        int client_size = global_client_list.size;
        pthread_mutex_unlock(&counter_mutex);


       if((strcmp(buf, "QUIT\n") == 0 || strcmp(buf, "QUIT") == 0 )) {
            if(client_size == 0) {
                shutdown_flag = 1;
                kill(getpid(), SIGUSR1);
                break;

            }else {
                printf("QUIT rejected, %d clients still connected.\n", client_size);
            }

        }else if(strcmp(buf, "DOC?\n") == 0) {
            pthread_mutex_lock(&doc_mutex);
            markdown_print(global_doc, stdout);
            pthread_mutex_unlock(&doc_mutex);
            printf("\n");

        }else if(strcmp(buf, "LOG?\n") == 0) {
            pthread_mutex_lock(&log_mutex);
            printf("%s", global_log);
            pthread_mutex_unlock(&log_mutex);

        }

    }
    return NULL;

}


int main(int argc, char** argv) {
    if(argc != 2) {
        perror("not enough args.");
    }
    int time_interval = atoi(argv[1]);

    //set up pipe
    if (pipe(pipe_fds) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    //set up document
    global_doc = markdown_init();
    //set up global log
    global_log = malloc(1);
    global_log[0] = '\0';

    // Block SIGRTMIN in main thread (and threads it creates)
    sigset_t block_set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGRTMIN);
    sigaddset(&block_set, SIGRTMIN+1);
    sigaddset(&block_set, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &block_set, NULL) == -1) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    // Setup signal handler for SIGRTMIN
    struct sigaction sa = {0};
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = signal_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    //shutdown signal;
    struct sigaction sa_usr1 = {0};
    sa_usr1.sa_flags = SA_SIGINFO;
    sa_usr1.sa_sigaction = sig_handler;  // your handler for SIGUSR1
    sigemptyset(&sa_usr1.sa_mask);
    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
        perror("sigaction SIGRTMIN+1");
        exit(EXIT_FAILURE);
    }  

    // Create dispatcher thread to handle signals
    pthread_t dispatcher_tid;
    if (pthread_create(&dispatcher_tid, NULL, signal_dispatcher_thread, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    // Create dedicated signal handling thread that unblocks SIGRTMIN and waits
    pthread_t sig_thread;
    if (pthread_create(&sig_thread, NULL, signal_handling_thread, NULL) != 0) {
        perror("pthread_create signal handler");
        exit(EXIT_FAILURE);
    }

    printf("SERVER PID: %d\n", getpid());
    //create dedicated stdin handling thread
    pthread_t stdin_thread;
    if (pthread_create(&stdin_thread, NULL, stdin_listening_thread, NULL) != 0) {
        perror("pthread_create stdin listener");
        exit(EXIT_FAILURE);
    }


    while(!shutdown_flag) {
        pthread_mutex_lock(&doc_mutex);
        pthread_mutex_lock(&message_mutex);
        char* edits = (char*)malloc(1);
        edits[0] = '\0';
    
        while(global_msg_list.size != 0) {
            //get the earliest time
                struct msg *curr = global_msg_list.messages;
                struct msg* min = global_msg_list.messages; 
                struct msg* prev = NULL;
                struct msg* min_prev = NULL;
            while(curr != NULL) {
                if(curr->time_recieved < min->time_recieved) {
                    min = curr;
                    min_prev = prev;

                }
                prev = curr;
                curr = curr->next;
            }
            //run the msg with earliest time on doc.
            char* r = run_on_doc(global_doc, min);
            append_char(&edits, r);
            free(r);

            //remove msg from queue
            remove_msg(&global_msg_list, min, min_prev);
        }
        //search for a success within it, if so increment.
    
        char* s = strstr(edits, "SUCCESS");
        if(s) {
            markdown_increment_version(global_doc);
        }
        int size_of_write = snprintf(NULL, 0, "VERSION %d\n%sEND\n", global_doc->version, edits);
        char* current_log = (char*)malloc(size_of_write + 1);
        snprintf(current_log, size_of_write + 1, "VERSION %d\n%sEND\n", global_doc->version, edits);

        //now send it out to the people
        pthread_mutex_lock(&counter_mutex);
        write_to_clients(&global_client_list, current_log);
        pthread_mutex_unlock(&counter_mutex);


        pthread_mutex_lock(&log_mutex);
        append_char(&global_log, current_log);

        free(edits);
        free(current_log);

        pthread_mutex_unlock(&log_mutex);
        pthread_mutex_unlock(&doc_mutex);
        pthread_mutex_unlock(&message_mutex);

        usleep(time_interval*1000);
        //now i'll run all of the changes.
    }


    pthread_join(stdin_thread, NULL);
    printf("stdin_listening thread closed\n");

    pthread_join(sig_thread, NULL);
    printf("signal_listening thread closed\n");

    printf("closing pipes\n");
    close(pipe_fds[0]);
    close(pipe_fds[1]);

    pthread_join(dispatcher_tid, NULL);
    printf("dispatch thread closed\n");


    printf("printing to doc.md and freeing doc.\n");
    pthread_mutex_lock(&doc_mutex);
    pthread_mutex_lock(&log_mutex);
    pthread_mutex_lock(&message_mutex);

    FILE* fd_doc = fopen("doc.md", "w");
    char * doc_conts = markdown_flatten(global_doc);
    fputs(doc_conts, fd_doc);
    fclose(fd_doc);
    free(doc_conts);



    printf("freeing doc\n");
    markdown_free(global_doc);

    printf("freeing log\n");
    free(global_log);

    printf("freeing messages\n");
    free_msg_list(&global_msg_list);

    pthread_mutex_unlock(&log_mutex);
    pthread_mutex_unlock(&doc_mutex);
    pthread_mutex_unlock(&message_mutex);

    printf("destroying mutexes\n");

    pthread_mutex_destroy(&log_mutex);
    pthread_mutex_destroy(&doc_mutex);
    pthread_mutex_destroy(&message_mutex);
    pthread_mutex_destroy(&counter_mutex);

    printf("SHUTDOWN\n");

    return 0;
}
