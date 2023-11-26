#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <errno.h> 
#include <fcntl.h> 
#include <sys/stat.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <form.h>
#include "ui.h"

#define MAX_LINE_LENGTH 101 //100 chars plus a newline
#define MAX_LINE_COUNT 40

const char* username;
const char* filename;

//filestream with a lock around it (so we can easily update lines)
typedef struct locking_file{
    FILE* file_ref; //<reference to the actual file
    pthread_mutex_t file_lock;
} locking_file_t;

//structure for a file line, contents char[], int to represent who owns it
typedef struct file_line {
    char line_contents[MAX_LINE_LENGTH];
    int owner; //which thread is currently holding it, -1 means unheld
} file_line_t;

//data structure representing the contents of a file
typedef struct file_rep {
    file_line_t contents[MAX_LINE_COUNT];
}file_rep_t;

//file (the stream, actual writing thing) and file data structure bundled for threads to have copies
typedef struct thread_file_package{
    locking_file_t* document; //<writable stream
    file_rep_t* representation; //<data structure with file contents
} thread_file_package_t;

file_rep_t* our_file; //global struct to contain the file representation

locking_file_t* real_file; //global struct to contain the actual file


void draw_form() {//TODO: give this params bc threads will need it//doesn't actually use forms yet
    initscr();  // Initialize ncurses
    cbreak();
    //noecho();
    int row, col;
    //getmaxyx(stdscr, row, col);
    WINDOW *form = newwin(MAX_LINE_COUNT+3, MAX_LINE_LENGTH+3, 1, 1);
    nodelay(form, true);
    keypad(form, true);
    box(form, 0, 0);
    for (int i = 0; i < MAX_LINE_COUNT; i++) {
        char line_copy[MAX_LINE_LENGTH];
        memcpy(line_copy,our_file->contents[i].line_contents, MAX_LINE_LENGTH);
        line_copy[MAX_LINE_LENGTH-1]='\0'; //replace the newline with null terminator
        mvwprintw(form, i + 1, 1, "%s", line_copy);
    }
    wrefresh(form);
    post_form(form);

   int ch;
    do {//if you copy this part out the form shows up
        ch = getch();
        //wrefresh(form);
    } while (ch != 'q');//this works but WHERE TF IS THE FORM

    endwin();
}


/**

* This function is run whenever the user hits enter after typing a message

* Sends the message to the original sender's connection

* \param message the message typed in from the user interaction pane

**/

void input_callback(const char* message) {
  if (strcmp(message, ":quit") == 0 || strcmp(message, ":q") == 0) {
    ui_exit();
  } else {
    ui_display(username, message);
  }
}


int main(int argc, char **argv){
    //ui_init(input_callback);
    bool pre_existing = true;
    our_file = malloc(sizeof(file_rep_t));
    real_file = malloc(sizeof(locking_file_t));
    pthread_mutex_init(&real_file->file_lock, NULL);

    if(argc != 3 && argc !=4){
        fprintf(stderr, "Usage: %s <username>  <filepath> OR %s <username> <hostname> <port number>]\n", argv[0], argv[0]);
    }
    if (argc == 3){ //setup for session host. run program, username, and filename
        username = argv[1];
        filename = argv[2];
    // TODO:  ncurses 
    //O_CREAT | O_RDWR| O_EXCL
    //read the specified file into the data structure
    chmod(filename, 00777);//set rwx to good for everyone (if it exists, this doesn't do anything otherwise)
    real_file->file_ref = fopen(filename, "r+");//tries to open the file (does not overwrite)
    if (real_file->file_ref ==NULL){//the file does not exist
        pre_existing = false;
        real_file->file_ref = fopen(filename, "w+"); //create the file if it doesn't exist which it doesn't
    }
    //printf("did not crash trying to r+x open the file\n");
    //when we launch threads, we probably want the void* to include the global file_rep as well
    char put_this;
    for (int file_lines = 0; file_lines < MAX_LINE_COUNT; file_lines++){
        put_this=fgetc(real_file->file_ref);//read the first char of the file, for a starting off place
        for (int chars = 0; chars < (MAX_LINE_LENGTH-1); chars++){//if we didn't just break, start reading characters into lines
            if (put_this != EOF && put_this != '\n'){//if it's the newline or EOF, stop reading into this line
                our_file->contents[file_lines].line_contents[chars]= put_this;
                put_this = fgetc(real_file->file_ref);//read for next time
            }
            else{
                if (chars < (MAX_LINE_LENGTH-1)){//not the very last char in array
                    our_file->contents[file_lines].line_contents[chars]= ' '; //fill up to end of line with spaces, for navigational ease (pointer math of line lengths)
                }
                else{//very last char in line should be the null terminator
                    our_file->contents[file_lines].line_contents[chars]= '\n';
                }
            }
        }
        //if the 100 characters was not enough, oops, sorry your line gets cut off, should have read the rules
        while (put_this != '\n' && put_this != EOF){//enters this loop if 100 characters was not enough
            put_this = fgetc(real_file->file_ref); //don't store it, just walk through it
        }
        our_file->contents[file_lines].line_contents[MAX_LINE_LENGTH-1]= '\n';
    }
    if (pre_existing){
        freopen(filename, "w+", real_file->file_ref);//overwrite it so we can write our data structure to it
    }
    for (int num_lines = 0; num_lines < MAX_LINE_COUNT; num_lines++){
        if (num_lines == MAX_LINE_COUNT-1){
            fwrite(our_file->contents[num_lines].line_contents, 1, MAX_LINE_LENGTH-1, real_file->file_ref); //write everything but the newline for the last line
        }
        else{
            fwrite(our_file->contents[num_lines].line_contents, 1, MAX_LINE_LENGTH, real_file->file_ref); //write the whole line, including newline
        }
    }
    fseek(real_file->file_ref, 0, SEEK_SET); //puts the pointer back at the start of the file
    // WINDOW* mainwin = initscr();
    // fclose(real_file->file_ref);
    // if (mainwin == NULL) {
    //     fprintf(stderr, "Error initializing ncurses.\n");
    //     exit(2);
    // }
    // printw("Hello world!");
    // refresh();
    // for (int i = 0; i < MAX_LINE_COUNT; i++){
    //     printw("|\n");
    //     refresh();
    // }
    // sleep(1000);
    // endwin();
    }
    if (argc == 4){//connecting to editing session
        //TODO: set up connection
    }
    draw_form();
    return 0;
} 
