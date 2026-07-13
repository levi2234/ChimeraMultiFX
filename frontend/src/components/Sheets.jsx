import { effectColor, effectGlyph } from '../effects.js';
import { ParameterKnob } from './ParameterKnob.jsx';

function Sheet({ title, eyebrow, onClose, children, className = '' }) {
  return (
    <div class="sheet-layer" role="presentation">
      <button class="sheet-backdrop" type="button" onClick={onClose} aria-label="Close panel" />
      <section class={`bottom-sheet ${className}`} role="dialog" aria-modal="true" aria-label={title}>
        <header class="sheet-header">
          <div><small>{eyebrow}</small><h2>{title}</h2></div>
          <button type="button" class="close-button" onClick={onClose}>×</button>
        </header>
        {children}
      </section>
    </div>
  );
}

export function EffectLibrarySheet({ lane, effects, onChoose, onClose }) {
  return (
    <Sheet title="Add effect" eyebrow={`LANE ${lane + 1}`} onClose={onClose} className="library-sheet">
      <div class="effect-library-grid">
        {effects.map((name) => (
          <button
            type="button"
            class="library-effect"
            style={{ '--effect-color': effectColor(name) }}
            onClick={() => onChoose(name)}
            key={name}
          >
            <span>{effectGlyph(name)}</span>
            <strong>{name}</strong>
          </button>
        ))}
      </div>
    </Sheet>
  );
}

export function ParameterSheet({ selection, effect, metadata, onSet, onBypass, onRemove, onClose }) {
  if (!effect) return null;
  return (
    <Sheet title={effect.name} eyebrow={`LANE ${selection.lane + 1} · SLOT ${selection.slot + 1}`} onClose={onClose} className="parameter-sheet">
      <div class="sheet-actions">
        <button type="button" class={effect.enabled ? 'active-action' : ''} onClick={() => onBypass(!effect.enabled)}>
          {effect.enabled ? 'BYPASS' : 'ENABLE'}
        </button>
        <button type="button" class="danger-action" onClick={onRemove}>REMOVE</button>
      </div>
      {!metadata && <div class="sheet-loading">Loading parameters…</div>}
      <div class="parameter-grid">
        {Object.entries(metadata?.params || {}).map(([name, info]) => (
          <ParameterKnob
            key={name}
            name={name}
            value={effect.params[name] ?? info.default}
            info={info}
            onCommit={onSet}
          />
        ))}
      </div>
    </Sheet>
  );
}

function ChoiceGroup({ label, values, selected, onSelect }) {
  return (
    <fieldset class="choice-group">
      <legend>{label}</legend>
      <div>
        {values.map((value) => (
          <button type="button" class={value === selected ? 'is-selected' : ''} onClick={() => onSelect(value)} key={value}>
            {value.toUpperCase()}
          </button>
        ))}
      </div>
    </fieldset>
  );
}

export function RouteSheet({ lane, info, onRoute, onLevel, onClear, onClose }) {
  return (
    <Sheet title={`Lane ${lane.lane + 1}`} eyebrow="ROUTING & LEVEL" onClose={onClose} className="route-sheet">
      <div class="route-layout">
        <ChoiceGroup label="Input" values={info.inputs} selected={lane.input} onSelect={(input) => onRoute(input, lane.output)} />
        <ChoiceGroup label="Output" values={info.outputs} selected={lane.output} onSelect={(output) => onRoute(lane.input, output)} />
        <label class="lane-level">
          <span>LEVEL</span>
          <input type="range" min="0" max="1.5" step="0.01" value={lane.level} onChange={(event) => onLevel(event.currentTarget.value)} />
          <output>{Number(lane.level).toFixed(2)}</output>
        </label>
        <button type="button" class="clear-lane danger-action" onClick={onClear}>CLEAR LANE</button>
      </div>
    </Sheet>
  );
}
