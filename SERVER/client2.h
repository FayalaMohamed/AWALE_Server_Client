#ifndef CLIENT_H
#define CLIENT_H

#include "server2.h"

struct Client
{
   SOCKET sock;
   char name[BUF_SIZE];
   int gameId;
   int score;
   int isPlaying;
} typedef Client;

#endif /* guard */
