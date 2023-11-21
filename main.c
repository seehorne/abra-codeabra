#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <errno.h> 
#include <fcntl.h> 
#include <sys/stat.h>

#define MAX_LINE_LENGTH 100
#define MAX_LINE_COUNT 40

const char* username;
const char* filename;

typedef struct file_line {
    char line_contents[MAX_LINE_LENGTH];
    int owner; //which thread is currently holding it, -1 means unheld
} file_line_t;

typedef struct file_rep {
    file_line_t contents[MAX_LINE_COUNT];
}file_rep_t;

file_rep_t* our_file; //global struct to contain the file

int main(int argc, char **argv){
    if (argc == 3){ //run program, username, and filename
        username = argv[1];
        filename = argv[2];
    }
    our_file = malloc(sizeof(file_rep_t));
    // TODO: Open and read a file in ncurses 
    //O_CREAT | O_RDWR| O_EXCL
    
    //read the specified file into the data structure
    chmod(filename, 00777);//set rwx to good for everyone (if it exists, this doesn't do anything otherwise)
    FILE* file = fopen(filename, "r+");//tries to open the file (does not overwrite)
    if (file==NULL){//the file does not exist
        file = fopen(filename, "w+"); //create the file if it doesn't exist which it doesn't
    }
    //printf("did not crash trying to r+x open the file\n");
    //when we launch threads, we probably want the void* to include the global file_rep as well
    char put_this=fgetc(file);//read the first char of the file, for a starting off place
    for (int file_lines = 0; file_lines < MAX_LINE_COUNT; file_lines++){
        printf("\n");
        if (put_this == EOF){//if at the start, the file is empty, you can just leave
            break; //leave the reading loop
        }
        for (int chars = 0; chars < MAX_LINE_LENGTH; chars++){//if we didn't just break, start reading characters into lines
            if (put_this == EOF){//if it's the newline or EOF, stop reading into this line
                break; //leave the reading loop for this line
            }
            else if (put_this == '\n'){//if it was a newline, add the null terminator to string, then break reading loop
                our_file->contents[file_lines].line_contents[chars]= '\0';
            }
            printf("%c", put_this);
            our_file->contents[file_lines].line_contents[chars]= put_this;
            put_this = fgetc(file);//read for next time
        }
        //if the 100 characters was not enough, oops, sorry your line gets cut off, should have read the rules
        while (put_this != '\n' && put_this != EOF){//enters this loop if 100 characters was not enough
            put_this = fgetc(file); //don't store it, just walk through it
        }
    }
    fseek(file, 0, SEEK_SET); //puts the pointer back at the start of the file
    fclose(file);
    return 0;
} 
