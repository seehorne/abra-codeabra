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
#include "socket.h"
#include "message.h"

#define MAX_LINE_LENGTH 101 //100 chars plus a newline
#define MAX_LINE_COUNT 40
#define HOST_RUN 3
#define CLIENT_RUN 4


const char* username;
const char* filename;
FILE* log_f;
FILE* log_f2;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
char buffer[100];
char* claim_indicators[MAX_LINE_COUNT];

/**
 * struct for 1 node in a linked list
 * */
typedef struct list_node {
  int data;            //<the entry (a file descriptor, represents a socket)
  struct list_node* next;  //<the next node in the list
} list_node_t; 

/**
 * struct for a linked_list
 * */
typedef struct list {
  list_node_t* head;          //<head of the linked list
} list_t;

//make the list of connections global so all threads can get to it
list_t* users = NULL;

//filestream with a lock around it (so we can easily update lines)
typedef struct locking_file{
    FILE* file_ref; //<reference to the actual file
    pthread_mutex_t file_lock;
} locking_file_t;

//structure for a file line, contents char[], int to represent who owns it
typedef struct file_line {
    char line_contents[MAX_LINE_LENGTH];
    int num_of_owners; // how many people are using this line at this moment
} file_line_t;

//data structure representing the contents of a file
typedef struct file_rep {
    file_line_t contents[MAX_LINE_COUNT];
}file_rep_t;

//file (the stream, actual writing thing) and file data structure bundled for threads to have copies
typedef struct thread_file_package{
    file_rep_t* representation; //<data structure with file contents
    uint8_t port_num;
} thread_file_package_t;

// Define a structure for basic information passing
typedef struct info_passing {
  int port;  // Port number for communication
  int argc;  // Number of command-line arguments
} info_passing_t;

// Define a structure for extended information passing
typedef struct extended_info_passing {
  info_passing_t info;  // Basic information structure
  char** argv;          // Array of command-line argument strings
} extended_info_passing_t;

file_rep_t* our_file; //global struct to contain the file representation

locking_file_t* real_file; //global struct to contain the actual file

WINDOW* ui_win;

/**
 * Converts an integer to a string.
 * 
 * This function works for both client and server applications.
 * It allocates memory for a character array to store the result.
 * 
 * @param i The integer to be converted.
 * @return A dynamically allocated string representing the integer.
 */
char* int_to_string(int i){//works for client and server
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

/**
 * Writes the contents of the file to the UI.
 *
 * This function works for both client and server applications.
 * It clears the designated area on the UI, writes line numbers,
 * and displays the contents of the file line by line.
 * 
 * @param argc An integer indicating the mode of execution (CLIENT_RUN or SERVER_RUN).
 *             Used to determine whether to display line claim indicators for the client.
 */
void write_contents(int argc){//works for client and server
    int row = 1;
    int col = 0;
    for (; col < MAX_LINE_LENGTH; col++){
        mvaddch(row, col, ' ');
    }
    row++;
    col = 0;
    for (; row <= MAX_LINE_COUNT+1; row++){
        char* line_num = malloc((sizeof(char))*2);//has been freed
        memcpy(line_num, int_to_string(row-1), 2);
        if (argc == CLIENT_RUN){
          mvaddstr(row, 0, claim_indicators[row-2]);//add whatever line claim is currently true
        }
        else{
        mvaddch(row, 0, ' ');//leave space for asterisks
        }
        mvaddch(row, 1, line_num[0]);
        mvaddch(row, 2, line_num[1]);
        mvaddch(row, 3, ' ');
        
        for (col = 4; col <= MAX_LINE_LENGTH+ 3; col++){
        mvaddch(row, col, our_file->contents[row-2].line_contents[col-4]);
        }
        
        wrefresh(ui_win);
        free(line_num);
    }
    move(1,10);
    refresh();
}

/**
 * Distributes a message to all connected clients.
 *
 * This function is designed for both the host and clients. The host sends the message
 * to all clients, and clients forward the message to other clients. If the provided
 * line number representation or message is null, or if the client is no longer connected,
 * the function removes the client from the list of connected clients.
 * 
 * @param client_socket_fd The file descriptor of the client's socket.
 * @param message The message to be distributed.
 * @param line_num_rep The line number representation associated with the message.
 * @param argc An integer indicating the mode of execution (HOST_RUN or CLIENT_RUN).
 * @return 0 on success, -1 if there's an error in sending the message.
 */
int distribute(int client_socket_fd, char* message, char* line_num_rep, int argc){
  //HOST & CLIENTS: Recieve and Distribute to the rest of the clients
  if(line_num_rep == NULL || message == NULL) { 
    //if the message received is null or the client is no longer connected, delete the client holding this client_socket_fd 
    pthread_mutex_lock(&lock);
    if (users != NULL){
      list_node_t* temp = users->head; 
      if(temp->data == client_socket_fd){// if the client to be removed is the top client
          users->head = temp->next;
          free(temp);
      }else{ // if the client isn't the first in the linked list find it and remove it
        list_node_t* prev = temp;
        temp = temp->next;
        //loop through the link list to find the client that has this client socket fd
        while(temp!= NULL){
          if(temp->data == client_socket_fd){
            prev->next = temp->next;
            free(temp);
            break;
          }
          prev = temp;
          temp = temp->next;
        }
      }
    }
    pthread_mutex_unlock(&lock);
    return -1;
  }else{
    if(argc == HOST_RUN){ // only the host needs to send to everyone
      pthread_mutex_lock(&lock);
      if (users != NULL){
      list_node_t* temp = users->head; 
      while(temp!= NULL){
        //check to send line number and message to other connected clients
        if(temp->data != client_socket_fd || strcmp(message, " ")==0){//if it's a space, send it to who you got it from anyways
          int rc = send_message(temp->data, line_num_rep);
          if(rc == -1) {
            perror("Failed to send line number to client");
            pthread_mutex_unlock(&lock);
            exit(EXIT_FAILURE);
          }
        
          rc = send_message(temp->data, message);
          if(rc == -1) {
            perror("Failed to send message to client");
            pthread_mutex_unlock(&lock);
            exit(EXIT_FAILURE);
          }
        }
        temp = temp->next;
      }
      pthread_mutex_unlock(&lock);
    }
    }
  }
  return 0; //0 on success
}

/**
 * Overwrites a specific line in the file and distributes the changes to connected clients.
 *
 * This function prompts the user to enter a line number, then collects the new content for that line. 
 * The function updates the file representation and the actual file on the server. It also sends the 
 * changes to connected clients if the server is in HOST_RUN mode. If the server is in CLIENT_RUN mode, 
 * it sends the changes to the host. The function ensures proper synchronization using mutex locks.
 * 
 * @param argc An integer indicating the mode of execution (HOST_RUN or CLIENT_RUN).
 * @return 0 on success, -1 if there's an error in sending the message, -2 if the line number
 *         is too large, -3 if there's an error in converting the line number, -5 if the host has left.
 */
int overwrite_line(int argc){
  char line_num_rep[3];//make a char array of 2 slots for storing the line number
  line_num_rep[2]='\0';
  
  while (strcmp(line_num_rep, ":q")!=0){//run until quit
    mvaddstr(1, 0, "Line num: "); 
    wrefresh(ui_win);
    
    int c = getch();
    int i = 0;

    //loop to collect the line number from the terminal
    while (c != '\n'){

      if(c != ERR){
        if(c == KEY_BACKSPACE || c == KEY_DC || c == 127 ){ // check for backspace and delete input and handle it
          delch(); // delete the previous character typed
          refresh();
          c = getch();
          i--; // decrement so that the previous character can now be overwritten
          continue;
        }
        if (i < 2)
        { // making sure there is only two digits
          line_num_rep[i]= (char)c;
          fprintf(log_f, "typecasted char %c \n", (char) c );
          fflush(log_f);
        }
        i++;
      }
      c = getch();
    }
    //check if there is no input.
    if (i == 0){
      return -1;
    }

    if (i == 1){//only one digit long
      line_num_rep[i]='\0';//end the string early
    }
    fprintf(log_f, "line ''number'': %s\n", line_num_rep);
    if (strcmp(line_num_rep, ":q")==0){
      break;
    }
    int line_num = atoi((const char*)line_num_rep);//transform the line number rep to the index of the array
    //IMPELEMENT LATER: Let user know they put in a number to big and to try again.
    if (line_num > MAX_LINE_COUNT){
      return -2;
    }
    if (line_num == 0){
      return -3; //atoi error
    }
    line_num--; //subtract 1

    // Update the number of owners of a line
    if(argc == HOST_RUN){
      pthread_mutex_lock(&lock);
      our_file->contents[line_num].num_of_owners++; // update the number of owners of this line
      pthread_mutex_unlock(&lock);
      // make everyone else print astriks for this line claim
      if (users != NULL && users->head !=NULL){//if the list has been intialized and currently has contents
      distribute(-1, "*", line_num_rep, argc);// do not need to error check because this is the host and the line_num_rep and message are not NULL
      }
    }else if(argc == CLIENT_RUN){ // CLIENT: tell the host they are using this line
      if (users->head == NULL){//the host has left
        return -5; //error code for host left
      }
      int rc = send_message(users->head->data, line_num_rep);
    
    }

    mvaddch(line_num+2, 0, '*'); //print the astrisk to show line number claimed
    wrefresh(ui_win);
    move(1,10);
    wrefresh(ui_win);
    
    fprintf(log_f, "line we read: %d)\n", line_num);
    fflush(log_f);
    
    clrtoeol(); // clears what was typed on input line after input is entered

    //prompt user to input the contents
    mvaddstr(1, 0, "Contents: "); 
    wrefresh(ui_win);

    int overwriting = getch();
    i = 0; //reset the index overwriter

    // loop to collect what they want to write to this line (collecting character by character)
    while (overwriting != '\n'){
      if (overwriting != ERR){
        fprintf(log_f, "char we allegedly read: %c\n", overwriting);
        fflush(log_f);
        if(overwriting == KEY_BACKSPACE || overwriting == KEY_DC || overwriting == 127 ){ // check for backspace and delete input and handle it
          delch(); // delete the previous character typed
          refresh();
          overwriting = getch();
          i--;
          continue;
        }
        if (i < MAX_LINE_LENGTH-1){ // enforcing the 100 character per line limit  
            our_file->contents[line_num].line_contents[i]= (char)overwriting;// store the character in the file representation
        }
        i++;
        fprintf(log_f, "reading char: %c\n", our_file->contents[line_num].line_contents[i]);//fprintf for log_f file
        fflush(log_f);
      }
      overwriting = getch();// get the next character inputed
    }
    clrtoeol(); // clears what was typed on input line after input is entered

    // if what is inputed is less than 100 characters then padd the left over with spaces inside our file rep
    while (i < MAX_LINE_LENGTH-1){
      our_file->contents[line_num].line_contents[i]=' '; //pad with spaces again
      i++;
    }
    // log file stuff
    our_file->contents[line_num].line_contents[MAX_LINE_LENGTH-1]='\0';
    fprintf(log_f, "read line: %s\n", our_file->contents[line_num].line_contents);
    fflush(log_f);
    our_file->contents[line_num].line_contents[MAX_LINE_LENGTH-1]='\n';

    // HOST: write to the actual file & send to everyone
    if (argc == HOST_RUN){
      fseek(real_file->file_ref, 0, SEEK_SET);
      fseek(real_file->file_ref, (line_num * MAX_LINE_LENGTH), SEEK_CUR);//go to the right line in the file
      // real_file->file_ref += (line_num * MAX_LINE_LENGTH);
      fwrite(our_file->contents[line_num].line_contents, 1, MAX_LINE_LENGTH-1, real_file->file_ref); //write everything that changed (all but newline)
      fseek(real_file->file_ref, 0, SEEK_SET);//put the cursor back at the top of the file

      // HOST: hosts sends the new change to the rest of the clients
      if(users != NULL){ // first check if there are even users to send to.
        pthread_mutex_lock(&lock);
        list_node_t* temp = users->head; 
        while(temp!= NULL){
          int rc = send_message(temp->data, line_num_rep);
          if (rc == -1) {
            perror("Failed to send line number to client");
            pthread_mutex_unlock(&lock);
            exit(EXIT_FAILURE);
          }
          rc = send_message(temp->data, our_file->contents[line_num].line_contents);
          if (rc == -1) {
            perror("Failed to send message to client");
            pthread_mutex_unlock(&lock);
            exit(EXIT_FAILURE);
          }
          temp = temp->next;
        }
        pthread_mutex_unlock(&lock);
      }

      // HOST: Update the number of owners of a line
      pthread_mutex_lock(&lock);
      our_file->contents[line_num].num_of_owners--; // update the number of owners of this line
      pthread_mutex_unlock(&lock);
      // print to the host screen if the line is no longer in use
      if(our_file->contents[line_num].num_of_owners == 0){
        if (users !=NULL && users->head != NULL){//if the list has been initialized and currently has contents
          distribute(-1, " ", line_num_rep, argc);// let everyone know there is no one using this line
        }
          mvaddch(line_num+2, 0, ' '); //print the astrisk to show line number claimed
          wrefresh(ui_win);
          move(1,10);
          wrefresh(ui_win);
      }
       // CLIENT: send new changes to the host
    }else if (argc == CLIENT_RUN){
      if (users->head == NULL){//the host has left
        return -5; //error code for host left
      }
      send_message(users->head->data, line_num_rep);
      if (users->head == NULL){//the host has left
        return -5; //error code for host left
      }
      send_message(users->head->data, our_file->contents[line_num].line_contents);
    }
    
    //HOST & CLIENT: Print the new changes to personal screens
    int to_row = line_num+2; //+1 to undo the subtraction from indexing, +1 to adjust to connection line
    for (int col = 4; col <= MAX_LINE_LENGTH + 3; col++){
      mvaddch(to_row, col, our_file->contents[line_num].line_contents[col-4]);
      wrefresh(ui_win);
    }
    wrefresh(ui_win);
    move(1,10);//put the cursor back at the top (but not over the connection message)
    wrefresh(ui_win);
  }
  return 0;
}
 

/**
 * adds the new node of peer data to the front of users
 * \param value the data to be added to the list
 **/
void list_add(int value) {
  list_node_t* to_insert = malloc(sizeof(list_node_t));  // make space for the node
  to_insert->data = value;
  if (users == NULL){//list not initialized, needs to be
    users = malloc(sizeof(list_t));//malloc space for the list
    users->head = to_insert;//make the node we just created the head of the list
    users->head->next = NULL;//set the next to NULL
  }
  else{
    list_node_t* current = users->head;           // save the current head
    users->head = to_insert;                  // set what used to be head to next
    users->head->next= current;                      // set the new thing to head
  }
}

/**
* This thread function continues running for each client that connects to continously
* receive messages in the backgroud and prints them to the host's screen and the file.
* Then the host will send the change to the rest of the clients. 
*/
void* recieve_and_distribute(void* arg){
  bool newly_launched = true;
  extended_info_passing_t* unpacked = (extended_info_passing_t*)arg;
  int client_socket_fd = unpacked->info.port;
  int argc = unpacked->info.argc;
  char** argv = unpacked->argv;
  //this while loop loops until this the client program does not exist
  while(true){
    //HOST: send the file rep contents to the newly connected client
    if (argc==HOST_RUN && newly_launched==true){
      int rc;
      for (int i = 0; i< MAX_LINE_COUNT; i++){
      if (our_file->contents[i].num_of_owners == 0){
        rc = send_message(client_socket_fd, " ");
      }
      else{
        rc = send_message(client_socket_fd, "*");
      }
      if (rc==-1){
        fprintf(log_f, "I'm a host: didn't send the message ):\n");
        fflush(log_f);
      }
      rc = send_message(client_socket_fd, our_file->contents[i].line_contents);
      if (rc==-1){
        fprintf(log_f, "I'm a host: didn't send the message ):\n");
        fflush(log_f);
      }
      }
      newly_launched=false;
    }
    // CLIENT: recieve the file rep contents as a newly connected client
    if (argc==CLIENT_RUN && newly_launched==true){
      fprintf(log_f2, "made it into client\n");
      fflush(log_f2);
      for (int j = 0; j<MAX_LINE_COUNT; j++){
        char* claimed = receive_message(client_socket_fd);
        if (claimed == NULL){//did not recieve message
          fprintf(log_f2, "I'm a client: didn't read/recieve the message ):\n");
          fflush(log_f2);
        }
        char* message = receive_message(client_socket_fd);
        if (message == NULL){//did not recieve message
          fprintf(log_f2, "I'm a client: didn't read/recieve the message ):\n");
          fflush(log_f2);
        }
        else{
          claim_indicators[j] = malloc(strlen(claimed));//will get freed
          memcpy(claim_indicators[j],claimed,strlen(claimed));//2 for the char and the null terminator
          memcpy(our_file->contents[j].line_contents,message,MAX_LINE_LENGTH);//copy the message into the local copy of the data structure
          fprintf(log_f2,"trying to write in: %s\n",our_file->contents[j].line_contents);
          fflush(log_f2);
        }
      }
      newly_launched=false;
      sprintf(buffer, "user: %s, type: client\n", argv[1]);
      mvaddstr(0, 0, buffer); //write this to the very top line
      write_contents(CLIENT_RUN);
      //prompt user to input the line num
      mvaddstr(1, 0, "Line num: "); 
      wrefresh(ui_win);
    }

    char* line_num_rep;
    // HOST: update the line number the client has just claimed
    if(argc == HOST_RUN){
      line_num_rep = receive_message(client_socket_fd);//recieve a line number
      int line_num;
      if(line_num_rep != NULL){
        line_num = atoi(line_num_rep);
        pthread_mutex_lock(&lock);
        our_file->contents[line_num-1].num_of_owners++; // update the number of owners of this line
        pthread_mutex_unlock(&lock);
        int rc = distribute(client_socket_fd,"*",line_num_rep,argc);//send out the asterisk to represent it being in use
        if (rc == -1){
          return NULL; //end this thread
        }
        pthread_mutex_lock(&lock);
        int xcoor = getcurx(ui_win);
        int ycoor = getcury(ui_win);
        pthread_mutex_unlock(&lock);
        mvaddch(line_num+1, 0, '*'); //print the astrisk to show line number claimed
       
        move(ycoor,xcoor);
        wrefresh(ui_win);
      }
    }

    line_num_rep = receive_message(client_socket_fd);//recieve a line number
    char* message = receive_message(client_socket_fd);
    //HOST & CLIENTS: Recieve and Distribute to the rest of the clients
    int rc = distribute(client_socket_fd, message, line_num_rep, argc);//distribute the message
    if (rc == -1){
      return NULL; //end this thread
    }
    if (argc == HOST_RUN){
      int line_num;
      if(line_num_rep != NULL){
        line_num = atoi(line_num_rep);
        pthread_mutex_lock(&lock);
        our_file->contents[line_num-1].num_of_owners--; // update the number of owners of this line
        pthread_mutex_unlock(&lock);
        if (our_file->contents[line_num-1].num_of_owners == 0){
          rc =distribute(client_socket_fd," ",line_num_rep,argc);//if its not in use by anyone anymore, make sure there's no asterisk
          if (rc == -1){
            return NULL; //end this thread
          }
        pthread_mutex_lock(&lock);
        int xcoor = getcurx(ui_win);
        int ycoor = getcury(ui_win);
        pthread_mutex_unlock(&lock);
          mvaddch(line_num+1, 0, ' ');//first char in message here is the usage thing
          
          move(ycoor,xcoor);
          wrefresh(ui_win);
        }
      }
    }
    //HOST & CLIENT: Print the new changes to personal screens
    int line_num = atoi((const char*)line_num_rep);
    int to_row = line_num+1; //adjust to the extra line at the top for connection info
    int row_index = line_num -1; //array place of the line 
    int i = 0;
    if (argc == CLIENT_RUN && strcmp(message," ")==0 ){
      pthread_mutex_lock(&lock);
      int xcoor = getcurx(ui_win);
      int ycoor = getcury(ui_win);
      pthread_mutex_unlock(&lock);
      mvaddch(to_row, 0, ' ');//first char in message here is the usage thing
      move(ycoor,xcoor);
      wrefresh(ui_win);
      wrefresh(ui_win);
    }
    else if (argc == CLIENT_RUN && strcmp(message,"*")==0){
      pthread_mutex_lock(&lock);
      int xcoor = getcurx(ui_win);
      int ycoor = getcury(ui_win);
      pthread_mutex_unlock(&lock);
      mvaddch(to_row, 0, '*');//first char in message here is the usage thing
      move(ycoor,xcoor);
      wrefresh(ui_win);
      wrefresh(ui_win);
    }
    else{
      memcpy(our_file->contents[row_index].line_contents, message, MAX_LINE_LENGTH-1); //in our data structure, overwrite the line entirely but leave the /n
      pthread_mutex_lock(&lock);
      int xcoor = getcurx(ui_win);
      int ycoor = getcury(ui_win);
      pthread_mutex_unlock(&lock);
      for (int col = 4; col <= MAX_LINE_LENGTH + 3; col++){
        mvaddch(to_row, col, message[i]);
        i++;
       
      }
     
      move(ycoor,xcoor);//put the cursor back at the top
      wrefresh(ui_win);
        // This is a thread made by host recieving messages from clients
        // therefore we need to:
        // write to the host's screen.
        // write to file
          //HOST: WRITE THE CHANGE BACK TO THE FILE
      if (argc== HOST_RUN){
        fseek(real_file->file_ref, 0, SEEK_SET);
        fseek(real_file->file_ref, (row_index * MAX_LINE_LENGTH), SEEK_CUR);//go to the right line in the file
        // real_file->file_ref += (line_num * MAX_LINE_LENGTH);
        fwrite(our_file->contents[row_index].line_contents, 1, MAX_LINE_LENGTH-1, real_file->file_ref); //write everything that changed (all but newline)
        fseek(real_file->file_ref, 0, SEEK_SET); //move cursor back to the top
      }
    }
  }  
  return NULL;
}


/**
* add_user
* connects a client to you, and adds them to your list of connections 
* \param arg argument to function, int* wrapped in void*
* \returns NULL if it ever manages to leave, which it should not be able to do
**/
void* add_user(void* arg){ //get socket out of arg
  extended_info_passing_t* unpacked = (extended_info_passing_t*)arg;
  int argc = unpacked->info.argc;
  int socket_val = unpacked->info.port;
  int* socket = &socket_val;
  //running indefinitely looking for new connections
  while (true){
  // Waiting thread for a new thread to join
    int* client_socket_fd = malloc(sizeof(int));
    *client_socket_fd = server_socket_accept(*socket);
    if (*client_socket_fd == -1) {
      perror("accept failed");
      exit(EXIT_FAILURE);
    }
    //add to the list
    //lock while we accept the connection and add a client 
    pthread_mutex_lock(&lock);
    list_add(*client_socket_fd); 
    extended_info_passing_t pass;
    pass.info.port = *client_socket_fd;
    pass.info.argc = argc;
    pass.argv = unpacked->argv;
    pthread_mutex_unlock(&lock);
    pthread_t disseminate_thread; //launches a thread to recieve from this connection and disseminate to other clients
    pthread_create(&disseminate_thread, NULL, recieve_and_distribute, &pass);
  }
  return NULL; //bc it must return a void*
}

int main(int argc, char **argv){
    if(argc != HOST_RUN && argc !=CLIENT_RUN){
        fprintf(stderr, "Usage: %s <username>  <filepath> OR %s <username> <hostname> <port number>\n", argv[0], argv[0]);
        return -1; //don't run the program
    }
    ui_win = initscr();
    keypad(ui_win, true);   // Support arrow keys
    //Set up the log_f file
    log_f = fopen("log.txt", "w+");
    fflush(log_f);
    log_f2 = fopen("log2.txt", "w+");
    fflush(log_f2);
    bool pre_existing = true;
    our_file = malloc(sizeof(file_rep_t));//gets freed
    real_file = malloc(sizeof(locking_file_t));//gets freed
    pthread_mutex_init(&real_file->file_lock, NULL);
    
    //Set up a server socket to accept incoming connections
    unsigned short port = 0;
    int server_socket_fd = server_socket_open(&port);
    if (server_socket_fd == -1) {
        perror("Server socket was not opened");
        fprintf(log_f, "server socket couldn't open\n");
        exit(EXIT_FAILURE);
    }
    if (listen(server_socket_fd, 1)) {
        perror("listen failed"); 
        exit(EXIT_FAILURE);
    }
    if (argc == HOST_RUN){ //setup for session host. run program, username, and filename
        username = argv[1];
        filename = argv[2];
        // send the port 
        char info_message[200];
        
        sprintf(buffer, "user: %s, type: host, connect with port %d\n", username, port);
        mvaddstr(0, 0, buffer); //write this to the very top line
        fflush(log_f);
        // read the specified file into the data structure
        chmod(filename, 00777);//set rwx to good for everyone (if it exists, this doesn't do anything otherwise)
        real_file->file_ref = fopen(filename, "r+");//tries to open the file (does not overwrite)
        if (real_file->file_ref ==NULL){//the file does not exist
            pre_existing = false;
            real_file->file_ref = fopen(filename, "w+"); //create the file if it doesn't exist which it doesn't
        }
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
            our_file->contents[num_lines].num_of_owners = 0; //initialize all the lines number of owners to 0
            if (num_lines == MAX_LINE_COUNT-1){
                fwrite(our_file->contents[num_lines].line_contents, 1, MAX_LINE_LENGTH-1, real_file->file_ref); //write everything but the newline for the last line
            }
            else{
                fwrite(our_file->contents[num_lines].line_contents, 1, MAX_LINE_LENGTH, real_file->file_ref); //write the whole line, including newline
            }
        }
        fseek(real_file->file_ref, 0, SEEK_SET); //puts the pointer back at the start of the file
   
        info_passing_t args;
        args.port = server_socket_fd;
        args.argc= argc;
        extended_info_passing_t arg;
        arg.info = args;
        arg.argv = argv;
        pthread_t listen_thread;
        if(pthread_create(&listen_thread, NULL, add_user, &arg)){//inside of host, listen for new clients
            perror("pthread failed"); 
            exit(EXIT_FAILURE);
        }
        write_contents(HOST_RUN);
        //prompt user to input the line num
        mvaddstr(1, 0, "Line num: "); 
        wrefresh(ui_win);
    }
    if (argc == CLIENT_RUN){//connecting to editing session
        // Unpack arguments
        char* peer_hostname = argv[2];
        unsigned short peer_port = atoi(argv[3]);
        int my_host = socket_connect(peer_hostname, peer_port);
        if (my_host == -1) {
            perror("Failed to connect");
            exit(EXIT_FAILURE);
        }
        list_add(my_host);
        info_passing_t args;
        args.port = users->head->data;
        args.argc= argc;
        extended_info_passing_t arg;
        arg.info = args;
        arg.argv = argv;
        pthread_t disseminate_thread;
        if (pthread_create(&disseminate_thread, NULL, recieve_and_distribute, &arg)) {
            perror("pthread_create failed");
            exit(EXIT_FAILURE);
        }
    }

    
    int close_time = overwrite_line(argc);
    while (close_time != 0){
        clrtoeol(); // clears what was typed on input line after input is entered
        wrefresh(ui_win);
        //prompt user to input the line num
        close_time = overwrite_line(argc); //run until we exit normally or get the code for host leaving
        if (close_time == -5 && argc == CLIENT_RUN){
          wclear(ui_win);
          wrefresh(ui_win);
          mvaddstr(0, 0, "Host terminated session, you no longer have document access");
          wrefresh(ui_win);
          sleep(5);
          break; //exit while loop
        }
    }
    if (argc==HOST_RUN){
      fclose(real_file->file_ref);
    }
    fclose(log_f);
    fclose(log_f2);
    endwin();
    free(our_file);
    free(real_file);
    for (int i=0; i < MAX_LINE_COUNT; i++){
      free(claim_indicators[i]);
    }
    return 0;
} 
