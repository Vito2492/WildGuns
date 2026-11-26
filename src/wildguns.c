// Wild Guns: Frontier Edition — native cross-platform frontend for
// Wild Guns (USA, 1994, Natsume, SNES).
// Runs the player's own legally dumped ROM through snes9x_libretro, extracts
// the Title Screen logo backdrop and authentic 2bpp Western 8x8 font from that
// ROM at startup, and adds Fast Start (1P & 2P Co-op), Classic Mode (full
// original Natsume logo / intro / title boot with all mods active), official
// Stage Select launcher, Versus Mode launcher, optional Mouse & Direct-Touch
// Crosshair Aiming, Auto-Fire, Live Weapon Selector (Shotgun / Grenade /
// Machine Gun / Vulcan), God Mode, Infinite Lives/Bombs/Ammo (P1 & P2),
// Freeze Timer, No Slowdown, 3 Save State slots, 2-Player Local Support,
// smart Android multi-touch controls, accessible menus, and ES/EN UI.
// The ROM file on disk is never patched or redistributed.

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "libretro.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#define CORE_NAME "snes9x_libretro.dll"
#define PATH_SEP '\\'
#elif defined(__ANDROID__)
#define CORE_NAME "libsnes9x_libretro.so"
#define PATH_SEP '/'
#else
#define CORE_NAME "snes9x_libretro.so"
#define PATH_SEP '/'
#endif

#define W 256
#define H 224
#define ROM_SIZE 0x100000u
#define ROM_CRC  0x085D8D14u

/* ------------------------------------------------------------------ */
/* Verified Wild Guns (USA) WRAM Addresses (7E:xxxx)                    */
/* ------------------------------------------------------------------ */
#define RAM_CAM_X       0x0090u  // Camera X scroll offset (16-bit LE)
#define RAM_TIMER       0x0244u  // Stage countdown timer (seconds)
#define RAM_P1_CROSS_X  0x1609u  // P1 Crosshair world X (screen_x + cam_x)
#define RAM_P1_CROSS_Y  0x1709u  // P1 Crosshair screen Y
#define RAM_P1_INVULN   0x1C82u  // P1 Invulnerability timer
#define RAM_P2_INVULN   0x1C86u  // P2 Invulnerability timer
#define RAM_P1_BOMBS    0x1FA0u  // P1 Bombs (0..5)
#define RAM_P1_HUD_DIRTY 0x1FA2u // P1 HUD refresh flags (bit2=bombs, bit3=lives)
#define RAM_P2_BOMBS    0x1FA4u  // P2 Bombs
#define RAM_P2_HUD_DIRTY 0x1FA6u // P2 HUD refresh flags
#define RAM_P1_WEAPON   0x1FA8u  // P1 Weapon ID (0=Norm, 4=S.Gun, 8=G.Gun, 12=M.Gun, 16=V.Gun)
#define RAM_P2_WEAPON   0x1FACu  // P2 Weapon ID
#define RAM_P1_AMMO     0x1FB0u  // P1 Special Weapon Ammo (BCD/count)
#define RAM_P1_LIVES    0x1FB2u  // P1 Lives (0..9)
#define RAM_P2_AMMO     0x1FB4u  // P2 Special Weapon Ammo
#define RAM_P2_LIVES    0x1FB6u  // P2 Lives (0..9)

/* ------------------------------------------------------------------ */
/* Libretro Core                                                        */
/* ------------------------------------------------------------------ */

static struct {
    void (*init)(void), (*deinit)(void), (*run)(void), (*reset)(void);
    void (*set_environment)(retro_environment_t);
    void (*set_video_refresh)(retro_video_refresh_t);
    void (*set_audio_sample)(retro_audio_sample_t);
    void (*set_audio_sample_batch)(retro_audio_sample_batch_t);
    void (*set_input_poll)(retro_input_poll_t);
    void (*set_input_state)(retro_input_state_t);
    bool (*load_game)(const struct retro_game_info *);
    void (*get_av)(struct retro_system_av_info *);
    void *(*mem_data)(unsigned);
    size_t (*mem_size)(unsigned);
    size_t (*ser_size)(void);
    bool (*serialize)(void *, size_t);
    bool (*unserialize)(const void *, size_t);
} core;

static enum retro_pixel_format pixfmt = RETRO_PIXEL_FORMAT_0RGB1555;
static uint32_t game[512 * 512];
static int gw = W, gh = H;
static uint16_t pad[2];
static bool overclock;
static bool scripting;
static bool vars_dirty;
static SDL_AudioDeviceID audio;
static bool mute;
static char base_dir[1024];
static char save_dir[1024];

static bool env_cb(unsigned cmd, void *data)
{
    switch (cmd & 0xffff) {
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        pixfmt = *(enum retro_pixel_format *)data;
        return true;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *(const char **)data = save_dir;
        return true;
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        *(bool *)data = true;
        return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE: {
        struct retro_variable *v = (struct retro_variable *)data;
        if (!strcmp(v->key, "snes9x_overclock_cycles")) {
            v->value = overclock && !scripting ? "compatible" : "disabled";
            return true;
        }
        return false;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        *(bool *)data = vars_dirty;
        vars_dirty = false;
        return true;
    }
    return false;
}

static void video_cb(const void *data, unsigned w, unsigned h, size_t pitch)
{
    if (!data || w > 512 || h > 512) return;
    gw = (int)w; gh = (int)h;
    for (unsigned y = 0; y < h; y++) {
        const uint8_t *row = (const uint8_t *)data + y * pitch;
        uint32_t *out = game + y * w;
        for (unsigned x = 0; x < w; x++) {
            uint32_t r, g, b;
            if (pixfmt == RETRO_PIXEL_FORMAT_XRGB8888) {
                out[x] = 0xff000000u | ((const uint32_t *)row)[x];
                continue;
            }
            uint16_t p = ((const uint16_t *)row)[x];
            if (pixfmt == RETRO_PIXEL_FORMAT_RGB565) {
                r = p >> 11; g = (p >> 5) & 63; b = p & 31;
                r = (r << 3) | (r >> 2); g = (g << 2) | (g >> 4); b = (b << 3) | (b >> 2);
            } else {
                r = (p >> 10) & 31; g = (p >> 5) & 31; b = p & 31;
                r = (r << 3) | (r >> 2); g = (g << 3) | (g >> 2); b = (b << 3) | (b >> 2);
            }
            out[x] = 0xff000000u | (r << 16) | (g << 8) | b;
        }
    }
}

static size_t audio_batch_cb(const int16_t *d, size_t frames)
{
    if (audio && !mute) SDL_QueueAudio(audio, d, (Uint32)(frames * 4));
    return frames;
}
static void audio_cb(int16_t l, int16_t r) { int16_t s[2] = { l, r }; audio_batch_cb(s, 1); }
static void poll_cb(void) {}
static int16_t input_cb(unsigned port, unsigned device, unsigned index, unsigned id)
{
    (void)index;
    if (port >= 2 || device != RETRO_DEVICE_JOYPAD) return 0;
    if (id == RETRO_DEVICE_ID_JOYPAD_MASK) return (int16_t)pad[port];
    return id < 16 ? (int16_t)((pad[port] >> id) & 1) : 0;
}

static uint8_t *wram(void) { return (uint8_t *)core.mem_data(RETRO_MEMORY_SYSTEM_RAM); }

static bool load_core(const char *path)
{
    void *so = SDL_LoadObject(path);
    if (!so) return false;
#define SYM(field, name) if (!(*(void **)&core.field = SDL_LoadFunction(so, name))) return false
    SYM(init, "retro_init"); SYM(deinit, "retro_deinit");
    SYM(run, "retro_run"); SYM(reset, "retro_reset");
    SYM(set_environment, "retro_set_environment");
    SYM(set_video_refresh, "retro_set_video_refresh");
    SYM(set_audio_sample, "retro_set_audio_sample");
    SYM(set_audio_sample_batch, "retro_set_audio_sample_batch");
    SYM(set_input_poll, "retro_set_input_poll");
    SYM(set_input_state, "retro_set_input_state");
    SYM(load_game, "retro_load_game");
    SYM(get_av, "retro_get_system_av_info");
    SYM(mem_data, "retro_get_memory_data");
    SYM(mem_size, "retro_get_memory_size");
    SYM(ser_size, "retro_serialize_size");
    SYM(serialize, "retro_serialize");
    SYM(unserialize, "retro_unserialize");
#undef SYM
    core.set_environment(env_cb);
    core.set_video_refresh(video_cb);
    core.set_audio_sample(audio_cb);
    core.set_audio_sample_batch(audio_batch_cb);
    core.set_input_poll(poll_cb);
    core.set_input_state(input_cb);
    core.init();
    return true;
}

static void step2(int frames, uint16_t p0, uint16_t p1)
{
    pad[0] = p0; pad[1] = p1;
    for (int i = 0; i < frames; i++) core.run();
}
static void tap1(int btn, int hold, int wait) { step2(hold, (uint16_t)(1u << btn), 0); step2(wait, 0, 0); }

/* ------------------------------------------------------------------ */
/* ROM Validation & Native File Picker                                  */
/* ------------------------------------------------------------------ */

static uint32_t crc32_buf(const uint8_t *p, size_t n)
{
    uint32_t c = 0xffffffffu;
    for (size_t i = 0; i < n; i++) {
        c ^= p[i];
        for (int b = 0; b < 8; b++) c = (c >> 1) ^ (0xedb88320u & -(int)(c & 1u));
    }
    return ~c;
}

static uint8_t *rom_buf;
static size_t rom_len;
static char rom_path[1024];

static bool load_rom_file(const char *path, char *err, size_t errsz)
{
    FILE *f = fopen(path, "rb");
    if (!f) { snprintf(err, errsz, "No se pudo abrir el archivo"); return false; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < (long)ROM_SIZE || sz > (long)(ROM_SIZE + 512)) {
        fclose(f); snprintf(err, errsz, "Tamano invalido (esperado 1 MB)"); return false;
    }
    uint8_t *tmp = (uint8_t *)malloc((size_t)sz);
    if (!tmp || fread(tmp, 1, (size_t)sz, f) != (size_t)sz) {
        free(tmp); fclose(f); snprintf(err, errsz, "Error leyendo archivo"); return false;
    }
    fclose(f);
    size_t skip = (sz == (long)(ROM_SIZE + 512)) ? 512u : 0u;
    const uint8_t *raw = tmp + skip;
    bool hdr_ok = (memcmp(raw + 0x7FC0, "WILD GUNS", 9) == 0);
    uint32_t crc = crc32_buf(raw, ROM_SIZE);
    if (!hdr_ok && crc != ROM_CRC) {
        free(tmp); snprintf(err, errsz, "No es Wild Guns (USA) SNES"); return false;
    }
    uint8_t *clean = (uint8_t *)malloc(ROM_SIZE);
    if (!clean) { free(tmp); return false; }
    memcpy(clean, raw, ROM_SIZE);
    free(tmp); free(rom_buf);
    rom_buf = clean; rom_len = ROM_SIZE;
    snprintf(rom_path, sizeof rom_path, "%s", path);
    return true;
}

static bool pick_rom_dialog(char *out, size_t outsz)
{
#ifdef _WIN32
    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof ofn);
    out[0] = 0;
    ofn.lStructSize = sizeof ofn;
    ofn.lpstrFilter = "SNES ROM (*.sfc;*.smc)\0*.sfc;*.smc\0Todos (*.*)\0*.*\0";
    ofn.lpstrFile = out;
    ofn.nMaxFile = (DWORD)outsz;
    ofn.lpstrTitle = "Seleccionar ROM de Wild Guns (USA)";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    return GetOpenFileNameA(&ofn) != 0;
#elif defined(__ANDROID__)
    (void)out; (void)outsz;
    return false;
#else
    const char *cmds[] = {
        "zenity --file-selection --title='Seleccionar ROM de Wild Guns (USA)' --file-filter='SNES ROM | *.sfc *.smc' 2>/dev/null",
        "kdialog --getopenfilename . '*.sfc *.smc|SNES ROM' 2>/dev/null"
    };
    for (int i = 0; i < 2; i++) {
        FILE *p = popen(cmds[i], "r");
        if (!p) continue;
        out[0] = 0;
        if (fgets(out, (int)outsz, p)) {
            size_t l = strlen(out);
            while (l && (out[l - 1] == '\n' || out[l - 1] == '\r')) out[--l] = 0;
        }
        int rc = pclose(p);
        if (rc == 0 && out[0]) return true;
    }
    return false;
#endif
}

/* ------------------------------------------------------------------ */
/* Config & Persistence                                                 */
/* ------------------------------------------------------------------ */

static const int snes_btn[12] = {
    RETRO_DEVICE_ID_JOYPAD_UP,    RETRO_DEVICE_ID_JOYPAD_DOWN,
    RETRO_DEVICE_ID_JOYPAD_LEFT,  RETRO_DEVICE_ID_JOYPAD_RIGHT,
    RETRO_DEVICE_ID_JOYPAD_Y,     RETRO_DEVICE_ID_JOYPAD_B,
    RETRO_DEVICE_ID_JOYPAD_X,     RETRO_DEVICE_ID_JOYPAD_A,
    RETRO_DEVICE_ID_JOYPAD_L,     RETRO_DEVICE_ID_JOYPAD_R,
    RETRO_DEVICE_ID_JOYPAD_START, RETRO_DEVICE_ID_JOYPAD_SELECT
};

static const char *btn_es[12] = {
    "ARRIBA", "ABAJO", "IZQUIERDA", "DERECHA",
    "DISPARAR (Y)", "SALTO/ROD (B)", "BOMBA (X)", "LAZO (A)",
    "L", "R", "START", "SELECT"
};
static const char *btn_en[12] = {
    "UP", "DOWN", "LEFT", "RIGHT",
    "FIRE (Y)", "JUMP/ROLL (B)", "BOMB (X)", "LASSO (A)",
    "L", "R", "START", "SELECT"
};

typedef struct {
    int character;     // 0=Clint, 1=Annie (for Fast Start 1P)
    int mouse_aim;     // 0=Off, 1=On (PC mouse / Android direct-touch aim!)
    int autofire;      // 0=Off, 1=On
    int scale;         // 0=Pixel, 1=Smooth
    int fullscreen;    // 0/1
    int noslow;        // 0/1
    int touch;         // 0=Auto, 1=On, 2=Off
    int lang;          // 0=ES, 1=EN
    int slot;          // 0..2
    // Live cheats (apply to both P1 & P2 in co-op!)
    int c_god;         // Invulnerability
    int c_lives;       // Infinite Lives (9)
    int c_bombs;       // Infinite Bombs (5)
    int c_ammo;        // Infinite Special Ammo
    int c_weapon;      // 0=Original, 1=S.Gun, 2=G.Gun, 3=M.Gun, 4=V.Gun
    int c_timer;       // Freeze Timer
    int kb[12];
    int gp[12];
    char last_rom[1024];
} cfg_t;

static cfg_t cfg;

static void default_controls(void)
{
    int dk[12] = {
        SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT,
        SDL_SCANCODE_Z,  SDL_SCANCODE_X,    SDL_SCANCODE_C,    SDL_SCANCODE_A,
        SDL_SCANCODE_Q,  SDL_SCANCODE_W,    SDL_SCANCODE_RETURN, SDL_SCANCODE_RSHIFT
    };
    int dg[12] = {
        SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
        SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
        SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_A,
        SDL_CONTROLLER_BUTTON_Y, SDL_CONTROLLER_BUTTON_B,
        SDL_CONTROLLER_BUTTON_LEFTSHOULDER, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
        SDL_CONTROLLER_BUTTON_START, SDL_CONTROLLER_BUTTON_BACK
    };
    memcpy(cfg.kb, dk, sizeof dk);
    memcpy(cfg.gp, dg, sizeof dg);
}

static void cfg_path(char *out, size_t n) { snprintf(out, n, "%s%cwildguns.cfg", save_dir, PATH_SEP); }

static void load_cfg(void)
{
    memset(&cfg, 0, sizeof cfg);
    cfg.mouse_aim = 1; // enabled by default for awesome PC/Android light-gun feel (toggleable!)
    default_controls();
    char p[1024]; cfg_path(p, sizeof p);
    FILE *f = fopen(p, "r");
    if (!f) return;
    char line[1200];
    while (fgets(line, sizeof line, f)) {
        size_t l = strlen(line);
        while (l && (line[l - 1] == '\n' || line[l - 1] == '\r')) line[--l] = 0;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq++ = 0;
        int v = atoi(eq), idx;
        if (!strcmp(line, "character")) cfg.character = v & 1;
        else if (!strcmp(line, "mouse_aim")) cfg.mouse_aim = v != 0;
        else if (!strcmp(line, "autofire")) cfg.autofire = v != 0;
        else if (!strcmp(line, "scale")) cfg.scale = v != 0;
        else if (!strcmp(line, "fullscreen")) cfg.fullscreen = v != 0;
        else if (!strcmp(line, "noslow")) cfg.noslow = v != 0;
        else if (!strcmp(line, "touch")) cfg.touch = v % 3;
        else if (!strcmp(line, "lang")) cfg.lang = v != 0;
        else if (!strcmp(line, "slot")) cfg.slot = v % 3;
        else if (!strcmp(line, "c_god")) cfg.c_god = v != 0;
        else if (!strcmp(line, "c_lives")) cfg.c_lives = v != 0;
        else if (!strcmp(line, "c_bombs")) cfg.c_bombs = v != 0;
        else if (!strcmp(line, "c_ammo")) cfg.c_ammo = v != 0;
        else if (!strcmp(line, "c_weapon")) cfg.c_weapon = v % 5;
        else if (!strcmp(line, "c_timer")) cfg.c_timer = v != 0;
        else if (!strcmp(line, "rom")) snprintf(cfg.last_rom, sizeof cfg.last_rom, "%s", eq);
        else if (sscanf(line, "kb%d", &idx) == 1 && idx >= 0 && idx < 12) cfg.kb[idx] = v;
        else if (sscanf(line, "gp%d", &idx) == 1 && idx >= 0 && idx < 12) cfg.gp[idx] = v;
    }
    fclose(f);
    overclock = cfg.noslow != 0;
}

static void save_cfg(void)
{
    char p[1024]; cfg_path(p, sizeof p);
    FILE *f = fopen(p, "w");
    if (!f) return;
    fprintf(f, "character=%d\nmouse_aim=%d\nautofire=%d\nscale=%d\nfullscreen=%d\nnoslow=%d\ntouch=%d\nlang=%d\nslot=%d\n",
            cfg.character, cfg.mouse_aim, cfg.autofire, cfg.scale, cfg.fullscreen, cfg.noslow, cfg.touch, cfg.lang, cfg.slot);
    fprintf(f, "c_god=%d\nc_lives=%d\nc_bombs=%d\nc_ammo=%d\nc_weapon=%d\nc_timer=%d\n",
            cfg.c_god, cfg.c_lives, cfg.c_bombs, cfg.c_ammo, cfg.c_weapon, cfg.c_timer);
    if (rom_path[0]) fprintf(f, "rom=%s\n", rom_path);
    for (int i = 0; i < 12; i++) fprintf(f, "kb%d=%d\ngp%d=%d\n", i, cfg.kb[i], i, cfg.gp[i]);
    fclose(f);
}

/* ------------------------------------------------------------------ */
/* ROM-Extracted Western 8x8 Font & Title Logo Backdrop                 */
/* ------------------------------------------------------------------ */

static uint32_t bg_art[W * H];
static uint8_t  glyph[128][8 * 10]; // 2bpp 8x16 Western font sampled to crisp 8x10 (VRAM 0xE200)
static uint8_t  cursor_tile[8 * 10]; // '>' cursor tile extracted from VRAM (0xEAC0/0xEBC0)
static bool     has_font;

static void decode_2bpp_8x16_to_10(const uint8_t *top_tb, const uint8_t *bot_tb, uint8_t out[8 * 10])
{
    uint8_t full[16][8];
    for (int r = 0; r < 8; r++) {
        uint8_t tp0 = top_tb[r * 2], tp1 = top_tb[r * 2 + 1];
        uint8_t bp0 = bot_tb[r * 2], bp1 = bot_tb[r * 2 + 1];
        for (int c = 0; c < 8; c++) {
            int bit = 7 - c;
            full[r][c]     = (uint8_t)(((tp0 >> bit) & 1) | (((tp1 >> bit) & 1) << 1));
            full[r + 8][c] = (uint8_t)(((bp0 >> bit) & 1) | (((bp1 >> bit) & 1) << 1));
        }
    }
    static const int rows[10] = { 1, 2, 4, 6, 7, 9, 11, 12, 14, 15 };
    for (int r = 0; r < 10; r++)
        for (int c = 0; c < 8; c++)
            out[r * 8 + c] = full[rows[r]][c];
}

static void extract_font_from_opt_vram(const uint8_t *snap, size_t snap_sz)
{
    const uint8_t *vram = NULL;
    for (size_t i = 0; i + 11 + 65536 <= snap_sz; i++) {
        if (snap[i] == 'V' && snap[i+1] == 'R' && snap[i+2] == 'A' && snap[i+3] == ':') {
            vram = snap + i + 11;
            break;
        }
    }
    if (!vram) return;
    memset(glyph, 0, sizeof glyph);
    // Verified 2bpp 8x16 Western ASCII font in OPTION MODE VRAM:
    // row16 = (ch - 0x20)/16, col16 = (ch - 0x20)%16, top = 0xE200 + row16*0x200 + col16*16, bot = top + 0x100
    for (int ch = 0x20; ch <= 0x5F; ch++) {
        int row16 = (ch - 0x20) / 16;
        int col16 = (ch - 0x20) % 16;
        int top_off = 0xE200 + row16 * 0x200 + col16 * 16;
        decode_2bpp_8x16_to_10(vram + top_off, vram + top_off + 0x100, glyph[ch]);
    }
    for (int ch = 'a'; ch <= 'z'; ch++) memcpy(glyph[ch], glyph[ch - 32], 8 * 10);
    // Official right-arrow triangle cursor at VRAM 0xEAC0 (top) / 0xEBC0 (bot)
    decode_2bpp_8x16_to_10(vram + 0xEAC0, vram + 0xEBC0, cursor_tile);
    has_font = true;
}

/* ------------------------------------------------------------------ */
/* Snapshots, Modes & Live Gameplay Cheats + Mouse/Touch Aim            */
/* ------------------------------------------------------------------ */

static uint8_t *title_snap;
static size_t snap_size;
static bool started;
static bool classic_boot;
static uint32_t total_frames;

// Live mouse/touch aim state (in 256x224 game coordinates)
static bool aim_active;
static int  aim_x = 128, aim_y = 96;
static bool mouse_fire_held, mouse_jump_held, mouse_bomb_pulse;

static void boot_and_cache_snapshots(void)
{
    mute = true; scripting = true; vars_dirty = true;
    // Boot cleanly to Title Screen ("GAME START / V.S START / OPTION")
    step2(420, 0, 0);
    for (int i = 0; i < 5; i++) tap1(RETRO_DEVICE_ID_JOYPAD_START, 8, 100);
    memcpy(bg_art, game, sizeof bg_art);

    snap_size = core.ser_size();
    free(title_snap);
    title_snap = (uint8_t *)malloc(snap_size);
    if (title_snap) core.serialize(title_snap, snap_size);

    // Briefly enter OPTION MODE to extract the crisp 2bpp 8x8 Western font (VRAM 0xE100)
    tap1(RETRO_DEVICE_ID_JOYPAD_DOWN, 6, 18);
    tap1(RETRO_DEVICE_ID_JOYPAD_DOWN, 6, 18);
    tap1(RETRO_DEVICE_ID_JOYPAD_START, 8, 90);
    uint8_t *opt_snap = (uint8_t *)malloc(snap_size);
    if (opt_snap && core.serialize(opt_snap, snap_size)) {
        extract_font_from_opt_vram(opt_snap, snap_size);
    }
    free(opt_snap);

    if (title_snap) core.unserialize(title_snap, snap_size);
    scripting = false; vars_dirty = true; mute = false;
}

static bool in_live_action(const uint8_t *m)
{
    if (!m) return false;
    // During live gameplay: timer is active (>0 or bombs/lives initialized) and crosshair Y is in arena
    return (m[RAM_TIMER] > 0 && m[RAM_TIMER] <= 99) ||
           (m[RAM_P1_CROSS_Y] >= 12 && m[RAM_P1_CROSS_Y] <= 190 && (m[RAM_P1_LIVES] > 0 || m[RAM_P1_BOMBS] > 0));
}

static void apply_cheats_frame(void)
{
    uint8_t *m = wram();
    if (!m || !started) return;
    // In Classic Mode, keep Natsume logo, intro, title, and select screens 100% untouched!
    if (classic_boot && !in_live_action(m)) return;

    // Only modify P2 HUD/stats when Player 2 is actually active (0x1FA6 non-zero in 2P mode)
    bool p2_on = (m[RAM_P2_HUD_DIRTY] != 0);

    if (cfg.c_god) {
        if (m[RAM_P1_INVULN] < 0x30) m[RAM_P1_INVULN] = 0x40;
        if (p2_on && m[RAM_P2_INVULN] < 0x30) m[RAM_P2_INVULN] = 0x40;
    }
    if (cfg.c_lives) {
        if (m[RAM_P1_LIVES] < 9) { m[RAM_P1_LIVES] = 9; m[RAM_P1_HUD_DIRTY] |= 0x08; }
        if (p2_on && m[RAM_P2_LIVES] < 9) { m[RAM_P2_LIVES] = 9; m[RAM_P2_HUD_DIRTY] |= 0x08; }
    }
    if (cfg.c_bombs) {
        if (m[RAM_P1_BOMBS] < 5) { m[RAM_P1_BOMBS] = 5; m[RAM_P1_HUD_DIRTY] |= 0x04; }
        if (p2_on && m[RAM_P2_BOMBS] < 5) { m[RAM_P2_BOMBS] = 5; m[RAM_P2_HUD_DIRTY] |= 0x04; }
    }
    if (cfg.c_weapon > 0) {
        // Verified weapon IDs: 4=S.Gun, 8=G.Gun, 12=M.Gun, 16=P.Gun
        uint8_t wid = (uint8_t)(cfg.c_weapon * 4);
        m[RAM_P1_WEAPON] = wid;
        if (m[RAM_P1_AMMO] < 0x50) m[RAM_P1_AMMO] = 0x99;
        if (p2_on) {
            m[RAM_P2_WEAPON] = wid;
            if (m[RAM_P2_AMMO] < 0x50) m[RAM_P2_AMMO] = 0x99;
        }
    } else if (cfg.c_ammo) {
        if (m[RAM_P1_WEAPON] != 0 && m[RAM_P1_AMMO] < 0x50) m[RAM_P1_AMMO] = 0x99;
        if (p2_on && m[RAM_P2_WEAPON] != 0 && m[RAM_P2_AMMO] < 0x50) m[RAM_P2_AMMO] = 0x99;
    }
    if (cfg.c_timer && m[RAM_TIMER] > 0 && m[RAM_TIMER] < 80) {
        m[RAM_TIMER] = 80;
    }
    // Optional PC Mouse / Android Direct-Touch Crosshair Aiming for P1!
    if (cfg.mouse_aim && aim_active && in_live_action(m)) {
        int cam_x = (int)(m[RAM_CAM_X] | (m[RAM_CAM_X + 1] << 8));
        int wx = aim_x + cam_x;
        if (wx < cam_x + 12) wx = cam_x + 12;
        if (wx > cam_x + 244) wx = cam_x + 244;
        int wy = aim_y;
        if (wy < 14) wy = 14;
        if (wy > 176) wy = 176;
        m[RAM_P1_CROSS_X] = (uint8_t)(wx & 0xFF);
        m[RAM_P1_CROSS_Y] = (uint8_t)(wy & 0xFF);
    }
}

static void launch_classic_boot(void)
{
    if (audio) SDL_ClearQueuedAudio(audio);
    scripting = false; vars_dirty = true; mute = false;
    core.reset();
    started = true;
    classic_boot = true;
    aim_active = false;
}

static void launch_fast_game(bool two_players, bool stage_select)
{
    if (!title_snap) return;
    if (audio) SDL_ClearQueuedAudio(audio);
    mute = true; scripting = true; vars_dirty = true;

    core.unserialize(title_snap, snap_size);
    tap1(RETRO_DEVICE_ID_JOYPAD_START, 8, 100); // GAME START -> 1P / 2P
    if (two_players) tap1(RETRO_DEVICE_ID_JOYPAD_DOWN, 6, 20);
    tap1(RETRO_DEVICE_ID_JOYPAD_START, 8, 150); // -> SELECT PLAYER

    if (!two_players && cfg.character == 1) {
        tap1(RETRO_DEVICE_ID_JOYPAD_RIGHT, 6, 20); // Select Annie
    }
    if (stage_select) {
        // Official Natsume Stage Select code: hold SELECT + AAAA BBBB ABABABAB
        static const uint16_t seq[16] = {
            1u << RETRO_DEVICE_ID_JOYPAD_A, 1u << RETRO_DEVICE_ID_JOYPAD_A,
            1u << RETRO_DEVICE_ID_JOYPAD_A, 1u << RETRO_DEVICE_ID_JOYPAD_A,
            1u << RETRO_DEVICE_ID_JOYPAD_B, 1u << RETRO_DEVICE_ID_JOYPAD_B,
            1u << RETRO_DEVICE_ID_JOYPAD_B, 1u << RETRO_DEVICE_ID_JOYPAD_B,
            1u << RETRO_DEVICE_ID_JOYPAD_A, 1u << RETRO_DEVICE_ID_JOYPAD_B,
            1u << RETRO_DEVICE_ID_JOYPAD_A, 1u << RETRO_DEVICE_ID_JOYPAD_B,
            1u << RETRO_DEVICE_ID_JOYPAD_A, 1u << RETRO_DEVICE_ID_JOYPAD_B,
            1u << RETRO_DEVICE_ID_JOYPAD_A, 1u << RETRO_DEVICE_ID_JOYPAD_B
        };
        uint16_t selb = 1u << RETRO_DEVICE_ID_JOYPAD_SELECT;
        for (int i = 0; i < 16; i++) {
            step2(4, selb | seq[i], 0);
            step2(4, selb, 0);
        }
        step2(10, 0, 0);
        if (two_players) step2(8, 1u << RETRO_DEVICE_ID_JOYPAD_START, 1u << RETRO_DEVICE_ID_JOYPAD_START);
        else tap1(RETRO_DEVICE_ID_JOYPAD_START, 8, 0);
        step2(120, 0, 0); // Lands directly on the visual STAGE SELECT screen!
    } else {
        if (two_players) step2(8, 1u << RETRO_DEVICE_ID_JOYPAD_START, 1u << RETRO_DEVICE_ID_JOYPAD_START);
        else tap1(RETRO_DEVICE_ID_JOYPAD_START, 8, 0);
        step2(840, 0, 0); // Fast-forward past GOOD LUCK! straight into Carson City!
    }

    started = true;
    classic_boot = false;
    aim_active = false;
    apply_cheats_frame();
    scripting = false; vars_dirty = true; mute = false;
}

static void launch_versus(void)
{
    if (!title_snap) return;
    if (audio) SDL_ClearQueuedAudio(audio);
    mute = true; scripting = true; vars_dirty = true;

    core.unserialize(title_snap, snap_size);
    tap1(RETRO_DEVICE_ID_JOYPAD_DOWN, 6, 20);  // V.S START
    tap1(RETRO_DEVICE_ID_JOYPAD_START, 8, 90); // 1P V.S COM / 1P V.S 2P

    started = true;
    classic_boot = true; // let user choose 1P vs COM or 1P vs 2P cleanly
    aim_active = false;
    scripting = false; vars_dirty = true; mute = false;
}

static void state_path(int slot, char *out, size_t n)
{
    snprintf(out, n, "%s%cwildguns_slot%d.sav", save_dir, PATH_SEP, slot + 1);
}

static bool save_state(int slot)
{
    if (!started || !snap_size) return false;
    uint8_t *buf = (uint8_t *)malloc(snap_size);
    if (!buf || !core.serialize(buf, snap_size)) { free(buf); return false; }
    char p[1024]; state_path(slot, p, sizeof p);
    FILE *f = fopen(p, "wb");
    bool ok = f && fwrite(buf, 1, snap_size, f) == snap_size;
    if (f) fclose(f);
    free(buf);
    return ok;
}

static bool load_state(int slot)
{
    if (!snap_size) return false;
    char p[1024]; state_path(slot, p, sizeof p);
    FILE *f = fopen(p, "rb");
    if (!f) return false;
    uint8_t *buf = (uint8_t *)malloc(snap_size);
    bool ok = buf && fread(buf, 1, snap_size, f) == snap_size && core.unserialize(buf, snap_size);
    fclose(f); free(buf);
    if (ok) { started = true; classic_boot = false; if (audio) SDL_ClearQueuedAudio(audio); }
    return ok;
}

/* ------------------------------------------------------------------ */
/* UI Rendering (256x224)                                               */
/* ------------------------------------------------------------------ */

static uint32_t ui[W * H];
static char toast[80];
static Uint32 toast_until;

static void show_toast(const char *msg)
{
    snprintf(toast, sizeof toast, "%s", msg);
    toast_until = SDL_GetTicks() + 2200u;
}

#define T(es, en) (cfg.lang == 1 ? (en) : (es))

static const uint32_t GOLD   = 0xFFFFD848u;
static const uint32_t COPPER = 0xFFE88430u;
static const uint32_t WHITE  = 0xFFF8F4E8u;
static const uint32_t DIM    = 0xFFB0A89Cu;
static const uint32_t CYAN   = 0xFF60E8FFu;
static const uint32_t GREEN  = 0xFF70F088u;

static void put_px(int x, int y, uint32_t c)
{
    if (x >= 0 && x < W && y >= 0 && y < H) ui[y * W + x] = c;
}

static void fill_rect(int x, int y, int w, int h, uint32_t argb)
{
    uint32_t a = (argb >> 24) & 0xFFu;
    uint32_t sr = (argb >> 16) & 0xFFu, sg = (argb >> 8) & 0xFFu, sb = argb & 0xFFu;
    for (int j = y; j < y + h; j++) {
        if (j < 0 || j >= H) continue;
        for (int i = x; i < x + w; i++) {
            if (i < 0 || i >= W) continue;
            if (a == 255) { ui[j * W + i] = argb; continue; }
            uint32_t dst = ui[j * W + i];
            uint32_t dr = (dst >> 16) & 0xFFu, dg = (dst >> 8) & 0xFFu, db = dst & 0xFFu;
            uint32_t r = (sr * a + dr * (255 - a)) / 255;
            uint32_t g = (sg * a + dg * (255 - a)) / 255;
            uint32_t b = (sb * a + db * (255 - a)) / 255;
            ui[j * W + i] = 0xFF000000u | (r << 16) | (g << 8) | b;
        }
    }
}

static void draw_box(int x, int y, int w, int h, uint32_t bg, uint32_t border)
{
    fill_rect(x, y, w, h, bg);
    fill_rect(x, y, w, 1, border);
    fill_rect(x, y + h - 1, w, 1, border);
    fill_rect(x, y, 1, h, border);
    fill_rect(x + w - 1, y, 1, h, border);
}

static void draw_char(int x, int y, unsigned char ch, uint32_t col)
{
    if (ch >= 'a' && ch <= 'z') ch = (unsigned char)(ch - 32);
    if (ch >= 128 || !has_font) return;
    const uint8_t *g = glyph[ch];
    uint32_t mid = 0xFF000000u | ((((col >> 16) & 0xFFu) * 2 / 3) << 16) |
                   ((((col >> 8) & 0xFFu) * 2 / 3) << 8) | ((col & 0xFFu) * 2 / 3);
    for (int r = 0; r < 10; r++) {
        for (int c = 0; c < 8; c++) {
            uint8_t v = g[r * 8 + c];
            if (!v) continue;
            put_px(x + c + 1, y + r + 1, 0xFF140C08u);
            put_px(x + c, y + r, v == 3 ? col : (v == 2 ? mid : 0xFF281810u));
        }
    }
}

static void text(int x, int y, const char *s, uint32_t col)
{
    for (; *s; s++, x += 8) draw_char(x, y, (unsigned char)*s, col);
}
static void text_c(int y, const char *s, uint32_t col) { text((W - (int)strlen(s) * 8) / 2, y, s, col); }
static void text_r(int rx, int y, const char *s, uint32_t col) { text(rx - (int)strlen(s) * 8, y, s, col); }

static void draw_cursor(int x, int y)
{
    if (!has_font) return;
    for (int r = 0; r < 10; r++) {
        for (int c = 0; c < 8; c++) {
            uint8_t v = cursor_tile[r * 8 + c];
            if (!v) continue;
            put_px(x + c + 1, y + r + 1, 0xFF140C08u);
            put_px(x + c, y + r, v == 3 ? GOLD : COPPER);
        }
    }
}

/* ------------------------------------------------------------------ */
/* Menus                                                                */
/* ------------------------------------------------------------------ */

enum { ST_MENU, ST_PLAY };
enum { P_START, P_PAUSE, P_CHEATS, P_OPTIONS, P_CONTROLS };

static int state = ST_MENU;
static int page = P_START;
static int sel = 0;
static int binding = -1;
static bool bind_pad = false;
static int menu_last_y0, menu_last_step, menu_last_first, menu_last_rows, menu_last_n;

typedef struct { const char *label, *value, *value2; int id; } item_t;

static const char *weapon_name(void)
{
    static const char *w[] = { "ORIGINAL", "S.GUN", "G.GUN", "M.GUN", "P.GUN" };
    return w[cfg.c_weapon % 5];
}
static const char *touch_name(void)
{
    static const char *es[] = { "AUTO", "SI", "NO" };
    static const char *en[] = { "AUTO", "ON", "OFF" };
    return cfg.lang ? en[cfg.touch % 3] : es[cfg.touch % 3];
}

static int build(item_t *it)
{
    int n = 0;
    static char slot_buf[16], kb_buf[12][16], gp_buf[12][16];
    const char *on_s = T("SI", "ON"), *off_s = T("NO", "OFF");

    if (page == P_START) {
        it[n++] = (item_t){ T("INICIO RAPIDO 1P", "FAST START 1P"),  cfg.character ? "ANNIE" : "CLINT", NULL, 1 };
        it[n++] = (item_t){ T("INICIO RAPIDO 2P", "FAST START 2P"),  "CO-OP", NULL, 2 };
        it[n++] = (item_t){ T("MODO CLASICO",     "CLASSIC MODE"),   NULL, NULL, 3 };
        it[n++] = (item_t){ T("ELEGIR ESCENARIO", "STAGE SELECT"),   "6 ZONAS", NULL, 4 };
        it[n++] = (item_t){ T("MODO VERSUS",      "VERSUS MODE"),    "VS", NULL, 5 };
        it[n++] = (item_t){ T("PERSONAJE 1P",     "1P CHARACTER"),   cfg.character ? "ANNIE" : "CLINT", NULL, 6 };
        it[n++] = (item_t){ T("ARSENAL Y TRUCOS", "WEAPONS & CHEATS"), NULL, NULL, 7 };
        it[n++] = (item_t){ T("OPCIONES",         "OPTIONS"),        NULL, NULL, 8 };
        it[n++] = (item_t){ T("CONTROLES",        "CONTROLS"),       NULL, NULL, 9 };
        it[n++] = (item_t){ T("SALIR",            "QUIT"),           NULL, NULL, 10 };
    } else if (page == P_PAUSE) {
        snprintf(slot_buf, sizeof slot_buf, "SLOT %d", cfg.slot + 1);
        it[n++] = (item_t){ T("CONTINUAR",        "RESUME"),         NULL, NULL, 1 };
        it[n++] = (item_t){ T("GUARDAR ESTADO",   "SAVE STATE"),     slot_buf, NULL, 2 };
        it[n++] = (item_t){ T("CARGAR ESTADO",    "LOAD STATE"),     slot_buf, NULL, 3 };
        it[n++] = (item_t){ T("CAMBIAR SLOT",     "SWITCH SLOT"),    slot_buf, NULL, 4 };
        it[n++] = (item_t){ T("ARSENAL Y TRUCOS", "WEAPONS & CHEATS"), NULL, NULL, 5 };
        it[n++] = (item_t){ T("ELEGIR ESCENARIO", "STAGE SELECT"),   NULL, NULL, 6 };
        it[n++] = (item_t){ T("OPCIONES",         "OPTIONS"),        NULL, NULL, 7 };
        it[n++] = (item_t){ T("CONTROLES",        "CONTROLS"),       NULL, NULL, 8 };
        it[n++] = (item_t){ T("MENU INICIAL",     "TITLE MENU"),     NULL, NULL, 9 };
    } else if (page == P_CHEATS) {
        it[n++] = (item_t){ T("ARMA ACTIVA",      "ACTIVE WEAPON"),  weapon_name(), NULL, 1 };
        it[n++] = (item_t){ T("MUNICION INFINITA","INFINITE AMMO"),  cfg.c_ammo   ? on_s : off_s, NULL, 2 };
        it[n++] = (item_t){ T("BOMBAS INFINITAS", "INFINITE BOMBS"), cfg.c_bombs  ? on_s : off_s, NULL, 3 };
        it[n++] = (item_t){ T("VIDAS INFINITAS",  "INFINITE LIVES"), cfg.c_lives  ? on_s : off_s, NULL, 4 };
        it[n++] = (item_t){ T("MODO DIOS",        "GOD MODE"),       cfg.c_god    ? on_s : off_s, NULL, 5 };
        it[n++] = (item_t){ T("RELOJ CONGELADO",  "FREEZE TIMER"),   cfg.c_timer  ? on_s : off_s, NULL, 6 };
        it[n++] = (item_t){ T("DISPARO AUTO",     "AUTO FIRE"),      cfg.autofire ? on_s : off_s, NULL, 7 };
        it[n++] = (item_t){ T("VOLVER",           "BACK"),           NULL, NULL, 99 };
    } else if (page == P_OPTIONS) {
        it[n++] = (item_t){ T("MIRA RATON/TACTIL","MOUSE/TOUCH AIM"),cfg.mouse_aim  ? on_s : off_s, NULL, 1 };
        it[n++] = (item_t){ T("FILTRO VIDEO",     "VIDEO FILTER"),   cfg.scale ? T("SUAVE", "SMOOTH") : "PIXEL", NULL, 2 };
        it[n++] = (item_t){ T("PANTALLA COMP",    "FULLSCREEN"),     cfg.fullscreen ? on_s : off_s, NULL, 3 };
        it[n++] = (item_t){ T("SIN SLOWDOWN",     "NO SLOWDOWN"),    cfg.noslow     ? on_s : off_s, NULL, 4 };
        it[n++] = (item_t){ T("TACTIL ANDROID",   "TOUCH PAD"),      touch_name(), NULL, 5 };
        it[n++] = (item_t){ T("IDIOMA",           "LANGUAGE"),       cfg.lang ? "ENGLISH" : "ESPANOL", NULL, 6 };
        it[n++] = (item_t){ T("CAMBIAR ROM",      "CHANGE ROM"),     NULL, NULL, 7 };
        it[n++] = (item_t){ T("VOLVER",           "BACK"),           NULL, NULL, 99 };
    } else if (page == P_CONTROLS) {
        for (int i = 0; i < 12; i++) {
            const char *kn = SDL_GetScancodeName((SDL_Scancode)cfg.kb[i]);
            snprintf(kb_buf[i], sizeof kb_buf[i], "%.7s", (kn && *kn) ? kn : "-");
            const char *gn = cfg.gp[i] >= 0 ? SDL_GameControllerGetStringForButton((SDL_GameControllerButton)cfg.gp[i]) : "-";
            snprintf(gp_buf[i], sizeof gp_buf[i], "%.6s", (gn && *gn) ? gn : "-");
            it[n++] = (item_t){ cfg.lang ? btn_en[i] : btn_es[i],
                                (binding == i && !bind_pad) ? "..." : kb_buf[i],
                                (binding == i &&  bind_pad) ? "..." : gp_buf[i], i };
        }
        it[n++] = (item_t){ T("RESTAURAR", "RESET DEFAULTS"), NULL, NULL, 98 };
        it[n++] = (item_t){ T("VOLVER", "BACK"), NULL, NULL, 99 };
    }
    return n;
}

static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Texture *tex_game, *tex_ui;
static bool running = true;

static void apply_video_settings(void)
{
    if (!win || !ren) return;
    SDL_SetWindowFullscreen(win, cfg.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, cfg.scale ? "1" : "0");
    if (tex_game) SDL_DestroyTexture(tex_game);
    tex_game = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, W, H);
}

static int parent_page(void) { return started ? P_PAUSE : P_START; }

static void back(void)
{
    if (binding >= 0) { binding = -1; return; }
    if (page == P_PAUSE) { state = ST_PLAY; return; }
    if (page == P_START) return;
    page = parent_page(); sel = 0;
}

static void pause_game(void)
{
    if (audio) SDL_ClearQueuedAudio(audio);
    state = ST_MENU; page = P_PAUSE; sel = 0;
    mouse_fire_held = false; mouse_jump_held = false;
}

static void activate(void)
{
    item_t it[24];
    int n = build(it);
    if (sel < 0 || sel >= n) return;
    int id = it[sel].id;

    if (page == P_START) {
        if (id == 1)      { launch_fast_game(false, false); state = ST_PLAY; }
        else if (id == 2) { launch_fast_game(true,  false); state = ST_PLAY; }
        else if (id == 3) { launch_classic_boot();          state = ST_PLAY; }
        else if (id == 4) { launch_fast_game(false, true);  state = ST_PLAY; }
        else if (id == 5) { launch_versus();                state = ST_PLAY; }
        else if (id == 6) { cfg.character ^= 1; save_cfg(); }
        else if (id == 7) { page = P_CHEATS; sel = 0; }
        else if (id == 8) { page = P_OPTIONS; sel = 0; }
        else if (id == 9) { page = P_CONTROLS; sel = 0; }
        else if (id == 10){ running = false; }
    } else if (page == P_PAUSE) {
        if (id == 1) state = ST_PLAY;
        else if (id == 2) show_toast(save_state(cfg.slot) ? T("ESTADO GUARDADO", "STATE SAVED") : T("ERROR", "ERROR"));
        else if (id == 3) {
            if (load_state(cfg.slot)) { state = ST_PLAY; show_toast(T("ESTADO CARGADO", "STATE LOADED")); }
            else show_toast(T("SLOT VACIO", "EMPTY SLOT"));
        }
        else if (id == 4) { cfg.slot = (cfg.slot + 1) % 3; save_cfg(); }
        else if (id == 5) { page = P_CHEATS; sel = 0; }
        else if (id == 6) { launch_fast_game(false, true); state = ST_PLAY; }
        else if (id == 7) { page = P_OPTIONS; sel = 0; }
        else if (id == 8) { page = P_CONTROLS; sel = 0; }
        else if (id == 9) { started = false; page = P_START; sel = 0; }
    } else if (page == P_CHEATS) {
        if (id == 99) back();
        else if (id == 1) cfg.c_weapon = (cfg.c_weapon + 1) % 5;
        else if (id == 2) cfg.c_ammo = !cfg.c_ammo;
        else if (id == 3) cfg.c_bombs = !cfg.c_bombs;
        else if (id == 4) cfg.c_lives = !cfg.c_lives;
        else if (id == 5) cfg.c_god = !cfg.c_god;
        else if (id == 6) cfg.c_timer = !cfg.c_timer;
        else if (id == 7) cfg.autofire = !cfg.autofire;
        apply_cheats_frame();
        save_cfg();
    } else if (page == P_OPTIONS) {
        if (id == 99) back();
        else if (id == 1) cfg.mouse_aim = !cfg.mouse_aim;
        else if (id == 2) { cfg.scale = !cfg.scale; apply_video_settings(); }
        else if (id == 3) { cfg.fullscreen = !cfg.fullscreen; apply_video_settings(); }
        else if (id == 4) { cfg.noslow = !cfg.noslow; overclock = cfg.noslow != 0; vars_dirty = true; }
        else if (id == 5) cfg.touch = (cfg.touch + 1) % 3;
        else if (id == 6) cfg.lang = !cfg.lang;
        else if (id == 7) {
            char chosen[1024], err[128];
            if (pick_rom_dialog(chosen, sizeof chosen)) {
                if (load_rom_file(chosen, err, sizeof err)) {
                    struct retro_game_info gi = { rom_path, rom_buf, rom_len, NULL };
                    core.load_game(&gi);
                    boot_and_cache_snapshots();
                    started = false; page = P_START; sel = 0; save_cfg();
                    show_toast(T("ROM CARGADA!", "ROM LOADED!"));
                } else show_toast(err);
            }
        }
        save_cfg();
    } else if (page == P_CONTROLS) {
        if (id == 99) back();
        else if (id == 98) { default_controls(); save_cfg(); show_toast(T("CONTROLES RESTAURADOS", "CONTROLS RESET")); }
        else { binding = id; bind_pad = false; }
    }
}

static void draw_menu(void)
{
    const uint32_t *src = (state == ST_MENU && page != P_START && started) ? game : bg_art;
    for (int i = 0; i < W * H; i++) {
        uint32_t c = src[i];
        uint32_t r = ((c >> 16) & 0xFFu) * 38 / 100;
        uint32_t g = ((c >>  8) & 0xFFu) * 36 / 100;
        uint32_t b = ( c        & 0xFFu) * 32 / 100;
        ui[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
    }

    // Western wood-and-brass frontier frame (matches Wild Guns OPTION MODE aesthetic)
    draw_box(10, 10, W - 20, H - 20, 0xD41C120Au, GOLD);
    draw_box(12, 12, W - 24, H - 24, 0x00000000u, COPPER);

    static const char *t_es[] = { "WILD GUNS", "PAUSA", "ARSENAL Y TRUCOS", "OPCIONES", "CONTROLES" };
    static const char *t_en[] = { "WILD GUNS", "PAUSED", "WEAPONS & CHEATS", "OPTIONS", "CONTROLS" };
    const char *title = cfg.lang ? t_en[page] : t_es[page];
    fill_rect(20, 16, W - 40, 14, 0xFF4A2010u);
    text_c(19, title, GOLD);

    item_t it[24];
    int n = build(it);
    int x = 28, y0 = 38, step_y = 15, vx = 236, rows = n, first = 0;
    if (page == P_START)    { step_y = 15; y0 = 38; }
    if (page == P_PAUSE)    { step_y = 16; y0 = 38; }
    if (page == P_CHEATS)   { step_y = 18; y0 = 42; }
    if (page == P_OPTIONS)  { step_y = 18; y0 = 42; }
    if (page == P_CONTROLS) {
        x = 22; vx = 176; step_y = 13; y0 = 46; rows = 10;
        first = sel - rows / 2;
        if (first > n - rows) first = n - rows;
        if (first < 0) first = 0;
        text_r(176, 34, T("TECLA", "KEY"), DIM);
        text(184, 34, T("MANDO", "PAD"), DIM);
    }

    menu_last_y0 = y0; menu_last_step = step_y;
    menu_last_first = first; menu_last_rows = rows; menu_last_n = n;

    for (int i = first; i < n && i < first + rows; i++) {
        int y = y0 + (i - first) * step_y;
        bool on = (i == sel);
        if (on) fill_rect(16, y - 2, W - 32, 12, 0x78A84414u);
        text(x, y, it[i].label, on ? GOLD : WHITE);
        if (it[i].value) {
            uint32_t vc = on ? GOLD : CYAN;
            if (!strcmp(it[i].value, "SI") || !strcmp(it[i].value, "ON")) vc = GREEN;
            else if (!strcmp(it[i].value, "NO") || !strcmp(it[i].value, "OFF")) vc = DIM;
            text_r(vx, y, it[i].value, vc);
        }
        if (it[i].value2) text(184, y, it[i].value2, on ? GOLD : WHITE);
        if (on) draw_cursor(x - 10, y);
    }

    const char *foot = "";
    if (page == P_START)       foot = T("ESC/F1: PAUSA  RATON: APUNTAR", "ESC/F1: PAUSE  MOUSE: AIM");
    else if (page == P_CHEATS) foot = T("APLICAN A 1P Y 2P EN VIVO", "APPLIES TO 1P & 2P LIVE");
    else if (page == P_OPTIONS)foot = T("CLIC IZQ: DISPARO  DER: SALTO", "L-CLICK: FIRE  R-CLICK: ROLL");
    else if (page == P_CONTROLS) foot = binding >= 0 ? T("PULSA TECLA O BOTON...", "PRESS KEY OR BUTTON...")
                                                     : T("ENTER: TECLA  IZQ/DER: MANDO", "ENTER: KEY  L/R: PAD");
    if (*foot) text_c(196, foot, DIM);
}

static void draw_toast(void)
{
    if (!*toast || SDL_TICKS_PASSED(SDL_GetTicks(), toast_until)) { *toast = 0; return; }
    int tw = (int)strlen(toast) * 8 + 14;
    draw_box((W - tw) / 2, 4, tw, 15, 0xEC24120Au, GOLD);
    text_c(7, toast, GOLD);
}

/* ------------------------------------------------------------------ */
/* Input, 2P Co-op, Mouse/Direct-Touch Aim & Android Multi-Touch        */
/* ------------------------------------------------------------------ */

static SDL_GameController *pads[4];

#define MAX_FINGERS 10
typedef struct { SDL_FingerID id; float x, y; bool down; bool is_aim; } finger_t;
static finger_t fingers[MAX_FINGERS];
#ifdef __ANDROID__
static bool touch_seen = true;
#else
static bool touch_seen = false;
#endif
static bool pad_active_recently = false;
static Uint32 last_pad_tick = 0;

typedef struct {
    float cx, cy, hw, hh;
    int btn;
    const char *label;
    uint8_t r, g, b;
} tbtn_t;

enum { TACT_MENU = -1, TACT_SAVE = -2, TACT_LOAD = -3 };

static const tbtn_t touch_btns[] = {
    { 0.805f, 0.675f, 0.048f, 0.082f, RETRO_DEVICE_ID_JOYPAD_Y,      "FIRE", 240,  75,  45 },
    { 0.865f, 0.785f, 0.048f, 0.082f, RETRO_DEVICE_ID_JOYPAD_B,      "ROLL", 240, 195,  45 },
    { 0.925f, 0.675f, 0.048f, 0.082f, RETRO_DEVICE_ID_JOYPAD_A,      "LASO",  65, 195,  95 },
    { 0.865f, 0.565f, 0.048f, 0.082f, RETRO_DEVICE_ID_JOYPAD_X,      "BOMB", 160,  80, 240 },
    { 0.085f, 0.365f, 0.062f, 0.052f, RETRO_DEVICE_ID_JOYPAD_L,      "L",    185, 195, 220 },
    { 0.915f, 0.365f, 0.062f, 0.052f, RETRO_DEVICE_ID_JOYPAD_R,      "R",    185, 195, 220 },
    { 0.145f, 0.925f, 0.058f, 0.042f, RETRO_DEVICE_ID_JOYPAD_SELECT, "SEL",  155, 165, 190 },
    { 0.855f, 0.925f, 0.058f, 0.042f, RETRO_DEVICE_ID_JOYPAD_START,  "STRT", 155, 165, 190 },
    { 0.080f, 0.068f, 0.065f, 0.048f, TACT_MENU,                     "MENU", 245, 205,  60 },
    { 0.840f, 0.068f, 0.052f, 0.044f, TACT_SAVE,                     "SAVE",  80, 210, 245 },
    { 0.942f, 0.068f, 0.052f, 0.044f, TACT_LOAD,                     "LOAD", 120, 230, 140 },
};
#define NTOUCH_BTNS (int)(sizeof touch_btns / sizeof touch_btns[0])
static const float DPAD_CX = 0.125f, DPAD_CY = 0.695f, DPAD_RX = 0.095f, DPAD_RY = 0.165f;

static bool touch_pad_visible(void)
{
    if (cfg.touch == 1) return true;
    if (cfg.touch == 2) return false;
    if (!touch_seen) return false;
    if (pad_active_recently && (SDL_GetTicks() - last_pad_tick) < 15000u) return false;
    return true;
}

static SDL_Rect game_dst_rect(int rw, int rh)
{
    int dw = rw, dh = rh;
    if (rw * 3 > rh * 4) dw = rh * 4 / 3;
    else dh = rw * 3 / 4;
    SDL_Rect r = { (rw - dw) / 2, (rh - dh) / 2, dw, dh };
    return r;
}

static bool point_in_touch_ui(float fx, float fy)
{
    float dx = (fx - DPAD_CX) / DPAD_RX, dy = (fy - DPAD_CY) / DPAD_RY;
    if (dx * dx + dy * dy <= 1.55f) return true;
    for (int i = 0; i < NTOUCH_BTNS; i++) {
        const tbtn_t *tb = &touch_btns[i];
        if (fx >= tb->cx - tb->hw && fx <= tb->cx + tb->hw &&
            fy >= tb->cy - tb->hh && fy <= tb->cy + tb->hh) return true;
    }
    return false;
}

static void update_aim_from_norm(float nx, float ny)
{
    int rw = 0, rh = 0;
    SDL_GetRendererOutputSize(ren, &rw, &rh);
    if (rw <= 0 || rh <= 0) return;
    SDL_Rect dr = game_dst_rect(rw, rh);
    int gx = (int)((nx * rw - dr.x) * (float)W / (float)dr.w);
    int gy = (int)((ny * rh - dr.y) * (float)H / (float)dr.h);
    if (gx >= 0 && gx < W && gy >= 0 && gy < H) {
        aim_x = gx; aim_y = gy; aim_active = true;
    }
}

static uint16_t read_touch_pad(void)
{
    uint16_t b = 0;
    bool pad_vis = touch_pad_visible();
    for (int f = 0; f < MAX_FINGERS; f++) {
        if (!fingers[f].down) continue;
        if (fingers[f].is_aim) {
            if (cfg.mouse_aim) {
                update_aim_from_norm(fingers[f].x, fingers[f].y);
                b |= 1u << RETRO_DEVICE_ID_JOYPAD_Y; // touching battlefield aims & fires!
            }
            continue;
        }
        if (!pad_vis) continue;
        float fx = fingers[f].x, fy = fingers[f].y;
        float dx = (fx - DPAD_CX) / DPAD_RX, dy = (fy - DPAD_CY) / DPAD_RY;
        float d2 = dx * dx + dy * dy;
        if (d2 <= 1.45f && d2 >= 0.04f) {
            if (dx < -0.32f) b |= 1u << RETRO_DEVICE_ID_JOYPAD_LEFT;
            if (dx >  0.32f) b |= 1u << RETRO_DEVICE_ID_JOYPAD_RIGHT;
            if (dy < -0.32f) b |= 1u << RETRO_DEVICE_ID_JOYPAD_UP;
            if (dy >  0.32f) b |= 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
        }
        for (int i = 0; i < NTOUCH_BTNS; i++) {
            const tbtn_t *tb = &touch_btns[i];
            if (tb->btn < 0) continue;
            if (fx >= tb->cx - tb->hw && fx <= tb->cx + tb->hw &&
                fy >= tb->cy - tb->hh && fy <= tb->cy + tb->hh)
                b |= (uint16_t)(1u << tb->btn);
        }
    }
    return b;
}

static uint16_t read_controller(SDL_GameController *c)
{
    if (!c) return 0;
    uint16_t pb = 0;
    for (int i = 0; i < 12; i++)
        if (cfg.gp[i] >= 0 && SDL_GameControllerGetButton(c, (SDL_GameControllerButton)cfg.gp[i]))
            pb |= (uint16_t)(1u << snes_btn[i]);
    Sint16 ax = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTX);
    Sint16 ay = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTY);
    if (ax < -16000) pb |= 1u << RETRO_DEVICE_ID_JOYPAD_LEFT;
    if (ax >  16000) pb |= 1u << RETRO_DEVICE_ID_JOYPAD_RIGHT;
    if (ay < -16000) pb |= 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    if (ay >  16000) pb |= 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
    if (pb) { pad_active_recently = true; last_pad_tick = SDL_GetTicks(); }
    return pb;
}

static void read_both_pads(void)
{
    const Uint8 *k = SDL_GetKeyboardState(NULL);
    uint16_t p0 = read_touch_pad(), p1 = 0;

    // P1 Keyboard
    for (int i = 0; i < 12; i++)
        if (cfg.kb[i] > 0 && cfg.kb[i] < SDL_NUM_SCANCODES && k[cfg.kb[i]])
            p0 |= (uint16_t)(1u << snes_btn[i]);

    // P1 Mouse buttons (when Mouse Aim is enabled)
    if (cfg.mouse_aim) {
        if (mouse_fire_held) p0 |= 1u << RETRO_DEVICE_ID_JOYPAD_Y;
        if (mouse_jump_held) p0 |= 1u << RETRO_DEVICE_ID_JOYPAD_B;
        if (mouse_bomb_pulse) { p0 |= 1u << RETRO_DEVICE_ID_JOYPAD_X; mouse_bomb_pulse = false; }
    }

    // P2 Secondary Keyboard (IJKL move/aim, N fire, M jump/roll, H bomb, U lasso, O start)
    if (k[SDL_SCANCODE_I]) p1 |= 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    if (k[SDL_SCANCODE_K]) p1 |= 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
    if (k[SDL_SCANCODE_J]) p1 |= 1u << RETRO_DEVICE_ID_JOYPAD_LEFT;
    if (k[SDL_SCANCODE_L]) p1 |= 1u << RETRO_DEVICE_ID_JOYPAD_RIGHT;
    if (k[SDL_SCANCODE_N]) p1 |= 1u << RETRO_DEVICE_ID_JOYPAD_Y;
    if (k[SDL_SCANCODE_M]) p1 |= 1u << RETRO_DEVICE_ID_JOYPAD_B;
    if (k[SDL_SCANCODE_H]) p1 |= 1u << RETRO_DEVICE_ID_JOYPAD_X;
    if (k[SDL_SCANCODE_U]) p1 |= 1u << RETRO_DEVICE_ID_JOYPAD_A;
    if (k[SDL_SCANCODE_O]) p1 |= 1u << RETRO_DEVICE_ID_JOYPAD_START;

    // Gamepads: Pad 0 -> P1, Pad 1 -> P2
    int found = 0;
    for (int i = 0; i < 4; i++) {
        if (!pads[i]) continue;
        uint16_t gb = read_controller(pads[i]);
        if (found == 0) p0 |= gb;
        else if (found == 1) p1 |= gb;
        found++;
    }

    // Auto-fire turbo pulsing on Y when held
    if (cfg.autofire) {
        if ((p0 & (1u << RETRO_DEVICE_ID_JOYPAD_Y)) && (total_frames & 2))
            p0 &= (uint16_t)~(1u << RETRO_DEVICE_ID_JOYPAD_Y);
        if ((p1 & (1u << RETRO_DEVICE_ID_JOYPAD_Y)) && (total_frames & 2))
            p1 &= (uint16_t)~(1u << RETRO_DEVICE_ID_JOYPAD_Y);
    }

    // If user uses keyboard/pad D-Pad while NOT firing with mouse, release mouse override cleanly
    if (!mouse_fire_held && (p0 & ((1u << RETRO_DEVICE_ID_JOYPAD_UP) | (1u << RETRO_DEVICE_ID_JOYPAD_DOWN) |
                                   (1u << RETRO_DEVICE_ID_JOYPAD_LEFT) | (1u << RETRO_DEVICE_ID_JOYPAD_RIGHT)))) {
        // Keep aim_active only when mouse moves or fires
    }

    pad[0] = p0;
    pad[1] = p1;
}

enum { M_NONE, M_UP, M_DOWN, M_LEFT, M_RIGHT, M_OK, M_BACK, M_PAUSE, M_PGUP, M_PGDN };

static void menu_input(int m)
{
    item_t it[24];
    int n = build(it);
    if (n <= 0) return;
    if (m == M_UP) sel = (sel + n - 1) % n;
    else if (m == M_DOWN) sel = (sel + 1) % n;
    else if (m == M_PGUP) { sel -= 5; if (sel < 0) sel = 0; }
    else if (m == M_PGDN) { sel += 5; if (sel >= n) sel = n - 1; }
    else if (page == P_CONTROLS && (m == M_LEFT || m == M_RIGHT) && it[sel].id < 12) {
        binding = it[sel].id; bind_pad = true;
    } else if (m == M_OK || ((m == M_LEFT || m == M_RIGHT) && it[sel].value && page != P_CONTROLS)) {
        activate();
    } else if (m == M_BACK) back();
}

static void handle(int m)
{
    if (m == M_NONE) return;
    if (state == ST_PLAY) { if (m == M_PAUSE) pause_game(); return; }
    if (m == M_PAUSE) { if (page == P_START) return; back(); return; }
    menu_input(m);
}

static int key_action(SDL_Keycode k)
{
    switch (k) {
    case SDLK_UP: return M_UP;
    case SDLK_DOWN: return M_DOWN;
    case SDLK_LEFT: return M_LEFT;
    case SDLK_RIGHT: return M_RIGHT;
    case SDLK_RETURN: case SDLK_KP_ENTER: case SDLK_z: case SDLK_x: case SDLK_SPACE: return M_OK;
    case SDLK_BACKSPACE: return M_BACK;
    case SDLK_ESCAPE: case SDLK_F1: case SDLK_AC_BACK: return M_PAUSE;
    }
    return M_NONE;
}

static int pad_action(int b)
{
    switch (b) {
    case SDL_CONTROLLER_BUTTON_DPAD_UP: return M_UP;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return M_DOWN;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return M_LEFT;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return M_RIGHT;
    case SDL_CONTROLLER_BUTTON_A: case SDL_CONTROLLER_BUTTON_START: return M_OK;
    case SDL_CONTROLLER_BUTTON_B: return M_BACK;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return M_PGUP;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return M_PGDN;
    case SDL_CONTROLLER_BUTTON_BACK: case SDL_CONTROLLER_BUTTON_GUIDE: return M_PAUSE;
    }
    return M_NONE;
}

static void poll_stick_menu(void)
{
    static int held_dir = M_NONE;
    static Uint32 next_rep = 0;
    if (state == ST_PLAY || binding >= 0) { held_dir = M_NONE; return; }
    int dir = M_NONE;
    for (int p = 0; p < 4; p++) {
        SDL_GameController *c = pads[p];
        if (!c) continue;
        Sint16 ax = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTX);
        Sint16 ay = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTY);
        if (ay < -18000) { dir = M_UP; break; }
        if (ay >  18000) { dir = M_DOWN; break; }
        if (ax < -18000) { dir = M_LEFT; break; }
        if (ax >  18000) { dir = M_RIGHT; break; }
    }
    Uint32 now = SDL_GetTicks();
    if (dir == M_NONE) { held_dir = M_NONE; return; }
    pad_active_recently = true; last_pad_tick = now;
    if (dir != held_dir) { held_dir = dir; next_rep = now + 260u; handle(dir); }
    else if (SDL_TICKS_PASSED(now, next_rep)) { next_rep = now + 110u; handle(dir); }
}

static void handle_menu_tap(float nx, float ny)
{
    int rw = 0, rh = 0;
    SDL_GetRendererOutputSize(ren, &rw, &rh);
    if (rw <= 0 || rh <= 0) return;
    SDL_Rect dr = game_dst_rect(rw, rh);
    float gx = (nx * rw - dr.x) * (float)W / (float)dr.w;
    float gy = (ny * rh - dr.y) * (float)H / (float)dr.h;
    if (gx < 10 || gx > W - 10 || gy < 10 || gy > H - 10) {
        if (page != P_START) back();
        return;
    }
    if (menu_last_step <= 0) return;
    int idx = menu_last_first + (int)((gy - (float)(menu_last_y0 - 2)) / (float)menu_last_step);
    if (idx >= menu_last_first && idx < menu_last_first + menu_last_rows && idx < menu_last_n) {
        sel = idx;
        activate();
    }
}

static void draw_touch_overlay(int rw, int rh)
{
    if (state != ST_PLAY) return;
    bool show_pad = touch_pad_visible();
    bool show_pill = (cfg.touch != 2) && (touch_seen || cfg.touch == 1);
    if (!show_pad && !show_pill) return;

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    if (show_pad) {
        int cx = (int)(DPAD_CX * rw), cy = (int)(DPAD_CY * rh);
        int rx = (int)(DPAD_RX * rw), ry = (int)(DPAD_RY * rh);
        SDL_Rect hbar = { cx - rx, cy - ry / 3, rx * 2, ry * 2 / 3 };
        SDL_Rect vbar = { cx - rx / 3, cy - ry, rx * 2 / 3, ry * 2 };
        SDL_SetRenderDrawColor(ren, 28, 18, 12, 120);
        SDL_RenderFillRect(ren, &hbar);
        SDL_RenderFillRect(ren, &vbar);
        SDL_SetRenderDrawColor(ren, 255, 215, 80, 175);
        SDL_RenderDrawRect(ren, &hbar);
        SDL_RenderDrawRect(ren, &vbar);
    }
    for (int i = 0; i < NTOUCH_BTNS; i++) {
        const tbtn_t *tb = &touch_btns[i];
        if (tb->btn >= 0 && !show_pad) continue;
        if (tb->btn < 0 && !show_pill) continue;
        SDL_Rect rc = {
            (int)((tb->cx - tb->hw) * rw), (int)((tb->cy - tb->hh) * rh),
            (int)(tb->hw * 2 * rw),        (int)(tb->hh * 2 * rh)
        };
        bool pressed = (tb->btn >= 0) && ((pad[0] >> tb->btn) & 1);
        SDL_SetRenderDrawColor(ren, tb->r, tb->g, tb->b, pressed ? 210 : 105);
        SDL_RenderFillRect(ren, &rc);
        SDL_SetRenderDrawColor(ren, 255, 255, 255, pressed ? 255 : 175);
        SDL_RenderDrawRect(ren, &rc);
    }
}

/* ------------------------------------------------------------------ */
/* ROM Link Screen & Initial Lookup                                     */
/* ------------------------------------------------------------------ */

static bool rom_link_screen(void)
{
    char err[128] = "";
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) return false;
            if (ev.type == SDL_DROPFILE) {
                char *dropped = ev.drop.file;
                if (load_rom_file(dropped, err, sizeof err)) { SDL_free(dropped); return true; }
                SDL_free(dropped);
            }
            if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_ESCAPE) return false;
                if (ev.key.keysym.sym == SDLK_RETURN || ev.key.keysym.sym == SDLK_SPACE) {
                    char chosen[1024];
                    if (pick_rom_dialog(chosen, sizeof chosen) && load_rom_file(chosen, err, sizeof err)) return true;
                }
            }
            if (ev.type == SDL_MOUSEBUTTONDOWN || ev.type == SDL_FINGERDOWN) {
                char chosen[1024];
                if (pick_rom_dialog(chosen, sizeof chosen) && load_rom_file(chosen, err, sizeof err)) return true;
            }
        }
        SDL_SetRenderDrawColor(ren, 28, 16, 10, 255);
        SDL_RenderClear(ren);
        int rw = 0, rh = 0; SDL_GetRendererOutputSize(ren, &rw, &rh);
        SDL_Rect box = { rw / 8, rh / 4, rw * 3 / 4, rh / 2 };
        SDL_SetRenderDrawColor(ren, 72, 32, 16, 255);
        SDL_RenderFillRect(ren, &box);
        SDL_SetRenderDrawColor(ren, 255, 215, 72, 255);
        SDL_RenderDrawRect(ren, &box);
        SDL_RenderPresent(ren);
        SDL_SetWindowTitle(win, err[0] ? err : "Wild Guns: Frontier Edition - Arrastra tu ROM (.sfc) o pulsa ENTER");
        SDL_Delay(16);
    }
    return false;
}

static bool try_find_initial_rom(int argc, char **argv)
{
    char err[128];
    if (argc > 1 && argv[1][0] != '-' && load_rom_file(argv[1], err, sizeof err)) return true;
    if (cfg.last_rom[0] && load_rom_file(cfg.last_rom, err, sizeof err)) return true;
    const char *names[] = { "Wild Guns (USA).sfc", "Wild Guns (USA).smc", "wildguns.sfc", "wildguns.smc", "rom.sfc", "rom.smc" };
    const char *dirs[] = { base_dir, save_dir, "." };
    for (int d = 0; d < 3; d++) {
        for (int i = 0; i < 6; i++) {
            char p[1024]; snprintf(p, sizeof p, "%s%c%s", dirs[d], PATH_SEP, names[i]);
            if (load_rom_file(p, err, sizeof err)) return true;
        }
    }
    return false;
}

/* ------------------------------------------------------------------ */
/* Self-Test (--test <outdir>)                                          */
/* ------------------------------------------------------------------ */

static void save_bmp(const char *path, const uint32_t *argb)
{
    SDL_Surface *s = SDL_CreateRGBSurfaceFrom((void *)argb, W, H, 32, W * 4,
                                              0x00FF0000u, 0x0000FF00u, 0x000000FFu, 0xFF000000u);
    if (s) { SDL_SaveBMP(s, path); SDL_FreeSurface(s); }
}

static int run_self_test(const char *outdir)
{
    char p[1024];
    state = ST_MENU; page = P_START; sel = 0; draw_menu();
    snprintf(p, sizeof p, "%s%c01_start.bmp", outdir, PATH_SEP); save_bmp(p, ui);

    page = P_CHEATS; sel = 0; cfg.c_weapon = 1; cfg.c_lives = 1; cfg.c_bombs = 1; cfg.c_god = 1; draw_menu();
    snprintf(p, sizeof p, "%s%c02_cheats.bmp", outdir, PATH_SEP); save_bmp(p, ui);

    // 1) Fast Start 1P with Shotgun + 9 Lives + 5 Bombs + Mouse Aim at (205, 55)!
    launch_fast_game(false, false);
    aim_x = 205; aim_y = 55; aim_active = true;
    for (int i = 0; i < 15; i++) { apply_cheats_frame(); step2(1, 0, 0); }
    snprintf(p, sizeof p, "%s%c03_carson_sgun_aim.bmp", outdir, PATH_SEP); save_bmp(p, game);

    // 2) Test Machine Gun (3) & Vulcan (4) live weapon swap
    cfg.c_weapon = 3;
    for (int i = 0; i < 10; i++) { apply_cheats_frame(); step2(1, 0, 0); }
    snprintf(p, sizeof p, "%s%c04_weapon_mgun.bmp", outdir, PATH_SEP); save_bmp(p, game);

    cfg.c_weapon = 4;
    for (int i = 0; i < 10; i++) { apply_cheats_frame(); step2(1, 0, 0); }
    snprintf(p, sizeof p, "%s%c05_weapon_vgun.bmp", outdir, PATH_SEP); save_bmp(p, game);

    // 3) Stage Select launcher!
    launch_fast_game(false, true);
    snprintf(p, sizeof p, "%s%c06_stage_select.bmp", outdir, PATH_SEP); save_bmp(p, game);

    // 4) Classic Mode boot check (Natsume logo intact!)
    launch_classic_boot();
    step2(220, 0, 0);
    snprintf(p, sizeof p, "%s%c07_classic_natsume.bmp", outdir, PATH_SEP); save_bmp(p, game);

    return 0;
}

/* ------------------------------------------------------------------ */
/* Main                                                                 */
/* ------------------------------------------------------------------ */

int main(int argc, char **argv)
{
    bool test_mode = false;
    const char *test_dir = ".";
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--test")) {
            test_mode = true;
            if (i + 1 < argc) test_dir = argv[++i];
        }
    }

    Uint32 init_flags = SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER;
    if (!test_mode) init_flags |= SDL_INIT_AUDIO;
    if (SDL_Init(init_flags) != 0) return 1;

    char *bp = SDL_GetBasePath();
    snprintf(base_dir, sizeof base_dir, "%s", bp ? bp : ".");
    if (bp) SDL_free(bp);
    size_t bl = strlen(base_dir);
    while (bl > 1 && (base_dir[bl - 1] == '/' || base_dir[bl - 1] == '\\')) base_dir[--bl] = 0;

#ifdef __ANDROID__
    const char *ip = SDL_AndroidGetInternalStoragePath();
    snprintf(save_dir, sizeof save_dir, "%s", ip ? ip : ".");
#else
    snprintf(save_dir, sizeof save_dir, "%s", base_dir);
#endif

    load_cfg();

    char core_path[1024];
#ifdef __ANDROID__
    snprintf(core_path, sizeof core_path, "%s", CORE_NAME);
#else
    snprintf(core_path, sizeof core_path, "%s%c%s", base_dir, PATH_SEP, CORE_NAME);
#endif
    if (!load_core(core_path) && !load_core(CORE_NAME)) { SDL_Quit(); return 1; }

    if (!test_mode) {
        win = SDL_CreateWindow("Wild Guns: Frontier Edition",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               W * 3, H * 3, SDL_WINDOW_RESIZABLE);
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        apply_video_settings();
        tex_ui = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, W, H);
    }

    if (!try_find_initial_rom(argc, argv)) {
        if (test_mode) return 2;
        char chosen[1024], err[128];
        if (!pick_rom_dialog(chosen, sizeof chosen) || !load_rom_file(chosen, err, sizeof err)) {
            if (!rom_link_screen()) { core.deinit(); SDL_Quit(); return 0; }
        }
    }
    save_cfg();

    struct retro_game_info gi = { rom_path, rom_buf, rom_len, NULL };
    if (!core.load_game(&gi)) return 3;

    if (!test_mode) {
        struct retro_system_av_info av;
        core.get_av(&av);
        SDL_AudioSpec want, have;
        memset(&want, 0, sizeof want);
        want.freq = (int)(av.timing.sample_rate > 0 ? av.timing.sample_rate : 32040);
        want.format = AUDIO_S16SYS;
        want.channels = 2;
        want.samples = 1024;
        audio = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        if (audio) SDL_PauseAudioDevice(audio, 0);
    }

    boot_and_cache_snapshots();

    if (test_mode) {
        int rc = run_self_test(test_dir);
        core.deinit(); SDL_Quit();
        return rc;
    }

    for (int i = 0; i < SDL_NumJoysticks() && i < 4; i++)
        if (SDL_IsGameController(i)) pads[i] = SDL_GameControllerOpen(i);

    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 next_frame = SDL_GetPerformanceCounter();

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT: running = false; break;
            case SDL_CONTROLLERDEVICEADDED:
                for (int i = 0; i < 4; i++) if (!pads[i]) { pads[i] = SDL_GameControllerOpen(ev.cdevice.which); break; }
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                for (int i = 0; i < 4; i++)
                    if (pads[i] && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pads[i])) == ev.cdevice.which) {
                        SDL_GameControllerClose(pads[i]); pads[i] = NULL;
                    }
                break;
            case SDL_KEYDOWN:
                if (ev.key.repeat && state == ST_PLAY) break;
                if (binding >= 0 && !bind_pad) {
                    if (ev.key.keysym.sym != SDLK_ESCAPE) { cfg.kb[binding] = ev.key.keysym.scancode; save_cfg(); }
                    binding = -1; break;
                }
                if (ev.key.keysym.sym == SDLK_F5 && started)
                    show_toast(save_state(cfg.slot) ? T("ESTADO GUARDADO", "STATE SAVED") : T("ERROR", "ERROR"));
                else if (ev.key.keysym.sym == SDLK_F7) {
                    if (load_state(cfg.slot)) { state = ST_PLAY; show_toast(T("ESTADO CARGADO", "STATE LOADED")); }
                    else show_toast(T("SLOT VACIO", "EMPTY SLOT"));
                } else if (ev.key.keysym.sym == SDLK_F6) {
                    cfg.slot = (cfg.slot + 1) % 3; save_cfg();
                    char m[32]; snprintf(m, sizeof m, "SLOT %d", cfg.slot + 1); show_toast(m);
                } else if (ev.key.keysym.sym == SDLK_F11) {
                    cfg.fullscreen = !cfg.fullscreen; apply_video_settings(); save_cfg();
                } else handle(key_action(ev.key.keysym.sym));
                break;
            case SDL_CONTROLLERBUTTONDOWN:
                pad_active_recently = true; last_pad_tick = SDL_GetTicks();
                if (binding >= 0 && bind_pad) { cfg.gp[binding] = ev.cbutton.button; save_cfg(); binding = -1; break; }
                handle(pad_action(ev.cbutton.button));
                break;
            case SDL_MOUSEMOTION:
                if (state == ST_PLAY && cfg.mouse_aim) {
                    int rw = 0, rh = 0; SDL_GetRendererOutputSize(ren, &rw, &rh);
                    if (rw > 0 && rh > 0) update_aim_from_norm((float)ev.motion.x / rw, (float)ev.motion.y / rh);
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (state == ST_MENU && ev.button.button == SDL_BUTTON_LEFT) {
                    int rw = 0, rh = 0; SDL_GetRendererOutputSize(ren, &rw, &rh);
                    if (rw > 0 && rh > 0) handle_menu_tap((float)ev.button.x / rw, (float)ev.button.y / rh);
                } else if (state == ST_PLAY && cfg.mouse_aim) {
                    int rw = 0, rh = 0; SDL_GetRendererOutputSize(ren, &rw, &rh);
                    if (rw > 0 && rh > 0) update_aim_from_norm((float)ev.button.x / rw, (float)ev.button.y / rh);
                    if (ev.button.button == SDL_BUTTON_LEFT)   mouse_fire_held = true;
                    if (ev.button.button == SDL_BUTTON_RIGHT)  mouse_jump_held = true;
                    if (ev.button.button == SDL_BUTTON_MIDDLE) mouse_bomb_pulse = true;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (ev.button.button == SDL_BUTTON_LEFT)  mouse_fire_held = false;
                if (ev.button.button == SDL_BUTTON_RIGHT) mouse_jump_held = false;
                break;
            case SDL_FINGERDOWN: {
                touch_seen = true;
                float fx = ev.tfinger.x, fy = ev.tfinger.y;
                if (state == ST_MENU) { handle_menu_tap(fx, fy); break; }
                bool special = false;
                for (int i = 0; i < NTOUCH_BTNS; i++) {
                    const tbtn_t *tb = &touch_btns[i];
                    if (tb->btn >= 0) continue;
                    if (fx >= tb->cx - tb->hw && fx <= tb->cx + tb->hw &&
                        fy >= tb->cy - tb->hh && fy <= tb->cy + tb->hh) {
                        if (tb->btn == TACT_MENU) pause_game();
                        else if (tb->btn == TACT_SAVE) show_toast(save_state(cfg.slot) ? T("ESTADO GUARDADO", "STATE SAVED") : T("ERROR", "ERROR"));
                        else if (tb->btn == TACT_LOAD) {
                            if (load_state(cfg.slot)) show_toast(T("ESTADO CARGADO", "STATE LOADED"));
                            else show_toast(T("SLOT VACIO", "EMPTY SLOT"));
                        }
                        special = true; break;
                    }
                }
                if (!special) {
                    bool aim_f = cfg.mouse_aim && !point_in_touch_ui(fx, fy);
                    for (int f = 0; f < MAX_FINGERS; f++)
                        if (!fingers[f].down) { fingers[f] = (finger_t){ ev.tfinger.fingerId, fx, fy, true, aim_f }; break; }
                }
                break;
            }
            case SDL_FINGERMOTION:
                for (int f = 0; f < MAX_FINGERS; f++)
                    if (fingers[f].down && fingers[f].id == ev.tfinger.fingerId) {
                        fingers[f].x = ev.tfinger.x; fingers[f].y = ev.tfinger.y; break;
                    }
                break;
            case SDL_FINGERUP:
                for (int f = 0; f < MAX_FINGERS; f++)
                    if (fingers[f].down && fingers[f].id == ev.tfinger.fingerId) { fingers[f].down = false; break; }
                break;
            }
        }

        poll_stick_menu();

        if (state == ST_PLAY) {
            read_both_pads();
            if ((pad[0] & (1u << RETRO_DEVICE_ID_JOYPAD_L)) &&
                (pad[0] & (1u << RETRO_DEVICE_ID_JOYPAD_R)) &&
                (pad[0] & (1u << RETRO_DEVICE_ID_JOYPAD_START))) {
                pause_game();
            } else {
                apply_cheats_frame();
                core.run();
                total_frames++;
            }
        }

        int rw = 0, rh = 0; SDL_GetRendererOutputSize(ren, &rw, &rh);
        SDL_Rect dst = game_dst_rect(rw, rh);
        SDL_SetRenderDrawColor(ren, 10, 6, 4, 255);
        SDL_RenderClear(ren);

        if (state == ST_PLAY) {
            SDL_UpdateTexture(tex_game, NULL, game, gw * 4);
            SDL_RenderCopy(ren, tex_game, NULL, &dst);
            if (*toast) {
                memset(ui, 0, sizeof ui);
                draw_toast();
                SDL_SetTextureBlendMode(tex_ui, SDL_BLENDMODE_BLEND);
                SDL_UpdateTexture(tex_ui, NULL, ui, W * 4);
                SDL_RenderCopy(ren, tex_ui, NULL, &dst);
            }
            draw_touch_overlay(rw, rh);
        } else {
            draw_menu();
            draw_toast();
            SDL_SetTextureBlendMode(tex_ui, SDL_BLENDMODE_NONE);
            SDL_UpdateTexture(tex_ui, NULL, ui, W * 4);
            SDL_RenderCopy(ren, tex_ui, NULL, &dst);
        }
        SDL_RenderPresent(ren);

        next_frame += freq / 60;
        Uint64 now = SDL_GetPerformanceCounter();
        if (next_frame > now) {
            Uint32 ms = (Uint32)((next_frame - now) * 1000 / freq);
            if (ms > 0) SDL_Delay(ms);
        } else next_frame = now;
    }

    for (int i = 0; i < 4; i++) if (pads[i]) SDL_GameControllerClose(pads[i]);
    if (audio) SDL_CloseAudioDevice(audio);
    core.deinit();
    free(rom_buf); free(title_snap);
    SDL_Quit();
    return 0;
}
