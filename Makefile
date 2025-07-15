# TODO: make sure the rules for server client and markdown filled!
CC := gcc
CFLAGS := -Wall -Wextra

all: server client

server: 

client:

# Object file rules
document.o: source/document.c
	gcc -c source/document.c -o document.o

markdown_file.o: source/markdown.c
	gcc -c source/markdown.c -o markdown_file.o

markdown.o: markdown_file.o document.o
	ld -r document.o markdown_file.o -o markdown.o

# Test program that links everything
test/test: test/mkd_insert_tests.c markdown.o
	gcc test/mkd_insert_tests.c markdown.o -o test/test

server_misc.o : source/server_misc.c
	gcc -c source/server_misc.c -o server_misc.o

server: server_misc.o markdown.o
	gcc source/server.c server_misc.o markdown.o -o server

client: server
	gcc source/client.c server_misc.o markdown.o -o client

clean:
	rm *.o
	rm client
	rm server
	rm doc.md
	rm FIFO*