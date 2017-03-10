## Scenario

### Initialisation
1. Lancement du serveur avec arguments: *port* et le *nombre de threads*.
 * Exemple d'arguments: 9000 2. Ici port = 9000, nombre de threads du serveur = 2.
1. Le serveur cree le socket et se met en attente d'une requête client.
1. Lancement du client avec arguments: *port*, *nb de clients*, *nb de requtes* de clients, *nombre de resources différentes* et les *nombres d'instances pour chaque type de ressource*.
 * Exemple d'arguments: 9000 2 3 4 1 1 1 1. Ici port = 9000, nombre de threads du client = 2, nb de requtes = 3, nombre de resources différentes = 4, nombres d'instances pour chaque type de ressource = 1.
1. Le client convertit les arguments en variables respectives.
 * L'arg. 3 est convertit en requete BEG ... (ici on a BEG 4);
 * Les arg. 3 à 7 sont convertits en PRO ... (ici on a PRO 1 1 1 1);
1. Le client cree le socket et fait la connexion avec le serveur.
1. Le client envoie les requêtes BEG et PRO vers le serveur pour le configurer.
1. Le serveur renvoit au client le message de confirmation.
1. Le client affiche une notification dans le terminal.

### Requêtes
1. 
