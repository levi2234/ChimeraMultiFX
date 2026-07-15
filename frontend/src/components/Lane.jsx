import { EffectNode } from './EffectNode.jsx';

function DropZone({ lane, slot }) {
  return <span class="drop-zone" data-lane={lane} data-slot={slot} aria-hidden="true" />;
}

function Endpoint({ side, value, lane, onPress }) {
  return (
    <button type="button" class={`endpoint endpoint-${side}`} onClick={onPress}>
      <span>{side === 'input' ? 'IN' : 'OUT'}</span>
      <strong>{value.toUpperCase()}</strong>
      <small>L{lane + 1}</small>
    </button>
  );
}

export function Lane({ lane, info, metadata, onOpenEffect, quickEdit, onQuickEdit, onQuickBypass, onQuickRemove, onAdd, onRoute }) {
  return (
    <section class={`lane-row ${lane.active ? 'is-active' : ''}`} aria-label={`Lane ${lane.lane + 1}`}>
      <Endpoint side="input" value={lane.input} lane={lane.lane} onPress={() => onRoute(lane.lane)} />
      <div class="chain-scroll">
        <div class="signal-line" aria-hidden="true" />
        <div class="chain-content">
          {lane.effects.map((effect, index) => (
            <span class="effect-position" key={`${effect.name}-${effect.slot}`}>
              <DropZone lane={lane.lane} slot={index} />
              <EffectNode
                effect={effect}
                lane={lane.lane}
                metadata={metadata[effect.name]}
                onOpen={onOpenEffect}
                quickEdit={quickEdit?.lane === lane.lane && quickEdit?.slot === effect.slot}
                onQuickEdit={onQuickEdit}
                onQuickBypass={onQuickBypass}
                onQuickRemove={onQuickRemove}
              />
            </span>
          ))}
          <DropZone lane={lane.lane} slot={lane.effects.length} />
          <button type="button" class="add-node" onClick={() => onAdd(lane.lane)} aria-label={`Add effect to lane ${lane.lane + 1}`}>+</button>
          {!lane.effects.length && <span class="empty-hint">ADD EFFECT</span>}
        </div>
      </div>
      <Endpoint side="output" value={lane.output} lane={lane.lane} onPress={() => onRoute(lane.lane)} />
      <span class="lane-number">{lane.lane + 1}</span>
    </section>
  );
}
