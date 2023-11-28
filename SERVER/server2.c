#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server2.h"
#include "client2.h"
#include "awale.h"

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if (err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

/*void PlayGame(Game *game, int gameId)
{
   int beginPlayer = 1 + rand() % (2)
}*/

void InitGame(Game *game, int gameId, int socket_joueur1, int socket_joueur2)
{
   game->gameId = gameId;
   game->socket_joueur1 = socket_joueur1;
   game->socket_joueur2 = socket_joueur2;
   game->nb_tours = 0;
   initPartie(&game->plateau, &game->score_joueur1, &game->score_joueur2, &game->sens_rotation);

   char *buffer;
   buffer = (char *)malloc(BUF_SIZE*sizeof(char));
   buffer[0] = 0;
   createPlateauMessage(buffer, game);
   printf("buffer = %s\n", buffer);
   write_client(game->socket_joueur1, buffer);
   free(buffer);
   // PlayGame(&game, gameId);
}

void sendMenu(Client c)
{
   write_client(c.sock, "1.  Afficher la liste des pseudos en ligne\n");
   write_client(c.sock, "2.  Choisir un adversaire \n");
}

bool sendAvailablePlayers(Client c)
{
   char buffer[BUF_SIZE];
   bool foundPlayers = false;
   for (int i = 0; i < actual; i++)
   {
      if (strcmp(clients[i].name, c.name) && !clients[i].isPlaying && clients[i].sock)
      {
         if (!foundPlayers)
         {
            strncpy(buffer, "Les joueurs disponibles : ", BUF_SIZE - 1);
         }
         else
         {
            strncat(buffer, ", ", BUF_SIZE - strlen(buffer) - 1);
         }
         foundPlayers = true;

         strncat(buffer, clients[i].name, BUF_SIZE - strlen(buffer) - 1);
      }
   }
   if (!foundPlayers)
   {
      strncpy(buffer, "Aucun joueur disponible en ce moment", BUF_SIZE - 1);
   }
   else
   {
      strncat(buffer, "\nPour choisir un adversaire envoye 2", BUF_SIZE - (2 * strlen(buffer)) - 1);
   }
   write_client(c.sock, buffer);
   return foundPlayers;
}

void confirmAdversaire(Client *c, char *reponse)
{
   bool adversaireTrouve = false;
   Client *adv;
   for (int i = 0; i < actual; i++)
   {
      if (!strcmp(clients[i].name, c->pseudoAdversaire) && !clients[i].isPlaying && clients[i].sock)
      {
         adversaireTrouve = true;
         adv = &clients[i];
      }
   }
   if (!adversaireTrouve)
   {
      return;
   }
   char buffer[BUF_SIZE];
   char answer = reponse[0];
   switch (answer)
   {
   case 'y':
   case 'Y':
      strncpy(buffer, "La partie d'Awalé avec ", BUF_SIZE - 1);
      strncat(buffer, adv->name, BUF_SIZE - strlen(buffer) - 1);
      strncat(buffer, " commencera bientôt", BUF_SIZE - strlen(buffer) - 1);
      write_client(c->sock, buffer);

      strncpy(buffer, "Le joueur ", BUF_SIZE - 1);
      strncat(buffer, c->name, BUF_SIZE - strlen(buffer) - 1);
      strncat(buffer, " a accepté ta demande", BUF_SIZE - strlen(buffer) - 1);
      write_client(adv->sock, buffer);

      c->isPlaying = true;
      adv->isPlaying = true;
      InitGame(&games[nb_games], nb_games, c->sock, adv->sock);
      nb_games++;
      break;
   case 'n':
   case 'N':
      strncpy(buffer, "Tu as réfusé la demande de ", BUF_SIZE - 1);
      strncat(buffer, adv->name, BUF_SIZE - strlen(buffer) - 1);
      write_client(c->sock, buffer);

      strncpy(buffer, "Le joueur ", BUF_SIZE - 1);
      strncat(buffer, c->name, BUF_SIZE - strlen(buffer) - 1);
      strncat(buffer, " a refusé ta demande", BUF_SIZE - strlen(buffer) - 1);
      write_client(adv->sock, buffer);
      break;
   }
}

Client *choixAdversaire(Client *c, char *adversaire)
{
   if (!strcmp(c->name, adversaire))
   {
      write_client(c->sock, "Tu ne peux pas jouer toute seul");
      return NULL;
   }
   char buffer[BUF_SIZE];
   bool foundPlayer = false;
   for (int i = 0; i < actual; i++)
   {
      if (!strcmp(clients[i].name, adversaire) && !clients[i].isPlaying && clients[i].sock)
      {
         c->pseudoAdversaire = adversaire;
         clients[i].pseudoAdversaire = c->name;
         foundPlayer = true;
         write_client(c->sock, "Notification envoyée à l'adversaire ");
         char buffer[BUF_SIZE];
         strncpy(buffer, "3", BUF_SIZE - 1);
         strncat(buffer, "Voulez vous jouer avec ", BUF_SIZE - strlen(buffer) - 1);
         strncat(buffer, c->name, BUF_SIZE - strlen(buffer) - 1);
         strncat(buffer, "? (Y/N) : ", BUF_SIZE - strlen(buffer) - 1);
         write_client(clients[i].sock, buffer);
         return &clients[i];
      }
   }
   if (!foundPlayer)
   {
      write_client(c->sock, "Le joueur saisi n'existe pas ou est en train de jouer");
      return NULL;
   }
}

bool validerPseudo(char *pseudo)
{
   if (pseudo[0] >= '0' && pseudo[0] <= '9')
   {
      return false;
   }
   for (int i = 0; i < actual; i++)
   {
      if (strcmp(clients[i].name, pseudo) == 0)
      {
         return false;
      }
   }
   return true;
}

void closeBuffer(char **buffer, int bufferSize)
{
   char *p = NULL;
   p = strstr(*buffer, "\n");
   if (p != NULL)
   {
      *p = 0;
   }
   else
   {
      /* fclean */
      (*buffer)[bufferSize - 1] = 0;
   }
}

void buffLectureToBuff(char (*buffer)[BUF_SIZE], char *buffLecture, char messageCode)
{
   (*buffer)[0] = messageCode;
   for (int i = 0; i < BUF_SIZE - 2; i++)
   {
      (*buffer)[i + 1] = buffLecture[i];
   }
   char *p = NULL;
   p = strstr(*buffer, "\n");
   if (p != NULL)
   {
      *p = 0;
   }
   else
   {
      /* fclean */
      (*buffer)[BUF_SIZE - 1] = 0;
   }
}

void gererMessageClient(Client *c, char *message)
{
   printf("%s a parlé\n", c->name);

   // On récupère le premier caractère du message pour déterminer l'action à effectuer
   char action = message[0];
   char contenu[BUF_SIZE - 2];
   strncpy(contenu, message + 1, BUF_SIZE - 2);
   bool res;
   char buffer[BUF_SIZE];
   switch (action)
   {
   case '0':
      res = validerPseudo(contenu);
      printf("res = %d\n", res);
      if (res)
      {
         contenu[BUF_SIZE - 2] = '\0';
         strncpy(c->name, contenu, BUF_SIZE - 2);
         write_client(c->sock, "01");
      }
      else
      {
         write_client(c->sock, "00");
      }
      break;
   case '1':
      sendAvailablePlayers(*c);
      break;
   case '2':
      choixAdversaire(c, contenu);
      break;
   case '3':
      confirmAdversaire(c, contenu);
      break;
   default:
      sendMenu(*c);
      break;
   }
}

static void app(void)
{
   // InitGame(&games[0], 0, 1, 2);
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   int max = sock;

   fd_set rdfs;

   while (1)
   {
      int i = 0;
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for (i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if (FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if (FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = {0};
         size_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if (csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client c = {csock};
         c.isPlaying = false;
         c.score = 0;
         c.gameId = 0;
         clients[actual] = c;
         actual++;
      }
      else
      {
         int i = 0;
         for (i = 0; i < actual; i++)
         {
            /* a client is talking */
            if (FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if (c == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
               }
               else
               {
                  printf("buffer = %s\n", buffer);
                  gererMessageClient(&clients[i], buffer);
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

static void createPlateauMessage(char *buffer, Game* game)
{
   buffer[0] = '4';
   buffer[1] = (char)(game->gameId + '0');
   // plateau
   for (int i = 0; i < 12; i++)
   {
      buffer[i + 1] = (char)(game->plateau[i] + '0');
      printf("Case plateau : %hhu\n", game->plateau[i]);
   }
   printf("gameId : %hhu\n", game->gameId);
   printf("score_joueur1 : %hhu\n", game->score_joueur1);
   printf("score_joueur2 : %hhu\n", game->score_joueur2);
   printf("nb_tours : %hhu\n", game->nb_tours);

   // scores
   buffer[13] = (char)(game->score_joueur1 + '0');
   buffer[14] = (char)(game->score_joueur2 + '0');
   buffer[15] = (char)(game->nb_tours + '0');
   buffer[16] = 0;
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for (i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   message[0] = 0;
   for (i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if (sender.sock != clients[i].sock)
      {
         if (from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = {0};

   if (sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if (send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
