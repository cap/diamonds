#pragma once


void draw_mask(
  Image *dst, v2i position, const char *src, v2i src_shape, c4f color) {
  for(i32 sy = 0; sy < src_shape[1]; ++sy) {
    i32 dy = sy + position[1];
    if(dy >= 0 && dy < dst->shape[1]) {
      for(i32 sx = 0; sx < src_shape[0]; ++sx) {
        i32 dx = sx + position[0];
        if(dx >= 0 && dx < dst->shape[0]) {
          i32 si = (src_shape[1] - sy - 1) * src_shape[0] + sx;
          if(src[si] == '#') {
            i32 di = dy * dst->shape[0] + dx;
            dst->data[di] = color;
          }
        }
      }
    }
  }
}


void draw_copy_masked(Image *dst, v2i position, Image *src) {
  for(i32 sy = 0; sy < src->shape[1]; ++sy) {
    i32 dy = sy + position[1];
    if(dy >= 0 && dy < dst->shape[1]) {
      for(i32 sx = 0; sx < src->shape[0]; ++sx) {
        i32 dx = sx + position[0];
        if(dx >= 0 && dx < dst->shape[0]) {
          i32 si = sy * src->shape[0] + sx;
          if(src->data[si][3] != 0) {
            i32 di = dy * dst->shape[0] + dx;
            dst->data[di] = src->data[si];
          }
        }
      }
    }
  }
}


void draw_multiplied_masked(Image *dst, v2i position, Image *src, c4f color) {
  for(i32 sy = 0; sy < src->shape[1]; ++sy) {
    i32 dy = sy + position[1];
    if(dy >= 0 && dy < dst->shape[1]) {
      for(i32 sx = 0; sx < src->shape[0]; ++sx) {
        i32 dx = sx + position[0];
        if(dx >= 0 && dx < dst->shape[0]) {
          i32 si = sy * src->shape[0] + sx;
          c4f sc = src->data[si].cwiseProduct(color);
          if(sc[3] != 0) {
            i32 di = dy * dst->shape[0] + dx;
            dst->data[di] = sc;
          }
        }
      }
    }
  }
}


void draw_copy(Image *dst, v2i position, Image *src) {
  for(i32 sy = 0; sy < src->shape[1]; ++sy) {
    i32 dy = sy + position[1];
    if(dy >= 0 && dy < dst->shape[1]) {
      for(i32 sx = 0; sx < src->shape[0]; ++sx) {
        i32 dx = sx + position[0];
        if(dx >= 0 && dx < dst->shape[0]) {
          i32 si = sy * src->shape[0] + sx;
          i32 di = dy * dst->shape[0] + dx;
          dst->data[di] = src->data[si];
        }
      }
    }
  }
}


void draw_copy(ImageC4u8 *dst, v2i position, Image *src) {
  for(i32 sy = 0; sy < src->shape[1]; ++sy) {
    i32 dy = sy + position[1];
    if(dy >= 0 && dy < dst->shape[1]) {
      for(i32 sx = 0; sx < src->shape[0]; ++sx) {
        i32 dx = sx + position[0];
        if(dx >= 0 && dx < dst->shape[0]) {
          i32 si = sy * src->shape[0] + sx;
          i32 di = dy * dst->shape[0] + dx;
          dst->data[di] = (src->data[si] * 255).cast<u8>();
        }
      }
    }
  }
}


void draw_multiply(Image *dst, v2i position, Image *src) {
  for(i32 sy = 0; sy < src->shape[1]; ++sy) {
    i32 dy = sy + position[1];
    if(dy >= 0 && dy < dst->shape[1]) {
      for(i32 sx = 0; sx < src->shape[0]; ++sx) {
        i32 dx = sx + position[0];
        if(dx >= 0 && dx < dst->shape[0]) {
          i32 si = sy * src->shape[0] + sx;
          i32 di = dy * dst->shape[0] + dx;
          dst->data[di] = dst->data[di].cwiseProduct(src->data[si]);
        }
      }
    }
  }
}


void draw_set(Image *dst, v2i position, v2i shape, c4f color) {
  for(i32 sy = 0; sy < shape[1]; ++sy) {
    i32 dy = sy + position[1];
    if(dy >= 0 && dy < dst->shape[1]) {
      for(i32 sx = 0; sx < shape[0]; ++sx) {
        i32 dx = sx + position[0];
        if(dx >= 0 && dx < dst->shape[0]) {
          i32 di = dy * dst->shape[0] + dx;
          dst->data[di] = color;
        }
      }
    }
  }
}


i32 get_text_width(Image *glyphs, const char *text) {
  i32 width = 0;
  while(*text) {
    u8 c = *text - 32;
    if(c >= 0 && c < 64) {
      width += glyphs[c].shape[0] + 1;
    }
    ++text;
  }
  return width;
}


v2i get_text_shape(Image *glyphs, const char *text) {
  return {get_text_width(glyphs, text), 5};
}


void draw_text(
  Image *dst, v2i position, Image *glyphs, c4f color, const char* text) {
  while(*text) {
    u8 c = *text - 32;
    if(c >= 0 && c < 64) {
      draw_multiplied_masked(dst, position, &glyphs[c], color);
      position[0] += glyphs[c].shape[0] + 1;
    }
    ++text;
  }
}


void draw_centered_text(
  Image *dst, v2i position, Image *glyphs, c4f color, const char* text) {
  position[0] -= get_text_width(glyphs, text) / 2;
  draw_text(dst, position, glyphs, color, text);
}


void draw_shadowed_centered_text(
  Image *dst, v2i position, Image *glyphs, c4f shadow, c4f fg,
  const char* text) {
  draw_centered_text(dst, position + v2i{-1, 0}, glyphs, shadow, text);
  draw_centered_text(dst, position + v2i{+1, 0}, glyphs, shadow, text);
  draw_centered_text(dst, position + v2i{0, -1}, glyphs, shadow, text);
  draw_centered_text(dst, position + v2i{0, +1}, glyphs, shadow, text);
  draw_centered_text(dst, position, glyphs, fg, text);
}


void draw_shadowed_text(
  Image *dst, v2i position, Image *glyphs, c4f shadow, c4f fg,
  const char* text) {
  draw_text(dst, position + v2i{-1, 0}, glyphs, shadow, text);
  draw_text(dst, position + v2i{+1, 0}, glyphs, shadow, text);
  draw_text(dst, position + v2i{0, -1}, glyphs, shadow, text);
  draw_text(dst, position + v2i{0, +1}, glyphs, shadow, text);
  draw_text(dst, position, glyphs, fg, text);
}
