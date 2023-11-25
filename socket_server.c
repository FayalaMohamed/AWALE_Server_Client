/* Serveur sockets TCP
 * affichage de ce qui arrive sur la socket
 *    socket_server port (port > 1024 sauf root)
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include "awale.h"

#define MAX_CLIENTS 10
#define MAX_GAMES 100

struct Game
{
  int gameId;
  int socket_joueur1;
  int socket_joueur2;
  int score_joueur1;
  int score_joueur2;
  int sens_rotation;
  int *plateau;
}typedef Game;

struct Player{
  int socket_joueur;
  int gameId;
  int score;
  int isPlaying;
} typedef Player;

Game games[MAX_CLIENTS/2];


void InitGame(Game *game, int gameId, int socket_joueur1, int socket_joueur2){
  game->gameId = gameId;
  game->socket_joueur1 = socket_joueur1;
  game->socket_joueur2 = socket_joueur2;
  initPartie(&game->plateau, &game->score_joueur1, &game->score_joueur2, &game->sens_rotation);
  afficherPlateau(game->plateau, 1);
}

void PlayGame(Game* game){

}

int main(int argc, char **argv)
{
  int sockfd, newsockfd, clilen, chilpid, ok, nleft, nbwriten, scomm, pid;
  char c;
  struct sockaddr_in cli_addr, serv_addr;

  if (argc != 2)
  {
    printf("usage: socket_server port\n");
    exit(0);
  }

  printf("server starting...\n");

  /* ouverture du socket */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    printf("impossible d'ouvrir le socket\n");
    exit(0);
  }

  /* initialisation des parametres */
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv[1]));

  /* effecture le bind */
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("impossible de faire le bind\n");
    exit(0);
  }

  /* petit initialisation */
  listen(sockfd, MAX_CLIENTS);

  signal(SIGCHLD, SIG_IGN);
  while (1)
  {
    scomm = accept(sockfd, NULL, NULL);
    pid = fork();
    if (pid == 0) /* c’est le fils */
    {
      close(sockfd); /* socket inutile pour le fils */

      /* traiter la communication */
      while (1)
      {
        while (read(scomm, &c, 1) != 1)
          ;
        if (c == EOF)
        {
          close(scomm);
          exit(0);
        }
        printf("%c", c);
      }

      close(scomm);
      exit(0); /* on force la terminaison du fils */
    }
    else /* c’est le pere */
    {
      close(scomm); /* socket inutile pour le pere */
    }
  }
  close(sockfd);

  return 1;
}
