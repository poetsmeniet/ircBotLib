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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include "irclib.h"
#define MAXLEN 80000
#define RESPBUFSZ 2

//Returns user name from privmsg
int returnUserName(char *line, char *target)
{
    int len = strlen(line);
    int i;
    for(i = 1; i < len; i++){
        if(line[i] == '!' && line[i + 1] == '~'){
            target[i - 1] = '\0';
            return 1;
        }else{
            target[i - 1] = line[i];
        }
    }
    return 0;
}

/*Copies token at specified index (channel = 3)
 * Returns 0 if token is not found
 * Return 1 if token is found */
int returnTokenAtIndex(char *line, int index, char *target)
{
    //For each line in response look for channel names
    char respCpy[MAXLEN];
    char *sp1;
    memcpy(respCpy, line, strlen(line));

    //Tokenise line using whitespace
    char *token = strtok_r(respCpy, " ", &sp1);
    int tokCnt = 0;
    while(token != NULL){
        if(tokCnt == index && token[0] == '#'){
            //NULL is allowed as target
            if(target != NULL){
                //Copy to target
                size_t len = strlen(token);
                memcpy(target, token, len); 
                target[len] = '\0';
            }
            return 1;
        }
    
        token = strtok_r(NULL, " ", &sp1);
        tokCnt++;
    }

    return 0;
}

//Generated automatic responses and delivers msg accordingly
void genAutoResponses(aR *replies, char *line, hashMap *respCnts, int *clientSocket)
{
    int cnt = 0;
    while(bcmp(replies[cnt].regex, "EOA\0", 4) != 0){
        if(regexMatch(replies[cnt].regex, line) == 0){
            int rc2;

            //Retrieve channel name
            char respChan[100];
            rc2 = returnTokenAtIndex(line, 2, respChan);

            //Get user name to respond to
            char respUsr[100];
            returnUserName(line, respUsr);

            //Compose the private message to user
            char thisReply[400];
            thisReply[0] = '\0';

            //Compose private user message
            if(rc2 == 0 && replies[cnt].privateMsgFlag == 1){
                printf("\tUser to reply to: '%s'\n", respUsr);
                sprintf(thisReply, "PRIVMSG %s :%s\n", respUsr, replies[cnt].reply);
                
                //Test if repeatMsgCnt has not been depleted
                if(getValue(respCnts, respUsr, cnt, 0) >= replies[cnt].repeatMsgCnt\
                        && getValue(respCnts, respUsr, cnt, 1) == cnt){
                    printf("\t---Suppressing this private user response\n\n");
                    break;
                }else{
                    //Add keys to list (response bookkeeping)
                    addKey(respCnts, respUsr, cnt, strlen(respUsr));
                }
            }
            
            //Compose public channel message
            if(rc2 == 1 && replies[cnt].privateMsgFlag == 0){
                printf("\tchannel to reply to: '%s'\n", respChan);
                sprintf(thisReply, "PRIVMSG %s :%s\n", respChan, replies[cnt].reply);

                //Test if repeatMsgCnt has not been depleted
                if(getValue(respCnts, respChan, cnt, 0) >= replies[cnt].repeatMsgCnt\
                        && getValue(respCnts, respChan, cnt, 1) == cnt){
                    printf("\t---Suppressing this channel message\n");
                    break;
                }else{
                    //Add keys to list
                    addKey(respCnts, respChan, cnt, strlen(respChan));
                }
            }
        
            //Send message
            if(strlen(thisReply) > 0){
                printf("\tLine: '%s'\n\tComposed response: '%s'",line , thisReply);
                int rc = sendMessage(clientSocket, thisReply, strlen(thisReply));

                if(rc == 0){
                    printf("do some error handling dude\n");
                }
                
                break;
            }
        }

        cnt++;
    }//End of while bcmp of line with replies
}

/* Gets Automated replies, triggered by regex
 * returns: nr of replies found in file */
extern int retrieveAutomatedReplies(aR *replies, char *fileName)
{
    //First delete all lines
    memcpy(replies[0].regex, "EOA\0", 4); //Superfluous, ok

    FILE *fp;
    int lineNr = 0;
    int lineCnt = 0;
    fp = fopen(fileName, "r");

    if(fp != NULL){
        while(!feof(fp)){
            lineCnt++;
            int privateMsgFlag; //Private or channel msg
            int repeatMsgCnt; //Msg is to be repeated or not
            char regex[MAXLEN]; //Trigger
            char reply[MAXLEN]; //String reply

            int rc = fscanf(fp, "%d %d %s %200[^\n]\n", &privateMsgFlag, &repeatMsgCnt, &regex[0], reply);
            if(rc == 0){
                printf("\tError in '%s', line number %d\n", fileName, lineCnt - 1);
                return -1;
            }

            size_t keyLen = strlen(regex);
            size_t valLen = strlen(reply);
            if(keyLen > MAXLEN || valLen > MAXLEN){
               printf("\nSorry, maximum length of key value exceeded (%d)\n", MAXLEN);
               return 0;
            }

            //Check length of strings and store
            // * temporarily added check for privateMsgFlag
            if(strlen(regex) > 0 && strlen(reply) > 0 && privateMsgFlag < 2){
                //Compile regex, case insensitive
                regex_t preg;
                int rc = regcomp(&preg, regex, REG_ICASE);
                if(rc != 0){
                    printf("\tRegex compilation failed, rc = %d (%s)\n", rc, regex);
                }else{
                    //Add reply parameters to struct
                    replies[lineNr].privateMsgFlag = privateMsgFlag;
                    replies[lineNr].repeatMsgCnt = repeatMsgCnt;
                    memcpy(replies[lineNr].regex, regex, strlen(regex));
                    replies[lineNr].regex[keyLen] = '\0';
                    memcpy(replies[lineNr].reply, reply, strlen(reply));
                    replies[lineNr].reply[valLen] = '\0';
                    lineNr++;
                }
                regfree(&preg);
            }
        }
    }else{
        printf("Unable to open '%s' for reading. Not loading automated responses\n", fileName);
        memcpy(replies[lineNr].regex, "EOA\0", 4);
        return 0;
    }

    //Terminate this array
    memcpy(replies[lineNr].regex, "EOA\0", 4);
    fclose(fp);

    return lineNr;;
}

//Call "list" and store all channels into channel list
extern int getAllChannels(int *clientSocket, chanList *chans, int max)
{
    //Call list command
    int rc = sendMessage(clientSocket, "list\n", 5);

    if(rc == 0)
        return 1;
    
    respBuf *responses = malloc(RESPBUFSZ * sizeof(respBuf));

    int chanCnt = 0;

    //Get all response data until socket times out
    while(1){
        if(chanCnt >= max){
            printf("Max channels reached (%d), breaking off here\n", max);
            free(responses->buffer);
            free(responses);
            sleep(3);
            return 0;
        }
        int rc = recvMessage(clientSocket, responses, 1);
        
        //No response data after socket timeout
        if(rc == -1){
            free(responses->buffer);
            free(responses);
            return 1;
        }

        char respCpy[MAXLEN];
        if(strlen(responses->buffer) > MAXLEN){
            printf("\n\nTrying to FIT %zu into %d will not work. Resonse sz too graet\n\n", strlen(responses->buffer), MAXLEN);
            return 1;
        }
        memcpy(respCpy, responses->buffer, strlen(responses->buffer));

        //Parse data line by line
        char *line = NULL;
        line = strtok(respCpy, "\r\n");
        while(line != NULL){
            char newChannel[200];
            int rc2 = returnTokenAtIndex(line, 3, newChannel);
            if(rc2 == 1){
                //Add to channel struct
                addChannel(chans, newChannel);
                chanCnt++;
            }else{
                //End of response data?
                if(strstr(line, "End of /LIST") != NULL){
                    if(chanCnt == 0){
                        printf("Found %d channels..\n", chanCnt);
                        free(responses->buffer);
                        free(responses);
                        return -2;
                    }

                }
            }
            line = strtok(NULL, "\r\n");
        }
        
        free(responses->buffer);
    }

    free(responses->buffer);
    free(responses);
    return 0;
}

extern int parseResponses(int *clientSocket, aR *replies, appConfig *config)
{
    //Allocate some memory for server responses
    respBuf *responses = malloc(RESPBUFSZ * sizeof(respBuf));
    
    //Declare hash map for storing response counts per user/channel
    hashMap *respCnts = malloc(ASCIIEND * sizeof(hashMap)); 
    respCnts->totalCnt = 0;
    generateHashMap(respCnts);

    while(1){
        int rc = recvMessage(clientSocket, responses, 1);

        if(rc == -1) //No response data after socket timeout
            continue;

        if(rc == -2){//Something whent horribly wrong..
            printf("TCP error?, quitting parseresponse\n");
            free(responses->buffer);
            free(responses);
            return -2;
        }

        //Copy responses buffer
        int len = strlen(responses->buffer);
        //char *respCpy = malloc(len * sizeof(char));
        char respCpy[MAXLEN];
        memcpy(respCpy, responses->buffer, len);
        respCpy[len] = '\0';
        char *line = strtok(respCpy, "\r\n");

        //Parse each line
        while(line != NULL){
            //Disconnect bot.. if politely asked. Failsafe hardcoded..
            char quitString[100];
            sprintf(quitString, "%s, please disconnect", config->nick);
            if(regexMatch(quitString, line) == 0){
                    printf("\nDisconnecting as requested\n");
                    freeHashMap(respCnts);
                    free(responses->buffer);
                    free(responses);
                    return 1;
            }

            //Automatic ping
            if(strstr(line, "PING") != NULL){
                printf("\tReplying to ping..\n");
                int rc = sendMessage(clientSocket, "pong\n", 5);
                if(rc == 0){
                    free(responses->buffer);
                    free(responses);
                    return 1;
                }

                //Continue, dont parse responses
                break;
            }

            //Check all automated responses and reply accordingly
            genAutoResponses(replies, line, respCnts, clientSocket);

            //Get the next line
            line = strtok(NULL, "\r\n");
        }
        //End of while line != NULL

        responses->buffer[0] = '\0';
        
        //Reload replies at runtime (optional)
        if(retrieveAutomatedReplies(replies, "replies.txt") == -1){
            printf("There is an error in your replies configuration\n");
            return 1;
        }

        free(responses->buffer);
    }

    free(responses->buffer);
    free(responses);
    freeHashMap(respCnts);
    return 0;
}
    
//Join channels..
extern int joinChannels(int *clientSocket, chanList *chans)
{
    chanList *head = chans->next;
    int cnt = 0;

    if(head == NULL){
        printf("\tNo channels to join\n");
        return 1;
    }

    respBuf *responses = malloc(RESPBUFSZ * sizeof(respBuf));
    char cmd[86];
    int rc;

    while(head != NULL){
        printf("Joining channel: %s\n", head->chanName);
        cnt++;
        snprintf(cmd, MAXLEN, "join %s\n", head->chanName);

        rc = sendMessage(clientSocket, cmd, strlen(cmd));
        if(rc == 0){
            free(responses->buffer);
            free(responses);
            return 1;
        }
        
        while(recvMessage(clientSocket, responses, 1) != -1){
            printf("Server: %s", responses->buffer);

            //End of response data
            if(strstr(responses->buffer, "End of /NAMES list") != NULL){
                free(responses->buffer);
                break;
            }
            //Too many channels..
            if(strstr(responses->buffer, "You have joined too many channels") != NULL){
                free(responses->buffer);
                printf("Done joining channels, server limit reached..\n");
                return 0;
            }

            free(responses->buffer);
        }

        head = head->next;
    } 
    
    free(responses);
    return 0;
}

//Spawns interactive session to IRC server
//- mainly for debugging
extern int spawnShell(int *clientSocket)
{
    printf("Spawning shell..\n");

    //Loop stdin for issueing commands
    char cmd[86] = "Placeholder";
    respBuf *responses = malloc(RESPBUFSZ * sizeof(respBuf));
    
    while(1){
        memset(cmd, 0, strlen(cmd));
        printf("> ");
        fgets(cmd, MAXLEN, stdin);
        
        int rc = sendMessage(clientSocket, cmd, strlen(cmd));
        if(rc == 0){
            free(responses->buffer);
            free(responses);
            return 1;
        }
        
        recvMessage(clientSocket, responses, 1);
        printf("%s", responses->buffer);

        if(strstr(responses->buffer, "Quit") != NULL &&\
                strncmp(cmd, "quit", 4) == 0){
            free(responses->buffer);
            free(responses);
            return 0;
        }
        
        if(strstr(responses->buffer, "PING") != NULL){
            printf("Replying to ping..\n");
            rc = sendMessage(clientSocket, "pong\n", 5);
            if(rc == 0){
                free(responses->buffer);
                free(responses);
                return 1;
            }
        }

        memset(cmd, 0, strlen(cmd));
        free(responses->buffer);
    }
    
    free(responses->buffer);
    free(responses);
    return 0;
}

//logs into connected irc server using specified data
extern int ircLogin(appConfig *ircData, int *clientSocket)
{
    //Send a message and get response(s) irc
    int rc;

    respBuf *responses = malloc(RESPBUFSZ * sizeof(respBuf));

    while(recvMessage(clientSocket, responses, 1)){
        printf("%s", responses->buffer);
        
        //End of login function success
        if(strstr(responses->buffer, " MODE ") != NULL){
            free(responses->buffer);
            free(responses);
            return 0;
        }else if(strstr(responses->buffer, "Nickname is already in us") != NULL){
            printf("\tDetected NICK in use\n");
            free(responses->buffer);
            free(responses);
            return 2;
        }else if(strstr(responses->buffer, "Looking up") != NULL){
            //Respond to Checking Ident by sending login data
            printf("\tLogging in as '%s' len=%zu..\n", ircData->nick, strlen(ircData->nick));
            char req[86];

            //Send Nick
            snprintf(req, MAXLEN, "NICK %s\n", ircData->nick);
            rc = sendMessage(clientSocket, req, strlen(req));
            if(rc == 0){
                free(responses->buffer);
                free(responses);
                return 1;
            }

            memset(req,0,strlen(req));

            //Send userName
            snprintf(req, MAXLEN, "USER %s 0 * :Test Bot\n", ircData->userName);
            rc = sendMessage(clientSocket, req, strlen(req));

            if(rc == 0){
                free(responses->buffer);
                free(responses);
                return 1;
            }
        }else{
            //Nothing, just loop
        }
        free(responses->buffer);
    }
    free(responses->buffer);
    free(responses);

    return 0;
}

//Free channels linked list
extern void freeChannels(chanList *targetList)
{
    //Onlye one element, otherwise free consecutive elements
    if(targetList->next == NULL){
        free(targetList);
    }else{
        chanList *head = targetList;
        chanList *curr;
        while ((curr = head) != NULL) { // set curr to head, stop if list empty.
            head = head->next;          // advance head to next element.
            free (curr);                // delete saved pointer.
        }
    }
}

extern int addChannel(chanList *chans, char *channelName)
{
    chanList *curr = chans;
    while(curr->next != NULL){
        curr = curr->next;
        //check for collisions
        if(strncmp(curr->chanName, channelName, strlen(channelName)) == 0){
            printf("COllision on '%s', skipping add..\n", channelName);
            return 0;
        }

    }

    //Add channel to list
    curr->next = malloc(sizeof(chanList));
    memcpy(curr->next->chanName, channelName, strlen(channelName));
    curr->next->chanName[strlen(channelName)] = '\0';
    curr->next->next = NULL;
    printf("Added channel '%s'\n", channelName);

    return 0;
}

//A quick write for retrieving channels frome external file
extern int getChannelsFromFile(chanList *chans, char *fileName)
{
    size_t lineNr = 0;
    FILE *fp;
    fp = fopen(fileName, "r");

    if(fp != NULL){
        while(!feof(fp)){
            lineNr++;
            char sVal[MAXLEN];

            int rc = fscanf(fp, "%s", sVal);

            if(rc == 0){
                printf("\tError in '%s', line number %zu\n", fileName, lineNr);
                return -1;
            }

            size_t valLen = strlen(sVal) + 1;
            if(valLen > MAXLEN){
               printf("\nSorry, maximum length of channel name exceeded (%d)\n", MAXLEN);
               return 1;
            }
            
            if(rc == 1)
                addChannel(chans, sVal);
        }
    }else{
        //Pocess error
        printf("There was an issue loading '%s'\n", fileName);
        return 1;
    }
    fclose(fp);
    return 0;
}
