#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "generic_tcp_client_template/tcp_client.h"
#include "generic_unix_tools/generic_unix_tools.h"
#include "read_config/config.h"
#include "irclib.h"

/* This file demonstrates usage */

int main(void)
{
    printf("Retrieving automated responses...\n");
    int lineCnt = countLines("replies.txt");
    aR replies[lineCnt + 1];
    if(retrieveAutomatedReplies(replies, "replies.txt") == -1){
        printf("There is an error in your replies configuration\n");
        return 1;
    }
    
    chanList *chans = malloc(sizeof(chanList));
    chans->next = NULL;

    //Retrieve channels in this file
    getChannelsFromFile(chans, "channels.txt");

    //Retreive configuration parameters
    appConfig config;
    if(getConfig(&config, "config.txt") == 1){ //Offload this later as parameter
        printf("Loading of config failed, file '%s'\n", "config.txt");
        return 1;
    }

    //Connect to server and retrieve socket
    int clientSocket = connectToServer(config.serverName, config.serverPort, 12);

    if(clientSocket == 0){
        printf("Connect failed with code %d\n", clientSocket);
        return 1;
    }

    int rc = ircLogin(&config, &clientSocket);

    if(rc > 0){
        printf("Error logging in, rc %d\n", rc);
        return 1;
    }
    
    //printf("\nRequesting all channels...\n");
    //size_t max = 100;
    //if(getAllChannels(&clientSocket, chans, max) == -2){
    //    printf("Recall getallchans..\n");
    //    rc = getAllChannels(&clientSocket, chans, max);
    //}

    rc = joinChannels(&clientSocket, chans);

    //Running parsing of responses
    printf("Starting parseResponses..\n");
    parseResponses(&clientSocket, replies, &config);
    
    //Spawn "shell", mainly for debugging
    //spawnShell(&clientSocket);

    close(clientSocket);

    freeChannels(chans);

    return 0;
}

