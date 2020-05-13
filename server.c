/*
/   This is the biggest file of the collection. It contains all the information
/   for the server. Apart from the handlers. Every user command has a separate
/   Thread that handles its' functioning. All threads have their own handlers.
/   I learnt alot designing this Server. I felt that it was very challenging
/   and I am also ashamed that I cannot complete it to its full. By that I mean
/   Fix it so it has no BUGS ;) Enjoy!
/   
/   ZID : 3470429
/   NAME : Stephen Comino
*/

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <threads.h>
#include <errno.h>
#include <time.h>

#include <ctype.h>
#include <sysexits.h>
#include <assert.h>
#include "threadhandlers.h" // Thread header file
#include "userLogins.h"     // Some structs

// Create a list for users.
user_List users;
// This contains all the offline messages of the users
offlineMsg_List offlineMsgs;

// List of first login times so that we can track when they logged in
// Also tracks when they are loggedin or when they loggedout
loginTimes_List loginTimes;

// Some helper functions, very specific functions
int login(void *info, char *ubuffer, char *pbuffer, FILE *fp);
int sendP2PError(char * message);
int checkValidUser();


int main(int argc, char *argv[]) {

    if (argc < 4) {
        perror("Usage: ./server <port> <block_duration> <Timeout>\n");
        exit(1);
    }
    fflush(stdin);
    
    // Initialise the global lists
    users = NewUserList();
    offlineMsgs = newofflineMsgList();
    loginTimes = newLoginTimesList();
    
    // Get the command line arguments:
    int port = atoi(argv[1]);
    int block_duration = atoi(argv[2]);
    int timeout = atoi(argv[3]);
    
    // port to start the server on
    int SERVER_PORT = port;
    //change this port No if required
    
    // socket address used for the server
    struct sockaddr_in server_address;
    
    // zero the memory
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    
    // htons: host to network short: transforms a value in host byte
    // ordering format to a short value in network byte ordering format
    // Network byte ordering is little-endian format
    server_address.sin_port = htons(SERVER_PORT);
    
    // htonl: host to network long: same as htons but to long
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    
    // create a TCP socket (using SOCK_STREAM), creation returns -1 on failure
    int listen_sock;
    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("could not create listen socket\n");
        return 1;
    }
    
    // bind it to listen to the incoming connections on the created server
    // address, will return -1 on error
    if ((bind(listen_sock, (struct sockaddr *)&server_address,
              sizeof(server_address))) < 0) {
        printf("could not bind socket\n");
        return 1;
    }
    
    // Wait and listen for a connection
    int wait_size = 10;  // maximum number of waiting clients
    if (listen(listen_sock, wait_size) < 0) {
        printf("could not open socket for listening\n");
        return 1;
    }
    
    // The code below was borrowed from the lecture code
    // Create a new list, to keep track of the clients.
    // The list struct also contains a mutex and a condition variable
    // (equivalent to Python's threading.Condition()).
    List list = list_new();
    
    
    // Create an args struct for each thread. Both structs have a
    // pointer to the same list, but different sockets (different file
    // descriptors).
    Args server_info = new_args(list, listen_sock);
    // Set the timeout and Block_duration variables for the thread
    server_info->timeout = timeout;
    server_info->block_duration = block_duration;
    
    // This is the main thread that the server recieves information on.
    // All the other functions or threads are created from here
    thrd_t recv_thread;
    
    
    // open a new socket to transmit data per connection
    //different from listen_sock
    // socket address used to store client address
    struct sockaddr_in client_address;
    socklen_t client_address_len=sizeof(client_address);
    
    int sock;
    while (1) {
    
        if (sock =
            accept(listen_sock, (struct sockaddr *)&client_address,
                                                        &client_address_len)) {

            inet_ntoa(client_address.sin_addr);

            // Add temporary members too the list, incase they cannot
            // login
            // Host and port number
            server_info->tempSa = &client_address;


            // Create the thread
            int error;
            server_info->fd = sock;
            if (error = thrd_create(&recv_thread, recv_handler, 
                                                       (void *) server_info)) {
                printf("Error Creating thread: %d\n", error);
            }
        }
    }
             

    while (1) {
        // Equivalent to `sleep(0.1)`
        usleep(100000);
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    //      End of Thread code:
    ////////////////////////////////////////////////////////////////////////////
    
    // This code will never be reached, but assuming there was some way
    // to tell the server to shutdown, this code should happen at that
    // point.
    // Clean up the threads.
    int retval;
    thrd_join(recv_thread, &retval);

    
    // Close the sockets
    close(sock);
    close(listen_sock);
    
    
    return 0;
}

// This function runs when a connection is made
int recv_handler(void *args_) {
    
    Args args = (Args) args_;
    List list = args->list;
    int client_fd = args->fd; 
    int block_duration = args->block_duration;
    int timeout = args->timeout;
    
    // These are the threads to perform different actions
    // One thread per action :)
    thrd_t timeout_thread;
    thrd_t send_thread;
    thrd_t loginBroadcast_thread;
    thrd_t broadcast_thread;
    thrd_t whoelse_thread;
    thrd_t logout_thread;
    thrd_t whoelsesince_thread;
    thrd_t blacklist_thread;
    thrd_t unblock_thread;
    thrd_t message_thread;
    thrd_t offlineMsgs_thread;
    thrd_t invalidCommand_thread;
    thrd_t private_thread;
    thrd_t sendPrivate_thread;
    
    int len = 0, maxlen = 100;
    
    // Set the buffers for the username and password
    char usernameBuffer[maxlen];
    char* ubuffer = usernameBuffer;
    
    char passwordBuffer[maxlen];
    char* pbuffer = passwordBuffer;
    
    // Set the buffers to send 
    char send_username[maxlen];
    strcpy(send_username, "Username: ");
    char send_password[maxlen];
    strcpy(send_password, "Password: ");
    // Ask for credientials
    char loginName[maxlen];
    char loginPass[maxlen];
    int username = 0;
    int password = 0;
    
    int loggedIn = 0;
    int n = 0;
    
    // Run the login stage   
    // This calls a function that returns true if the credientials recived by
    // the cleint are correct
    while (loggedIn == 0) {
        memset(ubuffer, 0, maxlen);
        strcpy(send_username, "Username: ");
        send(client_fd, send_username, strlen(send_username)+1, 0);
        
        n = recv(client_fd, ubuffer, maxlen, 0);
        ubuffer[n] = '\0';
        // Copy the login name entered
        strcpy(loginName, ubuffer);
        
        memset(pbuffer, 0, maxlen);
        strcpy(send_password, "Password: ");
        send(client_fd, send_password, strlen(send_password)+1, 0);
        n = recv(client_fd, pbuffer, maxlen, 0);
        pbuffer[n] = '\0';
        // Copy the password entered
        strcpy(loginPass, pbuffer);
        
        // Get the file
        FILE * fp = fopen("./credentials.txt", "r");
        
        if (fp == NULL) {
            perror("Error opening file");
        }
        int loginAttempt = login(args, ubuffer, pbuffer, fp);
        if (loginAttempt == 1) {
            loggedIn = 1;
        } else if (loginAttempt == 3) {
            close(client_fd);
            return EXIT_SUCCESS;
        }
    }
    
/////////////////////////////////////////////////////////////////////////////
//                                                                        ///
//  The user is now logged in ..........................................  ///
//                                                                        ///
/////////////////////////////////////////////////////////////////////////////

    // Create a struct to manage the timeout of the user that is logged in  
    timeKeeper data = malloc (sizeof(*data));
    data->username = malloc(sizeof(char) * 100);
    strcpy(data->username, ubuffer);
    data->users = malloc (sizeof(users));
    data->users = users;
    
    // Populate thate thread with time.
    // It contains the timeout information
    time_t currTime;
    time(&currTime);
    data->currTime = currTime;
    data->timeout = timeout;
    data->socket = client_fd;
    
    int error;
    // Create a thread to deal with timeout
    // See timeout handler
    if ( error = thrd_create(&timeout_thread, timeout_handler, (void *) data)) {
        perror("Error!");
    }
   
    
    // Get the username so we can send broadcast only to the right users
    // and not this user
    
    int sendError;

    // Save the users username to global variables
    strcpy(users->username, ubuffer);
    strcpy(users->userToMessage, ubuffer);
    
    // This is the welcome broadCast!
    int sendBroadcast;
    if ( sendBroadcast = thrd_create(&loginBroadcast_thread, 
                                    loginBroadcast_handler, (void *) users)) {
        perror ("Error!");
    }
    
    // get the message to send to the user that was offline
    strcpy(users->username, ubuffer);
    //message = strtok(NULL, " ");
    int count = 0;
    // Send the offline messages the user received when he was logged out
    int offlineMsgs;
    if ( offlineMsgs = thrd_create(&offlineMsgs_thread, offlineMsgs_handler, 
                                                            (void *) users)) {
        perror ("Error!");
    }
    
    
    
    // Set up buffers for The users loggedin mode
    char userBuffer[maxlen];
    char* loginBuffer = userBuffer;
   
    n = 0;
    // Check if the user is logged in
    int flag = getLoginFlag (loginTimes, ubuffer);
    
    // While the user is logged in loop
    while (flag) {
        // clear memory
        memset(loginBuffer, 0, 500);
        //receive from the client
        n = recv(client_fd, loginBuffer, 500,  0);
        loginBuffer[n] = '\0';
        
///////////////////////////////////////////////////////////////////////////////
//                                                                          ///
//                    This is the Message Command                           ///                             
//                                                                          ///
///////////////////////////////////////////////////////////////////////////////
        if (strncmp(loginBuffer, "message", 7) == 0) {
            char *message = malloc(sizeof(char) * 1500); // set enough size
            message = strtok(loginBuffer, " ");
            
            // Collect the message information
            strcpy(users->message, ubuffer);
            strcat(users->message, ": ");
        
            strcpy(users->username, ubuffer);
        
            int count = 0;
            for (message = strtok(NULL, " "); message != NULL; 
                                                message = strtok(NULL, " ")) {
                if (count == 0) {
                    strcpy(users->userToMessage, message);
                } else {
                    strcat(users->message, message);
                    strcat(users->message, " ");
                }
                count++;
            }
            // perform Message() command by using a thread
            int sendMessage;
            if ( sendMessage = thrd_create(&message_thread, message_handler,
                                                             (void *) users)) {
                perror ("Error!");
            }
            // Update the time of the users last action
            time_t seconds;
            time(&seconds);
            setLoginTime(loginTimes, ubuffer, seconds, 1);
            n = 0;
///////////////////////////////////////////////////////////////////////////////
//                                                                          ///
//                    This is the BroadCast Command                         ///                             
//                                                                          ///
///////////////////////////////////////////////////////////////////////////////
        } else if (strncmp(loginBuffer, "broadcast", 9) == 0) {
            char *message = malloc(sizeof(char) * 1500);
            message = strtok(loginBuffer, " ");
          
            strcpy(users->message, ubuffer);
            strcat(users->message, ": ");
            strcat(users->message, (message + 10));
            strcpy(users->username, ubuffer);
            if ( sendBroadcast = thrd_create(&broadcast_thread, 
                                          broadcast_handler, (void *) users)) {
                perror ("Error!");
            }
            time_t seconds;
            time(&seconds);
            setLoginTime(loginTimes, ubuffer, seconds, 1);
            n = 0;
///////////////////////////////////////////////////////////////////////////////
//                                                                          ///
//                    This is the whoelse Command                           ///                             
//                                                                          ///
///////////////////////////////////////////////////////////////////////////////
        } else if (strcmp(loginBuffer, "whoelse") == 0) {
            int whoelseBroadcast;
            strcpy(users->message, ubuffer);
            if ( whoelseBroadcast = thrd_create(&whoelse_thread, 
                                            whoelse_handler, (void *) users)) {
                perror ("Error!");
            }
            time_t seconds;
            time(&seconds);
            setLoginTime(loginTimes, ubuffer, seconds, 1);
      
            n = 0;
///////////////////////////////////////////////////////////////////////////////
//                                                                          ///
//                    This is the whoelseSince Command                      ///                             
//                                                                          ///
///////////////////////////////////////////////////////////////////////////////
        } else if (strncmp(loginBuffer, "whoelsesince", 12) == 0) {
            int whoelsesinceBroadcast;
            char * message;
            message = strtok( loginBuffer, " ");
            users->timeSince = atoi(message + 13);
            strcpy(users->message, ubuffer); // get the username
            if ( whoelsesinceBroadcast = thrd_create(&whoelsesince_thread, 
                                       whoelsesince_handler, (void *) users)) {
                perror ("Error!");
            }
            time_t seconds;
            time(&seconds);
            setLoginTime(loginTimes, ubuffer, seconds, 1);
      
            n = 0;
///////////////////////////////////////////////////////////////////////////////
//                                                                          ///
//                    This is the block Command                           ///                             
//                                                                          ///
///////////////////////////////////////////////////////////////////////////////
        } else if (strncmp(loginBuffer, "block", 5) == 0) {
            int blacklist;
            char * message;
            message = strtok( loginBuffer, " ");
            strcpy(users->message, message + 6); // get the username
            strcpy(users->username, ubuffer);
            if ( blacklist = thrd_create(&blacklist_thread, blacklist_handler, 
                                                            (void *) users)) {
                perror ("Error!");
            }
            time_t seconds;
            time(&seconds);
            setLoginTime(loginTimes, ubuffer, seconds, 1);
      
            n = 0;
///////////////////////////////////////////////////////////////////////////////
//                                                                          ///
//                    This is the unblock Command                           ///                             
//                                                                          ///
///////////////////////////////////////////////////////////////////////////////
        } else if (strncmp(loginBuffer, "unblock", 7) == 0) {
            int unblock;
            char * message;
            message = strtok( loginBuffer, " ");
            strcpy(users->message, message + 8); // get the username
            strcpy(users->username, ubuffer);

            if ( unblock = thrd_create(&unblock_thread, unblock_handler, (void *) users)) {
                perror ("Error!");
            }
            //printf("WHOELSERECEIVESD!\n");
            time_t seconds;
            time(&seconds);
            setLoginTime(loginTimes, ubuffer, seconds, 1);
      
            n = 0;
///////////////////////////////////////////////////////////////////////////////
//                                                                          ///
//                    This is the startprivate Command                      ///                             
//                                                                          ///
///////////////////////////////////////////////////////////////////////////////
        } else if (strncmp(loginBuffer, "startprivate", 12) == 0) {
            // check for error
           
            time_t seconds;
            time(&seconds);
            // my username
            strcpy(users->username, ubuffer);
            // get the username
            // users->userToMessage is the target of startprivate
            char * message;
            message = strtok( loginBuffer, " ");
            strcpy(users->userToMessage, message + 13); // get the username
            
            // cehck if this user is blocked, offline or self
            int check = checkValidUser();
            // Return the listening server of this client
            if (check == 1) {
                int privateError;
                
                if ( privateError = thrd_create(&private_thread, 
                                            private_handler, (void *) users)) {
                    perror ("Error!");
                }
                
                setLoginTime(loginTimes, ubuffer, seconds, 1);
                
                // Server queries the <user> for its connectionInfo
                
                // The server then sends this information back onto the user wanting
                // to start private
            } else {
                // Set the name back from the false one
                 strcpy(users->userToMessage, ubuffer);
            }
///////////////////////////////////////////////////////////////////////////////
//                                                                          ///
//                    This is the private Command(Protocol)                 ///                             
//                                                                          ///
///////////////////////////////////////////////////////////////////////////////
// This is different to the others
// It is repsonsible for sending Protocol data. IE the users information
// back to the client
        } else if (strncmp(loginBuffer, "private!!", 9) == 0 && 
                                                    users->waitPrivate == 1) {
            time_t seconds;
            time(&seconds);
            char *message = malloc(sizeof(char) * 200);
            message = strtok(loginBuffer, " ");
            // I need to get the information from the User attempting to start
            // private.
            // Return the listening server of this client
            int sendPrivateError;
            //strcpy(users->username, ubuffer);
            
            strcpy(users->message, message);
            strcat(users->message, " ");
            strcat(users->message, "Recv");
            strcat(users->message, " ");
            strcat(users->message, users->userToMessage); // send the Target]
            strcat(users->message, " ");
            strcat(users->message, users->username);    // send my username
            strcat(users->message, " ");
            for (message = strtok(NULL, " "); message != NULL; 
                                                message = strtok(NULL, " ")) {
                strcat(users->message, message);
                strcat(users->message, " ");
            }
            // Send the information in loginBuffer back to the requesting client!
            if ( sendPrivateError = thrd_create(&sendPrivate_thread, 
                                        sendPrivate_handler, (void *) users)) {
                perror ("Error!");
            } 
///////////////////////////////////////////////////////////////////////////////
//                                                                          ///
//                    This is the logout Command                            ///                             
//                                                                          ///
///////////////////////////////////////////////////////////////////////////////
        } else if (strcmp(loginBuffer, "logout") == 0) {
            time_t seconds;
            time(&seconds);
            strcpy(users->userToMessage, ubuffer);
            int logout;
            setLoginTime(loginTimes, ubuffer, seconds, 0);
            if ( logout = thrd_create(&logout_thread, logout_handler, 
                                                            (void *) users)) {
                perror ("Error!");
            }
            flag = getLoginFlag (loginTimes, ubuffer);
            
            return EXIT_SUCCESS;
            
            // If the user sends data and it is of private and stopprivate types
            // Just ignore!!!!
        } else if (strncmp(loginBuffer, "private", 7) == 0 || 
                                strncmp(loginBuffer, "stopprivate", 11) == 0) {
        
            continue; // Don't do anything on the server side
        
        } else if (strlen(loginBuffer) >= 1) {
            int sendInvalid = 0;
            for (int i = 0; i <= strlen(loginBuffer); i++) {
                if (isprint(loginBuffer[i])) {
                    sendInvalid = 1;
                }
            }
            if (sendInvalid == 1) {
                //Error. Invalid command
                strcpy(users->username, ubuffer);
                int invalidCommand;
                if ( invalidCommand = thrd_create(&invalidCommand_thread, 
                                    invalidCommand_handler, (void *) users)) {
                    perror ("Error!");
                }
                time_t seconds;
                time(&seconds);
                setLoginTime(loginTimes, ubuffer, seconds, 1);
          
                n = 0;
                sendInvalid = 0;
            }
        }
    }
        
    //close listening socket
    return EXIT_SUCCESS;
}

// This function is used in the p2pInformation collection stage where the 
// credentials of the startprivate are being checked.
// THis function checks if the user is someone who can be contacted
// Returns 0 on failure
// Returns 1 on Success
// It checks 4 things:
// 1. It checks if the user exists
// 2. It checks if the user is not logged in
// 3. It checks if the user has blocked you
// 4. It checks if the person you are trying to talk to is yourself
int checkValidUser() {
    int loggedIn;
    char * message = malloc(sizeof(char) * 100);
    int check;
    FILE * fp = fopen("credentials.txt", "r");
    if ((check = checkFileForUser(fp,  users->userToMessage)) == 0) {
        // Invalid username, That user does not exist
        strcpy(message, "That user does not exist");
        sendP2PError(message);
        free(message);
        return 0;
    }
    if ((loggedIn = getLoginFlag (loginTimes,  users->userToMessage)) == 0) {
        // That User is not logged In!
        strcpy(message, "That User is not logged in!");
        sendP2PError(message);
        free(message);
        return 0;
    }
    int imIgnored;
    if (imIgnored = getUserBlackListed (loginTimes, users->userToMessage, 
                                                            users->username)) {
        // Can't reach that user
        strcpy(message, "Can't reach that user");
        sendP2PError(message);
        free(message);
        return 0;
    }
    
    
    if (strcmp(users->username, users->userToMessage) == 0) {
        // that is my name
        strcpy(message, "Can't talk to yourself!");
        sendP2PError(message);
        free(message);
        return 0;
    }
    
    free(message);
    return 1;
}

// This function is responsible for sending the error message back to the client
// who attemped to initiate a p2p conversation with an invalid user.

int sendP2PError(char * message) {
    
    // Create the send buffer
    char buf[100];
    char name[100] = {0};
    // get the users list

    int fd;
    // get the fd of the user we are interested in
    for (userNode curr = users->first; curr != NULL; curr = curr->next) {
        if(strcmp(users->username, curr->username) == 0) {
            fd = curr->client_fd;
        }
    }
    int messageLength = strlen(message) + 1;
    
    // Send the error
    int numbytes = send(fd, message, messageLength, 0);

}

// This function is responsible for handling login. It checks if the user is in 
// the file. Sets the block duration and initialises the user list.
// It also initialises the loggedIn List so that we can keep track on when the
// user has logged in.
// Input:
//      void *args_ : this contains the list that contains the users temp
//                     connection information
//      char *ubuffer : This is the username used to attempt to login
//      char *pbuffer : This is the password used to attempt to login
//      FILE *fp : this is the File pointer of the credentials.txt
// Output:
//      Returns 3 : On error, Anything other than incorrect credentials
//      Returns 1 : On Success
//      Returns 0 : On Failure
int login (void *args_, char *ubuffer, char *pbuffer, FILE *fp) {

    int maxlen = 100;
    Args args = (Args) args_;
    List list = args->list;

    int client_fd = args->fd; 
    int block_duration = args->block_duration;
    int timeout = args->timeout;
    // prepare to receive
    int n = 0;
    int num = 0;
    char tmpUser[maxlen];
    char tmpPass[maxlen];
    int match = 0;
    char success[maxlen];

    // Check the user isn't already logged on!
    for (userNode client = users->first; client != NULL; client = client->next) {
        int loggedIn = getLoginFlag (loginTimes, client->username);
        if (strcmp(client->username, ubuffer) == 0 && loggedIn == 1) {
            strcpy(success, "Error, Someone is already logged in as that user!");
            send(client_fd, success, strlen(success)+1, 0);
            return 3;
        }
    }

    // Loop over the credentials file
    while ((num = fscanf(fp, "%s %s", tmpUser, tmpPass)) != EOF) {
        
        // Found a login and password pair. SUCCESS!
        if ((strcmp(tmpUser, ubuffer) == 0) && (strcmp(tmpPass, pbuffer) == 0)) {
            
            // Check the user is within their block duration
            // Get the current time in seconds
            time_t seconds;
            time(&seconds);
            // get the userTime in seconds
            time_t userSeconds = getBlockTimeUser (loginTimes, ubuffer);
            int isTime = ((seconds - userSeconds) <= block_duration);
            if (isTime) {
                strcpy(success, "Error, You are prevented from logging in!");
                send(client_fd, success, strlen(success)+1, 0);
                return 3;
            }
            
            strcpy(success, "Success!");

            send(client_fd, success, strlen(success)+1, 0);

            match = 1;       
            // Check if the user is on the list
            // Add this information to the user list

            UserListInsert (users, ubuffer, 1, 1, client_fd);
            // set the Logged In Timer;
            time(&seconds);
            // check if an entry is already in the list
            // If the user isnt add him to the list so we can keep track of his
            // time. Also set the users login time
            if (!isInTimeTracker(loginTimes, ubuffer)) {
                time(&seconds);
                loginTimesInsert (loginTimes, ubuffer, seconds, 1);
            } else {
                time(&seconds);
                setLoginTime(loginTimes, ubuffer, seconds, 1); 
            }
            resetAttemptUser (loginTimes, ubuffer);
            return 1;

        }


        // Handle the case incorrect credentials were given
        if ((strcmp(tmpUser, ubuffer) == 0) && match == 0) {
            time_t seconds;
            time(&seconds);
            // Check the user is in our list
            time_t userSeconds = getBlockTimeUser (loginTimes, ubuffer);
            if (!isInTimeTracker(loginTimes, ubuffer)) {
                loginTimesInsert (loginTimes, ubuffer, 0, 0);
            } else {
                incrementAttemptUser(loginTimes, ubuffer);
            }

            int block = getAttemptUser(loginTimes, ubuffer);
            if (block >= 3) {
                // Set block_duration
                time_t seconds;
                time(&seconds);
                setBlockTimeUser (loginTimes, ubuffer, seconds);
            }     
        }
    }

    // Accept enter as input
    if (match == 0) {
        strcpy(success, "Invalid Username/Password\n");
        char space[2];
        send(client_fd, success, strlen(success)+1, 0);
        n = recv(client_fd, space, maxlen, 0);
        space[1] = '\0';
    }  
        fclose(fp);
    return 0;
}
