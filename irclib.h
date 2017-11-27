/*
irclib

Copyright (c) 2017 Thomas Wink <thomas@geenbs.nl>

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef irclib_H_
#define irclib_H_
#define MAXREGEXSZ 200
#define MAXREPLYSZ 500

//The chanList linked list stores 1 or more channels
typedef struct chanList{
    char chanName[100];
    struct chanList *next;
}chanList;

typedef struct automaticReplies{
    int privateMsgFlag; //Private or channel msg
    int repeatMsgCnt; //Msg is to be repeated or not
    char regex[MAXREGEXSZ]; //This regex match will trigger reply
    char reply[MAXREPLYSZ]; //Reply body
}aR;

//DEPRE:
//This struct stores initial data needed for login
//and channel entry
typedef struct ircdata{
    chanList *chans;
}ircData;

//logs into connected irc server using specified data
extern int ircLogin(appConfig *ircData, int *clientSocket);

//Spawns interactive session to IRC server
//- mainly for debugging
extern int spawnShell(int *clientSocket);

//Join channels..
extern int joinChannels(int *clientSocket, chanList *chans);

//Parse respones loop, add your own code here
extern int parseResponses(int *clientSocket, aR *replies, appConfig *config);

//Call "list" and store all channels into channel list
extern int getAllChannels(int *clientSocket, chanList *chans, int max);

//Retrieves all automated responses, including regex triggers
extern int retrieveAutomatedReplies(aR *replies, char *fileName);

//Free channels linked list
extern void freeChannels(chanList *targetList);

//Adds a channel to channel list
extern int addChannel(chanList *chans, char *channelName);

//A quick write for retrieving channels frome external file
extern int getChannelsFromFile(chanList *chans, char *fileName);

#endif
