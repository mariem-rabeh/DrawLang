# DrawLang

Un langage de dessin vectoriel inspiré de la tortue Logo, avec un IDE web intégré. Vous écrivez du code DrawLang, le serveur le compile et l'exécute, et le résultat s'affiche en SVG dans le navigateur.

---

## Architecture

```
drawlang_ide.html   →   server.js (Node.js)   →   drawlang (binaire C)
  interface web           API REST                  Flex + Bison + AST
```

Le pipeline complet pour une exécution :

1. L'IDE envoie le code via `POST /run` (JSON)
2. `server.js` écrit le code dans un fichier temporaire `.draw`
3. Le binaire C est lancé avec `execFile` — il lit le `.draw`, produit un `.svg`
4. Le serveur lit le SVG et le renvoie en JSON au navigateur
5. L'IDE injecte le SVG directement dans le DOM

---

## Structure du projet

```
.
├── draw.l              # Lexer Flex — tokenisation du code source
├── draw.y              # Parser Bison — grammaire et construction de l'AST
├── ast.h               # Définition des types de nœuds AST
├── ast.c               # Interpréteur — parcourt l'AST et exécute les instructions
├── svg.h               # API de la tortue SVG
├── svg.c               # Implémentation — gestion de la tortue et écriture SVG
├── server.js           # Backend Node.js / Express
└── drawlang_ide.html   # IDE web (frontend autonome)
```

---

## Prérequis

**Pour compiler le binaire C :**

- GCC (ou MinGW sur Windows)
- Flex
- Bison

**Pour le serveur Node.js :**

- Node.js ≥ 16
- npm

---

## Installation et démarrage

### 1. Compiler le binaire DrawLang

**Linux / macOS**
```bash
flex draw.l
bison -d draw.y
gcc -o drawlang draw.tab.c ast.c svg.c -lm -lfl
```

**Windows (MinGW)**
```bash
flex draw.l
bison -d draw.y
gcc -o drawlang.exe draw.tab.c ast.c svg.c -lm
```

Le binaire `drawlang` (ou `drawlang.exe`) doit se trouver dans le même dossier que `server.js`.

### 2. Installer les dépendances Node.js

```bash
npm install express
```

### 3. Démarrer le serveur

```bash
node server.js
```

Le serveur démarre sur `http://localhost:3000`. Vous verrez dans le terminal :

```
Plateforme : linux
Binaire attendu : /chemin/vers/drawlang
Binaire présent : true

DrawLang backend démarré → http://localhost:3000
Ouvrez drawlang_ide.html dans votre navigateur.
```

### 4. Ouvrir l'IDE

Ouvrez `drawlang_ide.html` directement dans votre navigateur (double-clic ou `file://`). Le badge de statut en haut à droite indique si le serveur et le binaire sont détectés.

---

## Utilisation

Écrivez du code DrawLang dans l'éditeur, puis cliquez sur **▶ Exécuter** ou appuyez sur `Ctrl+Entrée`.

### Exemple — un carré

```
couleur("#3d7eff");
epaisseur(3);
repeter 4 {
    avancer(120);
    tourner(90);
}
```

### Exemple — une spirale

```
couleur("#ff6b35");
epaisseur(2);
repeter 36 {
    avancer(5);
    tourner(10);
}
```

---

## Référence du langage

### Déplacement de la tortue

| Instruction | Description |
|---|---|
| `avancer(n)` | Avance de `n` pixels |
| `reculer(n)` | Recule de `n` pixels |
| `tourner(n)` | Tourne à droite de `n` degrés |
| `orientation(n)` | Fixe l'angle absolu (0° = droite, −90° = haut) |

### Stylo

| Instruction | Description |
|---|---|
| `couleur("valeur")` | Couleur du trait (nom CSS ou hex : `"red"`, `"#ff0000"`) |
| `epaisseur(n)` | Épaisseur du trait en pixels |
| `stylo_haut()` | Lève le stylo (déplacement sans tracer) |
| `stylo_bas()` | Abaisse le stylo (reprend le tracé) |

### Variables et expressions

```
soit x = 100;
soit angle = 360 / 6;
avancer(x);
tourner(angle);
```

### Structures de contrôle

```
repeter N { ... }

tantque condition { ... }

si condition { ... }
si condition { ... } sinon { ... }
```

### Fonctions

```
fonction carre(cote) {
    repeter 4 {
        avancer(cote);
        tourner(90);
    }
}

carre(80);
carre(120);
```

### Tableaux

```
tab valeurs[5];
valeurs[0] = 10;
afficher valeurs[0];
```

---

## API REST

### `GET /ping`

Vérifie que le serveur tourne et que le binaire est présent.

**Réponse**
```json
{
  "status": "ok",
  "binary": true,
  "platform": "linux"
}
```

### `POST /run`

Compile et exécute du code DrawLang.

**Corps de la requête**
```json
{
  "code": "avancer(100); tourner(90);"
}
```

**Réponse**
```json
{
  "ok": true,
  "logs": "[SVG] Fichier ouvert (800x600).\n[SVG] Fichier fermé.",
  "svg": "<svg xmlns=\"http://www.w3.org/2000/svg\" ...>...</svg>"
}
```

| Champ | Type | Description |
|---|---|---|
| `ok` | boolean | `true` si l'exécution a réussi et un SVG a été produit |
| `logs` | string | Sortie stdout + stderr du binaire (sans codes ANSI) |
| `svg` | string \| null | Contenu complet du fichier SVG généré |

---

## Fonctionnement interne

### Le lexer (`draw.l`)

Flex lit le code caractère par caractère et produit des tokens : `AVANCER`, `TOURNER`, `REPETER`, `NUMBER`, `STRING`, `IDENT`, etc. Les espaces et retours à la ligne sont ignorés.

### Le parser (`draw.y`)

Bison reçoit les tokens et applique les règles grammaticales pour construire l'AST. Chaque règle crée un nœud (`make_node`, `make_num`, `make_binop`…) et les enchaîne avec `make_seq`.

### L'AST (`ast.h` / `ast.c`)

Chaque instruction est représentée par un `Node` contenant son type (`NodeType`), ses valeurs (`fval`, `sval`) et ses enfants (`left`, `right`, `extra`). L'interpréteur `run()` parcourt l'arbre récursivement. `eval()` calcule la valeur `float` d'une expression.

### Le générateur SVG (`svg.c`)

Maintient l'état interne de la tortue (position, angle, couleur, épaisseur, stylo haut/bas) via des variables statiques globales. Chaque appel à `svg_avancer()` calcule la nouvelle position avec `cosf`/`sinf` et écrit une balise `<line>` dans le fichier de sortie.

---

## Dépannage

**Badge rouge "Serveur inaccessible"** — `server.js` n'est pas démarré. Lancez `node server.js`.

**Badge orange "Binaire introuvable"** — le binaire `drawlang` est absent ou mal compilé. Suivez les étapes de compilation et placez le binaire dans le même dossier que `server.js`.

**Erreur `[SYNTAXE]` dans la console** — le code DrawLang contient une erreur de syntaxe. Vérifiez les points-virgules, parenthèses et accolades.

**Timeout après 15 secondes** — le programme contient probablement une boucle infinie. Le serveur tue automatiquement le processus après 10 secondes.

---

## Licence

Projet académique — libre d'utilisation et de modification.
