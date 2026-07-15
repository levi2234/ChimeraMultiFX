import { effectColor, effectGlyph, effectType } from '../effects.js';

export function EffectNode({ effect, lane, metadata, onOpen }) {
  const color = effectColor(effect.name);
  const category = metadata?.category || effectType(effect.name).label;
  return (
    <button
      type="button"
      class={`effect-node ${effect.enabled ? '' : 'is-bypassed'}`}
      style={{ '--effect-color': color }}
      data-lane={lane}
      data-slot={effect.slot}
      draggable
      onClick={() => onOpen(lane, effect.slot)}
      aria-label={`${effect.name}, lane ${lane + 1}, slot ${effect.slot + 1}`}
    >
      <span class="effect-glyph" aria-hidden="true">{effectGlyph(effect.name)}</span>
      <span class="effect-label">{effect.name}</span>
      <span class="effect-category">{category}</span>
      <span class="effect-led" aria-hidden="true" />
    </button>
  );
}
