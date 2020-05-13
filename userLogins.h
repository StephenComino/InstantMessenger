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
