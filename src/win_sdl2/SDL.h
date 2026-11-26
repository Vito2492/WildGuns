/* Minimal ABI-exact SDL2 header shim for local MSVC builds against SDL2.dll.
 * Official SDL2 headers are used automatically on Linux, Android, and CI builds. */
#ifndef RANGERS_WIN_SDL2_H
#define RANGERS_WIN_SDL2_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t  Uint8;
typedef int8_t   Sint8;
typedef uint16_t Uint16;
typedef int16_t  Sint16;
typedef uint32_t Uint32;
typedef int32_t  Sint32;
typedef uint64_t Uint64;
typedef int64_t  Sint64;

typedef enum { SDL_FALSE = 0, SDL_TRUE = 1 } SDL_bool;

#define SDL_INIT_AUDIO          0x00000010u
#define SDL_INIT_VIDEO          0x00000020u
#define SDL_INIT_JOYSTICK       0x00000200u
#define SDL_INIT_GAMECONTROLLER 0x00002000u
#define SDL_INIT_EVENTS         0x00004000u

#define SDL_WINDOWPOS_CENTERED  0x2FFF0000u
#define SDL_WINDOW_FULLSCREEN         0x00000001u
#define SDL_WINDOW_RESIZABLE          0x00000020u
#define SDL_WINDOW_FULLSCREEN_DESKTOP (SDL_WINDOW_FULLSCREEN | 0x00001000u)
#define SDL_WINDOW_ALLOW_HIGHDPI      0x00002000u

#define SDL_RENDERER_ACCELERATED 0x00000002u
#define SDL_RENDERER_PRESENTVSYNC 0x00000004u
#define SDL_TEXTUREACCESS_STREAMING 1
#define SDL_BLENDMODE_NONE  0x00000000
#define SDL_BLENDMODE_BLEND 0x00000001
#define SDL_BUTTON_LEFT   1
#define SDL_BUTTON_MIDDLE 2
#define SDL_BUTTON_RIGHT  3
#define SDL_BUTTON_X1     4
#define SDL_BUTTON_X2     5
#define SDL_PIXELFORMAT_ARGB8888 0x16362004u
#define SDL_MESSAGEBOX_ERROR 0x00000010u
#define SDL_HINT_RENDER_SCALE_QUALITY "SDL_RENDER_SCALE_QUALITY"

#define AUDIO_S16LSB 0x8010u
#define AUDIO_S16SYS AUDIO_S16LSB

typedef Uint32 SDL_AudioDeviceID;
typedef Uint16 SDL_AudioFormat;
typedef Sint32 SDL_JoystickID;
typedef Sint32 SDL_Keycode;
typedef Sint64 SDL_TouchID;
typedef Sint64 SDL_FingerID;

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_GameController SDL_GameController;
typedef struct SDL_Joystick SDL_Joystick;
typedef struct SDL_RWops SDL_RWops;

typedef struct SDL_Rect { int x, y, w, h; } SDL_Rect;

typedef struct SDL_Surface {
    Uint32 flags;
    void *format;
    int w, h;
    int pitch;
    void *pixels;
    void *userdata;
    int locked;
    void *list_blitmap;
    SDL_Rect clip_rect;
    void *map;
    int refcount;
} SDL_Surface;

typedef struct SDL_Locale {
    const char *language;
    const char *country;
} SDL_Locale;

typedef void (*SDL_AudioCallback)(void *userdata, Uint8 *stream, int len);
typedef struct SDL_AudioSpec {
    int freq;
    SDL_AudioFormat format;
    Uint8 channels;
    Uint8 silence;
    Uint16 samples;
    Uint16 padding;
    Uint32 size;
    SDL_AudioCallback callback;
    void *userdata;
} SDL_AudioSpec;

typedef enum {
    SDL_SCANCODE_UNKNOWN = 0,
    SDL_SCANCODE_A = 4, SDL_SCANCODE_B = 5, SDL_SCANCODE_C = 6, SDL_SCANCODE_D = 7,
    SDL_SCANCODE_E = 8, SDL_SCANCODE_F = 9, SDL_SCANCODE_G = 10, SDL_SCANCODE_H = 11,
    SDL_SCANCODE_I = 12, SDL_SCANCODE_J = 13, SDL_SCANCODE_K = 14, SDL_SCANCODE_L = 15,
    SDL_SCANCODE_M = 16, SDL_SCANCODE_N = 17, SDL_SCANCODE_O = 18, SDL_SCANCODE_P = 19,
    SDL_SCANCODE_Q = 20, SDL_SCANCODE_R = 21, SDL_SCANCODE_S = 22, SDL_SCANCODE_T = 23,
    SDL_SCANCODE_U = 24, SDL_SCANCODE_V = 25, SDL_SCANCODE_W = 26, SDL_SCANCODE_X = 27,
    SDL_SCANCODE_Y = 28, SDL_SCANCODE_Z = 29,
    SDL_SCANCODE_RETURN = 40,
    SDL_SCANCODE_ESCAPE = 41,
    SDL_SCANCODE_BACKSPACE = 42,
    SDL_SCANCODE_TAB = 43,
    SDL_SCANCODE_SPACE = 44,
    SDL_SCANCODE_F1 = 58, SDL_SCANCODE_F5 = 62, SDL_SCANCODE_F6 = 63, SDL_SCANCODE_F7 = 64, SDL_SCANCODE_F8 = 65, SDL_SCANCODE_F11 = 68,
    SDL_SCANCODE_RIGHT = 79, SDL_SCANCODE_LEFT = 80, SDL_SCANCODE_DOWN = 81, SDL_SCANCODE_UP = 82,
    SDL_SCANCODE_LSHIFT = 225,
    SDL_SCANCODE_RSHIFT = 229,
    SDL_NUM_SCANCODES = 512
} SDL_Scancode;

#define SDLK_SCANCODE_MASK (1 << 30)
#define SDL_SCANCODE_TO_KEYCODE(X) ((X) | SDLK_SCANCODE_MASK)

enum {
    SDLK_RETURN = '\r',
    SDLK_ESCAPE = 27,
    SDLK_BACKSPACE = '\b',
    SDLK_SPACE = ' ',
    SDLK_x = 'x',
    SDLK_z = 'z',
    SDLK_F1 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F1),
    SDLK_F5 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F5),
    SDLK_F6 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F6),
    SDLK_F7 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F7),
    SDLK_F8 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F8),
    SDLK_F11 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F11),
    SDLK_RIGHT = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RIGHT),
    SDLK_LEFT = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LEFT),
    SDLK_DOWN = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_DOWN),
    SDLK_UP = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_UP),
    SDLK_KP_ENTER = SDL_SCANCODE_TO_KEYCODE(88),
    SDLK_AC_BACK = SDL_SCANCODE_TO_KEYCODE(270)
};

#define KMOD_LALT 0x0100
#define KMOD_RALT 0x0200
#define KMOD_ALT  (KMOD_LALT | KMOD_RALT)

typedef enum {
    SDL_CONTROLLER_AXIS_INVALID = -1,
    SDL_CONTROLLER_AXIS_LEFTX = 0,
    SDL_CONTROLLER_AXIS_LEFTY = 1,
    SDL_CONTROLLER_AXIS_RIGHTX = 2,
    SDL_CONTROLLER_AXIS_RIGHTY = 3,
    SDL_CONTROLLER_AXIS_TRIGGERLEFT = 4,
    SDL_CONTROLLER_AXIS_TRIGGERRIGHT = 5
} SDL_GameControllerAxis;

typedef enum {
    SDL_CONTROLLER_BUTTON_INVALID = -1,
    SDL_CONTROLLER_BUTTON_A = 0,
    SDL_CONTROLLER_BUTTON_B = 1,
    SDL_CONTROLLER_BUTTON_X = 2,
    SDL_CONTROLLER_BUTTON_Y = 3,
    SDL_CONTROLLER_BUTTON_BACK = 4,
    SDL_CONTROLLER_BUTTON_GUIDE = 5,
    SDL_CONTROLLER_BUTTON_START = 6,
    SDL_CONTROLLER_BUTTON_LEFTSTICK = 7,
    SDL_CONTROLLER_BUTTON_RIGHTSTICK = 8,
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER = 9,
    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER = 10,
    SDL_CONTROLLER_BUTTON_DPAD_UP = 11,
    SDL_CONTROLLER_BUTTON_DPAD_DOWN = 12,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT = 13,
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT = 14
} SDL_GameControllerButton;

enum {
    SDL_QUIT = 0x100,
    SDL_KEYDOWN = 0x300,
    SDL_KEYUP = 0x301,
    SDL_MOUSEMOTION = 0x400,
    SDL_MOUSEBUTTONDOWN = 0x401,
    SDL_MOUSEBUTTONUP = 0x402,
    SDL_CONTROLLERAXISMOTION = 0x650,
    SDL_CONTROLLERBUTTONDOWN = 0x651,
    SDL_CONTROLLERBUTTONUP = 0x652,
    SDL_CONTROLLERDEVICEADDED = 0x653,
    SDL_CONTROLLERDEVICEREMOVED = 0x654,
    SDL_FINGERDOWN = 0x700,
    SDL_FINGERUP = 0x701,
    SDL_FINGERMOTION = 0x702,
    SDL_DROPFILE = 0x1000
};

#define SDL_TOUCH_MOUSEID ((Uint32)-1)

typedef struct SDL_Keysym {
    SDL_Scancode scancode;
    SDL_Keycode sym;
    Uint16 mod;
    Uint32 unused;
} SDL_Keysym;

typedef struct SDL_KeyboardEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint8 state;
    Uint8 repeat;
    Uint8 padding2;
    Uint8 padding3;
    SDL_Keysym keysym;
} SDL_KeyboardEvent;

typedef struct SDL_ControllerAxisEvent {
    Uint32 type;
    Uint32 timestamp;
    SDL_JoystickID which;
    Uint8 axis;
    Uint8 padding1, padding2, padding3;
    Sint16 value;
    Uint16 padding4;
} SDL_ControllerAxisEvent;

typedef struct SDL_ControllerButtonEvent {
    Uint32 type;
    Uint32 timestamp;
    SDL_JoystickID which;
    Uint8 button;
    Uint8 state;
    Uint8 padding1, padding2;
} SDL_ControllerButtonEvent;

typedef struct SDL_ControllerDeviceEvent {
    Uint32 type;
    Uint32 timestamp;
    Sint32 which;
} SDL_ControllerDeviceEvent;

typedef struct SDL_MouseMotionEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint32 which;
    Uint32 state;
    Sint32 x;
    Sint32 y;
    Sint32 xrel;
    Sint32 yrel;
} SDL_MouseMotionEvent;

typedef struct SDL_MouseButtonEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint32 which;
    Uint8 button;
    Uint8 state;
    Uint8 clicks;
    Uint8 padding1;
    Sint32 x;
    Sint32 y;
} SDL_MouseButtonEvent;

typedef struct SDL_TouchFingerEvent {
    Uint32 type;
    Uint32 timestamp;
    SDL_TouchID touchId;
    SDL_FingerID fingerId;
    float x;
    float y;
    float dx;
    float dy;
    float pressure;
    Uint32 windowID;
} SDL_TouchFingerEvent;

typedef struct SDL_DropEvent {
    Uint32 type;
    Uint32 timestamp;
    char *file;
    Uint32 windowID;
} SDL_DropEvent;

typedef union SDL_Event {
    Uint32 type;
    SDL_KeyboardEvent key;
    SDL_MouseMotionEvent motion;
    SDL_MouseButtonEvent button;
    SDL_ControllerAxisEvent caxis;
    SDL_ControllerButtonEvent cbutton;
    SDL_ControllerDeviceEvent cdevice;
    SDL_TouchFingerEvent tfinger;
    SDL_DropEvent drop;
    Uint8 padding[56];
} SDL_Event;

#define SDL_TICKS_PASSED(A, B) ((Sint32)((B) - (A)) <= 0)

extern int SDL_Init(Uint32 flags);
extern void SDL_Quit(void);
extern const char *SDL_GetError(void);
extern void SDL_free(void *mem);
extern char *SDL_GetBasePath(void);
extern char *SDL_GetPrefPath(const char *org, const char *app);
extern SDL_Locale *SDL_GetPreferredLocales(void);
extern int SDL_ShowSimpleMessageBox(Uint32 flags, const char *title, const char *message, SDL_Window *window);
extern SDL_bool SDL_SetHint(const char *name, const char *value);

extern void *SDL_LoadObject(const char *sofile);
extern void *SDL_LoadFunction(void *handle, const char *name);

extern SDL_Window *SDL_CreateWindow(const char *title, int x, int y, int w, int h, Uint32 flags);
extern void SDL_SetWindowTitle(SDL_Window *window, const char *title);
extern int SDL_SetWindowFullscreen(SDL_Window *window, Uint32 flags);
extern SDL_Renderer *SDL_CreateRenderer(SDL_Window *window, int index, Uint32 flags);
extern int SDL_GetRendererOutputSize(SDL_Renderer *renderer, int *w, int *h);
extern int SDL_RenderSetLogicalSize(SDL_Renderer *renderer, int w, int h);
extern int SDL_SetRenderDrawBlendMode(SDL_Renderer *renderer, int blendMode);
extern int SDL_RenderFillRect(SDL_Renderer *renderer, const SDL_Rect *rect);
extern int SDL_RenderDrawRect(SDL_Renderer *renderer, const SDL_Rect *rect);
extern int SDL_RenderDrawLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2);
extern SDL_Texture *SDL_CreateTexture(SDL_Renderer *renderer, Uint32 format, int access, int w, int h);
extern void SDL_DestroyTexture(SDL_Texture *texture);
extern int SDL_SetTextureBlendMode(SDL_Texture *texture, int blendMode);
extern int SDL_UpdateTexture(SDL_Texture *texture, const SDL_Rect *rect, const void *pixels, int pitch);
extern int SDL_SetRenderDrawColor(SDL_Renderer *renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
extern int SDL_RenderClear(SDL_Renderer *renderer);
extern int SDL_RenderCopy(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *srcrect, const SDL_Rect *dstrect);
extern void SDL_RenderPresent(SDL_Renderer *renderer);

extern SDL_Surface *SDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth, int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
extern SDL_Surface *SDL_CreateRGBSurfaceWithFormatFrom(void *pixels, int width, int height, int depth, int pitch, Uint32 format);
extern void SDL_FreeSurface(SDL_Surface *surface);
extern SDL_RWops *SDL_RWFromFile(const char *file, const char *mode);
extern int SDL_SaveBMP_RW(SDL_Surface *surface, SDL_RWops *dst, int freedst);
#define SDL_SaveBMP(surface, file) SDL_SaveBMP_RW((surface), SDL_RWFromFile((file), "wb"), 1)

extern SDL_AudioDeviceID SDL_OpenAudioDevice(const char *device, int iscapture, const SDL_AudioSpec *desired, SDL_AudioSpec *obtained, int allowed_changes);
extern void SDL_CloseAudioDevice(SDL_AudioDeviceID dev);
extern void SDL_PauseAudioDevice(SDL_AudioDeviceID dev, int pause_on);
extern int SDL_QueueAudio(SDL_AudioDeviceID dev, const void *data, Uint32 len);
extern Uint32 SDL_GetQueuedAudioSize(SDL_AudioDeviceID dev);
extern void SDL_ClearQueuedAudio(SDL_AudioDeviceID dev);

extern int SDL_PollEvent(SDL_Event *event);
extern const Uint8 *SDL_GetKeyboardState(int *numkeys);
extern const char *SDL_GetScancodeName(SDL_Scancode scancode);
extern int SDL_NumJoysticks(void);
extern SDL_bool SDL_IsGameController(int joystick_index);
extern SDL_GameController *SDL_GameControllerOpen(int joystick_index);
extern void SDL_GameControllerClose(SDL_GameController *gamecontroller);
extern const char *SDL_GameControllerGetStringForButton(SDL_GameControllerButton button);
extern SDL_Joystick *SDL_GameControllerGetJoystick(SDL_GameController *gamecontroller);
extern SDL_JoystickID SDL_JoystickInstanceID(SDL_Joystick *joystick);
extern Uint8 SDL_GameControllerGetButton(SDL_GameController *gamecontroller, SDL_GameControllerButton button);
extern Sint16 SDL_GameControllerGetAxis(SDL_GameController *gamecontroller, SDL_GameControllerAxis axis);

extern Uint32 SDL_GetTicks(void);
extern void SDL_Delay(Uint32 ms);
extern Uint64 SDL_GetPerformanceCounter(void);
extern Uint64 SDL_GetPerformanceFrequency(void);

#ifdef __cplusplus
}
#endif
#endif
