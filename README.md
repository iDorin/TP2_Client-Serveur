L'énoncé: https://www.iro.umontreal.ca/~monnier/2245/h2017/tp2.pdf

## Scenario

### Initialisation
1. Lancement du serveur avec arguments: *port* et le *nombre de threads*.
 * Exemple d'arguments: 9000 2. Ici port = 9000, nombre de threads du serveur = 2.
1. Le serveur cree le socket et se met en attente d'une requête client.
1. Lancement du client avec arguments: *port*, *nb de clients*, *nb de requtes* de clients et les *nombres d'instances pour chaque type de ressource*.
 * Exemple d'arguments: 9000 2 3 1 1 1 1. Ici port = 9000, nombre de threads du client = 2, nb de requtes = 3, nombres d'instances pour chaque type de ressource = 1.
1. Le client convertit les arguments en variables respectives.
 * Compte le nombre d'instances pour BEG ... (ici on a BEG 4);
 * Les arg. 4 à 6 sont convertits en PRO ... (ici on a PRO 1 1 1 1);
1. Le client cree le socket et fait la connexion avec le serveur.
1. Le client envoie les requêtes BEG et PRO vers le serveur pour le configurer.
1. Le serveur renvoit au client le message de confirmation.
1. Le client affiche une notification dans le terminal.

### Requêtes
1. 
