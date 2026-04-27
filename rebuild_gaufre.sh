#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src"

echo "Suppression des fichiers .class..."
find "$SCRIPT_DIR" -name "*.class" -type f -delete

echo "Compilation des sources Java..."
cd "$SRC_DIR"
find . -name "*.java" -print0 | xargs -0 javac

echo "Lancement du jeu..."
java Gaufre
