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

/*
Controles that the game is played right
i.e. checks whose turn it is, if the players game move is allowed and who has won
and updates the observer and the players when a move has been made
*/
void PlayGame(Client *joueur, uint8_t case_jeu, char *buffer)
{
   for (int i = 0; i < nb_games; i++)
   {
      if (joueur->gameId == games[i].gameId)
      {
         int num_joueur;
         uint8_t *scoreJoueur;
         uint8_t res;
         //checks who wants to make a move (player 1 or 2)
         if (joueur == games[i].joueur1)
         {
            num_joueur = 1;
            scoreJoueur = &games[i].score_joueur1;
            //tests if the games is already over
            res = testFinPartie(games[i].plateau, num_joueur, -1, *scoreJoueur, games[i].score_joueur2);
         }
         else
         {
            num_joueur = 2;
            scoreJoueur = &games[i].score_joueur2;
            //tests if the games is already over
            res = testFinPartie(games[i].plateau, num_joueur, -1, games[i].score_joueur1, *scoreJoueur);
         }
         if (res == 1)
         {
            //If the game is over and player one has won, both players are notified
            write_client(games[i].joueur1->sock, "Tu as gagné!\n");
            write_client(games[i].joueur2->sock, "Tu as perdu!\n");
            //The players stop playing and can start a new game
            games[i].joueur1->isPlaying = false;
            games[i].joueur2->isPlaying = false;
            //the score of gamesWon of the player no. 1 is augmented
            games[i].joueur1->gamesWon += 1;
            //Notify observers of outcome and end of the game
            notifyObservers(games[i], games[i].joueur1, -1);
            return;
         }
         else if (res == 2)
         {
            //If the game is over and player one has won, both players are notified
            write_client(games[i].joueur1->sock, "Tu as perdu!\n");
            write_client(games[i].joueur2->sock, "Tu as gagné!\n");
            //The players stop playing and can start a new game
            games[i].joueur1->isPlaying = false;
            games[i].joueur2->isPlaying = false;
            //the score of gamesWon of the player no. 2 is augmented
            games[i].joueur2->gamesWon += 1;
            //Notify observers of outcome and end of the game
            notifyObservers(games[i], games[i].joueur2, -1);
            return;
         }
         if (num_joueur != games[i].next_joueur)
         {
            //Notifies player if it's not his turn
            write_client(joueur->sock, "C'est pas ton tour\n");
            return;
         }
         //return value of jourCoup informs us about possible complications that were encoutered during the game move
         res = jouerCoup(case_jeu, num_joueur, scoreJoueur, -1, &games[i].plateau);
         //there were complications while carrying out the game move
         if (res != 0)
         {
            //asks player to make a different move
            strncpy(buffer, "6", BUF_SIZE - 1);
            strncat(buffer, "Votre saisie est incorrecte : case vide ou indice pas entre 1 et 6 !\n", BUF_SIZE - strlen(buffer) - 1);
            write_client(joueur->sock, buffer);
            return;
         }
         //alternates between 1,2 to indicate which player can make a move
         games[i].next_joueur = games[i].next_joueur % 2 + 1;
         //creates the visuals for the plateau of player 1
         createPlateauMessage(buffer, &games[i], games[i].joueur1, false);
         write_client(games[i].joueur1->sock, buffer);

         //creates the visuals for the plateau of player 1
         createPlateauMessage(buffer, &games[i], games[i].joueur2, false);
         write_client(games[i].joueur2->sock, buffer);

         //updates observes of the game move
         notifyObservers(games[i], joueur, case_jeu);
         break;
      }
   }
}

//initializes a new game
//calls function initPartie from awale.c
void InitGame(Game *game, int gameId, Client *joueur1, Client *joueur2)
{
   game->gameId = gameId;
   game->joueur1 = joueur1;
   game->joueur2 = joueur2;
   game->nb_observers = 0;
   game->next_joueur = 1 + rand() % (2);
   game->plateau = (uint8_t *)malloc(NB_CASES * sizeof(uint8_t));
   initPartie(&game->plateau, &game->score_joueur1, &game->score_joueur2, &game->sens_rotation);
   notifyObservers(*game, NULL, -1);
}

//Sends a list of the operations a user can ask for 
void sendMenu(Client c, char *buffer)
{
   char *string = (char *)malloc(BUF_SIZE * sizeof(char));
   if (!c.isPlaying && !c.observe)
   {
      //Informs the player of the number of games he/she has won
      strncpy(buffer, "\nVous avez gagné ", BUF_SIZE - 1);
      sprintf(string, "%d", c.gamesWon);
      strncat(buffer, string, BUF_SIZE - strlen(buffer) - 1);
      strncat(buffer, " parties !\n\n", BUF_SIZE - strlen(buffer) - 1);

      //shows which options the user has
      strncat(buffer, "1.  Afficher la liste des pseudos en ligne\n", BUF_SIZE - strlen(buffer) - 1);
      strncat(buffer, "2.  Choisir un adversaire \n", BUF_SIZE - strlen(buffer) - 1);
      strncat(buffer, "6.  Ecrire une bio \n", BUF_SIZE - strlen(buffer) - 1);
      strncat(buffer, "7.  Lister les parties en cours \n", BUF_SIZE - strlen(buffer) - 1);
      strncat(buffer, "8.  Observer une partie \n", BUF_SIZE - strlen(buffer) - 1);
      strncat(buffer, "9.  Déconnexion \n", BUF_SIZE - strlen(buffer) - 1);
   }
   else if (!c.observe)
   {
      //shows the users options during a game (specifically also whose turn it is)
      strncpy(buffer, "4.  Choisir la case à jouer\n", BUF_SIZE - 1);
      strncat(buffer, "5.  Abandonner\n", BUF_SIZE - strlen(buffer) - 1);
   }
   else
   {
      //special options for observer
      strncpy(buffer, "0.  Arreter d'observer\n", BUF_SIZE - 1);
   }
   free(string);
}

//Is called when a player quits the game
//deletes the games and declares the other player as winner
//resets the clients attributes used for controling th game flow (i.e. opponent, current game)
static void abandonJoueur(Client *client)
{
   char *buffer = (char *)malloc(BUF_SIZE * sizeof(char));
   client->isPlaying = false;
   int indice = -1;
   for (int i = 0; i < nb_games; i++)
   {
      if (games[i].gameId == client->gameId)
      {
         indice = i;
         //Resets the opponent
         if (client == games[i].joueur1)
         {
            games[i].joueur2->isPlaying = false;
            games[i].joueur2->gamesWon += 1;
            games[i].joueur2->gameId = -1;
            games[i].joueur2->pseudoAdversaire = NULL;
            write_client(games[i].joueur2->sock, "Votre adversaire a abandonné. Vous avez gangné la partie\n");
            sendMenu(*(games[i].joueur2), buffer);
            write_client(games[i].joueur2->sock, buffer);

            //Notify observer
            for (int j = 0; j < games[i].nb_observers; j++)
            {
               notifyObservers(games[i], games[i].joueur2, -1);
            }
         }
         //resets the attributes of the player who left the game
         else if (client == games[i].joueur2)
         {
            games[i].joueur1->isPlaying = false;
            games[i].joueur1->gamesWon += 1;
            games[i].joueur1->gameId = -1;
            games[i].joueur1->pseudoAdversaire = NULL;
            write_client(games[i].joueur1->sock, "Votre adversaire a abandonné. Vous avez gangné la partie\n");
            sendMenu(*(games[i].joueur1), buffer);
            write_client(games[i].joueur1->sock, buffer);
            for (int j = 0; j < games[i].nb_observers; j++)
            {
               notifyObservers(games[i], games[i].joueur1, -1);
            }
         }
         break;
      }
   }
   //error handling
   if (indice != -1)
   {
      remove_game_en_cours(games, indice, &nb_games);
   }
   free(buffer);
}

//Overview of current games which are stored in the games array
bool preparerListePartiesObserver(Client c, char *buffer)
{
   if (c.isPlaying)
   {
      return false;
   }
   bool foundGames = false;
   for (int i = 0; i < nb_games; i++)
   {
      if (!foundGames)
      {
         strncpy(buffer, "Les parties en cours : \n", BUF_SIZE - 1);
      }
      foundGames = true;
      //Prints players names
      snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer), "    %d. %s vs %s\n", i + 1, games[i].joueur1->name, games[i].joueur2->name);
   }
   if (!foundGames)
   {
      strncpy(buffer, "Aucune partie en cours", BUF_SIZE - 1);
   }
   else
   {
      strncat(buffer, "\nPour choisir une partie à regarder envoyez 8\n", BUF_SIZE - (2 * strlen(buffer)) - 1);
   }
   return foundGames;
}

void observerPartieEnCours(Client *c, char *contenu, char *buffer)
{
   int index = -1;
   int conversionSuccessful = sscanf(contenu, "%d", &index);
   index--;
   if (conversionSuccessful != 1 || index >= nb_games || index<0)
   {
      strncpy(buffer, "Saisie invalide : il faut choisir un numéro de partie valide\n", BUF_SIZE - 1);
      return;
   }
   games[index].observers[games[index].nb_observers] = c;
   games[index].nb_observers += 1;
   c->observe = true;
   snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer), "Vous suivez désormais la partie entre %s et %s\n", games[index].joueur1->name, games[index].joueur2->name);
   snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer), "Vous allez la suivre du point de vue de %s\n", games[index].joueur1->name);
}

void notifyObservers(Game game, Client *joueurQuiAJoue, int case_jouee)
{
   char buffer[BUF_SIZE];
   buffer[0] = 0;
   if (joueurQuiAJoue == NULL)
   {
      snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer), "La partie entre %s et %s vient de commencer\n", game.joueur1->name, game.joueur2->name);
      snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer), "Vous allez la suivre du point de vue de %s\n", game.joueur1->name);
   }
   else if (case_jouee == -1)
   {
      snprintf(buffer, sizeof(buffer), "%s a gagné la partie ! Il a désormais gagné %d parties.\n", joueurQuiAJoue->name, joueurQuiAJoue->gamesWon);
   }
   else
   {
      snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer), "%s et %s\n", game.joueur1->name, game.joueur2->name);
      snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer), "\nLe joueur %s a choisi de jouer la case %d \n\n", joueurQuiAJoue->name, case_jouee);
      createPlateauMessage(buffer, &game, game.joueur1, true);
   }
   for (int j = 0; j < game.nb_observers; j++)
   {
      write_client(game.observers[j]->sock, buffer);
   }
}

void arreterObserver(Client *c, char *buffer)
{
   bool arrete = false;
   for (int i = 0; i < nb_games; i++)
   {
      for (int j = 0; j < games[i].nb_observers; j++)
      {
         if (games[i].observers[j] == c)
         {
            c->observe = false;
            memmove(games[i].observers + j, games[i].observers + j + 1, (games[i].nb_observers - j - 1) * sizeof(Client *));
            (games[i].nb_observers)--;
            arrete = true;
            break;
         }
      }
   }
   if (arrete)
   {
      strncpy(buffer, "Vous avez arrêté d'observer la partie ! \n", BUF_SIZE - 1);
   }
   else
   {
      c->observe = false;
      strncpy(buffer, "Vous n'observez aucune partie en cours \n", BUF_SIZE - 1);
   }
}

//Reformates bio i.e. reintroduces newlines 
char *processBio(char *contenu)
{
   char *bio = malloc(BUF_SIZE); 

   //prevents invalid pointer error
   strncpy(bio, contenu, strlen(contenu));
   char *tildePos = strstr(bio, "~");

   // Process each occurrence of tilde in the string
   while (tildePos != NULL) 
   {
      *tildePos = '\n';
      tildePos = strchr(tildePos, '~');
   }
   return bio;
}

/*
Either sends a list of the currents clients or send the bio of a specific client
This depends on whether the pseudo if the player is send by the client
*/
bool sendAvailablePlayers(Client c, char *buffer, char *player)
{ 
   if (c.isPlaying)
   {
      return false;
   }
   bool foundPlayers = false;
   //Test if client send name of a player i.e. if he/ she wants a bio or just the overview
   if (player[0] == '\0')
   {
      for (int i = 0; i < actual; i++)
      {
         //searches for active players
         if (strcmp(clients[i].name, "") && strcmp(clients[i].name, c.name) && !clients[i].isPlaying && !clients[i].observe && clients[i].sock)
         {
            if (!foundPlayers) // lists all available clienst
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
      if (!foundPlayers) //sends message of there are no other clients online
      {
         strncpy(buffer, "Aucun joueur disponible en ce moment", BUF_SIZE - 1);
      }
      else
      {
         //Tells the client about his further options if there are other clients
         //they can choose an opponent
         strncat(buffer, "\nPour choisir un adversaire envoyez 2\n", BUF_SIZE - (strlen(buffer)) - 1);
         //they can request a bio
         strncat(buffer, "Pour voir la bio d'un joueur mettez 1<pseudo du joueur>", BUF_SIZE - (strlen(buffer)) - 1);
      }
   }
   else{ //sending the bio 
      char *temp;
      int cmp;
      for (int i = 0; i < actual; i++)
      {
         //finds the player whose bio was requested
         if (strcmp(clients[i].name, player) == 0)
         {
            foundPlayers = true;
            //checks if the player has a bio
            if (clients[i].bio[0] != '\0')
            {
               //gets the bio in the right format and sends it to the client
               temp = processBio(clients[i].bio);
               temp[BUF_SIZE-1] = '\0';
               strncpy(buffer, temp , BUF_SIZE - 1);
               free(temp);
            }
            else{
               strncpy(buffer, "Ce joueur n'a pas une bio.\n", BUF_SIZE - strlen(buffer));
            }
         }
      }
      if (!foundPlayers)
      {
         strncpy(buffer, "Ce joueur n'existe pas\n", BUF_SIZE - (2 * strlen(buffer)) - 1);
      }
   return foundPlayers;
   }
}

/*
Handles communication after a game request was send
i.e. checks if the player might already play another game,
if the requested player still exists and
if he/she even wants to play this game
*/
void confirmAdversaire(Client *c, char *reponse)
{
   if (c->isPlaying)
   {
      return;
   }
   char buffer[BUF_SIZE];
   buffer[0] = 0;
   bool adversaireTrouve = false;
   Client *adv = NULL;
   for (int i = 0; i < actual; i++)
   {
      //Checks if player has disconnected or started another game
      if (!strcmp(clients[i].name, c->pseudoAdversaire) && !clients[i].isPlaying && !clients[i].observe && clients[i].sock)
      {
         adversaireTrouve = true;
         adv = &clients[i];
      }
   }
   
   if (!adversaireTrouve)
   {
      strncpy(buffer, "L'adversaire s'est déconnecté ou a commencé une autre partie !", BUF_SIZE - 1);
      write_client(c->sock, buffer);
      return;
   }
   //Evaluates the clients response to the game request
   char answer = reponse[0];
   switch (answer)
   {
   case 'y':
   case 'Y':
      strncpy(buffer, "La partie d'Awalé avec ", BUF_SIZE - 1);
      strncat(buffer, adv->name, BUF_SIZE - strlen(buffer) - 1);
      strncat(buffer, " commence.\n", BUF_SIZE - strlen(buffer) - 1);
      write_client(c->sock, buffer);

      strncpy(buffer, "Le joueur ", BUF_SIZE - 1);
      strncat(buffer, c->name, BUF_SIZE - strlen(buffer) - 1);
      strncat(buffer, " a accepté ta demande.\n", BUF_SIZE - strlen(buffer) - 1);
      write_client(adv->sock, buffer);

      //prepares the game play
      c->isPlaying = true;
      c->gameId = nb_games;
      adv->isPlaying = true;
      adv->gameId = nb_games;
      InitGame(&games[nb_games], nb_games, c, adv);

      //visualizes the initial situation if the game
      char *buff;
      buff = (char *)malloc(BUF_SIZE * sizeof(char));
      buff[0] = 0;
      createPlateauMessage(buff, &games[nb_games], games[nb_games].joueur1, false);
      write_client(games[nb_games].joueur1->sock, buff);
      buff[0] = 0;
      createPlateauMessage(buff, &games[nb_games], games[nb_games].joueur2, false);
      write_client(games[nb_games].joueur2->sock, buff);
      free(buff);
      nb_games++;
      break;

      //if the player does not confirm his/her wish to play the game
   default:
      strncpy(buffer, "Tu as réfusé la demande de ", BUF_SIZE - 1);
      strncat(buffer, adv->name, BUF_SIZE - strlen(buffer) - 1);
      write_client(c->sock, buffer);

      //Informs the client who send the request
      strncpy(buffer, "Le joueur ", BUF_SIZE - 1);
      strncat(buffer, c->name, BUF_SIZE - strlen(buffer) - 1);
      strncat(buffer, " a refusé ta demande", BUF_SIZE - strlen(buffer) - 1);
      write_client(adv->sock, buffer);
      break;
   }
}

//Finds the opponent a client wants to play against and sends him/her a message
Client *choixAdversaire(Client *c, char *adversaire)
{
   if (c->isPlaying)
   {
      return NULL;
   }
   if (!strcmp(c->name, adversaire))
   {
      write_client(c->sock, "Tu ne peux pas jouer tout seul");
      return NULL;
   }
   char buffer[BUF_SIZE];
   buffer[0] = 0;
   for (int i = 0; i < actual; i++)
   {
      if (!strcmp(clients[i].name, adversaire) && !clients[i].isPlaying && clients[i].sock)
      {
         c->pseudoAdversaire = adversaire;
         clients[i].pseudoAdversaire = c->name;
         write_client(c->sock, "Notification envoyée à l'adversaire ");

         strncpy(buffer, "3", BUF_SIZE - 1);
         strncat(buffer, "Voulez vous jouer avec ", BUF_SIZE - strlen(buffer) - 1);
         strncat(buffer, c->name, BUF_SIZE - strlen(buffer) - 1);
         strncat(buffer, "? (Y/N) : ", BUF_SIZE - strlen(buffer) - 1);
         write_client(clients[i].sock, buffer);
         return &clients[i];
      }
   }
   write_client(c->sock, "Le joueur saisi n'existe pas ou est en train de jouer");
   return NULL;
}

//Checks if pseudo has the right format
bool validerPseudo(char *pseudo)
{
   //first character cannot be a digit because that's what we use for conrtoling the flow of the program
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

/*
The main method wher we handle the program flow by using digits from 0 to 9
*/
void gererMessageClient(Client *c, char *message)
{
   // On récupère le premier caractère du message pour déterminer l'action à effectuer
   char action = message[0];
   char contenu[BUF_SIZE - 2];
   strncpy(contenu, message + 1, BUF_SIZE - 2);
   bool res;
   char *buffer = (char *)malloc(BUF_SIZE * sizeof(char));
   buffer[0] = 0;
   int index = -1;
   switch (action)
   {
   case '0': // stop observing or validate the chosen pseudo
      if (c->observe)
      {
         arreterObserver(c, buffer);
         write_client(c->sock, buffer);
      }
      else
      {
         res = validerPseudo(contenu);
         if (res)
         {
            contenu[BUF_SIZE - 2] = '\0';
            strncpy(c->name, contenu, BUF_SIZE - 2);
            //control message
            write_client(c->sock, "01");
         }
         else
         {
            //control message
            write_client(c->sock, "00");
         }
      }
      break;
   case '1': // sends all available players or the menu
      if (!c->isPlaying)
      {
         sendAvailablePlayers(*c, buffer, contenu);
         write_client(c->sock, buffer);
      }
      else
      {
         sendMenu(*c, buffer);
         write_client(c->sock, buffer);
      }
      break;
   case '2': //allows player to chose an opponent or sends the overview of actions
      if (!c->isPlaying)
      {
         choixAdversaire(c, contenu);
      }
      else
      {
         sendMenu(*c, buffer);
         write_client(c->sock, buffer);
      }
      break;
   case '3': //checks if a game can be started i.e. if both parties want to play
      if (!c->isPlaying)
      {
         confirmAdversaire(c, contenu);
      }
      else
      {
         sendMenu(*c, buffer);
         write_client(c->sock, buffer);
      }
      break;
   case '4': //is used to make moves during the game
      if (c->isPlaying)
      {
         PlayGame(c, (uint8_t)(contenu[0] - '0'), buffer);
      }
      else
      {
         sendMenu(*c, buffer);
         write_client(c->sock, buffer);
      }
      break;
   case '5': //exit from a game
      if (c->isPlaying)
      {
         abandonJoueur(c);
         write_client(c->sock, "Vous avez quitté la partie !\n");
      }
      sendMenu(*c, buffer);
      write_client(c->sock, buffer);

      break;
   case '6': // client wants to write a bio
      strcpy(c->bio, contenu);
      write_client(c->sock, "Tu as cree une bio\n");
      break;
   case '7': //looks for current games
      if (!c->isPlaying)
      {
         preparerListePartiesObserver(*c, buffer);
         write_client(c->sock, buffer);
      }
      else
      {
         sendMenu(*c, buffer);
         write_client(c->sock, buffer);
      }
      break;
   case '8': //lists current games
      if (!c->isPlaying)
      {
         observerPartieEnCours(c, contenu, buffer);
         write_client(c->sock, buffer);
      }
      else
      {
         sendMenu(*c, buffer);
         write_client(c->sock, buffer);
      }
      break;
   case '9': // déconnexion
      if (c->isPlaying)
      {
         sendMenu(*c, buffer);
         write_client(c->sock, buffer);
         break;
      }
      for (int i = 0; i < actual; i++)
      {
         if (strcmp(clients[i].name, c->name) == 0)
         {
            index = i;
            break;
         }
      }
      abandonJoueur(c);
      closesocket(c->sock);
      remove_client(clients, index, &actual);
      strncpy(buffer, c->name, BUF_SIZE - 1);
      strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
      send_message_to_all_clients(clients, *c, actual, buffer, 1);
      break;
   default:
      sendMenu(*c, buffer);
      write_client(c->sock, buffer);
      break;
   }
   free(buffer);
}

static void app(void)
{
   // InitGame(&games[0], 0, 1, 2);
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   buffer[0] = 0;
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

         //init of client struct
         Client c = {csock};
         c.isPlaying = false;
         c.observe = false;
         c.gamesWon = 0;
         c.gameId = 0;
         c.bio[0] = 0;
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
                  abandonJoueur(&clients[i]);
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
               }
               else
               {
                  gererMessageClient(&clients[i], buffer);
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   clear_games();
   end_connection(sock);
}

static void createPlateauMessage(char *buffer, Game *game, Client *joueur, bool pourObserveur)
{
   int idx_row1, idx_row2, num_joueur;
   (joueur == game->joueur1) ? (num_joueur = 1) : (num_joueur = 2);
   char *string = (char *)malloc(5 * sizeof(char));
   // indices des cases par lesquelles commencent les lignes à afficher
   idx_row1 = NB_CASES / 2 * (2 - num_joueur);
   idx_row2 = NB_CASES / 2 * num_joueur - 1;

   // affichage ligne par ligne
   strncat(buffer, "Point de vue du joueur ", BUF_SIZE - strlen(buffer) - 1);
   strncat(buffer, joueur->name, BUF_SIZE - strlen(buffer) - 1);
   strncat(buffer, "\n", BUF_SIZE - strlen(buffer) - 1);
   snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer), "Le score de %s = ", joueur->name);
   if (joueur == game->joueur1)
   {
      sprintf(string, "%d", game->score_joueur1);
      strncat(buffer, string, BUF_SIZE - strlen(buffer) - 1);
      strncat(buffer, "\n", BUF_SIZE - strlen(buffer) - 1);
      strncat(buffer, "Le score de l'adversaire = ", BUF_SIZE - strlen(buffer) - 1);
      sprintf(string, "%d", game->score_joueur2);
      strncat(buffer, string, BUF_SIZE - strlen(buffer) - 1);
   }
   else
   {
      sprintf(string, "%d", game->score_joueur2);
      strncat(buffer, string, BUF_SIZE - strlen(buffer) - 1);
      strncat(buffer, "\n", BUF_SIZE - strlen(buffer) - 1);
      strncat(buffer, "Le score de l'adversaire = ", BUF_SIZE - strlen(buffer) - 1);
      sprintf(string, "%d", game->score_joueur1);
      strncat(buffer, string, BUF_SIZE - strlen(buffer) - 1);
   }
   strncat(buffer, "\n", BUF_SIZE - strlen(buffer) - 1);

   strncat(buffer, "Plateau de jeu :\n\n", BUF_SIZE - strlen(buffer) - 1);

   for (int i = idx_row1; i < idx_row1 + 6; ++i)
   {
      strncat(buffer, " | ", BUF_SIZE - strlen(buffer) - 1);
      sprintf(string, "%d", game->plateau[i]);
      strncat(buffer, string, BUF_SIZE - strlen(buffer) - 1);
   }
   strncat(buffer, " |\n", BUF_SIZE - strlen(buffer) - 1);
   for (int i = 0; i < NB_CASES / 2; ++i)
   {
      strncat(buffer, "-----", BUF_SIZE - strlen(buffer) - 1);
   }
   strncat(buffer, "\n", BUF_SIZE - strlen(buffer) - 1);
   for (int i = idx_row2; i > idx_row2 - 6; --i)
   {
      strncat(buffer, " | ", BUF_SIZE - strlen(buffer) - 1);
      sprintf(string, "%d", game->plateau[i]);
      strncat(buffer, string, BUF_SIZE - strlen(buffer) - 1);
   }
   strncat(buffer, " |\n\n", BUF_SIZE - strlen(buffer) - 1);
   if (!pourObserveur)
   {
      if (num_joueur == game->next_joueur)
      {
         strncat(buffer, "4.  Choisir la case à jouer\n", BUF_SIZE - strlen(buffer) - 1);
      }
      strncat(buffer, "5.  Abandonner\n", BUF_SIZE - strlen(buffer) - 1);
   }
   else
   {
      strncat(buffer, "0.  Arreter d'observer\n", BUF_SIZE - strlen(buffer) - 1);
   }
   free(string);
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for (i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void clear_games()
{
   for (int i = 0; i < nb_games; i++)
   {
      free(games[i].plateau);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void remove_game_en_cours(Game *games_en_cours, int to_remove,
                                 uint8_t *cur_game)
{
   char *buffer = (char *)malloc(BUF_SIZE * sizeof(char));
   buffer[0] = 0;
   for (int i = 0; i < games[to_remove].nb_observers; i++)
   {
      games[to_remove].observers[i]->observe = false;
      sendMenu(*(games[to_remove].observers[i]), buffer);
      write_client(games[to_remove].observers[i]->sock, buffer);
   }
   free(games[to_remove].plateau);
   /* we remove the client in the array */
   memmove(games_en_cours + to_remove, games_en_cours + to_remove + 1,
           (*cur_game - to_remove - 1) * sizeof(Game));
   /* number client - 1 */
   (*cur_game)--;
   free(buffer);
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
