#pragma once

#include "image.h"


struct Arena {
  u8 *data;
  i32 size;
  i32 max_size;
};


#define PUSH_STRUCT(arena, type) \
  (type *)PUSH_SIZE(arena, sizeof(type))
#define PUSH_ARRAY(arena, size, type) \
  (type *)PUSH_SIZE(arena, (size) * sizeof(type))
void *PUSH_SIZE(Arena *arena, i32 size) {
  assert(arena->size + size <= arena->max_size);
  void *result = arena->data + arena->size;
  arena->size += size;
  return result;
}


struct PlatformContext {
  void *state;
};


struct KeyState {
  i32 transitions;
  b32 down;
};


struct GameInput {
  union Keys {
    KeyState data[9];
    struct {
      KeyState kill0;
      KeyState kill1;
      KeyState action0;
      KeyState action1;
      KeyState left0;
      KeyState left1;
      KeyState right0;
      KeyState right1;
      KeyState speed;
    };
  } keys;
  b32 pause;
  f32 dt;
};


#define PLATFORM_READ_FILE(fname) size_t fname(PlatformContext *ctx, const char *name, char *buf, size_t max_len)
typedef PLATFORM_READ_FILE(PlatformReadFile);
#define PLATFORM_WRITE_FILE(fname) void fname(PlatformContext *ctx, const char* name, const char* buf, size_t len)
typedef PLATFORM_WRITE_FILE(PlatformWriteFile);
#define PLATFORM_QUIT(fname) void fname(PlatformContext *ctx)
typedef PLATFORM_QUIT(PlatformQuit);


struct GameMemory {
  Arena arena;
  PlatformReadFile *platform_read_file;
  PlatformWriteFile *platform_write_file;
  PlatformQuit *platform_quit;
};


struct GameAudio {
  f32 *samples;
  i32 rate;
  size_t size;
  size_t max_size;
};


#define GAME_UPDATE_AND_RENDER(fname) void fname(PlatformContext *platform, GameInput *input, GameMemory *memory, Image *screen)
typedef GAME_UPDATE_AND_RENDER(GameUpdateAndRender);
#define GAME_GET_AUDIO(fname) void fname(PlatformContext *platform, GameMemory *memory, GameAudio *audio)
typedef GAME_GET_AUDIO(GameGetAudio);
