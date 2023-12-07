#pragma once

#define MAX_MESSAGE_LENGTH 2048

// #define MAX_LINE_LENGTH 101 //100 chars plus a newline
// #define MAX_LINE_COUNT 40

// //structure for a file line, contents char[], int to represent who owns it
// typedef struct file_line file_line_t;

// //data structure representing the contents of a file
// typedef struct file_rep file_rep_t;

//structure for a file line, contents char[], int to represent who owns it

// Send a across a socket with a header that includes the message length. Returns non-zero value if
// an error occurs.
int send_message(int fd, char* message);

// Receive a message from a socket and return the message string (which must be
//freed later).
// Returns NULL when an error occurs.
char* receive_message(int fd);

//int send_file(int fd, void* message);
