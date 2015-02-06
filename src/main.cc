#include <assert.h>
#include <stdio.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include "base.h"
#include "shared.h"
#include "main.h"

#include "image.cc"
#include "path.cc"
#include "synth.cc"


void copy_flat_from_ring(
  f32 *dst, f32 *src_data, i32 src_i, i32 src_max_size, i32 count) {
  src_i = src_i % src_max_size;
  i32 src_chunk_size = src_max_size - src_i;
  while(count > src_chunk_size) {
    memcpy(dst, src_data + src_i, src_chunk_size * sizeof(f32));
    dst += src_chunk_size;
    count -= src_chunk_size;
    src_i = 0;
    src_chunk_size = src_max_size;
  }
  memcpy(dst, src_data + src_i, count * sizeof(f32));
}


void copy_ring_from_flat(
  f32 *dst_data, i32 dst_i, i32 dst_max_size, f32 *src, i32 count) {
  dst_i = dst_i % dst_max_size;
  i32 dst_chunk_size = dst_max_size - dst_i;
  while(count > dst_chunk_size) {
    memcpy(dst_data + dst_i, src, dst_chunk_size * sizeof(f32));
    src += dst_chunk_size;
    count -= dst_chunk_size;
    dst_i = 0;
    dst_chunk_size = dst_max_size;
  }
  memcpy(dst_data + dst_i, src, count * sizeof(f32));
}


void audio_callback(void *userdata, u8 *audiodata, i32 len) {
  PlatformState *state = (PlatformState *)userdata;
  f32 *dst = (f32 *)audiodata;
  size_t count = len / sizeof(f32);
  copy_flat_from_ring(
    dst, state->audio_ring, state->audio_read, state->audio_max_size, count);
  state->audio_read += count;
}


f64 timestamp() {
  return (f64)SDL_GetPerformanceCounter() / (f64)SDL_GetPerformanceFrequency();
}


#ifdef CAP_LOAD_DYNAMIC
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>


i64 get_file_timestamp(const char *path) {
  struct stat buf;
  if(stat(path, &buf) == 0) {
    return buf.st_mtimespec.tv_sec;
  }
  return 0;
}


void load_game_code(GameLib *lib) {
  if(lib->handle) dlclose(lib->handle);
  lib->file_timestamp = get_file_timestamp(lib->path);
  lib->handle = dlopen(lib->path, RTLD_LAZY | RTLD_LOCAL);

  lib->game_update_and_render =
    (GameUpdateAndRender *)dlsym(lib->handle, "game_update_and_render");
  assert(lib->game_update_and_render);
  lib->game_get_audio = (GameGetAudio *)dlsym(lib->handle, "game_get_audio");
  assert(lib->game_get_audio);
}


void reload_game_code(GameLib *lib) {
  if(get_file_timestamp(lib->path) != lib->file_timestamp) {
    if(access(lib->path, F_OK) != -1) {
      load_game_code(lib);
    }
  }
}
#else
#include "game.cc"


void load_game_code(GameLib *lib) {
  lib->game_update_and_render = &game_update_and_render;
  lib->game_get_audio = &game_get_audio;
}


void reload_game_code(GameLib *lib) {
  // nothing to do
}
#endif


void create_window(PlatformState *state, v2i screen_shape) {
  if(state->texture_id) {
    glDeleteTextures(1, &state->texture_id);
    state->texture_id = 0;
  }
  if(state->context) {
    SDL_GL_DeleteContext(state->context);
    state->context = 0;
  }
  if(state->window) {
    SDL_DestroyWindow(state->window);
    state->window = 0;
  }

  i32 window_flags = 0;
  window_flags |= SDL_WINDOW_OPENGL;
#ifdef CAP_DEBUG
#ifdef _WIN32
  v2i window_position = {8, 24};
#else
  v2i window_position = {0, 0};
#endif
#else
  v2i window_position = {
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED};
#endif
  state->window = SDL_CreateWindow(
    cap_str_def(CAP_PACKAGE),
    window_position[0], window_position[1],
    state->window_shape[0], state->window_shape[1],
    window_flags);
  if(state->window) {
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    state->context = SDL_GL_CreateContext(state->window);
    if(state->context) {
      SDL_GL_SetSwapInterval(1);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glTranslatef(-1, -1, 1);
      glScalef(2. / screen_shape[0], 2. / screen_shape[1], 1);
      glClearColor(1.f, 0.f, 1.f, 1.f);

      glGenTextures(1, &state->texture_id);
      glBindTexture(GL_TEXTURE_2D, state->texture_id);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
  }
}


void draw_rect_vertices(f32 x, f32 y, f32 w, f32 h) {
  glVertex2f(x, y);
  glVertex2f(x + w, y);
  glVertex2f(x + w, y + h);
  glVertex2f(x, y + h);
}


void draw_horizon(f64 *values, i64 values_size, i64 head) {
  static f32 colors[6][3] = {
    {.7, .9, .7},
    {.5, .8, .5},
    {.3, .7, .3},

    {.9, .7, .7},
    {.8, .5, .5},
    {.6, .3, .3},
  };

  b32 flip = true;

  glBegin(GL_QUADS); {
    glColor3f(1, 1, 1);
    draw_rect_vertices(0, 0, values_size, 1);
    for(i32 i = 1; i < values_size; ++i) {
      f64 dip, fp;
      f64 value = values[i];
      i32 co = 0;
      b32 negative = false;
      f64 y, h;
      if(value < 0) {
        value *= -1;
        co = 3;
        negative = true;
      }
      fp = modf(value, &dip);
      if(dip > 2) {
        dip = 2;
        fp = 1;
      }
      if(flip && negative) {
        y = 1 - fp;
        h = fp;
      } else {
        y = 0;
        h = fp;
      }
      i32 ip = dip;
      if(ip > 0) {
        i32 c = co + ip - 1;
        glColor3f(colors[c][0], colors[c][1], colors[c][2]);
        draw_rect_vertices(i, 0, 1, 1);
      }
      i32 c = co + ip;
      glColor3f(colors[c][0], colors[c][1], colors[c][2]);
      draw_rect_vertices(i, y, 1, h);
    }
    glColor3f(0, 0, 0);
    draw_rect_vertices(head + 1, 0, 1, 1);
  } glEnd();
}


void update_key(KeyState *key, b32 down) {
  if(key->down != down) {
    key->transitions++;
    key->down = down;
  }
}


PLATFORM_READ_FILE(platform_read_file) {
  PlatformState *state = (PlatformState *)ctx->state;

  const size_t path_size = 4096;
  char path[path_size];
  sprintf(path, "%s%s", state->path, name);
  cap_log_debug("reading %s\n", path);
  size_t num = 0;
  SDL_RWops *fd = SDL_RWFromFile(path, "rb");
  if(fd) {
    num = SDL_RWread(fd, buf, 1, max_len);
    SDL_RWclose(fd);
  }
  return num;
}


PLATFORM_WRITE_FILE(platform_write_file) {
  PlatformState *state = (PlatformState *)ctx->state;

  const size_t path_size = 4096;
  char path[path_size];
  sprintf(path, "%s%s", state->path, name);
  cap_log_debug("writing %s\n", path);
  SDL_RWops *fd = SDL_RWFromFile(path, "wb");
  if(fd) {
    SDL_RWwrite(fd, buf, len, 1);
    SDL_RWclose(fd);
  }
}


PLATFORM_QUIT(platform_quit) {
  PlatformState *state = (PlatformState *)ctx->state;
  state->quit = true;
}


int main(int argc, char *argv[]) {
  printf(
    "%s v%s %s\n",
    cap_str_def(CAP_PACKAGE),
    cap_str_def(CAP_VERSION),
    cap_str_def(CAP_COMMIT));

  Arena _arena{};
  Arena *arena = &_arena;
  arena->max_size = cap_megabytes(64);
  arena->data = (u8 *)malloc(arena->max_size);
  memset(arena->data, 0, arena->max_size);
  if(!arena->data) {
    cap_log_error("failed to allocate %d bytes\n", arena->max_size);
    return 1;
  }

  GameLib lib{};
#ifdef CAP_DEBUG
  char buf[512];
  strcpy(buf, cap_str_def(CAP_EXECUTABLE));
  strcpy(buf + strlen(buf), ".dylib");
  lib.path = buf;
#endif
  load_game_code(&lib);

  PlatformState state{};
  v2i map_shape{10, 12};
  v2i tile_shape{12, 6};
  v2i screen_shape = (map_shape - v2i{1, 0}).cwiseProduct(tile_shape);
  i32 screen_scale = 4;
  state.window_shape = screen_shape * screen_scale;

  i32 metrics_size = 240;
  f64 *audio_frames_buffered = PUSH_ARRAY(arena, metrics_size, f64);
  f64 *audio_frames_created = PUSH_ARRAY(arena, metrics_size, f64);
  f64 *frame_error = PUSH_ARRAY(arena, metrics_size, f64);

  PlatformContext platform{};
  platform.state = &state;

  GameInput input{};
  f32 target_fps = 60;
  input.dt = 1. / target_fps;

  GameMemory memory{};
  memory.arena.max_size = cap_megabytes(16);
  memory.arena.data = PUSH_ARRAY(arena, memory.arena.max_size, u8);
  memory.platform_read_file = &platform_read_file;
  memory.platform_write_file = &platform_write_file;
  memory.platform_quit = &platform_quit;

  Image screen{};
  screen.shape = screen_shape;
  screen.data = PUSH_ARRAY(arena, screen.shape.prod(), c4f);

  ImageC4u8 native_screen{};
  native_screen.shape = screen_shape;
  native_screen.data = PUSH_ARRAY(arena, native_screen.shape.prod(), c4u8);

  GameAudio audio{};
  audio.rate = 48000;
  audio.max_size = audio.rate;
  audio.samples = PUSH_ARRAY(arena, audio.max_size, f32);

#ifdef CAP_DEBUG
  Replay replay{};
  replay.memory.arena.max_size = memory.arena.max_size;
  replay.memory.arena.data = PUSH_ARRAY(
    arena, replay.memory.arena.max_size, u8);
  replay.max_size = target_fps * 60 * 60;
  replay.inputs = PUSH_ARRAY(arena, replay.max_size, GameInput);

  enum {
    REPLAY_IDLE,
    REPLAY_RECORDING,
    REPLAY_PLAYING,
  } replay_state = REPLAY_IDLE;

  i32 replay_playback_index;
#endif

  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
    cap_log_error("sdl error: %s", SDL_GetError());
    return 1;
  }

  state.path = SDL_GetPrefPath("", cap_str_def(CAP_PACKAGE));
  if(!state.path) {
    cap_log_error("sdl error: %s", SDL_GetError());
    return 1;
  }

  create_window(&state, screen.shape);
  if(!state.window) {
    cap_log_error("sdl error: %s", SDL_GetError());
    return 1;
  }

  // audio
  {
    SDL_AudioSpec audio_spec;
    audio_spec.freq = audio.rate;
    audio_spec.format = AUDIO_F32;
    audio_spec.channels = 1;
    audio_spec.samples = 512; // <512 causes scratchiness on windows
    audio_spec.callback = audio_callback;
    audio_spec.userdata = &state;

    size_t size = audio_spec.freq;
    state.audio_ring = PUSH_ARRAY(arena, size, f32);
    state.audio_max_size = size;
    state.audio_read = 0;
    state.audio_write = 0;

    if(SDL_OpenAudio(&audio_spec, 0) != 0) {
      cap_log_error("sdl error: %s", SDL_GetError());
      return 1;
    }
    SDL_PauseAudio(0);
  }

  // loop
  b32 paused = false;
  b32 control = false;
  b32 metrics = false;

  f64 total_frame_error = 0;
  i64 ticks = 0;
  i64 mix_ticks = 0;
  f64 tick_begin = timestamp();

  while(!state.quit) {
    for(i32 i = 0; i < cap_array_size(input.keys.data); i++) {
      input.keys.data[i].transitions = 0;
    }
    input.pause = false;

    SDL_Event event;
    b32 toggle_metrics = false;
    b32 toggle_paused = false;
    b32 toggle_replay = false;
    b32 reset_memory = false;
    b32 step = false;
    b32 next_scale = false;
    while(SDL_PollEvent(&event)) {
      if(event.type == SDL_QUIT) state.quit = true;
      if(event.type == SDL_WINDOWEVENT) {
        switch(event.window.event) {
        case SDL_WINDOWEVENT_FOCUS_LOST: input.pause = true; break;
        default: break;
        }
      }
      if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        b32 down = (event.type == SDL_KEYDOWN);
        switch(event.key.keysym.scancode) {
        case SDL_SCANCODE_LCTRL:
        case SDL_SCANCODE_RCTRL: control = down; break;
        default: break;
        }
        if(control && down) {
          switch(event.key.keysym.scancode) {
          case SDL_SCANCODE_Q: state.quit = true; break;
          case SDL_SCANCODE_P: toggle_paused = true; break;
          case SDL_SCANCODE_L: toggle_replay = true; break;
          case SDL_SCANCODE_M: toggle_metrics = true; break;
          case SDL_SCANCODE_R: reset_memory = true; break;
          default: break;
          }
        }
        GameInput::Keys *keys = &input.keys;
        switch(event.key.keysym.scancode) {
        case SDL_SCANCODE_K: update_key(&keys->kill0, down); break;
        case SDL_SCANCODE_ESCAPE: update_key(&keys->kill1, down); break;
        case SDL_SCANCODE_A: update_key(&keys->left0, down); break;
        case SDL_SCANCODE_LEFT: update_key(&keys->left1, down); break;
        case SDL_SCANCODE_D: update_key(&keys->right0, down); break;
        case SDL_SCANCODE_RIGHT: update_key(&keys->right1, down); break;
        case SDL_SCANCODE_SPACE: update_key(&keys->action0, down); break;
        case SDL_SCANCODE_RETURN: update_key(&keys->action1, down); break;
        case SDL_SCANCODE_1: update_key(&keys->speed, down); break;
        case SDL_SCANCODE_O: step = down; break;
        case SDL_SCANCODE_9: next_scale = down; break;
        default: break;
        }
      }
    }
    if(state.quit) break;
    if(next_scale) {
      screen_scale *= 2;
      if(screen_scale > 8) screen_scale = 1;
      state.window_shape = screen_shape * screen_scale;
      create_window(&state, screen.shape);
      if(!state.window) {
        cap_log_error("sdl error: %s", SDL_GetError());
        return 1;
      }
    }

#ifdef CAP_DEBUG
    if(toggle_metrics) metrics = !metrics;
    if(toggle_paused) {
      paused = !paused;
    }
    if(step) {
      paused = true;
    }
    reload_game_code(&lib);
    if(toggle_replay) {
      if(replay_state == REPLAY_IDLE) {
        memcpy(replay.memory.arena.data, memory.arena.data,
               memory.arena.max_size);
        replay.memory.arena.size = memory.arena.size;
        replay.size = 0;
        replay_state = REPLAY_RECORDING;
      } else if(replay_state == REPLAY_RECORDING) {
        memcpy(memory.arena.data, replay.memory.arena.data,
               replay.memory.arena.max_size);
        memory.arena.size = replay.memory.arena.size;
        replay_playback_index = 0;
        replay_state = REPLAY_PLAYING;
      } else if(replay_state == REPLAY_PLAYING) {
        replay_state = REPLAY_IDLE;
      }
    }
    if(reset_memory) {
      replay_state = REPLAY_IDLE;
      cap_zero_array(memory.arena.data, memory.arena.max_size);
      memory.arena.size = 0;
    }
    if(!paused || step) {
      if(replay_state == REPLAY_RECORDING) {
        replay.inputs[replay.size++] = input;
      } else if(replay_state == REPLAY_PLAYING) {
        if(replay_playback_index == replay.size) {
          memcpy(memory.arena.data, replay.memory.arena.data,
                 replay.memory.arena.max_size);
          memory.arena.size = replay.memory.arena.size;
          replay_playback_index = 0;
        }
        input = replay.inputs[replay_playback_index++];
      }
    }
#endif

    if(!paused || step) {
      lib.game_update_and_render(&platform, &input, &memory, &screen);

      i32 buffered = state.audio_write - state.audio_read;
      i32 target_buffered = 2000;
      if(buffered < 0) {
        audio.size = 800;
        state.audio_write = state.audio_read + target_buffered - audio.size;
      } else {
        audio.size = target_buffered - buffered;
      }

      audio_frames_buffered[mix_ticks % metrics_size] =
        buffered / 800.f;
      audio_frames_created[mix_ticks % metrics_size] =
        audio.size / 800.f;
      ++mix_ticks;

      lib.game_get_audio(&platform, &memory, &audio);
      SDL_LockAudio(); {
        copy_ring_from_flat(
          state.audio_ring, state.audio_write, state.audio_max_size,
          audio.samples, audio.size);
        state.audio_write += audio.size;
      } SDL_UnlockAudio();
    }

    draw_copy(&native_screen, {0, 0}, &screen);

    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(-1, -1, 1);
    glScalef(2. / native_screen.shape[0], 2. / native_screen.shape[1], 1);
    glBindTexture(GL_TEXTURE_2D, state.texture_id);
    glTexImage2D(
      GL_TEXTURE_2D,
      0, // level
      GL_RGBA,
      native_screen.shape[0],
      native_screen.shape[1],
      0, // border
      GL_RGBA,
      GL_UNSIGNED_BYTE,
      native_screen.data);

    glEnable(GL_TEXTURE_2D); {
      glColor3f(1, 1, 1); // important because of default texture masking
      glBindTexture(GL_TEXTURE_2D, state.texture_id);
      glBegin(GL_QUADS); {
        v2i ss = native_screen.shape;
        glTexCoord2f(0, 0); glVertex2f(0, 0);
        glTexCoord2f(1, 0); glVertex2f(ss[0], 0);
        glTexCoord2f(1, 1); glVertex2f(ss[0], ss[1]);
        glTexCoord2f(0, 1); glVertex2f(0, ss[1]);
      } glEnd();
    } glDisable(GL_TEXTURE_2D);

    if(metrics) {
      glMatrixMode(GL_MODELVIEW);

      i32 y = 0;

      glLoadIdentity();
      glTranslatef(-1, -1, 1);
      glScalef(4. / state.window_shape[0], 100. / state.window_shape[1], 1);
      glTranslatef(0, y++, 0);
      draw_horizon(
        audio_frames_buffered, metrics_size, mix_ticks % metrics_size);

      glLoadIdentity();
      glTranslatef(-1, -1, 1);
      glScalef(4. / state.window_shape[0], 100. / state.window_shape[1], 1);
      glTranslatef(0, y++, 0);
      draw_horizon(
        audio_frames_created, metrics_size, mix_ticks % metrics_size);

      glLoadIdentity();
      glTranslatef(-1, -1, 1);
      glScalef(4. / state.window_shape[0], 100. / state.window_shape[1], 1);
      glTranslatef(0, y++, 0);
      draw_horizon(frame_error, metrics_size, ticks % metrics_size);
    }

    SDL_GL_SwapWindow(state.window);

    ++ticks;

    f64 tick_delay_buffer = 1.5 / 1000.;
    f64 tick_elapsed = timestamp() - tick_begin;
    f64 tick_remaining = input.dt - tick_elapsed;
    if(tick_remaining - tick_delay_buffer > 0) {
      SDL_Delay(1000 * (tick_remaining - tick_delay_buffer));
    }
    while(timestamp() - tick_begin < input.dt);

    f64 tick_time = timestamp() - tick_begin;
    total_frame_error += tick_time - input.dt;
    frame_error[ticks % metrics_size] = fmod(total_frame_error / input.dt, 3);
    tick_begin = timestamp();
  }

  return 0;
}
