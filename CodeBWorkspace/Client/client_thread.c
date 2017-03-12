/* This `define` tells unistd to define usleep and random.  */
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "client_thread.h"

// Added:
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
struct sockaddr_in serv_addr;
struct hostent *server;       // pointer to a structure of type hostent
#define h_addr h_addr_list[0] // Host address, for backward compatibility


int port_number = -1;
int num_request_per_client = -1;
int num_resources = -1;
int *provisioned_resources = NULL;

// Variable d'initialisation des threads clients.
unsigned int count = 0;


// Variable du journal.
// Nombre de requête acceptée (ACK reçus en réponse à REQ)
unsigned int count_accepted = 0;

// Nombre de requête en attente (WAIT reçus en réponse à REQ)
unsigned int count_on_wait = 0;

// Nombre de requête refusée (REFUSE reçus en réponse à REQ)
unsigned int count_invalid = 0;

// Nombre de client qui se sont terminés correctement (ACC reçu en réponse à END)
unsigned int count_dispatched = 0;

// Nombre total de requêtes envoyées.
unsigned int request_sent = 0;

char hostname[] = "localhost"; // "192.168.1.82" for testing on remote server



int
connection_serveur (char hostname[])
{
  int socket_fd = -1;                          // Descripteur fichier du socket

  socket_fd = socket(AF_INET, SOCK_STREAM, 0); // Ne pas mettre SOCK_STREAM | SOCK_NONBLOCK
  if (socket_fd < 0) perror("ERROR opening socket");

  // Get host name
  server = gethostbyname(hostname);
  if (server == NULL) {
    fprintf(stderr,"ERROR, no such host\n");
    exit(0);
  }

  // Sets fields in serv_addr
  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;

  bcopy( (char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length );

  serv_addr.sin_port = htons(port_number);

  // Establish connection to the server
  int checks = 0;
  while ( connect(socket_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0 ) // attempt to connect to a socket
  {
    if ( checks > 3 )
    {
      perror("Server offline");
      close(socket_fd);
      return -1;
    }
    sleep(1);
    checks++;
  }

  //printf("Done!\n");

  return socket_fd;

}


// Send/receive commandes to/from server
char *
send_cmd (int client_id, char buffer[], int socket_fd)
{
  FILE  *socket = fdopen (socket_fd, "w+"); // Cree un fichier pour ecriture et lecture

  if (client_id == NULL)
  {
    printf("Client > %s", buffer);
  } else
    printf("Client %d > %s", client_id, buffer);

  // Envoye commande vers le serveur
  fprintf (socket, buffer);
  fflush(socket);

  //sleep (1); // simulation attente reponse

  // Recoit message du serveur
  bzero(buffer, 256);
  read(socket_fd, buffer, 255);
  if (client_id == NULL)
  {
    printf("Server > %s", buffer);
  } else
    printf("Server to client %d > %s", client_id, buffer);

  return buffer; // reponse du serveur
}


/*
 * Configuration du serveur avec BEG et PRO
 */
void
configurer_serveur (int num_resources, int *provisioned_resources) // NEW
{
  char buffer[256];
  char str[256];
  int  socket_fd;

  printf("Trying to connect to the server...\n");

  socket_fd = connection_serveur(hostname);

  if (socket_fd > 0)
  {
    printf("Starting server configuration...\n");

    // Envoye la commande BEG ...
    bzero(buffer, 256);
    bzero(str, 256);
    strcat(buffer, "BEG"); // concatenation
    sprintf(str, " %d", num_resources); // met dans str le num_ressources
    strcat(buffer, str);
    strcat(buffer, "\n");
    send_cmd (NULL, buffer, socket_fd);

    // Envoye la commande PRO ...
    strcat(buffer, "PRO");
    for (int i = 0; i < num_resources; i++)  // Met les ressources dans le buffer en tant que string
    {
      sprintf(str, " %d", provisioned_resources[i]);
      strcat(buffer, str);
    }
    strcat(buffer, "\n");
    send_cmd (NULL, buffer, socket_fd);
  } else
    exit(0); // si le serveur est offline

  close(socket_fd);
}


/*
 * Initialise un thread du client
 */
void
ct_init (client_thread * ct)
{
  ct->id = count++;
  //printf("Client %d initialised.\n", ct->id); // TEMP
}


/*
 * Cree et demarre un thread du client
 */
void
ct_create_and_start (client_thread * ct)
{
  pthread_attr_init ( &(ct->pt_attr) );
  pthread_create    ( &(ct->pt_tid), &(ct->pt_attr), &ct_code, ct );
  pthread_detach    ( ct->pt_tid );
}


/*
 * Mise a jour statistiques lors de l'exécution du serveur
 */
void
update_results (char reponse[])
{
  char str[] = "nil";

  for (int i = 0; i < 3; i++)
    str[i] = reponse[i];

  if ( !strcmp(str, "ACK") )
    count_accepted++;

  if ( !strcmp(str, "ERR") )
    count_invalid++;

  if ( !strcmp(str, "WAI") )
    count_on_wait++;

  if ( !strcmp(str, "REF") )
    count_invalid++;

  if ( !strcmp(str, "ACC") )
    count_dispatched++;

  request_sent++;
}



// Annonce du client avec son usage maximum
// Retourne un tableau de ressources d'usage maximum
int *
send_INI (int client_id, int socket_fd)
{
  char buffer[256] = "";
  char str[8] = "";
  int *valeurs = malloc(num_resources * sizeof (int));;

  strcat(buffer, "INI");
  for (int i = 0; i < num_resources; i++) // transforme les chiffres du INI en characteres
  {
    valeurs[i] = rand() % 10;
    sprintf( str, " %d", valeurs[i] );
    strcat(buffer, str);
  }

  strcat(buffer, "\n"); // Maintenant le buffer contient une commande complete

  // Envoyer commande et mise a jour resultats
  update_results ( send_cmd (client_id, buffer, socket_fd) );

  return valeurs;
}


//
// Vous devez modifier cette fonction pour faire l'envoie des requêtes.
// Les ressources demandées par la requête doivent être choisies aléatoirement
// (sans dépasser le maximum pour le client). Elles peuvent être positives
// ou négatives.
// Assurez-vous que la dernière requête d'un client libère toute les ressources
// qu'il a jusqu'alors accumulées.
void
send_request (int client_id, int request_id, int socket_fd, int valeurs_INI[])
{
  char buffer[256] = "";
  char str[8] = "";

  strcat(buffer, "REQ");

  // Genere des valeurs aleatoires pour REQ, mais <= que valeurs_INI[]
  for (int i = 0; i < num_resources; i++) // transforme les chiffres du REQ en characteres
  {
    if (valeurs_INI[i] == 0)
    {
      sprintf( str, " %d", 0 );
    } else
      sprintf( str, " %d", (rand() % (valeurs_INI[i] + 2) - 2) );
    strcat(buffer, str);
  }

  strcat(buffer, "\n"); // Maintenant le buffer contient une commande complete

  // Envoyer commande et mise a jour resultats
  update_results ( send_cmd (client_id, buffer, socket_fd) );

  // TP2 TODO:END

}


//
void
send_CLO (int client_id, int socket_fd)
{
  char buffer[256] = "CLO\n";

  // Envoyer commande et mise a jour resultats
  update_results ( send_cmd (client_id, buffer, socket_fd) );
}


/*
 * Le code execute par le thread du client
 */
void *
ct_code (void *param)
{
  int socket_fd = -1;                          // Descripteur fichier du socket
  client_thread *ct = (client_thread *) param; // Thread du client

  // TP2 TODO
  // Vous devez ici faire l'initialisation des petits clients (`INI`).

  // Le client essaie de se connecter
  //printf("Client %d > ", ct->id);
  printf("Client %d trying connection...\n", ct->id);
  socket_fd = connection_serveur(hostname);

  // Si la connection n'a pas reussie le thread fini
  if (socket_fd < 0)
  {
    printf("Client %d signed out.\n", ct->id);
    pthread_exit (NULL);
  }

  printf("... client %d connected.\n", ct->id);

  // Envoye requete INI. Recupere les valeurs de INI dans le tableau valeursINI
  int *valeurs_INI = send_INI (ct->id, socket_fd);


  // TP2 TODO:END

  for (unsigned int request_id = 0; request_id < num_request_per_client; request_id++)
    {

      // TP2 TODO
      // Vous devez ici coder, conjointement avec le corps de send request,
      // le protocole d'envoi de requête.

      // Requetes REQs
      send_request (ct->id, request_id, socket_fd, valeurs_INI);

      // TP2 TODO:END

      /* Attendre un petit peu (0s-0.1s) pour simuler le calcul.  */
      usleep (random () % (100 * 1000));
      /* struct timespec delay;
       * delay.tv_nsec = random () % (100 * 1000000);
       * delay.tv_sec = 0;
       * nanosleep (&delay, NULL); */
    }

  // Envoye requete CLO
  send_CLO (ct->id, socket_fd);

  close(socket_fd); // TEMP

  pthread_exit (NULL);

}



// Vous devez changer le contenu de cette fonction afin de régler le
// problème de synchronisation de la terminaison.
// Le client doit attendre que le serveur termine le traitement de chacune
// de ses requêtes avant de terminer l'exécution.
/*
 * Met le client en attente
 */
void
ct_wait_server (int num_clients, client_thread *client_threads)
{

  // TP2 TODO

  sleep (5); // le temps permit d'utilisations de ressources

  int socket_fd = connection_serveur (hostname);

  send_cmd (NULL, "END\n", socket_fd);

  for (int id = 0; id < num_clients; id++) // transforme les chiffres du REQ en characteres
  {
    // TO DO ?
  }

  close(socket_fd);

  // TP2 TODO:END

}


//
// Affiche les données recueillies lors de l'exécution du
// serveur.
// La branche else ne doit PAS être modifiée.
//
void
st_print_results (FILE * fd, bool verbose)
{
  if (fd == NULL)
    fd = stdout;
  if (verbose)
    {
      fprintf (fd, "\n---- Résultat du client ----\n");
      fprintf (fd, "Requêtes acceptées: %d\n", count_accepted);
      fprintf (fd, "Requêtes : %d\n", count_on_wait);
      fprintf (fd, "Requêtes invalides: %d\n", count_invalid);
      fprintf (fd, "Clients : %d\n", count_dispatched);
      fprintf (fd, "Requêtes envoyées: %d\n", request_sent);
    }
  else
    {
      fprintf (fd, "%d %d %d %d %d\n", count_accepted, count_on_wait,
	       count_invalid, count_dispatched, request_sent);
    }
}
