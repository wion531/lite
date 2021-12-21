#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include "glrenderer.h"
#include "renderer.h"

#ifdef _MSC_VER
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#endif

/* a hardware-accelerated renderer using opengl */

#define BUFFER_SIZE 16384

static GLfloat   tex_buf[BUFFER_SIZE *  8];
static GLfloat  vert_buf[BUFFER_SIZE *  8];
static GLubyte color_buf[BUFFER_SIZE * 16];
static GLuint  index_buf[BUFFER_SIZE *  6];

static int buf_idx;
static RenFont *last_font;
static GlyphSet *last_glyphset;

static SDL_Window *window;

void glren_init(SDL_Window *win) {
  window = win;
  SDL_GL_CreateContext(win);

  /* init gl */
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);
  glEnable(GL_TEXTURE_2D);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
}


int glren_make_font_texture(GlyphSet *glyphset) {
  GLuint id;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glyphset->image->width,
    glyphset->image->height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
    glyphset->image->pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  assert(glGetError() == 0);
  last_glyphset = glyphset;
  return id;
}


static void flush(void) {
  if (buf_idx == 0) { return; }
  int width, height;
  ren_get_size(&width, &height);

  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0.0f, width, height, 0.0f, -1.0f, +1.0f);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glTexCoordPointer(2, GL_FLOAT, 0, tex_buf);
  glVertexPointer(2, GL_FLOAT, 0, vert_buf);
  glColorPointer(4, GL_UNSIGNED_BYTE, 0, color_buf);
  glDrawElements(GL_TRIANGLES, buf_idx * 6, GL_UNSIGNED_INT, index_buf);

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  buf_idx = 0;
}


void glren_set_clip_rect(RenRect rect) {
  flush();
  int width, height;
  ren_get_size(&width, &height);
  glScissor(rect.x, height - (rect.y + rect.height), rect.width, rect.height);
}


void glren_free_font(RenFont *font) {
  // we don't do anything lmao
}


void glren_draw_rect(RenRect rect, RenColor color) {
  if (buf_idx == BUFFER_SIZE) { flush(); }

  int texvert_idx = buf_idx *  8;
  int   color_idx = buf_idx * 16;
  int element_idx = buf_idx *  4;
  int   index_idx = buf_idx *  6;
  buf_idx++;

  /* update texture buffer */
  tex_buf[texvert_idx + 0] = 0;
  tex_buf[texvert_idx + 1] = 0;
  tex_buf[texvert_idx + 2] = 1.0f / last_glyphset->image->width;
  tex_buf[texvert_idx + 3] = 0;
  tex_buf[texvert_idx + 4] = 0;
  tex_buf[texvert_idx + 5] = 1.0f / last_glyphset->image->height;
  tex_buf[texvert_idx + 6] = 1.0f / last_glyphset->image->width;
  tex_buf[texvert_idx + 7] = 1.0f / last_glyphset->image->height;

  /* update vertex buffer */
  vert_buf[texvert_idx + 0] = rect.x;
  vert_buf[texvert_idx + 1] = rect.y;
  vert_buf[texvert_idx + 2] = rect.x + rect.width;
  vert_buf[texvert_idx + 3] = rect.y;
  vert_buf[texvert_idx + 4] = rect.x;
  vert_buf[texvert_idx + 5] = rect.y + rect.height;
  vert_buf[texvert_idx + 6] = rect.x + rect.width;
  vert_buf[texvert_idx + 7] = rect.y + rect.height;

  /* update color buffer */
  for (int i = 0; i < 4; i++) {
    color_buf[color_idx + (i * 4) + 0] = color.r;
    color_buf[color_idx + (i * 4) + 1] = color.g;
    color_buf[color_idx + (i * 4) + 2] = color.b;
    color_buf[color_idx + (i * 4) + 3] = color.a;
  }

  /* update index buffer */
  index_buf[index_idx + 0] = element_idx + 0;
  index_buf[index_idx + 1] = element_idx + 1;
  index_buf[index_idx + 2] = element_idx + 2;
  index_buf[index_idx + 3] = element_idx + 2;
  index_buf[index_idx + 4] = element_idx + 3;
  index_buf[index_idx + 5] = element_idx + 1;
}


static void* check_alloc(void *ptr) {
  if (!ptr) {
    fprintf(stderr, "Fatal error: memory allocation failed\n");
    exit(EXIT_FAILURE);
  }
  return ptr;
}


static const char* utf8_to_codepoint(const char *p, unsigned *dst) {
  unsigned res, n;
  switch (*p & 0xf0) {
    case 0xf0 :  res = *p & 0x07;  n = 3;  break;
    case 0xe0 :  res = *p & 0x0f;  n = 2;  break;
    case 0xd0 :
    case 0xc0 :  res = *p & 0x1f;  n = 1;  break;
    default   :  res = *p;         n = 0;  break;
  }
  while (n--) {
    res = (res << 6) | (*(++p) & 0x3f);
  }
  *dst = res;
  return p + 1;
}


static GlyphSet* load_glyphset(RenFont *font, int idx) {
  GlyphSet *set = check_alloc(calloc(1, sizeof(GlyphSet)));

  /* init image */
  int width = 128;
  int height = 128;
retry:
  set->image = ren_new_image(width, height);

  /* load glyphs */
  float s =
    stbtt_ScaleForMappingEmToPixels(&font->stbfont, 1) /
    stbtt_ScaleForPixelHeight(&font->stbfont, 1);
  int res = stbtt_BakeFontBitmap(
    font->data, 0, font->size * s, (void*) set->image->pixels,
    width, height, idx * 256, 256, set->glyphs);

  /* retry with a larger image buffer if the buffer wasn't large enough */
  if (res < 0) {
    width *= 2;
    height *= 2;
    ren_free_image(set->image);
    goto retry;
  }

  /* adjust glyph yoffsets and xadvance */
  int ascent, descent, linegap;
  stbtt_GetFontVMetrics(&font->stbfont, &ascent, &descent, &linegap);
  float scale = stbtt_ScaleForMappingEmToPixels(&font->stbfont, font->size);
  int scaled_ascent = ascent * scale + 0.5;
  for (int i = 0; i < 256; i++) {
    set->glyphs[i].yoff += scaled_ascent;
    set->glyphs[i].xadvance = floor(set->glyphs[i].xadvance);
  }

  /* convert 8bit data to 32bit */
  for (int i = width * height - 1; i >= 0; i--) {
    uint8_t n = *((uint8_t*) set->image->pixels + i);
    set->image->pixels[i] = (RenColor) { .r = 255, .g = 255, .b = 255, .a = n };
  }
  set->image->pixels[0] = (RenColor) { 255, 255, 255, 255 };

  /* make opengl texture */
#if RENDER_OPENGL
  set->gltex = glren_make_font_texture(set);
#endif

  return set;
}


static GlyphSet* get_glyphset(RenFont *font, int codepoint) {
  int idx = (codepoint >> 8) % MAX_GLYPHSET;
  if (!font->sets[idx]) {
    font->sets[idx] = load_glyphset(font, idx);
  }
  return font->sets[idx];
}


int glren_draw_text(RenFont *font, const char *text, int x, int y, RenColor color) {
  RenRect rect;
  const char *p = text;
  unsigned codepoint;
  while (*p) {
    p = utf8_to_codepoint(p, &codepoint);
    GlyphSet *set = get_glyphset(font, codepoint);
    stbtt_bakedchar *g = &set->glyphs[codepoint & 0xff];
    rect.x = g->x0;
    rect.y = g->y0;
    rect.width = g->x1 - g->x0;
    rect.height = g->y1 - g->y0;

    /* build buffers */
    if (buf_idx == BUFFER_SIZE) { flush(); }
    if (last_font != font || last_glyphset != set) {
      flush();
      last_font = font;
      last_glyphset = set;
      glBindTexture(GL_TEXTURE_2D, set->gltex);
    }

    int texvert_idx = buf_idx *  8;
    int   color_idx = buf_idx * 16;
    int element_idx = buf_idx *  4;
    int   index_idx = buf_idx *  6;
    buf_idx++;

    /* update texture buffer */
    tex_buf[texvert_idx + 0] = (float)(rect.x) / (float)set->image->width;
    tex_buf[texvert_idx + 1] = (float)(rect.y) / (float)set->image->height;
    tex_buf[texvert_idx + 2] = (float)(rect.x + rect.width) / (float)set->image->width;
    tex_buf[texvert_idx + 3] = (float)(rect.y) / (float)set->image->height;
    tex_buf[texvert_idx + 4] = (float)(rect.x) / (float)set->image->width;
    tex_buf[texvert_idx + 5] = (float)(rect.y + rect.height) / (float)set->image->height;
    tex_buf[texvert_idx + 6] = (float)(rect.x + rect.width) / (float)set->image->width;
    tex_buf[texvert_idx + 7] = (float)(rect.y + rect.height) / (float)set->image->height;

    /* update vertex buffer */
    vert_buf[texvert_idx + 0] = x + g->xoff;
    vert_buf[texvert_idx + 1] = y + g->yoff;
    vert_buf[texvert_idx + 2] = x + g->xoff + rect.width;
    vert_buf[texvert_idx + 3] = y + g->yoff;
    vert_buf[texvert_idx + 4] = x + g->xoff;
    vert_buf[texvert_idx + 5] = y + g->yoff + rect.height;
    vert_buf[texvert_idx + 6] = x + g->xoff + rect.width;
    vert_buf[texvert_idx + 7] = y + g->yoff + rect.height;

    /* update color buffer */
    for (int i = 0; i < 4; i++) {
      color_buf[color_idx + (i * 4) + 0] = color.r;
      color_buf[color_idx + (i * 4) + 1] = color.g;
      color_buf[color_idx + (i * 4) + 2] = color.b;
      color_buf[color_idx + (i * 4) + 3] = color.a;
    }

    /* update index buffer */
    index_buf[index_idx + 0] = element_idx + 0;
    index_buf[index_idx + 1] = element_idx + 1;
    index_buf[index_idx + 2] = element_idx + 2;
    index_buf[index_idx + 3] = element_idx + 2;
    index_buf[index_idx + 4] = element_idx + 3;
    index_buf[index_idx + 5] = element_idx + 1;

    x += g->xadvance;

    last_font = font;
    last_glyphset = set;
  }

  return x;
}


void glren_begin_frame(void) {
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}


void glren_end_frame(void) {
  flush();
  SDL_GL_SwapWindow(window);
}
