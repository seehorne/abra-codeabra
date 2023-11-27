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

WINDOW* ui_win;

//int string_to

char* int_to_string(int i){
    char* return_val = malloc((sizeof(char)*2));
    int tens_place = i / 10;
    int ones_place = i % 10;
    if (tens_place != 0){
        char tens = ((char)tens_place) + '0';
        return_val[0]=tens;
        char ones = ((char)ones_place)+'0';
        return_val[1]=ones;
    }
    else{//i is less than 10
        char val = ((char)i + '0');
        return_val[0]= val;
        return_val[1]= ' ';
    }
    return return_val;
}


void write_contents(){
    int row = 0;
    int col = 0;
    for (; col < MAX_LINE_LENGTH; col++){
        mvaddch(row, col, ' ');
    }
    row++;
    col = 0;
    for (; row <= MAX_LINE_COUNT; row++){
        char* line_num = malloc((sizeof(char))*2);
        memcpy(line_num, int_to_string(row), 2);
        mvaddch(row, 0, line_num[0]);
        mvaddch(row, 1, line_num[1]);
        mvaddch(row, 2, ' ');
        // if (row < 10){
        //     mvaddch(row, 0, (row-'0'));
        // }
        // else{
        //     mvaddch(row, 0, 'X');
        // }
        for (col = 3; col <= MAX_LINE_LENGTH+ 2; col++){
        mvaddch(row, col, our_file->contents[row-1].line_contents[col-3]);
        }
        refresh();
    }
    move(0,0);
    refresh();
}

void overwrite_line(){
    FILE* log = fopen("log.txt", "w+");
    fprintf(log, "we can write\n");
    fflush(log);
    char c = getch();
    int i = 0;
    char line_num_rep[3];//make a char array of 2 slots for storing the line number
    line_num_rep[2]='\0';
    while (c != '\n'){
        if (c != ERR){
        if (i < 2){
        line_num_rep[i]= c;
        }
        i++;
        }
        c = getch();
    }
    if (i == 0){
        //some error message saying to put a line number
        return;
    }
    if (i == 1){//only one digit long
        line_num_rep[i]='\0';//end the string early
    }
    int line_num = atoi((const char*)line_num_rep)-1;//transform the line number rep to the index of the array
    //TODO: set the owner field of the line here
    fprintf(log, "line we read: %d)\n", line_num);
    fflush(log);
    //char overwrite_buf[MAX_LINE_LENGTH-1];
    char overwriting = getch();//works fine
    i = 0; //reset the index overwriter
    while (overwriting != '\n'){
        if (overwriting != ERR){
            fprintf(log, "char we allegedly read: %c\n", overwriting);
            fflush(log);
        if (i < MAX_LINE_LENGTH-1){
            our_file->contents[line_num].line_contents[i]=overwriting;//we did not save somehow. fuck.
            // overwrite_buf[i]=overwriting;
            // fprintf(log, "new buffer content new: %c\n", overwriting);
            // fflush(log);
        }
        i++;
        fprintf(log, "reading char: %c\n", our_file->contents[line_num].line_contents[i]);//fprintf for log file
        fflush(log);
        }
        overwriting = getch();// works fine
    }
    while (i < MAX_LINE_LENGTH-1){
        our_file->contents[line_num].line_contents[i]=' '; //pad with spaces again
        i++;
    }
    our_file->contents[line_num].line_contents[MAX_LINE_LENGTH-1]='\0';
    fprintf(log, "read line: %s\n", our_file->contents[line_num].line_contents);
    fflush(log);
    our_file->contents[line_num].line_contents[MAX_LINE_LENGTH-1]='\n';
    fseek(real_file->file_ref, 0, SEEK_SET);
    fseek(real_file->file_ref, (line_num * MAX_LINE_LENGTH), SEEK_CUR);//go to the right line in the file
    // real_file->file_ref += (line_num * MAX_LINE_LENGTH);
    fwrite(our_file->contents[line_num].line_contents, 1, MAX_LINE_LENGTH-1, real_file->file_ref); //write everything that changed (all but newline)
    // for (int i = 0; i < MAX_LINE_LENGTH-1; i++){
    //     *real_file->file_ref = our_file->contents[line_num].line_contents[i];
    //     real_file->file_ref++;
    // }
    //real_file->file_ref -= (line_num * MAX_LINE_LENGTH)+99;//put us back
    fseek(real_file->file_ref, 0, SEEK_SET);//put the cursor back at the top of the file
    int to_row = line_num+1; //+1 to undo the subtraction from indexing
    for (int col = 3; col <= MAX_LINE_LENGTH + 2; col++){
        mvaddch(to_row, col, our_file->contents[line_num].line_contents[col-3]);
        wrefresh(ui_win);
    }
    wrefresh(ui_win);
    move(0,0);//put the cursor back at the top
    wrefresh(ui_win);
    fclose(log);
}

// void draw_form() {//TODO: give this params bc threads will need it//doesn't actually use forms yet
//     initscr();  // Initialize ncurses
//     cbreak();
//     //noecho();
//     int row, col;
//     //getmaxyx(stdscr, row, col);
//     WINDOW *form = newwin(MAX_LINE_COUNT+3, MAX_LINE_LENGTH+3, 1, 1);
//     nodelay(form, true);
//     keypad(form, true);
//     box(form, 0, 0);
//     for (int i = 0; i < MAX_LINE_COUNT; i++) {
//         char line_copy[MAX_LINE_LENGTH];
//         memcpy(line_copy,our_file->contents[i].line_contents, MAX_LINE_LENGTH);
//         line_copy[MAX_LINE_LENGTH-1]='\0'; //replace the newline with null terminator
//         mvwprintw(form, i + 1, 1, "%s", line_copy);
//     }
//     wrefresh(form);
//     //post_form(form);

//     ui_run();
   
//    while (true){
//     int ch;
//     //wrefresh(form);
//     do {//if you copy this part out the form shows up
//         ch = getch();
//         wrefresh(form);
//     } while (ch != 'q');//this works but WHERE TF IS THE FORM
//     break;
//    }
//     endwin();
    
// }


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
    //ui_init(ui_win);
    ui_win = initscr();
    keypad(ui_win, true);   // Support arrow keys
    //nodelay(ui_win, true);  // Non-blocking keyboard access
    mousemask(ALL_MOUSE_EVENTS, NULL); //listen to all the stuff a mouse can do  
    write_contents();
    //draw_form();
    while(true){
        overwrite_line();
    }
    fclose(real_file->file_ref);
    endwin();
    return 0;
} 
