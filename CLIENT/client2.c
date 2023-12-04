#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include "client2.h"
#include <stdbool.h>

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


static void app(const char *address)
{
   SOCKET sock = init_connection(address);
   char buffer[BUF_SIZE];
   char buffLecture[BUF_SIZE - 1];
   bool deconnected = false;

   fd_set rdfs;

   /*
   Demands from th client to first create a pseudo
   Waits for input and checks it
   Demands a new pseudo until the format is correct
   Then aks the server to send the menu 
   */
   printf("Saisissez votre pseudo (1er caractère ne doit pas être un chiffre) : \n");
   while (1)
   {
      fgets(buffLecture, BUF_SIZE - 2, stdin);
      buffLectureToBuff(&buffer, buffLecture, '0');
      write_server(sock, buffer);

      int n = read_server(sock, buffer);
      /* server down */
      if (n == 0)
      {
         printf("Server disconnected !\n");
         end_connection(sock);
         return;
      }
      if (buffer[0] == '0' && buffer[1] == '1')
      {
         printf("Pseudo validé\n");
         printf("Vous pouvez toujours demander le menu des actions possibles en tapant 'MENU' ou 'menu'\n");
         strncpy(buffer, "MENU", BUF_SIZE - 1);
         write_server(sock, buffer);
         break;
      }
      else if (buffer[0] == '0' && buffer[1] == '0')
      {
         printf("Le pseudo choisi est déjà pris. Saisissez votre pseudo : \n");
      }
   }

   while (!deconnected)
   {
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the socket */
      FD_SET(sock, &rdfs);

      if (select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }
      /* something from standard input : i.e keyboard */
      if (FD_ISSET(STDIN_FILENO, &rdfs))
      {
         char action[BUF_SIZE];
         fgets(action, BUF_SIZE - 1, stdin);
         switch (action[0])
         {
         case '0': //Checks if the player really wnats to leave the game, then sends 0 to the server
            printf("Etes vous sur de vouloir ne plus suivre la partie ? ? (Y/N)\n");
            fgets(action, BUF_SIZE - 1, stdin);
            {
               char *p = NULL;
               p = strstr(action, "\n");
               if (p != NULL)
               {
                  *p = 0;
               }
               else
               {
                  /* fclean */
                  action[BUF_SIZE - 1] = 0;
               }
            }
            if (strcmp(action, "y") == 0 || strcmp(action, "Y") == 0)
            {
               strncpy(buffer, "0", BUF_SIZE - 1);
            }
            break;
         case '1': // lists all players that are online or one of their bios
            strncpy(buffer, action, BUF_SIZE-  1);
           break;
         case '2': // envoyer une demande de jeu by demanding the pseudo of said player
            strncpy(buffer, "2", BUF_SIZE - 1);
            printf("Saisissez le pseudo du joueur à affronter : \n");
            fgets(action, BUF_SIZE - 1, stdin);
            strncat(buffer, action, BUF_SIZE - strlen(buffer) - 1);
            break;
         case '4': // jouer un coup, only sends the valid inout to the server
            strncpy(buffer, "4", BUF_SIZE - 1);
            printf("Saisissez la case où vous voulez jouer : \n");
            while (1)
            {
               fgets(action, BUF_SIZE - 1, stdin);
               int choice;
               //converts the input to an integer
               int conversionSuccessful = sscanf(action, "%d", &choice);

               // Check if the input is a valid integer and within the players' cases
               if (conversionSuccessful != 1 || choice < 1 || choice > 6)
               {
                  printf("Saisie invalide : Il faut choisir un chiffre entre 1 et 6 !\n");
               }
               else
               {
                  break;
               }
            }
            strncat(buffer, action, BUF_SIZE - strlen(buffer) - 1);
            break;
         case '5': // abandonner
            printf("Etes vous sur de vouloir abandonner ? (Y/N)\n");
            fgets(action, BUF_SIZE - 1, stdin);
            {
               //filters out the newline character to facilitade the string comparison
               char *p = NULL;
               p = strstr(action, "\n");
               if (p != NULL)
               {
                  *p = 0;
               }
               else
               {
                  // like fclean 
                  action[BUF_SIZE - 1] = 0;
               }
            }// Si confirmation, faire en sorte l'abandon 
            if (strcmp(action, "y") == 0 || strcmp(action, "Y") == 0)
            {
               //starts treatment of "action 5" on the server
               strncpy(buffer, "5", BUF_SIZE - 1);
            }
            break;     
         case '6': // ecrire une bio pour soi
            //Adds each line from the stdin to the buffer while removing the newline character and replacing it with a tilde 
            //(otherwise only the first line of inout would be send to the server)
            //Meanwhile it checks if the user already wrote 10 lines or if he/she is finished
            //User can type in exit if they are done with their bio
            printf("Ecrivez la bio et mettez quand vous avez fini (max. 10 lignes)\n ");
            strncpy(buffer, "6", BUF_SIZE - 1);
            while (1)
            {  
               fgets(action, sizeof(action), stdin);
               // Remove newline character at the end of action
               char *p = NULL;
               p = strstr(action, "\n");
               if (p != NULL) 
               {
                  *p = '\0';
               }
               char *exitPos = strstr(action, "exit");
               if(exitPos != NULL || res == 10)
               {
                  break;
               }
               strncat(buffer, action, BUF_SIZE - strlen(buffer)-1);
               strncat(buffer, "~", BUF_SIZE - strlen(buffer)-1);
               res ++;
               // Check if buffer is full
               if (strlen(buffer) >= BUF_SIZE-1) 
               { 
                  printf("Buffer for the bio is full.\n");
                  break;
               }
            }
            if (res == 0) //if the player does not write anything, there is no need to send a message to the server
            {
               strncpy(buffer, "", BUF_SIZE - 1);
            }
            break;
         case '7': // lister les parties en cours
            strncpy(buffer, "7", BUF_SIZE - 1);
            break;
         case '8': // observer une partie en cours
            strncpy(buffer, "8", BUF_SIZE - 1);
            printf("Saisissez le numéro de la partie que vous voulez suivre : \n");
            while (1)
            {
               fgets(action, BUF_SIZE - 1, stdin);
               int choice;
               //checks the input format and saves it into choice
               int conversionSuccessful = sscanf(action, "%d", &choice);

               // Check if the input is a valid integer
               if (conversionSuccessful != 1)
               {
                  printf("Saisie invalide : il faut choisir le numéro de la partie\n");
               }
               else
               {
                  //if the input has the right format we don't need new input from the user
                  break;
               }
            }
            strncat(buffer, action, BUF_SIZE - strlen(buffer) - 1);
            break;
         case '9': // deconnexion
            printf("Etes vous sur de vouloir vous déconnecter ? (Y/N)\n");
            //filters out newline character for better string comparison
            fgets(action, BUF_SIZE - 1, stdin);
            {
               char *p = NULL;
               p = strstr(action, "\n");
               if (p != NULL)
               {
                  *p = 0;
               }
               else
               {
                  /* fclean */
                  action[BUF_SIZE - 1] = 0;
               }
            }
            //checks if the player confirmed the wish to deconnect
            if (strcmp(action, "y") == 0 || strcmp(action, "Y") == 0)
            {
               //triggers action 9 on the server
               strncpy(buffer, "9", BUF_SIZE - 1);
               deconnected = true;
            }
            break;
         default:
            //if the input does not correspond to any use case, we send the menu to show the client their possible choices
            strncpy(buffer, "MENU", BUF_SIZE - 1);
            break;
         }
         {
            //filters out the newline character and send everything up to it to the server 
            char *p = NULL;
            p = strstr(buffer, "\n");
            if (p != NULL)
            {
               *p = 0;
            }
            else
            {
               /* fclean */
               buffer[BUF_SIZE - 1] = 0;
            }
         }
         write_server(sock, buffer);
      }
      else if (FD_ISSET(sock, &rdfs))
      {
         int n = read_server(sock, buffer);
         /* server down */
         if (n == 0)
         {
            printf("Server disconnected !\n");
            break;
         }
         //Handles the demandes from the server which are triggered by specicif numbers
         //The number is always in the first position of the buffer
         char action = buffer[0];
         //the remainder of the buffer usually contains additional information or is empty
         char contenu[BUF_SIZE - 1];
         strncpy(contenu, buffer + 1, BUF_SIZE - 2);
         //distinguishes the different demands
         switch (action)
         {
         case '3': // client recoit une demande de jeu
            printf("%s\n", contenu);
            fgets(contenu, BUF_SIZE - 2, stdin);
            buffLectureToBuff(&buffer, contenu, '3');
            write_server(sock, buffer);
            break;
         case '6': // checks if case choisie est vide ou invalide
            printf("%s\n", contenu);
            printf("Saisissez la case où vous voulez jouer : \n");
            while (1)
            {
               fgets(contenu, BUF_SIZE - 2, stdin);
               int choice;
               int conversionSuccessful = sscanf(contenu, "%d", &choice);

               // Check if the input is a valid integer and within the players' cases
               //otherwise a new input is demanded
               if (conversionSuccessful != 1 || choice < 1 || choice > 6)
               {
                  printf("Saisie invalide : il faut choisir un chiffre entre 1 et 6 !\n");
               }
               else
               {
                  break;
               }
            }
            buffLectureToBuff(&buffer, contenu, '4');
            write_server(sock, buffer);
            break; 
         default:
            puts(buffer);
            break;
         }
      }
   }

   end_connection(sock);
}

static int init_connection(const char *address)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = {0};
   struct hostent *hostinfo;

   if (sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   hostinfo = gethostbyname(address);
   if (hostinfo == NULL)
   {
      fprintf(stderr, "Unknown host %s.\n", address);
      exit(EXIT_FAILURE);
   }

   sin.sin_addr = *(IN_ADDR *)hostinfo->h_addr;
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if (connect(sock, (SOCKADDR *)&sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
   {
      perror("connect()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_server(SOCKET sock, char *buffer)
{
   int n = 0;

   if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      exit(errno);
   }

   buffer[n] = 0;

   return n;
}

static void write_server(SOCKET sock, const char *buffer)
{
   if (send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

int main(int argc, char **argv)
{
   if (argc < 2)
   {
      printf("Usage : %s [address]\n", argv[0]);
      return EXIT_FAILURE;
   }

   init();

   app(argv[1]);

   end();

   return EXIT_SUCCESS;
}
