# vitaQuake II (libretro)

A libretro core for **Quake II** and its expansions, based on the vitaQuake II
port by Rinnegatamante and running the original id Tech 2 engine (id Software,
GPLv2). The core runs inside RetroArch and other libretro frontends on a wide
range of desktop, mobile, console and embedded platforms.

One source tree builds four core variants, each with the corresponding game
logic statically linked (no game DLLs required):

| Core                          | Game                                    | Data directory |
|-------------------------------|-----------------------------------------|----------------|
| `vitaquake2_libretro`         | Quake II                                | `baseq2`       |
| `vitaquake2-xatrix_libretro`  | Quake II — The Reckoning                | `xatrix`       |
| `vitaquake2-rogue_libretro`   | Quake II — Ground Zero                  | `rogue`        |
| `vitaquake2-zaero_libretro`   | Quake II — Zaero (unofficial expansion) | `zaero`        |


## Loading content

Load a `.pak` file (e.g. `pak0.pak`) located inside the game data directory
for the chosen core variant, for example:

```
quake2/baseq2/pak0.pak    ->  vitaquake2_libretro
quake2/xatrix/pak0.pak    ->  vitaquake2-xatrix_libretro
```

The expansion cores automatically launch with the correct `game` directory
set, and validate that the expected game files are present. Shareware and
registered data are both supported, and the original `.cin` full-motion
cinematics play back with audio.


## Renderer backends

### OpenGL (hardware)

- Uses the frontend's OpenGL hardware context (desktop GL or OpenGL ES,
  including GLES builds for mobile/embedded targets), with XRGB8888 output.
- Internal resolutions from 320x240 up to 15360x8640 (the highest settings
  are OpenGL-only).
- Texture filtering: Nearest, Linear, plus HQ variants of both with improved
  mipmap handling to reduce shimmer on floors and ceilings.
- Adjustable environment brightness (`gl_modulate`, 1.0-5.0).
- Optional dynamic shadows cast by in-game objects.
- HUD / menu scaling, from pixel-perfect 1:1 up to sizes matching the
  original 320x240 presentation.
- X-flip mode: mirrors each level horizontally for an alternative playthrough
  of familiar maps.

### Software

- The classic 8-bit palettized Quake II rasterizer with RGB565 output.
- Zero-copy presentation: renders directly into the frontend's framebuffer
  when the frontend supports it, skipping an intermediate copy.
- **Colored lighting**: uses the map's 24-bit RGB lightmaps to tint the
  paletted output via an inverse-palette lookup (the original software
  renderer collapsed lighting to monochrome intensity).
- **Truecolor sky**: composites a high-colour RGB565 sky overlay over the
  paletted frame when truecolor `env/*.tga` skybox textures are present.
- Optional dithered texture filtering to reduce pixelation.


## Timing

- Selectable internal framerate from 30 to 600 fps, or **Auto**, which
  matches the refresh rate reported by the connected display/frontend.


## Audio

- Sound is mixed directly at a selectable output rate — 32 / 44.1 / 48 /
  96 kHz — or **Auto**, which queries the frontend's preferred rate and snaps
  to the nearest supported value (bypassing the frontend resampler for lower
  latency and less filtering).
- Float PCM output is negotiated with frontends that support it, with a
  transparent fallback to the standard 16-bit path.
- **CD soundtrack replacement**: OGG Vorbis rips placed in a `music` folder
  next to the pak files (named `XX.ogg` or `trackXX.ogg`) play as the level
  soundtrack. Decoding uses the fixed-point Tremor decoder for identical
  output on every platform and FPU. Music playback can be toggled via the
  "Play Music" core option.
- Optional forced 4:3 aspect ratio for cinematics (instead of stretching to
  fill the screen).


## Input

Two selectable input device types:

- **Analog Gamepad**: dual-analog FPS controls (move on left stick, camera on
  right stick) with a full set of mapped actions.
- **Keyboard + Mouse**: mouse look with left-click fire, backquote/tilde
  console toggle, and seventeen fully remappable action keys (movement,
  jump, crouch, run, menu navigation, help computer, inventory management,
  weapon cycling) configurable through core options.

Additional input features:

- Adjustable camera sensitivity (applies to right stick and mouse).
- Configurable analog stick deadzone to eliminate controller drift.
- Invert Y axis option.
- Auto Run toggle.
- Rumble: force feedback on the strong motor when taking damage.
- Weapon position option: Right, Left, Center or Hidden.
- Selectable crosshair: White Cross, Red Dot or Red Angle.
- **Accurate Aiming** option: makes weapons hit the exact centre of the
  crosshair, removing the original game's intentional projectile drift.


## Saves

- The native Quake II save/load system is fully supported; savegames are
  written to the frontend's save directory per game.
- Libretro savestates are not currently implemented.


## Multiplayer

Networking in this port is loopback-only (the internal client/server
architecture of the engine); online/LAN multiplayer is not available.


## Platforms

Builds for Windows, Linux, macOS, iOS/tvOS, Android, Emscripten (web),
PlayStation Vita, Nintendo Switch, Wii U, 3DS, PS3, GCW-Zero, webOS, QNX,
Rockchip boards and other libretro targets. Hardware OpenGL support is
enabled per-platform; software rendering is available everywhere.


## Building

```
make                     # Quake II (baseq2) core
make basegame=xatrix     # The Reckoning core
make basegame=rogue      # Ground Zero core
make basegame=zaero      # Zaero core
```

Cross builds follow the usual libretro conventions, e.g.
`make platform=vita`, `make platform=libnx`.


## Credits and license

- Quake II is Copyright (C) 1997-2001 Id Software, Inc.
- The Reckoning game code by Xatrix Entertainment; Ground Zero game code by
  Rogue Entertainment; Zaero game code by Team Evolve.
- vitaQuake II port by Rinnegatamante.
- libretro core by the libretro team and contributors.

Released under the GNU General Public License, version 2; see `LICENSE`.
