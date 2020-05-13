/*
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
