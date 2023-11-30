#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include "client2.h"

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

static void afficherPlateau(uint8_t *plateau, uint8_t joueur)
{

   int idx_row1, idx_row2;

   // indices des cases par lesquelles commencent les lignes à afficher
   idx_row1 = NB_CASES / 2 * (2 - joueur);
   idx_row2 = NB_CASES / 2 * joueur - 1;

   // affichage ligne par ligne
   printf("Point de vue du joueur %d\n", joueur);
   printf("Plateau de jeu :\n\n");
   for (int i = idx_row1; i < idx_row1 + 6; ++i)
   {
      printf(" | %d", plateau[i]);
   }
   printf(" |\n");
   for (int i = 0; i < NB_CASES / 2; ++i)
   {
      printf("-----");
   }
   printf("\n");
   for (int i = idx_row2; i > idx_row2 - 6; --i)
   {
      printf(" | %d", plateau[i]);
   }
   printf(" |\n\n");
}

static void app(const char *address, const char *name)
{
   SOCKET sock = init_connection(address);
   char buffer[BUF_SIZE];
   char buffLecture[BUF_SIZE - 1];

   fd_set rdfs;

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
         strncpy(buffer, "MENU", BUF_SIZE - 1);
         write_server(sock, buffer);
         break;
      }
      else if (buffer[0] == '0' && buffer[1] == '0')
      {
         printf("Le pseudo choisi est déjà pris. Saisissez votre pseudo : \n");
      }
   }

   while (1)
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
         case '1':
            strncpy(buffer, "1", BUF_SIZE - 1);
            break;
         case '2':
            strncpy(buffer, "2", BUF_SIZE - 1);
            printf("Saisissez le pseudo du joueur à affronter : \n");
            fgets(action, BUF_SIZE - 1, stdin);
            strncat(buffer, action, BUF_SIZE - strlen(buffer) - 1);
            break;
         case '4':
            strncpy(buffer, "4", BUF_SIZE - 1);
            printf("Saisissez la case où vous voulez jouer : \n");
            while (1)
            {
               fgets(action, BUF_SIZE - 1, stdin);
               int choice;
               int conversionSuccessful = sscanf(action, "%d", &choice);

               // Check if the input is a valid integer and within the desired range
               if (conversionSuccessful != 1 || choice < 1 || choice > 6)
               {
                  printf("Saisie invalide : il faut choisir un chiffre entre 1 et 6 !\n");
               }
               else
               {
                  break;
               }
            }
            strncat(buffer, action, BUF_SIZE - strlen(buffer) - 1);
            break;
         case '5':
            printf("Etes vous sur de vouloir abandonner ? (Y/N)\n");
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
            if (strcmp(action, "y") == 0 || strcmp(action, "Y") == 0){
               strncpy(buffer, "5", BUF_SIZE - 1);
            }
            break;
         default:
            strncpy(buffer, "MENU", BUF_SIZE - 1);
            break;
         }
         {

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
         char action = buffer[0];
         char contenu[BUF_SIZE - 1];
         char *plateau;
         strncpy(contenu, buffer + 1, BUF_SIZE - 2);
         switch (action)
         {
         case '3':
            printf("%s\n", contenu);
            fgets(contenu, BUF_SIZE - 2, stdin);
            buffLectureToBuff(&buffer, contenu, '3');
            write_server(sock, buffer);
            break;
         case '6':
            printf("%s\n", contenu);
            printf("Saisissez la case où vous voulez jouer : \n");
            while (1)
            {
               fgets(contenu, BUF_SIZE - 2, stdin);
               int choice;
               int conversionSuccessful = sscanf(contenu, "%d", &choice);

               // Check if the input is a valid integer and within the desired range
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
      printf("Usage : %s [address] [pseudo]\n", argv[0]);
      return EXIT_FAILURE;
   }

   init();

   app(argv[1], argv[2]);

   end();

   return EXIT_SUCCESS;
}
