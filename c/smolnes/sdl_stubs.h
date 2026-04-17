#ifndef SDL_STUBS_H
#define SDL_STUBS_H

#include <stddef.h>
#include <stdint.h>

// SDL constants
#define SDL_INIT_VIDEO 0x00000020
#define SDL_WINDOW_SHOWN 0x00000004
#define SDL_RENDERER_PRESENTVSYNC 0x00000004
#define SDL_TEXTUREACCESS_STREAMING 0x00000001
#define SDL_PIXELFORMAT_BGR565 0x15151002

// SDL event types
#define SDL_QUIT 0x100

// SDL scancodes
enum {
  SDL_SCANCODE_X = 27,
  SDL_SCANCODE_Z = 29,
  SDL_SCANCODE_TAB = 43,
  SDL_SCANCODE_RETURN = 40,
  SDL_SCANCODE_UP = 82,
  SDL_SCANCODE_DOWN = 81,
  SDL_SCANCODE_LEFT = 80,
  SDL_SCANCODE_RIGHT = 79,
};

// SDL types
typedef struct SDL_RWops SDL_RWops;
typedef union SDL_Event {
  uint32_t type;
  uint8_t padding[56];
} SDL_Event;

// SDL function declarations
SDL_RWops *SDL_RWFromFile(const char *file, const char *mode);
size_t SDL_RWread(SDL_RWops *context, void *ptr, size_t size, size_t maxnum);
int SDL_Init(uint32_t flags);
uint8_t *SDL_GetKeyboardState(int *numkeys);
void *SDL_CreateWindow(const char *title, int x, int y, int w, int h,
                       uint32_t flags);
void *SDL_CreateRenderer(void *window, int index, uint32_t flags);
void *SDL_CreateTexture(void *renderer, uint32_t format, int access, int w,
                        int h);
int SDL_UpdateTexture(void *texture, const void *rect, const void *pixels,
                      int pitch);
int SDL_RenderCopy(void *renderer, void *texture, const void *srcrect,
                   const void *dstrect);
void SDL_RenderPresent(void *renderer);
int SDL_PollEvent(SDL_Event *event);

#endif // SDL_STUBS_H
