import { useEffect, useState } from 'preact/hooks';

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
  useEffect(() => setLocalValue(Number(value)), [value]);
  const position = Math.min(1, Math.max(0, normalize(localValue, info)));
  const turn = -135 + position * 270;

  const updateFromSlider = (event) => {
    setLocalValue(denormalize(Number(event.currentTarget.value) / 1000, info));
  };
  const commit = (event) => {
    const committedValue = denormalize(Number(event.currentTarget.value) / 1000, info);
    setLocalValue(committedValue);
    onCommit(name, committedValue);
  };

  return (
    <label class="parameter-control">
      <span class="parameter-name">{info.label || name}</span>
      <span class="knob" style={{ '--knob-turn': `${turn}deg`, '--knob-progress': `${position * 270}deg` }} aria-hidden="true">
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
