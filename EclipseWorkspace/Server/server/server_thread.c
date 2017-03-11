#define _XOPEN_SOURCE 700   /* So as to allow use of `fdopen` and `getline`.  */

#include "server_thread.h"
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>

enum { NUL = '\0' };

enum {
	/* Configuration constants.  */
	max_wait_time = 30,
	server_backlog_size = 5
};

// Le socket du serveur
unsigned int server_socket_fd;

// Variable du journal.
// Nombre de requêtes acceptées immédiatement (ACK envoyé en réponse à REQ).
unsigned int count_accepted = 0;

// Nombre de requêtes acceptées après un délai (ACK après REQ, mais retardé).
unsigned int count_wait = 0;

// Nombre de requête erronées (ERR envoyé en réponse à REQ).
unsigned int count_invalid = 0;

// Nombre de clients qui se sont terminés correctement
// (ACK envoyé en réponse à CLO).
unsigned int count_dispatched = 0;

// Nombre total de requête (REQ) traités.
unsigned int request_processed = 0;

// Nombre de clients ayant envoyé le message CLO.
unsigned int clients_ended = 0;

// TODO: Ajouter vos structures de données partagées, ici.
int *available;

/* Nombre de resources différentes.  */
int num_resources;

/* Quantité disponible pour chaque resource.  */
int *provisioned_resources;


/*
 * Ouvre un socket pour le serveur.
 * Initialise variable-socket: server_socket_fd
 */
void
st_open_socket (int port_number)
{
	server_socket_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

	if (server_socket_fd < 0) perror("ERROR opening socket");

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons (port_number);

	if ( bind( server_socket_fd, (struct sockaddr *) &serv_addr,
			sizeof (serv_addr)) < 0 )
		perror ("ERROR on binding");

	listen (server_socket_fd, server_backlog_size);
}


/*
 * Attend la connection d'un client.
 * Initialise les donnees pour l'algo du banquier ?
 */
void
st_init ()
{
	// TODO

	// Attend la connection d'un client et initialise les structures pour
	// l'algorithme du banquier.


	// Attend la connection du client

	struct sockaddr_in  serv_addr, cli_addr;
	socklen_t           clilen = sizeof (cli_addr);

	int newsockfd = -1;

	printf("The server waiting for initialisation...\n");

	while (newsockfd < 0)
		newsockfd = accept (server_socket_fd, (struct sockaddr *) &cli_addr, &clilen); // attend jusqu'a la initialisation du client


	// Initialisation structures pour l'algo du banquier

	server_thread *st = NULL; // server thread necessaire seulement pour l'argument de la fonction  st_process_requests()

	st_process_requests (st, newsockfd);

	//printf("num_resources = %d\n", num_resources); // pour tester
	//printf("provisioned_resources = %d\n", provisioned_resources[0]);  // pour tester


	printf("The server is initialised.\n");

	// END TODO
}


/*
 * Le code executé par un thread
 * Variables utilisees: server_socket_fd, max_wait_time
 */
void *
st_code (void *param)
{
	server_thread *st = (server_thread *) param;

	struct sockaddr_in  thread_addr;
	socklen_t           socket_len       = sizeof (thread_addr);
	int                 thread_socket_fd = -1;
	int                 end_time         = time (NULL) + max_wait_time;

	// Boucle jusqu'à ce que `accept` reçoive la première connection.
	while (thread_socket_fd < 0)
	{
		thread_socket_fd =
				accept (server_socket_fd, (struct sockaddr *) &thread_addr, &socket_len); // attend jusqu'a la requete d'un client

		if (time (NULL) >= end_time) break; // time() returns the current time
	}

	// Boucle de traitement des requêtes.
	while (accepting_connections) // variable globale
	{
		if (time (NULL) >= end_time) // if timeout...
		{
			fprintf (stderr, "Time out on thread %d.\n", st->id);
			pthread_exit (NULL);
		}

		// Si le socket est cree, executer la fonction de requetes. Ferme le socket...
		if (thread_socket_fd > 0)
		{
			st_process_requests (st, thread_socket_fd);
			close(thread_socket_fd);
			end_time = time (NULL) + max_wait_time;
		}

		thread_socket_fd =
				accept(server_socket_fd, (struct sockaddr *) &thread_addr, &socket_len);
	}

	return NULL;
}



/*
 * Traitement de commandes globales BEG, PRO et END.
 */
void
traitement_cmd_globale (char cmd[], char *args)
{
	// Traitement commande BEG.
	// Recupere le nombre de ressources et initialise num_resources
	if ( !strcmp(cmd, "BEG") )
	{
		num_resources = atoi(args);
		//printf("num_resources = %d\n", num_resources); // pour tester
	}

	// Traitement commande PRO. Recupere le nombre de d'instances pour chaque ressource
	// et initialise provisioned_resources[]
	if ( !strcmp(cmd, "PRO") ) // si la commande est PRO...
	{
		char str[256]; // dans ce string on collecte les valeurs du PRO
		int index = 0; // indexe du provision_ressources[]

		provisioned_resources = malloc(num_resources * sizeof (int));

		for (int i = 1; i < strlen(args); i++) // 0-me char est un " ", alors on commence par indexe 1.
		{
			int j = 0; // indexe dans l'interior du string du valeur str
			bzero(str, 256);

			while ( i < strlen(args) && args[i] != ' ')
			{
				str[j] = args[i];
				j++;
				i++;
			}

			provisioned_resources[ index ] = atoi( str );
			//printf("provisioned_resources = %d\n", provisioned_resources[index]); // pour tester
			index++;
		}
	}

	// Traitement commande END.
	// TO DO
}


/*
 * Traitement de requetes.
 */
void
st_process_requests (server_thread * st, int socket_fd)
{
	// TODO: Remplacer le contenu de cette fonction
	FILE *socket = fdopen (socket_fd, "r+"); // ouvre un fichier pour lecture et ecriture

	// Traitement 0..* requetes ?
	while (true)
	{
		char cmd[4] = {NUL, NUL, NUL, NUL}; // la commande recue. Pourquoi 4 characteres ?

		// Validation commande.
		// Si le nom de la commande cmd ne contient pas 3 caracteres => break
		if ( !fread(cmd, 3, 1, socket) ) break;
		fflush(socket);

		// Read entire line from stream
		char   *args     = NULL;
		size_t  args_len = 0;
		ssize_t cnt      = getline(&args, &args_len, socket);

		// Si la commande recue est incomplete...
		if (!args || cnt < 1 || args[cnt - 1] != '\n')
		{
			if ( st == NULL )
			{
				printf ("Server received incomplete cmd = %s!\n", cmd); // cas d'initialisation du serveur
			} else
				printf ("Thread %d received incomplete cmd = %s!\n", st->id, cmd);

			printf (" incomplete cmd\n");
			break;
		}

		// Initialisation serveur
		if ( st == NULL )
		{
			printf ("Server received the command: %s%s", cmd, args);

			traitement_cmd_globale (cmd, args);

		} else
			printf ("Thread %d received the command: %s%s", st->id, cmd, args);


		//fprintf (socket, "ERR Unknown command\n"); // sends formatted output to a stream
		fprintf (socket, "ACK\n"); // sends formatted output to a stream

		free (args); // deallocates the block of memory...
	}

	fclose (socket);
	// TODO end
}


/*
 * Signale aux clients de se terminer
 */
void
st_signal ()
{
	// TODO: Remplacer le contenu de cette fonction

	printf("Lancement signal aux clients de se terminer...\n"); // TEMP

	fprintf (socket, "Ce client doit se terminer...\n"); // sends formatted output to a stream

	//close(server_socket_fd); // TEMP
	// TODO end
}


/*
 * Affiche les données recueillies lors de l'exécution du serveur.
 * La branche else ne doit PAS être modifiée
 */
void
st_print_results (FILE * fd, bool verbose)
{
	if (fd == NULL) fd = stdout;
	if (verbose)
	{
		fprintf (fd, "\n---- Résultat du serveur ----\n");
		fprintf (fd, "Requêtes acceptées: %d\n", count_accepted);
		fprintf (fd, "Requêtes : %d\n",          count_wait);
		fprintf (fd, "Requêtes invalides: %d\n", count_invalid);
		fprintf (fd, "Clients : %d\n",           count_dispatched);
		fprintf (fd, "Requêtes traitées: %d\n",  request_processed);
	}
	else
	{
		fprintf (fd, "%d %d %d %d %d\n", count_accepted, count_wait,
				count_invalid, count_dispatched, request_processed);
	}
}
