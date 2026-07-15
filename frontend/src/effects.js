export const EFFECT_COLORS = {
  distortion: '#ff8a18',
  bitcrusher: '#ffca3a',
  overdrive: '#ff5a36',
  boost: '#f43f5e',
  chorus: '#18d879',
  tremolo: '#27b7ff',
  delay: '#8a5cff',
  compressor: '#ff3f67',
  noisegate: '#22c55e',
  sustain: '#eab308',
  lowpass: '#16dfd0',
  highpass: '#06b6d4',
  bandpass: '#0ea5e9',
  notch: '#14b8a6',
  eq: '#84cc16',
  autowah: '#f97316',
};

export const EFFECT_GLYPHS = {
  distortion: '∿',
  bitcrusher: '▥',
  overdrive: '⌁',
  boost: '↑',
  chorus: '≈',
  tremolo: '⌇',
  delay: '◉',
  compressor: '↔',
  noisegate: '⊣',
  sustain: '∞',
  lowpass: '⌁',
  highpass: '⌁',
  bandpass: '◇',
  notch: '⌄',
  eq: '≡',
  autowah: '◒',
};

export const EFFECT_TYPES = [
  { id: 'drive', label: 'Drive', effects: ['distortion', 'overdrive', 'boost', 'bitcrusher'] },
  { id: 'modulation', label: 'Modulation', effects: ['chorus', 'tremolo'] },
  { id: 'time', label: 'Time', effects: ['delay'] },
  { id: 'dynamics', label: 'Dynamics', effects: ['compressor', 'noisegate', 'sustain'] },
  { id: 'filter', label: 'Filter', effects: ['lowpass', 'highpass', 'bandpass', 'notch', 'eq', 'autowah'] },
];

const TYPE_BY_EFFECT = EFFECT_TYPES.reduce((lookup, type) => {
  type.effects.forEach((effect) => { lookup[effect] = type; });
  return lookup;
}, {});

export function effectColor(name) {
  return EFFECT_COLORS[name] || '#a762e9';
}

export function effectGlyph(name) {
  return EFFECT_GLYPHS[name] || '∿';
}

export function effectType(name) {
  return TYPE_BY_EFFECT[name] || { id: 'other', label: 'Other', effects: [] };
}

export function groupEffectsByType(effects) {
  const groups = new Map(EFFECT_TYPES.map((type) => [type.id, { ...type, effects: [] }]));
  effects.forEach((name) => {
    const type = effectType(name);
    if (!groups.has(type.id)) groups.set(type.id, { ...type, effects: [] });
    groups.get(type.id).effects.push(name);
  });
  return [...groups.values()].filter((group) => group.effects.length);
}
