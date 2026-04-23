#ifndef AST_H
#define AST_H

/* ═══════════════════════════════════════════════════════════════
   CONSTANTES PARTAGÉES
   ═══════════════════════════════════════════════════════════════ */
#define MAX_NAME    64
#define MAX_PARAMS  16
#define MAX_ARRAY   256   /* taille max d'un tableau */

/* ═══════════════════════════════════════════════════════════════
   AST — types de nœuds
   ═══════════════════════════════════════════════════════════════ */
typedef enum {
    NODE_AVANCER, NODE_RECULER, NODE_TOURNER, NODE_ORIENTATION,
    NODE_COULEUR, NODE_EPAISSEUR,
    NODE_STYLO_HAUT, NODE_STYLO_BAS,
    NODE_AFFICHER_EXPR, NODE_AFFICHER_STR,
    NODE_ASSIGN,
    NODE_REPETER, NODE_TANTQUE,
    NODE_SI, NODE_SI_SINON,
    NODE_SEQ, NODE_NOP,
    NODE_FUNC_DEF, NODE_FUNC_CALL,
    NODE_RETOURNER, NODE_CALL_EXPR,
    NODE_NUM, NODE_VAR, NODE_BINOP, NODE_UNOP,
    /* ── Tableaux ── */
    NODE_TAB_DECL,    /* tab nom[taille]        */
    NODE_TAB_ASSIGN,  /* nom[i] = expr          */
    NODE_TAB_READ,    /* expr utilisant nom[i]  */
} NodeType;

/* ═══════════════════════════════════════════════════════════════
   Structures AST
   ═══════════════════════════════════════════════════════════════ */
typedef struct Node Node;
typedef struct ArgList ArgList;

struct ArgList { Node *expr; ArgList *next; };

struct Node {
    NodeType  type;
    float     fval;
    char     *sval;
    int       op;
    int       ival;       /* utilisé pour taille tableau */
    Node     *left, *right, *extra;
    ArgList  *args;
    char    (*params)[MAX_NAME];
    int       param_count;
};

/* ═══════════════════════════════════════════════════════════════
   Constructeurs AST
   ═══════════════════════════════════════════════════════════════ */
Node *make_node(NodeType t);
Node *make_num(float v);
Node *make_var(char *s);
Node *make_binop(int op, Node *l, Node *r);
Node *make_unop(int op, Node *c);
Node *make_seq(Node *a, Node *b);

/* Point d'entrée de l'interpréteur */
void run_program(Node *root);

#endif /* AST_H */