#pragma once

#include "path.h"


bool contains_t(f64 *ts, i32 size, f64 t) {
  return ts[size - 1] > ts[0] && t >= ts[0] && t <= ts[size - 1];
}


i32 find_t(f64 *ts, i32 size, f64 t) {
  i32 i = 0;
  for(; i < size - 1; ++i) {
    if(t < ts[i]) return i - 1;
  }
  if(t <= ts[i]) return i - 1;
  return -1;
}


f32 lerp(f64 *ts, f32 *ps, i32 size, f64 t) {
  int i0 = find_t(ts, size, t);
  assert(i0 >= 0 && i0 < size - 1);
  int i1 = i0 + 1;
  f64 a = 1 - (t - ts[i0]) / (ts[i1] - ts[i0]);
  return a * ps[i0] + (1 - a) * ps[i1];
}


f32 lerp_default(f64 *ts, f32 *ps, i32 size, f64 t, f32 def) {
  return contains_t(ts, size, t)? lerp(ts, ps, size, t) : def;
}


bool contains_t(lp1f *path, f64 t) {
  return contains_t(path->t, path->size, t);
}


i32 find_t(lp1f *path, f64 t) {
  return find_t(path->t, path->size, t);
}


f32 lerp(lp1f *path, f64 t) {
  return lerp(path->t, path->p, path->size, t);
}


f32 lerp_default(lp1f *path, f64 t, f32 def) {
  return lerp_default(path->t, path->p, path->size, t, def);
}
