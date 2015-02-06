#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "base.h"
#include "shared.h"

#include "data/glyphs.cc"
#include "data/maps.cc"
#include "data/masks.cc"

#include "image.cc"
#include "path.cc"
#include "synth.cc"

#include "game.h"

#include "audio.cc"


void parse_map(const char *raw, Map *map) {
  for(i32 y = 0; y < map->shape[1]; ++y) {
    for(i32 x = 0; x < map->shape[0]; ++x) {
      i32 i = (map->shape[1] - y - 1) * map->shape[0] + x;
      Tile *tile = &map->tiles[i];
      if((x == 0 || x == map->shape[0] - 1) ||
         (y == 0 || y == map->shape[1] - 1)) {
        tile->type = Tile::BORDER;
      } else {
        i32 t = *raw++;
        switch(t) {
        case '.': tile->type = Tile::EMPTY; break;
        case 'o': {
          tile->type = Tile::EMPTY;
          map->start = {x, map->shape[1] - y - 1};
        } break;
        case 'g': tile->type = Tile::INERT; break;
        case 'f': tile->type = Tile::DIAMOND; break;
        case 'S': tile->type = Tile::SWITCH; break;
        case 'L': tile->type = Tile::LOCK; break;
        case 'K': tile->type = Tile::KEY; break;
        case 'X': tile->type = Tile::KILL; break;
        default: {
          if(t >= 'A' && t <= 'E') {
            tile->type = Tile::PAINT;
            tile->color = t - 'A';
          } else if(t >= 'a' && t <= 'e') {
            tile->type = Tile::COLOR;
            tile->color = t - 'a';
          }
        }
        }
      }
    }
  }
  assert(*raw == 0);
}


void convert_map_to_mask(char *mask, const char *map, v2i shape) {
  shape -= v2i{2, 2};
  for(i32 y = 0; y < shape[1]; ++y) {
    for(i32 x = 0; x < shape[0]; ++x) {
      char mask_value;
      char tile = map[y * shape[0] + x];
      if(tile == '.' || tile == 'o') {
        mask_value = '.';
      } else {
        mask_value = '#';
      }
      mask[y * shape[0] * 2 + x * 2] = mask_value;
      mask[y * shape[0] * 2 + x * 2 + 1] = mask_value;
    }
  }
}


Tile *get_map_tile(Map *map, v2i ts, v2i p) {
  v2i tp{p[0] / ts[0], p[1] / ts[1]};
  v2i to{p[0] % ts[0], p[1] % ts[1]};
  b32 is_corner =
    (to[0] == 0 || to[0] == ts[0] - 1) &&
    (to[1] == 0 || to[1] == ts[1] - 1);
  if(is_corner) return 0;
  Tile *tile = &map->tiles[tp[1] * map->shape[0] + tp[0]];
  if(tile->type == Tile::EMPTY) return 0;
  return tile;
}


v2f get_camera_target(GameState *state) {
  v2f target = state->camera_offset;
  target[0] += state->map_index * 24;
  return target;
}


void start_game(GameState *state) {
  state->state = GameState::GAME;
  parse_map(global_maps[state->map_index], &state->map);
  state->ball_position = state->map.start.cwiseProduct(
    state->tile_shape);
  state->ball_position[0] += (state->tile_shape[0] - state->ball_shape[0]) / 2;
  state->ball_color = 0;
  state->ball_direction = 1;
  state->ball_kick = 0;
  state->ball_switched = false;
  state->ball_has_key = false;
  state->ball_killed = false;
  state->your_bounces = 0;
  state->your_seconds = 0;

  state->camera_position = get_camera_target(state);
  state->camera_velocity = {0, 0};
}


b32 key_down_ever(KeyState *key) {
  return key->down || key->transitions > 1;
}


b32 key_down_started(KeyState *key) {
  return (key->down && key->transitions > 0) ||
    (!key->down && key->transitions > 1);
}


#ifdef CAP_DEBUG
#define SAVE_NAME "save_debug"
#else
#define SAVE_NAME "save"
#endif


void write_save(
  PlatformContext *platform, GameMemory *memory, GameState *state) {
  Save *save = PUSH_STRUCT(&state->tmp, Save);
  save->map_index = state->map_index;
  memcpy(&save->records, &state->records, sizeof(state->records));
  memory->platform_write_file(platform, SAVE_NAME, (char *)save, sizeof(Save));
}


void read_save(
  PlatformContext *platform, GameMemory *memory, GameState *state) {
  Save *save = PUSH_STRUCT(&state->tmp, Save);
  i32 expected = sizeof(Save);
  i32 read = memory->platform_read_file(
    platform, SAVE_NAME, (char *)save, expected);
  if(read != expected) {
    cap_log_debug("couldn't read %s\n", SAVE_NAME);
    memset(save, 0, expected);
  }
  state->map_index = save->map_index;
  state->menu_position = save->map_index;
  memcpy(&state->records, &save->records, sizeof(state->records));
}


void push_lp1f(Arena *arena, lp1f *path, i32 size) {
  path->size = size;
  path->t = PUSH_ARRAY(arena, size, f64);
  path->p = PUSH_ARRAY(arena, size, f32);
}


void push_image(Arena *arena, Image *image, v2i shape) {
  image->shape = shape;
  image->data = PUSH_ARRAY(arena, shape.prod(), c4f);
}


extern "C"
GAME_UPDATE_AND_RENDER(game_update_and_render) {
  GameState *state;
  if(memory->arena.size == 0) {
    Arena *arena = &memory->arena;
    state = PUSH_STRUCT(arena, GameState);

    state->tmp.max_size = cap_megabytes(1);
    state->tmp.data = PUSH_ARRAY(arena, state->tmp.max_size, u8);

    state->colors.shadow = {.95f, .95f, .85f, 1.f};
    state->colors.bg = {.80f, .82f, .40f, 1.f};
    state->colors.fg = {.15f, .15f, .35f, 1.f};
    // alternate colors taken from hp48 picture
    // state->colors.bg = {.77f, .85f, .35f, 1.f};
    // state->colors.fg = {.12f, .14f, .47f, 1.f};

    state->tile_shape = {12, 6};
    state->ball_shape = {5, 5};

    for(i32 i = 0; i < cap_array_size(global_glyphs); ++i) {
      Image *glyph = &state->glyphs[i];
      if(global_glyphs[i]) {
          push_image(arena, glyph, {(i32)strlen(global_glyphs[i]) / 5, 5});
        draw_mask(glyph, {0, 0}, global_glyphs[i], glyph->shape, {1, 1, 1, 1});
      }
    }

    for(i32 i = 0; i < cap_array_size(global_tiles); ++i) {
      Image *tile = &state->tiles[i];
      push_image(arena, tile, state->tile_shape);
      draw_mask(
        tile, {0, 0}, global_tiles[Tile::SHADOW], tile->shape,
        state->colors.shadow);
      draw_mask(tile, {0, 0}, global_tiles[i], tile->shape, state->colors.fg);
    }

    for(i32 i = 0; i < cap_array_size(global_balls); ++i) {
      Image *ball = &state->balls[i];
      push_image(arena, ball, state->ball_shape);
      draw_mask(
        ball, {0, 0}, global_balls[6], ball->shape, state->colors.shadow);
      draw_mask(ball, {0, 0}, global_balls[i], ball->shape, state->colors.fg);
    }

    state->map.shape = {10, 12};
    state->map.tiles = PUSH_ARRAY(arena, state->map.shape.prod(), Tile);

    v2i ms = state->map.shape - v2i{2, 2};
    state->minimap_shape = {ms[0] * 2, ms[1]};

    for(i32 i = 0; i < cap_array_size(global_maps); ++i) {
      Image *minimap = &state->minimaps[i];
      push_image(arena, minimap, state->minimap_shape);
      char buf[1024];
      convert_map_to_mask(buf, global_maps[i], state->map.shape);
      draw_mask(minimap, {0, 0}, buf, minimap->shape, state->colors.fg);
    }

    {
      Particles *particles = &state->particles;
      particles->size = screen->shape.prod() * 2;
      particles->t = PUSH_ARRAY(arena, particles->size, f32);
      particles->p = PUSH_ARRAY(arena, particles->size, v2f);
      particles->v = PUSH_ARRAY(arena, particles->size, v2f);
      particles->a = PUSH_ARRAY(arena, particles->size, v2f);
      particles->c = PUSH_ARRAY(arena, particles->size, c4f);
    }

    push_lp1f(arena, &state->bounce_patch.envelope, 5);
    push_lp1f(arena, &state->bounce_patch.frequency, 4);
    push_lp1f(arena, &state->break_patch.envelope, 5);
    push_lp1f(arena, &state->break_patch.frequency, 3);
    push_lp1f(arena, &state->end_patch.envelope, 5);
    push_lp1f(arena, &state->end_patch.frequency, 3);

    state->ball_movement_time = state->ball_movement_period;

    read_save(platform, memory, state);

    state->camera_offset = {-screen->shape[0] * .5f, 0.f};
    state->camera_position = get_camera_target(state);
  } else {
    state = (GameState *)memory->arena.data;
    state->tmp.size = 0;
  }

  if(state->state == GameState::MENU) {
    b32 left = false;
    left |= key_down_ever(&input->keys.left0);
    left |= key_down_ever(&input->keys.left1);

    b32 right = false;
    right |= key_down_ever(&input->keys.right0);
    right |= key_down_ever(&input->keys.right1);

    f32 menu_acceleration = 0;
    if(left) --menu_acceleration;
    if(right) ++menu_acceleration;
    if(menu_acceleration == 0) {
      state->menu_velocity = 0;
      state->menu_position = (i32)roundf(state->menu_position);
    } else {
      if(state->menu_velocity == 0) {
        state->menu_position += cap_sign(menu_acceleration);
      }
      menu_acceleration *= 20;
      state->menu_velocity += input->dt * menu_acceleration;
      state->menu_position += input->dt * state->menu_velocity;
      state->menu_velocity = cap_min(state->menu_velocity, +20.f);
      state->menu_velocity = cap_max(state->menu_velocity, -20.f);
    }
    if(state->menu_position < 0) {
      state->menu_position = 0;
      state->menu_velocity = 0;
    }
    if(state->menu_position > cap_array_size(global_maps) - 1) {
      state->menu_position = cap_array_size(global_maps) - 1;
      state->menu_velocity = 0;
    }

    v2f camera_target = get_camera_target(state);
    if((state->camera_position - camera_target).norm() < .5) {
      state->camera_position = camera_target;
      state->camera_velocity = {0, 0};
    } else {
      v2f camera_acceleration = camera_target - state->camera_position;
      state->camera_velocity = camera_acceleration * 10;
      state->camera_position += input->dt * state->camera_velocity;
    }

    state->map_index = (i32)roundf(state->menu_position);

    draw_set(screen, {0, 0}, screen->shape, state->colors.bg);

    v2i text_position = {
      (i32)(screen->shape[0] * .5), (i32)(screen->shape[1] - 15)};
    draw_shadowed_centered_text(
      screen, text_position, state->glyphs, state->colors.shadow,
      state->colors.fg, "DIAMONDS");

    for(i32 i = 0; i < cap_array_size(global_maps); ++i) {
      v2i position = {i * 24 - 8, 35};
      position -= state->camera_position.cast<i32>();
      if(i == state->map_index) {
        draw_set(
          screen, position - v2i{2, 2}, state->minimap_shape + v2i{4, 4},
          state->colors.shadow);
        draw_set(
          screen, position - v2i{1, 1}, state->minimap_shape + v2i{2, 2},
          state->colors.fg);
        draw_set(
          screen, position - v2i{0, 0}, state->minimap_shape + v2i{0, 0},
          state->colors.shadow);
      }
      draw_copy_masked(screen, position, &state->minimaps[i]);

      position += {17, -3};
      if(state->records[i].bounces) {
        if(i == state->map_index) {
          draw_shadowed_centered_text(
            screen, position, state->glyphs, state->colors.fg,
            state->colors.shadow, "%");
        } else {
          draw_centered_text(
            screen, position, state->glyphs, state->colors.shadow, "%");
        }
      }
    }

    b32 start = false;
    start |= key_down_started(&input->keys.action0);
    start |= key_down_started(&input->keys.action1);
    if(start) start_game(state);

    b32 quit = false;
    quit |= key_down_started(&input->keys.kill0);
    quit |= key_down_started(&input->keys.kill1);
    if(quit) {
      write_save(platform, memory, state);
      memory->platform_quit(platform);
    }

    return;
  }

  if(state->state == GameState::SCOREBOARD) {
    if(!state->scoreboard_configured) {
      trigger_end_patch(&state->end_patch, state->audio_t, state->ball_killed);

      i32 max_display_bounces = 99999;
      f32 max_display_time = 99.99f;

      Record *record = &state->records[state->map_index];
      if(record->bounces == 0) {
        sprintf(state->scoreboard.best_bounces, "-");
      } else if(record->bounces > max_display_bounces) {
        sprintf(state->scoreboard.best_bounces, "%05d+", max_display_bounces);
      } else {
        sprintf(state->scoreboard.best_bounces, "%05d", record->bounces);
      }
      if(record->time == 0.f) {
        sprintf(state->scoreboard.best_time, "-");
      } else if(record->time > max_display_time) {
        sprintf(state->scoreboard.best_time, "%05.2f+", max_display_time);
      } else {
        sprintf(state->scoreboard.best_time, "%05.2f", record->time);
      }
      if(state->your_bounces > max_display_bounces) {
        sprintf(state->scoreboard.your_bounces, "%05d+", max_display_bounces);
      } else {
        sprintf(state->scoreboard.your_bounces, "%05d", state->your_bounces);
      }
      if(state->your_seconds > max_display_time) {
        sprintf(state->scoreboard.your_time, "%05.2f+", max_display_time);
      } else {
        sprintf(state->scoreboard.your_time, "%05.2f", state->your_seconds);
      }
      state->scoreboard.new_bounces_record = false;
      state->scoreboard.new_time_record = false;
      if(!state->ball_killed) {
        if(record->bounces == 0 || state->your_bounces < record->bounces) {
          state->scoreboard.new_bounces_record = true;
          record->bounces = state->your_bounces;
        }
        if(record->time == 0.f || state->your_seconds < record->time) {
          state->scoreboard.new_time_record = true;
          record->time = state->your_seconds;
        }
      }
      if(state->scoreboard.new_bounces_record ||
         state->scoreboard.new_time_record) {
        write_save(platform, memory, state);
      }

      if(state->ball_killed) {
        i32 pi = 0;
        Particles *particles = &state->particles;
        for(v2i i{0, 0}; i[1] < screen->shape[1]; ++i[1]) {
          for(i[0] = 0; i[0] < screen->shape[0]; ++i[0]) {
            c4f cf = screen->data[i[1] * screen->shape[0] + i[0]];
            if((cf - state->colors.bg).squaredNorm() < 1) {
              particles->t[pi] = 10;
              particles->p[pi] = i.cast<f32>();
              v2f d = particles->p[pi] - state->ball_position.cast<f32>();
              particles->v[pi] = d.normalized() * (300. / sqrt(d.norm()));
              particles->v[pi] += {rand() % 10 - 5.f, rand() % 10 - 5.f};
              particles->a[pi] = v2f{0, -200};
              particles->c[pi] = cf;
              ++pi;
            }
          }
        }
      } else {
        i32 pi = 0;
        Particles *particles = &state->particles;
        for(v2i i{0, 0}; i[1] < screen->shape[1]; ++i[1]) {
          for(i[0] = 0; i[0] < screen->shape[0]; ++i[0]) {
            c4f cf = screen->data[i[1] * screen->shape[0] + i[0]];
            if((cf - state->colors.bg).squaredNorm() < 1) {
              particles->t[pi] = 10;
              particles->p[pi] = i.cast<f32>();
              i32 dir = state->ball_position[0] - i[0];
              dir *= .5;
              f32 ball_center = state->ball_position[0] +
                state->ball_shape[0] / 2;
              if(ball_center < screen->shape[0] / 2) dir *= -1;
              v2f d = particles->p[pi] - state->ball_position.cast<f32>();
              d[1] = -dir / 2;
              d[0] /= 2;
              particles->v[pi] = {(dir + rand() % 5) * 5.f, -dir + 75.f};
              particles->v[pi] = -d * ((rand() % 100) / 100.f * 1 + 4);
              particles->v[pi][1] += 75;
              particles->v[pi][1] *= -1;

              particles->a[pi] = {0.f, 200.f};
              particles->c[pi] = cf;
              ++pi;
            }
          }
        }
      }
      state->scoreboard_configured = true;
    }

    draw_set(screen, {0, 0}, screen->shape, state->colors.bg);

    {
      Particles *particles = &state->particles;
      for(i32 i = 0; i < particles->size; ++i) {
        if(particles->t[i] > 0) {
          particles->t[i] -= input->dt;
          particles->v[i] += input->dt * particles->a[i];
          particles->p[i] += input->dt * particles->v[i];
          draw_set(
            screen, particles->p[i].cast<i32>(), {1, 1}, particles->c[i]);
        }
      }
    }

    v2i text_position = {
      (i32)(screen->shape[0] * .5), (i32)(screen->shape[1] - 15)};


    char buf[256];

    sprintf(buf, "%s", state->ball_killed? "< FAILED <" : "> PASSED >");
    draw_shadowed_centered_text(
      screen, text_position, state->glyphs, state->colors.shadow,
      state->colors.fg, buf);
    text_position[1] -= 15;

    static i32 stops[3] = {18, 32 , 64};
    i32 stop_index = 0;
    text_position[0] = stops[stop_index++];
    text_position[0] = stops[stop_index++];
    draw_text(
      screen, text_position, state->glyphs,
      state->colors.fg, "BEST");
    text_position[0] = stops[stop_index++];
    draw_text(
      screen, text_position, state->glyphs,
      state->colors.fg, "YOURS");
    text_position[1] -= 7;
    stop_index = 0;

    text_position[0] = stops[stop_index++];
    draw_text(
      screen, text_position, state->glyphs,
      state->colors.fg, "$");
    text_position[0] = stops[stop_index++];
    draw_text(
      screen, text_position, state->glyphs,
      state->colors.fg, state->scoreboard.best_time);
    text_position[0] = stops[stop_index++];
    if(state->scoreboard.new_time_record) {
      draw_shadowed_text(
        screen, text_position, state->glyphs, state->colors.shadow,
        state->colors.fg, state->scoreboard.your_time);
    } else {
      draw_text(
        screen, text_position, state->glyphs,
        state->colors.fg, state->scoreboard.your_time);
    }
    text_position[1] -= 7;
    stop_index = 0;

    text_position[0] = stops[stop_index++];
    draw_text(
      screen, text_position, state->glyphs,
      state->colors.fg, "#");
    text_position[0] = stops[stop_index++];
    draw_text(
      screen, text_position, state->glyphs,
      state->colors.fg, state->scoreboard.best_bounces);
    text_position[0] = stops[stop_index++];
    if(state->scoreboard.new_bounces_record) {
      draw_shadowed_text(
        screen, text_position, state->glyphs, state->colors.shadow,
        state->colors.fg, state->scoreboard.your_bounces);
    } else {
      draw_text(
        screen, text_position, state->glyphs,
        state->colors.fg, state->scoreboard.your_bounces);
    }
    text_position[1] -= 7;
    stop_index = 0;

    b32 next = false;
    next |= key_down_started(&input->keys.action0);
    next |= key_down_started(&input->keys.action1);
    next |= key_down_started(&input->keys.kill0);
    next |= key_down_started(&input->keys.kill1);
    if(next) {
      state->state = GameState::MENU;
      state->scoreboard_configured = false;
    }
    return;
  }

  if(state->state == GameState::PAUSED) {
    b32 unpause = false;
    unpause |= key_down_started(&input->keys.action0);
    unpause |= key_down_started(&input->keys.action1);
    if(unpause) {
      state->state = GameState::GAME;
    }
    v2i text_position = {
      (i32)(screen->shape[0] * .5), (i32)(screen->shape[1] - 5) / 2};
    draw_shadowed_centered_text(
      screen, text_position, state->glyphs, state->colors.shadow,
      state->colors.fg, "PAUSED");

    return;
  }

  if(state->state == GameState::GAME) {
    if(key_down_started(&input->keys.speed)) {
      state->ball_slow = !state->ball_slow;
    }
    state->ball_movement_period = state->ball_slow? 1. / 30. : 1. / 60.;

    b32 pause = false;
    pause |= input->pause;
    pause |= key_down_started(&input->keys.action0);
    pause |= key_down_started(&input->keys.action1);
    if(pause) {
      state->state = GameState::PAUSED;
      return;
    }

    state->ball_killed |= key_down_started(&input->keys.kill0);
    state->ball_killed |= key_down_started(&input->keys.kill1);
    if(state->ball_killed) {
      state->state = GameState::SCOREBOARD;
      return;
    }

    state->your_seconds += input->dt;
    state->ball_movement_time -= input->dt;
    if(state->ball_movement_time >= 0) {
      state->buffered_transitions_left0 += input->keys.left0.transitions;
      state->buffered_transitions_left1 += input->keys.left1.transitions;
      state->buffered_transitions_right0 += input->keys.right0.transitions;
      state->buffered_transitions_right1 += input->keys.right1.transitions;
    } else {
      input->keys.left0.transitions += state->buffered_transitions_left0;
      input->keys.left1.transitions += state->buffered_transitions_left1;
      input->keys.right0.transitions += state->buffered_transitions_right0;
      input->keys.right1.transitions += state->buffered_transitions_right1;
      state->buffered_transitions_left0 = 0;
      state->buffered_transitions_left1 = 0;
      state->buffered_transitions_right0 = 0;
      state->buffered_transitions_right1 = 0;
    }
    while(state->ball_movement_time < 0) {
      state->ball_movement_time += state->ball_movement_period;

      v2i dp{0, state->ball_direction};
      if(state->ball_kick == 0) {
        b32 left = false;
        left |= key_down_ever(&input->keys.left0);
        left |= key_down_ever(&input->keys.left1);

        b32 right = false;
        right |= key_down_ever(&input->keys.right0);
        right |= key_down_ever(&input->keys.right1);

        if(left) --dp[0];
        if(right) ++dp[0];
        if(state->ball_switched) dp[0] *= -1;
      } else {
        dp[0] = cap_sign(state->ball_kick);
        state->ball_kick -= cap_sign(state->ball_kick);
      }

      Tile *mt[5][5];
      for(v2i i{0, 0}; i[1] < 5; ++i[1]) {
        for(i[0] = 0; i[0] < 5; ++i[0]) {
          mt[i[1]][i[0]] = get_map_tile(
            &state->map, state->tile_shape, state->ball_position + dp + i);
        }
      }

      Tile *collisions[2] = {};
      i32 old_direction = state->ball_direction;

      if(dp == v2i{0, 1}) { // up
        if(mt[3][0]) { // left
          state->ball_direction *= -1;
          if(!mt[4][4]) state->ball_kick = 2;
          collisions[0] = mt[3][0];
        } else if(mt[3][4]) { // right
          state->ball_direction *= -1;
          if(!mt[4][0]) state->ball_kick = -2;
          collisions[0] = mt[3][4];
        } else if(mt[4][1] || mt[4][2] || mt[4][3]) { // middle
          state->ball_direction *= -1;
          collisions[0] = cap_first_3(mt[4][1], mt[4][2], mt[4][3]);
        }
      } else if(dp == v2i{0, -1}) { // down
        if(mt[1][0]) { // left
          state->ball_direction *= -1;
          if(!mt[0][4]) state->ball_kick = 2;
          collisions[0] = mt[1][0];
        } else if(mt[1][4]) { // right
          state->ball_direction *= -1;
          if(!mt[4][0]) state->ball_kick = -2;
          collisions[0] = mt[1][4];
        } else if(mt[0][1] || mt[0][2] || mt[0][3]) { // middle
          state->ball_direction *= -1;
          collisions[0] = cap_first_3(mt[0][1], mt[0][2], mt[0][3]);
        }
      } else if(dp == v2i{1, 1}) { // up right
        if(mt[4][1] || mt[4][2] || mt[4][3]) {
          state->ball_direction *= -1;
          collisions[0] = cap_first_3(mt[4][2], mt[4][1], mt[4][3]);
        } else if(mt[3][4] && !(mt[2][4] || mt[1][4] || mt[0][4])) {
          collisions[0] = mt[3][4];
        }
        if(mt[1][4] || mt[2][4] || mt[3][4]) {
          state->ball_kick = -2;
          collisions[1] = cap_first_3(mt[2][4], mt[1][4], mt[3][4]);
        }
      } else if(dp == v2i{-1, 1}) { // up left
        if(mt[4][1] || mt[4][2] || mt[4][3]) {
          state->ball_direction *= -1;
          collisions[0] = cap_first_3(mt[4][2], mt[4][1], mt[4][3]);
        } else if(mt[3][0] && !(mt[2][0] || mt[1][0] || mt[0][0])) {
          collisions[0] = mt[3][0];
        }
        if(mt[1][0] || mt[2][0] || mt[3][0]) {
          state->ball_kick = 2;
          collisions[1] = cap_first_3(mt[2][0], mt[1][0], mt[3][0]);
        }
      } else if(dp == v2i{1, -1}) { // down right
        if(mt[0][1] || mt[0][2] || mt[0][3]) {
          state->ball_direction *= -1;
          collisions[0] = cap_first_3(mt[0][2], mt[0][1], mt[0][3]);
        } else if(mt[1][4] && !(mt[2][4] || mt[3][4] || mt[4][4])) {
          collisions[0] = mt[1][4];
        }
        if(mt[1][4] || mt[2][4] || mt[3][4]) {
          state->ball_kick = -2;
          collisions[1] = cap_first_3(mt[2][4], mt[1][4], mt[3][4]);
        }
      } else if(dp == v2i{-1, -1}) { // down left
        if(mt[0][1] || mt[0][2] || mt[0][3]) {
          state->ball_direction *= -1;
          collisions[0] = cap_first_3(mt[0][2], mt[0][1], mt[0][3]);
        } else if(mt[1][0] && !(mt[2][0] || mt[3][0] || mt[4][0])) {
          collisions[0] = mt[1][0];
        }
        if(mt[1][0] || mt[2][0] || mt[3][0]) {
          state->ball_kick = 2;
          collisions[1] = cap_first_3(mt[2][0], mt[1][0], mt[3][0]);
        }
      }

      if(!collisions[0] && !collisions[1]) {
        state->ball_position += dp;
      }

      if(old_direction != state->ball_direction) ++state->your_bounces;

      b32 broken = false;
      for(i32 i = 0; i < 2; ++i) {
        if(collisions[i]) {
          i32 mi = collisions[i] - state->map.tiles;

          switch(collisions[i]->type) {
          case Tile::DIAMOND: {
            if(state->ball_color == 5) {
              state->map.tiles[mi].type = Tile::EMPTY;
              broken = true;
            }
          } break;
          case Tile::KEY: {
            if(!state->ball_has_key) {
              state->ball_has_key = true;
              state->map.tiles[mi].type = Tile::EMPTY;
              broken = true;
            }
          } break;
          case Tile::LOCK: {
            if(state->ball_has_key) {
              state->ball_has_key = false;
              state->map.tiles[mi].type = Tile::EMPTY;
              broken = true;
            }
          } break;
          case Tile::KILL: {
            state->ball_killed = true;
          } break;
          case Tile::SWITCH: {
            state->ball_switched = !state->ball_switched;
            state->map.tiles[mi].type = Tile::EMPTY;
            broken = true;
          } break;
          case Tile::COLOR: {
            if(collisions[i]->color == state->ball_color) {
              state->map.tiles[mi].type = Tile::EMPTY;
              broken = true;
            }
            b32 color_left = false;
            for(i32 i = 0; i < state->map.shape.prod(); ++i) {
              if(state->map.tiles[i].type == Tile::COLOR) {
                color_left = true;
              }
            }
            if(!color_left) state->ball_color = 5;
          } break;
          case Tile::PAINT: {
            if(state->ball_color != 5) {
              state->ball_color = collisions[i]->color;
            }
          } break;
          default: break;
          }
        }
      }

      if(broken) {
        trigger_break_patch(&state->break_patch, state->audio_t);
      } else if(collisions[0] || collisions[1]) {
        trigger_bounce_patch(&state->bounce_patch, state->audio_t);
      }

      b32 finished = true;
      for(i32 i = 0; i < state->map.shape.prod(); ++i) {
        if(state->map.tiles[i].type == Tile::DIAMOND) {
          finished = false;
        }
      }
      if(finished) {
        state->state = GameState::SCOREBOARD;
      }
    }

    // render
    draw_set(screen, {0, 0}, screen->shape, state->colors.bg);

    for(i32 y = 0; y < state->map.shape[1]; ++y) {
      for(i32 x = 0; x < state->map.shape[0]; ++x) {
        Tile *t = &state->map.tiles[y * state->map.shape[0] + x];
        Image *mask = state->tiles + t->type;
        if(t->type == Tile::COLOR || t->type == Tile::PAINT) {
          mask += 2 * t->color;
        }

        v2i position = {x, y};
        position = position.cwiseProduct(mask->shape);
        position -= (mask->shape.cast<f32>().cwiseProduct({.5, 0}))
          .cast<i32>();

        if(t->type != Tile::EMPTY) {
          draw_copy_masked(screen, position, mask);
        }
      }
    }

    {
      v2i position = state->ball_position;
      position -= (state->tile_shape.cast<f32>().cwiseProduct({.5, 0}))
        .cast<i32>();
      draw_copy_masked(screen, position, &state->balls[state->ball_color]);
    }
  }

  ++state->ticks;
}
