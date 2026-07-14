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

export function ParameterKnob({ name, value, info, onCommit }) {
  const [localValue, setLocalValue] = useState(Number(value));
  const dragRef = useRef(null);
  useEffect(() => setLocalValue(Number(value)), [value]);
  const position = Math.min(1, Math.max(0, normalize(localValue, info)));
  const turn = -135 + position * 270;

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
      startX: event.clientX,
      startPosition: position,
    };
    event.currentTarget.setPointerCapture(event.pointerId);
  };

  const handlePointerMove = (event) => {
    const drag = dragRef.current;
    if (!drag || drag.pointerId !== event.pointerId) return;
    const deltaX = event.clientX - drag.startX;
    const nextPosition = drag.startPosition + deltaX / 220;
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
        style={{ '--knob-turn': `${turn}deg`, '--knob-progress': `${position * 270}deg` }}
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
        value={Math.round(position * 1000)}
        onInput={updateFromSlider}
        onPointerUp={commit}
        onKeyUp={commit}
        onBlur={commit}
        aria-label={info.label || name}
      />
      <output>{format(localValue, info)}</output>
    </label>
  );
}
