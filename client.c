clientP2PList.c                                                                                     0000640 0124623 0124623 00000005421 13565432174 012536  0                                                                                                    ustar   z3470429                        z3470429                                                                                                                                                                                                               /*
/   This file is responsible for the List functions that handle the server
/   and client lists for p2p in Client.c. These functions and structs are 
/   defined in ClientP2P.h
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
#include <time.h>
#include <err.h>
#include "clientP2P.h"

// NewList
// Create a new linked list
struct p2pConnectionsRep* NewP2PList (void)
{
	struct p2pConnectionsRep *L = malloc (sizeof *L);
	if (L == NULL) err (EX_OSERR, "couldn't allocate List");
	L->size = 0;
	L->first = NULL;
	L->last = NULL;
	return L;
}

// Create a new node for the list
struct p2pConnectionsNode *newP2PNode (char *username, int serverfd, int clientfd)
{
	struct p2pConnectionsNode *n = malloc (sizeof *n);
	if (n == NULL) err (EX_OSERR, "couldn't allocate node");
	
	n->username = malloc(sizeof (char) * 100);
	strcpy(n->username, username);
	n->serverfd = serverfd;
	n->clientfd = clientfd;
	return n;
}

// Insert a node into the list
void P2PNodeInsert (p2p_List L, char *username, int serverfd, int clientfd)
{
	assert (L != NULL);
    
	struct p2pConnectionsNode *n = newP2PNode (username, serverfd, clientfd);
	if (L->first == NULL) {
		L->first = L->last = n;
    } else {
		L->last->next = n;
		L->last = n;
	}
	L->size++;
}

// This function removes an item from the list
void removeFromList (p2p_List L, char *username) {
    if (L->first == NULL) {
       return;
    }
    p2p_node  prev = NULL;
    for (p2p_node client = L->first; client != NULL; client = client->next) {

        if (strcmp(client->username, username) == 0) {

            // Hand the case it is the first entry on the list
            if (prev == NULL) {
                if (client->next != NULL) {
                    L->first = client->next;
                    if (client->next->next == NULL) {
                        L->last = client->next;
                    }
                    free(client->username);


                    //  free(client);
                    } else {
                        L->first = NULL;
                        L->last = NULL;
                        free(client->username);
                        //free(client);
                    }   
                        // prev is not NULL
            } else {
                printf("PREV DELTE: %s\n", prev->username);
                if (client->next == NULL) {
                    L->last = prev;
                }
                prev->next = client->next;
                free(client->username);
                //free(client);
            }

        }
        prev = client;
    }
}
                                                                                                                                                                                                                                               server.c                                                                                            0000640 0124623 0124623 00000073755 13565457361 011434  0                                                                                                    ustar   z3470429                        z3470429                                                                                                                                                                                                               /*
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
                   threads.c                                                                                           0000640 0124623 0124623 00000063066 13565450265 011546  0                                                                                                    ustar   z3470429                        z3470429                                                                                                                                                                                                               /*
/   This function implements all the thread_handlers for the threads used in
/   server.c
/   This file has alot of code.
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
#include "threadhandlers.h" // Get the prototypes for the threads


// This function is used to send the queried client information to the client
// wishing to start a p2p connection with another user.
// It is pivotal in the P2P protocol
int sendPrivate_handler (void *args_) {
    user_List users = (user_List) args_;

    char buf[100];

    char name[100] = {0};
    // For each client:

    // get the users list

    int fd;
    // get the fd
    for (userNode curr = users->first; curr != NULL; curr = curr->next) {
        if(strcmp(users->username, curr->username) == 0) {
            fd = curr->client_fd;
        }
    }
    int messageLength = strlen(users->message) + 1;
    
    // Send the message to the user
    int numbytes = send(fd, users->message, messageLength, 0);
    // A toggle for send() recv() state transition
    users->waitPrivate = 0;
}

// THis function is responsible for sending more p2p protocol information to 
// the client who initiated the p2p connection
int private_handler(void *args_) {
    user_List users = (user_List) args_;
    
    char buf[100];
   
   // strcpy(broadcastBuffer, users->username);
    char name[100] = {0};
    // For each client:
       
        // get the users list
        
        int fd;
        // get the fd
    for (userNode curr = users->first; curr != NULL; curr = curr->next) {
        if(strcmp(users->userToMessage, curr->username) == 0) {
            fd = curr->client_fd;
        }
    }
    
    users->waitPrivate = 1; 
    // Ge tthe use
    time_t minutesAgo;
    time_t current_time;
    
    strcpy(buf, "private!!");
    int messageLength = strlen(buf) + 1;
    int waitForReply = 0;
    
    
    int numbytes = send(fd, buf, messageLength, 0);
        
    return EXIT_SUCCESS;
}

// This threadHandler is responsible for unblocking a user.
int unblock_handler(void *args_) {
    
    user_List users = (user_List) args_;
    //char * username = args->username;
    
    char buf[100];
    char *broadcastBuffer = buf;
    
    char name[100] = {0};
    // For each client:
    memset(buf, 0, 100);
    
    // get the fd of the user we want to talk to
    int fd;
    for (userNode curr = users->first; curr != NULL; curr = curr->next) {
        if(strcmp(users->username, curr->username) == 0) {
            fd = curr->client_fd;
        }
    }
        
    // Check if the name entered is not my own
    if (strcmp(users->message, users->username) == 0) {
        //setBlackListUser (users, users->username, users->message);
        strcat(users->message, ": error that is my name!");
        int length = strlen(users->message) + 1;
        snprintf(buf, length, "%s", users->message); 
        
        int numbytes = send(fd, buf, length, 0);
        
    } else {
    
    // check the name entered is on the list of users
    
        int num;
        FILE * fp = fopen("credentials.txt", "r");
        if (fp == NULL) {
            printf("Error opening file");
        }
        // check the credentials file for the user
        int success = checkFileForUser(fp, users->message);
        fclose(fp);
        if (success == 0) {
            strcat(users->message, ": error that user does not exist!");
            int length = strlen(users->message) + 1;
            snprintf(buf, length, "%s", users->message); 
            
            int numbytes = send(fd, buf, length, 0);
            return EXIT_SUCCESS;
        }
        
        // check the user - Send the response back to the client who 
        // reuquested the action
        success = getUserBlackListed (loginTimes, users->username, 
                                                                users->message);
        
        if (success == 0) {
            strcat(users->message, ": error that user is not currently"
                                                                   " blocked!");
            int length = strlen(users->message) + 1;
            snprintf(buf, length, "%s", users->message); 
            
            int numbytes = send(fd, buf, length, 0);
            return EXIT_SUCCESS;
        // Unblock
        } else {
            removeBlackList (loginTimes, users->username, users->message);
            strcat(users->message, " unblocked!");
            int length = strlen(users->message) + 1;
            snprintf(buf, length, "%s", users->message); 
            
            int numbytes = send(fd, buf, length, 0);
        }
    }
 
    return EXIT_SUCCESS;
}


// This function blocks the user. At the request of the client
int blacklist_handler(void *args_) {
    
    user_List users = (user_List) args_;
    //char * username = args->username;
    
    char buf[100];
    char *broadcastBuffer = buf;
    
    char name[100] = {0};
    // For each client:
    memset(buf, 0, 100);
    
    // get the users list
    int fd;
    for (userNode curr = users->first; curr != NULL; curr = curr->next) {
        if(strcmp(users->username, curr->username) == 0) {
            fd = curr->client_fd;
        }
    }
        
    // Check if the name entered is not my own
    if (strcmp(users->message, users->username) == 0) {
        strcat(users->message, ": error that is my name!");
        int length = strlen(users->message) + 1;
        snprintf(buf, length, "%s", users->message); 
        
        int numbytes = send(fd, buf, length, 0);
        
    } else {
        FILE * fp = fopen("credentials.txt", "r");
        if (fp == NULL) {
            printf("Error opening file");
        }
        // check the credentials file for the user - send the conformation
        // To the user
        int success = checkFileForUser(fp, users->message);
        fclose(fp);
        if (success == 0) {
            setBlackListUser (loginTimes, users->username, users->message);
            strcat(users->message, " does not exist!");
            int length = strlen(users->message) + 1;
            snprintf(buf, length, "%s", users->message); 
            
            int numbytes = send(fd, buf, length, 0);        
        } else {
        // block the user, Send the confirmation to the user
            setBlackListUser (loginTimes, users->username, users->message);
            strcat(users->message, " Blocked!");
            int length = strlen(users->message) + 1;
            snprintf(buf, length, "%s", users->message); 
            
            int numbytes = send(fd, buf, length, 0);
        }
    }
    
    return EXIT_SUCCESS;
}

// The following function implements the timeout handler.
// Handle the timeout frame of the user.
// TImeout the user if he/she breaks the timeout time.
int timeout_handler(void *args_) {
    
    /// Struct containing timeout information
    timeKeeper data = (timeKeeper) args_;
    // information on users time data 
    user_List user_info = data->users;
    
    int n = 0;
    int timeFrame = 0;
    // check if the user is logged in
    int loggedIn = getLoginFlag (loginTimes, data->username);
    
    // Check the user is within its time frame!
    while (timeFrame = ((data->currTime - getLoginTime (loginTimes, 
                        data->username)) < data->timeout) && (loggedIn == 1)) {
        time_t seconds;
        time(&seconds);
        data->currTime = seconds;
        loggedIn = getLoginFlag (loginTimes,data->username);
    }
    // If they were still logged in! Time them out - send information back to 
    // client
    if (loggedIn == 1) {
        char *username = data->username;
        char *disconnectString = "Timeout: ";
        char *disconnectString2 = " The server has Timed you out";
        int length = strlen(username) + strlen(disconnectString) + 
                                                 strlen(disconnectString2) + 1;
        char *message = malloc (sizeof(char) * length);
        snprintf(message, length, "%s%s%s", disconnectString, username, 
                                                            disconnectString2);
        send(data->socket, message, length, 0);
    }
    int logout;
    time_t seconds;
    time(&seconds);
    thrd_t logout_thread;
    setLoginTime(loginTimes, data->username, seconds, 0);
    // Remove the timedata for the user from the list
    for (loginTimesNode times = loginTimes->first; times != NULL; 
                                                        times = times->next) {
        if (strcmp(data->username, times->username) == 0) {
            time_t seconds;
            time(&seconds);
            times->loggedIn = 0;
            times->logoutTime = seconds;
        }
    }

    strcpy(users->userToMessage, data->username);
    return EXIT_SUCCESS;
}

// This is the handler for the logout command. It logs the user out of the 
// server.
int logout_handler(void *args_) {
    user_List users = (user_List) args_;
    
    // The buffers for sending data
    char buf[100];
    char *broadcastBuffer = buf;
    
    char name[100] = {0};
    // For each client:
    memset(buf, 0, 100);

    int fd;
    // get the fd of the current users socket
    for (userNode curr = users->first; curr != NULL; curr = curr->next) {
        if(strcmp(users->username, curr->username) == 0) {
            fd = curr->client_fd;
        }
    }
        
    for (userNode curr = users->first; curr != NULL; curr = curr->next) {
        //printf("New List:!\n");
        int client_fd = curr->client_fd;
        int ignored = 0;
        ignored = getUserBlackListed (loginTimes, users->userToMessage, 
                                                                curr->username);

        // Check if the recipient has blacklisted me 
        int imIgnored = 0;
        imIgnored = getUserBlackListed (loginTimes, curr->username,  
                                                               users->username);
        strcpy(curr->welcomeBroadcast, " LoggedOut");
            // Only send to those clients that havnt ignored me
            if (!ignored) {
                int usernamelength = strlen(users->userToMessage);
                int messageLength = strlen(curr->welcomeBroadcast) + 2;
                int total = usernamelength + messageLength;
                snprintf(buf, total, "%s%s", users->userToMessage, 
                                curr->welcomeBroadcast); //users->userToMessage 
                int numbytes = send(client_fd, buf, total, 0);
                // Find the logout name and set the time.
                for (loginTimesNode times = loginTimes->first; times != NULL; 
                                                        times = times->next) {
                    if (strcmp(users->userToMessage, times->username) == 0) {
                        time_t seconds;
                        time(&seconds);
                        times->loggedIn = 0;
                        times->logoutTime = seconds;
                    }
                }
            }
    }
    time_t seconds;
    time(&seconds);

    // Remove the entry from the list - From the client logged in list
    userNode prev = NULL;
    for (userNode client = users->first; client != NULL; 
                                                       client = client->next) {
        if (strcmp(client->username, users->userToMessage) == 0) {
            
            // Hand the case it is the first entry on the list
            if (prev == NULL) {
                if (client->next != NULL) {
                    users->first = client->next;
                    if (client->next->next == NULL) {
                        users->last = client->next;
                    }
                    free(client->username);
                    free(client->welcomeBroadcast);
                    free(client);
                  //  free(client);
                } else {
                    users->first = NULL;
                    users->last = NULL;
                    free(client->username);
                    free(client->welcomeBroadcast);
                    free(client);
                }   
            // prev is not NULL
            } else {
                //printf("PREV DELTE: %s\n", prev->username);
                if (client->next == NULL) {
                    users->last = prev;
                }
                prev->next = client->next;
                free(client->username);
                free(client->welcomeBroadcast);
                free(client);
            }
            
       }
       prev = client;
   }
    return EXIT_SUCCESS;
}

// This function handles the broadcast of the login Message!
int loginBroadcast_handler(void *args_) {
    user_List users = (user_List) args_;

    char buf[100];
    char *broadcastBuffer = buf;
    
    char name[100] = {0};
    // For each client:
    memset(buf, 0, 100);
    // get the users list
    int fd = 0;
    for (userNode curr = users->first; curr != NULL; curr = curr->next) {
        if (curr->username != NULL && users->message != NULL) {
            if(strcmp(users->username, curr->username) == 0) {
                fd = curr->client_fd;  
            }
        }
    }
    
    // Loop over the users currently logged into the server.
    // Check if I am blocked or if I blocked them    
    for (userNode curr = users->first; curr != NULL; curr = curr->next) {
        int client_fd = curr->client_fd;
        int ignored = 0;
        ignored = getUserBlackListed (loginTimes, users->username, 
                                                                curr->username);

        // Check if the recipient has blacklisted me 
        int imIgnored = 0;
        imIgnored = getUserBlackListed (loginTimes, curr->username, 
                                                              users->username);
        
        // Broadcast to those that aren't ignored!
        if (client_fd != fd && !ignored) {
            strcpy(curr->welcomeBroadcast, ": Logged in!");
            int usernamelength = strlen(users->userToMessage);
            int messageLength = strlen(curr->welcomeBroadcast) + 3;
            int total = usernamelength + messageLength;
            snprintf(buf, total, "%s%s", users->userToMessage, 
                                                        curr->welcomeBroadcast); 

            int numbytes = send(client_fd, buf, total, 0);
        }
    }
    return EXIT_SUCCESS;
}

// This function handles direct message to other users
int message_handler(void *args_) {
    
    user_List users = (user_List) args_;
    
    char buf[100];
    char *broadcastBuffer = malloc (sizeof(char)
                                               * (strlen(users->message) + 1));
    strcpy(broadcastBuffer, users->message);
    char name[100] = {0};

    int notOffline = 0;
    // Get the fd of the user we want to message 
    int fd;
    for (userNode curr = users->first; curr != NULL; curr = curr->next) {
        if(strcmp(users->username, curr->username) == 0) {
            fd = curr->client_fd;
        }
    }
    
    // check the credentials file for the user 
    FILE * fp = fopen("credentials.txt", "r");
    if (fp == NULL) {
        printf("Error opening file");
    }
    
    int success = checkFileForUser(fp, users->userToMessage);
    fclose(fp);
    
    // Check for a valid user
    if (success == 0) {
        int usernamelength = strlen(users->username);
        strcpy(users->message, users->username);
        strcat(users->message, ": Error That user doesn't exist!");
        int messageLength = strlen(users->message) + 1;

        int numbytes = send(fd, users->message, messageLength, 0);
        return EXIT_SUCCESS;
    }
   
    if (strcmp(users->userToMessage, users->username) == 0) {
        int usernamelength = strlen(users->username);
        strcpy(users->message, users->username);
        strcat(users->message, ": Cant message yourself!");
        int messageLength = strlen(users->message) + 3;
        int total = usernamelength + messageLength;
        snprintf(buf, total, "%s", users->message); 

        int numbytes = send(fd, buf, total, 0);
        return EXIT_SUCCESS;    
        
    }
  
    
    // Find the user to message - From the userList
    for (userNode client = users->first; client != NULL; 
                                                       client = client->next) {
        int client_fd = client->client_fd;
        
        // Check if the intended recipient is blacklisted
        int ignored = 0;
        ignored = getUserBlackListed (loginTimes, users->username, 
                                                             client->username);
        
        int userOnline = getLoginFlag (loginTimes, users->userToMessage);
        // Check if the recipient has blacklisted me 
        int imIgnored = 0;
        imIgnored = getUserBlackListed (loginTimes, client->username, 
                                                              users->username);
        // Send to users if they havent blocked me 
        if (client_fd != fd && !imIgnored && strcmp(client->username, 
                               users->userToMessage) == 0 && userOnline == 1) {
            int messageLength = strlen(users->message) - 1;
            strcpy(broadcastBuffer, users->message);
            int numbytes = send(client_fd, broadcastBuffer, messageLength, 0);
            notOffline = 1;
       } else if (strcmp(client->username, users->userToMessage) == 0 
                                                          && userOnline == 0) {
            // Add to a list to send when the user logs in
            int messageLength = 200;
            printf("Added offline Message!!\n");
            notOffline = 1;
            addOfflineMsg (offlineMsgs, users->userToMessage, broadcastBuffer);  
       } else if (imIgnored) { //imIgnored
            strcpy(users->message, "Message: Can't reach user!\n");
            int size = strlen(users->message) + 1;
            int numbytes = send(fd, users->message, size, 0);
            notOffline = 1; // prevent sending an offline message
       }
   }
        
    // handle the case that the user hasnt logged in this session 
    // Add to a list to send when the user logs in
    if (notOffline == 0) {
        offlineMsgListInsert (offlineMsgs, users->userToMessage, 
                                                              broadcastBuffer);
    }
       
    return EXIT_SUCCESS;
}

// Broadcast message function. This function broadcasts a message to all other
// users. This is a handler for a thread.
int broadcast_handler(void *args_) {
  
    user_List users = (user_List) args_;

    char buf[100];
    char *broadcastBuffer = malloc(sizeof(char) * 200);
    strcpy(broadcastBuffer, users->message);
    char name[100] = {0};

    // get the users list
    int fd;
    for (userNode curr = users->first; curr != NULL; curr = curr->next) {
        if(strcmp(users->username, curr->username) == 0) {
            fd = curr->client_fd;
        }
    }

    for (userNode client = users->first; client != NULL; 
                                                       client = client->next) {
        int client_fd = client->client_fd;
        
        // Check if the intended recipient is blacklisted
        int ignored = 0;
        ignored = getUserBlackListed (loginTimes, users->username, 
                                                            client->username);
        
        // Check if the recipient has blacklisted me 
        int imIgnored = 0;
        imIgnored = getUserBlackListed (loginTimes, client->username, 
                                                             users->username);
        if (client_fd != fd  && !imIgnored) {
            
            int messageLength = strlen (broadcastBuffer) + 1;
            int numbytes = send(client_fd, broadcastBuffer, messageLength, 0);
            
        } else if (imIgnored) {
            strcpy(users->message, "Message: Can't reach some Users!\n");
            int size = strlen(users->message) + 1;
            int numbytes = send(fd, users->message, size, 0);
        }
   }
 
    return EXIT_SUCCESS;
}

int whoelsesince_handler(void *args_) {
    user_List users = (user_List) args_;
    char buf[100];
   
   // strcpy(broadcastBuffer, users->username);
    char name[100] = {0};
    int fd;
        // get the fd
    for (userNode curr = users->first; curr != NULL; curr = curr->next) {
        if(strcmp(users->message, curr->username) == 0) {
            fd = curr->client_fd;
        }
    }
    
    // Get the current time()
    time_t minutesAgo;
    time_t current_time;
    
    // Loop over the list containing the users
    for (loginTimesNode client = loginTimes->first; client != NULL; 
                                                       client = client->next) {
       time_t loggedInTime = client->firstLogin;
       time_t loggedOutTime = client->logoutTime;
        //printf(client->username);
        int messageLength = strlen(client->username) + 1;
        current_time = time(NULL);
        minutesAgo = current_time - (users->timeSince);//the time 10 minutes ago is 10*60
        int currentlyLoggedIn = getLoginFlag (loginTimes, client->username);
        // get the time
        
        // Get the users who fit the time entered by the user who sent
        // the command
        if (strcmp(users->message, client->username) != 0 && 
                                                  (minutesAgo <= loggedInTime || 
            strcmp(users->message, client->username) != 0 && currentlyLoggedIn) 
                                           || ((minutesAgo <= loggedOutTime) && 
                              strcmp(users->message, client->username) != 0)) {
            snprintf(buf, messageLength, "%s", client->username); 
            int numbytes = send(fd, buf, messageLength, 0);
        }   
    }
    return EXIT_SUCCESS;
}

// Whoelse function. This function sends all the users who are online to the 
// user who requests this command.
int whoelse_handler(void *args_) {
    
    user_List users = (user_List) args_;
    
    // The send buffers
    char buf[100];
    char name[100] = {0};
        
    int fd;
        // get the fd
    for (userNode curr = users->first; curr != NULL; curr = curr->next) {
        if(strcmp(users->message, curr->username) == 0) {
            fd = curr->client_fd;
        }
    }
    
    // Find the users who are online and send to the client who requested the
    // whoelse
    for (userNode client = users->first; client != NULL; 
                                                       client = client->next) {
       
        int messageLength = strlen(client->username) + 1;
        if (strcmp(users->message, client->username) != 0 && 
                                                       client->loggedIn == 1) {
            snprintf(buf, messageLength, "%s", client->username); 
            int numbytes = send(fd, buf, messageLength, 0);
        }   
    }  
    return EXIT_SUCCESS;
}

// This function is responsible for sending offline messages when the user logs
// in. It is also responsible for calling another function to delete that entry
// from the offline message function.
int offlineMsgs_handler(void *args_) {
    user_List users = (user_List) args_;

    // Send buffers
    char buf[100];
    char name[100] = {0};
    
    int fd;
    // get the right user
    for (userNode curr = users->first; curr != NULL; curr = curr->next) {
        if(strcmp(users->username, curr->username) == 0) {
            fd = curr->client_fd; 
        }
    }
    

    for (userNode client = users->first; client != NULL; client = client->next) {
        int client_fd = client->client_fd;
        
        
        // get the messages that need to go to this user
        if (client_fd == fd ) {
            int i = 0;
            for (offlineMsgNode curr = offlineMsgs->first; curr != NULL; 
                                                           curr = curr->next) {
                if(strcmp(users->username, curr->username) == 0) {
                    for (int i = 0; i <= curr->messagesSize; i++) { 
                        int messageLength = strlen(curr->messages[i]) + 1;
                        int numbytes = send(client_fd, (curr->messages[i]), 
                                                            messageLength, 0);
                        
                        // PROCEED TO DELETE MESSAGE
                        removeOfflineMsg (offlineMsgs, curr->username, 
                                                        (curr->messages[i]));
                    }
                }
            }

        }
   }
    return EXIT_SUCCESS;
}

// This function is responsible for sending an "Invalid command" message 
// back to the user.
int invalidCommand_handler(void *args_) {

    user_List users = (user_List) args_;

    char buf[100];   
    char name[100] = {0};

    int fd;
        // get the fd of the user we are interested in
    for (userNode curr = users->first; curr != NULL; curr = curr->next) {
        if(strcmp(users->username, curr->username) == 0) {
            fd = curr->client_fd;
        }
    }
    
    // Send the invalid command back to the user
    strcpy(buf, users->username);
    strcat(buf, " : Invalid Command!");
    int messageLength = strlen(buf) + 1;

    int numbytes = send(fd, buf, messageLength, 0);
     
    return EXIT_SUCCESS;

}

// Checks if a user is in the credentials file
int checkFileForUser(FILE * fp, char * username) {
    // check the name entered is on the list of users
    int success = 0;
    int num;
    char *tmpUser = malloc (sizeof (char) * 100);
    char *tmpPass = malloc (sizeof (char)* 100);
    while ((num = fscanf(fp, "%s %s", tmpUser, tmpPass)) != EOF) {
    //fgetc(fp); // read the newline
    
        if ((strcmp(tmpUser, username) == 0)) {
            return 1;
        } 
    }

    return 0;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                          users.c                                                                                             0000640 0124623 0124623 00000044303 13565447612 011250  0                                                                                                    ustar   z3470429                        z3470429                                                                                                                                                                                                               /*
/   This file is responsible for defining the data structures and the prototypes
/   used in the file Client.c and clientP2PList.c,
/   This function helps set up the LISTS that contain the server and client
/   information.
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
#include "threadhandlers.h"
#include <err.h>

/** Create a new, empty IntList. */
struct UserListRep* NewUserList (void)
{
	struct UserListRep *L = malloc (sizeof *L);
	if (L == NULL) err (EX_OSERR, "couldn't allocate UserList");
	L->size = 0;
	L->username = malloc(sizeof(char)*100);
	L->message = malloc(sizeof (char) * 2000);
	L->userToMessage = malloc(sizeof (char) * 100);
	L->timeSince = 0; // Used to set the time to figure out when the user was logged in last
	L->waitPrivate = 0;
	L->first = NULL;
	L->last = NULL;
	return L;
}

struct clientsRep* newClientsList (void) {
    struct clientsRep *L = malloc (sizeof *L);
	if (L == NULL) err (EX_OSERR, "couldn't allocate UserList");
	L->size = 0;
	L->first = NULL;
	L->last = NULL;
	return L;
}

struct clientsNode *newClientListNode (char *name, int fd)
{
	struct clientsNode *n = malloc (sizeof *n);
	if (n == NULL) err (EX_OSERR, "couldn't allocate UserList node");
	
    // or malloc (char * 100)
	n->username = malloc(sizeof (name) + 1);
	n->fd = fd;
	
	strcpy(n->username, name);

	n->next = NULL;
	return n;
}
///////////////////////////////////////////////////////////////////////////////
//
//      ARGS
//
///////////////////////////////////////////////////////////////////////////////
Args new_args(List list, int fd) {
    Args args = calloc(1, sizeof(*args));
    args->list = list;
    args->fd = fd;
    return args;
}

//////////////////////////////////////////////////////////////////////////////
//
//  List Functions
//
//////////////////////////////////////////////////////////////////////////////
List list_new(void) {
    List list = calloc(1, sizeof(*list));
    return list;
}

void list_add(List list, struct client_info client) {
    Node new = node_new(client);
    list_add_node(list, new);
}

void list_add_node(List list, Node node) {
    assert(list != NULL);

    if (list->tail == NULL) {
        assert(list->head == NULL);
        list->head = list->tail = node;
    } else {
        assert(list->head != NULL);
        list->tail->next = node;
        node->prev = list->tail;
        list->tail = node;
    }
}


void list_remove(List list, Node to_remove) {

    if (to_remove == NULL) {
        //fprintf(stderr, "Tried to remove a node that wasn't in the list!");
        return;
    }

    if (list->head == to_remove) {
        assert(to_remove->prev == NULL);
        list->head = to_remove->next;
    }

    if (list->tail == to_remove) {
        assert(to_remove->next == NULL);
        list->tail = to_remove->prev;
    }

    if (to_remove->next) to_remove->next = to_remove->next->next;
    if (to_remove->prev) to_remove->prev = to_remove->prev->prev;

    node_destroy(to_remove);

}


void list_destroy(List list) {
    for (Node curr = list->head; curr != NULL;) {
        Node tmp = curr;
        curr = curr->next;
        node_destroy(tmp);
    }


    free(list);
}

Node node_new(struct client_info client) {
    Node node = calloc(1, sizeof(*node));
    node->client = client;
    return node;
}

void node_destroy(Node node) {
    free(node);
}

//////////////////////////////////////////////////////////////////////////////
//
//  loginTimesNode
//
//////////////////////////////////////////////////////////////////////////////

// The list
struct loginTimesRep* newLoginTimesList (void) {
    struct loginTimesRep *L = malloc (sizeof *L);
	if (L == NULL) err (EX_OSERR, "couldn't allocate List");
	L->size = 0;
	L->first = NULL;
	L->last = NULL;
	return L;
}

// Node for loginsTime - holds the time of each user
struct loginTimesNode *newLoginTimeNode (char *username, time_t firstLogin, 
                                                                       int flag)
{
	struct loginTimesNode *n = malloc (sizeof *n);
	if (n == NULL) err (EX_OSERR, "couldn't allocate List node");
	
	n->username = malloc(sizeof (char) * 100);
	strcpy(n->username, username);
	n->firstLogin = firstLogin;
	n->loggedIn = flag;
	n->attempts = 1;
    n->next = NULL;
    n->blacklist = malloc(sizeof (char*) * 10);
	for (int i = 0; i < 10; i++) {
	    n->blacklist[i] = malloc(sizeof (char ) * 100);
	}
	n->blacklistSize = 0;
	
	
	return n;
}

// add to the list
void loginTimesInsert (loginTimes_List L, char *username, time_t firstLogin, 
                                                                      int flag)
{
	assert (L != NULL);
    
	struct loginTimesNode *n =newLoginTimeNode (username, firstLogin, flag);
	if (L->first == NULL) {
		L->first = L->last = n;
    } else {
		L->last->next = n;
		L->last = n;
	}
	L->size++;
}

/* Returns the login time of the user */
time_t getLoginTime (loginTimes_List list, char *name) {
    struct loginTimesNode *tmp;
    tmp = list->first;
    while (tmp != NULL) {
        if (strcmp(tmp->username, name) == 0) {
            //printf("Login Attempts: %d\n", tmp->loginAttempts);
            return tmp->firstLogin;
        }
        tmp = tmp->next;
    }
    return 0;
}

// Return the logged out time of this user
time_t getLogoutTime (loginTimes_List list, char *username ) {
    struct loginTimesNode *tmp;
    tmp = list->first;
    while (tmp != NULL) {
        if (strcmp(tmp->username, username) == 0) {
            return tmp->logoutTime;
        }
        tmp = tmp->next;
    } 
    return 0;
}

/* Rset the login time of the user */
void setLoginTime(loginTimes_List list, char *name, time_t time , int flag) {
    struct loginTimesNode *tmp;
    tmp = list->first;
    while (tmp != NULL) {
        if (strcmp(tmp->username, name) == 0) {
            tmp->firstLogin = time;
            tmp->loggedIn = flag;
        }
        tmp = tmp->next;
    }
}

/* Returns 1 if A user is already on the list */
int isInTimeTracker (loginTimes_List list, char *name) {
    struct loginTimesNode *tmp;
    tmp = list->first;
    
    if (tmp == NULL) {
        return 0;
    }
    while (tmp != NULL) {
        if (strcmp(tmp->username, name) == 0) {
            return 1;
        }
        tmp = tmp->next;
    }
    return 0;
}

// Check if the user is loggedin 1 if yes 0 if no
int getLoginFlag (loginTimes_List list, char *username) {
    struct loginTimesNode *tmp;
    tmp = list->first;
    if (tmp == NULL) {
        return 0;
    }
    while (tmp != NULL) {
        if (strcmp(tmp->username, username) == 0) {
            return tmp->loggedIn;
        }
        tmp = tmp->next;
    }
    return 0;
}

/* Get the amount of attempts for the user */
int getAttemptUser (loginTimes_List list, char *name) {
    struct loginTimesNode *tmp;
    tmp = list->first;
    while (tmp != NULL) {
        if (strcmp(tmp->username, name) == 0) {
            return tmp->attempts;
        }
        tmp = tmp->next;
    }
    return 0; // Return if nothing is found
}

/* set the block-time for the user */
void setBlockTimeUser (loginTimes_List list, char *name, time_t time) {
    struct loginTimesNode *tmp;
    tmp = list->first;
    while (tmp != NULL) {
        if (strcmp(tmp->username, name) == 0) {
            //printf("Login Attempts: %d\n", tmp->loginAttempts);
            tmp->blockTime = time;
        }
        tmp = tmp->next;
    }
}

/* Increment the Attempts of a user */
void incrementAttemptUser (loginTimes_List list, char *name) {
    struct loginTimesNode *tmp;
    tmp = list->first;
    while (tmp != NULL) {
        if (strcmp(tmp->username, name) == 0) {
            //printf("Invrement me!:\n");
            tmp->attempts += 1;
        }
        tmp = tmp->next;
    }
}

/* Reset to 0 the Attempts of a user */
void resetAttemptUser (loginTimes_List list, char *name) {
    struct loginTimesNode *tmp;
    tmp = list->first;
    while (tmp != NULL) {
        if (strcmp(tmp->username, name) == 0) {
            tmp->attempts = 1;
        }
        tmp = tmp->next;
    }
}

/* Returns the block-time for the user */
time_t getBlockTimeUser(loginTimes_List list, char *name) {
    //printf("gET Attempts: %s\n", name);
    struct loginTimesNode *tmp;
    tmp = list->first;
    while (tmp != NULL) {
        if (strcmp(tmp->username, name) == 0) {
            return tmp->blockTime;
        }
        tmp = tmp->next;
    }
    return 0; // Return if nothing is found
}


///////////////////////////////////////////////////////////////////////////////
//
//      End of LoginTimes
//
///////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//  OfflineMessageNode
//
//////////////////////////////////////////////////////////////////////////////

// List
struct offlineMsgRep* newofflineMsgList (void) {
    struct offlineMsgRep *L = malloc (sizeof *L);
	if (L == NULL) err (EX_OSERR, "couldn't allocate UserList");
	L->size = 0;
	L->first = NULL;
	L->last = NULL;
	return L;
}

// Offline msgs Node
struct offlineMsgNode *newofflineMsgNode (char *name, char *message)
{
	struct offlineMsgNode *n = malloc (sizeof *n);
	if (n == NULL) err (EX_OSERR, "couldn't allocate UserList node");
	
	n->username = malloc(sizeof (char) * 100);
	n->messages = malloc(sizeof (char*) * 1);
	for (int i = 0; i < 1; i++) {
	    n->messages[i] = malloc(sizeof (char ) * 100);
	    strcpy(n->messages[i], message);
	}
	n->messagesSize = 0;
	strcpy(n->username, name);
	n->next = NULL;
	return n;
}

// Add a msg
void offlineMsgListInsert (offlineMsg_List L, char *name, char *message)
{
	assert (L != NULL);
    
	struct offlineMsgNode *n = newofflineMsgNode (name, message);
	if (L->first == NULL) {
		L->first = L->last = n;
    } else {
		L->last->next = n;
		L->last = n;
	}
	L->size++;
}

// Delete an entry from the blacklist
void removeOfflineMsg (offlineMsg_List L, char *myname, char *message)
{
	assert (L != NULL);

	// find where v occurs in list
	struct offlineMsgNode *curr = L->first;
	while (curr != NULL) {
	    if (strcmp(curr->username, myname) == 0) {
	        for (int i = 0; i <= curr->messagesSize; i++) {
                if (strcmp(message, curr->messages[i]) == 0) {
                    if ((i+1) <= curr->messagesSize) {
                        curr->messages[i] = curr->messages[i+1];
                        curr->messages[i+1] == NULL;
                        free(curr->messages[i+1]);
                        curr->messagesSize--;
                    } else {
                        curr->messages[i] == NULL;
                        free(curr->messages[i]);
                        curr->messagesSize--;
                    }
                } 
            }
	    }
	    curr = curr->next;	
	}
}

// Add a message
void addOfflineMsg (offlineMsg_List list, char *name, char *message) {
    struct offlineMsgNode *tmp;
    tmp = list->first;
    
    while (tmp != NULL) {
        if (strcmp(tmp->username, name) == 0) {
            if (tmp->messagesSize < list->size) {
                strcpy(tmp->messages[tmp->messagesSize], message);
                tmp->messagesSize++;
            } else {
               tmp->messages = realloc (tmp->messages, tmp->messagesSize + list->size);
                for (int i = tmp->messagesSize; i <= list->size; i++) {
                    int size = strlen(message) + 1;
                    tmp->messages[i] = malloc(sizeof (char ) * size);
                }
                strcpy(tmp->messages[tmp->messagesSize], message);
                tmp->messagesSize++;
            }
        }
        tmp = tmp->next;
    }
}


///////////////////////////////////////////////////////////////////////////////
//
//      End of offlineMsgs
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//                  Client List
//
///////////////////////////////////////////////////////////////////////////////
// List
void clientListInsert (client_List L, char *name, int client_fd)
{
	assert (L != NULL);
    
	struct clientsNode *n = newClientListNode (name, client_fd);
	if (L->first == NULL) {
		L->first = L->last = n;
    } else {
		L->last->next = n;
		L->last = n;
	}
	L->size++;
}

// CLient Node 
struct UserListNode *newUserListNode (char *name, int loginAttempts, 
                                                        int loggedIn,  int fd)
{
	struct UserListNode *n = malloc (sizeof *n);
	if (n == NULL) err (EX_OSERR, "couldn't allocate UserList node");
	n->loginAttempts = loginAttempts;
	n->loggedIn = loggedIn;
	n->username = malloc(sizeof (char) * 100);
	n->welcomeBroadcast = malloc(sizeof (char) * 100);
	strcpy(n->welcomeBroadcast, " has logged in");
	n->blockTime = 0;
	n->loggedInTimer = 0;
	
	strcpy(n->username, name);
	n->client_fd = fd;
	n->next = NULL;
	return n;
}

// Delete an entry from the blacklist
void removeBlackList (loginTimes_List L, char *myname, char *unblName)
{
	assert (L != NULL);

	// find where v occurs in list
	struct loginTimesNode *curr = L->first;
	while (curr != NULL) {
	    if (strcmp(curr->username, myname) == 0) {
	        for (int i = 0; i <= curr->blacklistSize; i++) {
                if (strcmp(unblName, curr->blacklist[i]) == 0) {
                    if ((i+1) <= curr->blacklistSize) {
                        curr->blacklist[i] = curr->blacklist[i+1];
                        curr->blacklist[i+1] == NULL;
                        free(curr->blacklist[i+1]);
                        curr->blacklistSize--;
                    } else {
                        curr->blacklist[i] == NULL;
                        free(curr->blacklist[i]);
                        curr->blacklistSize--;
                    }

                }
		        
            }
	    }
	    curr = curr->next;
		
	}
}

// Add a user to the blacklist
void setBlackListUser (loginTimes_List list, char *myname, char *userToBl) {
    struct loginTimesNode *tmp;
    tmp = list->first;
    
    while (tmp != NULL) {
        if (strcmp(tmp->username, myname) == 0) {
            if (tmp->blacklistSize <= list->size) {
                strcpy(tmp->blacklist[tmp->blacklistSize], userToBl);
                tmp->blacklistSize++;
            } else {
               tmp->blacklist = realloc (tmp->blacklist, tmp->blacklistSize + list->size);
                for (int i = tmp->blacklistSize; i <= list->size; i++) {
                    tmp->blacklist[i] = malloc(sizeof (char ) * 100);
                    strcpy(tmp->blacklist[i], userToBl);
                }
                
                tmp->blacklistSize++;
            }
        }
        tmp = tmp->next;
    }
}

// Check if a user is blacklisted, returns 0 if the user isn't
// returns 1 if the user is
int getUserBlackListed (loginTimes_List list, char *myname, char *user) {
    struct loginTimesNode *tmp;
    tmp = list->first;
     
     while (tmp != NULL) {  
        if (strcmp(tmp->username, myname) == 0) {
            for (int i = 0; i < tmp->blacklistSize; i++) {
                if (strcmp(tmp->blacklist[i], user) == 0) {
                    return 1;
                }
            }
        }
        tmp = tmp->next;
    }
    return 0;
}





/* Returns 1 if A user is already on the list */
int isInUserList (user_List list, char *name) {
    struct UserListNode *tmp;
    tmp = list->first;
    
    if (tmp == NULL) {
        return 0;
    }
    while (tmp != NULL) {
        if (strcmp(tmp->username, name) == 0) {
            return 1;
        }
        tmp = tmp->next;
    }
    return 0;
}

/* Returns the block-time for the user */
void setServerSendTagUserList (user_List list, char *name, int tag) {
    struct UserListNode *tmp;
    tmp = list->first;
    while (tmp != NULL) {
        if (strcmp(tmp->username, name) == 0) {
            //printf("Login Attempts: %d\n", tmp->loginAttempts);
            tmp->sendTag = tag;
        }
        tmp = tmp->next;
    }
}

/* Returns the block-time for the user */
int getServerSendTagUserList (user_List list, char *name) {
    struct UserListNode *tmp;
    tmp = list->first;
    while (tmp != NULL) {
        if (strcmp(tmp->username, name) == 0) {
            //printf("Login Attempts: %d\n", tmp->loginAttempts);
            return tmp->sendTag;
        }
        tmp = tmp->next;
    }
    return 0;
}

void setLoginFlagInUserList (user_List list, char *username, int loggedIn) {
    struct UserListNode *tmp;
    tmp = list->first;
    while (tmp != NULL) {
        if (strcmp(tmp->username, username) == 0) {
            tmp->loggedIn = loggedIn;
        }
        tmp = tmp->next;
    }
}

// Set the broadcast message of the user
void setBroadcastUserList (user_List list, char *username, char *msg) {
    struct UserListNode *tmp;
    tmp = list->first;
    while (tmp != NULL) {
        if (strcmp(tmp->username, username) == 0) {
            strcpy(tmp->welcomeBroadcast, msg);
        }
        tmp = tmp->next;
    }
}

/** Apppend one integer to the end of an UserList. */
void UserListInsert (user_List L, char *name, int loginAttempts, int loggedIn,  
                                                                 int client_fd)
{
	assert (L != NULL);
    
	struct UserListNode *n = newUserListNode (name, loginAttempts, loggedIn, 
	                                                                client_fd);
	if (L->first == NULL) {
		L->first = L->last = n;
    } else {
		L->last->next = n;
		L->last = n;
	}
	L->size++;
}

/** Delete first occurrence of `v` from an UserList.
 * If `v` does not occur in IntList, no effect */
void UserListDelete (user_List L, int v)
{
	assert (L != NULL);

	// find where v occurs in list
	struct UserListNode *prev = NULL;
	struct UserListNode *curr = L->first;
	while (curr != NULL) {
		prev = curr;
		curr = curr->next;
	}

	// not found; give up
	if (curr == NULL)
		return;

	// unlink curr
	if (prev == NULL)
		L->first = curr->next;
	else
		prev->next = curr->next;
	if (L->last == curr)
		L->last = prev;
	L->size--;

	// drop curr
	free (curr);
}

/** Release all resources associated with an UserList. */
void freeUserList (user_List L)
{
	if (L == NULL) return;

	for (struct UserListNode *curr = L->first, *next;
		curr != NULL; curr = next) {
		next = curr->next;
		free (curr);
	}

	free (L);
}



                                                                                                                                                                                                                                                                                                                             clientP2P.h                                                                                         0000640 0124623 0124623 00000005614 13565432022 011703  0                                                                                                    ustar   z3470429                        z3470429                                                                                                                                                                                                               /*
/   This file is responsible for defining the data structures and the prototypes
/   used in the file Client.c and clientP2PList.c,
/   This function helps set up the LISTS that contain the server and client
/   information.
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
#include <time.h>

#ifndef P2PLIST_H
#define P2PLIST_H


// These are the structs
typedef struct p2pConnectionsNode *p2p_node;
typedef struct p2pConnectionsRep *p2p_List;
typedef struct p2pClientThread *p2pClient;
typedef struct p2pServerThread *p2pServer;
typedef struct clientThread *client_info;
typedef struct timeoutThread *timeout;

// Global initialisers
extern p2pClient client;
extern client_info connectionInfo;
extern p2pServer server_info;
extern int isOn;
extern p2p_List p2p_ServerClients;
extern p2p_List p2p_Clients;

// Nodes for the list
struct p2pConnectionsNode {
    int serverfd;
    int clientfd;
    char * username;
    struct p2pConnectionsNode *next;
};

// The list structure itself
struct p2pConnectionsRep {
	int size;        

	struct p2pConnectionsNode *first;
	struct p2pConnectionsNode *last;
};

// The timeout thread structure 
struct timeoutThread {
    int fd;
    int authorised;
};

// The client thread struct
struct clientThread {
    const char * server_name;
    int server_port;
    const char * p2pServer_name;
    int p2p_port;
    struct sockaddr_in p2p_server;
    char * message;
};

// Helper functions
struct p2pServerThread {
    int fd;
    int isActive;

    char * myName;
    char * userToMessage;
    
    int serverfd;
    int clientfd;
    int iAmServer;
};

struct p2pClientThread {
    int fd;
    int isActive;
    int isServer;
    char * userToMessage;
};

///////////////////////////////////////////////////////////////////////////////
//                  P2P list functions                                      ///
///////////////////////////////////////////////////////////////////////////////
struct p2pConnectionsRep* NewP2PList (void);
struct p2pConnectionsNode *newP2PNode (char *username, int serverfd,
                                                                 int clientfd);
void P2PNodeInsert (p2p_List L, char *username, int serverfd, int clientfd);
void removeFromList (p2p_List L, char *username);

// client function headers
int timeoutHandler (void *info);
int clientHandler (void *info);
int p2pHandler (void *info);
int initiateP2PHandler (void *info); // for client only
int p2pServer_handler (void *info); // for server

// send the p2p message
int sendPrivate(char *message);
int sendPrivateServer(char *message);
int p2pClientListener_handler(void * info);
int checkFileForUser(FILE * fp, char * username);

#endif
                                                                                                                    serverList.h                                                                                        0000640 0124623 0124623 00000003062 13565437127 012253  0                                                                                                    ustar   z3470429                        z3470429                                                                                                                                                                                                               /*
/   This file is not too important. These functions are used but scarcly.
/   They are needed to hold temporary information on the user connecting to
/   the server
/
/   ZID : 3470429
/   NAME : STEPHEN COMINO
*/

#include <time.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "userLogins.h"

#ifndef CS3331_SERVER_H
#define CS3331_SERVER_H


typedef struct client_list *List;
typedef struct args *Args;
typedef struct client_node *Node;

// Arguments struct (and creation function) to pass the required info
// into the thread handlers.
struct args {
    List list;
    int fd;
    int block_duration;
    int timeout;
    struct sockaddr_in *tempSa;  // Hold the clients information temporarily
    user_List *clients;
};

struct client_list {
    Node head, tail;
};

struct client_info {
    char host[INET_ADDRSTRLEN];
    int port;
    char *user;
};

struct client_node {
    struct client_info client;
    Node next, prev;
};

// These are the functions for these lists
Args new_args(List list, int fd);
List list_new(void);
void list_add_node(List list, Node node);
void list_add(List list, struct client_info client);
void list_remove_client(List list, struct client_info client);
void list_remove(List list, Node to_remove);
Node list_find(List list, struct client_info client);
void list_destroy(List list);
void print_list(List list);
struct client_info get_client_info(struct sockaddr_in *sa, char *user);
Node node_new(struct client_info client);
void node_destroy(Node node);
////////////////////////////////////////////////////////////////////////


#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                              threadhandlers.h                                                                                    0000640 0124623 0124623 00000002423 13565437261 013100  0                                                                                                    ustar   z3470429                        z3470429                                                                                                                                                                                                               /*
/   This header file defines the prototypes for threads.c.
/   Threads.c contains the threadhandlers for server.c
/   
/   ZID : 3470429
/   NAME : STEPHEN COMINO
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
#include "userLogins.h"                   // Thread Lists


#ifndef THREADH_H
#define THREADH_H

// GLobal Variables
extern user_List users;
extern client_List clients;
extern offlineMsg_List offlineMsgs;
extern loginTimes_List loginTimes;

int loggedIn_handler(void *info);
int logout_handler(void *info);
int whoelse_handler(void *info);
int whoelsesince_handler(void *info);
int blacklist_handler(void *info);
int unblock_handler(void *info);
int message_handler(void *info);
int offlineMsgs_handler(void *info);
int recv_handler(void *info);
int timeout_handler(void *info);

// p2p handler
int private_handler(void *info);
int sendPrivate_handler (void *info);
// Broadcast handlers
int loginBroadcast_handler(void *info);
int broadcast_handler(void *info);
int invalidCommand_handler(void *info);
int checkFileForUser(FILE * fp, char * username);

#endif
                                                                                                                                                                                                                                             userLogins.h                                                                                        0000640 0124623 0124623 00000017131 13565447141 012242  0                                                                                                    ustar   z3470429                        z3470429                                                                                                                                                                                                               #include <arpa/inet.h>
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
#include <time.h>

#ifndef USERLIST_H
#define USERLIST_H

/** External view of IntList ... implementation in IntList.c */
typedef struct UserListRep *user_List;
typedef struct clientsRep *client_List;
typedef struct loginTimesRep *loginTimes_List;
typedef struct loginTimesNode *loginTimesNode;
typedef struct offlineMsgRep *offlineMsg_List;
typedef struct clientsNode *clientNode;
typedef struct UserListNode *userNode;
typedef struct offlineMsgNode *offlineMsgNode;
typedef struct timeoutNode *timeKeeper;

// data structures representing UserList
struct UserListNode {
	int loginAttempts;           /**< Records the amount of login attempts */
	int loggedIn;               /**< Is 1 when the user is logged in */
	time_t blockTime;           /** signed 32-bit Int */
	time_t loggedInTimer;       /** Timer that controls when to log out AFK*/
	int sendTag;                /** Is 1 when it is the servers time to send*/
	char *username;
    int client_fd;
    char *welcomeBroadcast;
	
	struct UserListNode *next;
	                    /**< pointer to node containing next element */
};

struct timeoutNode {
    char *username;
    
    time_t currTime;
    int timeout;
    int socket;
    struct UserListRep *users;
    
};

struct UserListRep {
	int size;           /**< number of elements in list */
	     /** This is the name of the person calling the function */
	struct UserListNode *first;
	                    /**< node containing first value */
	struct UserListNode *last;
	                    /**< node containing last value */
    struct clientsRep *client;
    
    struct offlineMsgRep * offline;
    int waitPrivate;
    time_t timeSince;
    char *message;
	char *userToMessage;
	char *username;
};

///////////////////////////////////////////////////////////////////////////////
//
//              Clients list
//
///////////////////////////////////////////////////////////////////////////////
struct clientsNode {
   char * username;
   int fd;
   struct clientsNode *next;
};

struct clientsRep {
    struct clientsNode *first;
	                    /**< node containing first value */
	int size;
	struct clientsNode *last;
};

// Linkedlists to deal with offline messages
struct offlineMsgRep {
    struct offlineMsgNode *first;
	                    /**< node containing first value */
	int size;
	struct offlineMsgNode *last;
};

struct offlineMsgNode {
   char * username;
   char ** messages;
   int messagesSize;
   struct offlineMsgNode *next;
};

///////////////////////////////////////////////////////////////////////////////
//
//  Persistant login-times
//
///////////////////////////////////////////////////////////////////////////////
struct loginTimesRep {
    struct loginTimesNode *first;
	                    /**< node containing first value */
	int size;
	struct loginTimesNode *last;
};

struct loginTimesNode {
   char * username;
   time_t firstLogin;
   time_t logoutTime;
   char **blacklist;
    int blacklistSize;
   int loggedIn;
   int attempts;
   time_t blockTime;
   struct loginTimesNode *next;
};

////////////////////////////////////////////////////////////////////////
// List ADT
typedef struct client_list *List;
typedef struct args *Args;
typedef struct client_node *Node;

// Arguments struct (and creation function) to pass the required info
// into the thread handlers.
struct args {
    List list;
    int fd;
    int block_duration;
    int timeout;
    struct sockaddr_in *tempSa;  // Hold the clients information temporarily
    user_List *clients;
};

struct client_list {
    Node head, tail;
};

struct client_info {
    char host[INET_ADDRSTRLEN];
    int port;
    char *user;
};

struct client_node {
    struct client_info client;
    Node next, prev;
};

Args new_args(List list, int fd);
List list_new(void);
void list_add_node(List list, Node node);
void list_add(List list, struct client_info client);
void list_remove_client(List list, struct client_info client);
void list_remove(List list, Node to_remove);
Node list_find(List list, struct client_info client);
void list_destroy(List list);
void print_list(List list);
struct client_info get_client_info(struct sockaddr_in *sa, char *user);
Node node_new(struct client_info client);
void node_destroy(Node node);
////////////////////////////////////////////////////////////////////////


/** Create a new, empty IntList. */
struct UserListRep *NewUserList (void);
/** Release all resources associated with an IntList. */
void freeUserList (user_List list);

/** Apppend one integer to the end of an IntList. */
void UserListInsert (user_List list, char *name, int loginAttempts, 
                                                        int loggedIn, int fd);

/** Delete first occurrence of `v` from an IntList.
 * If `v` does not occur in IntList, no effect. */
void UserListDelete (user_List list, int v);
void incrementAttemptUserList (user_List list, char *username);
void flagLoggedInUserList (user_List list, char *username, int flag, 
                                                            time_t loggedOut);
int getLoginFlagInUserList (user_List list, char *username);
void setLoginFlagInUserList (user_List list, char *username, int loggedIn);
void setLoginTimeUserList (user_List list, char *name, time_t time);
int isInUserList (user_List list, char *name);
void setFirstLoginUserList (user_List list, char *username, time_t time);
time_t getFirstLoginUserList (user_List list, char *name); // when first loggin occurs
void setServerSendTagUserList (user_List list, char *name, int tag);
int getServerSendTagUserList (user_List list, char *name);
void setBroadcastUserList (user_List list, char *username, char *msg);

// BlackList user functions
void setBlackListUser (loginTimes_List list, char *myname, char *userToBl);
int getUserBlackListed (loginTimes_List list, char *myname, char *user);
void removeBlackList (loginTimes_List L, char *myname, char *unblName);

// Offline messages 
void addOfflineMsg (offlineMsg_List list, char *name, char *message);
void removeOfflineMsg (offlineMsg_List L, char *myname, char *message);
void offlineMsgListInsert (offlineMsg_List L, char *name, char *message);
struct offlineMsgNode *newofflineMsgNode (char *name, char *message);
struct offlineMsgRep* newofflineMsgList (void);


// Login times functions
void loginTimesInsert (loginTimes_List L, char *username, time_t firstLogin, 
                                                                    int flag);
struct loginTimesNode *newLoginTimeNode (char *username, time_t firstLogin, 
                                                                    int flag);
int getLoginFlag (loginTimes_List list, char *username);
struct loginTimesRep* newLoginTimesList (void);
time_t getLoginTime (loginTimes_List list, char *name);
time_t getLogoutTime (loginTimes_List list, char *name);
void setLoginTime(loginTimes_List list, char *name, time_t time, int flag);
int isInTimeTracker (loginTimes_List list, char *name);
// for 3 attempts retry login
void resetAttemptUser (loginTimes_List list, char *name);
void incrementAttemptUser (loginTimes_List list, char *name);
void setBlockTimeUser (loginTimes_List list, char *name, time_t time);
int getAttemptUser (loginTimes_List list, char *name);
time_t getBlockTimeUser(loginTimes_List list, char *name);

//// Client list functions
struct clientsNode *newClientListNode (char *name, int client_fd);
struct clientsRep* newClientsList (void);
void clientListInsert (client_List L, char *name, int client_fd);

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                       Report.pdf                                                                                          0000640 0124623 0124623 00000441510 13565457653 011720  0                                                                                                    ustar   z3470429                        z3470429                                                                                                                                                                                                               %PDF-1.7
%����
1 0 obj
<</Type/Catalog/Pages 2 0 R/Lang(en-AU) /StructTreeRoot 30 0 R/MarkInfo<</Marked true>>/Metadata 121 0 R/ViewerPreferences 122 0 R>>
endobj
2 0 obj
<</Type/Pages/Count 3/Kids[ 3 0 R 25 0 R 27 0 R] >>
endobj
3 0 obj
<</Type/Page/Parent 2 0 R/Resources<</Font<</F1 5 0 R/F2 9 0 R/F3 11 0 R/F4 16 0 R/F5 18 0 R/F6 23 0 R>>/ExtGState<</GS7 7 0 R/GS8 8 0 R>>/ProcSet[/PDF/Text/ImageB/ImageC/ImageI] >>/MediaBox[ 0 0 595.32 841.92] /Contents 4 0 R/Group<</Type/Group/S/Transparency/CS/DeviceRGB>>/Tabs/S/StructParents 0>>
endobj
4 0 obj
<</Filter/FlateDecode/Length 4315>>
stream
x��]Yo#�~��Џd`���9��{�vĉ���F�P,E�3#����U=�.Q!���X���U_]����Z��}�p�]��e9g�]�n����X^��պh������]O��,�e�����>�?./�(�?Y�r3��H
�)�������֗��^^\���Q������3�R�B�4O�8c��}?}N�}c>��㣬{����o���i2y�^����wv��ˋO��}yၗ<�d����Q��+i�I�/��C�W�0��d3�ғ�e��<���-��ǩ�,��r��
�]A���!u�pj	���[eJ�c'���[�Z���7�闏�]�������?���� �Q�8�V���2�L��|Nn��Y��{׳���C�v�ɣ]�Si5��N]�eW:���<u:�b��o�a�%C�e�%���<1�9J#uG7����~�&��*��9����tد��n�3����_p�"Ž�ׁ5����H"y�0"N6k�2k1��y��7F���p�Cf�� ��4����E|�yn��>�*���쟭��X9��c[/��`�%Qz���˪9��:����"i��6�i���k��b�4�4M��W�V<>�W�Ѯ�`�kt���0l՛O����~j_5�|������u�8�ҔZױMU�7՘By��/ �V3�|b��ڇ��K�Nfl����ՔǓA������_�K�g2	��%䙎Ҍ g�,�x��#hK;ԫ]��1���A`���g{���y|���$��d�X�X���I�?D*#N-�vY���E½�A�a�4bO<H�i8�4ŅQK����1�u٘�Y�g����Ԟ��NV��^�b`�f��M��0� s��
�͘R	m����ԣ�I`(�(4�?��q
@�y:�̇�O�ڿ8z��r0��� {8��Ao\�[�-0�
�I�O��d�x�[s�������I5M��T���H��"���� �� ��1�lU����@�<D�LCL�nb*���+�������'�hDn�-�X�/������ᠪf�1}���6,�b	��H�C�;��2�#$L	��/�BV�A����̉K`������QFJ��K�B�{L0�1~b�j���� 6P1�C��:��E� ~G�Hfn��C�t�ͱ W�r� L��G9�E�U�r�����l�ĽܪO��g<?Vr�<Ϡ�@p�RX\B�E�����|�.�^���)�e���).�c�JC�ڎ��f�wv�d� f^�����E�g6���w����Q9�}N�b��?���v�M<�(��" xH��r�D��6Tj��,��1o�iK�ꥠND�;���7=��ż��Y� �aݑ��.�il���:Ca�
�h޿��e�E8@_GIB�+ت��<=w�]���,�A���v�n+q�F�֝)@��7��F����ɨ�M�َ�bS�q.:%8�A�sw�8�;ie9���|�H���^'NdR�o"w����[�z(o�8W.4:�O�5B�H	�1Aڨْ��4l��K�Q"(&l'Y ��L�G���A}(��K�~'s�pږ]��6��I)X�9k���H=�8���"�*��b���`��������zY>����W �+T$I����Ƒ	�	�6c��0&af�H][��3���:�~Z)�~��$�A��ʻáq�K��5ZC����B�Ҹ��<J��J����;J�c������|w�t��#|W��)`����ޮ��Zo��}*���m�w�-�m��ɔ�(�Ll���v�h'1��IU�$9��p�l�w��iQՠ�$�T��-��%TP7��ǙS��<�5fC���g��Ls��PgY��1_S�n�1��6�V����;#R`E�`hV��Em[O9$�:�#�~�����=�@��@`�f�SR�	�Si���j���R�䥎��?��1���ؿ��1tD䎙�̿��	 �}F~�`'6�12F�o��81F��^?��^q��1����rI�G�	�� Y]�	z���k����2qe�u�����	v�`<6��
x��Qhډ5@�ה��EmY<�Eu�F[}�\Md6H����:�bWI��Y&F�g�
;�J�ae�.\i�bX?V�,�p�J�H�C g������1ER�7-���m��^h�J]���K'NC���:���y�汆�+A�B��p{��0��RH�\,�n��~|�=�������@���Z�jc����.�g�=� �~�g�|�Q�m�bà]��!�0D�5��7�X�!�1�i�i�jaD��1���R���C���@�m�w�%��#�S"�=�P���![�>��`�^�8��ݲh����<���MԿ���(KHzGM��QQ�sh���9�?+*��	���=>��Y'1`�Y�_���A	0:
A��0��E�2 ����/��̨01���stT	N�ue
z%�b�Y���O�	�:��g/s[rޜ0��$�ɵ^��#�ލ��p���=C��k`<���d��bq��;���cFQ=����g�NGQ��!�Bp���p��&�"N�r��ɥP ���m>�����.Μ�8��`~�@|o ��$Jr��!���W�I	��'XR�crx����&�z�7�2T�!�Nֶ�$�|�3ddk�1�ozǧ�	;b��sL��Ȼ�E�� �4�@7�`hZ*	JtX��N/�a�ޟ������%�D���c����<�v+�8��0o�p�S,b�D_ik8���L������а� �[WĴ��9P"�u����KgW���P#)��+�I�������R66��5Crs�*��ΰ.V8�k>
���R�E��C��0�u7;e6
Q2�e А@B��������aA�a'���]Y��A?֔�yj�5K񌿲��C�F��ʾ���t��%�M52�rr�GѶ�)>	EW9\�	A����m��eR֐����0��� �������o��#�s����:a�y75�U�Ѹc���X]�J���;��Ʈ7txA��$�M	���l�����B��+�k���QQ@�6n�*���o{�j�
�`�ⱀ]����j��	�uF���u��jao��Co�2���_���p1�1q��ׁ�E�ƚ�j�݊�kQ ��l!:[��XV��{^ֻ�:@�D&0/F�Y�1�x��5��OU��U٥�0H���'�͞�wtd����4�Tr+��+�m�"�n�f�qS=�wiczkQu���r�apE��cpE�ij��8�qWW�<
j�wk�
CV��0=0{�.�u����L�>^fo3���m��w�k\�BGѻ�S�q$Hz/;��>зqam�+�>��������c��z�BЄkx*qe	a��!��p$ѣ���ϻC'LJKb�d�ާ�����'�s���C�Q\�C�_��_b�m�c�]�)���dn�$�u '6,�R00*���f�*#��	6�
�'���kkn;qc�����ܴmu�eh�2�}�W���9+�
�����S,!8$�pVb�G��^��CӃkH��XF����s����ї�Rds��^�>���p��I�����n/q�k�K�a�+��kWw�������8��~�l��Ƕ�<�GY'�2�������Y�"m��"�Dg��%��m��`���Ȭ�N�q$;k�h�h��)��XW�rl���6~p�����5ر*V8 f�i���1yx��&�7e�p�u�� ��Jj�s�����൥�!�k琊;h��e7��@����^�Ԏ��)����XT�	��լ)6C +2�Z�[�_��"u_
�[c��H���nz�11��^�9�32pc�C[�Z�'l�&埶���H����tm"v�d�ߖ��9�F��&!B|���+҆'�[UJ��L���lw��d7���w�	
�^W6��;��P.�/XUqw��{R:w��9��nN��?j�=~�	l���i��Pq�]�Z���P �7� �X)r�#2v�+<{��Ko)W���F@8��Fc���ns����/�H.�L�Zc�R��#�)z�Ni��,ĥgY�b���g{�E����*�|�����s�E����VQ)�"�3whz<�wt��r���v�0�G+7逆��wvo���Hh>%HR6�ǚ�`�y4L߳b�7��V�2�"Pw 8����a�*Vl�g��|���2������o�fc@IA-�<��Z��m��7
endstream
endobj
5 0 obj
<</Type/Font/Subtype/TrueType/Name/F1/BaseFont/BCDEEE+Calibri/Encoding/WinAnsiEncoding/FontDescriptor 6 0 R/FirstChar 32/LastChar 121/Widths 113 0 R>>
endobj
6 0 obj
<</Type/FontDescriptor/FontName/BCDEEE+Calibri/Flags 32/ItalicAngle 0/Ascent 750/Descent -250/CapHeight 750/AvgWidth 521/MaxWidth 1743/FontWeight 400/XHeight 250/StemV 52/FontBBox[ -503 -250 1240 750] /FontFile2 111 0 R>>
endobj
7 0 obj
<</Type/ExtGState/BM/Normal/ca 1>>
endobj
8 0 obj
<</Type/ExtGState/BM/Normal/CA 1>>
endobj
9 0 obj
<</Type/Font/Subtype/TrueType/Name/F2/BaseFont/BCDFEE+Calibri-Light/Encoding/WinAnsiEncoding/FontDescriptor 10 0 R/FirstChar 32/LastChar 121/Widths 117 0 R>>
endobj
10 0 obj
<</Type/FontDescriptor/FontName/BCDFEE+Calibri-Light/Flags 32/ItalicAngle 0/Ascent 750/Descent -250/CapHeight 750/AvgWidth 520/MaxWidth 1820/FontWeight 300/XHeight 250/StemV 52/FontBBox[ -511 -250 1309 750] /FontFile2 115 0 R>>
endobj
11 0 obj
<</Type/Font/Subtype/Type0/BaseFont/BCDGEE+Calibri-Light/Encoding/Identity-H/DescendantFonts 12 0 R/ToUnicode 114 0 R>>
endobj
12 0 obj
[ 13 0 R] 
endobj
13 0 obj
<</BaseFont/BCDGEE+Calibri-Light/Subtype/CIDFontType2/Type/Font/CIDToGIDMap/Identity/DW 1000/CIDSystemInfo 14 0 R/FontDescriptor 15 0 R/W 116 0 R>>
endobj
14 0 obj
<</Ordering(Identity) /Registry(Adobe) /Supplement 0>>
endobj
15 0 obj
<</Type/FontDescriptor/FontName/BCDGEE+Calibri-Light/Flags 32/ItalicAngle 0/Ascent 750/Descent -250/CapHeight 750/AvgWidth 520/MaxWidth 1820/FontWeight 300/XHeight 250/StemV 52/FontBBox[ -511 -250 1309 750] /FontFile2 115 0 R>>
endobj
16 0 obj
<</Type/Font/Subtype/TrueType/Name/F4/BaseFont/BCDHEE+Calibri-Bold/Encoding/WinAnsiEncoding/FontDescriptor 17 0 R/FirstChar 32/LastChar 118/Widths 118 0 R>>
endobj
17 0 obj
<</Type/FontDescriptor/FontName/BCDHEE+Calibri-Bold/Flags 32/ItalicAngle 0/Ascent 750/Descent -250/CapHeight 750/AvgWidth 536/MaxWidth 1781/FontWeight 700/XHeight 250/StemV 53/FontBBox[ -519 -250 1263 750] /FontFile2 119 0 R>>
endobj
18 0 obj
<</Type/Font/Subtype/Type0/BaseFont/BCDIEE+Calibri/Encoding/Identity-H/DescendantFonts 19 0 R/ToUnicode 110 0 R>>
endobj
19 0 obj
[ 20 0 R] 
endobj
20 0 obj
<</BaseFont/BCDIEE+Calibri/Subtype/CIDFontType2/Type/Font/CIDToGIDMap/Identity/DW 1000/CIDSystemInfo 21 0 R/FontDescriptor 22 0 R/W 112 0 R>>
endobj
21 0 obj
<</Ordering(Identity) /Registry(Adobe) /Supplement 0>>
endobj
22 0 obj
<</Type/FontDescriptor/FontName/BCDIEE+Calibri/Flags 32/ItalicAngle 0/Ascent 750/Descent -250/CapHeight 750/AvgWidth 521/MaxWidth 1743/FontWeight 400/XHeight 250/StemV 52/FontBBox[ -503 -250 1240 750] /FontFile2 111 0 R>>
endobj
23 0 obj
<</Type/Font/Subtype/TrueType/Name/F6/BaseFont/ArialMT/Encoding/WinAnsiEncoding/FontDescriptor 24 0 R/FirstChar 32/LastChar 32/Widths 120 0 R>>
endobj
24 0 obj
<</Type/FontDescriptor/FontName/ArialMT/Flags 32/ItalicAngle 0/Ascent 905/Descent -210/CapHeight 728/AvgWidth 441/MaxWidth 2665/FontWeight 400/XHeight 250/Leading 33/StemV 44/FontBBox[ -665 -210 2000 728] >>
endobj
25 0 obj
<</Type/Page/Parent 2 0 R/Resources<</Font<</F1 5 0 R/F6 23 0 R/F4 16 0 R/F5 18 0 R/F2 9 0 R/F3 11 0 R>>/ExtGState<</GS7 7 0 R/GS8 8 0 R>>/ProcSet[/PDF/Text/ImageB/ImageC/ImageI] >>/MediaBox[ 0 0 595.32 841.92] /Contents 26 0 R/Group<</Type/Group/S/Transparency/CS/DeviceRGB>>/Tabs/S/StructParents 1>>
endobj
26 0 obj
<</Filter/FlateDecode/Length 5249>>
stream
x��]�o��n���~$��|�{�"`'N�E�Zh>��p"��E��NV��wf���ڑq'�ؤH����<~��7ow]��f����7o���-�����f��7�_�����m���f�~���÷�RW�z��w��߫��~G1�W�V�J�4�F��J�v��W?�I�_�zw��՛��:�u�x�
?+�r�&Qy�Eq����s?}��m�Z�ҫ����W�L��L����*�l��Q�}��=���~����l|���S��+�����K�W��d3�J'�N�7���ħ�ƿ�S;Y��Z�
R��f|	ƦQ��%l�Ӄ�ڜ��^H/o��*L�%!yS����Ro>����~Pq��PƎ鴌�S�*�Ybru=��]�,3��O8�/��T�V���׳ZuxL��Qx��Ĥ��k@'�n��D��ȭw�:�֓�U38��d�o�f��̂r��{��l��k�nG�����˳�Qc�8q��FK�i�u���#��/@}����P������KUM���W8���w�OL���L�2z�L?6�28ǩ����8�������X�D��Ƴ Gg< fչa�PMu<i��<<_�i���UM�P�fvW��U�z�re:�s�	g�q��}�jeܦ
0�H�}��7i������w�
�
}=�rI��J^�Ȭ�s��3v�A6���̤���4Y:�d�f��3iN
M�b���)8��F{�d\d�;��@���ËGYL����3Nm�L�T���&s�1S�x��0����_8�y����T���v�_��B?и�	kx�%+��e��cS�(�e[����V�Cб���@K
{��ASVZ��꽳^�K���v���3��:X"�Y�>���b����S�Tb�sf����3\��Y=���d]vƱ�Ҝc!�gd V�|"��K��b�OD_�\��,�z��!x1��]v���jG������2�"eXģ	V� լ�Q�vPF�;�WM��"ƣ�eyY���n/G�B�Q��������4'ʛQ< IЈ�Y�8_�n'E)��3�r���h���qx6�-hFa:G}�����-�9,:W�{�5S�Zny�+I�'�j=|���|�aC�Ha�a�y�8�x� @����*��0i�Y��?p�XQ�=爾Ci F�0%� h�P�!�����{T��1�8�y������¶߄���d��H2�L�KH][����l-.;�����3|���w���H�[4��zRWwCh���Q������Ec҂��asP+��/�Ȥqĵ��S��7 ͐������J�_�1q���;�ܐ�G[x+�G�!f+2���@�L���aV���K�����Y&x�����O9��R�8�KZ0�b��Y	{�ȶ�|��̡�C�?8�,&����G¢��3�����b�@ߛ���
>�M��Q��+
�:5����h�(����c�"��c�}5R��16}�N�*��V,������੄Y�x���&*�=�k�N8����s|�fj��zy��dt?�@# * �eE��:����Y�\|ޗ�	�y��Vn�+���� ���)�(I9�;ʲ��BA��Ѣ�>�iӄt�	a��"��S�2e:��e:6�M�-R���U��t�������e����U/����s9bJ~����v�<A�W��$�����`tOr��sn�^���֡�^F��1b�d>F���L��\qkJ�U2������'���ͣ3��L��Q;�L>�����hj��L~��f��t�Eٷ&]Z��_��?p�E�}9G�m�4��L3/Fx�D�9Ee�%��:2<�l͕˴^Yڻ�J
�`�5��%�JӪE�쀅ja)6���8(��F�������E�m��-zM/�`�Fe��ۃR���֪5��\�hv���vlIE^Ʌ�q�
�a��Lj=��Ra^�Avݾ�֕Y`H��L��@���m.#��UH���
�=���^�3u�����p�A��&J,SA���>E�m�y��1�z��GPtb��a�q�OW{z,�T �E	�h��q-��1�"*�p�����(�8LR`����t�������yYxo�"?c��Ec��_���ÐmG��ޥ��,�	���t�V���>�����,=��gn���{�`�ހuc�X_�\�r�k:VOswqN��ܙ2��n��׼��;w:��ū���I�Ts|H�#d��7�H|��e�Ɨb�)�-~�29f��Q�
��_��ʐ&G��-���A�Kr�=��#(��B���@o�F|�ۦ���i���[���նm=�N��Uc�Ú�I�I��#�G��S�1Qv;=�0e�W��,��I�f�����9<��2.��@Y5�c_S(��9�Ux�\"_{sJ_np�����ň�3�[��O\���Է�("J儏���a�zP�.�J8��D�]�j�����:#dܱ�P9�{����b*?���
�Z������q7�J�~m���ۺ����jT]����{��ItQ�{,GQ�G�F�~z�E�y�h1�t�Oy���۝`��,#���y���#����vf�[u��W_��H'`&�2���m�8�Y�����7�� �����ţ����+�\`	Zj�\�]��Ӹޗ�_3ĆK�Ix�Miv�ӽ%G�W:��C5�XK5"�st�2r�\�S>4�P�����i�Y���>H�X�PY�̘�,Kol�Y<Ã¦�!T:x:'M�fJ �e��{T��З��칣�_J���땯�=}eEկ5�2|��^��o�_�mϼ�"������?/�N5CVf��]�1|4�m����S֮=����B,p��ݟ�	4�fd���b]"V���#����a�Z9�������>v�\*��X�L#�ˬ R(�X �P���i���Ȩ��.��$:Z�z� �����KGN����k��V��$�p��=�S�s��N�^}�zG3���Οb�̸f�m	RML�(�ˮn���8��Z?��je%�[� ,�d�f���cQ�w,���{��0E�1KQ�^�KCv��u����;�5(3+�+�u\��"r��3��B?=T�e�ޢR)�"��x�Ap����2��G<��n�31ˇD�NJ:�O�`TQ���+��}_oi�V�:siY�Z��M|6��
��9v�Z�ع��������P�>����⸤�7l|�I��Y�n��yxU�L����W������Ypv=I���l�����`	�����]azHfL��o�9�+>�gcL�	{���;d�Ȕ�g'y��<l�,��S��ӻ��[QK^ҿ��8��p�C'��wc�2`��2/2���2�������X��H]D�游4B+P��F�Q��O[�O3�:�6�i�=� �tW��j:�KoT�@�
�Y�ǉ���	N$�P��'�o��ܗA�������P@����}��=�/��S��eHR�cO9�hp������*7g�>+���W@��y�hd�{I����n��?<&i�c���m��T�L����/�V�@�0g.�b�-��CFS��A��7yB����n�19����+�����ŋXLʭw��p�7����j���ӕ{?����:j@Y}���.��F)�"�@�/�J�s�+���2��ʈ�N;�i� E.kv^�%��2�w4ô.\�kC\=��W;�M&Xnۺ1h�*��|�~bd��3RoW4u��\�D�vn96����W���,6v����
�P@r'��p���KԡgX������`]��8������Z;�Th3W�Z����ls�jL�c�t�ş4�"T��mA�>���/��Iܱ�P��Db �8��08�ێ��gWuE���*}F�MsH�O�Y�N���|�12�l?[Oy�I�92�M�d��p��Py�6�/����
��n��0W�dN��f��2Cc�2���pٙ�>�2�R���m�\�r�RFE�')y9��	G?/�<U�奺>�p�	�sG>`E�,�N�m����u�qZWOr���_$8���쇟�d
�}	��u�G)=�6�1�{^�O`_TS$��3L\>zU��(�< ���(<�ML��O��g~Є�"�s��>�k��Z1��F�� ߓ��a����YKe{�Uۭ�C��y��uϕC�2�p�
�UoPM�E�r�ƻ������1����o�n�iOZY>�9�+ +�O2c�"����sF�S1���z(���e�u]�EIᜏ�Qդ�d������ܹ��ݩ��g,�� ûBi�"��}e�Т)Њ��&>���mkп"�(����p̍Txm��W���l��q�aƤ�	���vT`3�Qܙ��N�Ю�苀�(��1rC�a?���>�BoZ5�ZF��'s�>?�j�!mؚ��vr����1�nw�������C��Jnųn%�q=(��
�}�5�)��j�������=��ȓ�5�i1)v�sb��*��$����6S�����d>�Ǎ��� �q�6�y�\e��l.cŬ|α�M�s��)���?�N���pd.���&�wtxC�u�]�4*9�����k̞:�`A�7ݻ9c��k&�2�lD����N�G����	'�	���
��p%Q?Gy�
)�,Z�'(R	��XK*�a�^L*����Q�4��-l�-�!�4�8��K^80)�Y|�^>^�,�v�}��%?9G��-��t�.�vn0�N����Yx�-K�j�y �,��'��	�o���21���P��T��p'�a+��^ݺ"g�i�s	hX�AG7[�v����c<R_�\,��vU����6gX���	�2?=�� �]~��ͧ�.�;�TXa�gDh:���uf�}�zGؖ@�o�c���>����)�g����v?��g�Nэ,���ĸE��dhC�O� ��Pue�I�0q��v=��Qΐ���«�X6�
/��~�:������o�[���A�	���g��pS��Ӿ�T�J��
�W�UP��l@rHN���S��&A� ���]2�:�c����������r���)�����z�C�����u	oaV ���V�0�$by�t�zB�/`���+,%-���� wum���� yE������~?��Of��+�4�1�|�� n�J
endstream
endobj
27 0 obj
<</Type/Page/Parent 2 0 R/Resources<</Font<</F1 5 0 R/F4 16 0 R/F5 18 0 R/F2 9 0 R>>/ExtGState<</GS7 7 0 R/GS8 8 0 R>>/ProcSet[/PDF/Text/ImageB/ImageC/ImageI] >>/MediaBox[ 0 0 595.32 841.92] /Contents 28 0 R/Group<</Type/Group/S/Transparency/CS/DeviceRGB>>/Tabs/S/StructParents 2>>
endobj
28 0 obj
<</Filter/FlateDecode/Length 2244>>
stream
x��[�n�F}7�ؾ�E���x	� q��.� M�!�-�$6���tR�}g��G\;�vm��Iњ��s93;>}�tլ�t���ӧ]WNv�>�^���^ܬ��r^-ˮZ-O�]_vx�7[Nm��	;�����(�	~�y&X�La��,ׂ�5�����ly|tvq|t�B0!x������N�`���,+R����
�{�.c�>���*�^}����4��Ot���b�=�������"�*��� �U|��%�o��� BG���DM���%޸�'�u�c-�]2X��WP-�/A*�S�Z�ʿ<xTe.ylOy~�mL+�<�>��篞1v���ճ�_Y��?�;@���]��Q��,c}�`ES�����y���c`��x*]`Z��$�>��N��u�.&�G�n�]�3��w�e� ����2���][>�?ՖMˮ���X����ޟt�M,dd���Hs.S�_��R(�i��5�dv�Bpx1|��:���$���,�kZ���.Id��%>OqRFQv�����T��p-��s6E�O�a�ی�	R��8Q�����Ds!\��P��-��YK���%��A�7 �0�*&�,?����u��HO���D��c�x;>����b����^��2p��o�s��\:`�X�j�{<��e�_1���V��0Ѱ��Ր�gM:� �0P�1�%V.Y�4eOμ���J9 ��?V%�.q�wC%�k��:��/a#ҨlnЅZ[6����� �dx�a���č�ɱ:�O�z�Ix�`J�Fq�r�/d��l�0�d��e/PyJ����M���)��6v�?>H�M���Ӵ�����r?~(A,/ұ�Ns��2�S���s�����!��?�Ts-���fՂ���\�b�o�,��"�Ű~X=;�Y蜫0�#�t�\�;,���,��{�| Q�3�?���R{/�S{$���,3�]��^�A�h8xuIi���!�y�;����*!
`p�Ҡ�ɴ�zFH��b}�� ���|Fujb�S�PFEv��쮰��p�ɢ�
f���@�n��DL�l���S4� L9�E�X��mW A�=bl�+J�ĚX�gZK�E9�J�@!��f-v��	vH6�=�����줼F�i���ZKl<��j}G�ܥoݗژe[�#�
E�S�y�K,�6j�����
�zsv`�[7夫�&�;X�,����񟴁y����[���z�<�v�,z����)J������Р'� �Ug؇�(�I�b����/\➮!Q�o��]b����i�g+(���:p`�	I�6e�o�؍�TK���v����m����S��n��?T�K�"��L�3�������l��$5��O@�w	� z�=������bT�I)������ZI�JI���I+ɵ#8���F+l 9#o�s�ޑ\z���~J6���PW����[L��I�n)2#���(� r�m!z��y��y	J�� ����y�u����a��r+6�C�Yؼg��B�%��O�� ρ. #���8�q�I^<��UFy�l�e,_�8v�4�.鲜ۖMW���@U���+���S��Sl�:��j-�5t�r�t�z��ב�:&7P!���!��mf4�19l�g<_C�P.�8��c�sn���ˮoH9S���������2.\HlYW-�P;�-�S�(#���ᔐm�b��(����c��e����N(�=�����!�5'LK\��O<�N]��c?���8�ꎑE��;�"��-�V�زNѧ[z`�:��ie�'8��B��צo��Mk��id���$���tW���0�з[�D}�Z�o}B��j�>8P���,������tȻϼs��-���vf��C����ם9������Ps:0p�auM�a[�)���囡N���	�l 6�%O��`��<.���C�P�����CjG{���1آ�#���ux���{t�U�S�8�\-���0�@��7�������Q��(�tD�ڝ��9�t� �/Z���)�/Ү���`D"��8@ '�F�����KnjD̶�ċz��-��%�`"W`Ǻp��������c��ӷaRۅH��mȯ��l�f��b4ʸ�rv�-�������ySh<�r����a�gտth*�޲�*�pq�'N-����M#�)XSA���݉�IC����~��xB��!=@����7��1�y!
endstream
endobj
29 0 obj
<</Author(Stephen Comino) /Creator(�� M i c r o s o f t �   W o r d   2 0 1 6) /CreationDate(D:20191121205645+11'00') /ModDate(D:20191121205645+11'00') /Producer(�� M i c r o s o f t �   W o r d   2 0 1 6) >>
endobj
37 0 obj
<</Type/ObjStm/N 76/First 595/Filter/FlateDecode/Length 1103>>
stream
x��X�jI}��f��!��$881�2�!��Ğ�Ed��e��~O�ڶ�;�2��j�rNUu��n��ԒM��L�Zr-���L�2>��d�DΑ�����0�\$φH�#�)p �R{����d<yG)�)L	��D�n�M��`J�($b#��gXĸ���é������C΀��?��Զ���pf�'˼���%�(�d8��m8!X&K�$�`�'�l���q Kg1|�R&l������e�����K�/�/�|Y�&`��L����L�EB�$0n��a)jh��T�Z#sP/+���CV���9q>oer�C��`��( <�
3
�$�J��A$(�
��;���]���i�͛�THZ:kf�7��}3[��/��Ms���?�9�"+�޾}��%�t�{!Gfc����!n:�O���8��CrE-k�_# �P WH�+4�"�
p��B\!3*|������a7*|�����2<����[��8�9Ƨ�9*����v�:U���=�*��w��7���7Q��1�N)�.�C����O�|ޙC��]<�ʡ(���8޿����ߑ�G��w{LV�1����:�쪎.~Y��~�}[�.���*�
L���
L���
�����O����6�ib�5p��F\#�Q�H�Qq�������i [����
L���
L�����*-���i��pq�/ף��Un:6f�%�U�ư�F7�G�8�[�ӣ�S�,^Y��xe����+�W�,z�)���3lLi)KR��,IY��$eIʒ4��,YY��de�ʒ�%+KV��p�TP�Śbm��X_l(6��-{~)�VA�W}6��lX�_�[�^��v+�NF�Az�jOǣ�ѓ���� [�?�k9���D~>,/���}�����us�w��J����i��/��u'Jǻ%��|X��j=���æ�uX��>?��&=w�}�� �͗�b5l����V���[W[����ߚ�~0�j��4�W����zrs�ϨܼlVY�^������z�t7��7m�/��Ş���M(�D$f=��j9B�G��/�(�_�bU,
endstream
endobj
59 0 obj
<</O/List/ListNumbering/Decimal>>
endobj
66 0 obj
<</O/List/ListNumbering/LowerAlpha>>
endobj
72 0 obj
<</O/List/ListNumbering/Decimal>>
endobj
110 0 obj
<</Filter/FlateDecode/Length 235>>
stream
x�]��j�0����l/��tcB���b?,�8���(�E�~�S:�����'�$��s�l�A^wa���~!���h�(0Vǫʿ�T��n�#N���k��ܜ#��;��^�w2H֍��>w��%�N�"�i����^UxS��ء5ܷq=0���ZB�u������FRnDQ\�/\�@g�����������'vWE��$�p��cf��4%-{��"N�/�c�@���h��D����q�
endstream
endobj
111 0 obj
<</Filter/FlateDecode/Length 54360/Length1 137396>>
stream
x��||T���;�l�M�dS	Y�ݰ$6Z �B
���	-!�:A�F�b�.V�^A�,(���]���۽��^�zQ����@(*~�����������w�y�̜3sfB����0QE����uQ�.#��D	O]в:�#N!
[8nbN����:D$6�UEռʅs/\|�w<Q��U˖:�,|;�hK��.�9o͇z?���Y\3�ή��^�&��}�2�����kַG>�S�YpX�N�r!ʝf�[�|�/�ף�ѤںU�o�o��X�����U._������W����[:�#qx�����y5����%qw'���,Y�j�� �pq����q��\�K�sڻ��.�3�jo&i~���o\�X���#�_�qM
'���.�ZH�������¿22���0�i�M7������Vʡ����j��M.��B��5�7R�1�/��̤ńh�f�5��ں�:�4z 3�� 7��&�C�Z��D���w�DˑR�)�Xo�K��7������i���*O'V��v�V�ݴ#$�&�V�f:�\�B��u��:o�h��#�L��1y�ω�;U�3��#�8�z��1,����׿���G�i����i�)�j�\���p|�W�1�n2�Ku'�[~����s�>Vimp ��F;p궡���%��3�E���we�����D����a�8e�2�p�57Ӎ�}�#�:��������~���oДӽ�2SڪϠ�SՅ-��� ����8�z?��ӹ���2B����a�}m@���i����T>�k�|Yg:t�֙2�}�{R���mUZ�I��?�1�Mi�I���	Y�o9.��S����m�wR_�N}�~5>�K�K{���z:���M�=���{(����Q���x�)����xJ����ߏ�1����L��:�4�|����~#u<�<ҴT�}Ju�X��k�4L<F������7�UT)浾�r��Fu�I���@��mď�4T$�l��'��ekk������?������?�AZЂƦ]+"~����-�;VW�߉�B����'LoG��G�M[�w������Iq�����>ϣ��������wZ}˥�;���L{���W4(1y���֩�.�N|��	8��S��L��sh��������:�w`�#65�` 0�w���������'~��G���i��eh�Sѩg&�U ���?POm�F�I	Z1�:e�ӌZЂ���?��>Ne��A��.�C���v']�]�r!���t�qל��z��T���uڵt��@�ڙd2�)�T�Ǿ�-hAZЂ��-hAZ���M~�4��c�%��Lɿ�=ӈ�|7��M�f�{f�;fЂ��-hAZЂ��-hAڟg�W��{Ђ��-hAZЂ��-hAZЂ�癶��@&�t��F-M��8�'<�Z����KЂ��-hAZЂ��������W|���.��3p���B(�B�u�\@4�F�xԞISh:�M��n�I{DO�_jvj�Ԟ��SsS�Ju�u�;��xG{G�c�c�㢌~6���Y���M=�u!��@����FV�IY�#��6Y�:V;�qlFVBV���?	xHO��h�̸R����P�Ҟԋ������ˍ'�9�up��ʃCn :�������m��<�`(�?"/M������� ϐ���ԯ�=�b�K&��8܃LʢB��(�k9�WM��Hi��/��T1[ԉ�^,����q��"�����1�xڤ�L�p��b���(T������s�������qK��ǜ�}��<��_�������9�<<N
��('�Wc&5x	-=�c����ó�;���Mo#����߰,���'�������%w���K�,^�p��yus�̞5���z��iS�L./�zJ'N(?n�ѣF�>����`�w��3�?�_�ܜ�ݲ;gftrv�''�Zc,������	�.rW8|�>S�s��n�쬄�����瀫�����s�Fd�	�n�t�V� �-�Q�t�^,t:�Ey�zs����;d�1�6e
��h�(J�U���
G��x٬Ƣ�B�k��(p�Dt˦��H�H(_g��&�y�0�ֹh@�Ff���O�(����/������|��0#�c��3]�h���xa��fT����ՕS�>�����ƍ�X������e�'�r�/�YX�s9�lԄ����������C_�xB3�?��r�G�	�J��b|��/4�i
��/�4��'w��̧UȚ��&�#kT����ty��*?�f%�f8�ec��������3+fT͒\Y��,,�y+��܅���X��z� ����-�����q.�%8�r yfO�M�|	>��
����~9�+
��2��Ļ�z�h����M}�L�×T���Y�譮��+l�x>k^[��]��+szk��]rZ}]�r���V�	�*X�<,���j6�L�-8��p�
+n�Q�wt� �W�H��*���˃��Q0\V�i�p[zY:�ot��SH���&���}���j�8Zv������M�K�` ۩��ɹ\-��vWUzV.|�.y�>��:k�eN<C��^969���5�9���k���SRz\����tT��V�g��eS��(3�G��O�����_���M�g�G��$RpA�o�����r��~v�n2STziE�j1�;gq�/�����ֆ�Mnw�¢�Y�.�#����lF�'xW�V�k��(1�t(Ri4��)6�4�Ŧ���=V"ǦR�_ZA�в�N���q�`x5�NYpȂ�4�o��&j0jM��(W52|f�Tլ����4.������5nm��̾���6��*k �HȨdk"9������,�T���<��pA;��Eؚ�s��nM�n�#ӄ@d"���=�am�z<pϱxʽ;���OD���0y�!�O����[U6���L���g?�'��ɧ9�ǡQ�g�P_�s���K>�C�?O�H��r�m�pb#Ɗ�M�Z�eJGskk�7�Eۡ�t��)@�����-$c$�IT�=��PU)�A�l�1���R%D�_82�2 ��h#�U�Y�tnle�2���wv��^�>����!��B9e�q�^�惵��QR8�F�챡����$�E��UNTUU8�����/�{j��2kD��$��gDZ"|�ݑ?RGv�{NHFXYw�(m��V_$z��f*0;�!�����}L�)i�	���:e��La��Y2FT����#�q��f�	Fr�co�y�[Bs�γ���������`�RY��dW�l�^��nl4[N݀��l9ʆS˨�o�|����Q$_�ΑM�X����ƑN�A�	tt,�tGu��B��{ٯ�6A�5m$o�T%(��l��<�8�h�X����|��P�^�ge��W�'S��;�htX����h<L�7���㏧N.��*�wv$,�h,n�GԪ������:.%օ�ÃDr8���2G���ě�n�j;jqNuV�W�x��r�R�(q�I���Ë���ƙ�7�O�@<������![c���g��b#}&��I�Y�rV��#t�<A�m��]cvd6[�k�nc.1q��fȏ�Fy@�Z��L�6�5:�7b����)�jR^U��0nu�%L�Y*C"ϐ��do湚��e�?\l6��g���*�XOR,r��vy���ʽj��e�L�O�M�v��Ro���GȦ6uø<�;$����m�{h�s��~��!�g��(�������wɣ�~�v���~��U�+�G���?D2i�Q�Џ�j�V�u ��"��H���=N�@5��A�#��9��v�'�����8W�s�hPb�k�X��*%V*�B���X��YJ,S�^��J,Qb��X��|%�)Q��\%�(1[�YJ�T�V�%���Rb��JT(1]�iJLUb���(W�L	�g*1I	��JLTb�%J�Wb�c���h%F)1R�JWb��J)Q�D�C���[�|%+q����� %�+��D?%�*��D%z+�K��J�P"G��JtS"[	�]��Dg%���T"C�NJ8��D�%�J�)��D%lJ�(�^�d%�)��D�	J�+�D�V%b��V¢D��JD(��Y�0%B�Q¤�����P�B�*Ѣ�%~Q�g%+�o%~R�_J���J|��?����)��(����J�/���S�%>W�3%>U�%*�W%>V�)�(��)��(�o)�o(��)��(�/)�_��xA��xN�g�xF���xJ�'�ا�J<��cJ�U�Q%Q�a%R�A%Pb��J�V�~%�Sb�;��+Ѥ�O�{��G���ء�v%�R�/Jܩ�Jܮ�mJܪ�-Jܬ�MJlS�F%nP�z%�S�Z%�Qb�W+q�W*q��+q��*q�[��X���جąJ\�D��+�I��JlPb���#ԱG�c�P���=B{�:�u���#ԱG�c�P���=B{�:�u���#ԱG,VB��:�u���#��G��P���?B��:�u���#��G��P���?B��:�u���#��G��P���?B��:�u���#��G��P���?B{�:�u��#�iG�ӎP��N;B�v�:�u��#
vJѬ��OlǙٟ�:�K�������i�?-
��K��V2�`:۟:�ܟZ :�iS=�-�����\�O
Zȴ�i>��c�c���P��4�i�L�Z�BP�����f0U2U0Mg����ri
�d�r�2&/әL��<L�L�&0�0�g�4�i�h�QL#���L�����aL�~�(P��6T�T�4��p;7S>��t� ��4���g�c��ԗ)���a��Yz1�d���r��s�nL�L.��L]�:3eq�L��ى��ԑS�39���)�)����)ş2Ԟ)ٟ2Ԏ)���L	�g�c��:+S;��,LQ\���uf�0�P�����Ig��%�D�V�#D��/L?3�s�'�1���?���?y"�\��wL�r�7\����W\�%����7�/�>g��C>��'\:ȥ�2}�t��>b���0���ӻ���fz���LЛ�v�@o0���ט^ez��ey�i?;_dz��y��8�Y�g��4�SLO2�cz�#��cL{��G�f�CL2=�����#ws�~���v1��'����ɠ&&ӽL�0�ʹ�i;�]�$���/��N�;��v�ۘne���f����1���n�,�3]�u�2]ô��jnp��d���r����\�t	�ma���"��Lr�\jd:�i�F���J�z��yL�����s���'z@�Dl�b�?�/h�jn��ۭdZ�O���͗3�Ŵ���i)�N���/bZ�O�-�d�9rS�\�9L���,��ܳZn^�T͑UL3�*�*��3M�AO�Ma�̃.��e|!/ә��I|!g)e��4��ğ���'�+��'��{�?ah�?�h4��b�O��@���p�a�,�'��6�
�	kA���P\1h��)�i�?�wq��c�@��c�џ)�;�����ǖ�r��Sol6�G���ʁ���ʵ��ԝ�w�+d3�8YW�.��3SS&S�?V�R'&'���9�9���ؙҸ]*S&S
S{�u*(�o�j�N%1%2%0�3�q�Xn`egS4��)�##92���Lf�0�P��H;u&�I0��5f�]�%��~$������a����߿������'��}���7���!���D��Q���9�Y�L��ѳ� ���w ��!������޶̵�e�i������%���*�+��e�%`?�_���<����A?��e��i�l�S�Y�'-3����	�{xp�����#��Q��E-�?���@�R������}�ۅ�����&��y������#W�wD��o�\c��p'pp;p[d7���[����&�ȹ��o����Z���"���]\	\\\\�v� ߖ����#��/��i�q��;����yz�}�ȳ��i𜳽��ֳڳf�jO�j�ڶz�ꕫ��~o�;.4b�g�g����=gy�o?�󀶁j���A�e��=���������b{�(�=�F��zG��Գسd�b-��a�o�i�o��-ͭ{w.���ݫ[�ŋ<<�/�̯�癃�Λ陵}��6��S���S�7�S�Wᙞ7�3m�Tϔ�r���垲<��L�O�+�x��z&�x&l/������Q���GyF����>�3,��S��SkG�*;0�zB61���m;`��f"�϶צ�Ť�S�.1�E���bA���/n��$������dǴ{��G��ig�w��ҽ���I�$=Q�-iLi�����=s��ړ���1�"&ў�}�(6�.B���t3bv�D{�����/���B��Q�f�0�g?�'6�2&�OwI�/t��<哽MB\Tf��_��K%Fy��͔:t�/u�ׯoۖ:�l��Aj��ЭRB�\Ӗ�/qy�gP��oc��G�/Y�����c���h{�&?Z�uwt�~�1�E��=�m�G�/+j|iqL�=R��G���ܑ����n=�O�N9N��k�4|L[��e��T&�e�%��g�R��z�L��4M_[��K����&�����M�!��yT�������`5�
X	� ��gˀz`)�X, �y@0��f3�Z����@%PL�S�)�d�(����$���	@	0�� ��Q�H`0E@!P � n ���@���}��@/�'���݀l�t� ��, � :N�#�8 ;�� ����v@�$ �@X� � Q@$�f B ӐV|���Z�'Z�#�/���a���O������� ����__�||||
|�
| >>> �������� ^^^^^^�// ���� OOO��'�ǁǀ����#���C���������v~�	��� w;���]�_�;�;�ہۀ[�[�����m������u���5�V�j�*�J�
�r�2�R�`p1p��� h�6��z�� ��ֿ��X��_`����/��ֿ��X��_`����/��ֿX`�� �=@`�� �=@`�� �=@`�� �=@`�� !��=� �=@`�� �=@`�� �=@`�ֿ��X�k_`��}��/��־��X�k_`����������7Z����LZ�t��W�n j���2���j����.�G�=�A렶�6���B>z����N�߼����2����J�D��[��4�D��\�R��q��jm����-��Z[�C�(�hk�^����H�a�rQn�+��F���wa7���r�	sPB�4���T��J���f�l��\��y4�(�G�L|֢$���C�Z@�Ŵ��i�,�^(ɺEF����t6�����V>�2<�P��(/��Zܙs�\C)f�::���m�Mt�o��?�����"��W���J[������t]IW㹸��;�{�ῆn���Ⱥ+��P��!z��{�^�ߘ�*�ψ��ZcbVa������ﬣ��c�ck�t9��i�,0�2r"9��e�	3�c`}lD\���1o�Y�-������̵FI�������o§�U�n�fu����o8��(�B��m�wJ1{n�����ھ����9��*�{�n��������v�N�O�����Vݩ�;~�Q�z�����N�8�(���=��3|\~��@YFq�)z;�s�<�@/ѓ(�7>�A�ez�^�����>���!�P4���0��Ѵ�����BR(�����zV�O�p��8@��]�E���c��N��R�j�Q��|�ݐY-7�~C!�5��b��)���KW�ֻ����$ �/����-��@4r�c&!
�1&Ͳ;%%߹;7t�;�Ytە����#ٟs��Cq�s��>��c�w�c�������{���R,���4׹�.W�\�������|���I��])�]�s\�]H��ѳLĦ�H����B��k�Y�}{��5X������>}��{�J����ɲ�_��\w$T[�̟�;$-%&��uH��6(�:qrƠ�azX�b��oh�QuE��MMLJ�3��R�ScÎ�}�!�?��~�\8%��~u�Y3��6�%��:0}Ĥ�x�)2��d����\8�Ȇ�2G��D�ud���zش&$�:R&�/�}uj�bW�U�v6Dfs뷻"!"���p�H�a���3��tw�:;R������>*2*�c�3�"�LQe���u>�|ɩ;��Qq��<!��Ϗ��?'g���v�c!c{[���ݳ�pM��].�;)�2��k��m�d��h���e$%�w,KOףug��̾�ߦvaN=�To��=#>ܴ��gs�xg�Ԍa~��}V��kJ�i��H<~F�-ڤ�E���-φ[�M!Ѷ$�?2ڬ����GV�i�Adx���Ey�/9��{�U��[c��Q�p`���ywwNIt�>э����l�-��ep�Ζ���K4���2{�>�D$�۝1����(���)Y��-�"�Fj�)Y������o�%}�EdSX)��7VL�3�cc�{��b!W��?k��"Rzf}_�V�cW��$Lf��!N�Ѡ�\3	�&gz����>}{�c���I�E���+WN�1i��qU�F��ӮK�v"s��U��\C��N)��r$%�|�_�����f�[���@oA�Xr��	��&ڳL�fٳKW��^:,/."w�|M����2�9pܑx�[�:�� �te뷦��4�7�^��tf��E�Wr�_�Ytf�����l�ȡt������EWʥ�{S�$l>��9<]�7�aƚғ�E�κ���f���.~b��Yt�Y��C�O�:����sI��5!:�����I������ْ��)J1'��������L��ysʋm��d�4G��hܤ���r��L���OLXD��ۚ��%�Vz�w���˽S]m��)q	�ór��6<�j��k�d�d�Ʀ��³��³�M���jJ�
�VV`���\V���
�VV���w�;(�R���mi�{3�˙"33���_P��,PS��'p�Ř�c[O���L#Ad�=IGk�l���R*��B�=|ӧML�v���L�=A�L���S�՚-�|��
���=VfK�iX�-��Ϛ9Ζg�5��	�v��K����4�ڌ�j=��b��h�1_a����W|`�����x��}�TJKÈv�Ƿm�wv,i/����,g_l�6�/C�ClG��Έ�Fx��uҘ�kI͊^���Ƅa��v�)�̘�bû/�;<�jK��ņ�/־<��Z�֮^��굺z��jI-ukk���-�,K�%۱,ے����Nl��&�$!q	��xؖmC���I��1�>C�	tH�%ıݞ{oU�Z��<���K��[u����s�v�O���i�B}��}��7�}�k�^F|�r:�<d1�Y�W�p̕���bOGpo$�!#z�Jz�Jz9�r�e+����x-^bJ=����m�q5�j<z,5jv4>�␓�&3�ى�3EJ����1N�p�l�z�Ή��8��`u&ƕZtQ4�7pf0Z��9�{�KQ�(%˰���X��۟h������u��G�@���jT��m��y�/_��V���C3=��Zor����򻞚=�̝�.~�?�T�ƼI��<;�ooy���oQ�/#_��=q��6d����dc��1���CL�0�$�a��Ϝ���j�J�x<VH ��h' Y!�$8I�d�W+�>;D����}f� f��*�����T�@��`���b�ж���VW��qu���
��
��
��
��
J�`˪!��mj�65D�����@�L�����ަ��5U
OٰgUA���nH��2P���j�'�P�L y�1��A�&�C��X~H�I�D��1��c��LJ"�$5���6k�|��{ML�c��:ȫ�4~X#x��z��]��G5CR��@|����� +�9.��O�����e�|  ��bO�\��Ͳ�ѻ^~ע�7��Ͳ����nuUU�=���80a`�8$1`��u�>B�!7A�G6�f����$�%'���J��#��r��I[2^@$u��"h�H `�o�v8	�P�<<�1*+�W����&W}���ӛ<v�ר�3ɬq%"��S�l�<���E��v�_m�z.��vj�D|�;C�S@�d�2��חs�k�3��eI������f34�Z��̘L��X5vЍn��n�n��2	�e�O!�������y<v�F�Wg'�z��ގ(M?J�#���Li%\V�Q���*�z������/���?�#���\ω�u_ܹ����/ܸ���e��_�xtr��w��M���߿uÏ�^���[n|������~�̉σXwbQ�(+�T�Trx+��V�SU@�84��e`�x���.gŸ�n������%�R�X��P��O��p<~ND',͘��i�b�*�|z������Cz+pK�����񉊯~i��� ����nh�W@�����y|��Zݥ�e}S�](�K=֍�!�m���`np�h�p�- -���N��s�``�����dr�9��Ye ����4�N�Z��O��m2�!����?v�*O=!�}�̓�|*ˢ�/�]��"�����&e�o3Y�xm8d�Fa��YC��T��l��`WP�j:����G��R���2�n�2�{��=���vOuz �)A�ʩ�o\�u��@�Ф�ql��c�P�YkY^��c�E~*���(�Y_�
��}�s���ex�Tw��WJ�ٜ��;d����1OT�c��ɌgiԻ�`�u��\L?���S�u�'����;P�}�]~7K�'���ت�xk b&��p��!�a�,�I��V��Y[���p��Q+m9���rTͱ؄!g���^,��a����5�r'|JD�FᰧD4..R8;&�@T�4�H��劂���������f�!�.�bW�DW01�톭#��m�����A*4�&�=�T��VH�^���I|�u��R���C������Ɇ��5ɶ����o���=&Ǜ���q\���P��D�utTDz�?�q��nC��OC5�A��A��
	�0�F(�]2�$�Dz��٘���1�ˈ4���0�A^_�~���7�ؒ��NT� ~�+E�oRrN�I��y��l�/��4xRIY�{X��@a��+�x���e��H4|a�H�.;o��!�O%%hL�sF\�61�OLL�&b�S��xt�$|(����K�N�jU0�)Jg���>�c2�F���N�QE�7��6R	��ԒJ7���Z�꿀r���T;��/�ǭ@O�̽�i�p����n��M�G	7��xȀ��p?7�~��������a�|$P�!K[T@���������H�[�+�,l-��b��wPg��9uT�-�=R'D��
���[�sa�'hb<��fRe��>��ģ�"���0�tX�A-����y1^ئ�c��t�g�&�ЦR���լ۴N��|d q��0�O�Z�o��a�<��U���ׄ��D3�uᐫ<�.�]3ƭ�V���%n��g��0�����upyW\8G���am��M�1��tjӑ�aHC�����x>=B�MA�+�#��6��#n{�7j��'���[Z�p�
V�����0qZ�g5:��I��my5�������S^�S�i�p�4Ab�CX��V�<�
�"��aB���o3[J;fgρ�s�s	��\�j��^�L�m��c�Uc�םɞUS��n�x��-c�`} 6����R�����2K��a�����u']x�v�'�7�|�7|.�TG���R�,�Z�?h*��vT��b�U|��X����$�:l;��1��mi5k�w���7����x�[S]C�T��uG�m��5�C/@B����.��š�"���xW�a���Dp^עZ�Z�.��P�l�A<vko�m���x��e�	�O�{o����]�:�Z�U(i�.��s��>�¡�vo���>��)=AwÖ�'6=<�t�
�	�)D�E�#��ڤ'&"�a�@��*^7�8Oo)Hl�A��ϋ�i�q^��,�ԁߖ
ǋ�����. 7rw��;��_�����o>���������z�|���{�4]|�f��7��t��*�Յ�1��k�0���M{W;yW��K<���CC�4����m����V�(T���Ct�t�Q\z΁02��3��F;�`��� �S��ڌ4��0�ze�g�A0qv���`G3���&�Y [�fFspfF�F����<�٨�i���-�´�}^�x�":j��W�_��J����o�w*X8	V��_	�'�'p�y���mЛ�S�&� �}�R��������.^~�|�J`il'�8n����C����DcZ]x�N��j5\�ߴ���h�6�C�7R� ������x[D'4�3�Ħ���cb����`޽V*� %`]�`��S-��QQ�����]k�>;\�1��I����V�����=ˆ��@׍��j��àt�\-�?�f��[���]ʸ<�)��1
&M���������,��#��0��"�X�a=[P����!A��H!�5��<|�|�J<re�X�e)u�\��}�M�WJ����ݨ��do��������,J[���
��ߊ8�¿\ؖg��Z��T=``fj���-� ��Ͳ�½���{)��w�����ˁ+��?E���ZRa}���7��a��w�3<�fA����#�%0-�H�H�/��W���>�:�7���Q�A�]fSs	�פī�nX^�ht�`�[���\�9M��x�>�[/�e��0WYimL>E�M�!̘SSi-f-��k8�Wă�)2��8�YJ�A�S�R�F�@A[(��8H��S�vgЦ��\��Y�����IJU���^�RA��
o���W�����?{I�W��q��My�R�U�N�X�/�����΀���H�z�J;C���DjK�W!�f�p)��+�k�>�1�V=A�P��
�u�v�sH~�g �6b��;���TF $.�U~m\]Y�U��8�_7]iՐ��k�AvD�g�0�Z������K/�A�vAeu�~]P���aL^��kd���T���4���#c���#�EO��W�Q
O�v_Թ�\����dY Vy��g�{��{a��R-�cw������&�H3�E��3��:��y�� ���M�frK)efJkJ���
( ��Yz�d�&�Ֆ�"�I��n���ʆ"8N���.8ۃ���)�U@�
��ƻ8E������G����~���J���|p�k#e�V���o�.	x�Zg��7�n��`w2��4f&����+�)��7�q��E����9Q�6�E"�@���[���-���p����|��V��y � ���>�}�<����?9|ˋ�/�>��}�[ҳ_����ΝA�<v=@����VW�����o.�"o�4z�2�n-m�	�f-�wDt�?k���Sڭ"����o�H�.�<�t{��K��w�Y����yY��������J]�`�����&���;<�/�QO_���i�����c�j��T�W�Ơ�����������.�8�C�nf��Ex,�~F�`��Y�?E��e�a_n�ӞV�4��SC��m��O�\��1T��ס���wы�������m�jm؂ݟ���[~
<�����:�}m��â��� ^���u_[:w>({�1��$PR�7�	�\�!��¯=7#Q�wƸW��m�w`�I�T���%`��w���f��M�#��^�Y�J��X�ـIJ*2�1�%�t�,jrL���R@��O]W����8*C>�5�K�@��{ۖ'�&���i�V��CET���
�n逵�A�N��?�N�.���k��o�]v��"�� v�d�N�x"\�չ�<`:@Uڴ*�v��E�6��NzM�&�x]�g�4hU2����йZ1k;/��M���	�m!��H��3�k�����7�;����� �v�T�nl��6xb�k'���׮;�)�ѡ��U�u$IW�}kׯ���#���*��Ac��.��pY���hk<T���ؑ��5X�z�L��\�%�t�ڪ"e����� _m _^�	qF8��)ȱ���r��[�¾���_�v�k������9��g<fe�\�"^��&�]<T��-JTC�a��_EUD0ZZ�<��	���0�P�g3�*n/P�	��Q����,� �KU�F�_�4��l�//�L�@�d*��ۧ4yy�������¶���C�;��cg������l���˾�ݏ��f���	�ͫ�Y@)�Vkl�+-+b#H��7$5�PW�T��K��Wi���������y��IA�^O�̐x� �@e��\>-Ÿ�Y:�|��Т[�F�Sk�!�5\~O��{o�nF�RU\�b-��lb3�nam�6��y�x�a�|�: �|�
��fd��>o���)�,���w�� y���)i�ۃ��=�
�����!O��X���	�����a��gb�� �B*�>��od�w�?�J���P�V1��{�hV��B��&G5Z�W���i��9l�Z���k%�=J�D#&`e�V'�Ʉ ~0���EBc��0[9fM�S���Ǔq� @H�%5i��}�	��ja	_�YX�Wb.[C�J8y��4F=��UM�I���1O�����?���,��[�����YJmj9�Wh�lx-M���f�ߡ�}C-��/�?E�D ���v:��O����M�k�'���0Ӳ�V� ��ܥb=3��8�(�A���/��a�T�����
@� ��L~�o~�e�a�;dQQ�� p�C:Z�?�/r~1$uCA)S��B�QP��n�w���N���wPx����x'�Q�jY���j�#�Z|6P;��#��"=��|J�5��ƚ�M�㒦��yB�6s�g�ZC-��L-��⵵U��8�_�q��r�^���;Ha��Z0��gb����3��)y]X���	GZ��ᵶgE8�h1?n���U��Ū~��%���KV���<�^W@8�Օ�d�,��="OF�lV�n!�N���5?0�w�pe���v�Z�<պqY�dU���\��3+���\�t�g͊��V�e
�]����ܑ���]Q� YMi���.!�2U��e�[e{�w�����_һ�r�;���uԾz��T����^�:�Y�~7����b���g�^����1Z�H��*̢���Q4���p��אM�ͣ� J9��Tq�Ƃ�'�����ĴJ�N�� )JI	Xۖ*�L˕)Jb�B��pV+�[�LN�?[��Q�Ө u!(r�ʲ�L�l���eߵԎ��m�H�����v��=�����M��RWOI7��K�6������Cӭ���D����0�6��/`u؏�Fp�g��j��˫�^��ˇ������/�.-�$4im\���z�jm��ǉ9S?����Q�����j��X����,�Y��UQ�G8%��k�?�p�p嘨���_}�Bg�E�u/A3�������u�zdMl���W)�ViYմ�V_z�%5�c�ʢ���9�=�2����7?�3�ƈ�W�;���;Vǂ���䒬��-�ۃ�E�͖����7�V7c�y���Hd�|?�)��դ��Xn���i���Φ�DS�)���Zb:���V�Ĝ�?��ڡ��~P� r�I���Ą\��VQ�wgϞ�gO��XN���7y�,k�,�F�nM�E`��H;2*��1�H�=)=�1!U�3��Ⱦ�pX����oY�$�-��EZ�}}��=0�WA��13�꾚�=J�1)5}�x�#X�|���S��V��#_A�6�#Ϧ�{�[*L6s��G�E{��Z�~`Y���n��|�
������ի.�]�C�/���g���:N�"w�#�8�|l1؍�=�Dh��G�R���qe�htZ9�' �b�A[�^1�	����""��X�̈́xr��,W�&��_ �Y>���/r��ڣCQ��9�LD����U��F3��'�rY:���R��5�9y-����%����9q}�>�X����>Z���_/pR������(h�j�u?����@�� �`�>�#���;���_����?���U�����@���'�YV��Ӑ�V��-�]���U񏂁�/�PkU$�hT,o�?�q	���,v��ٓ9l�!#�0�e���&ٚ?{�ᵐL�{�G�F����412=2=9�j���$�\Ս�I>�k� V�NTv唽�y �(�j�HI�%ϝ=g8�!<���M����_�nD�Њ��|NW���9�rPٕ��P #��#�n%,٧��`-�%7H�E�E��C4X�U���H��=8�+���Ӣ��
��9#	G�TڽWo�TZf���3Z�YewW�h��jC�E�(}iZ ����9Gw�sv��j��U�N0�y��I�ZE�x�ۯ�j���M����AP�3�-�g���q8�3�K�#�si2�iT������w`a,��iRO{3�q��M�U�)��*5L
J�L!�2u\N�q9A�eu�t� �{5���ҕC���YK�"�!��^\L)/�C�'�.���3�D�_ϝ�ɰ]�DϢ�w����P�9y��|��f�Y��G���g�,��ɡ;Ҍ���ꉮOv��tO��5��d�����;�n�}�ξ�.BSX�z�(�M�݇f���֝ �}(�V����[�o���'MP#���#
&_\�V�+} 	i�4Ο�}#F��ʜ����%� ���K⟂���*�����"���_P8E9�/����7�v���u�ܪZ��$5_�I�}
	9�OU</~���t��]�t�'uN��SRBP�#�?Q||��G�/���1w������8�h$��~lWz��76��z|���6T>A޼�u]�� ��o`�X�E�0:�Qkҳ;oj�?��n���K�M��Teg`6]~�8�a��1���D��!�xGA�;d��_Z]��������ِ:W�'�s�o֐A	0�O�L�-IC&�r�4W�Ή���pN��E�,�~ų�R�*>�Y�%�I2q���X�QG�֫{A�Q�F���ă�t�� ����̎�@g�z�MG�4*>9ܴIb��.$�"����O�E�z��,��	Xjۇ�@����C՘R��	�c�bIm��A�u��Իv�����1o��u�����rݙ�l0c��.-j��
�͙$ҋ�s	�|Jm#N�x��F�nʉ�1�݋�d��+z�gi���Q$)� A�"��Li���1�J�JV���VTq?�R��:�|���И��t$�p^�-ڟY�t�xٿYaQ����:�ݶ��jE�,I��kG����WE)�a�J5��g�r}3f���D�>rd�q��''��L��Y�l{LŁ���?~b�3��;|���3�����9u0w8���LO�]��Q�:覦A���%�h�އ� 3�����6)+F$R�UZp"/��>��籜�t�`N\z�&JM���ri�/h͉��%r~������/q,^��2��1�&��e�Eyaa~\p}���z��UG�� )e)��N"���E,��� {V�=㓀(N�d{yq)
���}�5��Vi�	�-��%� �'>5&�Hu�6���Z]*�9	�u�SW"�J�����U ��j��q���g 7J+i�����AȿU��ϳ�o��T�J�[�7�ʵ���\*�:�,"cR���$�@H%��cGεʜ�mʉ��T<'����^�_G},/�vS�@�{��[_����#!�7�e���^��#~��>�
� ;�ǐ���X���Z���������`_׌�n�*ݲ��7��k��fZ���\.p>,_LJ��K�O΁!��12ɉ`PJ�%���4����J����^n,(�̌�X�#�|T��o`́���>�G�v�� �M��������g��LC��9:�683��CS����ض)�3{�.���;�d����;=�3�?s}v";��	�l([\sJ��z�rt��a��Z�}%q��dn�?'����̉�������辜H�Z�H	W/����O�h��
%��׿���Oi��|ղ�� �ÄHT6|,Z>B�W���@|�V}\>���=�$��	[%��<ڃ�&n�Ԙ���a/D����b83�k��V���t}&����?��!W�ȹ��o�=�s��p�e0'���+r�>]8F�Y��Xb�:�b�@~ �-�Z��L��q�"6��q�#V��}�з��Zt�\�	!gO�v�g�0!�t��*�=t����)eO�J�1���(��^�(���A���+*�A;�ULT�@V͞TW���R��u�(�u���e�4ݵ;7շ.�:���қ�ue�ق��y���p?Gq)Qn: �J���v�D8ڲ�9�וʉ�eBp���������JN��g�[*[��l�g(����Yy
!��T�\Z .�	��1�Î�f�R���Zᢽ"�^����ڂ=�bh\��@�[��-S	�c>��Vm�6ݜ��-�K��2��,�h�}H�ބ�^.��<�"�ZW�A�:�Ӂk3�'s"!Ӗ�v8�1�"�k��R0ε�*�0���ǎ:{��!s4���$A)�
�=�qU��'
߻�n�h8I�1F��Q�M�˖�-*&��1y�,��C���;	�o��j�j������޷�c}Gb������ܕ;�;9�=�GO���dWg��O5Ue���a�ʮ{��L��J�)��]9��g;�Gi�F80ȕph{yie#�&�ի;������.E�(�|e�^����+�v�G-�فQ�A(X�$(�����\�n���n9A.E�/����L(z�E�( R��:�%�E�yV��\ww�-�~[�Y�0��Y�07�*ͺU*=�J�ҫ�~����R�n��v��O�m�6��N�Cp�vw�I �@^$�ɀ;&&���~�����80!C d�7�ȧ�"ｷ��h�'��-�JU��s���s��DZ[�[��t֜�?�#����׵�msm�Desm�2;=RIOk�����j�FcA�aXa��ş��ɞ���Y�\�.o��lI]mjOƖ���Y�3��	�5�a�3,�����"��8z�k����$�:���S|g�'�X��:����0�8y���b={�x�xY�&����|z׮�[��]�d�^��}��>�E���aq�T��1?]���?Y�k<P�u��j�Ƈ��|e�t�t�bn�E��TR\��Y�nG�	�	��[5	5��5	5?_�Im7�ݥ��]�J��=�ȫO�-��
a�{�ְ�)x6����eq�fy�����S�]6�E;c2AQ<ȱ����ӪE�.����>_URe	��u��f�3��8����Vl]��qk�4�4\Z5�6�`�/Yk<[]���;�r���^]b���ž����8y^ֶ���2�EٰiSg>`1�������u�;vu"B���÷BB�4��r}mz�32\��N�P�9�d�% q͊��u�&H`����5	�3>\�ZZr���wC��>��R%��9x Ò-N��?pt���(�T�Z�cx���I���D��{��m]Ҵ���"�Ǿk�\��ٚwb��jbE��E�.�����b�E-T<�7��-x�5	�!r5	��T(7�i�\�5�A�9�b�.�L쟌��9Ei��ކb:]A<�GY'A���������"Ƽv<Ic���8���rW���o����"伤D���j��Lmljy�4�\��������id�=�b_g;v%�P�s�ߋ�Xڸ��?���L�����\���eb�R�����1%j�Jh�6� �^tjR[�颖䕣���˗��J�[n�I��>8�����$��{ {6�F�[�鲪��w顮��w�z�/B 2k|O�5���c~8(�B{�ht�"F$����_@�@%���o@[��c��A��j�oe������²ua�[S&�r^����2�j�{���mb��%vkh�%;��&�|	
V0=�U:�-X-)����諒n�Y6�0=���T�N�����~�l�d�t��+�A��r7
�nC���e�ұ2�c|����1��u�}w?���J:������_�)%�%��%9�麈B�(�\�
3�F}��n�������^�P���/%�|�ڗ�����/K�9�`�$'�
�j%���w<�����P�yTG������+�zh�;�H~E��D�D)������oq��g���8F{�X�8���o��?~c�*�b,��q�tj" ��^߻�s"��{}��wlڵ����p��S{G<ԡH �5?��M>ђ�Gr$C��lY�R���w�F��\��5�㱝kg>�9kЇ���`5���@_����weSI��:_��+���9��5��y Gߋ�H�&1���I1�_-�l�k"�h���$�u�3�i���69b�}�]�:g[�̢KOK����l�"Ӵ��V�"��d�8�����V�M:C��|�>�I���M�6=Z�@����oXLMNL$�������!�,�$������n���xV�սf����@��������w�J�7ȅ-���8�����=ÖԺ�K�V�_�KDQ�<;]��&����Ђy�``����gQR��LX�q��d}�eM�O��ι�dt�Ё#�`B91g�<�veq� 5N��Q�;�kV.�Sݶ�~�<'Zuj&��g��z8�5�X�������l��_�1#s��JT2�E�N���c%$*KT+>K.^�.SӮ\eW���d�CR�e[m�x�%T1_�e�<%�	g؜t���H�Y�t��t�V�nE�!�PE�����vf�o�:�}:�:�9�N�_Yn�����z��iFy@��^y��k�/�u���R[4?<��qH�*���A�ү����h+���#�NX�����G����M�j�grՁ���"t�0�tS!��3A�a̠�J�����J�[Ō��;��ʻ���h�	��7��������\��#I�h��{�J;�����'����3�-�6�X3�PM��������1�>�J�l3�&���JÁ�ƍ�ިy�g�.��o���uQs��IR�5$@�%�K?�?
�@�D�N�)�+<WDx�8���+�혚V��Ƒ���2��ׂf*�~���q��b������@Ն�B��p4�\ĹS�@��4�&��C^@۰����[�_t(��^s�6P6��{&�Y�A(�A��8By�I�Ǭ*��=�7g��l1"xaږ�T��~� " ��F8��r�Iu�,�i��{��"|�U{G�����Wbe�������6�J� U ����_�fWAI', ~r���%D���r��)��.sh�p��,��,Ob���U�xc�/s]��E^�yr��Tݷ��ֳ����.!/1Qw6���[���f��wm0�������S��9���H�1���Ⱦ�1�u츪c�Ⱦ��M�6�U~*�{��� ����|���=���xmq�_��-�-�`e��a'��"�B�ed_���}���V��m@H�F.�"�����Y�4X�=55�Yޣ�#���jv��"-�Ъ
>&�<Edu��%='���qZ��ŭ����TqRP�f�\�6�+Հ���ߓM��;�r�8C�Ɉ�r���}�n�����c�Bv��Dt���@�v���].?ҜO��2�Ώ&ci+4j�U�r�H��\�e���\��+����5?�2�e��V���mC��9�(�V��L��U�]ȫjX�*��!�I�0d��k��;��W6�>>��a1!w#�ա�
���9��t��R i)3Bn�s �qHcпU��_[/97�<� �`;�3$��I����"d����OF�RɚE�i
Y�x��?���IB6�b��zup�����z�������3�o�2!Oi�\e�X��_���ڻN�
�'����{n��u���=7t/܃��x�(�CH=T)yR���2��1��1�d�0���E��\�����VIV��+VI^�H^�NE���ݕ�$YH����-�g�{,�����,���[{.�O�}d���>7���_�wBwG�#����Ru�>�I٧�{��w��B߮��6m�s'���b�Q����!yl�é�XgPMF���e�_)��������0e���k2��C�qO���a�/��d�%�u�νuO
��$_�V�фH�vۀFz��k�H�dWr|�D��c[ͥ��V).�"�Z���E��\�!��6�h�K�[�;]�q^�����\�H|-Dي,8H�	7��Xِ����`�>M(S6}2M�d��H�3��,��:`5}�SK~���z�/jwu�FVj��������8�E��`0�m�龷O\���K�a&�[��R�|Ҥ
�n�O���>6�����{��(*ߵ��oư���r@�j�(�Hl�fTb�4���{�] �LH���J�1�+qOq$��i�y�y�y�Lɐ�?A�ɓ��Bh�1+P�;�ĶB1�x��X_�yC"̂9h�L�����<I�����Čj[�3�B��=��[�w��a-gJ���Ǌ�DyÖ��Dj��|�2���(
�C�{��t9eO��l*'�i����EVh�{�K�7�N��{n�vp���pE�h�tz=�`8=����I����ѧ��+�^b��(�^;MYߏ��A����;j�y��^������J�v�x��}�So�o��=�8O��s�[����������|�Uyy=y�j-�Ђ� ��x�"��Z��U%�Pt���I�� g���<��׾��%x��K�~.�&�*���3eP��:��Y��Z��W��#W��~U��Ԍ^a���Ojt\ ��5�üaIĕ[�J��tG|z���=��L���d7�2��XDG�{7�߹9�s�L��� �Y4�,�� Z���7�_E:�.[�7��k��J|���m%��  B��5�_�Ub�x��a�d��UtEV�Ye�*z5��c����5G���."�٥T^��K�a�5	��(;�5�Q�ZC@�p�Ԇq{Y��
\#��`*��S��%�죪�����pD�6=m���-2��(�4������8�m�����.�y׬7^$�a�c�6�������ֶU�P����tL{+bM;�=(���[�2���E����x��m�5i[En�(��hŚ��P- .C-!���R��+Į�^F���T�W��P-sf:q�y9����tUϴ���<��5K�p���^���5�2_�I�r��5�'���A���Wh�D|�(ѣr�Ʃ������ߟᑖ䨞�hE�Ԇz*hE��،A.b�u���[�*�9b���dj�P���a#	���$��s�Z�U�v*�/�����5R�����wO/E�����:�F��	��Tfv�S6�5z�ѓ�L�VF+CC�Jg��l3ej=���-��r8�
�7j�ҹ�f��P!Y�ҳ̈́	�hQȨ��6��H�
u���xY�jm�ךW�����r$�䯏���t�?�:��ߩv�R�b�E�D#��/$���-�0��F��J� @ݐ�~iu}����_/`N�xh����	|[{�Z��X��߸�'.��i������=8qw�Pe]½P�!�hNm�Ai�g�gu�2��rTri$y~o	髜t�djR��j�͕m���f�Q�g%�2�CF��k��]}�\�ja#��ۭ�Z���|����#�-�ԙC�U�C/��Q6��(# 66�s���5��V��j����݄5�iY#Ҥ���p��בOF�"�W�gڷs�>�EYXkh��T̳	�;�=3��J����;���9�S�U4H�SQQ %y�G���c�ܔ_j�E��P��战�&�*v�v�l�FVϻ��5(d������4n���:�gp!L�|��4�
��B��՘xe�ޚ��x�<P_o�J݀M �,���ڳ$�^�8+��Œ(�|{�菧񖪊�P�� �6�f�	�vU���{��}��ŖE]O٠\`�ⅅ&��
���Ê�\o32G}Ng��>�v�1�4����J����:[ ���;����p#����v��m�B�֌j�=_�Ð����k_z��@���I, ��~ *԰+��;~�4�����r�p��f�@��$<�&�aB�B A|4�A����!B���
B9�v�1@#;�p%�'įj�����뮲�*,�=�ىW3�����0�s%�9C��@���F�6d��n�r�56�kY�Q��$u�Y��1�I�ISAC����ՠ�k�?��5�q��:��������Fob����@A�'��m7Ǒ?G����E|����}�/�AA��ĥo��@"��T�T?�C���@<����$� �)
��A0��@����$��2�_�^9���ZB���*>��$�	��	�l+BW5V�L�@�u���*V>�qG�uLc���;/�J!�e�4���7���a��}�U^�VN�O�hC��M,�&�������o9�C��م��υ��*���:�Q�\��-��i����ё��.�z�$����?������ڧ�04<_&��M,p�,z�I�i���N���7�L>l0����:o�X�t�2�`�LG�qP���T?�zJ@
��)(y⋲ ��i��iC9��c�,BWt���D��h��Q�]3L���eD��g�-t���쫚!y��|c&4p�9Ŷ����DS�l��k��[��j~����bf` ���z`M}���=cx|1i����d�h
aQ�?�`pi�j���LV���2��I0�E;W�6����Z�(u'�j/���@J� ��)p�p��M�&2a n�L7��~DVT]���Li�)%��vo������z��C�Q2%��q���݀���eh6��!m���I�����^���a���zKk�Q��~F0Ӝ�4��aw�hJ��Α/YY͉(j1F~�,�"K�-�}�<��4��σ�e;z�G�9Й+��\��K.�=�����q��S3c���ĴX0vL���G$"'�~��8
^����KLߣnCY%W�1�;������\zj��$���9�1�HNI�6]��p֯7ZLC{֌�pb>9�;:JQ�9�5v�RbG)�[�#���h��#^�������?ΉA1��& �$H��k�D��#v`Z1��g�V0�,�	k�1u�/�����N҇���������
w����讓<S�� oY9��ҠSb�v9C~�gy����D��6�[��ۻ���w��1P����ɉT��7Ǘ6d1��(q����ԟ�������z����r�@�:8��8i�I�+�3������/�Y~�z�#�9ݡ�=��fLe�g?.���g%~��z\��%?'��mÄw�jp�e�X+���t�]�}C��df��[&w�Z
^�,�&��&3�qV���g��bd����[��jY��1YL���hB��7��1�Q�c��#Z3�=�.1���g�6h�g�Ce���2P��R�	�E�d,l���$1yB����*�J�G�f�]�
�G\!��k�7��~��U*�'u<����D2
}/�_�G�9�_d�����/�e�q���
�t}S.�^����2=h�O�>��=F�;q�N����͞�y��� <֣&�!w$�`���G�K�c�C��NK�����A�d=*�k498�5�C�납G����pLrj��u��&�ho�&"y�m�����b�a��p��<v,1y��8tЬ�tG���p�k�"�sV��e��#�Ɨf���^���#ٍ������h�D/q^��X�xl+�����������A�� ��krY�o��t|[��1��@:|\J��W>����ʡ�Z�Qo3�<Cj��6�@��m���ޡ5ylv��Y�YGa��(�4I���FΓ��;�>g�Y�A�����:{��{�z��h#("�$��1;LL�?����$,`��@�pӹB��<k΃M�t�Rfb3����i�Ln<Z�o����>��z��q��ޥ�V��H9�NV�V�����ڒ,#�u����6� ���
½��0�q�˅�Q	����D�x����]^�w�-��}�H��j����
��UF����@���潗%��_��	�Z���k4LYL���z��>����F��*7��;�`�-��#[��y�Y�}n�ϓ����1ד ��Fx�O\�5��U�n#�8��Ԓ�x8���'�{��Q�1�9āL������/t��}	[g��������Ɠ#A�Ļ���!�3���O&-��^����^s�a:�Y
�w,��j��@�$�����8G�UhW#�\-W��K'xlV8���G*)Z�}��,�{_��ƢH�KCZ�+�l����u�I'�#�����S��#�S.��+�����z�}�%,�~���'���J�2�
v����e��ג�o4ﶫOlw�1ܙ�s���3�N�^K���\��O�~���#�&��3��6�R<`��S)�@ӈ���qߢ�q�v�?�s^�4�qٍR��[�m�K4�k�q���^9�ֻ����9>��x�9��+���C�\>D>�9����t�U�:(���ݗ�F���4u�Q_FY� >��Ȃa��@+���NY(Oqѻ߬ʕ"V�^Y8�����LƆiC�"N}�LF�U�t�(z��+e괆��q�����������a�
��e���f9Q���=&~{ܒ���4#�ǽR���=XU�~���Ѽ�2R�_�yw�8�GjoN��r�f$(^bμ6�trt)xr`i�7U\L-;Z��d�6 �#;F�]���/X��w�Q��3�(9�LlT��
vKSwOp]R˻�v�+���6��+q
�-NOW>��-خ<�W~&����+ח��]lt�|�"��!�:�����^3^Ӊ�R�\T	$��!��IcJ�NH-g�d�]���#�v1�
����H�DpEB���B�6��.��K�3P���6ד�7Vhu);|��Ch�+he�c�R䳲�в�'�z�t䳪{���l�䓩�[+s��pb�������|lM.�&iC�[�7�-�g��'��ؑ��_MN������n�zZ	�E~>-�rn�׸;D~;�rl��8Lg>��=��mGe9�ZQ�r����:����L<k��r_����ݑN:T�M�vw�0��u*��x2څ|��������ߦ�^HZhG����,*t��n��N���l�:�+������/�)�'{y�`�je�>�)��>�\�~�t��MvJ8����y��	ɹd�~B��ʾA�=�N-�^�&�u+�Y��!�������=��v�Z^4�\�{2���E����c�x��I�v� �<�}�0�� �n�Gn�f.$�¼ݿh_nfs����gnL0���א�%7y�e�}���zN_<���V��L�#VSH�Ao�f�IGki֙�տ�>�&I�^�ךЎ1k�o��EIF��1 6�	v;8�y���#����
�)���G\tl�9���̤Y|�>Ժo�����ʱB��$oA�}D�ך�#8}
jK3->�H����ڹz�ToOw㣦���EK�{�C� ��h���%{cّ�H��
l,��n����Z1���1YJSwd�f�,�s���т�'�!�#Y�IgDgd�7f�6
Ɣ�q�1��r��)�I�(�DO�@q]��l�`⸋=n=�y\'먋0��o��C�������Z���%�A]�q���i(*�v�Vz?mr��1��!~��Y�e����~
~b?L��Z��&)AtB�G�cđ��QZOk4���7���"�a-X��I�E۠�&��k���هfs�@�����/o����O�8�s�ݷo	��m�.W��0ҝ{��6D-��m.A�7��ԩ����������ֿ=Q�g��~�}
�߅ޝ�H(�_YB��}�5�A��}eV���E�2�(O���w;�������<A* 9��R׼�x�}ɛ�<���<C1����3Գ4m�6�k�R��ԃ�^��6hgq��nң�~=��ʙ9=M���Z��:����� �K�/؏ućpI0T�y[v�̆l�!�^�f���V"�:[�b~���$=�j���Aa�ͯԝ������׵���Ӊ+���O�.(�:J�EJo�=!��< Ƚ���>��?k(��msx�:��$y;�	.��e�R'�[��,���M-d��q�'�D3	l�h��@���d�Oh�&����0O,��E�&�D�ֆ���;����Y�<8}�`�M-FXڷhnQ���l��ώ-���[�L�	��r��gM��9�^�#7�@��~����G)���AA����|����ԡeJ�{�� Oj�@�a�t��|�9�����ju�s�C�ָtF��h o�7A
��u<dǟ}Z�g��CgBb� �΃O�9F�.����V�v�pl�N��L�Y��¼��l_ՌK���5Ke�43����vB�Es���5��N?h�7���9I���Ӑ:���F��uR�U@�A��չ,�Iu_��Z�p}_�(;%�T���_��\+�]B�Q-~�?������O��5A0E������=+��s���:�ә��L��ˤ��a�0�~^����"b8���yf��Swy����g���VӇ�7Jۍ[����v�Egz{����&�+���3$]p���N=�Y8Z�B�z2�P�Ų�,�l�R<�م��_'"��vr�,+���~��+\Pm��`ك�8t��m�k�������G����z�V�� F}޸�`pĽ��h =h�
���8��i����%�,�L�|Iø�P���W�2��!|X˳/!����f��o����T ���z���(��2?D�.��� ���3�vE�ݹ��$x(x����tz�қ�a��*�{V�Γ��1��k��^,]�)*�D�X�Ǣ�⡘W��a��`q0D\��
;�:��,zϾ���?q_Gu�[����z��W�Ԓ�[�n�[K�mY�my�w����,bc�,a2L�d��H¼!<�l�� Ȑ�	&�{ჄLH2q�GH�j�s����&x&�l��nߪ�u������{΅��_$���ʫA2<�2�ȎN|�q-�޼���{��V��36[���Wg�;3��C󳙹�������gny�Myχk��_��Li3�p>�־g��ZR�"
�*�Ҩ�z	��$K�'6B��o(��S�~gC�Z�M_��LN�NMR"+E����Q�Y��N����h������"��7�'��7�3���S��+V��2bު6�>��dy3_�C��p��\t�̐]�c�&��I�?[�;�M�OE�T<���M
��/X�`��W
'�!V�����p�Ӥ�����	�s�&�C�MIcP�$
S_v��GQ�����&U�|��(*��M۰9�'��6�?11�Gr\�W�l%����Ga'�u�#,�/i�l|W�'8�����zo�,���n����]���Գ�\~%	�c1ݟ{��0z����#���D=�C(I��E�+�%z�C}9��>�z��,���gr����S10��捛�3��*>U��W��m+�ț��Hf��u��Z�� �f��^߷��?ziͲ�I��7A߆F�9�Q��<����=�[��M��B��-�˙M�f��H���7+Mr�9��v�uy3%�>��#fæM�hR
x\�J��{��3;+K2�,|=��+��l�e�����;&D�<}$�`��%�9�%|�
���N�@	�d����D���8��"O�E��̢�Eh{HS�*O����Z��L\�X M���/?��8�c0c�ұ�O�ˣC`�Q��M��{Jo]ΐf}��K$�&B!8}��<h�d�
��_^�BOy���I^���#zp��"��`�V��/���l9�%h���Oh��ch�C6�P`�=�/'a����G�F�!b#��o�Ǉ�H�^j'���zߟ�/l����/��X�����֬�����<�����sF=Z�iC�f�5$�;'�ܾ_yWn�Y&�}6>q�d�賝:�y�Q�7B� C�H`=Ж�������M���ɒ�6�&��a�YX֘>j�2��$˹"w�ş��U�f���d�a�����sԺO���^Y`�N2$� �q��{�<�M0z����x�
�m��'�H��?�ڊ�V�M��R�(j?�n$p��o��p79mPv}[�H[F?�/U�a�I'Jp������V��ʾ���fmAʚ�=A��N��p�m���vXi�hG���yUk\���>���<ps�*w���o�볾��k��l��3���Ei��W���s˲���b�����D�d�1�z��y�m����u����r�:�&�3�'��V�,�Y����»��~�p�]j��:��"�pUf"cn�F��8���˞m���,ݵc,��Gg|������U��DI� ޕ�{��0"�uf,ܥ���[�O�[��\L��E����s��i��� ��b�ܾ+�c�+.�Im4c�k�v/��5-k��6��XXB��ձ�C=�w�#���{�^�=���+;��0ow�ܪWd��H�{u�ymO�"�\�Wb�=�S���T���n�-;�UqOf�y]�W�
$�[q5���G��$QKdL_h&*����j�&ң$�N�����G�甴��Bo$�Ē8AtGL�z¨(ᚏ�̪��9�L�����M�����Bƴ-��tNU��7ע�e|�W�,Ni���qF��xc��]�]��,���Ǫ^��2`S����k�K˭����ĩ��e#6�����%����WO��jr����"��iog�GQǉ�a�T�2�DL���>U������R�0�މ*� �.e7��I6��K���bԞ��򮶝˛�����v�\�4[x��p�:��p��<�T˝с��/�Ο�꡽��KsVG%�d��H��=�CC��H)��5QS�$M5o�K�1���<��TR��(�!����*��)y9+H�:�h%K�`ZU�P��u������٠��^V���A��[�7��̲���c�*������L?�݃N�*>�F���Y�y��=�M��;I� J�1ˁ��ǳq�G4��O��,���� 5M�Zc
}:�B�Z\�T	��M3 v�>-3�5y��S��p��&�K�P�( mp�Q�zE�sv��1��gxϺ�����ϒ@���ڳuټ���HM�*T_ԝ�{k��Zv�1a� U����j#9X�/Y�\RL.Y�,R��{ U;��ݬ�ꂕ���`����~��fUWF�'J�f�K�j�T��>��U��ֶ�V�r�mM+���m�խ<��f�����}:��7�o]����P��8ڇ���{=@�pǜ���w����W($:�AW�N�cݫ��� 5V\גh� PE�[�(�#��O����*셈Lp�OMD�4��Qѻ��\t}�?8y��8	���(���p�ϩ�Q�4�xJ�ԩ�oU��.���*���\0���t8����ϡr'zn��<ΐO�S�,��B�z��I<�x����7��S��N��V��QQO�T������%��L�Փp��'J�v@�'J�y����b�9�^\�w��Rgi�IV�����=0���巬�Z������R�4���m'����vū���OV���������f�+C6m�Ns2q���^�>�KՑ��K{G;�+1/}������ii/�R�	���_��V�Q�s�&=a��g�K j�Y��U�_�²E��+��Gڃ�#�g-6���� YG�Nbo!�Nq��*���x��fvbB�t��eҶK�B�V2��t�	�r�IG�.NG[ޟ����V�u�n���2�b�m��I���z�lxa7�$�y�֦�^���X�0��"ϓ}�����
y=�3�{�Ec��?��;�ڝ�]
]�����g��A&�&�RU:b�^A���XtW �=�˷0��t�����XY��`�ʷt��ɸ�+1�/��迌�U���lmٴ�Q6)�|m�Ʈ�u�!`��*����RaMOL������M� {,P����PӢt疾��~EZ��ph�3��������xY�שx��an*Ғ�|1��K����3>g[wn՜4G1�}� %�穣t����QqM��f";��wQtW��D����EwpW�S�Y�#��,J��*M��U�TV��?N�����"���v�X�r=(f3
t�ׅb���a8������������	#5%�y�r��!7X$/�l�M��['~Ki��}�Sx���'Ѳ�@K0K��P��o�)�k���%�R?A*E������h3��9��E12y!�c��
��������s�@�u��ۖ����s��&�<N���g�m���������	�^stC���/�T���n9Һ4�e�b(-����:�Sm�_#
���8)�G��-3��
U��@ByUE���;R�&�`k~L�ؠW�q\�ȕ�+�;�8��I��$�gl<;���4�<��[�K���"+�P|�Rد[��^�+Z,�WQ�����m6$�=�8�CM�ew�����OL;�
��,��{���d _X����sT�BU��?�#!�'�������H0CI��ԨG��r/��6��*��0��x��*����zT���;dZ�0b@s5�@n�e�k����(����1�c��P[�c�����t�z?�]{�/��L�1b�o��S��<.f���d
�}fk��y�<;�;��:���o�K��v�Sq<4���MODY�������hh���Y��id�St�-N��-,o��k��4厠Q0@d ��ߐdݲk&}Y�8CސQ���6��|`�
O���w��[��z�>��Wqa	6z�'xJy݅#�D-�ws�-��b�>�˭ma��a%&5�١�a�m��q��6��iCІ���b���Y��j�8�έ��"Z�ڷd9�g�dmr�꫻7}tc�=���&���T��"��ΐ�cG܆�o�VW��=�{+�rxd�T��5m�in�-l�+6��'E)M��
���i�[\�`��%�8��r,.ް��щ�Jlq���W��p��m���{͚6��Pv_Xv����`���;���:���\BmZ��w�=SN,�g�Gvu8C��r�����-����Ê��x��ݣ�J4���ߵ{yO2�!Çu�����;��^A�?��ukm"�
n�%Atİ��"_�V�b8:-f���,ک|���-/���'=Y�f��L(�v|[U�'k=z�	�ߨl�������_&�%k8�~%__h��=����
��D��$��/�2��`X\���Z� S���L�c�kf�l<e�8����ٳ4��$�*\͜��ǯ%�#U�l���zho�����pJ^��	h�9{�7u�iݹ%F#6E�� I����]�n�f��Ϸ%!��D ��(�C	/�a��ғ`*8�AߋA$oi@DOp��\N)s,efH�Rfhmj����I�_y�O�z���#pM�����J�7�%��x��9ɘ���W��Qd� (f<f��麱���\��9�歎�����X�C=�Jp	_c
K/F���9�Or�cN�*��p���aǜ���ȷ�gbO5��Jq�z߃�_Dn�NԭO��#z�2��T8�i{�<L��y���|�X�L��.2��z���Z�� ���mJ�iY�k�F�)��j��0�ҝ{O���WK51�r�n��J�5]-�|߱ku��s����X/��b��X�è�
�lēЍ����Gɦ"��ME��8�B�$q`��.�J�����~|����ҧ��rBϫN.&f�w�#Ɂ�
�сSjw���H��(r��2ԥ���x���Qמ8�2V�fvD�-v ��\e:��ă	�ھjoq�]�-�`ء�0�*Ч�r-k���麖<'T��0E 0*����u�>�4-�
�iX$?0�)�g|��8�DUףD5J$QU %�(��g���<(�FIJ:QRB�IJШΏt,U,M�5x�H��No>�w�d2����bjHx�K�OI8]��ӵHx���4)�m )=:�"����>����+z3oA��i�.'�]q���ȟ��ץ��#�i���� ف}r����C�Ɛt��*ޠ�D���Uҵ ��B���X����1�	�?'9�n�b�R �&��Z݂���tΔ�����b�fNa�ðjy
dZ�Jk��q,����Q� ~A� �����|��\g��)=�:��;�=XT�Y�����v�΢�Q��W��9��ec��?�I?p}Q.���P"b0N��]@I�t����eop�x9�`��+���]�j������H��Iw�1��`����똟���Y(YH�!w1oC��,�%��uz��,�"�C�bހ�Y2�1�J8�d9��`���,Y%���e��\PY%�%z�nX�"u$p#s��J�0.�U�ɒ���*	�����Y?^�o��C����ŃNrz����� v�,�e��S�v�ًh��(���.r�L-t�?�JG�`��}E��y��Px���0��5,Iݯj��V���³%��U��e��Y�=͗��s��G���y��ڹO�b\���#ۑ�X��c��gsw؞Y�`Z"��C���%�*�4,)6k�N��Ɋ��W�.G�uI:>wՕ�;���މ��pIŻ�F/��$�&��K�R�(z��#��E�o��؋�7Cd(�4<�{��M��t�Ƴum�.�C��JDH
�n
N�mx��{�q��N��o��\�l�<�O8���n��n��y�E-l�q��I-Z�54X9l�p��+�vv%z�@I��;����K��-\�k;#�,[xѣ��S�l	f#� �ؾ�L��Q��]����(̯�.�T����1�@�"�gy�У��6�9�2Ϙ�}#w��y�4)�LiZ�Y���ڨ<yJ�_o��A����W8���EAx�	o��+\�;�?'ϝ�����{|����.'��`-Pf����l�(�5s���=��*3mG*�ct�(����W�7h�Lu�Y�Mj{�kx�B���R�F{fz���̹43v�J��?/�۔h�±�o�'[ݛќu��ɂOU7�Z�v���W�h�k,n��:d�lW�Qq���HC�aU�h��iux��I�;Val)CW�?���������n���У#�0�36���.�zƌ��8O@%�a�B�-흭4�5��md�).+�BY�Y�
����n����.&���>t����;��!���VZ�Z�Ao]����}Y_�w�����~�����h_��J�BK3t�W�mP�M1�&�n�j[[I
�*�,�5%�o��~�P7���T������8=9���;�}����������?V�t�?8����@��ϗ��wu�$��n��.�/�O#��O�6�KryEF�!|$����%�攡���F`��81�w�# ����ˏc�Ocো�M� ϱ���p�Q�B���o�+k�c%�D���Τ���s�+/����U���9k���>f(�Q��o]��SZ6������PE�"�n�2��Ꜥb��Nɩ�\*�3gqz����'%�����V4�H�>�81ОH�S8���!45����Ȝp
��Eg8��9v"��mXBe����ş3k��2��R8��R�
���\��ON���c�4�ޔ=A����)T���f�v��âR�]�g�P������`!diP�b�D�d��χj�R�'Ӄ���!K^�_e��ʞ�ώ3]�*O��񬤊��:xQ�r�l��uUs������9p���.�f�>��h��E�]�gP��qG��þ�Ϋ<R�V�ju�v��v���f*K���هh�k��k�j/����������>�9š`�h�[�8������4���O9�O*y�jdp~�hD�O%���z�Fx�<T@���X��a7r?b�������7KO�
��~*�X)�a��H�������5ឡ%�������L @�aY�'ٞ���6�,�N4�������:7���V��mv;��@;��(����\�*:�O��`�/Bކ̪�#�v�L��.(�$�"}LJ�fI������Y�$��]�J4�$uT�2��%�D�$�����B�����%Z�b>��XCl&v{���h�ns]����P���[:o���@��Ȗ��ցE�"��O�Kg�tˁ����E��R��n���^������;��su�վ�M�Mʊ���d{7���f��]�iuw&ӽz�Շ��$wn�%����i�LClfG���P.��Z�s�+&	-��&�6\<�T��Ϳ���c��|n�u<����-��U��_�>��B����O��|c�+��������<����py�d���
�\5
��[�����'\���@�Ec��j>�����7���n���\�i| �}���@F�Je��>�G��B�@_��ɗ�_��u��c�c��`Y��Sz�a�9�q�wD���NhI��M�/m���F#�o�7��c�(y�Hj�j�}�X��Y#��*6�l��Fm%:V��OYSc%��=��Z�o{tK�p��ɗZ��#�� ~�=��ܺ>_�xے������|�o�3��=z͎`K�J�\6^��"A�������y�gKm��#挄$���3��;mv��	nCgȵ�,FuY|d��������N�d��v��Fɣ�e=6ܤ�cҸ.�Ό���F��*����9Φ�O@���˿���vi"T��|h��1_��tG���kUu�φ�Ah�0X��3��6?e>��Hӟ�W���So��7���=�B?~F?&���3���g���!�2xN?^����u�̃������kd�y��DkO%�v�(��h'�b^�{ڎw���$3	���3 $#�Y�z��Sm�K�	��d*�ɍe�r����eZ�&)7B�@��~��%`L��.��ѹ1�Pl���	|��!�1�I�����DU�p��)��9�[����ts��:H�WQ#m��~�����b�D����9ٴ����9`U�s;�lJ��a/|�<��Av+t�}K�����y�A��=�惞q�I9p��W���$Ԟ��6+ָ��X
��,є'��ԟ���85�F��,e}�g,�T;(ȯ;5����O�9���ͅ�w���eކ6�@`Fd�#��~=?1
����(��@i�@b�4��V�����(���d��i{,�t�� >�i����?�$�r@��<bG��Xp�d���=��F"��G��H/�*���-6�$i��L��UA7"D�������d�kh���u�"���K���C���jL�-ϏY��r�`��ٗ�������/�+9���j��O:e��]{��D�qQ�ӈ/�sd��Yn�-���uίI����"_�7�Z�}������3J����C���JOt�>���_�T~a6����د��\1M���-5P��dӑ��7潏��1�I��J�|c/&Q2�z��X�c�49���''} �$� j۽ wXl�X��)���^�\�Uܺ�V'�7�~�el^t͔tN��s���T���7�UM��It��筂�4�s���5���������@@�F��	�F�M(0���bYn��s,;�����;��4P9����z7V�'��:�J٩��w��fn��J�ʨ� �L�o&x!_�y\���e-������͡Nߗ8��n�xVt��DkGW��k���k�K�k%����<���:��H8���������dMq�ֶ�������:%�7Qe�4������&8B�1S�����,0��Mˍeƶ#����¡�!E�K,�&^��X�w������0<m��`��=o��"ބ�L��S��.�*o�����gs�ٜޢ��՜��c���Y:���#ϣ�1\)����7T���Kd�.$�6�c�,���YWB�<O�+�/P?�(.�2ȩ��|�~�>��_���fix��d�+>\B��]�,#�v�J�мD��8��G��]5Χ�* �Vkb��c�;=�F�l�eSԙ�lCE�f�ƙ.��=Dy����c$a��T��&��*�I���tjKHnu�]N��b��fE�K��<���a��/,�)��$�%_�\�ë��@5��s��K���w5��Dq���}���w���xs�Hxl2� .^^bw��� <vaƳJ��6�����>�0~}�����::��\j�����9}��O�i?�͍��s�;֦�h�څ��<�)�y%ا�:B&� !'�XV����~�3��Ȃ&�:��X�Rk��k�3��ۈ˺8T5��!�g�->�C����c���{ɢd>����W̭.�v���uҎd�`SU^��ϻ�
p/&�#��~#��H��hIzd��Z �3F�����M�w2#˞��i�\zf.oP��c��gMM��=�f�k���H�Xi�|����������] ���t�HU�)��҂[��~;;�y���V��򋏆Rbv�Q��Z��p@�ϳ1�Ǫ��w����޷��u����<��`xCa�� !0�$��&�� y��00�03��i:�i$5ZZ{jm{=�ã}��ö��Jo"�6�V=���O��������ԓ��_k�=3�G��;��z~����c�=���/S�şpw4���:�M=���T��+늖�s,��s:�}93�(�Hҿ��}S=��4T�}���Tv�!�X^�p�|����S���c޼8�_2��{�����U�27=mn����!����}��_����g�b�_rJ�]�����H=�	U�f*�sB�x���ұ�"�8�!i�������"Gj� i������GO��Nɒ���1/�"��#�95/++�Wu$y^NN��@�nI�1���^�NI�</�~�_�1��;�f����x��D��x�Eq5�4
}Lx�[b��>I�����.�w[��:����֫ ���:/�m�WmYs%�s�˂W����Li��,���{��淝�8q��2�`NN�|�L����<'g~�q大��������Y�}8/��Tf`�E�Rq�nH(�KiGė��H������է��L��ן�7��M��������W6�lܴಲ�c���rCaM׺����"��%�����~a��k�^�)#'㒴l�1##����eKZv�=)��b�.(4	UO�U3!�
p�ru�7� ��������@_��O�����1�hbXv)u�hT�����/о�m����ej�"6-�ؽ�h��EF�>U.(oX~U�5����mi}y��`�������-�l,�.�xݢ��Uyb��˫��ҍrz��촶�-is��2�T�Y^�����:'+�y�I}�%�+ʢn~y=�ϙG��:�",|rN�E����R^��b�4�v*�{떤�7n��D��D=q2_��i���7gjܒ,��M^�tyQ[Λ����Zl6˃�����7Sl��3���T5��.��tW
�B��s!�S��R��e�(��������4�W�!��*n���nX�9����<� �fSnIm�U>wxQU~�=#�ܜ�3u$�2��״H,_�۰Ȓ���g�d&�$���VSjiI�(+��(\�r_%<+%7�k��֬#��O���k�S�!���3Z��LV�i�T���%CVz^�^�r2#%5�}�Ւ���j���d����V�b�4 �K��,f�O��O<a�yETEXVNcmQ�E�ZV�Ԯ�dK���K�.[;��0ON1X��5�W��̱/�X��\���R��9���o�*�,�,%#�(%�dHM��uT/�_ݴ�� ��1S�%�
�OT��I�姽s��K+�>Ϥ�Oz!]|�	o����Ca]�2&r�K/"�V�]�;R��E��k֔���l�]V���c۝u�d���a�-�XɯX�N�-pT���ݼ��J2T�iU�%9���E�4��Ukj�3����f.ε�f�&g��Ɇ�˶��ۓDy;�DxJ��\,��xʇ��h�����`��Xw�J���r�"�3k�܌�7-IF���,)�ߙ�2��o���e�Y>���3/�ܒdA�� {���������C(}�d�����"��B�r.�D�����\�-Y�t�Q��ًW/i��Ù_�����Lj32�&��{�#�VlY3�d�J��,Śd�m^WcL� p�ϊ*�pvH���#X��ǭ�s�d���=�f��벷Muo'�L��u�W�?'e����M��/�?����,B��t~�k�J������$\��jn����ܼ��l��&'�=�����%�Y��#3h�bI?n�G����Jr'ӊ8�҅ȼ��҅�Rs^������ld���[�����:�胤�w����}����d��")�T)u�`1Q��l��6�5�2\/̤�Ϝ���&c��~��\�YN�{Υ��y���J��낌��ϥϜKG�.�o�-K�+�Ω�z��Ҳ%�QzG��{)���PEIE����K?tf����s��x*/<�^����+�T�S�*�Tz����o�I�λ�5�k�Cg�.��k�.i�M�SuN��5���5��j�k_�+�;X/�W��aeá������h����F��w6����l�b��7�S��ܮ�!�O6�Ձ�/������?��vKj˜(mT鞖4j]��F��>p��߆�6���������ĨC��\��ؖ�8�m�ܭ�ݶaۉm�ݾi{��o�<���s���_�B���W~�{����G�]�w�tt�A7t���pntNv��ߝN�U���z6��q�ܿ��3��������	���|�v���;�w�_׿��*P�J�������>�Y�)��HP���%��[����\���k����'�2��GDP�����%(A	JP���h��%(A	JP�����ȿ���O����Mo\[r����{�vu3�!A	JP���%(A	JP������gK���kiQ���}7J�I�;G�t��eIX�{^-����l��Բ!��(���I-��R�>�l�Z6����-�V��j�*,5�R˶$��ə$x3�O�9��LcֽjY�����%!;������9�l�k7
kr�Բ,ddޣ�M�=�]-�I[t�EX�ӭ��BFΧԲ�(�ܯ���U� 	���+��j����g^�8�.nǙ�q�g^�8�2Ǚ�9μ�q�e�3/ے��7�2��!A.B�P��f�#����B���h�E)(ث-�|B	zj/H���'��/�jn�vc�n��`�MhD�-na#Z����VR�fp���lE/J}L?~�ك��JTf�PN?��U�l}'80V��N�Cy��]�؍�����B�PT�N�{������p��3�yY���0]G�ǯj��U��b�j�an��bTCMA{?k�,4A&�����1\װ�n6�-`M�r{UT���
k1�z �f���?)<�
�L���É���r}�lE��)W'�Q^{PB)���~�({�LA��׃�>)�5�t�k��F.&���bvjbV�E��A�`��u���0�8!�!pu��J-P۵U����	�R��2�V�<C��t� Ӆǆ�-��˼�zB��T��ub�0����5���U�}�^~�m7�8^#��0�ǵޅz	��xk.a��=�A5J��ּϧz2՟�%ȼA�Q7�5��@T.c�:&��u*�0����������izi��I�l}��~	�.}�V���|Uy��[U��<��|q~O�5{�'�UvEm��s�d��ׁ�h���>�w3��ߓo͉���q�!�K(dQV��+��~&Y�g�����,;�sJT+Ey�>�A�.{�Jc���B�f:W/��J���GC��Lw��6�ZuC�g�=i�L8jmm��\j�Q^�0���W��� �է��ŭ֝jNv���ar麙��gZ,����<��7�C�Ee�+�0L����㓯[]g�<�1�\,�f�lH���"��b�G����9|g)���i<;w.�_�m||��]Q��0��k�>9S�خ8S�5q>@5��ӂ�+�ѓG�{},�8ϫ)�=�4������r�xy���O=l󨹅�#�,���Gy����q�"�w��g�Σ�L����K���v��P�����2NV�����<73
g�7��C�D�a֧Vu��"����+Uy^=#w����Ӏ&�_�;]�n����Ѭ�P��|ڸ�4�����ļ�B;������ڢ��;�p{s/p�k��S�^�t���v���>�Κs�
��������LO�S�Bl�������"�d�S�<j��Qcե��}L��=��N�!曪��-���yX�(���;��x�h~B�F={v+���4�g������5�bg�X��v"͆łvwF�´�;�C������?n��Rw3Y��N5�e|.�6,U-bQ�ʠ��t_�xT�wx�e�N3ݧcH1�J;j�� ���ȸ�$�a�t�.�`�+n�_ ����4�v��iY�	�~�qf?u����2��g�>1[N�>+�r�U����{��<F�1/�1�<�ν��k=@���z��*4���e;kiB��,ڎ���ա�-K0�C�_�,���C����q�G;^[P��r\���:�m���s��l�zp�`#���hm��zu�Q��-�����z-���!��=�Kډv%��t��؊�d�Qk�F����?*?]���[�r6���0�(gʳ5�m݂�m��֯a:si[����3	��%��|�g��CmD�kŴ�a42ib���w$��7������Y�4�`�ի�Qm�Y-��T-ӆ�J1�Cy3~6D�kg�\��8nӱ���c��~5�k-C��ո5jY��ي���lgz�\u��z6��i����\z�;��q����m�eѼZ�@�p.Z�����BQ�a�P�:�+��3�Ϳ�]h�������!筄���C�%��
e�����a����ΰ��+Qj�^����)��;���Sbktw�CJk����p+��=������y\����3��Q�,��*��v�7Я4:}.�kZ7��}J�`O�����	)�x>����������^E]c�XT	��.�B�r�ʠ��T��nesS���q�}!�%�v+�nwO��G��V��r=�[��vz���Z�����5�ʀ������W�ux�{�!O�_	v��n%�Ǻ_��а{ 3}=  �sC%JSX�u;ÃAwH	���'�5\�b%4��.g e:e`����78�bd�fBJ �5��������~��xNWX���0��a
t�a-����c��Ba�p�=��%��撐2���Q\�0)�����A't	zBQ�s@�e��-!�u�C��T%�����A�������`ԯ*��+�?��
��	V�\�}8��q8���̤Q������C}��*it:CE���!������@ei���Pɀ6��K�{���3п������!u�w���:.�Xh0�z�8��D��b{�A�P�:+m�@�`ڰ�X��p`n�@Ѓ^����	3���p��0�4wT�P+�����~�3�
Sw܍��t�� �3��q��I6�E=>�w���z�xX��Iˣ����A��;�� �C���@�� &h*	������~g�t��*xԁ�ha0@�qS5�~�70Q�%�.N�aq�����i~�uB�^?�*�
u���AV�/�)4#�����yvy�����+��R��Z�)E0/s���Ip���}uD3�
�5~�D�A,y�����$�rZ���ڨqB,x�7 pc\��+�A$="�>�L1V�(�+�n$;����g��
�]'���)�v�|���B�q��J���P�$�aِ�a�q,���8w+VݍJ�u{=�S�6��;V`AD5,����K� �A(�g�݃4xC�Q�hX
�Cn���Ϩ��<��A�"̈́��\@G�A�q3=~�P&�5nWXs����{<,�*��;����q���!Ó�Gc�)jW�����i��S4H���L�(��\ o��JGkC続�z��Ciko��TW_�,��@}I������uK���5-�;����e������X����^�ѡ��+M�ۚ�����Rۼ���e���ZZ��7!���U�����;(�������֬ojn��Q�44u�P�`Z��մw6�ni�iWڶ���v�c�:�mijih�*���[:�嶠M�ߊ���X��̖���ۙ|��m;ڛ64v*���u�h\_�j�7��TmsM��b��fs͆z6�\��0U�m���	���_mgSkU������bh��������X�ioꠀ4���=�3Z�k��\(��4�`�o騏�RW_�^tr����l�l�,�q�:qr)q�É����f��xx����[/� #� #� #� cf6O<Ę�CC'� #� #� ���f�3��f��.Q�Ԁ@
�ce�>�Х�-�Z	ƈ+.v��F�K�;>9���5_�x����o���))t���bǧ�a�N�H���б�:�\�^S t�0G�E*[,�`ɭ���0C?R� �2"�n��E��i�ңDN�d�mb&��?�<"�B�FڈBv���JRI<d��O\d��n�qr=9@���'���!�#O����)�d��@�m2A~O^$g���U�"�H����v�X�\�D
��RP�(��6�:q��O�B�A�%}V�NzO�~#�*�V�W��~'>-}(�N�ǥ���`�O�B��Eb ��[��g�Ń��q`�<�xX���9D,R�E�X,V�`�,��E X���x�g`�(���8,^o�w���dB�ȋb��,��E����F`�Xt�^`�{��!`q��X|X<,^��� �_B��c�-�,`�X��`�
,����������
X�,~,�#"�O&vP>�X,*�E��,z�EX�,n��ǀ�Q`�
�xX�
X|D�D=�[L!O��d\\F^+��F`�X��EX\,�1`�9`��xX<,�,^?����JiRJ�~'HJ�T)}$�!�[�caZ�E�(��Xla�D�q�pX�,�
,�������,
�E	�X,6����X�F�f`�O��`�U`qX�z� ���b9$��"`�X��V`q9�����A`�9`q?��
�x
X<,^?o���?H��d�=)E��� �R`�XlW �~`��XX��b.�X,� �M��r`�X ������u`�.��P�!F!D���%�b3��	,v�a`qX�	,���g��K��u`�K`�'2(Z�nQ!׋��A�Xl;�E���������	`�M`�
�8	,~	,�,�H�<(�Ha)_�NZ$퓖I7H�E��Xx��^`qX�,G��1`��t,�8,�E	��,� �i�-��`q������D�"8�2`�X\,��[�Ž��a`�u`�`��xX�%��W\L��j`�X8��X\,>,�X|X<,^o �I2!ɋR��Q�D�"�Iۤi;��\r�`q�8,ƀ�g�ţx}X�,���)�w��҇�b�n���1�k���l�?����n�Ȉ�'���������$��\�Y��d$����JD6Y^]�ܻ�n�L�����,�|��p}�slرc>x�����~v�u؊t��U�FG�:]c�j�>�%��pJ��zd�����n�-�lٯ�W6Vo��H�(�������([�iG)[���*^���t���S�Ȱ�d��z��^d0��uEpz�%:��*p]��YYRU�q&y$2vx���i�db0?�탸ؒ���:.*���eep�\@Y6HĠ;ɹ@C 24����`l���z��mkS�I0�F#��-Hu އ��Q96��"�(��:b Ā�+BpI�C��`�G"��PF�:�Ibt��x=k8m�#�U]ͪ�@�HD���?|�03l�a�)V	�ʲ]��V�LqA��hk;�՘3PWD��	4^]��`��J�I^�����pA#u�Hn��x��Ȗ"/D���O�r#�M��FpanԱ�^n������d�Ļ���9됣~N;��&i�N0��gst��y<]�t����骫�1E�o�:���g�:���ٝ�pg7Ĝݠ9{���v��y�EC��1��5w��n�����������������`f�^���nڜTK �I0�d!Dկn`�3�I�P�B�=e2�V��I���Y�T����h����&:��##9;&31Y�q�W}_���L21�_������X�j���*!*���������dL�)�z1��hD���ɂ�BL6��aQ�aa�#r�a6���G�a�cGh��F�K�Ӆ��P�h F�+��D��u�Y��j�4�R��`��4�����G�*�5�!b�3�Q#c�fB�1#F1Z������	��ք�ϗUۏ��H��������2Z���e���a��4q0~��0�x@@͂�RW]W�4B)�!ލζ�Qs�P���f}4x"����:��#t��	u���>�A�x�i�tD":�I�)��)�N�l1땸 RX;�-�<�a�5��S�� �h(Ekܗ�g�������Ӳ�98��G�EǣI'X�'1{R-u�O�M�����,���\w?��i����.v�Z� ��:�< R:���z�<�!�b��w�w!W�M��}�B}�M�q��l"f�ZU���
lq*�������G�,4 ��T}�Q0��0�3}ύ3Y�R%�W�(.ָ���¡,b�qlF5�X�n�h�P�m�p3�ou��������x�艅ƛpB,q���"��:�r���#�,D�hw!g�p�ɂ^>��)�l�D,q!G��X̩AgaAg֋fY�F�5X_�v����0�Q�dv4�Xu/�FI��TN8�Zqv,6��i��ي3J�AՑ"��:Rm1���,-2��\�j�<�54� ���K�b"K��� �p+���3=�b#������Å�����f�fyDf\�#�Ac���~��F�@����5�ϢU�!6��h����ux?�q�*�Q��E���BA���|�
b��@�z;�B*�a{��Z��B���Fb5�'�cG���Y������6T�a����B�c�Jm!�vd��##<�Eq�Ig��1��[�*��~+!�xDd���6>��#v���v��zX첺�c�n5x#�ɋ�D�DdHCu�)��jƕ/ p$�g=�k1�X�3&��?�m%���(>�e"��?y�ű�u&�Q�cƢ�@�O�d�Xi k�����XI�d�V#e�t��1է�X}x?���`��͢'�fMO_XW��,"���pi?���gb3D㙗i<[-�Ւ$$	s�E�"]�7`���U&V�鉉�c�'�=:q�jBC��t	�qԅ�|���=~��/�S��2>n5�5?���_�X)0���0�x�eM"V��ܓ��U�����|���C/:j=je�N�O��:~t4�_�G�_�Z�Ֆ/\���Q�����+����P�N�QF-���CU������$����U���ٱ�!��S�r,)��dbMy������!ס���W�X��j8ۑ�`G��þ������f����Qz���)�eB�V�~	ך>�߷���!������`ժ���S]�e��7�|{��aƾ�K=j�M7>.Q����6��!��u�f$6�8~br�����ĸ�d%��N�똘F�v#����e��*k\�;oQSІ�~ЕO�Ԗ��'�Ǩ-�����Sw5㭮p�
};�%PZ�����:�+�ﮞ�V<V5�ݕ݅#�I>��[�]��{�:��l�C���-�O��^"��G��Mz
�@��ZϺm�;�($�]] ����j0�&	�O>��)���u�IZ�`kTg����av�8��c�>�9�Yְw0�lGW��Pe@��^��I�<!*.z�8�وA��F����qH�cK#�$NR�'�2�:)�=3+ď�������/��rM��]���	z��A��b�w��J�3�P�O�������Η���c$��i��ڈQ<<�w�Ɛ��,�A�,I����`^Fa���p��c�⸖���Er�*F��/%��o��_֬���L�������󹷏<�S�����ç��d�#����ÒHD1�">����ص8e�)�-*-6�1�Ĕ��i▎�4G
��i�m�P�������$�hL3��{����y�\�bN˘��be���_Jˎ�wz��;�΁��V[㘗e+[��QQV�r��;Q]Wu����E2��B�-i�ͭm�eK�xu����!��W�;Z*ׯ��]^�Y���lŊ�E�\��Y5���q���x��^�FH��v�8B�����?Z��=��7�o|��5=������(�N5���9u�?�_��Ѻ�6-��V�ë?����5wNmyr�w��s���[������W�.n|������3���`��_|�_�������~"'����5w?8���܍����|��G����/��w,u�۵�Y?�V�w�v\�����/�����,�V~�<���ۯz����T,[S��/˭�U��r��F�7���w���}�v����==��_���b^��u��[g�q�C߻)������#[|?���m��8z`����ޑH�t�������3��j���Y�{ve߷qf��kϾ�=$�.Y�����.��V��,!�Hv��Ț5*���%��]����g��9�s>�s���~=��s����]l���ϵ������Q{6��e��h��>�T<I���oW	U��WQ��ؠڀ&��������{u[/��W�tu�m�:u�[�`q��7�X��awL	������ͪ�Z����������:�����} ���@� �oMB�~pH�J��v�O�-�=w�mn���-fn�a�e=���e���n� ��.��`��.��Lo��w��*	oz�"��H|���٬>��s�}�.F>n̈�q�{'�V��ٙe+�B��pm��˱��%3]雗�KT�h���`�~�`z��ݓ��&G���,y����ʃM<�&�Eފ�5S��k���y���������Z�C:�IG7��紲��O������A��ׂd[g�{+s�ݸ����P�5�^��V��QZ�R ��X[ݗ1kz'�=�:��zY��	qo̵ua;=;p&��w��
.��1�w?F�=�	��9;8�Z�س+��8��r�	�� �H8B	HaU
�+"\�Ͽ�\�[�ĨZ�`�+�i�~�Mk�>�^Y��7�JOq�V��N�H8`�B��|T1�N��;��:T�CH�IM_��`{�侘�����N��%��s:70M\�W�����O�wW(B�?�]s|��JŠ"�{�_�w;Z�E6�vIL<.n��[��iU�i��zi6�j�Q��s��4T��JҦ��5�?GS���s#�����2Y�� ��q.�GODs�Y��~Y��0��j�ʝͬ��NNM����-��߄�{G�J��р@�P| ��;�<?#X(�P��ET�_AM�m�����)<�FKq}Fޘ/m��fGp�� �l� c[`���'�*����WX��m���f��K�:�
(�)�)D��sI<��M��iٞ�C5@8yH%�9�]��~��P
���*�r����{e~���ڠJ��9sw2��Ά��ZX���8w�Z�sv���@�)�G妙�&Y@ѷ���v/I�V�H�]Q�Z3��-M����2�4��N Y�*����qg�?F�I8�YǨ��J�R����(ҪO���B�>=�]n����q.#�E�쭧�n	�D3�u���Z�E����B�o4.օ�*�xq� ���-�A$��#��~y�`Z%":�9*�C�x>�3����V�ǀ�[��6��Yb~�6m2��l�5�_�u)U�|^
�~SP�#^��z���y�_Pq�N�(��UUUh;>�U��ˡ���f~���ѭ� ���Z�p�"L�WP����[�բ���g���|�W�s4f����W���`�[����5�R];r�����_&���q�9�g�\<b�)7K��昩�h���o�߯ ӻ�\Q�_Z����<�t�ƗSQB�g�Ә���1�Ȫۖ���zd6�4��Y���i�p�]�Vs�!����-�9��k͍߰(�Ƣ��6���f�v�=&��R,B�I�x���p��=�����Z�(�O���p8\� ����qP�#�?ۯ�C�d���G��~E���ᧁ��-o�z�S�PǷK����:xZ����f�^-�8���
S�c��J|�"H�@Mȃ�5?�d/�uy/���Z��1�m3'YDPX��.�@�\�����b�^q�^}\�|�y*�������Oq���#G��"�].8���tH�_	Jc
Aw�~*������������t�5Q7����u����w(e��{He=��J���	k:�+���7�o�L^�5J0(J�6{ס�M�i��׮o��ޑ��H��$mZD���I���t����u,�In*kt_~�p��Pl�Z����J@���4B$ �B6�8`{�^BD�\�}8��؊Y��!�$��bpq;@�wѱ��݌~���g�{I�/8�ϣ㟪�YO�=`�k�X��4Ξ�p?"�� ���C80���!(�m߈�]� d�x?��Nƀo;l<���?��\�[���CCa�BƉ��^���VcV�ho����e��BUT	�&���
ef4E�	Y`�ɻ|5���T�M��
f���K���$"���H�i}鬙!�X��:��ȹ����a���j����Ǽ_�l==qg+��F}�����5��w	��P���6ėogJۄw�0�W�&Hv��H��Li�L%�=(�5!��
E
3�1��f_w�̃;)=E���Ƚ���T;�aZџ9�JV�)��	��sA��y�S|�N��ۑbdNԘ�*!z�n�ݮ��ϙǳ{��k$�W�¶�CV�sBj�꟞o	!�b/�5�_�whyi�f�E���2S��a(���z���)����zaߡ��.B��5x���<a �&)��8s4!������ڎ�P�r��h}�6æ�*r@~¥�_[����'=�f�*n���x�" J�񖵉SR?j��G��4�J�FW���=�U�����S��;�0^?�^Gz�ߓ�@h|�o�Eb� ?� �ҿ�� �W��}��KH" q��L��^���9���?��ϕ�myQ��y�BSi�=\�oʑ�xT�u�ޭZIf���-d�c�O���I6gT<� ���b��.�&��v+�Q���t�w"��.�pK��M�;���/S���0�/8��G�x��Ε�uU���z���:m<w�8a������m;q��<[J�>�k�ތ�X�̑7���5$ˬn�:EŦm̟���ȉ��C��JW#���E������эd^ǈ�M'J��A����~A�UAd*�'I��f�	�1��!��"�aw���u�2&����B��lZ�u��/�=���b����/ɓ.�q�+�2��1��K�l.����V��x,�84eQO�M�!k=��\o<-)�U��>�M����F�U��7Ҝ��P.p��1֞V�nj�
|�5�=Ѩ�Ѻ�|�d4�ʒ�:P\?�d�[�3V�0Ք�<��9�.PL�_T�6{����h�K���~����[���%t�&�N�<!�j�/T�I�����n*Lcy&%SV9Rq�a<G�}jE�J�[z���Kq	�2�?ɉ��0b:8�%c%�3؛�����d���a�y�� �����L�-���#���n�v�f�����X��:��� ���Ӈ`�� �C�S�g��_�����N���'�Wn���o݁A�ȿc��Y[o�9�[{�zzÜ|�����+��O��;n������ٚؒ��y���ia�,?���zta�k� fXߐ�#gi*��mR�bjHo Yb��LX�S�W�{ė�y�v�F՛76�Gm9��,�#CbU�Ȓ�{�5�m��������,!L kV�ha5�_��Ԃ]�I�@����D��+��U�/S=*��O����K��t5U�e#v�8��:��q����tw���ϝ�r�������tm
�u��A���Z�H�y�=u)��$�O��������l*�8��|��=ߧq����w�b�<��������e�r4�����?L&\8v�.D��������K�Mt���艑�O���L���i�+��
o�	�_�%�#�!��e�m���^{��(�aB�W
��q{ѥ����!�A�J��FZ!��UF[7�@keV�����������@�-���mU��9�]k4Ӧ}����Jd0j�Vq9�Ζo��.���u�.���Z���Wez��ut�L81����U�J�풎�&̥,�s�:jʏ۲Λ�������T���҆�!�{GC+ 4�6�S�kp�<'���$/�	N|�11Nv�M�,��H���Z:����P8V�F�H�H?��5y����E�:�C�����<�0��.�a��u�0�a��ن�����f(�wi|���,�`�C����F�t>���]��Ⱥ��P(xe!�K�ޕĜ��g�R���*f@�s�ʊ��em�LL�9UL�ۅ��> �N'�9�
�(�e�i+ۥPt��"n��wo1N��E��l�6x�����=��@}IW�7˨�)�_㮥g?t'_+����*��򐃡2}����"��Ix���"%&�m�%i	e5�P`*�y�f��Y��JRk�z�䢜��D ����@FŇ<�UfNV��N�rS��v�B1h0?6<��>Fp4���z�4�����?�v�&-��&I��"��A>�5J  �D"�b��`�#�����#�p	]���9/\&L~x��������
(�:�9���r����#��.73�㺸4f�5+KA/���qA]�A�[���@��k�܋�,�(�x�����9�C!�Г#>q��k���;�q�m��!�)�2�/|�0��DsgO��\[	�.s����Qp�='�E0|X�|���QE3a\q�}�a�\��b��J�-��~�W�j(2? ,'_ַ�j,5!9�ɬʒ�_���C/�F߉B�,(����^EC`���ŉW�hZm�AGm;�XKA�I�:�1�x�jR�
endstream
endobj
112 0 obj
[ 0[ 507]  3[ 226 579]  17[ 544 533]  28[ 488]  38[ 459 631]  47[ 252]  68[ 855 646]  75[ 662]  87[ 517]  90[ 543]  94[ 459]  100[ 487]  115[ 567 890]  258[ 479]  271[ 525 423]  282[ 525]  286[ 498]  296[ 305]  336[ 471]  346[ 525]  349[ 230]  361[ 239]  364[ 455]  367[ 230]  373[ 799 525]  381[ 527]  393[ 525]  395[ 525 349]  400[ 391]  410[ 335]  437[ 525]  448[ 452 715]  454[ 433 453]  842[ 326]  853[ 250]  855[ 268 252]  859[ 250]  880[ 386]  882[ 306]  884[ 498]  890[ 498]  894[ 303 303]  1005[ 507 507 507 507 507] ] 
endobj
113 0 obj
[ 226 326 0 0 0 0 0 0 303 303 0 0 250 306 252 0 0 507 507 507 507 507 0 0 0 0 268 0 0 0 0 0 0 579 544 533 0 488 459 631 0 252 0 0 0 855 646 662 517 0 543 459 487 0 567 890 0 0 0 0 386 0 0 498 0 479 525 423 525 498 305 471 525 230 239 455 230 799 525 527 525 525 349 391 335 525 452 715 433 453] 
endobj
114 0 obj
<</Filter/FlateDecode/Length 227>>
stream
x�]�Mj�0��>�����d��M�)�,�C���������,r��n�B6��}�Y��=u��w���'Ǹ��-�'UW�M{Wn;������$�;�j�".�78<�0���o�=Mp�����k��8#%�Tۂ�Q���jf]�c�D�i;
����"©��o.�XdC���j�y�j�����0ګaq���}��sq���߻��+��);(ArOx[S1S�� �o&
endstream
endobj
115 0 obj
<</Filter/FlateDecode/Length 31394/Length1 93840>>
stream
x��}	x�ŵ�̿I־��ɲ�E�w;^�{�E��klǁ8{�$�H���XZ -�ʁ�%��'-%�M�u�Gi{��n}[.�ۛ��;�K�8)������H��9��9g�,���#��� ����ǺҞA�{�Pϖ���]�����a+�p��6�"D=:�e�Ж��އP�1��s�7�h��~���D��7�{�X�0��!eBg��p4ԣ�v�.���$B������|lм�п~��ġ��[�"V@{���������+^��з�g��s;d�!	���޲y��B{�e[��wW�!�uF��^�����_~�KY�����|b��������\�[�$T��� �8�C��������%����f�������h7�a�
����8wj1L<ND,���0e�?��@G(D)i���b�E	�/��;
 �m6��U�O��I�iCx��ѧX�)�1
��_��Fq�3 S�n_�t���G?��8�O�x���6�0����r�,k���.F5?S�xN�)T�$]� �_��H�*?�X��H����y���ژz�]�w���M�C^fǍs2}����n=��o��9�ꟗ˾'��d��|��nX��y�K���&��D�f�Ec�A1�-ٶ�/˕>�?�Lz�^���j���c��ۡ�"�@�铭G�!w7r�O#���	����I�s-���B`ko\��ż9�I��~�^?���p���<��_��<��>�h�%� ��.\�Z���M�/���X</mB��ư�-�S_C��漄b�bܒkC�
�šX�[��'��{P<�*^��(��4s��_��T3��|%�E�����2�w�N�
�&�;�F�������;�q� �T����ڎ�[(](�Dџ���Wε�B���P_��7mk� k)�����hi����I�u�Z�����'P���9�>j�U?���򭷜�Nd�ۀ�n�b�G�mO������D���_>v�
c��~�_�/!�?�WP�_;�]��Q#�
�G�9ݍ�m���oF��/����@����a��4���9���Z�B�� !AB�� !AB�� !AB�� !�?n�W�!AB�� !AB�� !ABps����@^�7[c u�O7r���9!AB�� !AB��?��p�/�P�݋�EȆD~	K��P�@�h9*C!7����F�D~�u:�^@gp�-̦������UfV�bo�_
�P�o��=�v����x���G@���1{"�w�E�"���5�av]A����_����������]�P���3$[H�\P���)!ܡk�G��P"J~��r�u�>La%Va��qx%^�;�F��Ļ�>|/�?��.��+q.�E�X�����T��)tk���St��!�af��$��[VNHd_�{C��v(`w,A�;''g~�Pv����|�/����;��'��n���E���o��475�\�P_W[S]UYQ^VZ��]\TX�,?/7';+%9)1���M:�J)�J��"�eh
��rGE�����2NGUU�;z ѳ ����bq��[�f[��=�����t���*[*HJ��;lދe�i�����9�l�i�\/��P�C�n��r�`�͋�m�ފ]�#��e0ߘTR�(�$%�1��R(y�[�p\
T\y���r����)���ll-/���m�
sy�R�H�˶�Ќ%�<r�i��v��}=k[�t��GF�z�.o�����7&�r�7�QV�u9`�ڦ����Q9l#! �1��Ř� ��Q}�H�lq�M�,#�(���턖�N�Q/T�[�u굞B�W���&-/[��r0�27��a'�*��w��{mI��}�oh�yigw�A���8���|ki��ˠ��	�|,5��t�&664�zS[�:G�� lD�[�!�a^]�u��򦔗�l�#�e~�\���3(c�ݱL������^C)�Y>��7�廭}p>l�V����ks���)9T��wa9���0
�v]�`g�sQ���JY�6"-@�*��QR *�P%-)��b+
v�U=Hi�<P�cJ�HM��VY�mv?܂$k�&6�+^0�
s4�׹)i�ބ�x[y�M�̶4��E`a!&�
6�1psG�4�H�d󢕶VG���gȽ����Z�om���qM� ��)iYT��εJ^�`����P��sժ뚫�Ͷ���y���L�lp}`ǜ���\M&��
Pm���Me��9={�wd���R�=�O�pT��8�[�iM���{�RT�k[J�A�9��17>ּ���
!۱��S�J�K�Ƣ����T���� I�F*d�&�����3n�
�����Nc$��AF�NS~�*�� ��qnG $d���-�����68��Fn2�፽�Q����hS��+q��x���/&�b?�#x�
l����AG�W�����Sc�֑;��:;	���,W�T��y�R̛?��٧r�7�A�*�.�W@^y䥐�@�r7�Ő�@�y�ȃ����YsT���
�	�?B0��B���H!���}��PW  2:eBS&z��8� ��hY�rN�ɔ��C�� �q�)V�N�d�W���Y�Wc��/�����[���������O�o��������)ij��8�<���A^9���)ٲ�&�&��_���8���`��??�ş��_��_�8ȣ	Մm��M�NP�H��6��?���i<�?;��o|�i|;f�.���~�8>=���{W��&�Y��K*H�OI���X-��X���:���m�wl����a���v��ѭ��ѽ�ӣ��O=�?�D��'��?�����T~�빇v�wӕ�����C'z����?�ϓ���+޷y�S�.����i��>9��~j��{=w����qr�g�^|������#�ȃG�:B��yV���E�¦�!�wwm:0DfG�6����<�m�۶l;���Fm�lm���rr�g3��ۀ���=�'�{*�<�'�<�*{==��w7^[���~r��Ƴ:��l�xN�x�+=M'=�\�����@������������*O��*O��+++<��GGIxG��G4�g19x,�ȇP����h���+^���Ǽ1���ݸ���"Ϛ��1�?�v�ڢEЂB��@%�ވ��S���%m�ރ��v�YR����s��F�~]y�v�oT�c�CB��(�\�v��s����5�~�����`�B��<,��|	 �S'��O�C�i����*f~�ɣ�P	��.�4��?�� ����C��!�Wf�}�@:�*`:���k�l��A��S
�> �|���	c�3�b��:���l�H%e�ѓ�o��Z�L�
�Y�q� 6�����~��@�텼mD;�a��j!��Bhxp }��2�1�����U�ȣ�!�:�.��k� Һ�����hm���B��G��#0Ϸ ���At/z=-U�FV\�w�#�6X�!7L��7`� =:>�����g�G��2�:��(�h'��
�eu�v6q�c5�+��=s`��6��HB�IC�J�O}$HC����Q�;:ID�*S|�X/
۞3��Řvdqt�Y]]�`���;o�s��Z���S�~��iRJIQM�����4�����SP"��sD%SY����ETV����\fvN��IѺ ��������>�E/o�dz�B�2�.my���7�8�&�E,͊Eq9���Ҩw9�Q�1)9NiҨ�
8�W��U\�e_�O
:����bXfҨ�&��W�U��t�R&ӊEZ��Y�f�����<����¬~F!':�Ӂ�g���TU��wE�ӳv�ɔU�M�)�8$Q�,&Z-?g0����>Zr�\j78B&>�+���v8c�Ȥ�(S�C"7`F怗&�I�a=���X�ژG���PM��3�9�#�t�#}Q��w��y�͐�;\��b~v��vZA�R�3�(r�v���r�^oV��;g&�)�¬Շ�1�@˴V�ƪ��-�O��J���9�����H"b�F�\K9�J�C3U�����22�
�(֭G�s�|�Kv縶��J})l*��<MV�3}(��)���@��^I��B�ed#Һ{�����}�+���Ck��"���EF����G�
so���=�r�����R�B�u%DzF����W��a�J�ʔZ�B�ǧ�W�&﭂"+7	�����(ܭОC��0Ӹ��:ζ����i�2}yL�tD9��p��@�(hv�	�̎���������`Չ_?�pW��wxu������z���|n�5�y(ں��ﾸ���W\}}��K�{gg��u�z,pO+�N�#Pd�h\�5s�Q�fr�5y)@C�yu�tz�������'H]Ǆ�D>)��H��� �I�95T��}S���|V�S���9	�0��h�H�������OA.��!�0FK'�1�)z\����L���p&���YH�_�z��L�0�5pYg��b�#A�.H�>sb��	�fޣ"�e?ŘP�G��	��I�䷐� 8'��HiT�eu7. ��e�jA�D?�M�qN88�bpn�p2��I�P��e�Kgp_��`r:���K�E��������p
P�;N��jLI�$�L���4�L2����&�<Ŕ���q��'(�b���B�W���a��دb�cѩ�� zB �c9�Q��P>4�V��������:5%fg�N�O����h�PW��T�y>x�6�4͂���խO�T��z�A&�NL�m`�L'�B�(�m�U*u�����	��$IV�2fr8��3�-�iJp���9���N���"ic�3s^FL�T�3��<���F���ߒt��bV�������!ZFL�Ĝ`���[V�X���;k֙/�߾�/��-��;i�U��g�?�G����Fs�sH3�q2Ǹ�Q��_�V�]u����Y�w�g
���xn�n,��Y��*�PI����^϶�x�+k�����^t����;[v�)g�\u���З��P�;\�����C9����ɪq��X8no4N�@)� ��A;;5�b�d�q�	3#i��D ��@g:c���6`a3�+��٣O-�.=2��S�-5���%ks�YIq�=r�[5�#�r���n���e"��gu��͜Y�O���{n/r���j��E��;,�#���bP�1�U�����t0��Q�۵|":=:]f=�d5kA�K9�q�-�sԆ��j���V6��0ή"
��gM޴����A���i���FT2�n�nP2w���uE[o/ڴj�A��"�qkm�@M|�g�֍����{��[j�iY0�\'M*�,X֚��jh�ЪLܹ�XW��f7�F�A�:������U˳������7'�̼6L�U��&FDG�rj�V�de6o&�P�I �a��e��0�9�xD�����3��м�tA�]�'��'z�Hѷ_,3|��E������9Q= �B��-O%���qP�ۙ"����TY�D��h�ǀ5a��4%4�:��BK���$a�9%/O�gR�˚<�	��s�Й���@�-*��FZ��j4�z9훢|���j�Z]���r��`Q�.��EqJ�A���(\A�B�U���3�����^��\��J�w��>�l��L-r'� ��Fq��m3ceN�%��R��r�D$�Nkܯd�֘�A�j����vo�q�B����8����ϳ2��hU��>���+-:�E��ݴDc�**YGy������!�E�+~�uJ��d�f�0v��o!���
d$r�\��:3����^/M�����ɴ4Q̤�/cR�^p�j�PjA&`]��M@���jP�@#�6&֭)򽮴ەXݾ�!V��t$��ڿoH�/=3YT�dX��W���95yv���X��P�y�[�9����Y���'���'c�I&��������o�)���b4<�!}�I@Y(ƭ5��K2�dD�� 5��xA�a��M	��������K�3k�r���;�9�����,.zlIoY�����C+�w���,�ׇ��#R_5�����DD�-&�V�ٜ���__?p|S�-��)t�<i���u/!�T�iR�g�d�����X���?/�{���^96�߽?���mϞ��Pw*q�c�~FV9������׾��q8kS>X;%����)���Y&�N�<)��O �U��2}�w5/��0)���!�&��Z�����޹�,�\��k8?��Z�;��t8�9�NN�%��^��z���Y�T�TjE�VvR�62��>��b�<%pX�[9��=�·��i	]����E��߷�|�1Y� xАa�c�l���O�D�?��4M�8�,Ǥ��W��B���B� #
'T@ʜ1Zp�sjn�M�ȯ�'��Č"�v�n����n�����ɾ�B��+ռ\nohlr�9q��n䕝�C�9�0��2P�/��T熯mγXpI����&�E���K�?����Za��5� ��A��)�;� �菾p���q�"��j����[����"q��0JNd<~�5'�?���[�����bqǔD�/Px��#>���䰬��ȷ�r����F9��!�rʃ�酵rQ�ے�F�t��x#)� y������~������Jg�P���Q��	���L��r�Fg�HY_����_��j��E�Q,8S��W��/ �,P�̯��[<~� �/����63ˆ�����xvf"q�v
����If��8�["̢^�+����Qv�H*�3-��?	.|�N��{l:��\��mO�'Ś���)����L����K2H�g_Ġ*���pC��D«��.��v�Vq�AϦUj#�����Z���S��]����*�ݢc��%�]_����'�u��$�����n�b��y������>pF�'��8�ή�Τ^D����#Q\����#x}8�������ƍƌdz����HM�٦����:�r�ʿ��I�cKrW2%�(�j�A�����p*���躺�����?��k�����V��D�H9�\�g?`���(�X.�%yIb�T8&�7X�Oõ��2��=/����ܢ�/����]���꺻��h������h*��Ň���z��=�Py����O�,���dk��]%U{�K
v�9� ��9'7����Ա�,+�����?I4珊D�B!w�ڻ:N�Y�5�;2�e��Pm�1M��w�V�S���њ�c�8�,���l�&��*�M*��`/���\LS�ԙ������ԓ˖�K�?�C:8o.��KDts���H�ya;��h5F��nW�f�qW:�{"�k�Óc��v��LY��/[�cN:-�K�$VgE���.�b]~��4M)s�/���l�ޏ-I���Ȭ��p��ND�$v-�rf:3SH�z!��BD)����5]`�lO��n�ӸO�Q���mH_[���X��$bIbEwA��ȑX���Q���j{Y܊�T���+l�tU�Y��;z:��px��x�5\!��e��0��L)���Ҍ������e�*�TF�B������[M	y�����ҵd�� aH�&�j̔Aɨ&�}��űw�P�%��.�5@�v)�P�J�[,�:��u�|��D�)������K�CFХF�Q?%5F�؀��ͅd�haN����`�D�2Z��huV�����R��`ˌ!̢EJ�No���\PE��D¸k���` ���h[CN�ǐSC�'��c�y��ǐ���e�n��-ӇÖ�a����%j�N	T�^%!��<f�*
#0&��$���5J�0��F	5A��Ư߸Dt�N�n	L��/L .��J� AK��2�T�	M��~���J��{{�C���;'K��쬊�}�I1-��V���Da���R����s�F��J��!����Y�Q�xdp�tN���;��>�K���W�$�8m���h-ZJ�4A=̘9�bc�y�?�^��L�#b49�i�!��pTV�ԑ#_,�:�k8�!8� 2Wg�fD�~�ά,n�������`A�����@d�.�k�&y�VTnmH(�v��!S�����	�$���ܝ�qpE�����U�uk*�(�I�,+���TŔ��n��i�o+��+�H�2���Ij���%�"�������Y��YvJ@�h�;cϲ{�Q{��M��8}5�YZOSb�={L�:����,;æ�:k����Q���B�ɍy����c��F�/�#�	��`����uw&T�U�ǪS*�r����_\5ձ�����g��r�Wջj鉶�9�fǙ�jg~���S�,�Tf~�T��;��κ�m*�%Wg�v���n<B탽�p�P��l aWz fW̑Z����G��I)���T8l1�=�ȦG�̣|j�;�*�W�T��'F�jkO�	�<���	G��,���������[��#�ֵ�t>ܛQ��-IM���0�RK�����8+�k˒�DC>.�*���Hm�/��{�����f�J+�0F��'�v=ؕ�Lv����~�3w�ݍv�U���]�����4����.ݘ��T�ktse��x4���.��ĚzNG�)�̣t� j�����8��"6<����#Y����Й�(�G8���,��">�p����ښ/ֶ���,DT�����#�PN�,V]�U�&�K�,l-��]�3הj�k�;?�m�O��Ҏ����c�3����ܲ�6��kfx�����*hL�Z�%W�dF{�}!��D,��p�lkIeDa2iP
����:wΪw��tv��/3|g�^�Qک�~){{�e��2ޑ<jo�Y�}��^]��3'�b����� B�ߨ�D�T�W�`>j�:��g��B���NΌyA|�S���f�Ƭ˕�G���a���p�)j�w�0�{3>�0��
�؍��D����M�
H�}�B�Q�;oXA�U�P��2
3V�kX�\sj�ɱz��Κ����3L�����Ѥ��Qq���3��B����d'�!�~��\�9�Rz}����5,����-�'��Z�Il�Y]��.�H*fT�+���H��<��d��dsYg���Bň��C��.We�5*�S�$�����I��-�9�Vn6�|p�F�L&Hn4�dQt��n�7��#�ե=�|NCrq{�'ج�Ao�8Qv�r�tT�+�H:����O""Q�G���$Rq*=tXo�az��
S�U�Xj�Sd&��rJ�o����ZE¨�&sTQ��-�����!0���5W��s��B��d>R��a�Lr�/m_��y����� <\�A�7�� 	� ^ A�)R�d���˺I��|�w|(U�c[t�i'��6k(�S'�l6մ�fe'��;ݩ]7�ݮ��d���&�E�����$H�r+I ����~����+V�T�^Au��/����?;������w��ЛA��kҕ��������K���0Ge#z|Ͼ��dG�p^�k���<�h!�X�޳�O5c�EЋ�o����cr�х�mF&;L��1���t		��⒣��a��$�)/X�C�M)�0�ߛ���;����o*u�/)�@�罚�v��`���ez>s��R���h�𳳡�/~a��!/��n��3w�f�*Z�2�ѨK?��}Ko<�?��<�>����|���͏�s��v!�/a�q�σ�$����S�`ڃz�Br��-�r�<���8�`�Wf7�����V�*c��R�@�V��Á�G��p�ާ�[��AX-��Q����qtf�Ґ6��!�F�;g��K:��_�I�S�b�=�ԴW�by=`5����r2�i	��h1��,'����K�@O�Y\�!�ㅻF��Z#C���@�%����ӕ M\q9V��]7�P)��TS��	�(��a���2���o0�z1�#h��hi1�i�^�l�5J�SV��zj���JB��
�I�"�����B�b�BF4vͦ7����/Ly	��d2��}J�(�"�h��#OdF�~���sss#����[��̃�9�R�S���z�k�م�
O����w��i�	1e�c�,�F<٩�qH�d|$��R=2Q��)5i���D�:���(�f��.m�W��������Dh�S3f�]�`~C�_#��%ez�S�����X��5e4Ѓ�YY�+l��C��+̻��?��l��}��y�Ua�f�yt�d��׀|܅�3��!画��S�� �DDV�sh*���κ�h!��-yN�K%ơq���O�%Ӓ�V��\T9�O�1�� ��_R��k0_��u1�s7x6ԫ�q�P�m�W�C$$� H�D�gD�!]�N���-��ǆ�i�|���\�W�j�/���ᜱ#W����%�Z#�Tte���۷}����ڠ"�kӔ�&o�l�1y0��ԡ�"7�];'�=%�ez�� ��{��,.LM���,x�@K���9 n������-<�=�s��@�X~6?�3�RyO>V0���C�l�`�g3[�ђm��9忚s�x�5��H)��Z&����ʔ:剮͍V�-(Ѹ9-[f��qN�9��f� �Zdҙ�"�3��I{1k�̍���M���;d2��E��=%6Ahx���oo��o��5�=��/�j��`�д�TӚ-�����taJ �4m�L�S�6�L��6�L��}�
I/h��Qh8A��2`�DB�����9�P�mb"|���4�t��n)[��L��ߝ�/�"��ӅC3�]�\6lO�s�|��m��Z�r����	��PlK����c�7�L�X·��F�YDw���$ؼ��BN@i�Z ���-Ӱ��r|C=�$?�:`i�+�:0�.N�/S�b�x�~]�i�e��]�텃C�Ӆ���|O.��ܜ��ly���T�h�nįWl���k���Sh�'�����%�I�6�a	�E�+S*� Nv���(�K��\IC��9�\	y�-���N�CV����}J��C�Ƌȣ��'_���\���p($x����_���(�/t��0sٞ�K~1�p�V�g�x��®lWt��=חo�{r�?��P:]��Ug(Z�N��� ܶè�Z5�j�P��eMԮ�n�c��7�K�I�'��L�^�A�yN��/��V�[G�x��U�� 	H{�j���Lq��*��Ѭ��rI�1G�Mdū�[R7�����V�z�W]1��}� �d��+�}�d�SS����$�E�]'��s�b6�u ���\nO!����
�\2_�NU�+I�, J��L`ݾ���-�K`
�S�־�x���]Q���gŪʶL03R<�r!-�q�]	�(��xan,�K�::�p�24�p!G��AH @:��\��
�@S�����
S�O���C�*J�~@�� ����ŵ\����yi����2/>�I�I���Ֆ��/o��_*�ww���7�rҝ[�c		Qqo�Y�l&��c7}33�ʢi���'���
��A�C��s'rKK͇;��sÅ��f��Мs�u��ty[1^��R�PSD_�8TZ^o=H���g#��B��� ����E��3�%̭B��%������.��GXV�5L�����H-�[k��U҈Z��l+���6�kԿ����G�d
��Z1��Ҍ�b�1,j��a.d	a��?���]�.��
�0C��K�FLo�or�Ӽ*&җ�mE3���	:���f:���]Šg�*F��#n����k�]�����>��`�Vj9M�]����˚����Wf@B���'�S��Vڂ�,J�kW9`��(��q�r��7f�m��� "f��[٧�M�^�G����4�/�w��>eәr�����X��_�?��\�w4������}n�7؈�j��/i�h1-NN���߹�{w�̼���b֊?bB����jru-No�`��;:�NM5{zw%\=�-f�H�g��3��:,�M������푃K���������#yG2s���� ����d�q�DQ*��%#��=��Bxd��k(��R9R�H���W�M�zL�k�y1CE{�;"�Ct�in�M/�QQ�ã���+�]�IR)�:�a��q��&V�W*
Z%�h�Qg�_�q*��{Ҿ���Y��}"훔��PԾ"�'�{�8ҝ�z��h6�#Q>�ɆBhV���xPW��<�ŝ�����I�kP���;��(��2�'���/;�u%�-�=�]���<�#4F�=�@���E_dl�1�R �~�mqc���|(�@�Q����8�M�U�"�|�����d�!a�Z��K��E����u&ox�:�^fh7`���f��'W\���(�D�l#���E�,��Y��|����wŽ�Yqo<֌��K
6����ub���j�]�k�HY����]�6���U�U�"��	������K�a�?����A�7�6�Q� ��4��f����h�ƙ�mb��L�=�b�/����#=�p{.�7�@\j�M�;;;���D�[�'�J/6S}���Y��EI�G��ƤҪ�l֨���;~a0�{ ��N�f�;�٪�άS���T��ѝ���@�PH4,_� �8қ	�1m�	Qh5�&��� 9���X��ݕZ4tV�$�>{o�b�?�b����Y�5�&vA�V��g2����=�a�Yv�|֌5�o��֠�(�~J���32TN��1Ge�KpH���7T#�W��F�����r#��v�������u��+�[��5�cɇ�ٳ�&ώ��?�_�x����.-�x�ɑ��.=0v�x�����_:��q��h��G`=��-������Չ����PGIfC3[�߶Y3��^�:�l�˖8��^���S!
�x�T0����{�!57���<�ӈ���?x|H�NzW�e���S�\O��A���z&��S}����%��VľFOF�U��s&��ЈW����y���h��,��ڝ�z2'���V0���v��ܒJ{�K���+eU8}6� |^�W�rFnHLw���F��r*�S�2VN��y�7��\s9��yz��0f��c(�������8g.�?�F"�6�U����6lB]:[�cG�o`}��$��^_K�-���~�}n4:=�6�ޙ���m�ީ�S�~��	g�_O���X��e8��}�;������=�UL:�Q�S��E�O�!�+�5��3C����U҂I�I{SW���,aDN�'��s�����{����'�*��d�C�Z�Jۋ�=��=���d�g�eG_����oz���,dǞT���R�b\lMR�
���E#��F�]�����ҺX�	[�~K�k��6ɩ�)���F���>)�$=iI�B�	��J�C{k��㽖�N'���������*�W2���W��Q�a��Дښ�֙M�F|��k�j2+���gl��dG����p��dF\�42�+\4 
^�)�@ Q�m�z�����W�QQ0�K9��kP���^�,Fg���Ӑ�^�V`{�i��O�IHՊMR	�-RIq��?�p��T�i{oo�`�8n��H�	n�]�s{�R	�xaW�� ��%'������X@���$�R�X���|)�ؤ�u�]#��5x����YN�-b�^�a.��jP�B�R��GL�u�&A��[�c|�&��1iü�"Ec�YAo"�͛���T�D��$k�w����:�^��ZC�"����BgkNW��ɥ�k�X�Qq�
f}�և�� �6��w� t�|�3CQZ#�=��!d:��*���]EE��nHkh���:;�ñalx�2Th�i��g�Z�v����
,�����J���������A�����KչN��TfO}��_J�G�J��׏:�M�@%�
��Qt�B����C�2ޫ����t��"������Ž���}g�xS���w���+�־I9���S�:��z,w:��3�-�v��
]c�<	긔L��BK囚_I�>�;��,޲��0T)ҙ����_����������H�l�����=�v��;7т[T�Z~�5@�zPF�z\���a�z,�Z��u�3���l+�S��=����žf��1O��{r͹P��k�ڷ!�'G�� ��\b]�I���%����-�%���O�l@PWL;��첶�c��-6�v �3���&dg&i�9�pЎ���x�ϣOQ�
�Q(�]������A9\��m���9XS��WN��6�'���ĕ�2�畮�&�)ܡ��7|E1t��pNcT�M�q�R�`R�<Ќa���TF��H���h��/�@
�|ޣR|�ɯH`N�Q�)��͏�]`���s�=)��M�`
�(�B�!t؂&-�W�+рM��Qij��F�]�������`/����y���7M<n��yy�����l�����͖O�IcK�)k�CU��}t� ^%�5hm����]�"qx����c��ư+��,Z[�H��7���-FK'�)�W�kxO��@V�
��U�Į�d7�UXi%YE��%T�>*a Y�^��ٞ�5�o��Kv9*�+���61��KQ�#JL�2͙0�ɖ32�3JlCFK5���n��J���r��Fc���]w4������I�KӠ��Ge��}���D�!P�T[tZ#G��)�����^\I�K��V�J9N0j�p汫XiD� Z���i}�
7�VTڋ���,��yٿض̜Ɨ�V~�4�����Q����j�R{8k��:�\�)���>�sP�l�;#�����u��j5��um��� ��˰IoPu��a����4z�d���hk	>�0����Mpug��.Pċ�!�Lc��h}+���(����Ɠʥ�2y�\�H�T*j����Ug�Td�K%
�=3�m��#F�̩Y��b�ܓ����ʕ
&ZÓʭ�7��-���^��nw�a���q_d�с@����y"��R7�b HL�Y�Ya�ˍ�e�"���_�O��Y#ީ�^��S��"0xG�L�Os;���� crJFR�!�����!/C���	����\�i�b�Eqé89���㱣G�=�RQ� Wz���hE�2��u	��m'�rܿ,,Ɨ��(Ic��[�[=��Ψ�pdlW��&{f.�����ty$<t��Jg{(����y8����.n�h'A8���N$�#��X�;�	W1V�h�� ���bV����L�r$�X�/j�������=u�����ʡA����l�Q��mx���w02Z�Gb_~�;t��_��(1>���A6���f#�n��((9�������������!�(��2Ո$!�V�^�l9vʹ,,�-K�y1f�.��߈_���UR<�^GH���^��1��#Ȟ`�$��x ��4�M"CQ!����>��a��������j 
=d��:�>�-r���iA?�f ���K� 3��������W�C��1w���؊yz�C\L��'���UOr�=s1����ly�R���z�t��ݿ�y+��������K�Eoؼ���h
f8ٔ�vtM���
��ʵX��#��Ɛ��p6�G�T��_b�W�zlQ��������ɯ�;Ǜ��j��yCKcd8nR)�Fo�(L�N:�}�F���6e���0��ᄙ"u�h��:�ں����	�#mMƀGm�@�8�����	d�~͊�]A,j�A�Zt�4,�K�͸����	�n��˲Z�H���#��*�Mal6�1�,`�X��gb��e���ǵF0rwO�CM<����o�]�W,�
c���_�����p]m5�p$S]���%��l�x����� �0�����_e̲ ��C�_v�+���0�
"����Iu�j���T|^����,K�F|3q}=nW	F�W�z%%����Tl[�i�>���:h��G��Tr�u �u�;b+R�jqGl#tﭿ��Vߨ�����E�2֤k%ҥ^�dW�r�}9�ؕ�K��J�[�������1$R��ʛY��K��˝��V�T�w�"�:�E����N[�Cj,��\�?���2�-��f��ĺ�'��Rq��	NNv�%MR.���By��M*i�j�uV���HyE=i�d��@g1�s��k5hr�5��̆�/S��W�~N�YA�+�D�R�sk�w��s���l�D��Y:6�:��D�a�ܵ�]��d�7zb���DS`��ph��io���R�#b4��p-0kΘm1fE�1=ŘX�F���l% ����:�:]��1 K��6EB����p�,��uU����&��.��:�~���ѷ�o!� �x��E;K�$�U1vve�N�Z11/��*�uA�T�%�qE��.�D�4��
�B��-os�j���*�AF���q��͙Îկ��~�# �FF�bw��~ �1'�V���&θ�p���ɋ��Yd.����u�͞��<I�k�¥�,�g׎�5����Sc!Nb(�̈́�ɱhc���% �����������&��A���,�ky��6|�Vw* �9��A�I*xE���3��]b����7�8�f���[ѪXۼo��.k�CbT��b�Wޫ���3k ���oR2�٪��♲..�Gˮ�9H�$H�-CS���3������P��)5�Cp�.`5��:�g�h��C�p���´�XB$�GؼW��;䁟('� ^�/c&pQ ��+\�7h�&^�] _
�}�^I�@�K`XٍUZN��lx���=��^�E�#d\gX�W�u���;vG����+��S'���u�Z�\�v�p���T�ri�Ze��XX[X��櫪��ϫt
���	F�LZ�I-CB�%L�6juF��?I�B'-�<�=�R�C�~	�Ei��x9+�.��]=S�B���@Q֫�#ȠX�	��Id���2cXqrA�2#X�rՂ|	?��7�Nթ�ݷ��^�V�5���&�n�%�? Y�V+�����K�k�8a#�iN�:f�Tr��euC_�d ����_{�X�C�o �Sr�|��^�ҥ�$4V�CZ��v��h���y�D#x�{ּ���^��r¯՟����_�����֛�G�"w��#�3���]�=����<M���z׃W���:#�;d逳�T��:����:���Ђ��a2: v���A�b�����a�G��������z/X���ߠ��N��N�k�����:��k���U=�h�Ҥ�6()�n��jTӨ������JN�r%�����`t�C<NpUWRg��Α����`�.c�T!�k�kb]w��V���3 �ɮ�i��dr��k�^���Q�yCX�z�_�O;���e�f_%q�k)��x���T��$��y~�V ��b�����ܼy�_���U�F᧑y�K�ߍ�~�n�p�ˬ�r�4{�<<*t-�¶�z��{F�c�HN�PjX� W�5Z��^}a�S>�X�"��meZ�o�s
q"g��G臄�3�*�6��ثi�ٍ�G,�v��%%�m�R �~(�%�嫿e��hg<+~,,1�	s���fp��+�J��K��P�p�WW��D��x�L��O��'&�}���D���v7��ttG�-���i�)	��x��@t�R�ؚ�t��3�*���3��YT����z+�N���#K;�Y��F�b��8=4|b������	�`������Ύ�C�8ǃa���~�c�w�ٮ���3���v���1�
���s�]we��n��w�@wx���	yHNP3���z����R��v#��Otx0A�]�m�U͂Ҡո�� �x�=�]e;�2��<;���V"�"�pԍ�e(R�Ѩm-�J�������q��GZ%�����c��=�4kop�0�x����)/�����4�;՝j!���T�[v;��>��ۤ���8�Y�١N_���v.e�?J)X�,/�	�>8�,NL�׾�d�Lp��T�����զdU�*8����u���Ws˹]�*�T��� ���o��>:���{k��ި�o�xAۗ?Vj �2
����=%��%u����1VP)Ԝ�xXi��C��n�׈\�2���s�V���Bi��d���,Mˡ���7+��1�4xW:����]��KqrQ\O��e}�H.�:��P ��P�擣{b��1��m,ܵ�����G�f�\�W���lmi���X�GS �n���'b��,P�Y^�*mf���+qGX������2�E`+�x2���V5�CV��Z����+�๪�j��C���RƐ7�&I�{��X�ap8�5˪���p:]V���s�����(�P�Ɉ�0C��>��8�͛�1�k#����V,?) I���'5˱�Cg�񸖸��c���֚-6�npE�A�&I�_����(K���� JZ��V��J�ah��;;� �alrzixs��sp,!���n�:�������g��`�����.�B.�]��*ow[j���Ų�lE�hƄ~b�5�6ey5��j~����?�G���k�4�k���������c�8�zp�����v��;�={J߁�;��֖G=y�����rz��̀�@qeb�-I��&�"Q�9<����6�F�4�G:k�zR�v��E����
��ٰ�o���t?�O8M ��_A�o"��8��س�U,O~~+[�[�#��ǆ%��X-�W-���|�h
�:>�\�E��kJp��r�\�9�I�S*-��:h����ИL� ���XB�y�aE�7CXA��l��lΝmwg��v\i|Q�_�Z��K��/���}�Ɩ!Ar=��ʇ�y��R�V,�͝��ƒv��K9Z�B�wZ��q��]���}	'�j�]rM������c�l�Uݲ�ˡШi9P�Y+h��-nL4˳�H�K'N Vq2ΠUZ���!%�7�� 
��/bϕXx^d�gy�|���z����"�A'�
�c%!2�ga������q�����%�M��''I��iҴI����5MOҽP�6�n$-�"�&iH����
�TQDE��Ȧ����"*x/ *�����,₀��̜��e�{?����/Mf�����9��"�/�<Y~�q�'Ə�Bl$6���Z��M�'k4���ਅXp^^:����6}Ax�'ܗ�x3!�]q_�v7=
�������H8o�hc�f���;g�o��3?&��/N(V&����̡��E��
�o�s�Æ�,*��M�'<ͅ�KT�$�֪�Uq�%���259q<�X(�E�C�B��%�i��l�88X�g���E,�\0�yqpT��Aq9��Vtz>O��,�v�aE��9�E�����Zd��b��5��%1���XvV��J���l&*���Ͳ!�dl3;�M��l�T�6�6�2�kR��Ȓ5E$3~'))7bM�(wM0�e�\���!��@��N�מ|/�y�9*���ma��2�&-�;�$��5��׾ɩ#F��{���A�5)zcl�����G`��>�ʘ��V3|wrjUõ��Lm�	"\�ak�V[?wI�?˜��H,��43,Z����#��X'�ӋaQ��Q�1=��N0�/��#t1���)[}�dNNd�5xΤ8�Q��+�H�#R9��*0�Mx��_������T��ɩaF4;.%#.���$#�#)���?4�P��
��f�L:���#1e)KY"[�\��HO�	{8}	��������8��\7f}_����&�7qy�%����U������b���K��SxWY�CSU#�x<.�"<�ˍ��s��?��R�P��d�ዖ�b��KӍb��Ǳ0,�`�$ѽ������ϝ����Z�1m���m\ܒ75Ic��޵�)����R"�7�$��y�J��Y�V-���?ć��p�ÂY�X8�DfO�l��w��پ3j�e��q��i����2�}�#	"2:�1��� ��-,*�oO ���d�p�/A�%��+�{�dio����Ą���uΰ�k>��pC�����_L��/!׾n�
��W�3��1�o�~�����v3
��5%v���^�������a��\�Ԣh�%��!<P��G���?G��tRg��D>�A�V����y��H�,��ُ�M�$b��d �Dd��Ž��gA������.�w��-����u�H
:I$�#R�$G��JbV��H��}�XQ�1�̾��BV��)����BT��P��6���q���C���������1Q��i�����SJ%�^VJ��W�1��5;������ׅރB��� �P�I�������cd��)����H0QJ�e����X�T�S]�`0�	q�����8Ʀ�&E~GA�x���� �w��M�t�s�J,�V��ه�`[���A	�`�E
�6��6������gQ����||Nz���3F�)&G6�ujQ�8Ed�<f�J����3Ȇ{#�Y�0��"�������eڈ���U�&����2�%�zSC�S���2�2L�ӓVxʞ��:ݢ��y��dE��Dg}S��܍r��TJ���ƈT�4$�Y�V�2�����<fDFA�>{mV�85i|�=~�Vկ'�D����8�6�n�L\=d�^l�0!��w~�����g����҄�e	�yH�2,$�G�`��D泓�/�"��WU����ov�����o�ɺI�F[�x����$C[���r�^b�V9��¦���ʻb�T�|6[̍��RUQ�dl*b�R�4$^�knDx��+K23R�)krcx�[,��#ĥ�̂�Ԃzx�����R�(�0Vo�H�
�����4T�ߺ�6����a�|R 8���`�@ 3�	ȸ+�e���y��� ר0}iZZP*����ӹ�*U�*���F/��i1fpvR�f0�Js�B�Y92����c*o\���R��t��M������0_`'�d$���+Y���s� 2"�mcl���ɂ��w[�`�$5��� �����k���BD<�(�\�������ǹ։?�����#���5��"7˶1��L�(Ƙ�j����o_ ~���-U�Oe8]�ܾ�����+A��X$���3t�IxD��;X!�<�$��v���X<�u/���6�c�0�7��zp.a����f���B3�#��`1 6Sy�z�=٦P������p��<5���"�T���9n<Y!{�i��,�ZFƄ�AI%ʤ .�%�{G�d�45/��˻�|0�Y?,*B,�����%�E�#3KǪR"�B�-�E�BVC�:wxAgvN�6�lf���#�6�zl<�l�p��

p�^��Yb�T� �H*��1�p��=	�z�+:l-.aZ�j,sCe���R�0
D�&#r���J��|s���X�Tn�4TA��� '.kXzc�S-W(�J.Ɋ�g'Ŋ�,~P�rH���\��1�����"1�
T	�k��U�H� U�,=><q��	E\�$,�κ�Bylt������c��a8����`�ˁ�,"/QA-<Y�����qI[�D����=��k�3X"�~�>�n	��N�M�!�_�!!«��+{�H0�oF�m0�����V�ŷ�^�Z��?��0�;����CP�-�"�w8�4�ǹ�^�A�x >��7`�.���a�-�3��B��ɷ�3�B*o��[A8\�Ӈ�9�1����7�2�%�+">��l�T���������{2�=��،��@$~x
�-��$���!��4��a���4^M��@.|*�Ô����͑&L{ۇ��[`Q�Ռ����a�S���i�-�-Y�Y�Y;R�8�N��li�CٟgN����ve�Tʦ �7��~���/C���߀�M�V�m��p4�h�} ��ϫ�g V������p��G`)��[�L��B�-�R�KE�0�1��U8�G(�XW�M�7C�چ�E�04h���CזĔ<Z�⎂  �  �  �  �  �  � ��p���d�%7`¡  �  �  � �g�a�
�����?\�~{�X4�B�qL���e&F�^������"�:��[�2���O��X�E�y���e>���F��(�.��ep��r�0����B�*���_�1�ҭt����>��8Ƒ��L,R�.]f��ac����rh?K�9�����"���e&�̣�|FC��.�̨Et9��z�.�p�Q���'��e���+��2O�0$c�x@�0�W�L��*S��ʔ/�2˯��L��*S��ʔ/�2��L��*S��ʔ/�r�02QH�)_Pe&��+�����_%�
@��`F̉�1�i�ܠ�����@��l�\)ì �m�X��B53x7��S��	��4��Z�X'�Q���F���P�t;G+(�"I�c}����Wf��o� �#�@������4��d��pPk��j���է�[��[�ӂ�@`����V�B):vZSq� W�H_�u;��N��z����ކ�j1-�	Zǂ�!�A��Q3�xB+��+AK��K�v����Ox����;]�
7F@1��܌�PQހ�n�[�&��./�|@����Y~��L�̀� �̈́t��'#{���ύ�ڊ�@WOo��vP��`A�P�E=?��c�Hb��@�K��X5�
�m�\�4��5 :,([@k;��j���F~r�͠lEܨȅ����VڒU7ҙ�iC�&6z�A?j��Z@��da�k�}eA:S�r��q��EE4���n�piGcۍ�HIi-�+EӅ,� rt ]���=%�E��6:��T��4"�-Hc�7�)�Q\(?�h���݌z�$��Zm���z2�+n�g��Z;�Ѕ��A�c{{��FG�Ŋ�����f�k��A�6���t�ÿLyh��K#p����˓��@�o��+��܀c1�Љ�@1�?�)�|��]�C���]�RlA����OՊ�����C�f9ԅl�@��F��>胱(�)�w!{Q#���F��+�H�:����t��Z򉍶E�L�t���}-HCJ�f$���3�����<�--^�j��ƈ	��M�Ej���ʽ|j@�T'�����٬��Ԃf+����t���=�8K���e؛S�d�Wm럿�\G��ʍ<g�5j����PJ*wz�r�7�P&���d���T��E5���+�U�@ㅚ?MhT[蹏�{ZQf�u�R��u�����6��,���#��fZO��X�Tˑg�l��6��#!m@^0�uD'ʯ�}�Uh�jE����MӜ0`nO�G�/[�r�G�f��'W+D� 5D�7�'�6�O���r��^����v+0OT�z=��9.�U4�o*
�4/*k�h�ˑ�Nzu�Y�R�D+�gOSq��;Z����H1`�U��|����B�;�����&z�镇�꿦�����&-�}����P��t?���K���O��|k<O�g7��������h�d��G.�3�o��f"��g�
פ���/Bh5jE���7�RR7#Y��L����.�|�M{܅F��+�g\���?oU�������>Kt";���~��h�MY��'�	�B�>�L=�~s��6����&��g�+����e��?���e�W��y�f9��].�+(_5�z�|�5�£N��.�6D�E7>�����4�]�Ǫ@m4�-u�E��Eu��(P����%����S��F�yH�5�9����u�>�*�@uX��Z�^56�Pjz�S�hׂ����;*@K��r5ʂ�:p�����DJ�F�Nx5�/�q�HVj:@_C_-�����B�:��U��e�F�2�Y$�A5���@?=�_�t���C:T��.j$䬠u��A����@A�j |Z�!h�4>�U�� 9�_�6���Y�4�#�i�AmkPͧ�
��*�A%(ׂ�j��t蕒E�G���F��^�~e�k�\=�Qި@�F�+xUN�R���u4�D5�U�4�{#�
E/%�':)�~�P��o�e�D5q�1BQ�\o�=}�]��ːM�\z/�[Q�c����{��F�~~B}�@�6m-�"�D��贻�-n���t؝��nSeV+�����]���2;��M����i�$�f[c��L���n�jo�	����w�2�C���9�3Xm��`3ڍ�A�p{���t�\�Oc��EX��؝D���j1���������4�	(n��i&:l&��p���Zm#Qc1�m.��e6��f��d6V��0�]F���C<Lf��bu)�(&*VK��5t����9��t�[�*�����{:���i0���Ʉ����k�V������v��f1�5�4�+�@T;������@'�����8--D���b�":-�6���춚	@�f��Z�]@W���i3N�WAh�D����p�]��iqF��p��k�(�[�;�n���u�������F\��i�[��N�(NX�F7a�nh ���x5�-��0��m��7[&�󦺈v���0v�����?;@���j6��p���
Z\�{@w�(4�d @�S����m'��T��ݎ�����NE��
`�lw����48ں��������Z;�j��|�su8V�"xMA��w ٻ�On���dFv����r�h�L�pZ�U#�b�`P����vr�]HgOl���NO�r��K�#��[c*�W��0 ��l���$�L-6����v�Y�%�#~���IK)u�.��b�B�� E���d�4���'�a���f�L��g�L|ԱV��� )�d�j�>mf���EA�QDu����m�4[�0Y�4�[�0n�ȴ��D��d�ۼi��4:�6E�e��a6Y
��5ֲA�	t�I�Ea����yF�Y&���Q{|�<�t��QmY��΄��5CB�s\(3��	��.��2&9���0$Z�����V���v��2�ŀ��'���P ��e7Z0>Lv#H6��J�+�L��O[BO��ӑD&��(?ܴ�x��/��t�A�=���oH�IM[�DPC9̪��nFqt �\mh���p�`#%@�l������a�r�-E�<`I��H��6{�mt�à�i�d3$�$���	0_��7Y��+�B��l�j��}mv72TZ��Ø����f�fs��k�S�	ٻ� �,�E�9�v��M�&��U���tjB�'t�����J"�L�rb��QS��H����ƱD}QV7������1:�^O��mmC�Vڴu5M�ںj��WW&y-��hc=Ҥ�j=$V��Uh@��\[�m+'���u�f ZF4���M5e:��I�P�W���l���J��k�u�`�m�z�zMYMbU���!�*���՚FBS_S���j YYy��b���)��ʉʲڲj5��Pѡn�t�5j��������:�FE}]�T�@K]����Z�ZN��zh�*]= �	�GD�}uj�
45��#��7��>Y*�e5�����Y�g�P4_f��-��P\�i�]��.�?a��.�n��~;�3w2(�v3��݌�n��l��迣�N`W#������oW�O����]�����;�-	�d����XU���2��YNTt9�r��i�,GHɉ��v�k�>� ?q���p�]�dwܪ ^�l��K!��;n!hzg0����2�h6F���A�� g���ɑ�ܯ%ve����,�z���h��; �D?b�pftdI�ú��!����~�r����H3���Av3�-�GL� �nS��X�̍H��d�WZ�Չ�d6��$x�^)!Ű�G\m[��nS�H!l�H8:���n3)��X�D�tH�H&��LI��z��ݜ�w�DCE/Q�Ed�� �0/�NP-����6�G$!�@ª�o�)S��T5�Vaq�ς+�jB��+.W�dU�TeU�+T��� J�؛j��>Q'�I�f�1f7#�|����V�{y�ܘ�K�^�Α�עg��UZ��Rv)�[��.�UM��dÛ_\�攩�%��}p���?f�[ٰ���o�>�q/�cx&h�槺f�J�|�t�����,��٦g�]��h�����.��nY���mϽ�8�+�>�������܋M��lbܠĹ_�c��K�0�ɽQ�z�OI������=w��,.>��������~����N�?�>q��֦.,�8U]��+>�m������2˝����z�����~�)�����u檦W���g�q����a�q��qB���ifW�����*|=e�ɣ\�+���A�HRz����>�U9�gK�N��1s�����d#���%G�����ճ+���N���-�5��qe{�����R��c��``���UC'5�:��z�O���6�ݤ�;�L�=$���	�$�۰�3��۱eL�����HӞ5L{Ǵ�|RŒ��W7�vo�j��y�o�\[pb�/[/%�x�:�3��(:|ɰv��g�!��GR�?�<�Ф=��ϟ�:8Q3��rK��3���ce��cq�s��nmX�{�7�ev?�Ys��S�W�:(L,}�)><�UD���H�f=޳4�g���޾oӹ��wu�WC��/=��N��=߱e���W�+��5���5�)�_(�ds���u���[�B���\�)��ɣTj��"�ƹc��ϺW���z��:���?>Fvq@;O�1�Aږ��W���5�>oA�ܧ.�M�()�BE��5�Rf���8N��c�����b4��DY����`�"�\2G���!�@�R)Q5�������G����_�,Ș1Y�ն'�[2rP����G�%�����k^v���{Ρ�Eڅ1���3�L9�M�v���rB/	Y=��Mx?'yN�ş[c�O���ﾩ[��A�=�]Q���ʁ�嬕���>�z8��*���N�U)R_�]ߤ>Ŕ_����m�Oc��+�~�x㷉����#�O�����M�'��`ë[ĩ�-k��8h��|A\��~���M��K��a"�����zcgV㳯�O+Sv�}��!<�o������_e�K�x�2{ǻ���h���@�'a0���$��e��&8Gą�X �f�� =QG0`F��2Ӭ'�Y��.|�{b��ŧK~����_4����Ն�xxv��^������k\c~�Hf��J�Z��T/�X^6{؟O����[0��t��5dY���t������Tl-Z�Ȏq����g6��yl_��Z��{�]���u�ޚ�������ڛ���߯#$K��Szb���Y�Uc�KoL����0Νx�q>{�c�����-8��c�����/e?�<�dFr��꯿���Dr�s��fd]���|�-+��i�zo���qä=��Np�U��*�OU�d:��s�\�/��]�a���[d��=z�{y��Wm��͙������s�7����Ő�Å��e��Z�l�����C����m�c�������vm1����+�N��n�]ߞ�}A���+6������'W�q�m�{oʠ������ySs�������-�~W���"Ȗo���G�Z��x��L�7~��<���&%C�<���F���%�{���f�����&n������6L*��b�a�m����/V_�_{De}����ݲ�7nN�	/�ze��[N%�޼~�qӴF��2E�K�?7m�����t�IGR�j׶��y��/?����O���߽����K�}�`�.ˮ�m߽�x�2������Yq�J��a�&��ݒU��T S�e���9O�=a�hN��50'��I�id
50���̄�Ҋ6�AR�_�RR[�RY�R�E*%��Uy���+�tw�-����q`����{{?��=W�8��#}�"�n]�����D��������w]��?�Nc���ԚK�*�mb<�hƣ[-���!]�
�z�'����w����~�~י#S����o�ج�v��?�'*���0��X��'㭿Վ}�����/Ͽ�Z��9�����>k�Y~��WO7��m͚:�}��j������8jf���ߞ�zŅ�&ѩ�ߧ����![�>r1��ܣ��z�������u�ie=�-��+7L�a�O��/��ѣM��_��ׯ���]�v�������~`��Jt0\�z�j�o�b��'�v}r�Ո�^���޳��K��y����bX��yy��rI��%���T�2��<Ҙo.�*��Uf��*[������ܜ�B�9W�g"���56�����/�

�6���Ӂ/��������p�� ��k� �a<O�/YdAY����t�D�Y��@��<3�mX��`�l��؀�&�ʹ�K�O;_{�༉�o�X�h--o=��lI�ޏF�#']���_�=�2�7������jݚ�R�O�B��`-J���գ��/��E��߹o-#�T���=?vO߾�;r��|��!O�>����𞉮8:��O�Yy�-��|:xs�/���oź> �����ؑc����{��-2��*kt��O\�5��x��{��?�S�ٗ�i�u�.j�f�z��{��<Td^���������?�	�RW��Y<z��k�ڒ��>�su����g���5��-'�>0f���_+�,��?��u]?�xR����c�=�3��.j���}�nw���x������׶>��GMw4�\�p~p�e�������w3C�#������Җ����뻧�^�-���l]��ɿ��zgiN�9��������Б�g\�;�-��siENao�ǧ�V����[��9�A�>=n��1����.٪s�~1i�b��ۿ����'ψ�}����W+r&��b�<����ceM��}7$��e_��6
�1d�D2�!C�TS�z	S�2�D�},�n�i3	Y'j(�t���ů�~���������s����>���|�sT4��NK5��js��k������2�mD('P�5^�����ꓪ ��R����p}�?��'/:�� n������������	)��� _W�2K[���%tံ�W%��K�8���� ^�VΕ��P�9r�(`�U����l���jД���N]1�6��+�\'�]p�g^�%����^?�0̬7��{�D���J>"o*�����T��F�<���,��2�����򏜄��5&J7;<�������G�0�3�=���j_����Վ..�ٲ��䧥��M|��"��_C�PTC����ѝnҶ���1a/���C����Ǳ��⏍�i��T�$��d��`W��k[����JvCj�K�E;��ڊ4��x�ڼ-7w�'�_�eO>�;߅jLK?U�$����~��U��ۻ��q��2	��k���҇G��Kؔ���^Y�����X�}�;�]�_~4���*�r	4RwL�>m���;���#o��Gԉ�X�k��ż�P�7Z�͵H�<M<;k� JJ��g�(����_6g%�2�	�J���$������
��A�<��((�#�=�v�DǾy�Ej+׾��"s����!K�2�jB�����5����lη[�,��*<���b %mUP1X@��@�pC�X� ��Jq���x;� �[�)���ۥ����s؍L�t���S� ���[YR�/�-D&#��E V޲�����$��X8��<��8 v�`g�ϴ��~8�P��yHB��$\X}H��y"`�ws`�m�����G}p�;���������-)��$hh�!+6�����vųJ��a�U[\]Fr#�z�\��b?^L�w(��\.O����9f�q=x^R�V]�h����G��$w�A���+���>�r�EY�$R�+�C����!%�{�Π��?�mRW�}e(^�W%���r����at�<S(���xv��J����W�q��2>�g���"|��Cܲ3���3�v/t�kυ(��S�]ϙ��hV�-�!��ͩ�4�����]��o!��۹[�o��W<~*��h�?�,O]P^Vӗ	̮@��-���pp��B���B��<M���#��%���Y�D����]�aP�i�>����O�Ko��V��(��5��t�}S�)#�(�nNj��p��w���vqHݴ��z�Oc6��#s4@��G3����69�.�J5���2JJ����傘�6ҥ��	��K�A�Ȭ�3�b��:9xQ��J���W��gfxӁ�e��w�N���`�}��ξ��"G��
���3����UL39%�嘳��e�ٽ�Hwn�M�'|As}p��{(�_��o0"� Bn��؀����5�YR�к2���\�0��N�k%?��� �Vcm��U#�LG��qh&l�xX����s���������6d%�¿�����%e�a3��rǪ�
������u��������~�ۗ�N�4����R�O+t*"Bg�8��8:�-��;?����(�5T3�����p���i�����I;���G��6�_D.���Dӎh��[������!H+���c=,H^|�J���}c��=$�ݡ�F˓4�Hj�;�N5���rF=o=z�����Hz��ut9�u��tJ��]����l̤�B����ލC@�5\�1�Gp�? ]��ȧO��N?���⳨��B�k2eMO��'�a��%4����ٍ��u1�Dׇ$Ϛ���j|�lk֨�����&L����tD*����jL^ߎgd\z����:�J�β�v�˖��x�.�����k�S��z�*�yzF�Ɩ��R�ԩ�G��n�`��d\܁Ib����A��^KW� sJQf�?q�.gO��?��F�-�e�=g^�~�7��jŹ-���B��l�æ�U�w�?[�Kg�o���ZG:O����w�+6�����_	>��xW���.̃:��cw(R����+�?��%^�:�����%) �M�B�t�}�M�����/﶑�9���t�ęG��8��C�f���
�a�RN�$5yA��k
endstream
endobj
116 0 obj
[ 0[ 507]  3[ 226 563]  18[ 535]  24[ 607]  38[ 460]  44[ 619]  62[ 419]  69[ 638]  87[ 508]  90[ 532]  94[ 453]  100[ 483]  116[ 881]  258[ 471]  271[ 520 425]  282[ 520]  286[ 494]  296[ 299]  336[ 469]  346[ 520]  349[ 221]  364[ 441]  367[ 221]  373[ 791 520]  381[ 521]  393[ 520]  396[ 345]  400[ 387]  410[ 329]  448[ 440 699]  455[ 441]  842[ 326]  855[ 263]  884[ 498]  1005[ 507]  1007[ 507] ] 
endobj
117 0 obj
[ 226 326 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 507 0 507 0 0 0 0 0 0 263 0 0 0 0 0 0 563 0 535 607 0 460 0 619 0 0 0 419 0 638 0 508 0 532 453 483 0 0 881 0 0 0 0 0 0 0 0 0 471 520 425 520 494 299 469 520 221 0 441 221 791 520 521 520 0 345 387 329 0 440 699 0 441] 
endobj
118 0 obj
[ 226 0 0 0 0 0 0 0 0 0 0 0 0 306 267 0 0 0 507 0 0 0 0 0 0 0 276 0 0 0 0 0 0 0 561 0 0 0 0 637 0 0 0 0 423 874 659 0 532 0 563 473 495 0 0 0 0 0 0 0 0 0 0 0 0 494 537 418 537 503 316 474 537 246 0 480 246 813 537 538 0 0 355 399 347 537 473] 
endobj
119 0 obj
<</Filter/FlateDecode/Length 35394/Length1 102588>>
stream
x��	\������0̰�0��0;*�l��!��

��A(e�i�e��f�v[�F3L3�Z�nYݲ�^��Vn��-���s����u���>�y�y��<�9�����}aD` �ǃ���+v��6�# ͖���l�ւy9L���]�� ���Z�s�tL�K���\9���ֲG=_��:�bռ�S��y� '�PE�r�9�V,,9pQ3���緵�~��c{Zl�p>tۣ����|�����D��6�L�wz�ܖ��_9�_�۴�eI����,���օm�[޾3~
�Λ���hY��l�
`���dv�/Z0�J<��ܿ����G�_�}`|x,yy��a�} bC���/�{�����z��@2}U  	�S��n��Co\���Z&�Jn�̀�AK@Ěz�����W�RQ6A؁A��,��&-D�%xT��(���l���s�Yo{R��
V�C2��6�a�e�.y(?S���{~��΃տ��/�<��;��"�9��~X-��c�GV~��G�)�ʎ;��A�G��{D6Կ�N�����]���0�:2�s"z`��V��ޭ���_n�c��/�:�Oǯ���~�9~��Mp�ڸ�������8�C�q��#�| .>��dG���"~�~kY-�/6B��ʔˠF5�P*G��#�*a�!��Xœ�RA��a�v�=����'�wD�I��_l�-��M9bq����u�v�`��N��1���ǳ����q}����;f,��?g'"��G�+����=�.<�G��!Z��H����]��FM�u��~ί���,O���Dx
��$TB�DxJ�K/�
1�!hbKa���gA��t��� ��/��� ��:�ݿ}��o\��>�o�"(A	JPH�����-[ �}s
`�`�e��Q�k������DE������)��{�	���q��6@�	����'���O��'������?�/(A	�GdO��?�QK���w��U�S`��X"^K��wDQK�U�^��1��%(A	JP���%(A	��n���Cv|ߔ�5��A	JP���%(A	JP���%(�ua7��G��%(A	JP���%(A	JP���_�p��������@��?~$A	JP���%(A	JP���#"h<�\ք9L�SA��ߙ)+� S:H'䢥�^'�Th��g�x|,��4�K�.�ʪ�.�^h��, �e[�k�̡���4h�Ӡ�E��au��i�;Ds���>L*�+<%VC���W_ʇs�o���� �E�B�oP�����L� �(v�(�J��&��L`aL�★������d��v���b���jv{��b���Ʊ"V
�����G�ua��X�_�)��S��H�Ur�Q���T�P����J����8���6t���P~������l�x�l^��]3ઞ=k��3�=��iuS�L�4�v���q�U��c]e�cF�*).YX����HuؓmI	1�}�N�V�(r�(0Ȩ�U7[��f��a����y[Z���V4U��6Kn�#=]�9�(Oy��<��:FgfX�lV�J���5�y0����h��I�IRZ�2:�$&bkU��J��5[���g��j���z4�
[E�:3z�Lj0�M�u���R&%�Ԫ�Bt�[�h�ji�N��TU�%THmy^�Ԗu3\n����}E��4�k[m�-'{�bV����/�ҽi�Jo�ҏb�ۼ��*o���6����z��� ��m}_ii�(�������i������X.�u��xW�y(o�9&����B3/�5X��%+K��7��TU5|�5?ƻb�53�/}��˭^��<g�|Ζ�n[e%ŭ��uUb��2p�U=�Y��Ҍ'������Ͳux#m��+�����@5od�����fUU�qY���+i��-[�g����[M����k���+pRUݞ�yބfS+��yV�)��j��5�<m�|�lzo�>�.Q�Q���v���3?s�=��Lb#�-4X��`+�z�.)�g�|���L0膽x���`F�W��"�W��1%6&��Lc�۽!��ңahL�������ҬUm��xD���v�q
<c�>�5�E��\�	،d�c��T���fk��rM��s㱖淶�V[��f{`�4���"�y!�3B���t��J�qR~([sT���bkw�����7nh�x�I+�[./
��K�w7[u�� ��-��s�{\���%����n[�g�I�4�2�R�U8Բچ���{�{llU]����o�l�XW5x|*��{��̳͊{�d��y��3��i�	��M��v�B*�I)?���d�1��+�M?h�&#�K�q�I���!�����ʧ����͍��#N%~3/���W���0A���mm�^����˸���
nW��`F���{Rw��)\P01Z�"o��4x���q�������q��'��8��h�]1����^Wi?���`��2ޫ�T-�G�T�/G�4�'P��3����tީgA����^����S�r�(��;ܖ+]�x)��r�plP�!�	��Y#I�őϵa��f+F[s�q��^�6���D��MR�i��i�v�N�U9�A��i��_�r����/�.p���^��1,�0:X4���/šr�'x3u�0Ͷw>h�%%{u��-��S}ZlE��C��hc7Y��̵w�����vN�0�̰�_�`�Ɵ��6xg�gf�m�I�����+P�BtCD#�T�c�b.�Q�X<^.f�ZTdb��.F݋*3�P	b� �������1{�T1�����mR"�Z=v�8��Q�K���"�Hd!� ���CڐI�D�ܐ.�K�4~�P�F�-ÝTAJ��C�A������8��!�bԋQ�C݋�j=	[��ֵ����آkX��>�9�W��gIG��g�@|O8@��ʾ��7��	�	_�I�}�/���s�g�O	��A���ϢB|H�����}>s,�=�9�.��ۄ���MʽAx����W	{	�^&����E�4�=��	��J�>K�!<Cx��a7�τ'	Ov�6w#��v£�m�^�#����	[�	>B�/>�%l���!"<Hx���p�/>q�^�w�O��	w�$�A�o'l �'�FXG���^K����L��p#���T�:�µ�kW�"\IM���W.'t.#��
�V.!\L��p�ϔ�������p>a�<¹���sKg�"t:	g:��������	�N%, �'�B�Gh#���Z̈́لY����	3M�F_�H��pa:�Mh ���S	S�	�	��	���8B5��PI� ��\�2B)aa4a��P�)FF
	�|B!��CȖ 2_�sYdt2	�t�B!��Bp��Q�d���t�/��HF+!�`!�	�!�K�!D��(�!�z� c8�@���AK���BP�QA�d� $� �O�'�D8D8H����_R��{��2~G����k�~�W��_� |N���)���>��1�#������X�x���g�@��3V"�%�Cx�g�B��3V#�$�Ax��~��wj�Ujl/�����ߨ�K�	/��'<G��JM?K������S>c9b7U�3u�$��	jl�q�N�c���G��m�t/5�5���0au���#�P�^�&�C��	���E����E�E�C��/j�n_�d�]��)�;}Q�w��\���e��'���e��J�k)wy�L��*�H��5q=U����p-��<�"\鋪C�&�+��}��e��F�*_�ɈK}�3+}���"g .�����Br���	�?�*�К�}��	O�>���q��j�u�C��>���~��P�E��O�w�ޅz'����n@]�z�z~�-�7�ބz#��ף^���Z�kP�V�O�
�J�ըW��U	?	a:$���!�-�E���|_8_Z�	�|��:	g:�3	�N#�JM��s��	E���BB!��G����u�C�&�=!�J��pRz���!�	*BA���V�f ��ڇ�%����~���껨��ꛨo഼���N��Pw�nG}uNŭ��lEz�����9�%��	g��r��X��PF(%��S�"D"8���(�\	w�|�`7�(��\B=��4Ya*a
a2aa"��0�0�PCG�&T*	I�D���@�̄x��G�%��iF����~ԟP�D�'���~�z �;�oqV�A����~��ꇨ������y��P���,�_P�A}�)�ݨF�E}g|+�è[P7���/�S���#,��Q��'�Ba�Gh#���Z̈́لY����	3M�F��pa:�Mh d��LB!�0��FH%�;�M2�F�d� ]��@��~���;ꫨ{Q_A}�o�/����z�%�=�bљps&\X��}����5���o\��,��v��YfB��l㲷�)ΫY�>w�R�li�RA}N���%�vk�fڳj��]u}�%Fv5t�v-k/�wum���%�v�»�FU�躺K��r�X7'viB��t�m�t�:�;�Q�u�}�L��dS;�;��ܙ�Zͽ:�q����NW�xfM��cc�{J{{����폷˗�_�.l�jW�ϨY�~o!�B �����OT�o���+��
��0 �b 8Oq��x�{���ݶ��=�9���lv�v�t��8�}���=cc����q���ӝn��w���=mc�{�s�{2�'9k�7ֺ'8k��7ָ�ְq�jw�X��w��w�e�e�E�i6w���>�~���?^Xnbaq�㮊�� �!6!������b�aRB�v��:+B��exɰ� ��vU���Ma┰�a_��d��ئ��C_���m�By^ԻB�9�a��k\�N��+�MщW�K�̭v�S�˴S����z-sii�_�j��Ƃ�T�P1��1`z��s��E%T��1��O90v54���*�j�!Sgx�*���]uM^�*/��fxz����	�H��u)���`.����=>q�syc�wO�\R:�Ӏ.��u-Z�8}Q:Pg-B��.������Z�K/tI���8�$�E]���,@�"��s�$��k�?*?{&�	a����o�B�z������E1�gI"P��_3�S�׭��G�	�+��254�%�8|��7p�[%�b�,��?��k�H�t�.P���4p0����g�=����\��q��m����_Ph@/��ϡu?��x>P��¥<-�د�Ϳɿ���t@'t�8�¹�·�p��Ka\��X����
XW�Up5\������F�	n�[`-�q���x�6��A*�%w���>x y'�w�=p/����? ��,�-�v��	�܋�6�z��a��s����#�m8��a<;qw��>)ٸe0��t�3솧�ix����x��=����J����K�7x��^x�������>� Wݗǔ��o��;^����z��'����R�R{��>����&�!`�����,�#�=>;wIq��	�|����1��|�O�20�oFp0~Ǐ��C�ށ><�d�@,��	��Ρ��Ie>�ޓC��(��â���~��"Cѣ������2o���~�u)��.�������g�;|����B��/����'�}�O�
H���5�'��w��-�1w��h˿�������r�G���0��D�N�J*cr��=-����i���J��RU�*1S�=N�J�����e4�aq̄���YXKdI��b�J�Xbc��>Pf�j��M@��a�i,���G�y�,L�|V�F�b�db>�%X�-���8�?���#qW������~�����;�w�[Y{#
��3�6�g�i���,)�|\�K����,'����<�ޗM���)��ķp�A	�0	&C�бu�����TV�d*wbV +{Bp�ֹ"d��d*�(���˔WP���;O�aOxq���N�����O�����e�0C�A��PA�T(lIN� �Q���[*�;lI��d�/Y*��Z1r�R*�<��i�X՟,��8�>G����	!!b�Egϳ��N����e!
Q�L),��Ϟ��:&%ޜ�F���O�C~#=t������bOi���F��B֥Z��s�����t�PSt\�2��Q��s�=Z������y[��Q�� �B��ҡ��d�g$k��DEi��f,Ñ�W��K����99�����[4alb2�R�T������/�.eĔ������c��YV_nVF1��h��}�d�4b�e��$G�!�0/��g�"�|�`��4DN�
3;�O�ߛ���Ȫξ���1Ί��3�R��d�sɚ��Lc�����֝#kG&���:���F�d��g��֝אU_��W�N9���R�f�{MYe�?f�ˎ�_�Y�?�9>�l�<����������@H	�.���ts���Y'q�f-'Kْ�T�U��e�:V�C��f�T���R8"��\Y��џ����#�Ocٽ��0�a��o�"/-d�ģ"-��Gl8|aK3a�ssx��0$*�JW�|�g�M���:�������ru��=���PG�m>eA���75{�����J�VVe�N�<��{����,��d���h�Ƙ��oɌ��T7���-��{fE':��pp��H���+8J�ΥUش(�����a\��EŲ�����r<g;?gZmt�F��b?�0u���L�%ϑ�kѱ� ?Òf��¹Q�W+x8��&6��-pPq&^a��5�K��Ύ��R;cb�z��-�9Z��@ra]�V��e����[�6abN���S�z~��1gݩHH�Kp���n8Jxt1T�����œ�ۗk���xLV^�!O��?���H�7�X��S)�f2���"D�<�ۢIřs�=9;^+�/��'d'%e'����%�fMa���l����X�.!���cJ��%FWf>��Π��F��!�y�a��?��lDIrX(��Y�<��f���)ajgXX$��ř����ii<�aabZ�3I��)�F�˖=���M��u;��
F��7��t�\��)��,��?or0�X�#�f4F�����\e�z�=�Ö����_-�d�3��S�L]��OK��ɘ��g���w&'9c�'G'�hB�e����e�j���?C��(��YY:KA�?%��~jj�MU�l�^+�k�j�������!Χ�$+�k� f�	�
v�R�����
��6���а�U���Wm��<�H����Top�:r�+��9��>��t�%7őg�椲g��2�}�Փ�:�*sƋ�,�w�h�ܡ�uEV�2�t��9�����1���]����{��iHSo��眑�Z_Z�A���S?8t=��6�\�cc�ՕX=x���t���'m������v���N���-���4:�hd���c0V�B,%�#r�Z���׈i�a��ꔼr��e'9]�F��2S#�����G�e�����(>I��ugвĜ�yq���hޘ�"5�'�=i�i������Mluy��
�5��J~�>���a<l���
7>�����5��:�^6�,s�a$~GFdt/�tiǚ�i�Fi��]�������R����b�K.�g��a8��V�?e�����T�QG�_�/|hn�"Ϩ8�L�͛�>>{bA|��9��Lʮ�Z��<yji�R.�J�F�]}��tWzT֔��������n9%ߘ���L�ILM�Q��(�I��^\7s�Lgh�%24�gN���'�����t*_�Q����N���k��u�c���0��������˲v�������xx&>9Hw��������W\e:�u�Z��j�j��\v�9-V{�oh!Ehc�̖�M��/��xE��2 R�����C�+�=�@�Ɓ��;��<D�.�Vnn�/��ކ�^i,6+f/߃Ë���s�F#=���$*����(ɤG}Y��&&��5!�Q#s��5���yc�:sAzfN�R�&+ɥyi���j���5�>ۖ%�a����x#SG$�7�#�2&Sjڻ���yZ���2���Ǒ�]84���n��%Wd�&ny
Ky樂}9B{çk;�=��5����(O���2��8�̾��|������su�Q���~[����j�ϒ�faum״F�'gM��4�12�{MV~�!7*�rԭ��JҌ�b������d�x�#�R>�&��x�6$�l:;�����e��X�#�߄'���k
|!^.0�)ۅW@FV��jf�$�7��^�5+�,G���e��(��ޙ}�Az6�9;�4�'Y�"��U漚]����tٮ������]�6�dZqi�ؤ��m����R�J�B�+-�V���KW�|���hK|N���6YM9M�Ol��>%�bL�+gQV��	��8Dw�!NX*�aپ�VS/���,�����ϟ4+�o_%u׿z��]iJ��s����5��O;��9�!$]�҅�(خ���zIcnK�I+0�|=e�H2`�?��FX#@��B���|����׵��a�Q��3q`{hO˕�*�fl�Q������C��$�P�I��XUH�J&S�������$o1��)�Yetě���a�g7��~U���V⮤��� �G���u��k4&�7�q)l6�e��`��5�_�|�8�ݸ�ֹ��u�z��vV�C����vh-�G�E���7��'
�~|�6��/T��Z��H������E6���o�X�;�>H�)l�e\������T[c����柼(���]�{�y�>������xo^���%T(�{���$Wi���g�֞ט�=}ɸ�r�v%FgCɖZ�����Y�+_�~j��7.�xN��(�xABJ4_��M+�_�����TG%��%G�R��c�J�Q���~^�vRD�92s`VdF�o� �Ǧ�C�N���l��m�^�ХJN>j�zPXJt6#C�\��g�ZK��O��l�)�'Ȏd�I)|&IP��R������'*����+������Y7�=
"�ᕺn�Z�&��� �1�a���Zg�]'�P��x���7���l��^�᥮1���m�\��&��%3�X��S8�aA-��ҁ1�-Ic�o3�?�J��<�VU�U�������M��3#u��tό4�,i4�F�%˒�͖���;�,��p� 	ؒF�qb�<�&y�cB����9�s�Kp�8��^f�e����cuw��]������u�"��r�b�׾� �h�S��8�9�ɝ�`(�R��,�.(��:��J��� ��	v]�D%��k����k�g)'��F;-SJB�-�ުi|�aS�4
�Y�]�N�+�E� ��@Q/����K��H%���Z��#�ݞS����+�)ġqx����)�#*|	Z�.��F�� T�JR�<A�b�,t�{Jj���(�N��Q�,�BL�!�&�'��3����R�6�e�Y�^�eM�餍�?t�b6%��9��t�J��gb��S�����Hw�[�h[hR�~�V
$2	���w���CQ���|/����h��g��?�֠!Y� 	�����3T؈��S`�&��# P��5Wc03q�#{��?%5�t��cRS�㉙�q��5N�A�#���p���o�,]��a��򣃎2Sq4���H�v]c��c�wb��{�q(ʃ�q-���@���u�_@�q�_$0C���Er#�q�o�(����ã�<��O���{W���/��4ޱ�ό�vX��;G��K6�}����~�>~��?Z���+OMܹ)Y�>�r�ݛ��;�߆���.;��R�h���	�Yxs���G����ѳ~����֊���(�v�W�s=��vx/���,Nc�`�7��	4~��k��G3�A-�����������Tv|�R(l��� �o�{�@?4�hP�9�RE�Y�m��Q#�l��apx���X�7��[[6z����H[�"�4�cķd��X$�Ru�ݟ�W�˃�ʚ���t[rcYs��Q�%%��Vӽ�c7g�z��R%�m�Hko-�{����o��T�#���͎�ؖ���[��"���/DJ�(X�0��<Kg�6| E�<��$�E�0��a�Y��0�˄N�����t��8�5gp��OM�O�NHd�,Q�����Ck6�����D|]9�	BLI�p�Dҝ�iB�koX;�z`^�P����ipM��cU9�6O*�f*;��G&�
�Y�0z��Nb�U�����=�teD�
ea+�p�����V8c�I��L�Pl���w^B�U����] e�_��#r	�Kd�A�uW��K-���^�3���u;-V��.�$�l~�Y��Q�X�j=�P���W���?��-��u�OfO��9���x�`�I:��S��G�xSwf�Sw�{����_L�)X텱Q|�y��G`������}Gq�c$��9�d������4О�9!���`��	��|�GH�,Q��X���g�ʴJ��%�Rɼ��O �g���VBET:� (����7�R�����JK	����r����1��H�� �����*o�� ��W�Tj��3�ɲإ,�ElR��e����f*Y����y��%τg��Pz�\&�x��+�.)d0Q��TX��~���I%)|��fIV S��àZ�������)�9A��!��6�Fթ����Ͱu.�i:��[tHr29�C�W�,�9m5���8���c!u�u���}��_�&v�2*JM�H[yˁ���}okߺ��2��!�T��b���3���;<�ˇ�:�Ǧ��F���uhCk����_��U�g������ < <��J����0��w��@O�sn�0٬�$�ђ��Z˪Z�-9јoIX�KN�Of,�Qq�:�����/VH.�l:\��``.�nxSl���G��g�L��I�?i�yV�`�;\l�QΘ���#�-�Z�a�;�½���mc����z���֓t*��=s��wuJ�������������M��*��z�/�G���N�d�ڝE�imU�]�)(���SQv�Q4]����y-r�mz!iN�տ��E\�U��@!�\�h^��MŦ�����/�T���ۃ��A*$t*5�e���X2�Lv���Keb���Iit[T������;�&�Ai�P�>�ss�欄�58����(8|f�����t$��m�>L�^���z�#`D0{�:P��W����w2k����@+�ȍu`#<5E�<��OU�l�(��k2�.6CD����r54�e�7����h�w�������`�iNB/�ˡ{��>V��l)�?e�i�.:�-�|+�Qّ���'E�N��W�x,�9�R" �C��:��޻W�<
�)���ơ���P����Ir�*�]�������4��1s������;aqZ8~e��-^���6�l��[�B��<�.%$e5ǐ�"d�8�ęuc�CN�d8��3@�1�]��s�1F�ǟ�n4�976C	��X�k[*��
���=ɬ@���<V>;RWׄ5صYB��Lqa�����aKe���v"*�y(t�){&̺5���h��m����!h¤b�;ŶbU���X\&s��B��Y��Fp!U�}=T]���iد��m�1���O�������o'��ͳLa\É���ˌ��q�q?R"���[����If?�i'���gn�g`�<��clh�F�><��NI�l�3���nȖ�X��Ʈ�eh�/����`��zs�:h4�;z�S�fn�Kfcv�S�w>6���N�R�G�2i�"R"�Ԯx�Q�������;������}R!�S�<����������%Ι���&���w�#�A{!�845"���z�-�T+dbz��]��D�4���ʨ�(z#�>�Y���%tw�LHm���Q]�"�h�<���Z<����~K�Ժ�T1`(�M/
C����Tm���̖u�d\s�s�\<�D����ξzg$�匉�j���WHS��
����ۼ����M���XWl���ߑOU"zMhE֒��]����Q]=���z��6}���
&�z&׵w2����*D��I# ��ċV뎢=͑,L2 nn"�����K��XT�6!�*��<+�[b�XA�%�L�M�j�O�P)p5���dX]j����d�k�5�]�ÿ�C[�"�L�|���LI�����`I�u/�2�} ϲ$&΅|�c��9�TB�F���{㑡�gn��?�����+�.�i:YI�q��x�85���b��lp�
�Q�{�;��{�CG����{��o2���������xN*`EN>���E��W,tSihmΕ����b���4��2�/d.��D�����A����M�W��?/��0
0{�րE��R֔��̖��Rv�V+�����W�A�\�c�j�b�X��L��xӲ�?����6��J�9 ��|�m�q����2�}	G��|>Ϯ�닻�`�M��E�����>"�n��+;`�J���$]�dk>FU;��S0\F�J���E6辐a#h��l��p�C�D%#�<���E�3;`�]gV�d|�-��I(j��zOp��I�=5���{�X��"&�Iͩ��`��J&����C+��T����}et�]p�H$ohY*����R�T
���z�-��@�_�}�O#�Ξ���ް�:�=0���%i�U= '�~Ϲܽw�|��$l���;N"���2�j7�n`V�(2"H*P��]"~�Uy~����2y��p��4��3|x���"7B������!L��E���4p-�ϩC|2�ݞKB���b�-DMr&�����k���;�3���:�ʊ�IRA	���6C��Y���)5��щh�,��(W�5&��;���"q����Y�%rj�;����R�r�EP�	5�Z�X�����7�῅
nO��LV8�'���m`�l����#B�2���5�G�������$3�^������L�f��� �^@�a+�B���HԹۗ��`��.}w�@D�W;(X��KR0J׏��T̔R"����������~���p�@���d
3:d77:�x� ���+�3Z�
f��{��� �����0�C����b��OB�6� �f�*3�����붜|�)aaM:(����&ʜ���������od�ɺ�s���z�$�s�#P�������'v";xpr���y�O�Ȳ��_ֿ>F�G>�<�d���������T��-ŚU�곖��\AfP�4��ƹ�\l��*��������X���7���I��/����f
-�F��v�kP5�r)�v-5�?��c}$��<�`-�8��ǿH����v��F>]�ԑ�V\Q��l>�Ĕs���y���Y5|���b�c9ì�'�[j�503���G�B�*�B]n���
\r�U|J�ȸ!tPb!N�DJ�M�Kb�Ӵ�G��S.wf���K�p����^�ӑ����$��@��CL4�"v|bvb"v+�i6:�	ϟ#��QdO�:�9���t��x+ãvF(>�4iGe3��)�\E�6��L�	h�m���T�f��'��h:c«b|�K����F��B8Tp+�B(\pCx-h�~�̯�y�7�uE�] E�D�M�]�Ob��L�2�P,�;K0���4�A���+��W�z|+���}	�~�:k�s�s��-�t!-�Yaְ��f��τk"��Q���r�G���k�ں��-K7���c�(�]��"#_��b�uG�r5�_%(eO�U2q�JFP����{���о�;�{*�Ad�.�~����R�X~a��û,�K�u��}܃�;���tϾ^d��H��g����z?��P�cn3��CB�J2����M	�nd�����ٍ^l(�&D�l53T�����J��n`�'��
�7�a-70s�>iiUl���"�1ʸ���ϛ�%������=����b	'4%W*�8�(��l_�W�w�m}ホ���dM���qRB���	0<�tz�$
!'�^$&�Lb��E
;P�r3#lZז�z�WJ�G����b-��=AmR|�'ꂺs�\�NN&N'�a�5��ݏ���ǧ��B��{���4�	�����^6�.3�Zbm�h����<��Cx�* �j�^����)������P��H[Yv~�JTA�>U���@6����̌��!�]3�dcsX�DN�KL���6�3�J�PE���j��?0��H�Ĉ�9�zU�;�kM�?�ȏ}sQ5�vlU`ԫ���&R��F��
[��FvY����+�b!��$�6T���]���y����ߒ*J(��-_��i��5JJD�ߺ�!��������l�e,�6o�ߤ@q�e�r�'�z�~�>�H�6�&����.����]��l��I�"�j�)�]�����:v��p�MGearӚ���l)�>sba]��B��,��&b|�&�YB�F��c��r �+� �&����b5e羫�^��m؜����ZSB\sZ��u����1�Ba
�m^-4�"��7�vy �6'�B��10�~W�2)&���L��*�b���P��d�쇓�c3Mb��U��Ы��R���E���	��C��`z���܄��M��\��T������SS0�e�Dм|;�:��;XKk;��b����zeaʫ�����������fLID0�7��X�l���{�B��g0�@�ҭ��C�����Au�A�����Ș^���Q;�*�}䙣����ڝtF��7\�~��pݍY����&F�]~�D���k{��3�V4��8��[�qS�{�Ɉ^!��~\B����]���nȹ{i�q<ѳ-��>2v��)�6��L�#E�qS�py],��^�C���ہ���
�s����i���e(���H���I�O8�AtX]�::��,����:�O�k���9��њ� G�?"�.6+��L�s6ɿ�_n��<nl������m߾���ɜ�mD2�,��w��[���+�n[�RfB�9�t�L�~[����F)�N�2:�Z/�5����:Oک���mG�����u�����xcE%�jg�k華ғ���K�Tx����8
FS��gas��zIZ�U}e��n�e7r3�h��5}|�.�E���p_"~ڴ_Z߼*L��\�|�9Ԭ��`ߕ����c@�`�[����>��!	���gKf�w�[����,���k◠E1�5��>���k����]�ۺs�t�}�5$k�]#��"�v�x���`�,����
�R�N6_�̟��g����Sp� ��?w&3�Y��J���-.�L�5[�@���QK}�s��U7p�q��o`���KA	�;�&$��0!Yj>�M��IDW�~=�罆R��(
�HL0J ���gK��̭K ��1�)���%��
���A$�6k|��	�r
:i.���I�4Puh���j�iuH�`�����̝�� PN��=t�ޏ�]���)���jm��R�mr�Sjo�ۜg�&�'�on�8i	9]a#Y}|��aeZ���n81�j���c�}��0Fz�����w���������y�Z��5���B'_PX��k��]��]�}#-�4,@�>�&��^�M��Ml������@�`l����@ �@�'4m�?��8��P�b��,%���5�[�m�����v��AG��]��s��HW߉�n����]�����@t��Ԛ�⚼{�����1w���W�c��������ϳS�7�|�J�����t�	��$��ԑs�d�,E��R�ѳ��0�&E.�L����KQ˪�?/��I������1H�b��]]6��e�����-$��}��˯-��@z�Vb))Ӈ��e��߁ko�w��[��W���ı��hɷԅ�߂���ue/֋Ο�����D{֫0i�c5`�T�;MN!:���)v��TD�S�6ŗ\�w�:�����|f2��Id�LJEJk�\@�﫯����[�[/E�A�e)]Mz�.�?�j����#c�Ѥ�E�^���(,A�#�y����}��L�W*�N�գ�Mz�ūs�]6w�tǑ�K�C�1���.<f�f|��Y_H{�=l�4+��j�J����~�ߎ�ֵ�lG�]���o�G�Z����7������(�ܱ�y e�R6�e�a��.t.[�)�^K$4�Ă�d�T\�!Z~�_����tx.��
���$JO*�5z!�����~ow�`�I		�h�u�pusko*�<J�)w�w�xlkb�d�����t�9�"f��yW�˒-�_�������a�t��x�<b;��`��;�3�S�<�i�sj��'���t�(�5�u���]޼ G���,�9���U�$g.�/����b����Z<�y5���P/_����q�]XC@C�vJ$<Mh�~���J��
v�-gW�wfX���i����|��Nf�Q�C�����1�h��P�ZT�:���*:�|5�U~^un��a��n�"���d�p��j�@��H�lA��cx��7�w �<�ċ�5��"�.3��YB�
���%����4�u�q?;/��.HB��*T_�8% r�AFA�Փct�3����\�z�$�X���PK����k���V]�@@�Ʒ,@>ϥ�/CL�<��I���x
�UOG�6 I3ȇ�6� ��0�2ӳA��~:�h�����CV�^M�e��b��f��W+4,�]
�(
%��/XQѵqWL�P��ė
�x[C�
{g�D4�S�x�: ��J�b9�k����2O���π�No?2���rW 	���!�m�?A����Z%{��|�ͷ�Aè%���]�f8`ق���T�ꐆ���:;c�i8W��OG@��9���-/�4����^������B�VT�5X93�ϱ�R���R�[�UR��@N�Ǒ��3����I�N�Z*r���&�]��l��֦�r6E�j��7����O;������^�ީ��
�:��b(�:%���I�(Q�3�~&�tuEƁ��gH��|t�)�(��(X�Y�l����u�
�j��nU��/�=mk��e�񌿻L�cyԗ7w����mh��2́o�s"*�m\pB���H��G|�f�`��| ��p�5|z������	����2��v����,l0)bǲ��;<��v�I�M�U�/����UG��_C�v��w���"�w�*Q����)��'sh߮�����]�s>�'���a����s=�BA�]�:0���ja�]�������~,�BnK����$�,T��K�P�e�L�":�B���B�Zv��ٌ?,SK�R	��<�<>#t�� �O���sطg�5�a�*i�.�U��D��~.x��$y�8�cyP�2�j�̍q�e6��#�ν���y�7��Ǻ���c�9�v�bNRcP�+1��/�<)�<�q����r����0�N�cz�ީ'5zu2l4;���OK�f��a���m��;/�@�4�L�ǞE�;%5�:IE�۞C���Ùg�G�Ў�T^l��5H�P�ꊘ/�蹁l�����ǑMF�$:a�4�6�� 	��n,�,5�m��ϑsX�&�����(e�r
�@B�	�����ם\����G̮��|>s8��n�@�N��;WR����w�l��*_��]y�%��o���}�y�R�'n���HiE#�p!�x6z,6qpX�w��D�Jٻ�{{�yZ&�
0"�1�)���8V���()�Q����"��>11jtˍ.TA�\|QDȁ�i�9��  �͖Tj�~�����P�[�����A_ێ$(X_�$LN�8��w�T��.m����X';m���#�M���Y���o9��dj�M����ۍ���F�G[�y���z"͵<���`�Tz�gg�j������dV�B*x�Y�E�iS�q�!�LG��Y���k�ӡZ�x�y�w��Z���+�]��`�Mʥ��ΗcO9<q�Zݷ��<��Z�N��tأ�TF�>�������`e[��էq�-P�l���E��Y�(`�s����W�7{�f�Z����� ]��`Z�'l�5��f_��.5�2.En�ڨmR(�)��o����!|;hή*��R�_ԙ�BR���p���-�P�C��/�CaG<?�eKH��Qg����
-���&p� Va�A���{�@�H�GRbq*B���z�k�us��+�zR�<�������e�L�*=�?��ױc0����� >���//�GX\���_�F����QE"�~TA�RRois�1���v�}��%8vt��o�+�����?��`���oG�z����N餽�-�l����y�2�2Τ��DM)ո7�z}�rή�sn��E��y����xb$k�����z��K����N��m*���Q���4��I�8� 5�
����A��T�g���07Cն��٠���ß��j�A		F,9����6�ms��Y�1�mIG�+��4��|�iH=_��"��q(�h�P��<0`�
o`�>G:�լ:������i�\��E��9�&>���`)����
�IH1A�I�ws�`&
6�{�=PbS�_�3� ��ž=1��3:�u�����H�@rgqu�&b	�.�Xk0��JaP�f\�g�|PH|v{�Dʴw|U.m޽L���������ߐv�g�,FnM����;BsH�����%���-��6j�������-�ҩ��o�W�	]{��#��N��C����o�o�:�=3#i�~��˶<�d�%K����?b�6�� !�$�"�$$9v @n�-�����Х�_�v�-�[Z�!�LCC{� }Ѱ���n����f��s�sfF�IC۽���i>{tΙs����;gf��ڔ9G_�|��.�~������xJbP��˰KV�X�*D�W�ښ2�����@��v����d]wucɥ}tu�.�����6�Yf�k�D��-��=���!;ڴ�L���\f.!�l$J��ta����9���27?����xI|N�m�d�9O�v@��E�	Vu¼�`-����*��A�FQ(������{~�u��i�y�q`i��?:�8ꪝ�erl�h��Ue^g�ϮS�z�ݽ����	o�Hbx0y��f$>���X������:��=ԛd}�~�aC:��M�\�M]�=Q�B��P���;�O�/���U�MZ�dY�Ơ�XlvC��M�U����e(ɳ�J��J��
����J�
	+7��ԯ�I�F4���y��p����ǐ��c���r&�Pc���Ni�Mʻ���dyG��Wyk<3*�:z��%��%�U�]BKk�t:64V��\��׵;���u�5���m�r�2�TVZ�\[?����ŷ�oUiu�r{�E+���2���\�����`��4���"�F�֫ˊ-�SM/Yu<E�~�=�"gE5���H���W���BQ���,�%'ɷH�?��9��U������e�q���p����j�ڍة�X�I٤��Z!��b��+��E�U�	��x�q�ʥ��ć����Z����_���hp*�ڄ9��,Z3�Xcɚs�j�;g�p�-�Q�i_7�dV�IDT��Y��0��%��F���]&;�Q����Ů|����v�y��^�8}' �F���p�Jz��R��ð�����J�1?]*�$�� �F8%�uu�)]p��My���ծa`�*s��^mV2Q�d�Q����4��[L+�N=�z�C�EK<F�V�|�㨽
������e��R�b���g�)���7O
�GμK�%~f%%9!|�e�+���M�"}0��{�O�|n��Z7>޹�i�掶�_y���¯i$����U�*�>�8TB�JZU�U���ƋbՔ�O~L�ަ���
�Ϊ�l����K�����F��f��RC=(~у�SS����"��j;:�����ө�9�
���NIb^�П'�>X��t��ϯ��L��V�{Fi��*��
�������\��7W��_ظ���jjF���A`e�Cm@SiϵT񿽩r���@����������JA�G�#�Pڝ��,yr�IrU����Z���J���Ԯ�3���N�ꨫ�r�����Cu������v�1p+��"Uޠ8�`F �8ʌ|�wj��w�d�ݻz6^��g#n�M��A�B:d8�X�T%� �dE�Iu�|s���ee��d�R�j)��.���Ë�z��+��;4�$!^��τ��.�~UB� \%-;\+-uo�o�:��{eM�e�{n�4�>�(q�J��
����q�R�JTQ촕:J䔙�����K|Գ�e�������|��C��hz�������P����Zj���Yz�Z�QtL��{������l�w��)��<���Y͙G5_$�5�g���ߪ�,y��C2�Ay�e�2���W�[�E��5]�n�hX,
YEQ��[�o�{��ֿp�#Ի���56��R�r�]e�cޑ����Fgq)W�Z\n��JS�Xjؿg&��?���(׀�O��]�K|���	�Z�!��%dGVj��'��^ߚ+,�r���9�e�����)���W���ſ���]5�yݠ_#�*e�P������w=N��4�e���o��,�(s��{��l.�T�5��9I�
^n�����,ᙬM�	�y�����&�wt/���&ၔ�AM�5~ ��ndez[�r�&���hu6�uת��e��;�v^w��ko�S_*.	��-*���Q�q=�4��e�j�ڭ =X�D_Ti/��jn}K`����0���{x@R��$z��
��*,��f%�H>{����!�Ob�w>"���YM�\):�ۛ&�uf��ڶk65���iVY�a��@��P������G��zoy-���qV���%gS7��*aikwU�J}Ng1�U���n�x��|9�lnѣ�Ԕ^���V�$����RtR�XH��[Qe:B�+*�(Vʅ_�(�!�cTuX-���/,��7���z4������̾�^}}F�;7Y�G�w��v�*�)��t�_�c�г���j�U#�t߄�PR������F��K����7����2�ߓ�Q+�5]U����0W��*����滾r6��������Y	,��O�-7\h�o��Qj��o�l�D�G�k{�
�C���et��՚�ꖯ17�����d����d�o��Vl�V&U����*�ފ��
�L�2l��w��?�_����j��SԹ�z��RYf����X_��K�������b�IW�9�sL����w/��x�?�4`i܇�4X��������5-�g`�%���q@�>�z���"� ���ڢ����2�T!���������uC�WS2�n��I��F[I�&VZ�t���Fk�0�Z�%E������u�C�=.������V��L �H�fе��$������a�P;��m���݀fQ-���+=���T�c���5��'��	�B�������-�-���HKK�H(} ʖ]s��lKr��M7�6x[Ӎ��������Ɖ���t��e�W��w[l��^���c��c�3�7���4�����\~G���/R��֖�j�H�4��qvU~����)?_�����CK���x�����FN��7������fz���p��L���[��\TcKK#�>�r=����$N1�����777��>Hl��n��t�����>���МPi����q�W[�-^H��,�/�ߔ�3-�?��p}��J��$���K�g��	���_�M�3�?'����oe��qrg��>F�J��Ц��_jV�h�o�/65��Z]�P���h���z��}�^9d���}!�!������2�N/CVͫ/%йO� n$�h�U�빍�^�F�`Tj��e�۵�qx�p�g$�Q������$��Wx�v�YѸeǖF��]�+dp��Зp��bKqC����ݹ��s[g�\kT��k�ۮ/�kMV����j�-�A�-���A�0*9Xm�<Zb,w?I����B���Ѩ�<B�գGʒ����ز�Z��QVP��������ON�;8��+H����T�	����HL��l����dq8,��a�Ļm�[:�k6m��ڹ�yv����6���B�m:�:i6�����p���-�����-�]�hi��t�z�1'BN�����Id֛i�����Fu�P�j� ��[h���?^B�1�D!7~�z�V�xaE���w���0$2�4������*f�:����"Ś+���`�3{;�㟝�ɮ+VI���9��-֫�=�u�r�\~穯���<g(1hX[��'w�?�e7(t&{�0��?�t���$ˤ;%d��� ��	�����ƕ{,����ύF��s�b����q�7Y���(���++M2��׾a�da�Sm#:dT������Ak乥�M����nC���/�˲��/��oh�8\������:��j�&{�ݤ�����R�5]��Ug0�h����:�;��?�p�L.���U�Ӭr:x��h�Z��Q���{�z�Eԃ�[u����'�8a5��s��*ܥr�a�6�\��Θݭ�Q����|��Qn�='��2	]��VYS���*5i�S�j9EU��8gW��˩�*�Z-7՛Wn��Z�hE���������Ԧ/6�+TTT��!�1����}�zi`t��Q"M�Spzj�p�Qba�|�AI�&��Ċ�j���zi������U~�}��m�6������-��ƹ᚟%D��|v�^mR��#�x
Q�ƜK_�<B�}!X��y����=�����e�i�����a�KQ�w�+�m�>��b(�_��:�!�eLV��d%u.��X���ʰ0����@r��u�T�wMU~kcw{�K���0r���]�+� �=Wn�imt�ְ�*��ZR�(b��3�����L��U��"�p9�p���3���f�}O��՝5�����"�'\�"]�p5�9<�z�^�'ٍ��␆�?��!�ل�R4�M֭�u�b��J{{[[f��ξ�8����V��%zۇ�ZZ;���RV�V�U�����]����4�'��7���	�z;auZ�*�k%��uK�{���H�ٌ^�+�yԀ,�"#������ /D3�rV��@��"��+�+a]��rke�M��V�ಱ
�RWh�h/��uĨ�SU��t���6�uE:g����ֆ�U����[f��IjCN�'M/�0.��V���qJ�èU�\�G��y�DW^ɖ�;Ku�A�?J��f�F�G�RD.b�V�e,��:}�^���:Ys����j�ů���#j�~W���,1F��'�FB�Ɠ��Y���,�]�~�^�C�
���$�/���ڲ��e�;@+YR��oeI�p	��Z"�]9t�'M��4y�^sI�w�ǲ�����X���>t1����	�������5��K��������S�&���7D�2�>���h!����hϡ��������㩪'K�^�~�S�՟XM5ʚ����>���n�{-�\������9�pCò��)_)�G|��M���S�������F���������ϧ��<�h����N-��_�f�b����:�}k:y1�]L�Uk��������/�T���Nz�8�2��P]gSg�&�-���<Y��@*P��3�P�~����Y�~�n׺��@*P�
T��@*P�
T��@9����7�Aㄢ*P�
T��@*P�
T�}P�?-A���B6ZOJ�U-��4��W�4����i	�H�i)���2H�NH�h�T+��Nz��V ��WH+�2}��v���V�:�ZHk�2y��֢�i�-�(��A!M!��u!M#��HH3�lEBZ��V���B�,�e�v
iu[}BZ�JL_�
���(���x�/���,�ը��!�a�Q!�Em�Ӏ��( �Q�WB��3���̧y=�i^�|��3���̧y=�i^�|��3���̧y=�i^�|Z��p�	i^�!5!?jD�ڊ"(��(�R�?��P��$J�} J"��!/�EQ M@�,��c)��gj�}jj����$���p�)���84����cR�	�q��ڊ}p�~�)w&׎<�� pH@]�@?�G��n�����/��g
�#D��%��=ph��.-�����r��8$��]��IR� �BDk�ϑ��h0a�DH��k7i&5�h��Z�=' �r�<El,���r��i@��)�B�&B$�d���<����H�`�p�\P���EH��R �4��S�������)�k����#	��%E�4L�2%��S�oX�E����"E�"\��b�%�r��y�%�I(cP2Oz�y����p�	"?6D��أ�k�'�	��Q�C� ��&����׼��^x;���D�Ӥfq�DXkK�/�^�{��͵f5�6O8�'zXFi��E�	�����$� �h��{n"#�qV������4H�[h_�J�#x���%F�  	���B��5"T�Er���|m�F��6��cE~��L�K{���Ę�f����Y����؛y/�A�0��31XY���e�� 	�2�j���L�"N�����>�EB^y�=�+��������v��x\�,�o�F	� [C䷖����'���vت;���賟h��L:cm��+�B<�#�Ct��%�ȍ�	�ט3x.a!�t�D���G7Mp�V^m��Ђ���E%3<W	��"Dt�f$~|��z2�������DOA2���٢ i���(S�ȿX��?��@��<^�;��O�m���g|N����r���s�ٙr5������+1V&3����c$�.))�{�<���A\��R��2^��"s[D�-<\3J���}���1�2Y����4�H��z�Q]C�eX�A\u�Z��j�L��CH\s��s�GBͪ�&qz��2"��ت(��%�?�xް*v�
�7-�+����p6��V�yp�o��x;�^ïX��,�����p�W^z�Öό�T���7�a�/>j��{��Ia���ZiV���Ǽ_%�u�C���DN�S(;˯�g�	��h(@d�z��>$�ՠ�����sf���S�7���-�'��y�vm��B9g�����왎X{���Y�Dݯn%g
�Ur���k����D�=H<c�gfb>��!	rN%�6�3��	��0S-dl�Kx����(�f0��:ߗ�\��3</e�L���YM,=���vg�r��k&�� D��Ϭ^n����#}�x�G��@���x 8�I�Y{�#s�8�䞳���Z1%�U��
�Vӂ�kϹ�KX4��>E�4F�����?���m��chr;`�� %�P�A��#�!���PR5&����R;�<4���9��1�Q��$1nq$�sWA�Q����kH�m�Ԝ ��B�|�p�>(�y��L� ��(���!��9�G:�\F�|TäG�V�M �!�h/�&�0~�� I�fp
H{��0g̳��.���Po���Kd�ю�8/� A�{�
����~�G��0���T�DCMV}�9�1��pt��cв�H:I�7 �K;BrY�xK�i�V��˘�����&Ȟ�2��-_w;��l-^�^a�G47Fr�5�Hn��
��� r��u��R��H<��A�<z�;�>�r���a��b����ṈǷ	��X/X�D'�d��Kq�c�/u�=������"�W�d}�@KqM��vnk$����3i�/�Lē�t$�r��(7��K���p*��y5C��dx�K�cS�an$�?�����H������9��97�h�p�hb�
Ă��^(���qC��gj.�⢹|f�InSd:	���#ԉC�\*���9w1�s�P8ɥ�����)n$�R�n.s���p(qQ���S�d$��#}���@$������d������	�R�%��f��~n1���R��h�Kơ�Hl@A�txZ�B��d,�Ly��47������4�Ly��| �$ ���/Dӑ��-̇�P3N).���50Z����9P.�O�i.��X׀���1�+>�MGf	c��tx)�#{�^N�:��b�����Ǎ�%' K2��湅�8�BI*r3TO�A�}X� ������$X8��8T��'�)m�`շy��By.�S:��ɽXb֌w΂��8�"�wd!XHՂ%���x<=�N'�|���E����}����l2�����g�tJ�]R� ��v�ZH$�p|���/���s�Fi참+#�M�=\(�J��FM$#p4U�� S���t�M�'R�.	�߉'����s�����`ڃ]r���6b`�ŹHp.�"t����Y��xKM��9Ձ����#	�l�J'#A�)��/����j"��N�x�⋱h<��^�Wx��É�t"A(���u���D�F!6���ձA"d��E�#i�4S y&�G�,���MR�5�D�5�/�c����H"�����|P�!�Ԃy�[�q�٬�
`�j��?�j�12a��x�Bp#���X�y�R���I��r�
��
\4�p3I|x��`����AW`Qh�ŧ!�ŰR$X�~v�R`@�T*����A[�t����(h�s̓����j	����֬Gb-.�q7��n�x8?��Ƽ��l=�A�%��x���a�����#XO/�������><�a:���Q��P�]�F�4�8����x,$c &L��G	�����`Y?�E����]<0�Ιtc�42|@�Ø��Pj�	�ἑ�4��O���"`���s9��64�M�N����'������\u�$�=܎ᩡ�mSԘ���ɍr��;���G�=��5�����7�u|dx ʆG�F���n�6A��1�ۇa$ө1w(���̶L�A�w�����N78<5�y�^n�wbj�o�H�7�mb|lr ����ã����ց�)�vG���nr�wd�tջ�O|}c�;'�7MqCc#�P�i ��n���Fz��z��ޭ��H�1�2A�	�v�"������F�}c�S����S��;�'<\���$V������c�	��`Usy�*8�mr ���wxM�ƹ���+�B�|��g�z�R���͋�͋���͋���J�_���_�o��M��M��M��M��Ѽp##�F����͌�͌�͌��nf(��F�v��>���h������@��.�$�ZMA�ĕ��hp}:q��u:\�)���z=��J�����+�_T��Y�]!!�%��D�W��7 �Ծ��Zr�!l�P��ѭ��5�G��l�ʎ�
�>B��oQע�S��[T�C��PK���ԭT9u'UC�M5S�S�ǩ-�2���B�avP��5���ng�>¼K�Ŝ��1�!#�����^��w��k��(`�6`�`�	`�5`� 5��kc`�� ��q0��[ �G�]����3��P��12��`��2�XG\}8��h`���; �' ���À�i���x
0�0���RR*NF'`l�� �(`�0� �4`<����_��� �Ӏ��G����:�h��q����Z�Hw�[㝀�3��K����`�`|0�`\�vQZ�X �z�8��s�1Ə����a��M����2`|0�0�6#������������&��� �����G �3��%��:`|0� Q��J�0��[�n����]��~�/�À�`|0�����`ޥ%�Y�.��cT��`,�]��*�8?������ �O ����b��J`48c+`� �!�� ���Ư�g �	��O����,�L˘-t9���1���$���Go�S�1o��O.Cr���w⬜Er��o./�x���ɥ��vF�@r�s���?:���� �����W^y���r)�:s�ԩ3P?���%G�^>�d�t����L�d�3��S��H%��� � �4�;N�r��H�ĩ���Kr��Ŀ���S����S��Sgr9�R�H�,/#^�� G�4�3�|�29�)��xP���l	�d��|]���4+��Jx�0��8!1�H�iJ.!� ��F2��4�hHR�HN3%�>����>m���,��2H!Y�z��˨AF������'N�K),����\u��(�"c#�щ��F���B�,��f�c$r@�V����҉s���
IǞ�̔��D��R��b����R"��O,�D��=öDНYm/��{D���'x�Y�)hJ�L��d-�a�Ѐ8�ҝj5��T����`;�Rȉ��BN)���/;ܠ�R
�f�g*J�9ۯOo�k@�sϋ@�ɱ����׾���2��̱�0;,w:��#ZM�ge����.�2J)���,+�Xl���o'�!�|'�2����-?���"�4c�=P�e�"&%��<��)�$c�e|[���xT�X%Ūs�xb�t,�A`[��l���2bL�i�&`ͳZiqB����F��_1�4��XbQbR��`�R�i�����u(�tU%%FU��Rqn��q��rN)��������K�~r�QZ��;�TSJ����^�:���/��)�R���_�
t��c3�7�^�7����D�K�_>{@}�%�)�.X����9��*�R)0��	�ױ��%�8g��1l�ȱ}��ρ����X}�
 ��6���0�@��Ƈ�+,s�:q\ES�ksk^;x!��ЁB�F���5q�Ϙ����bn� o��� mA9b�ǎ�̬[g��澄�i��JOc�����,(���Ge��++���+������Yf�dM���pg�-{�B�T�}_c
٧R*����
I��B#Ky�$E�I%�E�{gJ���Y�<��=/3��s��>���}νg��`8&�r �9����mN�R���h8�����u�w�7�����°�`q��]Z���X�!�!�وY�|���{VgV?��hc@m���a��KK��T*TC���0,���t�� �`���Cw74-E�rW� #M+@#,7�]��1��� �e�����KK���
����ɿ#���$@L�p#&XqpV�����I�k������$0hj׷0D��	���X�M}����}++8K����@�V��;<���D�38<h&���Lp<�ҐH�0pv�� 
g�9����kpу����B�ե!�A$~ut���2�����8��/�/ b���-ǩ8*����C Ϻ�����4���P]���p�p��*��l�2lF��e���v��f���wAmdcNq�
á ďf��i����
�c�v�}`ry�&@��c`�X�:-	Zq��8-�xm����4����b�C�A'�y�!\����PcW`Ѱ��^��Ri&\5@�А��1��0��B�#�x�������V3@k�jɫ�������EWY�U@?����.D����v��D��Ac���0��@��� 4���n(88��e���6}����7p++ �ؾ�a�=�@xy;��o���C]�D#CDMB=�h�hD�]�A�G+N��*����	���h�D��Ox8"�"�&�"�p"@33ɳ!L0��#�G�)j8*��R�	��F��Vړ�`ڻ�ɾ 1��P��[�li�3�s���9i��غ���~O���di>G�2��V���������
�W[>�5i�b�B��� N(�ʅqp���&9 6(������#08ȃ(B).�_nW%�"P>���G��o�'���"je��� @���������3D����ex �c�P[�l�Ҁ$=*d�ma�dk$jdk���n��`��hD0ڠB$J��	��G�� ��q��L0$\����Z�7k"9��:q�f@(�f��:G�YV����K�Ӗ���KI��vF�~�y��;aA���bSo\��K�1I9��>8��ݛF�N�Yc��J�q�,�����ԁ� I`|BD�p7;'�@�AI?�h�i~��S߮��H��O�C%S�.�,k��*���J��	W0Aw_Ψ{zd�Q�~a�?��)�o�n�5<�s��C���qB�E�E������F���(M��J���3���_�w�}�&��7)W��L�p u?O���a��!bC�ه=��%SO��v�M־����!!q?��-�����83��eߗJ�r�j%;`] �� � f�&�F����˸���_&��JU��u3LiՌ�iFQ�^lgf���G����q ���O7��*����� �^I�|/���C"!�d���\cy1�|�}�l�f�}����Ut�-m�g�����]-}��5�KGG���������ls�#ye�ڸ�y;��h�A�%[���k�M�	�:UȾ��#%��������e��U�L��f�kE��i��i��qu8�[�9�����8+�N�C��r�p�_�c}=�o�}�Z�t����MR���;���/�ƨ�x?���8����itpf~e�Vtd����T��?�ܰ�4�nx�Ѿ39h���0���8���ȷ'�;�Ap0��46C�1�+��
��f/g-`�iRI��)x���"A[�|���^5� ���X���$�����ݕ�)�����K��X
PT e"IU� Y�D�E�(���Ͽ#������~�4�hŵ�u#��N[�[]�x�o)���a�C�+d@��5�c���uiW�vR�`�/���$��bCe�&��<P�8�;��[Pa)j�ЫI�������:�]�˺�P�>���Df�ض<�k\�XQ�r�V{�R�_j*tx~��ۛY�R,3v��k���m��u��<S�f/NiY���c��6��|��ӄM�;8m���b=� ��o�7�i"��	G���s��V����W,}ʹ�X��n�3��(�;є�#R�C������ $���<�$H#��Q(� 3��P�����Eg��T !9���2�Eo�t�$ג�0�6c�؅���:<)�Wx6�^1����K���S�`���'��sJ\͆6�C\F�C;:4��Mt����!�Cz���
����<Jܴ݅����+�����
E��@Wiǝ�Պ=k�t�v@<��:�|�ވCM��l�a!x�嚈�c]o��oG��0�&�����<�Z�66���8�q"}�Y�rꤜ���/��"N+�?�����[��cB3�4�x�Y��rۥ˛uLTw�E�����yQ[>��*D{��k�.�5e�I5�k�cq�T�w��]�5��{ܯ������p�u?�������J����C����^憤\�0��Y����̹Y�p�6m�@�,�E��Ӱr�󛘪�ɪՖ�����ڇ�hs|�NcC����r��<���H���<��K�5�ZϮ�2����yoڮ��G�7��3�p��j�Lыӹ�5����̜o����P��nT�i'�s�T�UT�dvM�ף��k˃֭��ۼ�r���@h�v���*��~=�G_��rzyaDie���O�s�]�D����x\�>��}����[۲ߚ}�{'acZ}['�^gveW��9��XWз�tVWў׿���WP
8A)��0�M9|򂃀�}�&�0�jB�B�2��1E�=<Em}�i{�AR�~�E�Ol�D��h���-i5
$����~��'���~t�1���}ڼ��u�'��67���fâ]})G�T�d��-�nX}`/l60��|��/̈>v���1�ȏ���6�zp������;:æ��h��vH� t�9bfb{IlN�5�7gQ�E�����ņf.��DF!�5�=��w����6n��s���/����	�]�ErT��J��Ba�x��n��ϯ���X��s�}sJ_���|�ʮ�dy��	�	/���b��k?W���n��������c�'l�2~����٦�3����]?�����D%����͜��\H=)m흾��})�]�t��'��]Gk (�NS/�����T e�') �$���<�j�҃��B�"����T�Uԉ�*DU@�?fǦA�VL�(�����W^������_�upHM@���Q³3�A ��:M\�� �r`��|W�����V��~ro�(lJB\���o�Q�s��v�~�E3_�;����5�r��~��9����b��^LK��_hbsQx���[�l**C��r��6�R�ѓU�;;J�"���=!Gէ�Z�VF����StF���>�3=�t��3�ߕq\�;t����E�.,����k^]L�w�.��1�Av{o�l�3�w'��Ū���|^u�r��[B���h��r�	�vB�cϒ�F�HZۭ�n̥��6��#Y�)�%;�$�R(#�0�S��yqQ)G�G�l/�ɞ .�����XF��=�S�
ygP���Ziϰwd�O��������'��7�~��z��^�ꊱȌ$����)�}&"���-��V�b�׋�Ğ"���v�N���&_��ɫ�k��sߒB�l�j�E`�v������%��S���2:��D�ͺC����1p�f�P�Ӻ�v�OA&ʏ���+ZWW�ꤢi��Nq]|��Ee?Y%�ͯ�׫�2�������Nq���ڔW�4�Tm��<$3��: �S�E�c/�Z왈t-@a�P�ܾ��R�iZ��Y��+\K :����Y�Y �k	u��A_Il�E� ����GA@@�� �����PA�+}A�(*��nl�3X'�?���R5�� ��v'�\ͿyMvV��i��V��pn���$|����t����fg�|vw��^�(P˞��_ݏ.{rj�}�)�}/I6Rk�&KY�:+7�t�kRD�-��8��������!u��A�E���Gz�-��`�?�Yh&;����~']{�˨�N�m2����Ѽi��oz��䁧Qע�=թ8��2i�A���c�Z�������Y&=�DjW\+KS��΍W�`�����$U#�#����+k�>x����y�˦�x���1��3�̍viu�,��:�+Ү
��x�q��1�u>2"��Hl��MS���r��~G���!N���������sOeݺ�-��S�f��՛�������B'%�Oߛ�+�� ���P\�2��1�l�y��HcfB�t���I3�b.�������n�ה>q8��>\F��t U��	=���Ç6%5����*�3>-D�nW�rr���٪|�Y��g-ߟ*�5����=r|uA5
��/�D?�b�v�n
'���Ҷ����s�����̅4�T�[�L�ï�[�œT��t���nͷ�ߒh�_=��Z�YW��Pv&�hb��A�l +��A�����/�'	yP�EQ	�@B:�pru���<��^Ϋ�w�����7�54�=$Lч�� T�I�B0s�'̛��י����[<��}���_QT�W��=�X�5d)�����>{�s�=������H\j������5�a���;�/1���MJ�����ׯW-�t�<�s��ʾ��-�Ep~��1�����:,��9��
{����5F�<�6iG���)J%H~�@
a,w�(Gͅ&\δϢ�bF�����v3w�o�άSc�7��7����U�:$yu�Lz���<[�i��,6m�kR���?2{���x����s�����kO�y�XmW'���x/��BAS�7�ڎ$����d�s\NF���e�m���0|"N0�'ƨx߸���y���["U�}Cf����9ނl���3��:��:��9������b��Wתف�u�;��-l2o_x*���X��T����n�|f!z�)��x�.�콊�B]�pa���s����_�h�!R��D<��مz��[2G^�x�J�t$����Jq��e�K�^c�_�j�-�(������'��n��Zﰾ ��$�FS���b��Ƃ#9{�Y�5���s�ě���k��k�	�����HA��e$d�����k�/K�� ��b4��c|��GKdsy �7�� ��y}0��'kH0��*6��,�#���� �|�x�_������@*^�O=�n�G��?i3����Q4��\+�1�w95������$+��	�1\eNK��b�O�L�E0/�ӻ��[�?j6�2���u����μQ��%��Mp�lؕ}�{c�t����`5*XIh��o.���s���-���×�W�ueb*o_wIy�ϗ���\����7t	}h�@��!�<�ZFRX�8E���XL����jީ������j%k��}N	�Y�4��?&y��U[Ԝ����قۤ��c���P�,ue���.�49:1(�s��i�*�Mk���P2��D⇍����I�f�����߼1`�	�g�$��D8X�j��5�j$����LT��DN]���k0�����XS8��s4+�dL�j)Ai��ɰjK����S�Ո-���&���U'���u:��۱+�>*���|�ХX&�������/;�n��	����T9�<��y�퓛�lh��f��S�qade<�y�*r��P�{�\�,g�B����T���������Eݶ�7�'>���t��<���Zlpiçkt�S�}c�����f�b�c�t�FTL4�b�>� U��t���?�>鸎}�(���Y�yZv�P��8��C�*%'c����If�^��7i��
endstream
endobj
120 0 obj
[ 278] 
endobj
121 0 obj
<</Type/Metadata/Subtype/XML/Length 3064>>
stream
<?xpacket begin="﻿" id="W5M0MpCehiHzreSzNTczkc9d"?><x:xmpmeta xmlns:x="adobe:ns:meta/" x:xmptk="3.1-701">
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
<rdf:Description rdf:about=""  xmlns:pdf="http://ns.adobe.com/pdf/1.3/">
<pdf:Producer>Microsoft® Word 2016</pdf:Producer></rdf:Description>
<rdf:Description rdf:about=""  xmlns:dc="http://purl.org/dc/elements/1.1/">
<dc:creator><rdf:Seq><rdf:li>Stephen Comino</rdf:li></rdf:Seq></dc:creator></rdf:Description>
<rdf:Description rdf:about=""  xmlns:xmp="http://ns.adobe.com/xap/1.0/">
<xmp:CreatorTool>Microsoft® Word 2016</xmp:CreatorTool><xmp:CreateDate>2019-11-21T20:56:45+11:00</xmp:CreateDate><xmp:ModifyDate>2019-11-21T20:56:45+11:00</xmp:ModifyDate></rdf:Description>
<rdf:Description rdf:about=""  xmlns:xmpMM="http://ns.adobe.com/xap/1.0/mm/">
<xmpMM:DocumentID>uuid:8F777ACA-E71D-4DEF-80BF-B0BE9A12ECF6</xmpMM:DocumentID><xmpMM:InstanceID>uuid:8F777ACA-E71D-4DEF-80BF-B0BE9A12ECF6</xmpMM:InstanceID></rdf:Description>
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                                    
</rdf:RDF></x:xmpmeta><?xpacket end="w"?>
endstream
endobj
122 0 obj
<</DisplayDocTitle true>>
endobj
123 0 obj
<</Type/XRef/Size 123/W[ 1 4 2] /Root 1 0 R/Info 29 0 R/ID[<CA7A778F1DE7EF4D80BFB0BE9A12ECF6><CA7A778F1DE7EF4D80BFB0BE9A12ECF6>] /Filter/FlateDecode/Length 318>>
stream
x�5�=/CQ��Ӗ���޶�EQ�c[Z���S��0�� �X�� ��"��� �XH����g8��������(e�v�f��L��S�go~߇�Op¯<�[!��B�L��-��&D�E�=q.�B�H�5���P96*�S�h�&l�r˘��['���	N�n��B��`<0^��n4�ǭf�Cb��9в�"0f&O�d�#0)�q��4L�LCf`�``���<,�2�@��%X�2T�
5X�u�%���4�Llڽ{t��(	E��]���&��<	y�R��7�
endstream
endobj
xref
0 124
0000000030 65535 f
0000000017 00000 n
0000000168 00000 n
0000000238 00000 n
0000000557 00000 n
0000004947 00000 n
0000005116 00000 n
0000005356 00000 n
0000005409 00000 n
0000005462 00000 n
0000005638 00000 n
0000005885 00000 n
0000006024 00000 n
0000006054 00000 n
0000006221 00000 n
0000006295 00000 n
0000006542 00000 n
0000006718 00000 n
0000006964 00000 n
0000007097 00000 n
0000007127 00000 n
0000007288 00000 n
0000007362 00000 n
0000007603 00000 n
0000007766 00000 n
0000007993 00000 n
0000008314 00000 n
0000013639 00000 n
0000013940 00000 n
0000016260 00000 n
0000000031 65535 f
0000000032 65535 f
0000000033 65535 f
0000000034 65535 f
0000000035 65535 f
0000000036 65535 f
0000000037 65535 f
0000000038 65535 f
0000000039 65535 f
0000000040 65535 f
0000000041 65535 f
0000000042 65535 f
0000000043 65535 f
0000000044 65535 f
0000000045 65535 f
0000000046 65535 f
0000000047 65535 f
0000000048 65535 f
0000000049 65535 f
0000000050 65535 f
0000000051 65535 f
0000000052 65535 f
0000000053 65535 f
0000000054 65535 f
0000000055 65535 f
0000000056 65535 f
0000000057 65535 f
0000000058 65535 f
0000000060 65535 f
0000017694 00000 n
0000000061 65535 f
0000000062 65535 f
0000000063 65535 f
0000000064 65535 f
0000000065 65535 f
0000000067 65535 f
0000017747 00000 n
0000000068 65535 f
0000000069 65535 f
0000000070 65535 f
0000000071 65535 f
0000000073 65535 f
0000017803 00000 n
0000000074 65535 f
0000000075 65535 f
0000000076 65535 f
0000000077 65535 f
0000000078 65535 f
0000000079 65535 f
0000000080 65535 f
0000000081 65535 f
0000000082 65535 f
0000000083 65535 f
0000000084 65535 f
0000000085 65535 f
0000000086 65535 f
0000000087 65535 f
0000000088 65535 f
0000000089 65535 f
0000000090 65535 f
0000000091 65535 f
0000000092 65535 f
0000000093 65535 f
0000000094 65535 f
0000000095 65535 f
0000000096 65535 f
0000000097 65535 f
0000000098 65535 f
0000000099 65535 f
0000000100 65535 f
0000000101 65535 f
0000000102 65535 f
0000000103 65535 f
0000000104 65535 f
0000000105 65535 f
0000000106 65535 f
0000000107 65535 f
0000000108 65535 f
0000000109 65535 f
0000000000 65535 f
0000017856 00000 n
0000018167 00000 n
0000072620 00000 n
0000073169 00000 n
0000073485 00000 n
0000073788 00000 n
0000105274 00000 n
0000105699 00000 n
0000105979 00000 n
0000106243 00000 n
0000141730 00000 n
0000141758 00000 n
0000144906 00000 n
0000144952 00000 n
trailer
<</Size 124/Root 1 0 R/Info 29 0 R/ID[<CA7A778F1DE7EF4D80BFB0BE9A12ECF6><CA7A778F1DE7EF4D80BFB0BE9A12ECF6>] >>
startxref
145473
%%EOF
xref
0 0
trailer
<</Size 124/Root 1 0 R/Info 29 0 R/ID[<CA7A778F1DE7EF4D80BFB0BE9A12ECF6><CA7A778F1DE7EF4D80BFB0BE9A12ECF6>] /Prev 145473/XRefStm 144952>>
startxref
148113
%%EOF                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        