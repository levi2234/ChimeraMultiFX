import { useCallback, useEffect, useMemo, useRef, useState } from 'preact/hooks';
import { GridSystem } from './components/GridSystem.jsx';
import { EffectLibrarySheet, ParameterSheet, RouteSheet } from './components/Sheets.jsx';
import { CommandManager } from './protocol/CommandManager.js';

const FALLBACK_INFO = {
  sample_rate: null,
  max_lanes: 4,
  max_slots: 8,
  effects: [],
  inputs: ['in1', 'in2', 'mix', 'lane0', 'lane1', 'lane2', 'lane3'],
  outputs: ['out1', 'out2', 'both', 'none'],
  commands: [],
  offline: true,
};

const FALLBACK_LANES = Array.from({ length: FALLBACK_INFO.max_lanes }, (_, lane) => ({
  lane,
  active: false,
  input: lane === 0 ? 'in1' : 'mix',
  output: lane === 0 ? 'both' : 'none',
  level: 1,
  effects: [],
}));

function Header({ connection, sampleRate, cpuUsage, busy, onRefresh }) {
  return (
    <header class="app-header">
      <div class="preset-number"><strong>1</strong><span>A</span></div>
      <div class="preset-name"><small>ACTIVE RIG</small><h1>CHIMERA MULTI FX</h1></div>
      <div class="header-status">
        <span class={`transport ${connection}`}>{connection === 'websocket' ? 'LIVE' : connection.toUpperCase()}</span>
        <span>{sampleRate ? `${sampleRate / 1000}K` : '--'}</span>
        <span>{Number.isFinite(cpuUsage) ? `${cpuUsage.toFixed(1)}% CPU` : '-- CPU'}</span>
        <button type="button" onClick={onRefresh} disabled={busy} aria-label="Refresh DSP state">↻</button>
      </div>
      <div class="mode-label">STOMP</div>
    </header>
  );
}

export function AppController() {
  const [info, setInfo] = useState(FALLBACK_INFO);
  const [lanes, setLanes] = useState(FALLBACK_LANES);
  const [cpuUsage, setCpuUsage] = useState(null);
  const [metadata, setMetadata] = useState({});
  const [connection, setConnection] = useState('offline');
  const [sheet, setSheet] = useState(null);
  const [quickEdit, setQuickEdit] = useState(null);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState('');
  const commandRef = useRef(null);
  if (!commandRef.current) commandRef.current = new CommandManager(setConnection);
  const commands = commandRef.current;

  const showError = useCallback((problem) => {
    setError(problem?.message || String(problem));
    window.setTimeout(() => setError(''), 3500);
  }, []);

  const refreshCpuUsage = useCallback(async () => {
    if (!info?.commands?.includes('cpu_usage')) return;
    try {
      setCpuUsage(await commands.cpuUsage({ dropIfBusy: true }));
    } catch (problem) {
      if (problem?.message !== 'Bridge busy' && !/timeout|unavailable/i.test(problem?.message || '')) showError(problem);
    }
  }, [commands, info, showError]);

  const mergeLane = useCallback((lane) => {
    setLanes((current) => {
      const next = [...current];
      next[lane.lane] = lane;
      return next;
    });
  }, []);

  const mergeSlot = useCallback((slot) => {
    setLanes((current) => current.map((lane, laneIndex) => {
      if (laneIndex !== slot.lane) return lane;
      return {
        ...lane,
        effects: lane.effects.map((effect) => effect.slot === slot.slot ? slot : effect),
      };
    }));
  }, []);

  const refreshLanes = useCallback(async (laneIndexes = null) => {
    let indexes = laneIndexes;
    if (!indexes) {
      const overview = await commands.status();
      indexes = Array.from({ length: overview.lane_count }, (_, lane) => lane);
    }
    for (const laneIndex of [...new Set(indexes)]) {
      mergeLane(await commands.statusLane(laneIndex));
    }
  }, [commands, mergeLane]);

  const refresh = useCallback(async () => {
    setBusy(true);
    try {
      await refreshLanes();
      if (info?.offline) {
        const capabilities = await commands.info();
        setInfo(capabilities);
      }
    } catch (problem) { showError(problem); }
    finally { setBusy(false); }
  }, [commands, info, refreshLanes, showError]);

  const mutate = useCallback(async (operation, { laneIndexes, slot, closeSheet = false } = {}) => {
    setBusy(true);
    try {
      await operation();
      if (closeSheet) setSheet(null);
      if (slot) mergeSlot(await commands.statusSlot(slot.lane, slot.slot));
      else await refreshLanes(laneIndexes);
    } catch (problem) {
      showError(problem);
      await refresh();
    } finally { setBusy(false); }
  }, [commands, mergeSlot, refresh, refreshLanes, showError]);

  const sameEffect = useCallback((a, b) => a && b && a.lane === b.lane && a.slot === b.slot, []);

  const toggleQuickEdit = useCallback((lane, slot) => {
    setQuickEdit((current) => sameEffect(current, { lane, slot }) ? null : { lane, slot });
  }, [sameEffect]);

  useEffect(() => {
    if (!quickEdit) return;
    const dismissQuickEdit = (event) => {
      if (event.target.closest('.effect-node-wrap')) return;
      setQuickEdit(null);
    };
    document.addEventListener('pointerdown', dismissQuickEdit, true);
    return () => document.removeEventListener('pointerdown', dismissQuickEdit, true);
  }, [quickEdit]);

  const bypassEffect = useCallback((lane, slot) => {
    if (info?.offline) return;
    const effect = lanes[lane]?.effects.find((candidate) => candidate.slot === slot);
    if (!effect) return;
    setQuickEdit(null);
    mutate(() => commands.bypass(lane, slot, !effect.enabled), { laneIndexes: [lane] });
  }, [commands, info, lanes, mutate]);

  const removeEffect = useCallback((lane, slot) => {
    if (info?.offline) return;
    setQuickEdit(null);
    mutate(() => commands.remove(lane, slot), { laneIndexes: [lane], closeSheet: true });
  }, [commands, info, mutate]);

  const initialize = useCallback(async ({ background = false } = {}) => {
    if (!background) setBusy(true);
    let lastError;
    try {
      const capabilities = await commands.info();
      setInfo(capabilities);
      await refreshLanes();
      if (capabilities.commands?.includes('cpu_usage')) {
        try { setCpuUsage(await commands.cpuUsage({ dropIfBusy: true })); }
        catch { setCpuUsage(null); }
      } else {
        setCpuUsage(null);
      }
      commands.connect().catch(() => setConnection('http'));
      if (!background) setBusy(false);
      return;
    } catch (problem) {
      lastError = problem;
    }
    if (!background) showError(lastError);
    if (!background) setBusy(false);
    else setConnection('http');
  }, [commands, refreshLanes, showError]);

  useEffect(() => {
    const timer = window.setTimeout(() => initialize({ background: true }), 750);
    return () => window.clearTimeout(timer);
  }, [initialize]);

  useEffect(() => {
    if (!info) return undefined;
    const timer = window.setInterval(refreshCpuUsage, 2000);
    return () => window.clearInterval(timer);
  }, [info, refreshCpuUsage]);

  const openEffect = useCallback(async (lane, slot) => {
    if (info?.offline) return;
    const effect = lanes[lane]?.effects.find((candidate) => candidate.slot === slot);
    if (!effect) return;
    setQuickEdit(null);
    setSheet({ type: 'parameter', lane, slot, effect });
    try {
      const slotStatus = await commands.statusSlot(lane, slot);
      setSheet((current) => sameEffect(current, { lane, slot })
        ? { ...current, effect: slotStatus }
        : current);
      mergeSlot(slotStatus);
      setMetadata((current) => ({
        ...current,
        [slotStatus.name]: {
          ...current[slotStatus.name],
          params: slotStatus.param_info,
        },
      }));
    } catch (problem) { showError(problem); }
  }, [commands, info, lanes, mergeSlot, showError]);

  const selectedEffect = useMemo(() => {
    if (sheet?.type !== 'parameter') return null;
    return lanes[sheet.lane]?.effects.find((effect) => effect.slot === sheet.slot) || sheet.effect || null;
  }, [lanes, sheet]);
  const routedLane = sheet?.type === 'route' ? lanes[sheet.lane] : null;

  return (
    <div class="touch-app">
      <Header connection={connection} sampleRate={info?.sample_rate} cpuUsage={cpuUsage} busy={busy} onRefresh={info ? refresh : initialize} />
      {info ? (
        <GridSystem
          lanes={lanes}
          info={info}
          metadata={metadata}
          onOpenEffect={openEffect}
          quickEdit={quickEdit}
          onQuickEdit={toggleQuickEdit}
          onQuickBypass={bypassEffect}
          onQuickRemove={removeEffect}
          onAdd={(lane) => info?.offline ? showError('Daisy unavailable; retry refresh') : setSheet({ type: 'library', lane })}
          onRoute={(lane) => info?.offline ? showError('Daisy unavailable; retry refresh') : setSheet({ type: 'route', lane })}
          onMove={(fromLane, fromSlot, toLane, toSlot) => info?.offline
            ? showError('Daisy unavailable; retry refresh')
            : mutate(() => commands.move(fromLane, fromSlot, toLane, toSlot), {
                laneIndexes: [fromLane, toLane],
                closeSheet: true,
              })}
        />
      ) : <div class="boot-state"><span class="spinner" />CONNECTING TO DSP</div>}

      {sheet?.type === 'library' && info && (
        <EffectLibrarySheet
          lane={sheet.lane}
          effects={info.effects}
          onChoose={(name) => mutate(() => commands.add(sheet.lane, name), {
            laneIndexes: [sheet.lane],
            closeSheet: true,
          })}
          onClose={() => setSheet(null)}
        />
      )}
      {sheet?.type === 'parameter' && selectedEffect && (
        <ParameterSheet
          selection={sheet}
          effect={selectedEffect}
          metadata={metadata[selectedEffect.name]}
          onSet={(parameter, value) => mutate(
            () => commands.set(sheet.lane, sheet.slot, parameter, Number(value).toPrecision(6)),
            { slot: { lane: sheet.lane, slot: sheet.slot } },
          )}
          onBypass={(enabled) => mutate(() => commands.bypass(sheet.lane, sheet.slot, enabled), {
            laneIndexes: [sheet.lane],
          })}
          onRemove={() => mutate(() => commands.remove(sheet.lane, sheet.slot), {
            laneIndexes: [sheet.lane],
            closeSheet: true,
          })}
          onClose={() => setSheet(null)}
        />
      )}
      {sheet?.type === 'route' && routedLane && info && (
        <RouteSheet
          lane={routedLane}
          info={info}
          onRoute={(input, output) => mutate(() => commands.route(routedLane.lane, input, output), {
            laneIndexes: [routedLane.lane],
          })}
          onLevel={(value) => mutate(() => commands.level(routedLane.lane, value), {
            laneIndexes: [routedLane.lane],
          })}
          onClear={() => {
            if (confirm(`Clear lane ${routedLane.lane + 1}?`)) {
              mutate(() => commands.clear(routedLane.lane), {
                laneIndexes: [routedLane.lane],
                closeSheet: true,
              });
            }
          }}
          onClose={() => setSheet(null)}
        />
      )}
      {busy && <div class="busy-indicator" aria-label="Applying change"><span /></div>}
      {error && <div class="error-toast" role="alert">{error}</div>}
    </div>
  );
}
