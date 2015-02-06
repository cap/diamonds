#pragma once

#include "audio.h"


struct Tile {
  enum {
    EMPTY,
    BORDER,
    SHADOW,
    DIAMOND,
    INERT,
    KEY,
    LOCK,
    KILL,
    SWITCH,
    COLOR,
    PAINT,
  } type;
  i32 color;
};


struct Map {
  Tile *tiles;
  v2i shape;
  v2i start;
};


struct Particles {
  i32 size;
  f32 *t;
  v2f *p;
  v2f *v;
  v2f *a;
  c4f *c;
};


struct Record {
  i32 bounces;
  f32 time;
};


struct Save {
  i32 map_index;
  Record records[cap_array_size(global_maps)];
};


struct Scoreboard {
  char best_bounces[8];
  char your_bounces[8];
  b32 new_bounces_record;

  char best_time[8];
  char your_time[8];
  b32 new_time_record;
};


struct Colors {
  c4f bg;
  c4f shadow;
  c4f fg;
};


struct GameState {
  Arena tmp;
  i64 ticks;

  enum {
    MENU,
    GAME,
    PAUSED,
    SCOREBOARD,
  } state;

  Colors colors;

  Image glyphs[cap_array_size(global_glyphs)];
  Image tiles[cap_array_size(global_tiles)];
  Image balls[cap_array_size(global_balls)];
  Image minimaps[cap_array_size(global_maps)];

  v2i tile_shape;
  v2i minimap_shape;

  i32 map_index;
  Map map;

  Record records[cap_array_size(global_maps)];
  Scoreboard scoreboard;
  b32 scoreboard_configured;
  i32 your_bounces;
  f32 your_seconds;

  i32 ball_color;
  v2i ball_shape;
  v2i ball_position;
  i32 ball_kick;
  i32 ball_direction;
  f32 ball_movement_time;
  f32 ball_movement_period;
  b32 ball_slow;
  b32 ball_switched;
  b32 ball_has_key;
  b32 ball_killed;

  v2f camera_offset;
  v2f camera_position;
  v2f camera_velocity;

  f32 menu_velocity;
  f32 menu_position;

  Particles particles;

  i32 buffered_transitions_left0;
  i32 buffered_transitions_left1;
  i32 buffered_transitions_right0;
  i32 buffered_transitions_right1;

  f64 audio_t;
  BouncePatch bounce_patch;
  BreakPatch break_patch;
  EndPatch end_patch;
};
