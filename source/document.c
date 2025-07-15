#include "../libs/document.h"
//HELPER FUNCTIONS FOR MARKDOWN.
//EDITING
chunk*head_init() {
    chunk* head = (chunk *)malloc(sizeof(chunk));
    head->size = 0;
    head->arr = NULL;

    head->tbd = 0;
    head->tbi = 0;
    head->newline = 0;
    head->head = 1;
    return head;
}


chunk * insert_init(const char* content) {
    chunk* insert = (chunk *)malloc(sizeof(chunk));
    //get the size
    insert->size = strlen(content);
    //malloc the content
    insert->arr = (char *)malloc(insert->size);
    memcpy(insert->arr, content, insert->size);

    //set markers
    insert->tbd = 0;
    insert->tbi = 1;
    insert->newline = 0;
    insert->head = 0;
    return insert;
}

chunk* delete_init(int len) {
    chunk* del_sec = (chunk *)malloc(sizeof(chunk));
    del_sec->size = len;
    del_sec->arr = NULL;

    del_sec->tbd = 1;
    del_sec->tbi = 0;
    del_sec->newline = 0;
    del_sec->head = 0;
    return del_sec;
}

//FORMATTING
chunk* newline_init() {
    chunk* newline = (chunk *)malloc(sizeof(chunk));
    //get the size
    newline->size = 1;
    //malloc the content
    newline->arr = (char *)malloc(2);
    newline->arr[0] = '\n';

    //set markers
    newline->tbd = 0;
    newline->tbi = 1;
    newline->newline = 1;
    newline->head = 0;
    return newline;
}

//BLOCKQUOTES
chunk* heading_init(int level) {
    chunk* heading = (chunk*)malloc(sizeof(chunk));
    if(level == 1) {
        heading->size = 2;
        heading->arr = (char *)malloc(3);
        strcpy(heading->arr, "# ");

    }
    if(level == 2) {
        heading->size = 3;
        heading->arr = (char *)malloc(4);
        strcpy(heading->arr, "## ");

    }
    if(level == 3) {
        heading->size = 4;
        heading->arr = (char *)malloc(5);
        strcpy(heading->arr, "### ");
    }

    //set markers
    heading->tbd = 0;
    heading->tbi = 1;
    heading->newline = 0;
    heading->head = 0;

    return heading;
}

chunk* bq_init() {
    chunk* bq = (chunk*)malloc(sizeof(chunk));
    bq->size = 2;
    bq->arr = (char *)malloc(3);
    strcpy(bq->arr, "> ");

    bq->tbd = 0;
    bq->tbi = 1;
    bq->newline = 0;
    bq->head = 0;
    return bq; 

}
chunk* ol_init(int num) {
    chunk* ol = (chunk*)malloc(sizeof(chunk));
    ol->size = 3;
    ol->arr = (char *)malloc(4);
    snprintf(ol->arr, 4, "%d. ", num);

    ol->tbd = 0;
    ol->tbi = 1;
    ol->newline = 0;
    ol->head = 0;
    return ol;
}
chunk* ul_init() {
    chunk* ul = (chunk *)malloc(sizeof(chunk));
    ul->size = 2;

    ul->arr = (char *)malloc(3);
    strcpy(ul->arr, "- ");

    ul->tbd = 0;
    ul->tbi = 1;
    ul->newline = 0;
    ul->head = 0;
    return ul;
}
chunk* hr_init() {
    chunk* hr = (chunk*)malloc(sizeof(chunk));
    hr->size = 3;

    hr->arr = (char *)malloc(4);
    strcpy(hr->arr, "---");

    hr->tbd = 0;
    hr->tbi = 1;
    hr->newline = 0;
    hr->head = 0;
}

//INLINE
chunk* bold_init() {
    chunk* bold = (chunk *)malloc(sizeof(chunk));
    //get the size
    bold->size = 2;
    //malloc the content
    bold->arr = (char *)malloc(3);
    bold->arr[0] = '*';
    bold->arr[1] = '*';

    //set markers
    bold->tbd = 0;
    bold->tbi = 1;
    bold->newline = 0;
    bold->head = 0;
    return bold;

}
chunk* italic_init() {
    chunk* italic = (chunk *)malloc(sizeof(chunk));
    //get the size
    italic->size = 1;
    //malloc the content
    italic->arr = (char *)malloc(1);
    italic->arr[0] = '*';

    //set markers
    italic->tbd = 0;
    italic->tbi = 1;
    italic->newline = 0;
    italic->head = 0;
    return italic;
}
chunk* code_init() {
    chunk* code = (chunk *)malloc(sizeof(chunk));
    //get the size
    code->size = 1;
    //malloc the content
    code->arr = (char *)malloc(1);
    code->arr[0] = '\'';

    //set markers
    code->tbd = 0;
    code->tbi = 1;
    code->newline = 0;
    code->head = 0;
    return code;
 
}
chunk* link_left_init() {
    chunk* link = (chunk *)malloc(sizeof(chunk));
    //get the size
    link->size = 1;
    //malloc the content
    link->arr = (char *)malloc(1);
    link->arr[0] = '[';

    //set markers
    link->tbd = 0;
    link->tbi = 1;
    link->newline = 0;
    link->head = 0;
    return link;
}

chunk* link_right_init(char* content) {
    chunk* link = (chunk *)malloc(sizeof(chunk));
    //get the size
    link->size = 3 + strlen(content);
    //malloc the content
    link->arr = (char *)malloc(link->size);
    snprintf(link->arr, link->size + 1, "](%s)", content);

    //set markers
    link->tbd = 0;
    link->tbi = 1;
    link->newline = 0;
    link->head = 0;
    return link;
}

//CURSOR FUNCTIONS

struct chunk_data* get_chunk(int pos, chunk* head) {
    struct chunk_data* c_d = (struct chunk_data*)malloc(sizeof(struct chunk_data));
    int curr = 0;

    chunk* temp = head;
    while(1) {
        if(temp->tbi == 1) {
            //if this is something that has been inserted during this version, just skip it.

        }else if (curr + temp->size >= pos) {
            break;
        }else {
            curr += temp->size;
        }
        temp = temp->next;
    }
    c_d->chunk = temp;
    c_d->pos_in_arr = pos - curr;
    return c_d;
}

void connect_two(chunk* one, chunk* two) {
    one->next = two;
    two->prev = one;
    return;
}

void connect_three(chunk* one, chunk* two, chunk* three) {
    one->next = two;
    two->prev = one;

    two->next = three;
    three->prev = two;
    return;
}

chunk* split_chunk(struct chunk_data* c_d) {
    chunk* end_chunk = (chunk*)malloc(sizeof(chunk));

    end_chunk->size = c_d->chunk->size - c_d->pos_in_arr;

    //make the arr
    if(end_chunk->size > 0) {
        end_chunk->arr = (char*)malloc(end_chunk->size);
        //copy from original chunk to new chunk, from where the insert starts
        memcpy(end_chunk->arr, c_d->chunk->arr + c_d->pos_in_arr, end_chunk->size);
    }else{
        end_chunk->arr = NULL;
    }

    //adjust other side of the insert
    c_d->chunk->size = c_d->pos_in_arr;

        if(c_d->pos_in_arr > 0) {
            c_d->chunk->arr = realloc(c_d->chunk->arr, c_d->pos_in_arr);
        }
        else {
            c_d->chunk->arr = NULL;
        }

        //if the chunk was originally tbd, make sure its still gonna be deleted
        if(c_d->chunk->tbd == 1) {
            end_chunk->tbd =1;
        } else {
            end_chunk->tbd = 0;
        }

    return end_chunk;
}

chunk* copy_del(struct chunk_data* c_d, int len) {
    int i = 0;
    int cursor = len;

    chunk* end_chunk = (chunk*)malloc(sizeof(chunk));

    chunk* curr_chunk = c_d->chunk;

    while(cursor != 0) {
        i = 0;

    }

}

int in_list_check(chunk* chunka) {
    chunk* curr = chunka;
    //track back until we hit a newline.
    while(1) {
        if(curr->head == 1) {
            return -1;
        }else if (curr->newline == 1 && curr->tbi == 0) {
            break;
        }
        curr = curr->prev;
    }
    //ok so current is our closest newline. lets go to the following chunk that isnt to be inserted!
    //and isn't of size 0.
    while(1) {
        curr = curr->next;
        if(curr->tbi == 0 && curr->size > 0 ) {
            break;
        }
        if(curr->head == 1) {
            return -1;
        }
    }
    //we're gonna need atleast size 3 for this to be a list.
    if(curr->size >= 3) {
        if(curr->arr[0] >= '1' && curr->arr[0] <= '9'
            && curr->arr[1] == '.' && curr->arr[2] == ' ') {
                //this is the num that is above us.
                int num = curr->arr[0] - '0';
                //this will be our num
                num++;
                //we are in a ordered list woohoo!, we'll return what number this one should be.
                return num;
        } 
    }

    return 0;

}

void check_for_list_after(chunk* insert_chunk, int num) {
    chunk* curr = insert_chunk;
    while(1) {
       if(curr->head == 1) {
           //if we hit the end of the doc stop.
           break;
       }else if(curr->tbi == 1) {
           //if its gonna be inserted skip
           curr = curr->next;
       }else if(curr->newline == 1) {
           //if we hit a new line we now need to check its closest chunk.
           while(1) {
               curr = curr->next;
               if(curr->head == 1) {
                   //if we hit the head doing this break.
                   break;
               }else if(curr->tbi == 1) {
                   curr = curr->next;
               }else if(curr->arr[0] >= '1' && curr->arr[0] <= '9'
                   && curr->arr[1] == '.' && curr->arr[2] == ' ') {
                       num++;
                       curr->arr[0] = num + '0';
                       break;
               }else {
                   //if we have a case where there is a newline but no ordered list following, just loop throug to
                   //end of document so that we can break both loops.
                   while(curr->next->head != 1) {
                       curr = curr->next;
                   }
               }
           }
           if(curr->head == 1) {
               //have to do this twice to protect against making cursor null.
               break;
           }
           curr = curr->next;
       }else {
           curr = curr->next;
       }
    }
    return;
}

void list_cleanup(chunk* head) {

    chunk* curr = head->next;
    while(1) {
       if(curr->head == 1) {
           //if we hit the end of the doc stop.
           break;
       } else if(curr->newline == 1 || curr->prev == head) {
            int num = 1;
           //if we hit a new line we now need to check its closest chunk.
           while(1) {
                if(curr->head == 1) {
                    //if we hit the head doing this break.
                    break;
                }else if(curr->newline == 1 && curr->next->newline == 0){
                   // printf("newline, outcomes: %s, %d, %d, %d, %d\n\n", curr->next->arr, curr->next->arr[0] >= '1', curr->next->arr[0] <= '9', curr->next->arr[1] == '.', curr->next->arr[2] == ' ' );
                        //if theres one newline betweeen each just continue on.
                }else if(curr->arr[0] >= '1' && curr->arr[0] <= '9'
                        && curr->arr[1] == '.' && curr->arr[2] == ' ') {
                            //printf("num: %d\n\n", num);
                        curr->arr[0] = num + '0';
                        num++;
                }else{
                    break;
                }
                curr = curr->next;
           }
           if(curr->head != 1) {
                curr = curr->next;
           }
       }else {
           curr = curr->next;
       }
    }
    return;
}


//FREE FUNCTION
void free_chunk(chunk* chunk) {
    free(chunk->arr);
    free(chunk);
}

