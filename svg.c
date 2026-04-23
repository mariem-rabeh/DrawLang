#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "svg.h"

static FILE  *svg_file   = NULL;
static float  turtle_x   = 0.0f;
static float  turtle_y   = 0.0f;
static float  turtle_ang = -90.0f;
static int    pen_down   = 1;
static char   pen_color[32] = "#2196F3";
static float  pen_width  = 2.0f;
static int    svg_width  = 800;
static int    svg_height = 600;

#define PI 3.14159265358979f
#define DEG2RAD(d) ((d) * PI / 180.0f)

void svg_init(const char *filename, int width, int height) {
    svg_width  = width;
    svg_height = height;
    turtle_x   = width  / 2.0f;
    turtle_y   = height / 2.0f;
    turtle_ang = -90.0f;
    svg_file = fopen(filename, "w");
    if (!svg_file) { fprintf(stderr, "SVG: impossible de créer '%s'\n", filename); exit(1); }
    fprintf(svg_file,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<svg xmlns=\"http://www.w3.org/2000/svg\"\n"
        "     width=\"%d\" height=\"%d\">\n"          /* ← supprime le style= */
        "  <!-- Généré par DrawLang -->\n"
        "  <rect width=\"%d\" height=\"%d\" fill=\"#f5f4f0\"/>\n"  /* ← fond garanti */
        "  <g id=\"turtle-drawing\">\n",
        width, height,
        width, height);                               /* ← 4 arguments maintenant */
    printf("[SVG] Fichier '%s' ouvert (%dx%d).\n", filename, width, height);
}

void svg_finish(void) {
    if (!svg_file) return;
    fprintf(svg_file, "  </g>\n</svg>\n");
    fclose(svg_file);
    svg_file = NULL;
    printf("[SVG] Fichier fermé.\n");
}

void svg_avancer(float distance) {
    if (!svg_file) return;
    float rad = DEG2RAD(turtle_ang);
    float nx  = turtle_x + distance * cosf(rad);
    float ny  = turtle_y + distance * sinf(rad);
    if (pen_down)
        fprintf(svg_file,
            "    <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\"\n"
            "          stroke=\"%s\" stroke-width=\"%.1f\" stroke-linecap=\"round\"/>\n",
            turtle_x, turtle_y, nx, ny, pen_color, pen_width);
    turtle_x = nx;
    turtle_y = ny;
}

void svg_reculer(float distance) { svg_avancer(-distance); }

void svg_tourner(float angle_deg) {
    turtle_ang += angle_deg;
    while (turtle_ang >  360.0f) turtle_ang -= 360.0f;
    while (turtle_ang < -360.0f) turtle_ang += 360.0f;
}

/* Fixe l'angle directement (sans accumulation) */
void svg_set_angle(float angle_deg) {
    turtle_ang = angle_deg;
    while (turtle_ang >  360.0f) turtle_ang -= 360.0f;
    while (turtle_ang < -360.0f) turtle_ang += 360.0f;
    printf("[SVG] Angle fixé à %.1f°\n", turtle_ang);
}

/* Alias exposé à l'AST via NODE_ORIENTATION */
void svg_orientation(float angle_deg) {
    svg_set_angle(angle_deg);
}

void svg_set_color(const char *color) {
    strncpy(pen_color, color, sizeof(pen_color) - 1);
    pen_color[sizeof(pen_color) - 1] = '\0';
}

void svg_set_width(float w)  { pen_width = w; }
void svg_pen_up(void)        { pen_down = 0; }
void svg_pen_down(void)      { pen_down = 1; }

void svg_print_state(void) {
    printf("[SVG] pos=(%.1f,%.1f) angle=%.1f° stylo=%s\n",
           turtle_x, turtle_y, turtle_ang, pen_down ? "bas" : "haut");
}