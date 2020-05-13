/*
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



