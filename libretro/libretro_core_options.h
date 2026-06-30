#ifndef LIBRETRO_CORE_OPTIONS_H__
#define LIBRETRO_CORE_OPTIONS_H__

#include <stdlib.h>
#include <string.h>

#include <libretro.h>
#include <retro_inline.h>

#ifndef HAVE_NO_LANGEXTRA
#include "libretro_core_options_intl.h"
#endif

/*
 ********************************
 * VERSION: 2.0
 ********************************
 *
 * - 2.0: Add support for core options v2 interface
 * - 1.3: Move translations to libretro_core_options_intl.h
 *        - libretro_core_options_intl.h includes BOM and utf-8
 *          fix for MSVC 2010-2013
 *        - Added HAVE_NO_LANGEXTRA flag to disable translations
 *          on platforms/compilers without BOM support
 * - 1.2: Use core options v1 interface when
 *        RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION is >= 1
 *        (previously required RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION == 1)
 * - 1.1: Support generation of core options v0 retro_core_option_value
 *        arrays containing options with a single value
 * - 1.0: First commit
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
 ********************************
 * Core Option Definitions
 ********************************
*/

/* RETRO_LANGUAGE_ENGLISH */

/* Default language:
 * - All other languages must include the same keys and values
 * - Will be used as a fallback in the event that frontend language
 *   is not available
 * - Will be used as a fallback for any missing entries in
 *   frontend language definition */

struct retro_core_option_v2_category option_cats_us[] = {
   {
      "input",
      "Input",
      "Configure auto run, Y axis inversion, camera sensitivity, analog deadzone, rumble settings and keyboard mapping."
   },
   { NULL, NULL, NULL },
};

struct retro_core_option_v2_definition option_defs_us[] = {
   {
      "vitaquakeii_resolution",
      "Internal Resolution (Restart)",
      NULL,
      "Set the in-game rendering resolution. Higher values improve clarity at the expense of increased performance requirements.",
      NULL,
      NULL,
      {
         { "320x240",    NULL },
         { "400x240",    NULL },
         { "480x272",    NULL },
         { "512x384",    NULL },
         { "640x368",    NULL },
         { "640x480",    NULL },
         { "720x408",    NULL },
         { "800x600",    NULL },
         { "960x544",    NULL },
         { "1024x768",   NULL },
         { "1280x720",   NULL },
         { "1280x800",   NULL },
#ifdef HAVE_OPENGL
         { "1280x1024",  "1280x1024 (OpenGL Only)" },
#endif
         { "1360x768",   NULL },
         { "1366x768",   NULL },
         { "1440x900",   NULL },
         { "1600x900",   NULL },
         { "1680x1050",  NULL },
         { "1920x1080",  NULL },
         { "1920x1200",  NULL },
#ifdef HAVE_OPENGL
         { "2560x1080",  "2560x1080 (OpenGL Only)" },
         { "2560x1440",  "2560x1440 (OpenGL Only)" },
         { "2560x1600",  "2560x1600 (OpenGL Only)" },
         { "3440x1440",  "3440x1440 (OpenGL Only)" },
         { "3840x2160",  "3840x2160 (OpenGL Only)" },
         { "5120x2880",  "5120x2880 (OpenGL Only)" },
         { "7680x4320",  "7680x4320 (OpenGL Only)" },
         { "15360x8640", "15360x8640 (OpenGL Only)" },
#endif
         { NULL, NULL },
      },
#if defined(DINGUX)
      "320x240"
#elif defined(_3DS)
      "400x240"
#else
      "960x544"
#endif
   },
   {
      "vitaquakeii_framerate",
      "Framerate (Restart)",
      NULL,
      "Set internal framerate. 'Auto' will attempt to match the refresh rate of the connected display.",
      NULL,
      NULL,
      {
         { "auto", "Auto" },
         { "30",   "30 fps" },
         { "50",   "50 fps" },
         { "60",   "60 fps" },
         { "72",   "72 fps" },
         { "75",   "75 fps" },
         { "90",   "90 fps" },
         { "100",  "100 fps" },
         { "119",  "119 fps" },
         { "120",  "120 fps" },
         { "144",  "144 fps" },
         { "155",  "155 fps" },
         { "160",  "160 fps" },
         { "165",  "165 fps" },
         { "180",  "180 fps" },
         { "200",  "200 fps" },
         { "240",  "240 fps" },
         { "244",  "244 fps" },
         { "300",  "300 fps" },
         { "320",  "320 fps" },
         { "360",  "360 fps" },
         { "380",  "380 fps" },
         { "400",  "400 fps" },
         { "420",  "420 fps" },
         { "440",  "440 fps" },
         { "460",  "460 fps" },
         { "480",  "480 fps" },
         { "500",  "500 fps" },
         { "520",  "520 fps" },
         { "540",  "540 fps" },
         { "560",  "560 fps" },
         { "580",  "580 fps" },
         { "600",  "600 fps" },
         { NULL, NULL },
      },
#if defined(DINGUX) || defined(_3DS)
      "30"
#else
      "auto"
#endif
   },
#ifdef HAVE_OPENGL
   {
      "vitaquakeii_renderer",
      "Renderer (Restart)",
      NULL,
      "Choose between fast hardware-accelerated (OpenGL) rendering or the slower software-based renderer.",
      NULL,
      NULL,
      {
         { "opengl",   "OpenGL" },
         { "software", "Software" },
         { NULL, NULL },
      },
      "opengl"
   },
   {
      "vitaquakeii_gl_modulate",
      "[GL] Brightness (Restart)",
      NULL,
      "Set the overall brightness of in-game environments. (Only supported by the OpenGL renderer)",
      NULL,
      NULL,
      {
         { "1.0", NULL },
         { "1.2", NULL },
         { "1.4", NULL },
         { "1.6", NULL },
         { "1.8", NULL },
         { "2.0", NULL },
         { "2.2", NULL },
         { "2.4", NULL },
         { "2.6", NULL },
         { "2.8", NULL },
         { "3.0", NULL },
         { "3.2", NULL },
         { "3.4", NULL },
         { "3.6", NULL },
         { "3.8", NULL },
         { "4.0", NULL },
         { "4.2", NULL },
         { "4.4", NULL },
         { "4.6", NULL },
         { "4.8", NULL },
         { "5.0", NULL },
         { NULL, NULL },
      },
      "2.0"
   },
   {
      "vitaquakeii_gl_texture_filtering",
      "[GL] Texture Filtering",
      NULL,
      "Set hardware-based filtering method for in-game textures. 'Nearest' is sharp, 'Linear' is smooth. 'HQ' versions improve mipmap handling, reducing 'shimmer' in floor/ceiling textures. (Only supported by the OpenGL renderer)",
      NULL,
      NULL,
      {
         { "nearest",    "Nearest" },
         { "linear",     "Linear" },
         { "nearest_hq", "Nearest (HQ)" },
         { "linear_hq",  "Linear (HQ)" },
         { NULL, NULL },
      },
      "nearest_hq"
   },
   {
      "vitaquakeii_gl_shadows",
      "[GL] Dynamic Shadows",
      NULL,
      "Enable the casting of dynamic shadows from in-game objects. (Only supported by the OpenGL renderer)",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "vitaquakeii_gl_xflip",
      "[GL] Mirror Mode",
      NULL,
      "Mirror each level by flipping the X coordinate of the display, providing an alternative experience for veteran players. (Only supported by the OpenGL renderer)",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "vitaquakeii_gl_hud_scale",
      "[GL] HUD Scale Factor",
      NULL,
      "Adjust the scale of on-screen HUD icons/text and in-game menus. A value of '1.00' will draw elements at the size expected when running the original game at a native resolution of 320x240. A value of '0.00' will draw elements with 1:1 (pixel perfect) scaling. (Only supported by the OpenGL renderer)",
      NULL,
      NULL,
      {
         { "0.00", NULL },
         { "0.02", NULL },
         { "0.04", NULL },
         { "0.06", NULL },
         { "0.08", NULL },
         { "0.10", NULL },
         { "0.12", NULL },
         { "0.14", NULL },
         { "0.16", NULL },
         { "0.18", NULL },
         { "0.20", NULL },
         { "0.22", NULL },
         { "0.24", NULL },
         { "0.26", NULL },
         { "0.28", NULL },
         { "0.30", NULL },
         { "0.32", NULL },
         { "0.34", NULL },
         { "0.36", NULL },
         { "0.38", NULL },
         { "0.40", NULL },
         { "0.42", NULL },
         { "0.44", NULL },
         { "0.46", NULL },
         { "0.48", NULL },
         { "0.50", NULL },
         { "0.52", NULL },
         { "0.54", NULL },
         { "0.56", NULL },
         { "0.58", NULL },
         { "0.60", NULL },
         { "0.62", NULL },
         { "0.64", NULL },
         { "0.66", NULL },
         { "0.68", NULL },
         { "0.70", NULL },
         { "0.72", NULL },
         { "0.74", NULL },
         { "0.76", NULL },
         { "0.78", NULL },
         { "0.80", NULL },
         { "0.82", NULL },
         { "0.84", NULL },
         { "0.86", NULL },
         { "0.88", NULL },
         { "0.90", NULL },
         { "0.92", NULL },
         { "0.94", NULL },
         { "0.96", NULL },
         { "0.98", NULL },
         { "1.00", NULL },
         { NULL, NULL },
      },
      "0.50"
   },
#endif
   {
      "vitaquakeii_sw_dithered_filtering",
      "[SW] Dithered Filtering",
      NULL,
      "Reduce pixelation of in-game textures at the expense of increased performance requirements. (Only supported by the Software renderer)",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "vitaquakeii_hand",
      "Weapon Position",
      NULL,
      "Set the on-screen location of the currently held weapon.",
      NULL,
      NULL,
      {
         { "right",  "Right" },
         { "left",   "Left" },
         { "center", "Center" },
         { "hidden", "Hidden" },
         { NULL, NULL },
      },
      "right"
   },
   {
      "vitaquakeii_xhair",
      "Show Crosshair",
      NULL,
      "Enable an in-game crosshair to facilitate aiming.",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "cross",    "White Cross" },
         { "dot",      "Red Dot" },
         { "angle",    "Red Angle" },
         { NULL, NULL },
      },
      "cross"
   },
   {
      "vitaquakeii_cin_force43",
      "Force 4:3 for Cinematics",
      NULL,
      "When enabled, all videos (introductions, cutscenes) will be displayed at the correct aspect ratio of 4:3. If disabled, videos will be stretched to fill the screen.",
      NULL,
      NULL,
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "vitaquakeii_sound_samplerate",
      "Sound Samplerate (Hint)",
      NULL,
      "Audio output rate. The engine mixes sound effects and the music stream directly at the chosen rate, so higher rates lower latency, push aliasing above the audible range, avoid the frontend resampler's low-pass smearing, and give the resamplers finer time resolution. 'Auto' queries the frontend's target rate and snaps to the nearest supported value. A core restart is required for a change to take effect.",
      NULL,
      NULL,
      {
         { "auto",  "Auto" },
         { "32000", "32 kHz" },
         { "44100", "44 kHz" },
         { "48000", "48 kHz" },
         { "96000", "96 kHz" },
         { NULL, NULL },
      },
      "auto"
   },
#if defined(HAVE_CDAUDIO)
   {
      "vitaquakeii_cdaudio_enabled",
      "Play Music",
      NULL,
      "Enable playback of original CD audio tracks at the expense of increased performance requirements. Music must be in OGG format, placed inside the '<PAK directory>/music' folder. Files may be named either 'XX.ogg' or 'trackXX.ogg'.",
      NULL,
      NULL,
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "enabled"
   },
#endif
   {
      "vitaquakeii_cl_run",
      "Auto Run",
      NULL,
      "When enabled, the player character will run by default instead of walking.",
      NULL,
      "input",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "vitaquakeii_aimfix",
      "Accurate Aiming",
      NULL,
      "When enabled, weapons will target the exact centre of the aiming crosshair. This modifies the behaviour of the original game, where shooting is slightly inaccurate and projectiles intentionally drift.",
      NULL,
      "input",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "vitaquakeii_invert_y_axis",
      "Invert Y Axis",
      NULL,
      "Invert the Y axis of the gamepad's right analog stick. When disabled, pressing 'up' will make the player character look down (i.e. flight sim camera controls).",
      NULL,
      "input",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "vitaquakeii_mouse_sensitivity",
      "Camera Sensitivity",
      NULL,
      "Set the base speed of camera movements when using the gamepad's right analog stick or the mouse.",
      NULL,
      "input",
      {
         { "0.4", NULL },
         { "0.6", NULL },
         { "0.8", NULL },
         { "1.0", NULL },
         { "1.2", NULL },
         { "1.4", NULL },
         { "1.6", NULL },
         { "1.8", NULL },
         { "2.0", NULL },
         { "2.2", NULL },
         { "2.4", NULL },
         { "2.6", NULL },
         { "2.8", NULL },
         { "3.0", NULL },
         { "3.2", NULL },
         { "3.4", NULL },
         { "3.6", NULL },
         { "3.8", NULL },
         { "4.0", NULL },
         { "4.2", NULL },
         { "4.4", NULL },
         { "4.6", NULL },
         { "4.8", NULL },
         { "5.0", NULL },
         { NULL, NULL },
      },
      "3.0"
   },
   {
      "vitaquakeii_analog_deadzone",
      "Analog Deadzone",
      NULL,
      "Set the deadzone of the gamepad's analog sticks. May be used to eliminate controller drift.",
      NULL,
      "input",
      {
         { "0",  "0%" },
         { "3",  "3%" },
         { "5",  "5%" },
         { "7",  "7%" },
         { "10", "10%" },
         { "13", "13%" },
         { "15", "15%" },
         { "17", "17%" },
         { "20", "20%" },
         { "23", "23%" },
         { "25", "25%" },
         { "27", "27%" },
         { "30", "30%" },
         { NULL, NULL },
      },
      "15"
   },
   {
      "vitaquakeii_rumble",
      "Rumble Effects",
      NULL,
      "Enable gamepad force feedback when receiving damage.",
      NULL,
      "input",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "vitaquakeii_kb_map_up",
      "Keyboard Mapping: Forwards/Menu Up",
      NULL,
      "Set key used to move forwards and navigate upwards in menus when input device type is 'Keyboard + Mouse'.",
      NULL,
      "input",
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "vitaquakeii_kb_map_down",
      "Keyboard Mapping: Backwards/Menu Down",
      NULL,
      "Set key used to move backwards and navigate downwards in menus when input device type is 'Keyboard + Mouse'.",
      NULL,
      "input",
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "vitaquakeii_kb_map_left",
      "Keyboard Mapping: Strafe Left",
      NULL,
      "Set key used to strafe left when input device type is 'Keyboard + Mouse'.",
      NULL,
      "input",
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "vitaquakeii_kb_map_right",
      "Keyboard Mapping: Strafe Right",
      NULL,
      "Set key used to strafe right when input device type is 'Keyboard + Mouse'.",
      NULL,
      "input",
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "vitaquakeii_kb_map_menu_show",
      "Keyboard Mapping: Show Menu",
      NULL,
      "Set key used to open the menu when input device type is 'Keyboard + Mouse'.",
      NULL,
      "input",
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "vitaquakeii_kb_map_menu_select",
      "Keyboard Mapping: Menu Select",
      NULL,
      "Set key used to select menu items when input device type is 'Keyboard + Mouse'.",
      NULL,
      "input",
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "vitaquakeii_kb_map_menu_cancel",
      "Keyboard Mapping: Menu Cancel",
      NULL,
      "Set key used to cancel menu selections when input device type is 'Keyboard + Mouse'.",
      NULL,
      "input",
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "vitaquakeii_kb_map_menu_help",
      "Keyboard Mapping: Show/Hide Help Computer",
      NULL,
      "Set key used to open and close the 'help computer' when input device type is 'Keyboard + Mouse'.",
      NULL,
      "input",
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "vitaquakeii_kb_map_inventory_show",
      "Keyboard Mapping: Open/Close Inventory",
      NULL,
      "Set key used to open and close the inventory when input device type is 'Keyboard + Mouse'.",
      NULL,
      "input",
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "vitaquakeii_kb_map_inventory_prev",
      "Keyboard Mapping: Previous Inventory Item",
      NULL,
      "Set key used to select the previous inventory item when input device type is 'Keyboard + Mouse'.",
      NULL,
      "input",
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "vitaquakeii_kb_map_inventory_next",
      "Keyboard Mapping: Next Inventory Item",
      NULL,
      "Set key used to select the next inventory item when input device type is 'Keyboard + Mouse'.",
      NULL,
      "input",
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "vitaquakeii_kb_map_inventory_use",
      "Keyboard Mapping: Use Inventory Item",
      NULL,
      "Set key used to activate current inventory item when input device type is 'Keyboard + Mouse'.",
      NULL,
      "input",
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "vitaquakeii_kb_map_inventory_drop",
      "Keyboard Mapping: Drop Inventory Item",
      NULL,
      "Set key used to drop current inventory item when input device type is 'Keyboard + Mouse'.",
      NULL,
      "input",
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "vitaquakeii_kb_map_weapon_next",
      "Keyboard Mapping: Next Weapon",
      NULL,
      "Set key used to switch to the next available weapon when input device type is 'Keyboard + Mouse'.",
      NULL,
      "input",
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "vitaquakeii_kb_map_run",
      "Keyboard Mapping: Run",
      NULL,
      "Set key used to enable running when input device type is 'Keyboard + Mouse'.",
      NULL,
      "input",
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "vitaquakeii_kb_map_jump",
      "Keyboard Mapping: Jump/Climb",
      NULL,
      "Set key used to jump and climb/ascend when input device type is 'Keyboard + Mouse'.",
      NULL,
      "input",
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "vitaquakeii_kb_map_crouch",
      "Keyboard Mapping: Crouch/Descend",
      NULL,
      "Set key used to crouch and descend when input device type is 'Keyboard + Mouse'.",
      NULL,
      "input",
      {
         { NULL, NULL },
      },
      NULL
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};

struct retro_core_options_v2 options_us = {
   option_cats_us,
   option_defs_us
};

/*
 ********************************
 * Language Mapping
 ********************************
*/

#ifndef HAVE_NO_LANGEXTRA
struct retro_core_options_v2 *options_intl[RETRO_LANGUAGE_LAST] = {
   &options_us, /* RETRO_LANGUAGE_ENGLISH */
   NULL,        /* RETRO_LANGUAGE_JAPANESE */
   NULL,        /* RETRO_LANGUAGE_FRENCH */
   NULL,        /* RETRO_LANGUAGE_SPANISH */
   NULL,        /* RETRO_LANGUAGE_GERMAN */
   NULL,        /* RETRO_LANGUAGE_ITALIAN */
   NULL,        /* RETRO_LANGUAGE_DUTCH */
   NULL,        /* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */
   NULL,        /* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */
   NULL,        /* RETRO_LANGUAGE_RUSSIAN */
   NULL,        /* RETRO_LANGUAGE_KOREAN */
   NULL,        /* RETRO_LANGUAGE_CHINESE_TRADITIONAL */
   NULL,        /* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */
   NULL,        /* RETRO_LANGUAGE_ESPERANTO */
   NULL,        /* RETRO_LANGUAGE_POLISH */
   NULL,        /* RETRO_LANGUAGE_VIETNAMESE */
   NULL,        /* RETRO_LANGUAGE_ARABIC */
   NULL,        /* RETRO_LANGUAGE_GREEK */
   NULL,        /* RETRO_LANGUAGE_TURKISH */
   NULL,        /* RETRO_LANGUAGE_SLOVAK */
   NULL,        /* RETRO_LANGUAGE_PERSIAN */
   NULL,        /* RETRO_LANGUAGE_HEBREW */
   NULL,        /* RETRO_LANGUAGE_ASTURIAN */
   NULL,        /* RETRO_LANGUAGE_FINNISH */
};
#endif

/*
 ********************************
 * Functions
 ********************************
*/

/* Handles configuration/setting of core options.
 * Should be called as early as possible - ideally inside
 * retro_set_environment(), and no later than retro_load_game()
 * > We place the function body in the header to avoid the
 *   necessity of adding more .c files (i.e. want this to
 *   be as painless as possible for core devs)
 */

static INLINE void libretro_set_core_options(retro_environment_t environ_cb,
      bool *categories_supported)
{
   unsigned version  = 0;
#ifndef HAVE_NO_LANGEXTRA
   unsigned language = 0;
#endif

   if (!environ_cb || !categories_supported)
      return;

   *categories_supported = false;

   if (!environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version))
      version = 0;

   if (version >= 2)
   {
#ifndef HAVE_NO_LANGEXTRA
      struct retro_core_options_v2_intl core_options_intl;

      core_options_intl.us    = &options_us;
      core_options_intl.local = NULL;

      if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
          (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH))
         core_options_intl.local = options_intl[language];

      *categories_supported = environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL,
            &core_options_intl);
#else
      *categories_supported = environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2,
            &options_us);
#endif
   }
   else
   {
      size_t i, j;
      size_t option_index              = 0;
      size_t num_options               = 0;
      struct retro_core_option_definition
            *option_v1_defs_us         = NULL;
#ifndef HAVE_NO_LANGEXTRA
      size_t num_options_intl          = 0;
      struct retro_core_option_v2_definition
            *option_defs_intl          = NULL;
      struct retro_core_option_definition
            *option_v1_defs_intl       = NULL;
      struct retro_core_options_intl
            core_options_v1_intl;
#endif
      struct retro_variable *variables = NULL;
      char **values_buf                = NULL;

      /* Determine total number of options */
      while (true)
      {
         if (option_defs_us[num_options].key)
            num_options++;
         else
            break;
      }

      if (version >= 1)
      {
         /* Allocate US array */
         option_v1_defs_us = (struct retro_core_option_definition *)
               calloc(num_options + 1, sizeof(struct retro_core_option_definition));

         /* Copy parameters from option_defs_us array */
         for (i = 0; i < num_options; i++)
         {
            struct retro_core_option_v2_definition *option_def_us = &option_defs_us[i];
            struct retro_core_option_value *option_values         = option_def_us->values;
            struct retro_core_option_definition *option_v1_def_us = &option_v1_defs_us[i];
            struct retro_core_option_value *option_v1_values      = option_v1_def_us->values;

            option_v1_def_us->key           = option_def_us->key;
            option_v1_def_us->desc          = option_def_us->desc;
            option_v1_def_us->info          = option_def_us->info;
            option_v1_def_us->default_value = option_def_us->default_value;

            /* Values must be copied individually... */
            while (option_values->value)
            {
               option_v1_values->value = option_values->value;
               option_v1_values->label = option_values->label;

               option_values++;
               option_v1_values++;
            }
         }

#ifndef HAVE_NO_LANGEXTRA
         if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
             (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH) &&
             options_intl[language])
            option_defs_intl = options_intl[language]->definitions;

         if (option_defs_intl)
         {
            /* Determine number of intl options */
            while (true)
            {
               if (option_defs_intl[num_options_intl].key)
                  num_options_intl++;
               else
                  break;
            }

            /* Allocate intl array */
            option_v1_defs_intl = (struct retro_core_option_definition *)
                  calloc(num_options_intl + 1, sizeof(struct retro_core_option_definition));

            /* Copy parameters from option_defs_intl array */
            for (i = 0; i < num_options_intl; i++)
            {
               struct retro_core_option_v2_definition *option_def_intl = &option_defs_intl[i];
               struct retro_core_option_value *option_values           = option_def_intl->values;
               struct retro_core_option_definition *option_v1_def_intl = &option_v1_defs_intl[i];
               struct retro_core_option_value *option_v1_values        = option_v1_def_intl->values;

               option_v1_def_intl->key           = option_def_intl->key;
               option_v1_def_intl->desc          = option_def_intl->desc;
               option_v1_def_intl->info          = option_def_intl->info;
               option_v1_def_intl->default_value = option_def_intl->default_value;

               /* Values must be copied individually... */
               while (option_values->value)
               {
                  option_v1_values->value = option_values->value;
                  option_v1_values->label = option_values->label;

                  option_values++;
                  option_v1_values++;
               }
            }
         }

         core_options_v1_intl.us    = option_v1_defs_us;
         core_options_v1_intl.local = option_v1_defs_intl;

         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL, &core_options_v1_intl);
#else
         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, option_v1_defs_us);
#endif
      }
      else
      {
         /* Allocate arrays */
         variables  = (struct retro_variable *)calloc(num_options + 1,
               sizeof(struct retro_variable));
         values_buf = (char **)calloc(num_options, sizeof(char *));

         if (!variables || !values_buf)
            goto error;

         /* Copy parameters from option_defs_us array */
         for (i = 0; i < num_options; i++)
         {
            const char *key                        = option_defs_us[i].key;
            const char *desc                       = option_defs_us[i].desc;
            const char *default_value              = option_defs_us[i].default_value;
            struct retro_core_option_value *values = option_defs_us[i].values;
            size_t buf_len                         = 3;
            size_t default_index                   = 0;

            values_buf[i] = NULL;

            if (desc)
            {
               size_t num_values = 0;

               /* Determine number of values */
               while (true)
               {
                  if (values[num_values].value)
                  {
                     /* Check if this is the default value */
                     if (default_value)
                        if (strcmp(values[num_values].value, default_value) == 0)
                           default_index = num_values;

                     buf_len += strlen(values[num_values].value);
                     num_values++;
                  }
                  else
                     break;
               }

               /* Build values string */
               if (num_values > 0)
               {
                  buf_len += num_values - 1;
                  buf_len += strlen(desc);

                  values_buf[i] = (char *)calloc(buf_len, sizeof(char));
                  if (!values_buf[i])
                     goto error;

                  strcpy(values_buf[i], desc);
                  strcat(values_buf[i], "; ");

                  /* Default value goes first */
                  strcat(values_buf[i], values[default_index].value);

                  /* Add remaining values */
                  for (j = 0; j < num_values; j++)
                  {
                     if (j != default_index)
                     {
                        strcat(values_buf[i], "|");
                        strcat(values_buf[i], values[j].value);
                     }
                  }
               }
            }

            variables[option_index].key   = key;
            variables[option_index].value = values_buf[i];
            option_index++;
         }

         /* Set variables */
         environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
      }

error:
      /* Clean up */

      if (option_v1_defs_us)
      {
         free(option_v1_defs_us);
         option_v1_defs_us = NULL;
      }

#ifndef HAVE_NO_LANGEXTRA
      if (option_v1_defs_intl)
      {
         free(option_v1_defs_intl);
         option_v1_defs_intl = NULL;
      }
#endif

      if (values_buf)
      {
         for (i = 0; i < num_options; i++)
         {
            if (values_buf[i])
            {
               free(values_buf[i]);
               values_buf[i] = NULL;
            }
         }

         free(values_buf);
         values_buf = NULL;
      }

      if (variables)
      {
         free(variables);
         variables = NULL;
      }
   }
}

#ifdef __cplusplus
}
#endif

#endif
