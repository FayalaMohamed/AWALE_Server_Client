#ifndef CLIENT_H
#define CLIENT_H

#include "server2.h"
#include <stdbool.h>

struct Client
{
   SOCKET sock;
   char name[100];
   uint8_t gameId; // saves the id of the game which the player plays at the moment
   uint8_t gamesWon;
   int isPlaying; //used to check wether a player can be challenged to play a game
   char* pseudoAdversaire; //saves the opponent for each game
   char bio[BUF_SIZE/2]; 
   bool observe; 
} typedef Client;

#endif /* guard */
