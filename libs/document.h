#ifndef DOCUMENT_H

#define DOCUMENT_H
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include<unistd.h>
#include <sys/types.h>
#include<sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>

/**
 * This file is the header file for all the document functions. You will be tested on the functions inside markdown.h
 * You are allowed to and encouraged multiple helper functions and data structures, and make your code as modular as possible. 
 * Ensure you DO NOT change the name of document struct.
 */

typedef struct chunk{
    //TODO
    struct chunk* next;
    struct chunk* prev;
    char* arr;
    int size;
    //tbd - to be deleted
    bool tbd;
    //tbi to be inserted
    bool tbi;
    //is it a newline?
    bool newline;
    //is it a heading.
    bool head;
} chunk;

typedef struct {
    // TODO
    chunk* HEAD;
    int size;
    int version;
} document;

struct chunk_data {
    chunk* chunk;
    int pos_in_arr;
};

// Functions from here onwards.
//init functions.
 chunk*head_init();
//EDITING
chunk* insert_init(const char* content);
chunk* delete_init(int size);

//FORMATTING
chunk* newline_init();

//BLOCKQUOTES
chunk* heading_init(int level);
chunk* bq_init();
chunk* ol_init(int num);
chunk* ul_init();
chunk* hr_init();

//INLINE
chunk* bold_init();
chunk* italic_init();
chunk* code_init();
chunk* link_init();

//CURSOR FUNCTIONS
struct chunk_data* get_chunk(int pos, chunk* head);
void connect_two(chunk* one, chunk*two);
void connect_three(chunk* one, chunk* two, chunk* three);
chunk* split_chunk(struct chunk_data* c_d);
int in_list_check(chunk* chunk);
void check_for_list_after(chunk* insert_chunk, int num);
void list_cleanup(chunk* head);

//FREE FUNCTION
void free_chunk(chunk* chunk);



#endif
