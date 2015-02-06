#pragma once

#include <math.h>
#include <stdlib.h>


// State Variable Filter (Double Sampled, Stable)
// From Simper, de Soras, and Diedrichsen
// http://www.musicdsp.org/archive.php?classid=3#92
struct SynthSvf {
  enum Mode {
    NOTCH,
    LOW,
    HIGH,
    BAND,
    PEAK,
  };

  f32 cutoff;
  f32 res;
  f32 drive;
  f32 freq;
  f32 damp;
  f32 s[5];
  Mode mode;

  void _update_freq(f32 dt) {
    freq = 2. * sin(M_PI * fmin(.25, cutoff * dt / 2.));
    _update_damp();
  }

  void _update_damp() {
    damp = fmin(2. * (1. - powf(res, .25f)), fmin(2., 2. / freq - freq * .5));
  }

  void init(f32 dt) {
    mode = LOW;
    cutoff = 600;
    res = 0;
    drive = 0;
    _update_freq(dt);
    s[0] = s[1] = s[2] = s[3] = s[4] = 0;
  }

  void set_cutoff(f32 value, f32 dt) {
    cutoff = value;
    _update_freq(dt);
  }

  void set_res(f32 value) {
    res = value;
    _update_damp();
  }

  void set_drive(f32 value) {
    drive = value;
  }

  void set_mode(Mode value) {
    mode = value;
  }

  f32 get_value(f32 in) {
    s[NOTCH] = in - damp * s[BAND];
    s[LOW] = s[LOW] + freq * s[BAND];
    s[HIGH] = s[NOTCH] - s[LOW];
    s[BAND] = freq * s[HIGH] + s[BAND] - drive * s[BAND] * s[BAND] * s[BAND];
    s[PEAK] = s[LOW] - s[HIGH];
    f32 out = 0.5 * s[mode];
    s[NOTCH] = in - damp * s[BAND];
    s[LOW] = s[LOW] + freq * s[BAND];
    s[HIGH] = s[NOTCH] - s[LOW];
    s[BAND] = freq * s[HIGH] + s[BAND] - drive * s[BAND] * s[BAND] * s[BAND];
    s[PEAK] = s[LOW] - s[HIGH];
    out = out + 0.5 * s[mode];
    return out;
  }
};


void synth_set(f32 *out, int size, f32 value) {
  for(int i = 0; i < size; ++i) {
    *out++ = value;
  }
}


void synth_path(f32 *out, lp1f *path, int size, f64 t, f32 dt) {
  for(int i = 0; i < size; ++i) {
    *out++ = lerp_default(path, t, 0);
    t += dt;
  }
}


f32 synth_phasor(f32 *out, f32 *freq, int size, f32 phase, f32 dt) {
  for(int i = 0; i < size; ++i) {
    f32 tmp = *freq++;
    *out++ = phase;
    phase = fmodf(phase + tmp * dt, 1.f);
  }
  return phase;
}


void synth_svf_cutoff(
  f32 *out, f32 *in, f32 *cutoff, int size, SynthSvf *svf, f32 dt) {
  for(int i = 0; i < size; ++i) {
    svf->set_cutoff(*cutoff++, dt);
    *out++ = svf->get_value(*in++);
  }
}


void synth_noise(f32 *out, int size) {
  for(int i = 0; i < size; ++i) {
    *out++ = (f32)(rand() / (RAND_MAX + 1.f));
  }
}


void synth_sine(f32 *out, f32 *phase, int size) {
  for(int i = 0; i < size; ++i) {
    *out++ = sinf(*phase++ * 2 * M_PI);
  }
}


void synth_clip(f32 *out, f32 *in, int size) {
  for(int i = 0; i < size; ++i) {
    *out++ = fmax(fmin(*in++, 1.f), -1.f);
  }
}


int synth_validate(f32 *out, int size) {
  for(int i = 0; i < size; ++i) {
    if(!isnormal(*out++)) {
      return i;
    }
  }
  return size;
}


void synth_mul(f32 *out, f32 *in0, f32 *in1, int size) {
  for(int i = 0; i < size; ++i) {
    *out++ = *in0++ * *in1++;
  }
}


void synth_add(f32 *out, f32 *in0, f32 *in1, int size) {
  for(int i = 0; i < size; ++i) {
    *out++ = *in0++ + *in1++;
  }
}


void synth_cross_fade(f32 *out, f32 *from, f32 *to, int size) {
  for(int i = 0; i < size; ++i) {
    f32 a = (f32)i / (f32)(size - 1);
    *out++ = (1 - a) * *from++ + a * *to++;
  }
}
