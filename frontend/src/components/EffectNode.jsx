import { effectColor, effectGlyph, effectType } from '../effects.js';
import { useRef } from 'preact/hooks';

export function EffectNode({ effect, lane, metadata, onOpen, quickEdit, onQuickEdit, onQuickBypass, onQuickRemove }) {
  const color = effectColor(effect.name);
  const category = metadata?.category || effectType(effect.name).label;
  const pressTimerRef = useRef(null);
  const longPressRef = useRef(false);

  const clearLongPress = () => {
    if (!pressTimerRef.current) return;
    window.clearTimeout(pressTimerRef.current);
    pressTimerRef.current = null;
  };

  const startLongPress = (event) => {
    if (event.target.closest('.effect-quick-action')) return;
    if (event.button !== undefined && event.button !== 0) return;
    clearLongPress();
    longPressRef.current = false;
    pressTimerRef.current = window.setTimeout(() => {
      pressTimerRef.current = null;
      longPressRef.current = true;
      onQuickEdit?.(lane, effect.slot);
    }, 520);
  };

  const openEffect = (event) => {
    if (event.target.closest('.effect-quick-action')) return;
    if (longPressRef.current) {
      longPressRef.current = false;
      return;
    }
    onOpen(lane, effect.slot);
  };

  return (
    <span class={`effect-node-wrap ${quickEdit ? 'is-quick-editing' : ''}`} style={{ '--effect-color': color }}>
      <button
        type="button"
        class={`effect-node ${effect.enabled ? '' : 'is-bypassed'} ${quickEdit ? 'is-quick-editing' : ''}`}
        style={{ '--effect-color': color }}
        data-lane={lane}
        data-slot={effect.slot}
        draggable
        onClick={openEffect}
        onPointerDown={startLongPress}
        onPointerMove={clearLongPress}
        onPointerUp={clearLongPress}
        onPointerCancel={clearLongPress}
        onPointerLeave={clearLongPress}
        aria-label={`${effect.name}, lane ${lane + 1}, slot ${effect.slot + 1}`}
      >
        <span class="effect-glyph" aria-hidden="true">{effectGlyph(effect.name)}</span>
        <span class="effect-label">{effect.name}</span>
        <span class="effect-category">{category}</span>
        <span class="effect-led" aria-hidden="true" />
      </button>
      {quickEdit && (
        <span class="effect-quick-actions" aria-hidden="false">
          <button
            type="button"
            class="effect-quick-action quick-bypass"
            onPointerDown={(event) => { event.preventDefault(); event.stopPropagation(); }}
            onClick={(event) => { event.preventDefault(); event.stopPropagation(); onQuickBypass?.(lane, effect.slot); }}
            aria-label={effect.enabled ? `Bypass ${effect.name}` : `Enable ${effect.name}`}
          >
            {effect.enabled ? 'B' : 'E'}
          </button>
          <button
            type="button"
            class="effect-quick-action quick-remove"
            onPointerDown={(event) => { event.preventDefault(); event.stopPropagation(); }}
            onClick={(event) => { event.preventDefault(); event.stopPropagation(); onQuickRemove?.(lane, effect.slot); }}
            aria-label={`Remove ${effect.name}`}
          >
            ×
          </button>
        </span>
      )}
    </span>
  );
}
