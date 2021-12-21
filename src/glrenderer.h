#ifndef GLRENDERER_H
#define GLRENDERER_H

#include "renderer.h"

void glren_init(SDL_Window *win);
void glren_set_clip_rect(RenRect rect);
int  glren_make_font_texture(GlyphSet *glyphset);
void glren_free_font(RenFont *font);
void glren_draw_rect(RenRect rect, RenColor color);
int  glren_draw_text(RenFont *font, const char *text, int x, int y, RenColor color);
void glren_begin_frame(void);
void glren_end_frame(void);

#endif
