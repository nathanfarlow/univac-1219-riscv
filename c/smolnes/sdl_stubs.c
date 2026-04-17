#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// SDL2 stub implementations for smolnes

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

// Stub data
static uint8_t key_state[512] = {0};
static FILE *rom_file = NULL;

// SDL_RWops stub
struct SDL_RWops {
  FILE *fp;
};

SDL_RWops *SDL_RWFromFile(const char *file, const char *mode) {
  static SDL_RWops rw;
  rw.fp = fopen(file, mode);
  rom_file = rw.fp;
  return rw.fp ? &rw : NULL;
}

size_t SDL_RWread(SDL_RWops *context, void *ptr, size_t size, size_t maxnum) {
  if (!context || !context->fp)
    return 0;
  return fread(ptr, size, maxnum, context->fp);
}

int SDL_Init(uint32_t flags) {
  (void)flags;
  return 0; // success
}

uint8_t *SDL_GetKeyboardState(int *numkeys) {
  if (numkeys)
    *numkeys = 512;
  return key_state;
}

void *SDL_CreateWindow(const char *title, int x, int y, int w, int h,
                       uint32_t flags) {
  (void)title;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)flags;
  static int dummy_window = 1;
  return &dummy_window;
}

void *SDL_CreateRenderer(void *window, int index, uint32_t flags) {
  (void)window;
  (void)index;
  (void)flags;
  static int dummy_renderer = 1;
  return &dummy_renderer;
}

void *SDL_CreateTexture(void *renderer, uint32_t format, int access, int w,
                        int h) {
  (void)renderer;
  (void)format;
  (void)access;
  (void)w;
  (void)h;
  static int dummy_texture = 1;
  return &dummy_texture;
}

// Convert BGR565 color to grayscale brightness (0-255)
static uint8_t bgr565_to_brightness(uint16_t color) {
  // Extract RGB components from BGR565 and approximate luminance directly
  // Using shifted values to avoid expensive division
  // 0.299*R + 0.587*G + 0.114*B approximated with shifts
  uint16_t b = (color >> 11) & 0x1F; // 5 bits
  uint16_t g = (color >> 5) & 0x3F;  // 6 bits
  uint16_t r = color & 0x1F;         // 5 bits

  // Approximate: (r*2 + g*4 + b) which roughly follows luminance weights
  return ((r << 1) + (g << 2) + b) << 1;
}

// ASCII characters from darkest to brightest (reduced for speed)
static const char ascii_gradient[] = " .=*@";
static const int gradient_len = sizeof(ascii_gradient) - 1;

int SDL_UpdateTexture(void *texture, const void *rect, const void *pixels,
                      int pitch) {
  (void)texture;
  (void)rect;

  const uint16_t *fb = (const uint16_t *)pixels;
  static int frame_count = 0;
  frame_count++;

  // Only render every Nth frame to avoid overwhelming output
  /* if (++frame_count % 2 != 0) { */
  /*     return 0; */
  /* } */

  // Frame is already downsampled to 64x28
  const int width = 64;
  const int height = 28;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      uint16_t pixel = fb[y * width + x];
      uint8_t brightness = bgr565_to_brightness(pixel);
      int idx = (brightness * gradient_len) / 256;
      if (idx >= gradient_len)
        idx = gradient_len - 1;
      printf("%c", ascii_gradient[idx]);
    }
    printf("\n");
  }
  fflush(stdout);

  return 0; // success
}

int SDL_RenderCopy(void *renderer, void *texture, const void *srcrect,
                   const void *dstrect) {
  (void)renderer;
  (void)texture;
  (void)srcrect;
  (void)dstrect;
  return 0; // success
}

void SDL_RenderPresent(void *renderer) {
  (void)renderer;
  // Do nothing for now
}

int SDL_PollEvent(SDL_Event *event) {
  static int total_frames = 0;
  const int MAX_FRAMES = 7; // Run for ~5 seconds at 60fps

  total_frames++;
  if (total_frames >= MAX_FRAMES) {
    if (event) {
      event->type = SDL_QUIT;
    }
    return 1; // event available
  }

  return 0; // no events
}

// File operation stubs (needed for fopen)
int open(const char *pathname, int flags, ...) {
  (void)pathname;
  (void)flags;
  return -1; // fail
}

ssize_t read(int fd, void *buf, size_t count) {
  (void)fd;
  (void)buf;
  (void)count;
  return -1; // fail
}

ssize_t write(int fd, const void *buf, size_t count) {
  (void)fd;
  (void)buf;
  (void)count;
  return -1; // fail
}

int close(int fd) {
  (void)fd;
  return -1; // fail
}

off_t lseek(int fd, off_t offset, int whence) {
  (void)fd;
  (void)offset;
  (void)whence;
  return -1; // fail
}
