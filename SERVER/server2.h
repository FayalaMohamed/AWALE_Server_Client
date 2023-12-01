#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>

#elif defined(linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h>  /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#define CRLF "\r\n"
#define PORT 1977
#define MAX_CLIENTS 100

#define BUF_SIZE 2048

#include "client2.h"
#include <stdbool.h>

struct Game
{
    uint8_t gameId;
    Client*  joueur1;
    Client* joueur2;
    uint8_t score_joueur1;
    uint8_t score_joueur2;
    uint8_t sens_rotation;
    uint8_t next_joueur;
    uint8_t *plateau;
    Client *observers[MAX_CLIENTS];
    int nb_observers;
} typedef Game;

Game games[MAX_CLIENTS];
Client clients[MAX_CLIENTS];
char message[BUF_SIZE];
/* the index for the clients array */
int actual = 0;
uint8_t nb_games = 0;

void notifyObservers(Game game, Client *joueurQuiAJoue, int case_jouee);
static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void send_message_to_all_clients(Client *clients, Client client, int actual, const char *buffer, char from_server);
static void clear_clients(Client *clients, int actual);
static void clear_games();
static void remove_client(Client *clients, int to_remove, int *actual);
static void remove_game_en_cours(Game *games_en_cours, int to_remove,
                                 uint8_t *cur_game);
static void createPlateauMessage(char *buffer, Game *game, Client *joueur, bool pourObserveur);
#endif /* guard */
