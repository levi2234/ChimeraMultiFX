import { effectColor, effectGlyph, groupEffectsByType } from '../effects.js';
import { ParameterKnob } from './ParameterKnob.jsx';
import { useState } from 'preact/hooks';

function Sheet({ title, eyebrow, onClose, children, className = '', style }) {
  const layerClass = className.includes('cortex-library-sheet') ? 'sheet-layer is-left-drawer' : 'sheet-layer';

  return (
    <div class={layerClass} role="presentation">
      <button class="sheet-backdrop" type="button" onClick={onClose} aria-label="Close panel" />
      <section class={`bottom-sheet ${className}`} style={style} role="dialog" aria-modal="true" aria-label={title}>
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
  const groupedEffects = groupEffectsByType(effects);
  const [selectedType, setSelectedType] = useState(groupedEffects[0]?.id);
  const selectedGroup = groupedEffects.find((group) => group.id === selectedType) || groupedEffects[0];

  return (
    <Sheet title="Effects" eyebrow={`LANE ${lane + 1}`} onClose={onClose} className="library-sheet cortex-library-sheet">
      <div class="cortex-library">
        <nav class="cortex-category-rail" aria-label="Effect categories">
          {groupedEffects.map((group) => (
            <button
              type="button"
              class={`cortex-category ${group.id === selectedGroup?.id ? 'is-selected' : ''}`}
              style={{ '--effect-color': effectColor(group.effects[0]) }}
              onClick={() => setSelectedType(group.id)}
              aria-label={group.label}
              aria-pressed={group.id === selectedGroup?.id}
              key={group.id}
            >
              <span aria-hidden="true">{effectGlyph(group.effects[0])}</span>
            </button>
          ))}
        </nav>
        <section class="cortex-effect-browser" aria-label={`${selectedGroup?.label || 'Effect'} list`}>
          <header class="cortex-tabs">
            <button type="button" class="is-selected">{selectedGroup?.label || 'Effects'}</button>
          </header>
          <div class="cortex-effect-list">
            {selectedGroup?.effects.map((name) => (
              <button type="button" class="cortex-effect-option" onClick={() => onChoose(name)} key={name}>
                <span>{name}</span>
              </button>
            ))}
          </div>
        </section>
      </div>
    </Sheet>
  );
}

export function ParameterSheet({ selection, effect, metadata, onSet, onBypass, onRemove, onClose }) {
  if (!effect) return null;
  return (
    <Sheet
      title={effect.name}
      eyebrow={`LANE ${selection.lane + 1} · SLOT ${selection.slot + 1}`}
      onClose={onClose}
      className="parameter-sheet"
      style={{ '--effect-color': effectColor(effect.name) }}
    >
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
