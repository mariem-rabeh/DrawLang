#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "svg.h"
#include "ast.h"

/* ═══════════════════════════════════════════════════════════════
   COULEURS ANSI
   ═══════════════════════════════════════════════════════════════ */
#define ANSI_RED     "\033[1;31m"
#define ANSI_GREEN   "\033[1;32m"
#define ANSI_YELLOW  "\033[1;33m"
#define ANSI_CYAN    "\033[1;36m"
#define ANSI_MAGENTA "\033[1;35m"
#define ANSI_RESET   "\033[0m"

#define ERR(fmt, ...)  fprintf(stderr, ANSI_RED    "[ERREUR]" ANSI_RESET " ligne %d: " fmt "\n", line_num, ##__VA_ARGS__)
#define WARN(fmt, ...) fprintf(stderr, ANSI_YELLOW "[AVERT]"  ANSI_RESET " ligne %d: " fmt "\n", line_num, ##__VA_ARGS__)
#define INFO(fmt, ...) printf(          ANSI_CYAN   "[INFO]"   ANSI_RESET " " fmt "\n", ##__VA_ARGS__)
#define OK(fmt, ...)   printf(          ANSI_GREEN  "[OK]"     ANSI_RESET " " fmt "\n", ##__VA_ARGS__)

/* ═══════════════════════════════════════════════════════════════
   CONSTANTES
   ═══════════════════════════════════════════════════════════════ */
#define CONST_PI      3.14159265358979f
#define CONST_VRAI    1.0f
#define CONST_FAUX    0.0f
#define MAX_VARS      256
#define MAX_FUNCS      64
#define MAX_ARRAYS     64
#define MAX_LOOP_ITER  100000
#define MAX_CALL_DEPTH 64
#define FLOAT_EPSILON  1e-6f

extern int line_num;

/* ═══════════════════════════════════════════════════════════════
   TABLE DE SYMBOLES (variables scalaires)
   ═══════════════════════════════════════════════════════════════ */
typedef struct { char name[MAX_NAME]; float value; int frame; } Symbol;
static Symbol sym_table[MAX_VARS];
static int    sym_count = 0;
static int    cur_frame = 0;

static float get_var(const char *name) {
    if (strcmp(name, "PI")   == 0) return CONST_PI;
    if (strcmp(name, "VRAI") == 0) return CONST_VRAI;
    if (strcmp(name, "FAUX") == 0) return CONST_FAUX;
    for (int f = cur_frame; f >= 0; f--)
        for (int i = sym_count - 1; i >= 0; i--)
            if (sym_table[i].frame == f && strcmp(sym_table[i].name, name) == 0)
                return sym_table[i].value;
    ERR("variable '%s' non definie", name);
    exit(1);
}

static void set_var(const char *name, float value) {
    for (int i = sym_count - 1; i >= 0; i--)
        if (sym_table[i].frame == cur_frame && strcmp(sym_table[i].name, name) == 0) {
            sym_table[i].value = value;
            return;
        }
    if (sym_count >= MAX_VARS) { ERR("trop de variables"); exit(1); }
    strncpy(sym_table[sym_count].name, name, MAX_NAME - 1);
    sym_table[sym_count].value = value;
    sym_table[sym_count].frame = cur_frame;
    sym_count++;
}

static void push_frame(void) { cur_frame++; }

static void pop_frame(void) {
    if (cur_frame <= 0) { WARN("pop_frame: frame global, ignore"); return; }
    int j = 0;
    for (int i = 0; i < sym_count; i++)
        if (sym_table[i].frame != cur_frame)
            sym_table[j++] = sym_table[i];
    sym_count = j;
    cur_frame--;
}

/* ═══════════════════════════════════════════════════════════════
   TABLE DES TABLEAUX
   ═══════════════════════════════════════════════════════════════ */
typedef struct {
    char  name[MAX_NAME];
    float data[MAX_ARRAY];
    int   size;
    int   frame;
} ArrayEntry;

static ArrayEntry arr_table[MAX_ARRAYS];
static int        arr_count = 0;

static ArrayEntry *find_array(const char *name) {
    for (int i = arr_count - 1; i >= 0; i--)
        if (strcmp(arr_table[i].name, name) == 0)
            return &arr_table[i];
    return NULL;
}

static void decl_array(const char *name, int size) {
    if (size <= 0 || size > MAX_ARRAY) {
        ERR("tableau '%s': taille invalide (%d), doit etre entre 1 et %d", name, size, MAX_ARRAY);
        exit(1);
    }
    /* redéclaration ? */
    ArrayEntry *existing = find_array(name);
    if (existing) {
        WARN("tableau '%s' redeclare, reinitialise", name);
        existing->size = size;
        memset(existing->data, 0, sizeof(float) * size);
        return;
    }
    if (arr_count >= MAX_ARRAYS) { ERR("trop de tableaux"); exit(1); }
    strncpy(arr_table[arr_count].name, name, MAX_NAME - 1);
    arr_table[arr_count].size  = size;
    arr_table[arr_count].frame = cur_frame;
    memset(arr_table[arr_count].data, 0, sizeof(float) * MAX_ARRAY);
    arr_count++;
    OK("Tableau '%s[%d]' declare", name, size);
}

static float get_array(const char *name, int idx) {
    ArrayEntry *a = find_array(name);
    if (!a) { ERR("tableau '%s' non declare", name); exit(1); }
    if (idx < 0 || idx >= a->size) {
        ERR("tableau '%s': index %d hors bornes [0..%d]", name, idx, a->size - 1);
        exit(1);
    }
    return a->data[idx];
}

static void set_array(const char *name, int idx, float val) {
    ArrayEntry *a = find_array(name);
    if (!a) { ERR("tableau '%s' non declare", name); exit(1); }
    if (idx < 0 || idx >= a->size) {
        ERR("tableau '%s': index %d hors bornes [0..%d]", name, idx, a->size - 1);
        exit(1);
    }
    a->data[idx] = val;
}

/* ═══════════════════════════════════════════════════════════════
   TABLE DES FONCTIONS
   ═══════════════════════════════════════════════════════════════ */
typedef struct {
    char  name[MAX_NAME];
    char  params[MAX_PARAMS][MAX_NAME];
    int   param_count;
    Node *body;
} Function;

static Function func_table[MAX_FUNCS];
static int      func_count   = 0;
static int      g_call_depth = 0;
static int      in_function  = 0;

typedef struct { float value; int active; } ReturnException;
static ReturnException g_return = {0, 0};

/* ═══════════════════════════════════════════════════════════════
   Constructeurs AST
   ═══════════════════════════════════════════════════════════════ */
Node *make_node(NodeType t) {
    Node *n = calloc(1, sizeof(Node));
    if (!n) { fprintf(stderr, "make_node: allocation echouee\n"); exit(1); }
    n->type = t;
    return n;
}
Node *make_num(float v)              { Node *n = make_node(NODE_NUM);   n->fval = v;                  return n; }
Node *make_var(char *s)              { Node *n = make_node(NODE_VAR);   n->sval = s;                  return n; }
Node *make_binop(int op, Node *l, Node *r) { Node *n = make_node(NODE_BINOP); n->op=op; n->left=l; n->right=r; return n; }
Node *make_unop(int op, Node *c)     { Node *n = make_node(NODE_UNOP);  n->op=op; n->left=c;          return n; }
Node *make_seq(Node *a, Node *b)     {
    if (!a) return b; if (!b) return a;
    Node *n = make_node(NODE_SEQ); n->left=a; n->right=b; return n;
}

/* ═══════════════════════════════════════════════════════════════
   Fonctions
   ═══════════════════════════════════════════════════════════════ */
static float eval(Node *n);
static void  run(Node *n);

static void register_func(const char *name, char params[][MAX_NAME], int pc, Node *body) {
    for (int i = 0; i < func_count; i++) {
        if (strcmp(func_table[i].name, name) == 0) {
            WARN("redefinition de la fonction '%s'", name);
            func_table[i].body = body; func_table[i].param_count = pc;
            for (int j = 0; j < pc; j++) strncpy(func_table[i].params[j], params[j], MAX_NAME-1);
            return;
        }
    }
    if (func_count >= MAX_FUNCS) { ERR("trop de fonctions"); exit(1); }
    strncpy(func_table[func_count].name, name, MAX_NAME-1);
    func_table[func_count].param_count = pc;
    for (int j = 0; j < pc; j++) strncpy(func_table[func_count].params[j], params[j], MAX_NAME-1);
    func_table[func_count].body = body;
    func_count++;
    OK("Fonction '%s' definie (%d param(s))", name, pc);
}

static float call_func(const char *name, ArgList *args) {
    if (g_call_depth >= MAX_CALL_DEPTH) { ERR("recursion infinie dans '%s'", name); exit(1); }
    Function *f = NULL;
    for (int i = 0; i < func_count; i++)
        if (strcmp(func_table[i].name, name) == 0) { f = &func_table[i]; break; }
    if (!f) { ERR("fonction '%s' non definie", name); exit(1); }
    int argc = 0;
    for (ArgList *a = args; a; a = a->next) argc++;
    if (argc != f->param_count) { ERR("fonction '%s': %d arg(s) attendus, %d recu(s)", name, f->param_count, argc); exit(1); }
    float vals[MAX_PARAMS];
    ArgList *a = args;
    for (int i = 0; i < argc; i++, a = a->next) vals[i] = eval(a->expr);
    g_call_depth++; push_frame();
    for (int i = 0; i < f->param_count; i++) set_var(f->params[i], vals[i]);
    g_return.active = 0; in_function++;
    run(f->body);
    in_function--;
    float ret = g_return.active ? g_return.value : 0.0f;
    g_return.active = 0;
    pop_frame(); g_call_depth--;
    return ret;
}

/* ═══════════════════════════════════════════════════════════════
   Évaluation d'expressions
   ═══════════════════════════════════════════════════════════════ */
static float eval(Node *n) {
    if (!n) return 0;
    switch (n->type) {
        case NODE_NUM:       return n->fval;
        case NODE_VAR:       return get_var(n->sval);
        case NODE_CALL_EXPR: return call_func(n->sval, n->args);

        /* ── Lecture tableau : tab[index] ── */
        case NODE_TAB_READ:
            return get_array(n->sval, (int)eval(n->left));

        case NODE_BINOP: {
            float l = eval(n->left), r = eval(n->right);
            switch (n->op) {
                case '+': return l + r;
                case '-': return l - r;
                case '*': return l * r;
                case '/': if (r==0){ERR("division par zero");exit(1);} return l/r;
                case '%': return (float)((int)l % (int)r);
                case 'E': return fabsf(l-r) < FLOAT_EPSILON ? 1.f : 0.f;
                case 'N': return fabsf(l-r) >= FLOAT_EPSILON ? 1.f : 0.f;
                case '>': return l >  r ? 1.f : 0.f;
                case '<': return l <  r ? 1.f : 0.f;
                case 'G': return l >= r ? 1.f : 0.f;
                case 'L': return l <= r ? 1.f : 0.f;
                case '&': return (l && r) ? 1.f : 0.f;
                case '|': return (l || r) ? 1.f : 0.f;
            }
            return 0;
        }
        case NODE_UNOP: {
            float v = eval(n->left);
            if (n->op == '!') return !v ? 1.f : 0.f;
            if (n->op == 'M') return -v;
            return v;
        }
        default: return 0;
    }
}

/* ═══════════════════════════════════════════════════════════════
   Exécution d'instructions
   ═══════════════════════════════════════════════════════════════ */
static void run(Node *n) {
    if (!n || g_return.active) return;
    switch (n->type) {
        case NODE_NOP: break;
        case NODE_SEQ: run(n->left); run(n->right); break;

        case NODE_AVANCER:     svg_avancer(eval(n->left));    break;
        case NODE_RECULER:     svg_reculer(eval(n->left));    break;
        case NODE_TOURNER:     svg_tourner(eval(n->left));    break;
        case NODE_ORIENTATION: svg_orientation(eval(n->left)); break;  /* FIX: appel correct */
        case NODE_COULEUR:     svg_set_color(n->sval);        break;
        case NODE_EPAISSEUR:   svg_set_width(eval(n->left));  break;
        case NODE_STYLO_HAUT:  svg_pen_up();                  break;
        case NODE_STYLO_BAS:   svg_pen_down();                break;

        case NODE_AFFICHER_EXPR:
            printf(ANSI_MAGENTA "► " ANSI_RESET "%.6g\n", eval(n->left)); break;
        case NODE_AFFICHER_STR:
            printf(ANSI_MAGENTA "► " ANSI_RESET "%s\n", n->sval); break;

        case NODE_ASSIGN:
            set_var(n->sval, eval(n->left)); break;

        /* ── Déclaration tableau : tab nom[taille] ── */
        case NODE_TAB_DECL:
            decl_array(n->sval, n->ival); break;

        /* ── Affectation tableau : nom[index] = expr ── */
        /* FIX: eval() n'est plus appelé deux fois — on stocke idx et val */
        case NODE_TAB_ASSIGN: {
            int   idx = (int)eval(n->left);
            float val = eval(n->right);
            set_array(n->sval, idx, val);
            INFO("tableau '%s[%d]' = %.6g", n->sval, idx, val);
            break;
        }

        case NODE_REPETER: {
            int times = (int)eval(n->left);
            if (times < 0) { WARN("repeter: valeur negative, ignore"); break; }
            if (times > MAX_LOOP_ITER) { WARN("repeter: limite atteinte"); times = MAX_LOOP_ITER; }
            for (int i = 0; i < times && !g_return.active; i++) run(n->right);
            break;
        }
        case NODE_TANTQUE: {
            int iter = 0;
            while (!g_return.active && eval(n->left)) {
                run(n->right);
                if (++iter >= MAX_LOOP_ITER) { WARN("tantque: boucle infinie, arret"); break; }
            }
            break;
        }
        case NODE_SI:       if (eval(n->left)) run(n->right); break;
        case NODE_SI_SINON: if (eval(n->left)) run(n->right); else run(n->extra); break;
        case NODE_FUNC_DEF: register_func(n->sval, n->params, n->param_count, n->left); break;
        case NODE_FUNC_CALL: call_func(n->sval, n->args); break;
        case NODE_RETOURNER:
            if (!in_function) { ERR("'retourner' hors d'une fonction"); exit(1); }
            g_return.value = eval(n->left); g_return.active = 1; break;
        default: break;
    }
}

void run_program(Node *root) {
    if (!root) {
        fprintf(stderr, "[AVERT] Programme vide ou AST non construit.\n");
        return;
    }
    run(root);
}