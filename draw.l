%{
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "draw.tab.h"

extern int line_num;
%}

%%

"avancer"         { return AVANCER; }
"reculer"         { return RECULER; }
"tourner"         { return TOURNER; }
"orientation"     { return ORIENTATION; }
"repeter"         { return REPETER; }
"tantque"         { return TANTQUE; }
"si"              { return SI; }
"sinon"           { return SINON; }
"fonction"        { return FONCTION; }
"retourner"       { return RETOURNER; }
"afficher"        { return AFFICHER; }
"couleur"         { return COULEUR; }
"epaisseur"       { return EPAISSEUR; }
"stylo_haut"      { return STYLO_HAUT; }
"stylo_bas"       { return STYLO_BAS; }
"tab"             { return TAB; }

[0-9]+\.[0-9]+    { yylval.fval = atof(yytext); return TFLOAT; }
[0-9]+            { yylval.val  = atoi(yytext); return NUMBER; }

\"[^\"]*\"        {
                      int len = strlen(yytext) - 2;
                      yylval.str = (char*)malloc(len + 1);
                      strncpy(yylval.str, yytext + 1, len);
                      yylval.str[len] = '\0';
                      return STRING;
                  }

[a-zA-Z_][a-zA-Z0-9_]* {
                      yylval.str = strdup(yytext);
                      return ID;
                  }

"+"   { return '+'; }
"-"   { return '-'; }
"*"   { return '*'; }
"/"   { return '/'; }
"%"   { return '%'; }
"="   { return '='; }
";"   { return ';'; }
","   { return ','; }
"("   { return '('; }
")"   { return ')'; }
"{"   { return '{'; }
"}"   { return '}'; }
"["   { return '['; }
"]"   { return ']'; }

"=="  { return EQ;  }
"!="  { return NEQ; }
">="  { return GEQ; }
"<="  { return LEQ; }
">"   { return GT;  }
"<"   { return LT;  }
"&&"  { return AND; }
"||"  { return OR;  }
"!"   { return NOT; }

\n            { line_num++; }
[ \t\r]       { }
"#"[^\n]*     { }

.   { printf("Erreur lexicale ligne %d: caractere inconnu '%s'\n", line_num, yytext); }

%%

int yywrap(){ return 1; }
