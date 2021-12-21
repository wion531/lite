#include "api.h"
#include "renderer.h"
#include "rencache.h"
#include "glrenderer.h"


static RenColor checkcolor(lua_State *L, int idx, int def) {
  RenColor color;
  if (lua_isnoneornil(L, idx)) {
    return (RenColor) { def, def, def, 255 };
  }
  lua_rawgeti(L, idx, 1);
  lua_rawgeti(L, idx, 2);
  lua_rawgeti(L, idx, 3);
  lua_rawgeti(L, idx, 4);
  color.r = luaL_checknumber(L, -4);
  color.g = luaL_checknumber(L, -3);
  color.b = luaL_checknumber(L, -2);
  color.a = luaL_optnumber(L, -1, 255);
  lua_pop(L, 4);
  return color;
}


static int f_show_debug(lua_State *L) {
  luaL_checkany(L, 1);
#ifdef RENDER_SOFT
  rencache_show_debug(lua_toboolean(L, 1));
#endif
  return 0;
}


static int f_get_size(lua_State *L) {
  int w, h;
  ren_get_size(&w, &h);
  lua_pushnumber(L, w);
  lua_pushnumber(L, h);
  return 2;
}


static int f_begin_frame(lua_State *L) {
#if RENDER_SOFT
  rencache_begin_frame();
#else
  glren_begin_frame();
#endif
  return 0;
}


static int f_end_frame(lua_State *L) {
#if RENDER_SOFT
  rencache_end_frame();
#else
  glren_end_frame();
#endif
  return 0;
}


static int f_set_clip_rect(lua_State *L) {
  RenRect rect;
  rect.x = luaL_checknumber(L, 1);
  rect.y = luaL_checknumber(L, 2);
  rect.width = luaL_checknumber(L, 3);
  rect.height = luaL_checknumber(L, 4);
#if RENDER_SOFT
  rencache_set_clip_rect(rect);
#else
  glren_set_clip_rect(rect);
#endif
  return 0;
}


static int f_draw_rect(lua_State *L) {
  RenRect rect;
  rect.x = luaL_checknumber(L, 1);
  rect.y = luaL_checknumber(L, 2);
  rect.width = luaL_checknumber(L, 3);
  rect.height = luaL_checknumber(L, 4);
  RenColor color = checkcolor(L, 5, 255);
#if RENDER_SOFT
  rencache_draw_rect(rect, color);
#else
  glren_draw_rect(rect, color);
#endif
  return 0;
}


static int f_draw_text(lua_State *L) {
  RenFont **font = luaL_checkudata(L, 1, API_TYPE_FONT);
  const char *text = luaL_checkstring(L, 2);
  int x = luaL_checknumber(L, 3);
  int y = luaL_checknumber(L, 4);
  RenColor color = checkcolor(L, 5, 255);
#if RENDER_SOFT
  x = rencache_draw_text(*font, text, x, y, color);
#else
  x = glren_draw_text(*font, text, x, y, color);
#endif
  lua_pushnumber(L, x);
  return 1;
}


static const luaL_Reg lib[] = {
  { "show_debug",    f_show_debug    },
  { "get_size",      f_get_size      },
  { "begin_frame",   f_begin_frame   },
  { "end_frame",     f_end_frame     },
  { "set_clip_rect", f_set_clip_rect },
  { "draw_rect",     f_draw_rect     },
  { "draw_text",     f_draw_text     },
  { NULL,            NULL            }
};


int luaopen_renderer_font(lua_State *L);

int luaopen_renderer(lua_State *L) {
  luaL_newlib(L, lib);
  luaopen_renderer_font(L);
  lua_setfield(L, -2, "font");
  return 1;
}
