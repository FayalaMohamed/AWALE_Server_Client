#ifndef CLIENT_H
#define CLIENT_H

#include "server2.h"

struct Client
{
   SOCKET sock;
   char name[BUF_SIZE];
   uint8_t gameId;
   uint8_t gamesWon;
   int isPlaying;
   char* pseudoAdversaire;
} typedef Client;

#endif /* guard */
