import { useEffect, useRef, useState } from 'preact/hooks';

function normalize(value, info) {
  const min = Number(info.min);
  const max = Number(info.max);
  if (info.scale === 'log' && min > 0 && value > 0) return Math.log(value / min) / Math.log(max / min);
  return (value - min) / (max - min);
}

function denormalize(position, info) {
  const min = Number(info.min);
  const max = Number(info.max);
  let value = info.scale === 'log' && min > 0
    ? min * Math.pow(max / min, position)
    : min + (max - min) * position;
  const step = Number(info.step) || 0;
  if (step) value = Math.round(value / step) * step;
  return Math.min(max, Math.max(min, value));
}

function format(value, info) {
  const step = Number(info.step) || 0.01;
  const decimals = step >= 1 ? 0 : Math.min(3, Math.max(1, Math.ceil(-Math.log10(step))));
  return `${Number(value).toFixed(decimals)}${info.unit || ''}`;
}

function optionLabels(info) {
  if (Array.isArray(info.options)) return info.options;
  if (typeof info.options === 'string') return info.options.split(',').map((option) => option.trim()).filter(Boolean);
  const min = Number(info.min) || 0;
  const max = Number(info.max) || min;
  return Array.from({ length: Math.max(0, Math.round(max - min) + 1) }, (_, index) => `${min + index}`);
}

function formatOptionLabel(label) {
  return String(label).replace(/[_-]/g, ' ').toUpperCase();
}

function ParameterSwitch({ name, value, info, onCommit }) {
  const min = Number(info.min) || 0;
  const labels = optionLabels(info);
  const selected = Math.round(Number(value ?? info.default ?? min));

  return (
    <fieldset class="parameter-control parameter-switch">
      <legend class="parameter-name">{info.label || name}</legend>
      <div class="parameter-switch-options">
        {labels.map((label, index) => {
          const optionValue = min + index;
          return (
            <button
              type="button"
              class={optionValue === selected ? 'is-selected' : ''}
              onClick={() => onCommit(name, optionValue)}
              aria-pressed={optionValue === selected}
              key={`${name}-${label}`}
            >
              {formatOptionLabel(label)}
            </button>
          );
        })}
      </div>
    </fieldset>
  );
}

export function ParameterKnob({ name, value, info, onCommit }) {
  if (info.type === 'switch' || info.type === 'enum') {
    return <ParameterSwitch name={name} value={value} info={info} onCommit={onCommit} />;
  }

  const [localValue, setLocalValue] = useState(Number(value));
  const dragRef = useRef(null);
  useEffect(() => setLocalValue(Number(value)), [value]);
  const position = Math.min(1, Math.max(0, normalize(localValue, info)));
  const turn = -135 + position * 270;
  const safePosition = Number.isFinite(position) ? position : 0;
  const level = Math.round(safePosition * 1000);

  const setValueFromPosition = (nextPosition) => {
    const clampedPosition = Math.min(1, Math.max(0, nextPosition));
    const nextValue = denormalize(clampedPosition, info);
    setLocalValue(nextValue);
  };

  const updateFromSlider = (event) => {
    setValueFromPosition(Number(event.currentTarget.value) / 1000);
  };

  const commit = (event) => {
    const committedValue = denormalize(Number(event.currentTarget.value) / 1000, info);
    setLocalValue(committedValue);
    onCommit(name, committedValue);
  };

  const handlePointerDown = (event) => {
    if (event.button !== undefined && event.button !== 0) return;
    dragRef.current = {
      pointerId: event.pointerId,
      startY: event.clientY,
      startPosition: position,
    };
    event.currentTarget.setPointerCapture(event.pointerId);
  };

  const handlePointerMove = (event) => {
    const drag = dragRef.current;
    if (!drag || drag.pointerId !== event.pointerId) return;
    const deltaY = drag.startY - event.clientY;
    const nextPosition = drag.startPosition + deltaY / 180;
    setValueFromPosition(nextPosition);
  };

  const handlePointerRelease = (event) => {
    const drag = dragRef.current;
    if (!drag || drag.pointerId !== event.pointerId) return;
    const committedValue = denormalize(position, info);
    setLocalValue(committedValue);
    onCommit(name, committedValue);
    dragRef.current = null;
  };

  return (
    <label class="parameter-control">
      <span class="parameter-name">{info.label || name}</span>
      <span
        class="knob"
        style={{ '--knob-turn': `${turn}deg`, '--knob-progress': `${safePosition * 270}deg` }}
        aria-hidden="true"
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerRelease}
        onPointerCancel={handlePointerRelease}
      >
        <span class="knob-cap" />
      </span>
      <input
        type="range"
        min="0"
        max="1000"
        step="1"
        value={level}
        class="parameter-slider"
        onInput={updateFromSlider}
        onPointerUp={commit}
        onKeyUp={commit}
        onBlur={commit}
        aria-label={info.label || name}
      />
      <output class="parameter-value">{format(localValue, info)}</output>
    </label>
  );
}
