import { useCallback, useEffect, useMemo, useRef, useState } from 'preact/hooks';
import { GridSystem } from './components/GridSystem.jsx';
import { EffectLibrarySheet, ParameterSheet, RouteSheet } from './components/Sheets.jsx';
import { CommandManager } from './protocol/CommandManager.js';

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
  const [info, setInfo] = useState(null);
  const [lanes, setLanes] = useState([]);
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
    try {
      setCpuUsage(await commands.cpuUsage());
    } catch (problem) { showError(problem); }
  }, [commands, showError]);

  const refresh = useCallback(async () => {
    try {
      const [status, usage] = await Promise.all([commands.status(), commands.cpuUsage()]);
      setLanes(status.lanes);
      setCpuUsage(usage);
    } catch (problem) { showError(problem); }
  }, [commands, showError]);

  const mutate = useCallback(async (operation, closeSheet = false) => {
    setBusy(true);
    try {
      await operation();
      const [status, usage] = await Promise.all([commands.status(), commands.cpuUsage()]);
      setLanes(status.lanes);
      setCpuUsage(usage);
      if (closeSheet) setSheet(null);
    } catch (problem) {
      showError(problem);
      await refresh();
    } finally { setBusy(false); }
  }, [commands, refresh, showError]);

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
    const effect = lanes[lane]?.effects[slot];
    if (!effect) return;
    setQuickEdit(null);
    mutate(() => commands.bypass(lane, slot, !effect.enabled));
  }, [commands, lanes, mutate]);

  const removeEffect = useCallback((lane, slot) => {
    setQuickEdit(null);
    mutate(() => commands.remove(lane, slot), true);
  }, [commands, mutate]);

  const initialize = useCallback(async () => {
    setBusy(true);
    await commands.connect();
    let lastError;
    for (let attempt = 0; attempt < 3; attempt += 1) {
      try {
        const capabilities = await commands.info();
        const [status, usage] = await Promise.all([commands.status(), commands.cpuUsage()]);
        setInfo(capabilities);
        setLanes(status.lanes);
        setCpuUsage(usage);
        setBusy(false);
        return;
      } catch (problem) {
        lastError = problem;
        if (attempt < 2) await new Promise((resolve) => window.setTimeout(resolve, 400 * (attempt + 1)));
      }
    }
    setBusy(false);
    showError(lastError);
  }, [commands, showError]);

  useEffect(() => { initialize(); }, [initialize]);

  useEffect(() => {
    if (!info) return undefined;
    const timer = window.setInterval(refreshCpuUsage, 2000);
    return () => window.clearInterval(timer);
  }, [info, refreshCpuUsage]);

  const openEffect = useCallback(async (lane, slot) => {
    const effect = lanes[lane]?.effects[slot];
    if (!effect) return;
    setQuickEdit(null);
    setSheet({ type: 'parameter', lane, slot });
    if (metadata[effect.name]) return;
    try {
      const response = await commands.effect(effect.name);
      setMetadata((current) => ({ ...current, [effect.name]: response[effect.name] }));
    } catch (problem) { showError(problem); }
  }, [commands, lanes, metadata, showError]);

  const selectedEffect = useMemo(() => {
    if (sheet?.type !== 'parameter') return null;
    return lanes[sheet.lane]?.effects[sheet.slot] || null;
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
          onAdd={(lane) => setSheet({ type: 'library', lane })}
          onRoute={(lane) => setSheet({ type: 'route', lane })}
          onMove={(...args) => mutate(() => commands.move(...args), true)}
        />
      ) : <div class="boot-state"><span class="spinner" />CONNECTING TO DSP</div>}

      {sheet?.type === 'library' && info && (
        <EffectLibrarySheet
          lane={sheet.lane}
          effects={info.effects}
          onChoose={(name) => mutate(() => commands.add(sheet.lane, name), true)}
          onClose={() => setSheet(null)}
        />
      )}
      {sheet?.type === 'parameter' && selectedEffect && (
        <ParameterSheet
          selection={sheet}
          effect={selectedEffect}
          metadata={metadata[selectedEffect.name]}
          onSet={(parameter, value) => mutate(() => commands.set(sheet.lane, sheet.slot, parameter, Number(value).toPrecision(6)))}
          onBypass={(enabled) => mutate(() => commands.bypass(sheet.lane, sheet.slot, enabled))}
          onRemove={() => mutate(() => commands.remove(sheet.lane, sheet.slot), true)}
          onClose={() => setSheet(null)}
        />
      )}
      {sheet?.type === 'route' && routedLane && info && (
        <RouteSheet
          lane={routedLane}
          info={info}
          onRoute={(input, output) => mutate(() => commands.route(routedLane.lane, input, output))}
          onLevel={(value) => mutate(() => commands.level(routedLane.lane, value))}
          onClear={() => {
            if (confirm(`Clear lane ${routedLane.lane + 1}?`)) mutate(() => commands.clear(routedLane.lane), true);
          }}
          onClose={() => setSheet(null)}
        />
      )}
      {busy && <div class="busy-indicator" aria-label="Applying change"><span /></div>}
      {error && <div class="error-toast" role="alert">{error}</div>}
    </div>
  );
}
