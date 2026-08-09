export const EFFECT_COLORS = {
  distortion: '#ff8a18',
  bitcrusher: '#ffca3a',
  overdrive: '#ff5a36',
  boost: '#f43f5e',
  TubeScreamer: '#3fbf5b',
  KlonCentaur: '#d9a441',
  ProcoRAT: '#e43d30',
  BigMuffPi: '#7c3aed',
  FulltoneOCD: '#f97316',
  BossBD2: '#3388ff',
  chorus: '#18d879',
  tremolo: '#27b7ff',
  boss_ce2: '#22c9a8',
  mxr_phase90: '#f59e0b',
  boss_bf2: '#38bdf8',
  TCSubNUp: '#00c9a7',
  delay: '#8a5cff',
  DeluxeMemoryMan: '#7c5cff',
  StrymonBluesky: '#2f80ed',
  compressor: '#ff3f67',
  noisegate: '#22c55e',
  sustain: '#eab308',
  lowpass: '#16dfd0',
  highpass: '#06b6d4',
  bandpass: '#0ea5e9',
  notch: '#14b8a6',
  eq: '#84cc16',
  autowah: '#f97316',
  CryBabyMutron: '#d6b21f',
};

export const EFFECT_GLYPHS = {
  distortion: '∿',
  bitcrusher: '▥',
  overdrive: '⌁',
  boost: '↑',
  TubeScreamer: '↯',
  KlonCentaur: '♞',
  ProcoRAT: '⚲',
  BigMuffPi: 'π',
  FulltoneOCD: '◇',
  BossBD2: '◆',
  chorus: '≈',
  tremolo: '⌇',
  boss_ce2: '⚹ ',
  mxr_phase90: 'Φ',
  boss_bf2: '⨝',
  TCSubNUp: '↕',
  delay: '◉',
  DeluxeMemoryMan: '⧖',
  StrymonBluesky: '☁',
  compressor: '↔',
  noisegate: '⊣',
  sustain: '∞',
  lowpass: '⌁',
  highpass: '⌁',
  bandpass: '◇',
  notch: '⌄',
  eq: '≡',
  autowah: '◒',
  CryBabyMutron: '⤿',
};

export const EFFECT_LABELS = {
  FulltoneOCD: 'Fulltone OCD',
  BossBD2: 'BOSS BD-2 Blues Driver',
  TCSubNUp: "TC Electronic Sub 'N' Up",
};

export const EFFECT_TYPES = [
  { id: 'distortion', label: 'Distortion' },
  { id: 'modulation', label: 'Modulation' },
  { id: 'time', label: 'Time' },
  { id: 'dynamics', label: 'Dynamics' },
  { id: 'filter', label: 'Filter' },
];

const TYPE_BY_ID = Object.fromEntries(EFFECT_TYPES.map((type) => [type.id, type]));

const LEGACY_TYPE_BY_EFFECT = {
  distortion: TYPE_BY_ID.distortion,
  bitcrusher: TYPE_BY_ID.distortion,
  overdrive: TYPE_BY_ID.distortion,
  boost: TYPE_BY_ID.distortion,
  FulltoneOCD: TYPE_BY_ID.distortion,
  BossBD2: TYPE_BY_ID.distortion,
  chorus: TYPE_BY_ID.modulation,
  tremolo: TYPE_BY_ID.modulation,
  TCSubNUp: TYPE_BY_ID.modulation,
  delay: TYPE_BY_ID.time,
  compressor: TYPE_BY_ID.dynamics,
  noisegate: TYPE_BY_ID.dynamics,
  sustain: TYPE_BY_ID.dynamics,
  lowpass: TYPE_BY_ID.filter,
  highpass: TYPE_BY_ID.filter,
  bandpass: TYPE_BY_ID.filter,
  notch: TYPE_BY_ID.filter,
  eq: TYPE_BY_ID.filter,
  autowah: TYPE_BY_ID.filter,
};

export function effectColor(name) {
  return EFFECT_COLORS[name] || '#a762e9';
}

export function effectGlyph(name) {
  return EFFECT_GLYPHS[name] || '∿';
}

export function effectLabel(name) {
  return EFFECT_LABELS[name] || name;
}

export function effectType(name) {
  return LEGACY_TYPE_BY_EFFECT[name] || { id: 'other', label: 'Other' };
}

export function groupEffectsByType(effects) {
  const groups = new Map(EFFECT_TYPES.map((type) => [type.id, { ...type, effects: [] }]));
  effects.forEach((effect) => {
    const name = typeof effect === 'string' ? effect : effect.name;
    const category = typeof effect === 'string' ? null : effect.category;
    const type = TYPE_BY_ID[category] || effectType(name);
    if (!groups.has(type.id)) groups.set(type.id, { ...type, effects: [] });
    groups.get(type.id).effects.push(name);
  });
  return [...groups.values()].filter((group) => group.effects.length);
}
