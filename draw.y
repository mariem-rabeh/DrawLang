%{
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "svg.h"
#include "ast.h"

#define ANSI_RED   "\033[1;31m"
#define ANSI_CYAN  "\033[1;36m"
#define ANSI_GREEN "\033[1;32m"
#define ANSI_RESET "\033[0m"

int yylex(void);
int yyerror(char *s);
int line_num = 1;
static Node *g_root = NULL;
%}

%union {
    int   val;
    float fval;
    char *str;
    struct Node    *node;
    struct ArgList *args;
    struct { char (*params)[MAX_NAME]; int count; } plist;
}

%token AVANCER RECULER TOURNER ORIENTATION REPETER TANTQUE SI SINON
%token FONCTION RETOURNER AFFICHER TAB
%token COULEUR EPAISSEUR STYLO_HAUT STYLO_BAS
%token <val>  NUMBER
%token <fval> TFLOAT
%token <str>  ID STRING
%token EQ NEQ GEQ LEQ GT LT AND OR NOT

%type <node>  expr instruction programme
%type <args>  call_args call_args_list
%type <plist> param_list param_list_ne

%left OR
%left AND
%right NOT
%left EQ NEQ
%left GT LT GEQ LEQ
%left '+' '-'
%left '*' '/' '%'
%right UMINUS

%%

programme:
      programme instruction  { $$ = make_seq($1, $2); g_root = $$; }
    | instruction             { $$ = $1; g_root = $$; }
;

param_list:
      /* vide */    { $$.params = NULL; $$.count = 0; }
    | param_list_ne { $$ = $1; }
;
param_list_ne:
      ID
        { $$.params = malloc(MAX_PARAMS * sizeof(char[MAX_NAME]));
          strncpy($$.params[0], $1, MAX_NAME-1); free($1); $$.count = 1; }
    | param_list_ne ',' ID
        { if ($1.count >= MAX_PARAMS) { yyerror("trop de parametres"); exit(1); }
          strncpy($1.params[$1.count], $3, MAX_NAME-1); free($3);
          $$.params = $1.params; $$.count = $1.count + 1; }
;

call_args:
      /* vide */     { $$ = NULL; }
    | call_args_list { $$ = $1; }
;
call_args_list:
      expr
        { ArgList *a = calloc(1, sizeof(ArgList)); a->expr = $1; $$ = a; }
    | call_args_list ',' expr
        { ArgList *a = calloc(1, sizeof(ArgList)); a->expr = $3;
          ArgList *c = $1; while (c->next) c = c->next; c->next = a; $$ = $1; }
;

instruction:
      /* ── Variables scalaires ── */
      ID '=' expr ';'
        { Node *n = make_node(NODE_ASSIGN); n->sval = $1; n->left = $3; $$ = n; }

      /* ── Tableaux ── */
    | TAB ID '[' NUMBER ']' ';'
        {
            Node *n = make_node(NODE_TAB_DECL);
            n->sval = $2;
            n->ival = $4;
            $$ = n;
        }
    | ID '[' expr ']' '=' expr ';'
        {
            Node *n = make_node(NODE_TAB_ASSIGN);
            n->sval  = $1;
            n->left  = $3;   /* index */
            n->right = $6;   /* valeur */
            $$ = n;
        }

      /* ── Commandes tortue ── */
    | AVANCER      '(' expr ')' ';'   { Node *n = make_node(NODE_AVANCER);      n->left = $3; $$ = n; }
    | RECULER      '(' expr ')' ';'   { Node *n = make_node(NODE_RECULER);      n->left = $3; $$ = n; }
    | TOURNER      '(' expr ')' ';'   { Node *n = make_node(NODE_TOURNER);      n->left = $3; $$ = n; }
    | ORIENTATION  '(' expr ')' ';'   { Node *n = make_node(NODE_ORIENTATION);  n->left = $3; $$ = n; }
    | COULEUR      '(' STRING ')' ';' { Node *n = make_node(NODE_COULEUR);      n->sval = $3; $$ = n; }
    | EPAISSEUR    '(' expr ')' ';'   { Node *n = make_node(NODE_EPAISSEUR);    n->left = $3; $$ = n; }
    | STYLO_HAUT   '(' ')' ';'        { $$ = make_node(NODE_STYLO_HAUT); }
    | STYLO_BAS    '(' ')' ';'        { $$ = make_node(NODE_STYLO_BAS);  }

      /* FIX: STRING avant expr pour éviter le conflit reduce/reduce */
    | AFFICHER '(' STRING ')' ';'
        { Node *n = make_node(NODE_AFFICHER_STR);  n->sval = $3; $$ = n; }
    | AFFICHER '(' expr   ')' ';'
        { Node *n = make_node(NODE_AFFICHER_EXPR); n->left = $3; $$ = n; }

      /* ── Structures de contrôle ── */
    | REPETER expr '{' programme '}'
        { Node *n = make_node(NODE_REPETER); n->left = $2; n->right = $4; $$ = n; }
    | TANTQUE expr '{' programme '}'
        { Node *n = make_node(NODE_TANTQUE); n->left = $2; n->right = $4; $$ = n; }
    | SI expr '{' programme '}'
        { Node *n = make_node(NODE_SI); n->left = $2; n->right = $4; $$ = n; }
    | SI expr '{' programme '}' SINON '{' programme '}'
        { Node *n = make_node(NODE_SI_SINON); n->left=$2; n->right=$4; n->extra=$8; $$ = n; }

      /* ── Fonctions ── */
    | FONCTION ID '(' param_list ')' '{' programme '}'
        { Node *n = make_node(NODE_FUNC_DEF); n->sval=$2;
          n->params=$4.params; n->param_count=$4.count; n->left=$7; $$ = n; }
    | RETOURNER expr ';'
        { Node *n = make_node(NODE_RETOURNER); n->left = $2; $$ = n; }
    | ID '(' call_args ')' ';'
        { Node *n = make_node(NODE_FUNC_CALL); n->sval = $1; n->args = $3; $$ = n; }
;

expr:
      expr '+' expr           { $$ = make_binop('+', $1, $3); }
    | expr '-' expr           { $$ = make_binop('-', $1, $3); }
    | expr '*' expr           { $$ = make_binop('*', $1, $3); }
    | expr '/' expr           { $$ = make_binop('/', $1, $3); }
    | expr '%' expr           { $$ = make_binop('%', $1, $3); }
    | expr EQ  expr           { $$ = make_binop('E', $1, $3); }
    | expr NEQ expr           { $$ = make_binop('N', $1, $3); }
    | expr GT  expr           { $$ = make_binop('>', $1, $3); }
    | expr LT  expr           { $$ = make_binop('<', $1, $3); }
    | expr GEQ expr           { $$ = make_binop('G', $1, $3); }
    | expr LEQ expr           { $$ = make_binop('L', $1, $3); }
    | expr AND expr           { $$ = make_binop('&', $1, $3); }
    | expr OR  expr           { $$ = make_binop('|', $1, $3); }
    | NOT expr                { $$ = make_unop('!', $2); }
    | '-' expr %prec UMINUS   { $$ = make_unop('M', $2); }
    | '(' expr ')'            { $$ = $2; }
    | NUMBER                  { $$ = make_num((float)$1); }
    | TFLOAT                  { $$ = make_num($1); }
    | ID '(' call_args ')'    { Node *n = make_node(NODE_CALL_EXPR); n->sval=$1; n->args=$3; $$=n; }

      /* ── Lecture tableau dans une expression ── */
    | ID '[' expr ']'
        {
            Node *n = make_node(NODE_TAB_READ);
            n->sval = $1;
            n->left = $3;
            $$ = n;
        }

    | ID                      { $$ = make_var($1); }
;

%%

#include "lex.yy.c"

int yyerror(char *s) {
    fprintf(stderr, ANSI_RED "[SYNTAXE]" ANSI_RESET " ligne %d: %s\n", line_num, s);
    return 0;
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif
    const char *svg_out = "output.svg";
    if (argc > 2) svg_out = argv[2];

    FILE *src_file = NULL;
    if (argc > 1) {
        src_file = fopen(argv[1], "r");
        if (!src_file) {
            fprintf(stderr, ANSI_RED "[ERREUR]" ANSI_RESET " Impossible d'ouvrir: %s\n", argv[1]);
            return 1;
        }
        yyin = src_file;
        printf(ANSI_CYAN "=== Execution de %s ===" ANSI_RESET "\n\n", argv[1]);
    } else {
        printf(ANSI_CYAN "=== Mode interactif ===" ANSI_RESET "\n\n");
    }

    svg_init(svg_out, 800, 600);
    int parse_ok = yyparse();
    if (src_file) { fclose(src_file); src_file = NULL; }
    if (parse_ok == 0 && g_root != NULL)
        run_program(g_root);
    svg_finish();
    return 0;
}
