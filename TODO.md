## Etape I – Serveur FTP de base

### Préliminaires/Bases

- [x] ***(Question 1)*** Définir le type enum des type de requêtes des  (implémenter le `typereq_t`) 
- [x] ***(Question 2)*** Définir la structure de donnée des requêtes (implémenter `request_t`) 
- [x] ***(Question 3)*** cp du tp sur la partie ECHO + Définir le squelette des différentes composantes (bonne modularité)
- [x] ***(Question 4)*** Gestion du boutisme
- [X] ***(Question 5)*** Définir les répertoires de travail par défaut côté client et côté serveur (distincts pour permettre une exécution sur la même machine)

### Gestion Serveur

- [X] ***(Question 6)*** Identifier et valider le type de requête reçue ; renvoyer une erreur si type invalide
- [X] ***(Question 6)*** Définir une structure de donnée pour les réponses du serveur (implémenter `response_t` (au moins un champ entier code de retour succès/erreur))
- [X] ***(Question 6)*** Traitement GET : chargement du fichier en mémoire en une seule fois, envoi au client (tout type de fichier, y compris binaire) ; code erreur si fichier absent
- [x] ***(Question 6)*** Fermer connexion après traitement de la requête (côté serveur)

### Gestion Client

- [x] ***(Question 7)*** Côté client : encoder la saisie utilisateur dans `request_t` et envoyer au serveur
- [x] ***(Question 7)*** Côté client : traiter la réponse du serveur : affichage d'un message d'erreur pertinent ou confirmation de succès + statistiques (taille / temps / débit)

---

## Etape II – Améliorations


### Plusieurs paquets par requête
- [X] ***(Question 8)*** Découpage en blocs de taille fixe : définir le protocole (taille de bloc, nombre de blocs communiqué au client avant envoi), envoi et réception par blocs (création d'une structure de paquet)

### Plusieurs fichiers par connexion
- [X] ***(Question 9)*** Gérer plusieurs demande de fichiers par connexion (terminer avec la commande "bye" par le client)

### Gestion deconnexions et reprise de transfert
- [x] ***(Question 10)*** Côté serveur : détecter la déconnexion du client (voir "FTP-SR-vania.pdf" pour savoir comment détecter) et nettoyer proprement les structures système
- [x] ***(Question 10)*** Côté client : stocker localement l'avancement du transfert (fichier caché, log…) pour permettre la reprise au relancement

---

## Etape III – Répartition de charge

### Architecture maître-esclaves
- [x] ***(Question 11)*** Implémenter un serveur maître (constante `NB_SLAVES`, port dédié aux esclaves (différent de 2121))
- [x] ***(Question 12)*** Implémenter l'interconnexion avec les esclaves (le maître établit `NB_SLAVES` connexions)
- [x] ***(Question 13)*** Adapter le protocole client : après connexion au maître, recevoir les infos de l'esclave désigné, puis se connecter directement à celui-ci.
- [ ] ***(Question 14 BONUS)*** Reconnexion en cas de panne d'un esclave.

---

## Etape IV – Opérations avancées

### Opérations sur les fichiers
- [x] ***(Question 15)*** Implémenter commande `ls` : retourner le contenu du répertoire courant du serveur.
- [ ] ***(Question 16)*** Implémenter commandes `rm` et `put` : suppression / téléversement de fichier ;
- [ ] ***(Question 16)*** Implémenter la cohérence faible

### Sécurité
- [ ] ***(Question 17)*** Authentification : système login/mot de passe requis pour `rm` et `put` ; échec si non authentifié au préalable

---

## Extentions
- [ ] EXTENTION Doxygen
- [ ] EXTENTION CuTest
- [ ] EXTENTION Chiffrement des données