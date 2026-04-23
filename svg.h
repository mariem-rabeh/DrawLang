#ifndef SVG_H
#define SVG_H

void svg_init(const char *filename, int width, int height);
void svg_finish(void);

void svg_avancer(float distance);
void svg_reculer(float distance);
void svg_tourner(float angle_deg);
void svg_set_angle(float angle_deg);   /* angle absolu en degrés (0=droite, -90=haut) */
void svg_orientation(float angle_deg); /* alias de svg_set_angle pour l'AST */

void svg_set_color(const char *color);
void svg_set_width(float w);
void svg_pen_up(void);
void svg_pen_down(void);

void svg_print_state(void);

#endif