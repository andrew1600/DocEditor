#include "../libs/markdown.h"

#define SUCCESS 0 

// === Init and Free ===
document *markdown_init(void) {
    document * doc = (document*)malloc(sizeof(document));
    doc->size = 0;
    doc->version = 0;
    chunk* front_head = head_init();
    chunk* back_head = head_init();

    doc->HEAD = front_head;
    front_head->prev = NULL;
    back_head->next = NULL;

    connect_two(front_head, back_head);

    return doc;
}

void markdown_free(document *doc) {
    chunk* curr = doc->HEAD;
    while(curr != NULL) {
        chunk* temp = curr;
        curr = curr->next;
        free_chunk(temp);
    }

    free(doc);
    return;
    //(void)doc;
}

// === Edit Commands ===
int markdown_insert(document *doc, uint64_t version, size_t pos, const char *content) {
    if(pos > doc->size || pos < 0) {
        return -1;
    } else if(doc->version != version) {
        return -3;
    }
    //let's get the new chunk first.
    chunk* insert_chunk = insert_init(content);

    //let's get chunk data
    struct chunk_data* c_d = get_chunk(pos, doc->HEAD);


    //we'll split the chunk we're currently in.
    chunk* end_chunk = split_chunk(c_d);
    //then set the markers
    //end_chunk->tbd = 0;
    end_chunk->tbi = 0;
    end_chunk->newline = 0;
    end_chunk->head = 0;
    
    //now i'll connect the new end to the old next
    connect_two(end_chunk, c_d->chunk->next);

    //then i'll connect the three new chunks
    connect_three(c_d->chunk, insert_chunk, end_chunk);

    //free the data!
    free(c_d);
    //(void)doc; (void)version; (void)pos; (void)content;
    return SUCCESS;
}

int markdown_delete(document *doc, uint64_t version, size_t pos, size_t len) {
    if(pos > doc->size || pos < 0) {
        return -1;
    } else if(doc->version != version) {
        return -3;
    }
    //if len goes past the end of the document, set it to the end of the document.
    if(len > (doc->size - pos)) {
        len = doc->size - pos;
    }
    struct chunk_data* c_d_sta = get_chunk(pos, doc->HEAD);
    struct chunk_data* c_d_end = get_chunk(pos+len, doc->HEAD);


    //split the back region of the deleted section from its chunk.
    chunk* end_chunk = split_chunk(c_d_end);
    //set up it's markers
    end_chunk->tbi = 0;
    //end_chunk->tbd = 0;
    end_chunk->newline = 0;
    end_chunk->head = 0;
    //now connect it's back to rest of document.
    connect_two(end_chunk, c_d_end->chunk->next);
    //now connect its front to rest of document
    connect_two(c_d_end->chunk, end_chunk);
    


    chunk* start_chunk = split_chunk(c_d_sta);

    //set up markers.
    start_chunk->tbi = 0;
    start_chunk->tbd = 1;
    start_chunk->newline = 0;
    start_chunk->head = 0;

    //now connect it's back to rest of document.
    connect_two(start_chunk, c_d_sta->chunk->next);
    //now connect its front to rest of document
    connect_two(c_d_sta->chunk, start_chunk); 



    //loop through and mark them all to be be deleted.
    chunk* curr = start_chunk;
    while(curr != end_chunk) {
        if(curr->tbi == 1) {
            //do nothing for this one since its gonna be inserted.
        }
        else{
            curr->tbd = 1;
        }

        if(curr->head == 1) {
            //if we hit the header at the end just stop.
            break;
        }
        curr = curr->next;
    }

    free(c_d_sta);
    free(c_d_end);
    //(void)doc; (void)version; (void)pos; (void)len;
    return SUCCESS;
}

// === Formatting Commands ===
int markdown_newline(document *doc, uint64_t version, size_t pos) {
    if(pos > doc->size || pos < 0) {
        return -1;
    } else if(doc->version != version) {
        return -3;
    }
    //let's get the new chunk first.
    chunk* insert_chunk = newline_init();

    //let's get chunk data
    struct chunk_data* c_d = get_chunk(pos, doc->HEAD);


    //we'll split the chunk we're currently in.
    chunk* end_chunk = split_chunk(c_d);
    //then set the markers
    //end_chunk->tbd = 0;
    end_chunk->tbi = 0;
    end_chunk->newline = 0;
    end_chunk->head = 0;
    
    //now i'll connect the new end to the old next

    
    connect_two(end_chunk, c_d->chunk->next);

    //then i'll connect the three new chunks
    connect_three(c_d->chunk, insert_chunk, end_chunk);
    
    //free the data!
    free(c_d);
    //(void)doc; (void)version; (void)pos; (void)content;
    return SUCCESS;


}

int markdown_heading(document *doc, uint64_t version, size_t level, size_t pos) {
    if(pos > doc->size || pos < 0) {
        return -1;
    } else if(doc->version != version) {
        return -3;
    }
    //lets get chunk_data.
    struct chunk_data* c_d = get_chunk(pos, doc->HEAD);

     //let's get the new chunk 
     chunk* insert_chunk = heading_init(level);


    //we'll split the chunk we're currently in.
    chunk* end_chunk = split_chunk(c_d);
    //then set the markers
    //end_chunk->tbd = 0;
    end_chunk->tbi = 0;
    end_chunk->newline = 0;
    end_chunk->head = 0;
    
    //now i'll connect the new end to the old next
    connect_two(end_chunk, c_d->chunk->next);

    //then i'll connect the three new chunks
    connect_three(c_d->chunk, insert_chunk, end_chunk);

    
/*
    //do i need a newline? or is the immediately preceding char a newline?
    if(pos == 0 || c_d->pos_in_arr != 0) {
        //in case im at the front head, shouldn't be able to reach the back head.
        markdown_newline(doc, version, pos);
    }else {
        struct chunk_data* prev_c_d = get_chunk(pos -1, doc->HEAD);
        if(prev_c_d->chunk->newline == 1) {

        }else {
            markdown_newline(doc, version, pos);
        }
        free(prev_c_d);
    }
        */
       if(c_d->chunk->newline == 1 || c_d->chunk->head == 1) {

       }else {
           markdown_newline(doc, version, pos);
       }


    
    return SUCCESS;
}

int markdown_bold(document *doc, uint64_t version, size_t start, size_t end) {
    struct chunk_data* c_d_sta = get_chunk(start, doc->HEAD);
    struct chunk_data* c_d_end = get_chunk(end, doc->HEAD);
    char* bold = (char *)malloc(3);
    strcpy(bold, "**");

    if(c_d_sta->chunk->tbd == 1 && c_d_end->chunk->tbd == 1) {
        return -2;
    }else {
        markdown_insert(doc, version, start, bold);
        markdown_insert(doc, version, end, bold);
    }
    free(c_d_sta);
    free(c_d_end);
    free(bold);

    return SUCCESS;
}

int markdown_italic(document *doc, uint64_t version, size_t start, size_t end) {
    struct chunk_data* c_d_sta = get_chunk(start, doc->HEAD);
    struct chunk_data* c_d_end = get_chunk(end, doc->HEAD);
    char* italic = (char *)malloc(2);
    strcpy(italic, "*");

    if(c_d_sta->chunk->tbd == 1 && c_d_end->chunk->tbd == 1) {
        return -2;
    }else {
        markdown_insert(doc, version, start, italic);
        markdown_insert(doc, version, end, italic);
    }
    free(c_d_sta);
    free(c_d_end);
    free(italic);

    return SUCCESS;
}

int markdown_blockquote(document *doc, uint64_t version, size_t pos) {
    if(pos > doc->size || pos < 0) {
        return -1;
    } else if(doc->version != version) {
        return -3;
    }
    //lets get chunk_data.
    struct chunk_data* c_d = get_chunk(pos, doc->HEAD);

     //let's get the new chunk 
     chunk* insert_chunk = bq_init();


    //we'll split the chunk we're currently in.
    chunk* end_chunk = split_chunk(c_d);
    //then set the markers
    //end_chunk->tbd = 0;
    end_chunk->tbi = 0;
    end_chunk->newline = 0;
    end_chunk->head = 0;
    
    //now i'll connect the new end to the old next
    connect_two(end_chunk, c_d->chunk->next);

    //then i'll connect the three new chunks
    connect_three(c_d->chunk, insert_chunk, end_chunk);

    
//check if we need a new line before.
    if(c_d->chunk->newline == 1 || c_d->chunk->head == 1) {

    }else {
        markdown_newline(doc, version, pos);
    }

  
    
    return SUCCESS;
}

int markdown_ordered_list(document *doc, uint64_t version, size_t pos) {
    if(pos > doc->size || pos < 0) {
        return -1;
    } else if(doc->version != version) {
        return -3;
    }

    //let's get chunk data
    struct chunk_data* c_d = get_chunk(pos, doc->HEAD);

    //lets check if we're in a list!. and what number do we need?
    int num = in_list_check(c_d->chunk);

    if(num >= 2 && num <= 9) {
        
    } else {
        num = 1;
    }

     //let's get the new chunk 
     chunk* insert_chunk = ol_init(num);


    //we'll split the chunk we're currently in.
    chunk* end_chunk = split_chunk(c_d);
    //then set the markers
    //end_chunk->tbd = 0;
    end_chunk->tbi = 0;
    end_chunk->newline = 0;
    end_chunk->head = 0;
    
    //now i'll connect the new end to the old next

    
    connect_two(end_chunk, c_d->chunk->next);

    //then i'll connect the three new chunks
    connect_three(c_d->chunk, insert_chunk, end_chunk);

      
    if(c_d->chunk->newline == 1 || c_d->chunk->head == 1) {

    }else {
        markdown_newline(doc, version, pos);
    }

  

    //free the data!
    free(c_d);

    return SUCCESS;
}




int markdown_unordered_list(document *doc, uint64_t version, size_t pos) {
    if(pos > doc->size || pos < 0) {
        return -1;
    } else if(doc->version != version) {
        return -3;
    }
    //lets get chunk_data.
    struct chunk_data* c_d = get_chunk(pos, doc->HEAD);

     //let's get the new chunk 
     chunk* insert_chunk = ul_init();


    //we'll split the chunk we're currently in.
    chunk* end_chunk = split_chunk(c_d);
    //then set the markers
    //end_chunk->tbd = 0;
    end_chunk->tbi = 0;
    end_chunk->newline = 0;
    end_chunk->head = 0;
    
    //now i'll connect the new end to the old next
    connect_two(end_chunk, c_d->chunk->next);

    //then i'll connect the three new chunks
    connect_three(c_d->chunk, insert_chunk, end_chunk);

    //check if we need a newline.

    if(c_d->chunk->newline == 1 || c_d->chunk->head == 1) {

    }else {
        markdown_newline(doc, version, pos);
    }


    
    
    return SUCCESS;
}

int markdown_code(document *doc, uint64_t version, size_t start, size_t end) {
    struct chunk_data* c_d_sta = get_chunk(start, doc->HEAD);
    struct chunk_data* c_d_end = get_chunk(end, doc->HEAD);
    char* code = (char *)malloc(2);
    strcpy(code, "`");

    if(c_d_sta->chunk->tbd == 1 && c_d_end->chunk->tbd == 1) {
        return -2;
    }else {
        markdown_insert(doc, version, start, code);
        markdown_insert(doc, version, end, code);
    }
    free(c_d_sta);
    free(c_d_end);
    free(code);

    return SUCCESS;

}

int markdown_horizontal_rule(document *doc, uint64_t version, size_t pos) {
    if(pos > doc->size || pos < 0) {
        return -1;
    } else if(doc->version != version) {
        return -3;
    }
    //do i need a newline at the back? 
    if(pos == doc->size) {
        //in case im at the front head, shouldn't be able to reach the back head.
        markdown_newline(doc, version, pos);
    }else {
        struct chunk_data* prev_c_d = get_chunk(pos+1, doc->HEAD);
        if(prev_c_d->chunk->newline == 1) {

        }else {
            markdown_newline(doc, version, pos);
        }
        free(prev_c_d);
    }


    //lets get chunk_data.
    struct chunk_data* c_d = get_chunk(pos, doc->HEAD);

     //let's get the new chunk 
     chunk* insert_chunk = hr_init();


    //we'll split the chunk we're currently in.
    chunk* end_chunk = split_chunk(c_d);
    //then set the markers
    //end_chunk->tbd = 0;
    end_chunk->tbi = 0;
    end_chunk->newline = 0;
    end_chunk->head = 0;
    
    //now i'll connect the new end to the old next
    connect_two(end_chunk, c_d->chunk->next);

    //then i'll connect the three new chunks
    connect_three(c_d->chunk, insert_chunk, end_chunk);

    

    //do i need a newline at the front? or is the immediately preceding char a newline?
  
    if(c_d->chunk->newline == 1 || c_d->chunk->head == 1) {

    }else {
        markdown_newline(doc, version, pos);
    }


    
    return SUCCESS;
}

int markdown_link(document *doc, uint64_t version, size_t start, size_t end, const char *url) {
    struct chunk_data* c_d_sta = get_chunk(start, doc->HEAD);
    struct chunk_data* c_d_end = get_chunk(end, doc->HEAD);
    char* link_a = (char *)malloc(2);
    strcpy(link_a, "[");

    int len_of_url = strlen(url);
    char* link_b = (char *)malloc(len_of_url + 4);
    snprintf(link_b, len_of_url +4, "](%s)", url );

    if(c_d_sta->chunk->tbd == 1 && c_d_end->chunk->tbd == 1) {
        return -2;
    }else {
        markdown_insert(doc, version, start, link_a);
        markdown_insert(doc, version, end, link_b);
    }
    free(c_d_sta);
    free(c_d_end);
    free(link_a);
    free(link_b);

    return SUCCESS;
}

// === Utilities ===
char *markdown_flatten(const document *doc) {
    char* msg = (char*)malloc(doc->size + 1);

    chunk* curr = doc->HEAD->next;
    int len = 0;
    //loop through and memcpy everything to msg until we hit the head.

    while(1) {
        if(curr->head == 1) {
            break;
        }else if(curr->tbi == 1) {

        } else {
                memcpy(msg+len, curr->arr, curr->size);
                len += curr->size;
        }
        curr = curr->next;
    }
    msg[doc->size] = '\0';
    
    return msg;

}

void markdown_print(const document *doc, FILE *stream) {
    char * text = markdown_flatten(doc);
    fprintf(stream, "%s", text);
    free(text);
    return; 
}

// === Versioning ===
void markdown_increment_version(document *doc) {
    chunk* curr = doc->HEAD->next;

    chunk* fill = curr;

    //len will keep the amount of each chunk as it is collated together
    int len = 0;
    int lenb = 0;
    //size will maintain the total size of the doc.
    int size_of_doc = 0;

    while(1) {
        if(curr->head == 1) {
            //when we hit the back head stop.
            size_of_doc += len;
            break;
        } else if(curr->tbd == 1 || curr->size == 0) {
            //if we hit a tbd or the chunk has nothing in it, free it and continue on
            if(fill == curr) {
               fill = fill->next;
            }
            chunk* temp = curr;
            connect_two(temp->prev, temp->next);
            curr = curr->next;
            free_chunk(temp);
            continue;
       } else if (curr->newline == 1) {
            //in the case that its just been inserted, set its tbi to 0
            curr->tbi = 0;
            //if we hit a newline character, shift to fill following chunk
            fill = curr->next;
            //+1 for newline as well.
            size_of_doc += len + 1;
            //now reset as we collate the chunk after the newline.
            len = 0;
            lenb = 0;

        }else {
            //if we hit a tbi or a regular chunk then add it to fill
            len += curr->size;
            if(fill->arr == NULL) {
                fill->arr = (char *)malloc(len);
            } else {
                fill->arr = realloc(fill->arr, len);
            }

            memcpy(fill->arr + lenb, curr->arr, curr->size);

            fill->size = len;

            lenb += curr->size;

            if(fill != curr) {
                chunk* temp = curr;
                connect_two(temp->prev, temp->next);
                curr = curr->next;
                free_chunk(temp);
            }else {
                curr = curr->next;
                fill->tbi = 0;
            }

             continue;
        }

        curr = curr->next;
    }
    list_cleanup(doc->HEAD);
    //update size of doc
    doc->size = size_of_doc;
    doc->version++;
    return;
}

