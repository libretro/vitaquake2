/*
Copyright (C) 2017 Felipe Izzo
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifdef HAVE_OPENGL
#include <glsym/rglgen_private_headers.h>
#include <glsym/glsym.h>
#endif

#include <ctype.h>
#include <libretro.h>
#include <libretro_file.h>
#include <retro_dirent.h>
#include <string/stdstring.h>
#include <features/features_cpu.h>
#include <array/rhmap.h>

#include <libretro_core_options.h>

#include "../client/client.h"
#include "../client/qmenu.h"
#include "../client/cdaudio.h"

#ifdef HAVE_OPENGL
#include "../ref_gl/gl_local.h"

#include <glsm/glsm.h>
#endif

static bool first_boot = true;
static qboolean gl_set = false;
bool is_soft_render = false;

unsigned	sys_frame_time;
uint64_t rumble_tick;
void *tex_buffer = NULL;

bool cdaudio_enabled = true;
float cdaudio_volume = 0.5f;

float libretro_gamma = 1.0f;
float libretro_hud_scale = 0.5f;
#ifdef HAVE_OPENGL
extern cvar_t *gl_shadows;
static bool libretro_shared_context = false;
static bool enable_opengl = true;
float libretro_gl_modulate = 4.0f;
#else
static const bool enable_opengl = false;
#endif
extern cvar_t *sw_texfilt;

refimport_t ri;

extern void M_PopMenu( void );
extern void M_ForceMenuOff( void );
extern int m_menudepth;

/* TODO/FIXME - should become float for better accuracy */
int      framerate    = 60;
unsigned framerate_ms = 16;

float *gVertexBuffer;
float *gColorBuffer;
float *gTexCoordBuffer;

#ifdef HAVE_OPENGL
void ( APIENTRY * qglBlendFunc )(GLenum sfactor, GLenum dfactor);
void ( APIENTRY * qglTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglTexParameteri )(GLenum target, GLenum pname, GLint param);
void ( APIENTRY * qglBindFramebuffer )(GLenum target, GLuint framebuffer);
void ( APIENTRY * qglGenerateMipmap )(GLenum target);
void ( APIENTRY * qglTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglDepthMask )(GLboolean flag);
void ( APIENTRY * qglPushMatrix )(void);
void ( APIENTRY * qglRotatef )(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglTranslatef )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglDepthRange )(GLclampd zNear, GLclampd zFar);
void ( APIENTRY * qglClear )(GLbitfield mask);
void ( APIENTRY * qglEnable )(GLenum cap);
void ( APIENTRY * qglDisable )(GLenum cap);
void ( APIENTRY * qglPopMatrix )(void);
void ( APIENTRY * qglGetFloatv )(GLenum pname, GLfloat *params);
void ( APIENTRY * qglOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void ( APIENTRY * qglFrustum )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void ( APIENTRY * qglLoadMatrixf )(const GLfloat *m);
void ( APIENTRY * qglLoadIdentity )(void);
void ( APIENTRY * qglMatrixMode )(GLenum mode);
void ( APIENTRY * qglBindTexture )(GLenum target, GLuint texture);
void ( APIENTRY * qglReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void ( APIENTRY * qglPolygonMode )(GLenum face, GLenum mode);
void ( APIENTRY * qglVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void ( APIENTRY * qglClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void ( APIENTRY * qglCullFace )(GLenum mode);
void ( APIENTRY * qglViewport )(GLint x, GLint y, GLsizei width, GLsizei height);
void ( APIENTRY * qglDeleteTextures )(GLsizei n, const GLuint *textures);
void ( APIENTRY * qglClearStencil )(GLint s);
void ( APIENTRY * qglColor4f )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void ( APIENTRY * qglScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
void ( APIENTRY * qglEnableClientState )(GLenum array);
void ( APIENTRY * qglDisableClientState )(GLenum array);
void ( APIENTRY * qglStencilFunc )(GLenum func, GLint ref, GLuint mask);
void ( APIENTRY * qglStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
void ( APIENTRY * qglScalef )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglDepthFunc )(GLenum func);
void ( APIENTRY * qglTexEnvi )(GLenum target, GLenum pname, GLint param);
void ( APIENTRY * qglAlphaFunc )(GLenum func,  GLclampf ref);

#define GL_FUNCS_NUM 44

typedef struct api_entry{
	void *ptr;
} api_entry;

api_entry funcs[GL_FUNCS_NUM];
#endif

char g_rom_dir[1024] = {0};
char g_save_dir[1024] = {0};
char g_music_dir[1024] = {0};

#ifdef HAVE_OPENGL
extern struct retro_hw_render_callback hw_render;

#define MAX_INDICES 4096
uint16_t* indices = NULL;

float *gVertexBufferPtr = NULL;
float *gColorBufferPtr = NULL;
float *gTexCoordBufferPtr = NULL;
#endif

static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
retro_environment_t environ_cb;
static retro_input_poll_t poll_cb;
static retro_input_state_t input_cb;
static struct retro_rumble_interface rumble;
static bool libretro_supports_bitmasks = false;

static void audio_callback(void);

static unsigned quake_input_device = RETRO_DEVICE_JOYPAD;

/* System analog stick range is -0x8000 to 0x8000 */
#define ANALOG_RANGE 0x8000
/* Default deadzone: 15% */
static int analog_deadzone = (int)(0.15f * ANALOG_RANGE);

typedef struct {
	char *keyname;
	int keynum;
	char *command;
} input_bind_t;

static struct retro_input_descriptor input_desc[] = {
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Open Inventory" },
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Menu Up" },
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Menu Down" },
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Use Inventory Item" },
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Menu Cancel" },
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Menu Select" },
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Next Weapon" },
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Run" },
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Crouch / Descend" },
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Jump / Climb" },
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Attack" },
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,     "Drop Inventory Item" },
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Show / Hide Help Computer" },
	{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Show Menu" },
	{ 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,  RETRO_DEVICE_ID_ANALOG_X, "Strafe" },
	{ 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,  RETRO_DEVICE_ID_ANALOG_Y, "Move" },
	{ 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Horizontal Turn" },
	{ 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Vertical Look" },
	{ 0 },
};

/* Note: K_AUX1 and K_AUX4 are reserved for use
 * in the menu - but we still have to bind them
 * to *something*, otherwise the keys will be
 * ignored (great design there...) */
static const input_bind_t input_binds_pad[] = {
	[RETRO_DEVICE_ID_JOYPAD_B]      = { "AUX4",       K_AUX4,       "" }, /* Menu Cancel */
	[RETRO_DEVICE_ID_JOYPAD_Y]      = { "AUX2",       K_AUX2,       "weapnext" },
	[RETRO_DEVICE_ID_JOYPAD_SELECT] = { "SELECT",     K_ENTER,      "help" },
	[RETRO_DEVICE_ID_JOYPAD_START]  = { "START",      K_ESCAPE,     "" }, /* Menu Show */
	[RETRO_DEVICE_ID_JOYPAD_UP]     = { "UPARROW",    K_UPARROW,    "invprev" },
	[RETRO_DEVICE_ID_JOYPAD_DOWN]   = { "DOWNARROW",  K_DOWNARROW,  "invnext" },
	[RETRO_DEVICE_ID_JOYPAD_LEFT]   = { "LEFTARROW",  K_LEFTARROW,  "inven" },
	[RETRO_DEVICE_ID_JOYPAD_RIGHT]  = { "RIGHTARROW", K_RIGHTARROW, "invuse" },
	[RETRO_DEVICE_ID_JOYPAD_A]      = { "AUX1",       K_AUX1,       "" }, /* Menu Select */
	[RETRO_DEVICE_ID_JOYPAD_X]      = { 0 },
	[RETRO_DEVICE_ID_JOYPAD_L]      = { "AUX3",       K_AUX3,       "+speed" },
	[RETRO_DEVICE_ID_JOYPAD_R]      = { "AUX5",       K_AUX5,       "+movedown" },
	[RETRO_DEVICE_ID_JOYPAD_L2]     = { "AUX6",       K_AUX6,       "+moveup" },
	[RETRO_DEVICE_ID_JOYPAD_R2]     = { "AUX7",       K_AUX7,       "+attack" },
	[RETRO_DEVICE_ID_JOYPAD_L3]     = { 0 },
	[RETRO_DEVICE_ID_JOYPAD_R3]     = { "AUX8",       K_AUX8,       "invdrop" },
};
#define INPUT_BINDS_PAD_LEN (sizeof(input_binds_pad) / sizeof(input_binds_pad[0]))

enum input_kb_key_type
{
   KB_KEY_UP = 0,
   KB_KEY_DOWN,
   KB_KEY_LEFT,
   KB_KEY_RIGHT,
   KB_KEY_MENU_SHOW,
   KB_KEY_MENU_SELECT,
   KB_KEY_MENU_CANCEL,
   KB_KEY_HELP,
   KB_KEY_INVENTORY_SHOW,
   KB_KEY_INVENTORY_PREV,
   KB_KEY_INVENTORY_NEXT,
   KB_KEY_INVENTORY_USE,
   KB_KEY_INVENTORY_DROP,
   KB_KEY_WEAPON_NEXT,
   KB_KEY_RUN,
   KB_KEY_JUMP,
   KB_KEY_CROUCH,
   KB_KEY_LAST
};

static unsigned input_kb_map[KB_KEY_LAST] = {0};

typedef struct {
	char *key;
   enum input_kb_key_type type;
   unsigned default_id;
} input_kb_map_option_t;

static const input_kb_map_option_t input_kb_map_options[] = {
   { "vitaquakeii_kb_map_up",             KB_KEY_UP,             RETROK_w },
   { "vitaquakeii_kb_map_down",           KB_KEY_DOWN,           RETROK_s },
   { "vitaquakeii_kb_map_left",           KB_KEY_LEFT,           RETROK_a },
   { "vitaquakeii_kb_map_right",          KB_KEY_RIGHT,          RETROK_d },
   { "vitaquakeii_kb_map_menu_show",      KB_KEY_MENU_SHOW,      RETROK_ESCAPE },
   { "vitaquakeii_kb_map_menu_select",    KB_KEY_MENU_SELECT,    RETROK_RETURN },
   { "vitaquakeii_kb_map_menu_cancel",    KB_KEY_MENU_CANCEL,    RETROK_BACKSPACE },
   { "vitaquakeii_kb_map_menu_help",      KB_KEY_HELP,           RETROK_F1 },
   { "vitaquakeii_kb_map_inventory_show", KB_KEY_INVENTORY_SHOW, RETROK_TAB },
   { "vitaquakeii_kb_map_inventory_prev", KB_KEY_INVENTORY_PREV, RETROK_q },
   { "vitaquakeii_kb_map_inventory_next", KB_KEY_INVENTORY_NEXT, RETROK_e },
   { "vitaquakeii_kb_map_inventory_use",  KB_KEY_INVENTORY_USE,  RETROK_r },
   { "vitaquakeii_kb_map_inventory_drop", KB_KEY_INVENTORY_DROP, RETROK_v },
   { "vitaquakeii_kb_map_weapon_next",    KB_KEY_WEAPON_NEXT,    RETROK_f },
   { "vitaquakeii_kb_map_run",            KB_KEY_RUN,            RETROK_LSHIFT },
   { "vitaquakeii_kb_map_jump",           KB_KEY_JUMP,           RETROK_SPACE },
   { "vitaquakeii_kb_map_crouch",         KB_KEY_CROUCH,         RETROK_LCTRL },
};
#define INPUT_KB_MAP_OPTIONS_LEN (sizeof(input_kb_map_options) / sizeof(input_kb_map_options[0]))

typedef struct {
   unsigned id;
   char *id_str;
   char *name;
} input_kb_key_t;

#define INPUT_KB_KEYS_ENTRY(id, name) { id, #id, name }

static const input_kb_key_t input_kb_keys[] = {
   INPUT_KB_KEYS_ENTRY(RETROK_BACKSPACE,    "Backspace"),
   INPUT_KB_KEYS_ENTRY(RETROK_TAB,          "Tab"),
   INPUT_KB_KEYS_ENTRY(RETROK_CLEAR,        "Clear"),
   INPUT_KB_KEYS_ENTRY(RETROK_RETURN,       "Return"),
   INPUT_KB_KEYS_ENTRY(RETROK_PAUSE,        "Pause"),
   INPUT_KB_KEYS_ENTRY(RETROK_ESCAPE,       "Esc"),
   INPUT_KB_KEYS_ENTRY(RETROK_SPACE,        "Space"),
   INPUT_KB_KEYS_ENTRY(RETROK_HASH,         "#"),
   INPUT_KB_KEYS_ENTRY(RETROK_QUOTE,        "'"),
   INPUT_KB_KEYS_ENTRY(RETROK_COMMA,        ","),
   INPUT_KB_KEYS_ENTRY(RETROK_MINUS,        "-"),
   INPUT_KB_KEYS_ENTRY(RETROK_PERIOD,       "."),
   INPUT_KB_KEYS_ENTRY(RETROK_SLASH,        "/"),
   INPUT_KB_KEYS_ENTRY(RETROK_0,            "0"),
   INPUT_KB_KEYS_ENTRY(RETROK_1,            "1"),
   INPUT_KB_KEYS_ENTRY(RETROK_2,            "2"),
   INPUT_KB_KEYS_ENTRY(RETROK_3,            "3"),
   INPUT_KB_KEYS_ENTRY(RETROK_4,            "4"),
   INPUT_KB_KEYS_ENTRY(RETROK_5,            "5"),
   INPUT_KB_KEYS_ENTRY(RETROK_6,            "6"),
   INPUT_KB_KEYS_ENTRY(RETROK_7,            "7"),
   INPUT_KB_KEYS_ENTRY(RETROK_8,            "8"),
   INPUT_KB_KEYS_ENTRY(RETROK_9,            "9"),
   INPUT_KB_KEYS_ENTRY(RETROK_SEMICOLON,    ";"),
   INPUT_KB_KEYS_ENTRY(RETROK_LESS,         "<"),
   INPUT_KB_KEYS_ENTRY(RETROK_EQUALS,       "="),
   INPUT_KB_KEYS_ENTRY(RETROK_GREATER,      ">"),
   INPUT_KB_KEYS_ENTRY(RETROK_LEFTBRACKET,  "["),
   INPUT_KB_KEYS_ENTRY(RETROK_BACKSLASH,    "\\"),
   INPUT_KB_KEYS_ENTRY(RETROK_RIGHTBRACKET, "]"),
   INPUT_KB_KEYS_ENTRY(RETROK_BACKQUOTE,    "`"),
   INPUT_KB_KEYS_ENTRY(RETROK_a,            "a"),
   INPUT_KB_KEYS_ENTRY(RETROK_b,            "b"),
   INPUT_KB_KEYS_ENTRY(RETROK_c,            "c"),
   INPUT_KB_KEYS_ENTRY(RETROK_d,            "d"),
   INPUT_KB_KEYS_ENTRY(RETROK_e,            "e"),
   INPUT_KB_KEYS_ENTRY(RETROK_f,            "f"),
   INPUT_KB_KEYS_ENTRY(RETROK_g,            "g"),
   INPUT_KB_KEYS_ENTRY(RETROK_h,            "h"),
   INPUT_KB_KEYS_ENTRY(RETROK_i,            "i"),
   INPUT_KB_KEYS_ENTRY(RETROK_j,            "j"),
   INPUT_KB_KEYS_ENTRY(RETROK_k,            "k"),
   INPUT_KB_KEYS_ENTRY(RETROK_l,            "l"),
   INPUT_KB_KEYS_ENTRY(RETROK_m,            "m"),
   INPUT_KB_KEYS_ENTRY(RETROK_n,            "n"),
   INPUT_KB_KEYS_ENTRY(RETROK_o,            "o"),
   INPUT_KB_KEYS_ENTRY(RETROK_p,            "p"),
   INPUT_KB_KEYS_ENTRY(RETROK_q,            "q"),
   INPUT_KB_KEYS_ENTRY(RETROK_r,            "r"),
   INPUT_KB_KEYS_ENTRY(RETROK_s,            "s"),
   INPUT_KB_KEYS_ENTRY(RETROK_t,            "t"),
   INPUT_KB_KEYS_ENTRY(RETROK_u,            "u"),
   INPUT_KB_KEYS_ENTRY(RETROK_v,            "v"),
   INPUT_KB_KEYS_ENTRY(RETROK_w,            "w"),
   INPUT_KB_KEYS_ENTRY(RETROK_x,            "x"),
   INPUT_KB_KEYS_ENTRY(RETROK_y,            "y"),
   INPUT_KB_KEYS_ENTRY(RETROK_z,            "z"),
   INPUT_KB_KEYS_ENTRY(RETROK_DELETE,       "Delete"),
   INPUT_KB_KEYS_ENTRY(RETROK_KP0,          "Keypad 0"),
   INPUT_KB_KEYS_ENTRY(RETROK_KP1,          "Keypad 1"),
   INPUT_KB_KEYS_ENTRY(RETROK_KP2,          "Keypad 2"),
   INPUT_KB_KEYS_ENTRY(RETROK_KP3,          "Keypad 3"),
   INPUT_KB_KEYS_ENTRY(RETROK_KP4,          "Keypad 4"),
   INPUT_KB_KEYS_ENTRY(RETROK_KP5,          "Keypad 5"),
   INPUT_KB_KEYS_ENTRY(RETROK_KP6,          "Keypad 6"),
   INPUT_KB_KEYS_ENTRY(RETROK_KP7,          "Keypad 7"),
   INPUT_KB_KEYS_ENTRY(RETROK_KP8,          "Keypad 8"),
   INPUT_KB_KEYS_ENTRY(RETROK_KP9,          "Keypad 9"),
   INPUT_KB_KEYS_ENTRY(RETROK_KP_PERIOD,    "Keypad ."),
   INPUT_KB_KEYS_ENTRY(RETROK_KP_DIVIDE,    "Keypad /"),
   INPUT_KB_KEYS_ENTRY(RETROK_KP_MULTIPLY,  "Keypad *"),
   INPUT_KB_KEYS_ENTRY(RETROK_KP_MINUS,     "Keypad -"),
   INPUT_KB_KEYS_ENTRY(RETROK_KP_PLUS,      "Keypad +"),
   INPUT_KB_KEYS_ENTRY(RETROK_KP_ENTER,     "Keypad Enter"),
   INPUT_KB_KEYS_ENTRY(RETROK_KP_EQUALS,    "Keypad ="),
   INPUT_KB_KEYS_ENTRY(RETROK_UP,           "Cursor Up"),
   INPUT_KB_KEYS_ENTRY(RETROK_DOWN,         "Cursor Down"),
   INPUT_KB_KEYS_ENTRY(RETROK_RIGHT,        "Cursor Right"),
   INPUT_KB_KEYS_ENTRY(RETROK_LEFT,         "Cursor Left"),
   INPUT_KB_KEYS_ENTRY(RETROK_INSERT,       "Insert"),
   INPUT_KB_KEYS_ENTRY(RETROK_HOME,         "Home"),
   INPUT_KB_KEYS_ENTRY(RETROK_END,          "End"),
   INPUT_KB_KEYS_ENTRY(RETROK_PAGEUP,       "PgUp"),
   INPUT_KB_KEYS_ENTRY(RETROK_PAGEDOWN,     "PgDn"),
   INPUT_KB_KEYS_ENTRY(RETROK_F1,           "F1"),
   INPUT_KB_KEYS_ENTRY(RETROK_F2,           "F2"),
   INPUT_KB_KEYS_ENTRY(RETROK_F3,           "F3"),
   INPUT_KB_KEYS_ENTRY(RETROK_F4,           "F4"),
   INPUT_KB_KEYS_ENTRY(RETROK_F5,           "F5"),
   INPUT_KB_KEYS_ENTRY(RETROK_F6,           "F6"),
   INPUT_KB_KEYS_ENTRY(RETROK_F7,           "F7"),
   INPUT_KB_KEYS_ENTRY(RETROK_F8,           "F8"),
   INPUT_KB_KEYS_ENTRY(RETROK_F9,           "F9"),
   INPUT_KB_KEYS_ENTRY(RETROK_F10,          "F10"),
   INPUT_KB_KEYS_ENTRY(RETROK_F11,          "F11"),
   INPUT_KB_KEYS_ENTRY(RETROK_F12,          "F12"),
   INPUT_KB_KEYS_ENTRY(RETROK_F13,          "F13"),
   INPUT_KB_KEYS_ENTRY(RETROK_F14,          "F14"),
   INPUT_KB_KEYS_ENTRY(RETROK_F15,          "F15"),
   INPUT_KB_KEYS_ENTRY(RETROK_NUMLOCK,      "Num Lock"),
   INPUT_KB_KEYS_ENTRY(RETROK_CAPSLOCK,     "Caps Lock"),
   INPUT_KB_KEYS_ENTRY(RETROK_SCROLLOCK,    "Scroll Lock"),
   INPUT_KB_KEYS_ENTRY(RETROK_LSHIFT,       "Left Shift"),
   INPUT_KB_KEYS_ENTRY(RETROK_RSHIFT,       "Right Shift"),
   INPUT_KB_KEYS_ENTRY(RETROK_LCTRL,        "Left Ctrl"),
   INPUT_KB_KEYS_ENTRY(RETROK_RCTRL,        "Right Ctrl"),
   INPUT_KB_KEYS_ENTRY(RETROK_LALT,         "Left Alt"),
   INPUT_KB_KEYS_ENTRY(RETROK_RALT,         "Right Alt"),
   INPUT_KB_KEYS_ENTRY(RETROK_LMETA,        "Left Meta"),
   INPUT_KB_KEYS_ENTRY(RETROK_RMETA,        "Right Meta"),
   INPUT_KB_KEYS_ENTRY(RETROK_LSUPER,       "Left Super"),
   INPUT_KB_KEYS_ENTRY(RETROK_RSUPER,       "Right Super"),
};
#define INPUT_KB_KEYS_LEN (sizeof(input_kb_keys) / sizeof(input_kb_keys[0]))

static const input_kb_key_t **input_kb_keys_hash_map = NULL;

static const input_bind_t input_binds_kb[] = {
   [KB_KEY_UP]             = { "UPARROW",    K_UPARROW,    "+forward" },
   [KB_KEY_DOWN]           = { "DOWNARROW",  K_DOWNARROW,  "+back" },
   [KB_KEY_LEFT]           = { "LEFTARROW",  K_LEFTARROW,  "+moveleft" },
   [KB_KEY_RIGHT]          = { "RIGHTARROW", K_RIGHTARROW, "+moveright" },
   [KB_KEY_MENU_SHOW]      = { "START",      K_ESCAPE,     "" },
   [KB_KEY_MENU_SELECT]    = { "AUX1",       K_AUX1,       "" },
   [KB_KEY_MENU_CANCEL]    = { "AUX4",       K_AUX4,       "" },
   [KB_KEY_HELP]           = { "SELECT",     K_ENTER,      "help" },
   [KB_KEY_INVENTORY_SHOW] = { "AUX2",       K_AUX2,       "inven" },
   [KB_KEY_INVENTORY_PREV] = { "AUX3",       K_AUX3,       "invprev" },
   [KB_KEY_INVENTORY_NEXT] = { "AUX5",       K_AUX5,       "invnext" },
   [KB_KEY_INVENTORY_USE]  = { "AUX6",       K_AUX6,       "invuse" },
   [KB_KEY_INVENTORY_DROP] = { "AUX7",       K_AUX7,       "invdrop" },
   [KB_KEY_WEAPON_NEXT]    = { "AUX8",       K_AUX8,       "weapnext" },
   [KB_KEY_RUN]            = { "AUX9",       K_AUX9,       "+speed" },
   [KB_KEY_JUMP]           = { "AUX10",      K_AUX10,      "+moveup" },
   [KB_KEY_CROUCH]         = { "AUX11",      K_AUX11,      "+movedown" },
   /* Note: Attack is mapped to the left mouse button,
    * not the keyboard, but we still have to bind the
    * attack command and this is the most sensible
    * place to do it... */
   [KB_KEY_LAST]           = { "AUX12",      K_AUX12,      "+attack" },
};
#define INPUT_BINDS_KB_LEN (sizeof(input_binds_kb) / sizeof(input_binds_kb[0]))

static void initialise_kb_mapping_opts(void)
{
   size_t opt_idx;
   size_t opt_val_idx;

   /* Populate 'vitaquakeii_kb_map' entries
    * in libretro_core_options.h 'option_defs_us'
    * array */
   for (opt_idx = 0; opt_idx < INPUT_KB_MAP_OPTIONS_LEN; opt_idx++)
   {
      const char *key     = input_kb_map_options[opt_idx].key;
      unsigned default_id = input_kb_map_options[opt_idx].default_id;
      struct retro_core_option_v2_definition *opt_def;

      /* Find current option */
      for (opt_def = option_defs_us; opt_def->key; opt_def++)
      {
         if (!string_is_equal(opt_def->key, key))
            continue;

         /* We have a match
          * > Populate option values */
         for (opt_val_idx = 0; opt_val_idx < INPUT_KB_KEYS_LEN; opt_val_idx++)
         {
            opt_def->values[opt_val_idx].value = input_kb_keys[opt_val_idx].id_str;
            opt_def->values[opt_val_idx].label = input_kb_keys[opt_val_idx].name;

            /* Set default value */
            if (default_id == input_kb_keys[opt_val_idx].id)
               opt_def->default_value = input_kb_keys[opt_val_idx].id_str;
         }
         opt_def->values[INPUT_KB_KEYS_LEN].value = NULL;
         opt_def->values[INPUT_KB_KEYS_LEN].label = NULL;

         break;
      }
   }
}

static void update_kb_mapping(void)
{
   struct retro_variable var;
   size_t opt_idx;

   for (opt_idx = 0; opt_idx < INPUT_KB_MAP_OPTIONS_LEN; opt_idx++)
   {
      const char *key             = input_kb_map_options[opt_idx].key;
      enum input_kb_key_type type = input_kb_map_options[opt_idx].type;
      unsigned default_id         = input_kb_map_options[opt_idx].default_id;

      input_kb_map[type] = default_id;
      var.key            = key;
      var.value          = NULL;

      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      {
         const input_kb_key_t *input_kb_key = RHMAP_GET_STR(input_kb_keys_hash_map, var.value);
         if (input_kb_key)
            input_kb_map[type] = input_kb_key->id;
      }
   }
}

static void set_input_binds(void)
{
   const input_bind_t *input_binds;
   size_t input_binds_len;
   char buf[100];
   size_t i;

   if (quake_input_device == RETRO_DEVICE_KEYBOARD)
   {
      input_binds     = input_binds_kb;
      input_binds_len = INPUT_BINDS_KB_LEN;
   }
   else
   {
      input_binds     = input_binds_pad;
      input_binds_len = INPUT_BINDS_PAD_LEN;
   }

   Cbuf_AddText("unbindall\n");

   for (i = 0; i < input_binds_len; i++)
   {
      if (!input_binds[i].keyname)
         continue;

      snprintf(buf, sizeof(buf),
            "bind %s \"%s\"\n",
            input_binds[i].keyname,
            input_binds[i].command);

      Cbuf_AddText(buf);
   }
}

static bool context_needs_reinit = true;

#ifdef HAVE_OPENGL
void GL_DrawPolygon(GLenum prim, int num)
{
	qglDrawElements(prim, num, GL_UNSIGNED_SHORT, indices);
}

void glVertexAttribPointerMapped(int id, void* ptr)
{
   switch (id)
   {
      case 0: /* Vertex */
         qglVertexPointer(3, GL_FLOAT, 0, ptr);
         break;
      case 1: /* TexCoord */
         qglTexCoordPointer(2, GL_FLOAT, 0, ptr);
         break;
      case 2: /* Color */
         qglColorPointer(4, GL_FLOAT, 0, ptr);
         break;
   }
}

static bool initialize_gl()
{
	funcs[0].ptr  = qglTexImage2D         = (void (*)(GLenum,  GLint,  GLint,  GLsizei,  GLsizei,  GLint,  GLenum,  GLenum,  const GLvoid *))hw_render.get_proc_address ("glTexImage2D");
	funcs[1].ptr  = qglTexSubImage2D      = (void (*)(GLenum,  GLint,  GLint,  GLint,  GLsizei,  GLsizei,  GLenum,  GLenum,  const GLvoid *))hw_render.get_proc_address ("glTexSubImage2D");
	funcs[2].ptr  = qglTexParameteri      = (void (*)(GLenum,  GLenum,  GLint))hw_render.get_proc_address ("glTexParameteri");
	funcs[3].ptr  = qglBindFramebuffer    = (void (*)(GLenum,  GLuint))hw_render.get_proc_address ("glBindFramebuffer");
	funcs[4].ptr  = qglGenerateMipmap     = (void (*)(GLenum))hw_render.get_proc_address ("glGenerateMipmap");
	funcs[5].ptr  = qglBlendFunc          = (void (*)(GLenum,  GLenum))hw_render.get_proc_address ("glBlendFunc");
	funcs[6].ptr  = qglTexSubImage2D      = (void (*)(GLenum,  GLint,  GLint,  GLint,  GLsizei,  GLsizei,  GLenum,  GLenum,  const GLvoid *))hw_render.get_proc_address ("glTexSubImage2D");
	funcs[7].ptr  = qglDepthMask          = (void (*)(GLboolean))hw_render.get_proc_address ("glDepthMask");
	funcs[8].ptr  = qglPushMatrix         = hw_render.get_proc_address ("glPushMatrix");
	funcs[9].ptr  = qglRotatef            = (void (*)(GLfloat,  GLfloat,  GLfloat,  GLfloat))hw_render.get_proc_address ("glRotatef");
	funcs[10].ptr = qglTranslatef         = (void (*)(GLfloat,  GLfloat,  GLfloat))hw_render.get_proc_address ("glTranslatef");
	funcs[11].ptr = qglDepthRange         = (void (*)(GLdouble,  GLdouble))hw_render.get_proc_address ("glDepthRange");
	funcs[12].ptr = qglClear              = (void (*)(GLbitfield))hw_render.get_proc_address ("glClear");
	funcs[13].ptr = qglCullFace           = (void (*)(GLenum))hw_render.get_proc_address ("glCullFace");
	funcs[14].ptr = qglClearColor         = (void (*)(GLfloat,  GLfloat,  GLfloat,  GLfloat))hw_render.get_proc_address ("glClearColor");
	funcs[15].ptr = qglEnable             = (void (*)(GLenum))hw_render.get_proc_address ("glEnable");
	funcs[16].ptr = qglDisable            = (void (*)(GLenum))hw_render.get_proc_address ("glDisable");
	funcs[17].ptr = qglEnableClientState  = (void (*)(GLenum))hw_render.get_proc_address ("glEnableClientState");
	funcs[18].ptr = qglDisableClientState = (void (*)(GLenum))hw_render.get_proc_address ("glDisableClientState");
	funcs[19].ptr = qglPopMatrix          = hw_render.get_proc_address ("glPopMatrix");
	funcs[20].ptr = qglGetFloatv          = (void (*)(GLenum,  GLfloat *))hw_render.get_proc_address ("glGetFloatv");
	funcs[21].ptr = qglOrtho              = (void (*)(GLdouble,  GLdouble,  GLdouble,  GLdouble,  GLdouble,  GLdouble))hw_render.get_proc_address ("glOrtho");
	funcs[22].ptr = qglFrustum            = (void (*)(GLdouble,  GLdouble,  GLdouble,  GLdouble,  GLdouble,  GLdouble))hw_render.get_proc_address ("glFrustum");
	funcs[23].ptr = qglLoadMatrixf        = (void (*)(const GLfloat *))hw_render.get_proc_address ("glLoadMatrixf");
	funcs[24].ptr = qglLoadIdentity       = hw_render.get_proc_address ("glLoadIdentity");
	funcs[25].ptr = qglMatrixMode         = (void (*)(GLenum))hw_render.get_proc_address ("glMatrixMode");
	funcs[26].ptr = qglBindTexture        = (void (*)(GLenum,  GLuint))hw_render.get_proc_address ("glBindTexture");
	funcs[27].ptr = qglReadPixels         = (void (*)(GLint,  GLint,  GLsizei,  GLsizei,  GLenum,  GLenum,  GLvoid *))hw_render.get_proc_address ("glReadPixels");
	funcs[28].ptr = qglPolygonMode        = (void (*)(GLenum,  GLenum))hw_render.get_proc_address ("glPolygonMode");
	funcs[29].ptr = qglVertexPointer      = (void (*)(GLint,  GLenum,  GLsizei,  const GLvoid *))hw_render.get_proc_address ("glVertexPointer");
	funcs[30].ptr = qglTexCoordPointer    = (void (*)(GLint,  GLenum,  GLsizei,  const GLvoid *))hw_render.get_proc_address ("glTexCoordPointer");
	funcs[31].ptr = qglColorPointer       = (void (*)(GLint,  GLenum,  GLsizei,  const GLvoid *))hw_render.get_proc_address ("glColorPointer");
	funcs[32].ptr = qglDrawElements       = (void (*)(GLenum,  GLsizei,  GLenum,  const GLvoid *))hw_render.get_proc_address ("glDrawElements");
	funcs[33].ptr = qglViewport           = (void (*)(GLint,  GLint,  GLsizei,  GLsizei))hw_render.get_proc_address ("glViewport");
	funcs[34].ptr = qglDeleteTextures     = (void (*)(GLsizei,  const GLuint *))hw_render.get_proc_address ("glDeleteTextures");
	funcs[35].ptr = qglClearStencil       = (void (*)(GLint))hw_render.get_proc_address ("glClearStencil");
	funcs[36].ptr = qglColor4f            = (void (*)(GLfloat,  GLfloat,  GLfloat,  GLfloat))hw_render.get_proc_address ("glColor4f");
	funcs[37].ptr = qglScissor            = (void (*)(GLint,  GLint,  GLsizei,  GLsizei))hw_render.get_proc_address ("glScissor");
	funcs[38].ptr = qglStencilFunc        = (void (*)(GLenum,  GLint,  GLuint))hw_render.get_proc_address ("glStencilFunc");
	funcs[39].ptr = qglStencilOp          = (void (*)(GLenum,  GLenum,  GLenum))hw_render.get_proc_address ("glStencilOp");
	funcs[40].ptr = qglScalef             = (void (*)(GLfloat,  GLfloat,  GLfloat))hw_render.get_proc_address ("glScalef");
	funcs[41].ptr = qglDepthFunc          = (void (*)(GLenum))hw_render.get_proc_address ("glDepthFunc");
	funcs[42].ptr = qglTexEnvi            = (void (*)(GLenum,  GLenum,  GLint))hw_render.get_proc_address ("glTexEnvi");
	funcs[43].ptr = qglAlphaFunc          = (void (*)(GLenum,  GLfloat))hw_render.get_proc_address ("glAlphaFunc");
	
	if (log_cb) {
		int i;
		for (i = 0; i < GL_FUNCS_NUM; i++) {
			if (!funcs[i].ptr) log_cb(RETRO_LOG_ERROR, "vitaQuakeII: cannot get GL function #%d symbol.\n", i);
		}
	}
	
	return true;
}

int GLimp_Init( void *hinstance, void *wndproc )
{
   return 0;
}

void GLimp_AppActivate( qboolean active )
{
}

void GLimp_BeginFrame( float camera_separation )
{
	gVertexBuffer = gVertexBufferPtr;
	gColorBuffer = gColorBufferPtr;
	gTexCoordBuffer = gTexCoordBufferPtr;
	qglEnableClientState(GL_VERTEX_ARRAY);
	qglAlphaFunc(GL_GREATER,  0.666);
}

void GLimp_EndFrame (void)
{
}

qboolean GLimp_InitGL (void)
{
   int i;
	indices = (uint16_t*)malloc(sizeof(uint16_t)*MAX_INDICES);
	for (i=0;i<MAX_INDICES;i++){
		indices[i] = i;
	}
	gVertexBufferPtr = (float*)malloc(0x400000);
	gColorBufferPtr = (float*)malloc(0x200000);
	gTexCoordBufferPtr = (float*)malloc(0x200000);
	gl_set = true;
	return true;
}

void GLimp_DeinitGL (void)
{
	if (indices)
		free(indices);
	indices = NULL;

	if (gVertexBufferPtr)
		free(gVertexBufferPtr);
	gVertexBufferPtr = NULL;

	if (gColorBufferPtr)
		free(gColorBufferPtr);
	gColorBufferPtr = NULL;

	if (gTexCoordBufferPtr)
		free(gTexCoordBufferPtr);
	gTexCoordBufferPtr = NULL;

	gl_set = false;
}
#endif

static void context_destroy(void) 
{
	context_needs_reinit = true;
}

extern void restore_textures(void);
bool first_reset = true;

static void context_reset(void)
{
	if (!context_needs_reinit)
		return;
#ifdef HAVE_OPENGL
	glsm_ctl(GLSM_CTL_STATE_CONTEXT_RESET, NULL);

   if (!libretro_shared_context)
      if (!glsm_ctl(GLSM_CTL_STATE_SETUP, NULL))
         return;
	
	if (!is_soft_render) {
		initialize_gl();
		if (!first_reset)
			restore_textures();
		first_reset = false;
	}
#endif
	context_needs_reinit = false;
}

#ifdef HAVE_OPENGL
static bool context_framebuffer_lock(void *data)
{
    return false;
}

bool initialize_opengl(void)
{
   glsm_ctx_params_t params = {0};

   params.context_type     = RETRO_HW_CONTEXT_OPENGL;
   params.context_reset    = context_reset;
   params.context_destroy  = context_destroy;
   params.environ_cb       = environ_cb;
   params.stencil          = true;
   params.framebuffer_lock = context_framebuffer_lock;

   if (!glsm_ctl(GLSM_CTL_STATE_CONTEXT_INIT, &params))
   {
      log_cb(RETRO_LOG_ERROR, "Could not setup glsm.\n");
      return false;
   }

   if (environ_cb(RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT, NULL))
      libretro_shared_context = true;
   else
      libretro_shared_context = false;

   return true;
}

void destroy_opengl(void)
{
   if (!glsm_ctl(GLSM_CTL_STATE_CONTEXT_DESTROY, NULL))
   {
      log_cb(RETRO_LOG_ERROR, "Could not destroy glsm context.\n");
   }

   libretro_shared_context = false;
}
#endif

/* sys.c */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "errno.h"

int	curtime;
char cmd_line[256];
	
static byte	*membase    = NULL;
static int  hunkmaxsize = 0;
static int  cursize     = 0;

bool shutdown_core = false;

netadr_t	net_local_adr;

extern uint64_t rumble_tick;

int scr_width = 960;
int scr_height = 544;
float scr_aspect = 960.0f / 544.0f;

void *GetGameAPI (void *import);
qboolean	NET_CompareAdr (netadr_t a, netadr_t b)
{
	if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
		return true;
	return false;
}

void NET_Init (void)
{
	
}

void NET_Sleep(int msec)
{
}

#define	LOOPBACK	0x7f000001

#define	MAX_LOOPBACK	4

typedef struct
{
	byte	data[MAX_MSGLEN];
	int		datalen;
} loopmsg_t;

typedef struct
{
	loopmsg_t	msgs[MAX_LOOPBACK];
	int			get, send;
} loopback_t;

loopback_t	loopbacks[2];

qboolean	NET_GetLoopPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message)
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
		loop->get = loop->send - MAX_LOOPBACK;

	if (loop->get >= loop->send)
		return false;

	i = loop->get & (MAX_LOOPBACK-1);
	loop->get++;

	memcpy (net_message->data, loop->msgs[i].data, loop->msgs[i].datalen);
	net_message->cursize = loop->msgs[i].datalen;
	*net_from = net_local_adr;
	return true;

}


void NET_SendLoopPacket (netsrc_t sock, int length, void *data, netadr_t to)
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock^1];

	i = loop->send & (MAX_LOOPBACK-1);
	loop->send++;

	memcpy (loop->msgs[i].data, data, length);
	loop->msgs[i].datalen = length;
}

qboolean	NET_CompareBaseAdr (netadr_t a, netadr_t b)
{
	if (a.type != b.type)
		return false;

	if (a.type == NA_LOOPBACK)
		return true;

	if (a.type == NA_IP)
	{
		if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3])
			return true;
		return false;
	}

	if (a.type == NA_IPX)
	{
		if ((memcmp(a.ipx, b.ipx, 10) == 0))
			return true;
		return false;
	}
	return false;
}

char	*NET_AdrToString (netadr_t a)
{
	static	char	s[64];
#if 0
	Com_sprintf (s, sizeof(s), "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));
#endif

	return s;
}

qboolean	NET_StringToAdr (char *s, netadr_t *a)
{
	memset (a, 0, sizeof(*a));
	a->type = NA_LOOPBACK;
	return true;
}

void NET_SendPacket (netsrc_t sock, int length, void *data, netadr_t to)
{
	if ( to.type == NA_LOOPBACK )
	{
		NET_SendLoopPacket (sock, length, data, to);
		return;
	}
}

void	NET_Config (qboolean multiplayer)
{
}

qboolean	NET_IsLocalAddress (netadr_t adr)
{
	return NET_CompareAdr (adr, net_local_adr);
}

qboolean	NET_GetPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message)
{
	if (NET_GetLoopPacket (sock, net_from, net_message))
		return true;
	
	return false;
}

void GLimp_Shutdown( void )
{

}

static void extract_directory(char *out_dir, const char *in_dir, size_t size)
{
   size_t len;

   fill_pathname_parent_dir(out_dir, in_dir, size);

   /* Remove trailing slash, if required */
   len = strlen(out_dir);
   if ((len > 0) &&
       (out_dir[len - 1] == PATH_DEFAULT_SLASH_C()))
      out_dir[len - 1] = '\0';

   /* If parent directory is an empty string,
    * must set it to '.' */
   if (string_is_empty(out_dir))
      strlcpy(out_dir, ".", size);
}

int y = 20;
void LOG_FILE(const char *format, ...){
        va_list arg;
	int done;
	va_start(arg, format);
	char msg[512];
	done = vsnprintf(msg, 500, format, arg);
	va_end(arg);
	if (log_cb)
		log_cb(RETRO_LOG_INFO, "LOG2FILE: %s", msg);
	else
		fprintf(stderr, "LOG2FILE: %s\n", msg);
}

void Sys_Error (char *error, ...)
{
	char str[512] = { 0 };
	va_list		argptr;

	va_start (argptr,error);
	vsnprintf (str,512, error, argptr);
	va_end (argptr);
	LOG_FILE("Sys_Error: %s", str);
	Sys_Quit();
}

void Sys_Quit (void)
{
	LOG_FILE("Sys_Quit called");
	environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
	shutdown_core = true;
}

void Sys_UnloadGame (void)
{

} 

void *Sys_GetGameAPI (void *parms)
{
	return GetGameAPI (parms);
}


char *Sys_ConsoleInput (void)
{
	return NULL;
}

void Sys_ConsoleOutput (char *string)
{
	LOG_FILE("%s",string);
}

void utf2ascii(char* dst, uint16_t* src){
	if(!src || !dst)return;
	while(*src)*(dst++)=(*(src++))&0xFF;
	*dst=0x00;
}

void Sys_DefaultConfig(void)
{
	Cbuf_AddText ("lookstrafe \"1.000000\"\n");
	Cbuf_AddText ("lookspring \"0.000000\"\n");
}

extern menufield_s s_maxclients_field;
char *targetKeyboard;
void Sys_SetKeys(uint32_t keys, uint32_t state){
	Key_Event(keys, state, Sys_Milliseconds());
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   struct retro_core_option_display option_display;
   size_t opt_idx;

   if (port > 0)
      return;

   switch (device)
   {
      case RETRO_DEVICE_JOYPAD:
         quake_input_device = RETRO_DEVICE_JOYPAD;
         break;
      case RETRO_DEVICE_KEYBOARD:
         quake_input_device = RETRO_DEVICE_KEYBOARD;
         break;
      default:
         if (log_cb)
				log_cb(RETRO_LOG_INFO,
                  "Invalid libretro controller device, using default: RETRO_DEVICE_JOYPAD\n");
         quake_input_device = RETRO_DEVICE_JOYPAD;
         break;
   }

   /* Update internal input binds */
   if (!first_boot)
      set_input_binds();

   /* Show/hide keyboard mapping options */
   option_display.visible = (quake_input_device ==
         RETRO_DEVICE_KEYBOARD);

   for (opt_idx = 0; opt_idx < INPUT_KB_MAP_OPTIONS_LEN; opt_idx++)
   {
      option_display.key = input_kb_map_options[opt_idx].key;
      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY,
            &option_display);
   }
}

#define SET_BOUND_KEY_PAD(ret, id) Sys_SetKeys(input_binds_pad[id].keynum, (ret & (1 << id)) ? 1 : 0)

#define SET_BOUND_KEY_KB(id)              \
   Sys_SetKeys(input_binds_kb[id].keynum, \
         input_cb(0, RETRO_DEVICE_KEYBOARD, 0, input_kb_map[id]) ? 1 : 0)

void Sys_SendKeyEvents (void)
{
	int16_t ret = 0;

	if (!poll_cb)
		return;

	poll_cb();

	if (!input_cb)
		return;

   if (quake_input_device == RETRO_DEVICE_KEYBOARD)
   {
      /* Keyboard input */
      SET_BOUND_KEY_KB(KB_KEY_UP);
      SET_BOUND_KEY_KB(KB_KEY_DOWN);
      SET_BOUND_KEY_KB(KB_KEY_LEFT);
      SET_BOUND_KEY_KB(KB_KEY_RIGHT);
      SET_BOUND_KEY_KB(KB_KEY_MENU_SHOW);
      SET_BOUND_KEY_KB(KB_KEY_MENU_SELECT);
      SET_BOUND_KEY_KB(KB_KEY_MENU_CANCEL);
      SET_BOUND_KEY_KB(KB_KEY_HELP);
      SET_BOUND_KEY_KB(KB_KEY_INVENTORY_SHOW);
      SET_BOUND_KEY_KB(KB_KEY_INVENTORY_PREV);
      SET_BOUND_KEY_KB(KB_KEY_INVENTORY_NEXT);
      SET_BOUND_KEY_KB(KB_KEY_INVENTORY_USE);
      SET_BOUND_KEY_KB(KB_KEY_INVENTORY_DROP);
      SET_BOUND_KEY_KB(KB_KEY_WEAPON_NEXT);
      SET_BOUND_KEY_KB(KB_KEY_RUN);
      SET_BOUND_KEY_KB(KB_KEY_JUMP);
      SET_BOUND_KEY_KB(KB_KEY_CROUCH);

      /* Mouse input */
      Sys_SetKeys(input_binds_kb[KB_KEY_LAST].keynum,
            input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT) ?
                  1 : 0);
   }
   else
   {
      if (libretro_supports_bitmasks)
         ret = input_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
      else
      {
         unsigned i;
         for (i = RETRO_DEVICE_ID_JOYPAD_B; i <= RETRO_DEVICE_ID_JOYPAD_R3; i++)
         {
            if (input_cb(0, RETRO_DEVICE_JOYPAD, 0, i))
               ret |= (1 << i);
         }
      }

      /* Open Inventory */
      SET_BOUND_KEY_PAD(ret, RETRO_DEVICE_ID_JOYPAD_LEFT);

      /* Inventory Previous */
      SET_BOUND_KEY_PAD(ret, RETRO_DEVICE_ID_JOYPAD_UP);

      /* Inventory Next */
      SET_BOUND_KEY_PAD(ret, RETRO_DEVICE_ID_JOYPAD_DOWN);

      /* Use Inventory Item */
      SET_BOUND_KEY_PAD(ret, RETRO_DEVICE_ID_JOYPAD_RIGHT);

      /* Next Weapon */
      SET_BOUND_KEY_PAD(ret, RETRO_DEVICE_ID_JOYPAD_Y);

      /* Run */
      SET_BOUND_KEY_PAD(ret, RETRO_DEVICE_ID_JOYPAD_L);

      /* Crouch / Descend */
      SET_BOUND_KEY_PAD(ret, RETRO_DEVICE_ID_JOYPAD_R);

      /* Jump / Climb */
      SET_BOUND_KEY_PAD(ret, RETRO_DEVICE_ID_JOYPAD_L2);

      /* Attack */
      SET_BOUND_KEY_PAD(ret, RETRO_DEVICE_ID_JOYPAD_R2);

      /* Drop Inventory Item */
      SET_BOUND_KEY_PAD(ret, RETRO_DEVICE_ID_JOYPAD_R3);

      /* Show / Hide Help Computer */
      SET_BOUND_KEY_PAD(ret, RETRO_DEVICE_ID_JOYPAD_SELECT);

      /* Note: Menu open/cancel/select keys are
       * arbitrarily hard-coded
       * > Show Menu: K_ESCAPE
       * > Cancel: K_AUX4
       * > Select: K_AUX1 */

      /* Show Menu */
      SET_BOUND_KEY_PAD(ret, RETRO_DEVICE_ID_JOYPAD_START);

      /* Menu Cancel */
      SET_BOUND_KEY_PAD(ret, RETRO_DEVICE_ID_JOYPAD_B);

      /* Menu Select */
      SET_BOUND_KEY_PAD(ret, RETRO_DEVICE_ID_JOYPAD_A);
   }

	sys_frame_time = Sys_Milliseconds();
}


void Sys_AppActivate (void)
{
}

void Sys_CopyProtect (void)
{
}

char *Sys_GetClipboardData( void )
{
	return NULL;
}

void *Hunk_Begin (int maxsize)
{
	/* reserve a huge chunk of memory, but don't commit any yet */
	hunkmaxsize = maxsize;
	cursize     = 0;
	membase     = malloc(hunkmaxsize);

	if (!membase)
		Sys_Error("unable to allocate %d bytes", hunkmaxsize);
	else
		memset (membase, 0, hunkmaxsize);

	return (void*)membase;
}

void *Hunk_Alloc (int size)
{
	byte *buf;

	/* round to cacheline */
	size = (size+31)&~31;

	if (cursize + size > hunkmaxsize)
		Sys_Error("Hunk_Alloc overflow");

	buf = membase + cursize;
	cursize += size;

	return buf;
}

int Hunk_End (void)
{
	/* We would prefer to shrink the allocated memory
	 * buffer to 'cursize' bytes here, but there exists
	 * no robust cross-platform method for doing this
	 * given that pointers to arbitrary locations in
	 * the buffer are stored and used throughout the
	 * codebase...
	 * (i.e. realloc() would invalidate these pointers,
	 * and break everything)
	 * Attempts were made to allocate hunks dynamically,
	 * storing them in an RBUF array - but the codebase
	 * plays such dirty tricks with the returned pointers
	 * that this turned out to be impractical (it would
	 * have required a major rewrite of the renderers...) */
	return cursize;
}

void Hunk_Free (void *base)
{
	if (base == membase)
		membase = NULL;

	if (base)
		free(base);
}

int Sys_Milliseconds (void)
{
	static uint64_t	base;

	uint64_t time = cpu_features_get_time_usec() / 1000;
	
	if (!base)
	{
		base = time;
	}

	curtime = (int)(time - base);
	
	return curtime;
}

void Sys_Mkdir (char *path)
{
	if (string_is_empty(path) ||
		 path_is_directory(path))
		return;

	path_mkdir(path);
}

static	char	findbase[MAX_OSPATH];
static	char	findpath[MAX_OSPATH * 2];
static	char	findpattern[MAX_OSPATH];
static	RDIR	*fdir = NULL;

/* Forward declarations */
static int glob_match(char *pattern, char *text);

/* Like glob_match, but match PATTERN against any final segment of TEXT.  */
static int glob_match_after_star(char *pattern, char *text)
{
	register char *p = pattern, *t = text;
	register char c, c1;

	while ((c = *p++) == '?' || c == '*')
		if (c == '?' && *t++ == '\0')
			return 0;

	if (c == '\0')
		return 1;

	if (c == '\\')
		c1 = *p;
	else
		c1 = c;

	while (1) {
		if ((c == '[' || *t == c1) && glob_match(p - 1, t))
			return 1;
		if (*t++ == '\0')
			return 0;
	}
}

/* Match the pattern PATTERN against the string TEXT;
   return 1 if it matches, 0 otherwise.
   A match means the entire string TEXT is used up in matching.
   In the pattern string, `*' matches any sequence of characters,
   `?' matches any character, [SET] matches any character in the specified set,
   [!SET] matches any character not in the specified set.
   A set is composed of characters or ranges; a range looks like
   character hyphen character (as in 0-9 or A-Z).
   [0-9a-zA-Z_] is the set of characters allowed in C identifiers.
   Any other character in the pattern must be matched exactly.
   To suppress the special syntactic significance of any of `[]*?!-\',
   and match the character exactly, precede it with a `\'.
*/
static int glob_match(char *pattern, char *text)
{
	register char *p = pattern, *t = text;
	register char c;

	while ((c = *p++) != '\0')
		switch (c) {
		case '?':
			if (*t == '\0')
				return 0;
			else
				++t;
			break;

		case '\\':
			if (*p++ != *t++)
				return 0;
			break;

		case '*':
			return glob_match_after_star(p, t);

		case '[':
			{
				register char c1 = *t++;
				int invert;

				if (!c1)
					return (0);

				invert = ((*p == '!') || (*p == '^'));
				if (invert)
					p++;

				c = *p++;
				while (1) {
					register char cstart = c, cend = c;

					if (c == '\\') {
						cstart = *p++;
						cend = cstart;
					}
					if (c == '\0')
						return 0;

					c = *p++;
					if (c == '-' && *p != ']') {
						cend = *p++;
						if (cend == '\\')
							cend = *p++;
						if (cend == '\0')
							return 0;
						c = *p++;
					}
					if (c1 >= cstart && c1 <= cend)
						goto match;
					if (c == ']')
						break;
				}
				if (!invert)
					return 0;
				break;

			  match:
				/* Skip the rest of the [...] construct that already matched.  */
				while (c != ']') {
					if (c == '\0')
						return 0;
					c = *p++;
					if (c == '\0')
						return 0;
					else if (c == '\\')
						++p;
				}
				if (invert)
					return 0;
				break;
			}

		default:
			if (c != *t++)
				return 0;
		}

	return *t == '\0';
}

char *Sys_FindFirst (char *path, unsigned musthave, unsigned canhave)
{
	char *p;

	if (fdir != NULL)
		Sys_Error ("Sys_BeginFind without close");

	COM_FilePath (path, findbase);
	strcpy(findbase, path);

	if ((p = strrchr(findbase, '/')) != NULL) {
		*p = 0;
		strcpy(findpattern, p + 1);
	} else
		strcpy(findpattern, "*");

	if (strcmp(findpattern, "*.*") == 0)
		strcpy(findpattern, "*");
	
	fdir = retro_opendir(findbase);
	if (fdir == NULL)
		return NULL;
	while ((retro_readdir(fdir)) > 0)
   {
      if (!*findpattern || 
            glob_match(findpattern, retro_dirent_get_name(fdir)))
      {
            sprintf (findpath, "%s/%s", findbase, retro_dirent_get_name(fdir));
            return findpath;
      }
   }
	return NULL;
}

char *Sys_FindNext (unsigned musthave, unsigned canhave)
{
	if (fdir == NULL)
		return NULL;
	while ((retro_readdir(fdir)) > 0)
   {
      if (!*findpattern || glob_match(findpattern, retro_dirent_get_name(fdir)))
      {
            sprintf (findpath, "%s/%s", findbase, retro_dirent_get_name(fdir));
            return findpath;
      }
   }
	return NULL;
}

void Sys_FindClose (void)
{
	if (fdir != NULL)
		retro_closedir(fdir);
		
	fdir = NULL;
}

void	Sys_Init (void)
{
}

extern void IN_StopRumble();

static int invert_y_axis = 1;

/*============================================================================= */

/* Only discrete framerates are permitted
 * > Note that 40 fps is unsupported (internal
 *   SFX resampling fails at this framerate,
 *   with images and aliases spinning out of
 *   control...) */
static const unsigned supported_framerates[] = {
   30,
   50,
   60,
   72,
   75,
   90,
   100,
   119,
   120,
   144,
   155,
   160,
   165,
   180,
   200,
   240,
   244,
   300,
   360
};
#define NUM_SUPPORTED_FRAMERATES (sizeof(supported_framerates) / sizeof(supported_framerates[0]))

static unsigned sanitise_framerate(float target)
{
   unsigned target_int = (unsigned)(target + 0.5f);
   unsigned i = 1;

   if (target_int <= supported_framerates[0])
      return supported_framerates[0];

   if (target_int >= supported_framerates[NUM_SUPPORTED_FRAMERATES - 1])
      return supported_framerates[NUM_SUPPORTED_FRAMERATES - 1];

   while (i < NUM_SUPPORTED_FRAMERATES)
   {
      if (supported_framerates[i] > target_int)
         break;

      i++;
   }

   if ((supported_framerates[i] - target_int) <=
       (target_int - supported_framerates[i - 1]))
      return supported_framerates[i];

   return supported_framerates[i - 1];
}

bool initial_resolution_set = false;
static void update_variables(bool startup)
{
	struct retro_variable var;
	struct retro_core_option_display option_display;

   var.key = "vitaquakeii_framerate";
   var.value = NULL;

   if (startup)
   {
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
      {
         if (!strcmp(var.value, "auto"))
         {
            float target_framerate = 0.0f;
            if (!environ_cb(RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE,
                     &target_framerate))
               target_framerate = 60.0f;

            framerate = sanitise_framerate(target_framerate);
         }
         else
            framerate = atoi(var.value);
      }
      else
         framerate    = 60;

      switch (framerate)
      {
         case 30:
            framerate_ms = 33;
            break;
         case 50:
            framerate_ms = 20;
            break;
         case 60:
            framerate_ms = 16;
            break;
         case 72:
            framerate_ms = 14;
            break;
         case 75:
            framerate_ms = 13;
            break;
         case 90:
            framerate_ms = 11;
            break;
         case 100:
            framerate_ms = 10;
            break;
         case 119:
         case 120:
            framerate_ms = 8;
            break;
         case 144:
            framerate_ms = 7;
            break;
         case 155:
         case 160:
         case 165:
            framerate_ms = 6;
            break;
         case 180:
         case 200:
            framerate_ms = 5;
            break;
         case 240:
         case 244:
            framerate_ms = 4;
            break;
         case 300:
         case 360:
            framerate_ms = 3;
            break;
         default:
				framerate    = 60;
				framerate_ms = 16;
				break;
      }

		var.key = "vitaquakeii_gamma";
		var.value = NULL;
		libretro_gamma = 1.0f;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			/* Invert sense so greater = brighter,
			 * and scale to a range of 0.5 to 1.3 */
			float gamma_correction = atof(var.value);

			if (gamma_correction < 0.2f)
				gamma_correction = 0.2f;
			if (gamma_correction > 1.0f)
				gamma_correction = 1.0f;

			libretro_gamma = (-1.0f * gamma_correction) + 1.5f;
		}

#ifdef HAVE_OPENGL
      var.key = "vitaquakeii_renderer";
      var.value = NULL;

      enable_opengl = !environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || strcmp(var.value, "software") != 0;

		var.key = "vitaquakeii_gl_modulate";
		var.value = NULL;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			libretro_gl_modulate = atof(var.value);

			if (libretro_gl_modulate > 5.0f)
				libretro_gl_modulate = 5.0f;
			if (libretro_gl_modulate < 1.0f)
				libretro_gl_modulate = 1.0f;
		}

		/* Hide irrelevant options */
		option_display.visible = false;

		if (enable_opengl)
		{
			option_display.key = "vitaquakeii_sw_dithered_filtering";
			environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
		}
		else
		{
			option_display.key = "vitaquakeii_gl_modulate";
			environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

			option_display.key = "vitaquakeii_gl_texture_filtering";
			environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

			option_display.key = "vitaquakeii_gl_shadows";
			environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

			option_display.key = "vitaquakeii_gl_xflip";
			environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

			option_display.key = "vitaquakeii_gl_hud_scale";
			environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
		}
#endif
		var.key = "vitaquakeii_resolution";
		var.value = NULL;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && !initial_resolution_set)
		{
			char *pch;
			char str[100];
			snprintf(str, sizeof(str), "%s", var.value);

			pch = strtok(str, "x");
			if (pch)
				scr_width = strtoul(pch, NULL, 0);
			pch = strtok(NULL, "x");
			if (pch)
				scr_height = strtoul(pch, NULL, 0);

			/* Software renderer caps out at 1920x1200,
			 * and cannot handle aspect ratios below 4/3 */
			if (!enable_opengl)
			{
				bool invalid_resolution = false;

				if ((scr_width == 1280) &&
					 (scr_height == 1024))
				{
					/* Fall back to nearest 4:3 resolution */
					scr_width  = 1024;
					scr_height = 768;
					invalid_resolution = true;
				}
				else if ((scr_width > 1920) ||
							(scr_height > 1200))
				{
					/* Fall back to highest supported resolution */
					scr_width  = 1920;
					scr_height = 1200;
					invalid_resolution = true;
				}

				if (invalid_resolution && log_cb)
					log_cb(RETRO_LOG_WARN,
							"Specified resolution unsupported by software renderer - falling back to %i x %i.\n",
							scr_width, scr_height);
			}

			scr_aspect = (float)scr_width / (float)scr_height;

			if (log_cb)
				log_cb(RETRO_LOG_INFO, "Got size: %i x %i.\n", scr_width, scr_height);

			initial_resolution_set = true;
		}
   }
   
	var.key = "vitaquakeii_invert_y_axis";
	var.value = NULL;

	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	{
		if (strcmp(var.value, "disabled") == 0)
			invert_y_axis = 1;
		else
			invert_y_axis = -1;
	}

	var.key = "vitaquakeii_analog_deadzone";
	var.value = NULL;
	analog_deadzone = (int)(0.15f * ANALOG_RANGE);

	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		analog_deadzone = (int)(atoi(var.value) * 0.01f * ANALOG_RANGE);

#if defined(HAVE_CDAUDIO)
	var.key = "vitaquakeii_cdaudio_enabled";
	var.value = NULL;
	cdaudio_enabled = true;

	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		if (strcmp(var.value, "disabled") == 0)
			cdaudio_enabled = false;

	if (!cdaudio_enabled && CDAudio_Playing())
		CDAudio_Stop();

	var.key = "vitaquakeii_cdaudio_volume";
	var.value = NULL;
	cdaudio_volume = 0.5f;

	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	{
		float volume_level = atof(var.value);

		if (volume_level < 5.0f)
			volume_level = 5.0f;
		if (volume_level > 130.0f)
			volume_level = 130.0f;

		cdaudio_volume = volume_level / 100.0f;
	}
#endif

	/* We need Qcommon_Init to be executed to be able to set Cvars */
	if (!startup)
	{
#ifdef HAVE_OPENGL
		float libretro_hud_scale_prev;
#endif
		var.key = "vitaquakeii_rumble";
		var.value = NULL;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			if (strcmp(var.value, "disabled") == 0)
				Cvar_SetValue( "pstv_rumble", 0 );
			else
				Cvar_SetValue( "pstv_rumble", 1 );
		}

		var.key = "vitaquakeii_cl_run";
		var.value = NULL;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			if (strcmp(var.value, "disabled") == 0)
				Cvar_SetValue( "cl_run", 0 );
			else
				Cvar_SetValue( "cl_run", 1 );
		}

		var.key = "vitaquakeii_aimfix";
		var.value = NULL;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			if (strcmp(var.value, "disabled") == 0)
				Cvar_SetValue( "aimfix", 0 );
			else
				Cvar_SetValue( "aimfix", 1 );
		}

		var.key = "vitaquakeii_mouse_sensitivity";
		var.value = NULL;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			float mouse_sensitivity = atof(var.value);
			Cvar_SetValue( "sensitivity", mouse_sensitivity );
		}

		var.key = "vitaquakeii_sw_dithered_filtering";
		var.value = NULL;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && sw_texfilt)
		{
			if (strcmp(var.value, "enabled") == 0)
				Cvar_SetValue( "sw_texfilt", 1 );
			else
				Cvar_SetValue( "sw_texfilt", 0);
		}

#ifdef HAVE_OPENGL
		var.key = "vitaquakeii_gl_texture_filtering";
		var.value = NULL;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			if (strcmp(var.value, "nearest") == 0)
				Cvar_Set( "gl_texturemode", "GL_NEAREST" );
			else if (strcmp(var.value, "linear") == 0)
				Cvar_Set( "gl_texturemode", "GL_LINEAR" );
			else if (strcmp(var.value, "linear_hq") == 0)
				Cvar_Set( "gl_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
			else
				Cvar_Set( "gl_texturemode", "GL_NEAREST_MIPMAP_LINEAR" );
		}

		var.key = "vitaquakeii_gl_xflip";
		var.value = NULL;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			if (strcmp(var.value, "disabled") == 0)
				Cvar_SetValue( "gl_xflip", 0 );
			else
				Cvar_SetValue( "gl_xflip", 1 );
		}

		var.key = "vitaquakeii_gl_hud_scale";
		var.value = NULL;
		libretro_hud_scale_prev = libretro_hud_scale;
		libretro_hud_scale = 0.5f;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			libretro_hud_scale = atof(var.value);

			if (libretro_hud_scale > 1.0f)
				libretro_hud_scale = 1.0f;
			if (libretro_hud_scale < 0.0f)
				libretro_hud_scale = 0.0f;
		}

		/* TODO/FIXME: The menu does not at present
		 * support dynamic scaling (multiple values
		 * are set upon menu initialisation instead
		 * of while drawing the elements). This is
		 * tedious to fix, so in the meantime we
		 * will simply close the menu if it is currently
		 * open and the scale has changed */
		if ((libretro_hud_scale != libretro_hud_scale_prev) &&
			 (m_menudepth > 0))
			M_ForceMenuOff();
#endif

		var.key = "vitaquakeii_xhair";
		var.value = NULL;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			if (strcmp(var.value, "disabled") == 0)
				Cvar_SetValue( "crosshair", 0 );
			else if (strcmp(var.value, "dot") == 0)
				Cvar_SetValue( "crosshair", 2 );
			else if (strcmp(var.value, "angle") == 0)
				Cvar_SetValue( "crosshair", 3 );
			else
				Cvar_SetValue( "crosshair", 1 );
		}

		var.key = "vitaquakeii_fps";
		var.value = NULL;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			if (strcmp(var.value, "disabled") == 0)
				Cvar_SetValue( "cl_drawfps", 0 );
			else
				Cvar_SetValue( "cl_drawfps", 1 );
		}

#ifdef HAVE_OPENGL
		var.key = "vitaquakeii_gl_shadows";
		var.value = NULL;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && !is_soft_render)
		{
			if (strcmp(var.value, "disabled") == 0)
				Cvar_SetValue( "gl_shadows", 0 );
			else
				Cvar_SetValue( "gl_shadows", 1 );
		}
#endif

		var.key = "vitaquakeii_hand";
		var.value = NULL;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			if (strcmp(var.value, "right") == 0)
				Cvar_SetValue( "hand", 0 );
			else if (strcmp(var.value, "left") == 0)
				Cvar_SetValue( "hand", 1 );
			else if (strcmp(var.value, "center") == 0)
				Cvar_SetValue( "hand", 2 );
			else
				Cvar_SetValue( "hand", 3 );
		}

		var.key = "vitaquakeii_cin_force43";
		var.value = NULL;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			if (strcmp(var.value, "disabled") == 0)
				Cvar_SetValue( "cin_force43", 0 );
			else
				Cvar_SetValue( "cin_force43", 1 );
		}
	}

   update_kb_mapping();
}

void retro_init(void)
{
   size_t i;

   shutdown_core = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;

   /* Build keyboard key hash map */
   for (i = 0; i < INPUT_KB_KEYS_LEN; i++)
      RHMAP_SET_STR(input_kb_keys_hash_map, input_kb_keys[i].id_str, &input_kb_keys[i]);
}

void retro_deinit(void)
{
	if (!shutdown_core)
		CL_Quit_f();

#ifdef HAVE_OPENGL
	GLimp_DeinitGL();
#endif

	CDAudio_Shutdown();

   libretro_supports_bitmasks = false;

   RHMAP_FREE(input_kb_keys_hash_map);
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "vitaQuakeII";
   info->library_version  = "v2.3" ;
   info->need_fullpath    = true;
   info->valid_extensions = "pak";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   info->timing.fps            = framerate;
   info->timing.sample_rate    = AUDIO_SAMPLE_RATE;

   info->geometry.base_width   = scr_width;
   info->geometry.base_height  = scr_height;
   info->geometry.max_width    = scr_width;
   info->geometry.max_height   = scr_height;
   info->geometry.aspect_ratio = scr_aspect;
}

void retro_set_environment(retro_environment_t cb)
{
   bool option_categories_supported;
   struct retro_log_callback log;
   struct retro_vfs_interface_info vfs_iface_info;

   static const struct retro_controller_description port_1[] = {
      { "Analog Gamepad",   RETRO_DEVICE_JOYPAD },
      { "Keyboard + Mouse", RETRO_DEVICE_KEYBOARD },
   };
   static const struct retro_controller_info ports[] = {
      { port_1, 2 },
      { 0 },
   };

   environ_cb = cb;

   initialise_kb_mapping_opts();
   libretro_set_core_options(environ_cb,
         &option_categories_supported);

   if(environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = NULL;

   vfs_iface_info.required_interface_version = 1;
   vfs_iface_info.iface                      = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_iface_info))
   {
      filestream_vfs_init(&vfs_iface_info);
      dirent_vfs_init(&vfs_iface_info);
	}

   environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);
}

void retro_reset(void)
{
}

void retro_set_rumble_strong(void)
{
   uint16_t strength_strong = 0xffff;
   if (!rumble.set_rumble_state)
      return;

   rumble.set_rumble_state(0, RETRO_RUMBLE_STRONG, strength_strong);
}

void retro_unset_rumble_strong(void)
{
   if (!rumble.set_rumble_state)
      return;

   rumble.set_rumble_state(0, RETRO_RUMBLE_STRONG, 0);
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

bool retro_load_game(const struct retro_game_info *info)
{
	int i;
	char path_lower[1024];
	char parent_dir[1024];
	bool use_external_savedir = false;
	const char *base_save_dir = NULL;
	enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;

#if defined(ROGUE)
	const char *core_game_dir = "rogue";
	const char *core_game_error_msg = "Error: Quake II - Ground Zero (rogue) game files required";
#elif defined(XATRIX)
	const char *core_game_dir = "xatrix";
	const char *core_game_error_msg = "Error: Quake II - The Reckoning (xatrix) game files required";
#elif defined(ZAERO)
	const char *core_game_dir = "zaero";
	const char *core_game_error_msg = "Error: Quake II - Zaero (zaero) game files required";
#else
	const char *core_game_dir = "baseq2";
	const char *core_game_error_msg = "Error: Quake II (baseq2) game files required";
#endif

	path_lower[0] = '\0';
	parent_dir[0] = '\0';

	if (!info || !info->path)
		return false;

	environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, input_desc);

	if (environ_cb(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &rumble))
		log_cb(RETRO_LOG_INFO, "Rumble environment supported.\n");
	else
		log_cb(RETRO_LOG_INFO, "Rumble environment not supported.\n");

	update_variables(true);

	if (enable_opengl && !environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
	{
		if (log_cb)
			log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
		return false;
	}

	if (!enable_opengl
#ifdef HAVE_OPENGL
	    || !initialize_opengl()
#endif
	    )
	{
		if (log_cb)
			log_cb(RETRO_LOG_INFO, "vitaQuakeII: using software renderer.\n");
		
		fmt = RETRO_PIXEL_FORMAT_RGB565;
		if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
		{
			if (log_cb)
				log_cb(RETRO_LOG_INFO, "RGB565 is not supported.\n");
			return false;
		}
		is_soft_render = true;
	}
	else
	{
		if (log_cb)
			log_cb(RETRO_LOG_INFO, "vitaQuakeII: using OpenGL renderer.\n");
	}
	
	sprintf(path_lower, "%s", info->path);
	
	for (i=0; path_lower[i]; ++i)
		path_lower[i] = tolower(path_lower[i]);
	
	extract_directory(g_rom_dir, info->path, sizeof(g_rom_dir));

	/* Get CD audio directory */
	fill_pathname_join(g_music_dir, g_rom_dir, "music", sizeof(g_music_dir));

	if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &base_save_dir) && base_save_dir)
	{
		if (!string_is_empty(base_save_dir))
		{
			/* Get game 'name' (i.e. subdirectory) */
			const char *game_name = path_basename(g_rom_dir);

			/* > Build final save path */
			fill_pathname_join(g_save_dir, base_save_dir, game_name, sizeof(g_save_dir));
			use_external_savedir = true;

			/* > Create save directory, if required */
			if (!path_is_directory(g_save_dir))
				use_external_savedir = path_mkdir(g_save_dir);
		}
	}

	/* > Final check: is the save directory the same as the 'rom' directory?
	 *   (i.e. ensure logical behaviour if user has set a bizarre save path...) */
	use_external_savedir = use_external_savedir && (strcmp(g_save_dir, g_rom_dir) != 0);
	
	/* If we are not using an external save directory,
	 * then set g_save_dir to an empty string (rom directory
	 * will be used by default) */
	if (!use_external_savedir)
		g_save_dir[0] = '\0';
	
	/* Ensure that we have valid content
	 * (different cores are required for base
	 * game + expansions) */
	if (!strstr(path_lower, core_game_dir))
	{
		unsigned msg_interface_version = 0;
		environ_cb(RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION,
				&msg_interface_version);

		if (msg_interface_version >= 1)
		{
			struct retro_message_ext msg;

			msg.msg      = core_game_error_msg;
			msg.duration = 3000;
			msg.priority = 3;
			msg.level    = RETRO_LOG_ERROR;
			msg.target   = RETRO_MESSAGE_TARGET_ALL;
			msg.type     = RETRO_MESSAGE_TYPE_NOTIFICATION;
			msg.progress = -1;

			environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE_EXT, &msg);
		}
		else
		{
			struct retro_message msg;

			msg.msg      = core_game_error_msg;
			msg.frames   = 180;

			environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
		}

		return false;
	}

	/* Quake II base directory is the *parent*
	 * of the game directory */
	extract_directory(parent_dir, g_rom_dir, sizeof(parent_dir));
	strlcpy(g_rom_dir, parent_dir, sizeof(g_rom_dir));

	return true;
}

void retro_run(void)
{
	bool updated = false;
#ifdef HAVE_OPENGL
	if (!is_soft_render) {
      if (!libretro_shared_context)
         glsm_ctl(GLSM_CTL_STATE_BIND, NULL);
		qglBindFramebuffer(RARCH_GL_FRAMEBUFFER, hw_render.get_current_framebuffer());
		qglEnable(GL_TEXTURE_2D);
	}
#endif
	if (first_boot)
	{
		const char *argv[32];
		const char *empty_string = "";
	
		argv[0] = empty_string;
		int argc = 1;
#if defined(ROGUE) || defined(XATRIX) || defined(ZAERO)
		argc = 4;
		argv[1] = "+set";
		argv[2] = "game";
#if defined(ROGUE)
		argv[3] = "rogue";
#elif defined(XATRIX)
		argv[3] = "xatrix";
#elif defined(ZAERO)
		argv[3] = "zaero";
#endif
#endif
		Qcommon_Init(argc, (char**)argv);
		if (is_soft_render) Cvar_Set( "vid_ref", "soft" );
		update_variables(false);
		set_input_binds();
		first_boot = false;
	}
	
	if (rumble_tick != 0)
		if (cpu_features_get_time_usec() - rumble_tick > 500000)
         IN_StopRumble(); /* 0.5 sec */
	
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
		update_variables(false);

   /* TODO/FIXME - argument should be changed into float for better accuracy of fixed timestep */
	Qcommon_Frame (framerate_ms);

	if (shutdown_core)
		return;

	if (is_soft_render)
		video_cb(tex_buffer, scr_width, scr_height, scr_width << 1);
	else
	{
#ifdef HAVE_OPENGL
      if (!libretro_shared_context)
         glsm_ctl(GLSM_CTL_STATE_UNBIND, NULL);
		video_cb(RETRO_HW_FRAME_BUFFER_VALID, scr_width, scr_height, 0);
#endif
	}
	
	audio_callback();
}

void retro_unload_game(void)
{
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   (void)type;
   (void)info;
   (void)num;
   return false;
}

size_t retro_serialize_size(void)
{
   return 0;
}

bool retro_serialize(void *data_, size_t size)
{
   return false;
}

bool retro_unserialize(const void *data_, size_t size)
{
   return false;
}

void *retro_get_memory_data(unsigned id)
{
   (void)id;
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   (void)id;
   return 0;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}


/* snddma.c */
#include "../client/snd_loc.h"

static volatile int sound_initialized = 0;
static int stop_audio = false;

static int16_t audio_buffer[AUDIO_BUFFER_SIZE];
static int16_t audio_out_buffer[AUDIO_BUFFER_SIZE];
static unsigned audio_buffer_ptr = 0;

static unsigned audio_batch_frames_max = AUDIO_BUFFER_SIZE >> 1;

static void audio_callback(void)
{
   unsigned read_first;
   unsigned read_second;
   unsigned samples_per_frame      = (2 * AUDIO_SAMPLE_RATE) / framerate;
   unsigned audio_frames_remaining = samples_per_frame >> 1;
   unsigned read_end               = audio_buffer_ptr + samples_per_frame;
   int16_t *audio_out_ptr          = audio_out_buffer;
   uintptr_t i;

   if (read_end > AUDIO_BUFFER_SIZE)
      read_end = AUDIO_BUFFER_SIZE;

   read_first  = read_end - audio_buffer_ptr;
   read_second = samples_per_frame - read_first;

   for (i = 0; i < read_first; i++)
      *(audio_out_ptr++) = *(audio_buffer + audio_buffer_ptr + i);

   audio_buffer_ptr += read_first;

   if (read_second >= 1)
   {
      for (i = 0; i < read_second; i++)
         *(audio_out_ptr++) = *(audio_buffer + i);

      audio_buffer_ptr = read_second;
   }

   CDAudio_Mix(audio_out_buffer, samples_per_frame >> 1, cdaudio_volume);

   /* At 30 fps, we generate (2 * 1470) samples
    * per frame. This may exceed the capacity of
    * the frontend audio batch callback; if so,
    * write the audio samples in chunks */
   audio_out_ptr = audio_out_buffer;
   do
   {
      unsigned audio_frames_to_write =
            (audio_frames_remaining > audio_batch_frames_max) ?
                  audio_batch_frames_max : audio_frames_remaining;
      unsigned audio_frames_written  =
            audio_batch_cb(audio_out_ptr, audio_frames_to_write);

      if ((audio_frames_written < audio_frames_to_write) &&
          (audio_frames_written > 0))
         audio_batch_frames_max = audio_frames_written;

      audio_frames_remaining -= audio_frames_to_write;
      audio_out_ptr          += audio_frames_to_write << 1;
   }
   while (audio_frames_remaining > 0);
}

uint64_t initial_tick;

qboolean SNDDMA_Init(void)
{
   sound_initialized = 0;

   /* Force Quake to use our settings */
   Cvar_SetValue( "s_khz", AUDIO_SAMPLE_RATE );
   Cvar_SetValue( "s_loadas8bit", false );

   /* Fill the audio DMA information block */
   dma.samplebits       = 16;
   dma.speed            = AUDIO_SAMPLE_RATE;
   dma.channels         = 2;
   dma.samples          = AUDIO_BUFFER_SIZE;
   dma.samplepos        = 0;
   dma.submission_chunk = 1;
   dma.buffer           = (byte *)audio_buffer;

   sound_initialized    = 1;

   initial_tick         = cpu_features_get_time_usec();

   return true;
}

int SNDDMA_GetDMAPos(void)
{
	if(!sound_initialized)
		return 0;
	return dma.samplepos = audio_buffer_ptr;
}

void SNDDMA_Shutdown(void)
{
	if(!sound_initialized)
		return;

	stop_audio = true;

	sound_initialized = 0;
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void)
{
}

void SNDDMA_BeginPainting(void)
{    
}

/* vid.c */

#define REF_OPENGL  0

cvar_t *vid_ref;
cvar_t *vid_fullscreen;

extern cvar_t *gl_picmip;
cvar_t *gl_mode;
#ifdef HAVE_OPENGL
extern cvar_t *gl_driver;
#endif

static menuframework_s  s_opengl_menu;
static menuframework_s *s_current_menu;

static menulist_s       s_mode_list;
static menulist_s       s_ref_list;
static menuslider_s     s_screensize_slider;
static menuaction_s     s_cancel_action;
static menuaction_s     s_defaults_action;
static menulist_s       s_shadows_slider;

viddef_t    viddef;             /* global video state */

refexport_t re;

refexport_t GetRefAPI (refimport_t rimp);
refexport_t SWR_GetRefAPI (refimport_t rimp);


/*
==========================================================================

DIRECT LINK GLUE

==========================================================================
*/

#define MAXPRINTMSG 4096
void VID_Printf (int print_level, char *fmt, ...)
{
   va_list     argptr;
   char        msg[MAXPRINTMSG];

   va_start(argptr,fmt);
   vsprintf(msg,fmt,argptr);
   va_end(argptr);
#ifdef DEBUG
   printf(msg);
#endif
   if (print_level == PRINT_ALL)
      Com_Printf("%s", msg);
   else
      Com_DPrintf("%s", msg);
}

void VID_Error (int err_level, char *fmt, ...)
{
   va_list     argptr;
   char        msg[MAXPRINTMSG];

   va_start (argptr,fmt);
   vsprintf (msg,fmt,argptr);
   va_end (argptr);
#ifdef DEBUG
   printf(msg);
#endif
   Com_Error (err_level, "%s", msg);
}

void VID_NewWindow (int width, int height)
{
   viddef.width = width;
   viddef.height = height;
}

/*
** VID_GetModeInfo
*/
typedef struct vidmode_s
{
   const char *description;
   int         width;
   int         height;
} vidmode_t;

vidmode_t vid_modes[] =
{
   { "Mode 0: 320x240",       320,  240 },
   { "Mode 1: 400x240",       400,  240 },
   { "Mode 2: 480x272",       480,  272 },
   { "Mode 3: 512x384",       512,  384 },
   { "Mode 4: 640x368",       640,  368 },
   { "Mode 5: 640x480",       640,  480 },
   { "Mode 6: 720x408",       720,  408 },
   { "Mode 7: 800x600",       800,  600 },
   { "Mode 8: 960x544",       960,  544 },
   { "Mode 9: 1024x768",     1024,  768 },
   { "Mode 10: 1280x720",    1280,  720 },
   { "Mode 11: 1280x800",    1280,  800 },
   { "Mode 12: 1280x1024",   1280, 1024 },
   { "Mode 13: 1360x768",    1360,  768 },
   { "Mode 14: 1366x768",    1366,  768 },
   { "Mode 15: 1440x900",    1440,  900 },
   { "Mode 16: 1600x900",    1600,  900 },
   { "Mode 17: 1680x1050",   1680, 1050 },
   { "Mode 18: 1920x1080",   1920, 1080 },
   { "Mode 19: 1920x1200",   1920, 1200 },
   { "Mode 20: 2560x1080",   2560, 1080 },
   { "Mode 21: 2560x1440",   2560, 1440 },
   { "Mode 22: 2560x1600",   2560, 1600 },
   { "Mode 23: 3440x1440",   3440, 1440 },
   { "Mode 24: 3840x2160",   3840, 2160 },
   { "Mode 25: 5120x2880",   5120, 2880 },
   { "Mode 26: 7680x4320",   7680, 4320 },
   { "Mode 27: 15360x8640", 15360, 8640 },
};

#define VID_NUM_MODES ( sizeof( vid_modes ) / sizeof( vid_modes[0] ) )

int VID_GetMode ( int width, int height )
{
   int i;

   for (i = 0; i < VID_NUM_MODES; i++)
      if ((vid_modes[i].width == width) &&
          (vid_modes[i].height == height))
         return i;

   return -1;
}

qboolean VID_GetModeInfo( int *width, int *height, int mode )
{
   if ( mode < 0 || mode >= VID_NUM_MODES )
      return false;

   *width  = vid_modes[mode].width;
   *height = vid_modes[mode].height;

   return true;
}

static void NullCallback( void *unused )
{
}

static void ResCallback( void *unused )
{
}

static void ScreenSizeCallback( void *s )
{
    menuslider_s *slider = ( menuslider_s * ) s;

    Cvar_SetValue( "viewsize", slider->curvalue * 10 );
}

static void ShadowsCallback( void *unused )
{
	Cvar_SetValue( "gl_shadows", s_shadows_slider.curvalue );
}

static void ResetDefaults( void *unused )
{
	VID_MenuInit();
}

static void ApplyChanges( void *unused )
{
#ifdef HAVE_OPENGL
   if (enable_opengl)
   {
     Cvar_Set( "vid_ref", "gl" );
     Cvar_Set( "gl_driver", "opengl32" );
   }
#endif
   M_ForceMenuOff();
}

static void CancelChanges( void *unused )
{
   M_PopMenu();
}

void    VID_Init (void)
{
   refimport_t _ri;

   viddef.width = scr_width;
   viddef.height = scr_height;

   _ri.Cmd_AddCommand = Cmd_AddCommand;
   _ri.Cmd_RemoveCommand = Cmd_RemoveCommand;
   _ri.Cmd_Argc = Cmd_Argc;
   _ri.Cmd_Argv = Cmd_Argv;
   _ri.Cmd_ExecuteText = Cbuf_ExecuteText;
   _ri.Con_Printf = VID_Printf;
   _ri.Sys_Error = VID_Error;
   _ri.FS_LoadFile = FS_LoadFile;
   _ri.FS_FreeFile = FS_FreeFile;
   _ri.FS_Gamedir = FS_Gamedir;
   _ri.Vid_NewWindow = VID_NewWindow;
   _ri.Cvar_Get = Cvar_Get;
   _ri.Cvar_Set = Cvar_Set;
   _ri.Cvar_SetValue = Cvar_SetValue;
   _ri.Vid_GetModeInfo = VID_GetModeInfo;
   _ri.Vid_MenuInit = VID_MenuInit;

   if (is_soft_render) re = SWR_GetRefAPI(_ri);
#ifdef HAVE_OPENGL
   else re = GetRefAPI(_ri);
#endif
   if (re.api_version != API_VERSION)
      Com_Error (ERR_FATAL, "Re has incompatible api_version");

   /* call the init function */
   if (re.Init (NULL, NULL) == -1)
      Com_Error (ERR_FATAL, "Couldn't start refresh");

   vid_ref = Cvar_Get ("vid_ref", "soft", CVAR_ARCHIVE);
   vid_fullscreen = Cvar_Get ("vid_fullscreen", "0", CVAR_ARCHIVE);
}

void    VID_Shutdown (void)
{
    if (re.Shutdown)
        re.Shutdown ();
}

void    VID_CheckChanges (void)
{
}

void    VID_MenuInit (void)
{
   static const char *refs[] =
   {
      "openGL",
      0
   };

#ifdef HAVE_OPENGL
   if ( !gl_driver )
      gl_driver = Cvar_Get( "gl_driver", "opengl32", 0 );
#endif
   if ( !scr_viewsize )
      scr_viewsize = Cvar_Get ("viewsize", "100", CVAR_ARCHIVE);

   s_screensize_slider.curvalue = scr_viewsize->value/10;

   s_mode_list.curvalue = VID_GetMode ( viddef.width, viddef.height );
   s_mode_list.curvalue = (s_mode_list.curvalue < 0) ? 8 : s_mode_list.curvalue;
   Cvar_SetValue( "gl_mode", s_mode_list.curvalue );

   s_ref_list.curvalue = REF_OPENGL;
   s_opengl_menu.x = viddef.width * 0.50;
   s_opengl_menu.nitems = 0;

   s_ref_list.generic.type = MTYPE_SPINCONTROL;
   s_ref_list.generic.name = "driver";
   s_ref_list.generic.x = 0;
   s_ref_list.generic.y = 0;
   s_ref_list.generic.callback = NullCallback;
   s_ref_list.itemnames = refs;

   s_cancel_action.generic.type = MTYPE_ACTION;
   s_cancel_action.generic.name = "cancel";
   s_cancel_action.generic.x    = 0;
   s_cancel_action.generic.y    = 100;
   s_cancel_action.generic.callback = CancelChanges;

   Menu_AddItem( &s_opengl_menu, ( void * ) &s_ref_list );
   Menu_AddItem( &s_opengl_menu, ( void * ) &s_cancel_action );
   Menu_Center( &s_opengl_menu );

   s_opengl_menu.x -= 8;
}

void    VID_MenuDraw (void)
{
    int w, h;
	float scale = SCR_GetMenuScale();
	
    s_current_menu = &s_opengl_menu;

    /*
    ** draw the banner
    */
    re.DrawGetPicSize( &w, &h, "m_banner_video" );
    re.DrawPic( viddef.width / 2 - (w * scale) / 2, viddef.height /2 - 110 * scale, "m_banner_video", scale );

    /*
    ** move cursor to a reasonable starting position
    */
    Menu_AdjustCursor( s_current_menu, 1 );

    /*
    ** draw the menu
    */
    Menu_Draw( s_current_menu );
}

const char *VID_MenuKey( int k)
{
   menuframework_s *m = s_current_menu;
   static const char *sound = "misc/menu1.wav";

   switch ( k )
   {
      case K_AUX4:
         ApplyChanges( NULL );
         return NULL;
      case K_KP_UPARROW:
      case K_UPARROW:
         m->cursor--;
         Menu_AdjustCursor( m, -1 );
         break;
      case K_KP_DOWNARROW:
      case K_DOWNARROW:
         m->cursor++;
         Menu_AdjustCursor( m, 1 );
         break;
      case K_KP_LEFTARROW:
      case K_LEFTARROW:
         Menu_SlideItem( m, -1 );
         break;
      case K_KP_RIGHTARROW:
      case K_RIGHTARROW:
         Menu_SlideItem( m, 1 );
         break;
      case K_KP_ENTER:
      case K_AUX1:
         if ( !Menu_SelectItem( m ) )
            ApplyChanges( NULL );
         break;
   }

   return sound;
}

#ifdef HAVE_OPENGL
int GLimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
{
	/* Resolution (scr_width, scr_height) is set directly
	 * by the libretro frontend, so checking the validity
	 * of the 'mode' argument here is irrelevant */
#if 0
	int width, height;

	ri.Con_Printf ( PRINT_ALL, " %d %d\n", scr_width, scr_height );
	ri.Con_Printf ( PRINT_ALL, "Initializing OpenGL display\n");
	ri.Con_Printf ( PRINT_ALL, "...setting mode %d:", mode );

	if ( !ri.Vid_GetModeInfo( &width, &height, mode ) )
	{
		ri.Con_Printf( PRINT_ALL, " invalid mode\n" );
		return rserr_invalid_mode;
	}
#endif

	/* destroy the existing window */
	GLimp_Shutdown ();

	*pwidth  = scr_width;
	*pheight = scr_height;
	ri.Vid_NewWindow (scr_width, scr_height);

	if (!gl_set) GLimp_InitGL();

	return rserr_ok;
}
#endif

/* input.c */

cvar_t *in_joystick;
cvar_t *leftanalog_sensitivity;
cvar_t *rightanalog_sensitivity;
cvar_t *vert_motioncam_sensitivity;
cvar_t *hor_motioncam_sensitivity;
extern cvar_t *use_gyro;
extern cvar_t *pstv_rumble;
extern cvar_t *gl_xflip;

void IN_Init (void)
{
	in_joystick	= Cvar_Get ("in_joystick", "1",	CVAR_ARCHIVE);
	leftanalog_sensitivity = Cvar_Get ("leftanalog_sensitivity", "2.0", CVAR_ARCHIVE);
	rightanalog_sensitivity = Cvar_Get ("rightanalog_sensitivity", "2.0", CVAR_ARCHIVE);
	vert_motioncam_sensitivity = Cvar_Get ("vert_motioncam_sensitivity", "2.0", CVAR_ARCHIVE);
	hor_motioncam_sensitivity = Cvar_Get ("hor_motioncam_sensitivity", "2.0", CVAR_ARCHIVE);
	use_gyro = Cvar_Get ("use_gyro", "0", CVAR_ARCHIVE);
	pstv_rumble	= Cvar_Get ("pstv_rumble", "1",	CVAR_ARCHIVE);
	
	rumble_tick = cpu_features_get_time_usec();
}

void IN_Shutdown (void)
{
}

void IN_Commands (void)
{
}

void IN_Frame (void)
{
}

void IN_StartRumble (void)
{
	if (!pstv_rumble->value) return;
	
	uint16_t strength_strong = 0xffff;
	if (!rumble.set_rumble_state)
		return;

	rumble.set_rumble_state(0, RETRO_RUMBLE_STRONG, strength_strong);
	rumble_tick = cpu_features_get_time_usec();
}

void IN_StopRumble (void)
{
	if (!rumble.set_rumble_state)
		return;

	rumble.set_rumble_state(0, RETRO_RUMBLE_STRONG, 0);
	rumble_tick = 0;
}

void IN_Move (usercmd_t *cmd)
{
   if (quake_input_device == RETRO_DEVICE_KEYBOARD)
   {
      int mx, my;
      float mx_delta;
      float my_delta;
      static int mx_prev = 0;
      static int my_prev = 0;

      /* Mouse look */
      mx = input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
      my = input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);

      mx_delta = 0.03f * ((float)sensitivity->value *
            ((float)mx + (float)mx_prev) * 0.5f) /
                  ((float)framerate / 60.0f);
      my_delta = 0.03f * ((float)sensitivity->value *
            ((float)my + (float)my_prev) * 0.5f) /
                  ((float)framerate / 60.0f);

      if (enable_opengl && gl_xflip->value)
         cl.viewangles[YAW] += mx_delta;
      else
         cl.viewangles[YAW] -= mx_delta;

      cl.viewangles[PITCH] += my_delta;

      mx_prev = mx;
      my_prev = my;
   }
   else
   {
      float speed;
      int lsx, lsy, rsx, rsy;

      if ((in_speed.state & 1) ^ (int)cl_run->value)
         speed = 2.0f;
      else
         speed = 1.0f;

      /* Left stick move */
      lsx = input_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
            RETRO_DEVICE_ID_ANALOG_X);
      lsy = input_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
            RETRO_DEVICE_ID_ANALOG_Y);

      if (lsx > analog_deadzone || lsx < -analog_deadzone)
      {
         float lx_delta;

         if (lsx > analog_deadzone)
            lsx = lsx - analog_deadzone;
         if (lsx < -analog_deadzone)
            lsx = lsx + analog_deadzone;

         lx_delta = (speed * (float)cl_sidespeed->value * (float)lsx) /
               (float)(ANALOG_RANGE - analog_deadzone);

         if (enable_opengl && gl_xflip->value)
            cmd->sidemove -= lx_delta;
         else
            cmd->sidemove += lx_delta;
      }

      if (lsy > analog_deadzone || lsy < -analog_deadzone)
      {
         if (lsy > analog_deadzone)
            lsy = lsy - analog_deadzone;
         if (lsy < -analog_deadzone)
            lsy = lsy + analog_deadzone;

         cmd->forwardmove -= (speed * (float)cl_forwardspeed->value * (float)lsy) /
               (float)(ANALOG_RANGE - analog_deadzone);
      }

      /* Right stick Look */
      rsx = input_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
               RETRO_DEVICE_ID_ANALOG_X);
      rsy = invert_y_axis * input_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
               RETRO_DEVICE_ID_ANALOG_Y);

      if (rsx > analog_deadzone || rsx < -analog_deadzone)
      {
         float rx_delta;

         if (rsx > analog_deadzone)
            rsx = rsx - analog_deadzone;
         if (rsx < -analog_deadzone)
            rsx = rsx + analog_deadzone;

         /* For now we are sharing the sensitivity with the mouse setting */
         rx_delta = ((float)sensitivity->value * (float)rsx /
               (float)(ANALOG_RANGE - analog_deadzone)) /
                     ((float)framerate / 60.0f);

         if (enable_opengl && gl_xflip->value)
            cl.viewangles[YAW] += rx_delta;
         else
            cl.viewangles[YAW] -= rx_delta;
      }

      if (rsy > analog_deadzone || rsy < -analog_deadzone)
      {
         /* Have to correct for widescreen aspect ratios,
          * otherwise vertical motion is too fast */
         float aspect_correction = (float)(4 * viddef.height) /
               (float)(3 * viddef.width);

         if (rsy > analog_deadzone)
            rsy = rsy - analog_deadzone;
         if (rsy < -analog_deadzone)
            rsy = rsy + analog_deadzone;

         cl.viewangles[PITCH] -= (aspect_correction * (float)sensitivity->value * (float)rsy /
               (float)(ANALOG_RANGE - analog_deadzone)) /
                     ((float)framerate / 60.0f);
      }
   }

   if (cl.viewangles[PITCH] > 80)
      cl.viewangles[PITCH] = 80;
   if (cl.viewangles[PITCH] < -70)
      cl.viewangles[PITCH] = -70;
}

void IN_Activate (qboolean active)
{
}

void IN_ActivateMouse (void)
{
}

void IN_DeactivateMouse (void)
{
}

void IN_MouseEvent (int mstate)
{
}
