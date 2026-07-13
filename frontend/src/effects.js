export const EFFECT_COLORS = {
  distortion: '#ff8a18',
  bitcrusher: '#ffca3a',
  overdrive: '#ff5a36',
  chorus: '#18d879',
  tremolo: '#27b7ff',
  delay: '#8a5cff',
  compressor: '#ff3f67',
  lowpass: '#16dfd0',
};

export const EFFECT_GLYPHS = {
  distortion: '∿',
  bitcrusher: '▥',
  overdrive: '⌁',
  chorus: '≈',
  tremolo: '⌇',
  delay: '◉',
  compressor: '↔',
  lowpass: '⌁',
};

export function effectColor(name) {
  return EFFECT_COLORS[name] || '#a762e9';
}

export function effectGlyph(name) {
  return EFFECT_GLYPHS[name] || '∿';
}
