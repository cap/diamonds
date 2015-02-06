struct PlatformState {
  v2i window_shape;
  SDL_Window *window;
  SDL_GLContext context;
  GLuint texture_id;

  f32 *audio_ring;
  i32 audio_max_size;
  i32 audio_read;
  i32 audio_write;

  b32 quit;

  char *path;
};


struct GameLib {
  char *path;
  i64 file_timestamp;
  void *handle;

  GameUpdateAndRender *game_update_and_render;
  GameGetAudio *game_get_audio;
};


struct Replay {
  GameMemory memory;
  GameInput *inputs;
  i32 size;
  i32 max_size;
};
