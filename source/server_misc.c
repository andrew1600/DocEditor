#include "../libs/server_misc.h"
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>


void add_client(struct client_list* cl, pid_t *pid) {
    cl->size++;
    
    struct client* c = (struct client *)malloc(sizeof(struct client));
    c->client_pid = *pid;
    c->next = NULL;

    if(cl->size == 1) {
        cl->clients = c;
    } else {
        struct client* curr = cl->clients;
        while(curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = c;
    }
    return;

}
void remove_client(struct client_list* cl, pid_t*pid) {
    struct client* curr = cl->clients;
    struct client* prev = NULL;
    while(curr->client_pid != *pid) {
        prev = curr;
        curr = curr->next;
    }

    // Remove the node
    if (prev == NULL) {
        cl->clients = curr->next;
    } else {
        prev->next = curr->next;
    }

    free(curr);
    cl->size--;
    return;
}


void write_to_clients(struct client_list* cl, char* log) {
    struct client *curr = cl->clients;


    while(curr != NULL) {
        char s2c[25];
        snprintf(s2c, sizeof(s2c), "FIFO_S2C_%d", curr->client_pid);
        int fd_write = open(s2c, O_WRONLY);
        if (fd_write == -1) {
            perror("open fifo_write");
        }

        write(fd_write, log, strlen(log));
        close(fd_write);
        curr = curr->next;
    }
    return;
}

int check_username(char *username) {
    FILE* file = fopen("roles.txt", "r");
    if(file == NULL) {
        perror("fopen");
        return -1;
    }
    char buffer[256];
    while(fgets(buffer, sizeof(buffer), file) != NULL) {
        char* token = strtok(buffer, " \t");
        if(strcmp(token, username) == 0) {
            token = strtok(NULL, " \t");
            if(strcmp(token, "write") == 0 || strcmp(token, "write\n") == 0) {
                fclose(file);
                return 1;
            }else if(strcmp(token, "read") == 0 || strcmp(token, "read\n") == 0) {
                fclose(file);
                return 2;
            }
        }
    }
    fclose(file);
    return 0;
 }








 struct msg* msg_init(char *text, char *username, time_t time_rec, char perm) {
    struct msg* m = malloc(sizeof(struct msg));
    if (!m) return NULL;

    m->perms = perm;
    m->next = NULL;
    m->time_recieved = time_rec;

    m->text = malloc(strlen(text) + 1);
    if (!m->text) {
        free(m);
        return NULL;
    }
    strcpy(m->text, text);

    m->username = malloc(strlen(username) + 1);
    if (!m->username) {
        free(m->text);
        free(m);
        return NULL;
    }
    strcpy(m->username, username);

    return m;
}

void add_msg(struct msg_list * msl, struct msg* m) {

    if(msl->messages == NULL) {
        msl->messages = m;
    } else {
        struct msg* curr = msl->messages;
        while(curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = m;
    }
    msl->size++;
    return;
}

void remove_msg(struct msg_list* msl, struct msg* m, struct msg* m_prev ) {

    // Remove the node
    if (m_prev == NULL) {
        msl->messages = m->next;
    } else {
        m_prev->next = m->next;
    }

    free(m->text);
    free(m->username);
    free(m);
    msl->size--;
    return;
}

void free_msg_list(struct msg_list* msl) {
    struct msg* curr  = msl->messages;
    while(curr != NULL) {
        struct msg* temp = curr;
        curr = curr->next;
        free(temp->text);
        free(temp->username);
        free(temp);
    }
    return;

}

void append_char(char **dest, const char *src) {
    int dest_size = 0;
    if (*dest != NULL) {
        dest_size = strlen(*dest);
    }
    int src_size = 0;
    if(src != NULL) {
        src_size = strlen(src);
    }

    *dest = realloc(*dest, dest_size + src_size + 1);
    if (*dest == NULL) {
        perror("realloc failed");
        exit(1); // or handle error
    }

    strcpy(*dest + dest_size, src);
}

void error_handle(char **r, struct msg *min) {
    int size_of_string = snprintf(NULL, 0, "EDIT %s %s REJECT\n", min->username, min->text);
    *r = malloc(size_of_string + 1);
    if (*r == NULL) {
        perror("malloc failed");
        exit(1);
    }
    snprintf(*r, size_of_string + 1, "EDIT %s %s REJECT\n", min->username, min->text);
}

char *run_on_doc(document *global_doc, struct msg* min) {
    int size = strlen(min->text);
    min->text[size - 1] = '\0';
    char* copy = malloc(size + 1);
    strcpy(copy, min->text);
    char * r = NULL;


    if(min->perms == 'r') {
        int size_of_string = snprintf(NULL, 0, "EDIT %s %s Reject UNAUTHORISED\n", min->username, min->text) + 1;
        r = malloc(size_of_string);
        snprintf(r, size_of_string, "EDIT %s %s Reject UNAUTHORISED\n", min->username, min->text);
        free(copy);
        return r;
    }else if(size > 256) {
        error_handle(&r, min);
        free(copy);
        return r;
    }

    

    char *s1 = strchr(copy, ' ');
    if(s1 == NULL) {
        error_handle(&r, min);
        free(copy);
        return r;
    }
    *s1 = '\0';
    int result = -99;


    if(strcmp(copy, "INSERT") == 0) {

        char* s2 = strchr(s1 + 1, ' ');
        if(s2 == NULL) {
            error_handle(&r, min);
            free(copy);
            return r;
        }
        *s2 = '\0';


        int pos = atoi(s1 + 1);

        //make sure theres no newlines in this insert.
        char*s3 = strchr(s2 + 1, '\n');
        if(s3 != NULL) {
            int size_of_string = snprintf(NULL, 0, "EDIT %s %s Reject\n", min->username, min->text) + 1;
            r = malloc(size_of_string);
            snprintf(r, size_of_string, "EDIT %s %s Reject\n", min->username, min->text);
            free(copy);
            return r;
        }

        result = markdown_insert(global_doc, global_doc->version, pos, s2 + 1);
    }else if(strcmp(copy, "DEL") == 0) {

        char* s2 = strchr(s1 + 1, ' ');
        if(s2 == NULL) {
            error_handle(&r, min);
            free(copy);
            return r;
        }
        *s2 = '\0';

        int pos = atoi(s1 + 1);
        int no_char = atoi(s2 + 1);

        result = markdown_delete(global_doc, global_doc->version, pos, no_char);

    } else if(strcmp(copy, "NEWLINE") == 0) {
        int pos = atoi(s1 + 1);

        result = markdown_newline(global_doc, global_doc->version, pos);
        
    } else if(strcmp(copy, "HEADING") == 0) {

        char* s2 = strchr(s1 + 1, ' ');
        if(s2 == NULL) {
            error_handle(&r, min);
            free(copy);
            return r;
        }
        *s2 = '\0';

        int level = atoi(s1 + 1);
        int pos = atoi(s2 + 1);

        result = markdown_heading(global_doc, global_doc->version, level, pos);
        
    } else if(strcmp(copy, "BOLD") == 0) {

        char* s2 = strchr(s1 + 1, ' ');
        if(s2 == NULL) {
            error_handle(&r, min);
            free(copy);
            return r;
        }
        *s2 = '\0';

        int pos_start = atoi(s1 + 1);
        int pos_end = atoi(s2 + 1);

        result = markdown_bold(global_doc, global_doc->version, pos_start, pos_end);

        
    } else if(strcmp(copy, "ITALIC") == 0) {

        char* s2 = strchr(s1 + 1, ' ');
        if(s2 == NULL) {
            error_handle(&r, min);
            free(copy);
            return r;
        }
        *s2 = '\0';

        int pos_start = atoi(s1 + 1);
        int pos_end = atoi(s2 + 1);

        result = markdown_italic(global_doc, global_doc->version, pos_start, pos_end);
        
    } else if(strcmp(copy, "BLOCKQUOTE") == 0) {
        int pos = atoi(s1 + 1);

        result = markdown_blockquote(global_doc, global_doc->version, pos);
        
    } else if(strcmp(copy, "ORDERED_LIST") == 0) {
        int pos = atoi(s1 + 1);

        result = markdown_ordered_list(global_doc, global_doc->version, pos);
        
    } else if(strcmp(copy, "UNORDERED_LIST") == 0) {
        int pos = atoi(s1 + 1);

        result = markdown_unordered_list(global_doc, global_doc->version, pos);

    } else if(strcmp(copy, "CODE") == 0) {

        char* s2 = strchr(s1 + 1, ' ');
        if(s2 == NULL) {
            error_handle(&r, min);
            free(copy);
            return r;
        }
        *s2 = '\0';

        int pos_start = atoi(s1 + 1);
        int pos_end = atoi(s2 +1);

        result = markdown_code(global_doc, global_doc->version, pos_start, pos_end);
        
    } else if(strcmp(copy, "HORIZONTAL_RULE") == 0) {
        int pos = atoi(s1 + 1);

        result = markdown_horizontal_rule(global_doc, global_doc->version, pos);
        
    } else if(strcmp(copy, "LINK") == 0) {

        char* s2 = strchr(s1 + 1, ' ');
        if(s2 == NULL) {
            error_handle(&r, min);
            free(copy);
            return r;
        }
        *s2 = '\0';

        char* s3 = strchr(s2 + 1, ' ');
        if(s3 == NULL) {
            error_handle(&r, min);
            free(copy);
            return r;
        }
        *s3 = '\0';
        int pos_start = atoi(s1 + 1);
        int pos_end = atoi(s2 + 1);

        result = markdown_link(global_doc, global_doc->version, pos_start, pos_end, s3+1);
    }


    if(result == 0) {
        int size_of_string = snprintf(NULL, 0, "EDIT %s %s SUCCESS\n", min->username, min->text) + 1;
        r = malloc(size_of_string + 1);
        snprintf(r, size_of_string + 1, "EDIT %s %s SUCCESS\n", min->username, min->text);
        free(copy);
        return r;
    }else if(result == -1) {
        int size_of_string = snprintf(NULL, 0, "EDIT %s %s Reject INVALID_POSITION\n", min->username, min->text) + 1;
        r = malloc(size_of_string + 1);
        snprintf(r, size_of_string + 1, "EDIT %s %s Reject INVALID_POSITION\n", min->username, min->text);
        free(copy);
        return r;

    }else if(result == -2) {
        int size_of_string = snprintf(NULL, 0, "EDIT %s %s Reject DELETED_POSITION\n", min->username, min->text) + 1;
        r = malloc(size_of_string + 1);
        snprintf(r, size_of_string + 1, "EDIT %s %s Reject DELETED_POSITION\n", min->username, min->text);
        free(copy);
        return r;

    }else {
        error_handle(&r, min);
        free(copy);
        return r;
    }

}

