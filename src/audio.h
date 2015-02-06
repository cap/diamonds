#pragma once


struct BreakPatch {
  lp1f envelope;
  lp1f frequency;
  f32 phasor;
};


struct BouncePatch {
  lp1f envelope;
  lp1f frequency;
  f32 phasor;
};


struct EndPatch {
  lp1f envelope;
  lp1f frequency;
  SynthSvf svf;
  f32 phasor;
};
