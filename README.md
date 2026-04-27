# Gaufre Empoisonnee

Ce projet est une application Java Swing.

## Prerequis

- Java JDK installe (`javac` et `java` disponibles dans le terminal)

## Lancer le projet manuellement

Depuis le dossier `GaufreEmpoisonnee` :

```bash
cd src
find . -name "*.java" -print0 | xargs -0 javac
java Gaufre
```

## Nettoyer, reconstruire et lancer automatiquement

Depuis le dossier `GaufreEmpoisonnee` :

```bash
./rebuild_gaufre.sh
```

Ce script :

- supprime tous les fichiers `.class` du projet
- recompile toutes les sources Java de `src`
- lance le jeu avec `java Gaufre`
