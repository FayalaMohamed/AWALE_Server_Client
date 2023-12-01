#ifndef CLIENT_H
#define CLIENT_H

#include "server2.h"
#include <stdbool.h>

struct Client
{
   SOCKET sock;
   char name[100];
   uint8_t gameId;
   uint8_t gamesWon;
   int isPlaying;
   char *pseudoAdversaire;
   bool observe;
} typedef Client;

#endif /* guard */
