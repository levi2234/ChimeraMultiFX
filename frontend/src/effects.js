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

export const EFFECT_TYPES = [
  { id: 'drive', label: 'Drive', effects: ['distortion', 'overdrive', 'bitcrusher'] },
  { id: 'modulation', label: 'Modulation', effects: ['chorus', 'tremolo'] },
  { id: 'time', label: 'Time', effects: ['delay'] },
  { id: 'dynamics', label: 'Dynamics', effects: ['compressor'] },
  { id: 'filter', label: 'Filter', effects: ['lowpass'] },
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
