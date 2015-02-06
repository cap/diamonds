void trigger_bounce_patch(BouncePatch *patch, f64 t) {
  i32 i;

  lp1f *e = &patch->envelope;
  i = 0;
  e->p[i++] = lerp_default(e, t, 0);
  e->p[i++] = .9;
  e->p[i++] = .7;
  e->p[i++] = .7;
  e->p[i++] = .0;

  i = 0;
  e->t[i++] = t;
  e->t[i++] = e->t[i - 1] + .01;
  e->t[i++] = e->t[i - 1] + .01;
  e->t[i++] = e->t[i - 1] + .04;
  e->t[i++] = e->t[i - 1] + .02;

  lp1f *f = &patch->frequency;
  i = 0;
  f->p[i++] = lerp_default(f, t, 200);
  f->p[i++] = 200;
  f->p[i++] = 50;
  f->p[i++] = 125;

  i = 0;
  f->t[i++] = e->t[0];
  f->t[i++] = e->t[0] + .005;
  f->t[i++] = (e->t[0] + e->t[4]) / 2;
  f->t[i++] = e->t[4];
}


void trigger_break_patch(BreakPatch *patch, f64 t) {
  i32 i;

  lp1f *e = &patch->envelope;
  i = 0;
  e->p[i++] = lerp_default(e, t, 0);
  e->p[i++] = .8;
  e->p[i++] = .6;
  e->p[i++] = .6;
  e->p[i++] = .0;

  i = 0;
  e->t[i++] = t;
  e->t[i++] = e->t[i - 1] + .01;
  e->t[i++] = e->t[i - 1] + .01;
  e->t[i++] = e->t[i - 1] + .04;
  e->t[i++] = e->t[i - 1] + .02;

  lp1f *f = &patch->frequency;
  i = 0;
  f->p[i++] = 300;
  f->p[i++] = 150;
  f->p[i++] = 125;

  i = 0;
  f->t[i++] = e->t[0];
  f->t[i++] = (e->t[0] + e->t[4]) / 2;
  f->t[i++] = e->t[4];
}


void trigger_end_patch(EndPatch *patch, f64 t, bool killed) {
  i32 i;

  lp1f *e = &patch->envelope;
  i = 0;
  e->t[i++] = t;
  e->t[i++] = e->t[i - 1] + .2;
  e->t[i++] = e->t[i - 1] + .01;
  e->t[i++] = e->t[i - 1] + 2.0;
  e->t[i++] = e->t[i - 1] + .02;

  i = 0;
  e->p[i++] = .0;
  e->p[i++] = .5;
  e->p[i++] = .4;
  e->p[i++] = .01;
  e->p[i++] = .0;

  lp1f *f = &patch->frequency;
  i = 0;
  f->t[i++] = e->t[0];
  f->t[i++] = (e->t[0] + e->t[4]) / 2;
  f->t[i++] = e->t[4];

  i = 0;
  if(killed) {
    f->p[i++] = 1000;
    f->p[i++] = 500;
    f->p[i++] = 100;
  } else {
    f->p[i++] = 100;
    f->p[i++] = 5000;
    f->p[i++] = 10000;
  }
}


extern "C"
GAME_GET_AUDIO(game_get_audio) {
  if(memory->arena.size == 0) return;
  GameState *state = (GameState *)memory->arena.data;

  f64 t = state->audio_t;
  f32 dt = 1.f / audio->rate;
  i32 size = audio->size;

  f32 *buf = PUSH_ARRAY(&state->tmp, size, f32);
  f32 *env = PUSH_ARRAY(&state->tmp, size, f32);

  cap_zero_array(audio->samples, audio->size);
  {
    BouncePatch *patch = &state->bounce_patch;
    synth_path(buf, &patch->frequency, size, t, dt);
    patch->phasor = synth_phasor(buf, buf, size, patch->phasor, dt);
    synth_sine(buf, buf, size);
    synth_path(env, &patch->envelope, size, t, dt);
    synth_mul(buf, buf, env, size);
  }
  synth_add(audio->samples, audio->samples, buf, size);
  {
    BreakPatch *patch = &state->break_patch;
    synth_path(buf, &patch->frequency, size, t, dt);
    patch->phasor = synth_phasor(buf, buf, size, patch->phasor, dt);
    synth_sine(buf, buf, size);
    synth_path(env, &patch->envelope, size, t, dt);
    synth_mul(buf, buf, env, size);
  }
  synth_add(audio->samples, audio->samples, buf, size);
  {
    EndPatch *patch = &state->end_patch;
    synth_path(env, &patch->frequency, size, t, dt);
    synth_noise(buf, size);
    patch->svf.set_mode(SynthSvf::BAND);
    patch->svf.set_res(.8);
    patch->svf.set_drive(0);
    synth_svf_cutoff(buf, buf, env, size, &patch->svf, dt);
    synth_path(env, &patch->envelope, size, t, dt);
    synth_mul(buf, buf, env, size);
  }
  synth_add(audio->samples, audio->samples, buf, size);

  state->audio_t += (f64)audio->size / (f64)audio->rate;
}
