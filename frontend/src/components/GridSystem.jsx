import { useEffect, useRef } from 'preact/hooks';
import { Lane } from './Lane.jsx';

function position(element) {
  return { lane: Number(element.dataset.lane), slot: Number(element.dataset.slot) };
}

export function GridSystem({ lanes, info, metadata, onOpenEffect, quickEdit, onQuickEdit, onQuickBypass, onQuickRemove, onAdd, onRoute, onMove }) {
  const rootRef = useRef(null);
  const dragRef = useRef(null);
  const overRef = useRef(null);
  const suppressClickUntil = useRef(0);

  useEffect(() => {
    const root = rootRef.current;
    if (!root) return;

    const clearOver = () => {
      overRef.current?.classList.remove('is-drop-target');
      overRef.current = null;
    };
    const markOver = (zone) => {
      if (!zone || overRef.current === zone) return;
      clearOver();
      overRef.current = zone;
      zone.classList.add('is-drop-target');
    };
    const finishMove = (from, target) => {
      if (!from || !target) return;
      let slot = target.slot;
      if (from.lane === target.lane) {
        if (slot === from.slot || slot === from.slot + 1) return;
        if (slot > from.slot) slot -= 1;
      }
      onMove(from.lane, from.slot, target.lane, slot);
    };

    const onDragStart = (event) => {
      const node = event.target.closest('.effect-node');
      if (!node) return;
      if (event.target.closest('.effect-quick-action')) return;
      dragRef.current = { source: position(node), node, desktop: true };
      node.classList.add('is-dragging');
      event.dataTransfer.effectAllowed = 'move';
      event.dataTransfer.setData('text/plain', `${node.dataset.lane}:${node.dataset.slot}`);
    };
    const onDragOver = (event) => {
      const zone = event.target.closest('.drop-zone');
      if (!zone) return;
      event.preventDefault();
      markOver(zone);
    };
    const onDrop = (event) => {
      const zone = event.target.closest('.drop-zone');
      if (!zone) return;
      event.preventDefault();
      finishMove(dragRef.current?.source, position(zone));
      clearOver();
    };
    const onDragEnd = () => {
      dragRef.current?.node?.classList.remove('is-dragging');
      dragRef.current = null;
      clearOver();
    };
    const onPointerDown = (event) => {
      if (event.pointerType === 'mouse') return;
      const node = event.target.closest('.effect-node');
      if (!node) return;
      if (event.target.closest('.effect-quick-action')) return;
      dragRef.current = {
        id: event.pointerId,
        node,
        source: position(node),
        x: event.clientX,
        y: event.clientY,
        dragging: false,
      };
      node.setPointerCapture(event.pointerId);
    };
    const onPointerMove = (event) => {
      const drag = dragRef.current;
      if (!drag || drag.desktop || drag.id !== event.pointerId) return;
      if (!drag.dragging && Math.hypot(event.clientX - drag.x, event.clientY - drag.y) > 12) {
        drag.dragging = true;
        drag.node.classList.add('is-dragging');
      }
      if (!drag.dragging) return;
      event.preventDefault();
      markOver(document.elementFromPoint(event.clientX, event.clientY)?.closest('.drop-zone'));
    };
    const onPointerUp = (event) => {
      const drag = dragRef.current;
      if (!drag || drag.desktop || drag.id !== event.pointerId) return;
      if (drag.dragging) {
        if (overRef.current) finishMove(drag.source, position(overRef.current));
        suppressClickUntil.current = performance.now() + 400;
        event.preventDefault();
      }
      drag.node.classList.remove('is-dragging');
      dragRef.current = null;
      clearOver();
    };
    const onClickCapture = (event) => {
      if (performance.now() < suppressClickUntil.current) {
        event.preventDefault();
        event.stopPropagation();
      }
    };

    root.addEventListener('dragstart', onDragStart);
    root.addEventListener('dragover', onDragOver);
    root.addEventListener('drop', onDrop);
    root.addEventListener('dragend', onDragEnd);
    root.addEventListener('pointerdown', onPointerDown);
    root.addEventListener('pointermove', onPointerMove, { passive: false });
    root.addEventListener('pointerup', onPointerUp);
    root.addEventListener('pointercancel', onPointerUp);
    root.addEventListener('click', onClickCapture, true);
    return () => {
      root.removeEventListener('dragstart', onDragStart);
      root.removeEventListener('dragover', onDragOver);
      root.removeEventListener('drop', onDrop);
      root.removeEventListener('dragend', onDragEnd);
      root.removeEventListener('pointerdown', onPointerDown);
      root.removeEventListener('pointermove', onPointerMove);
      root.removeEventListener('pointerup', onPointerUp);
      root.removeEventListener('pointercancel', onPointerUp);
      root.removeEventListener('click', onClickCapture, true);
    };
  }, [onMove]);

  return (
    <main class="grid-system" ref={rootRef}>
      {lanes.map((lane) => (
        <Lane
          key={lane.lane}
          lane={lane}
          info={info}
          metadata={metadata}
          onOpenEffect={onOpenEffect}
          quickEdit={quickEdit}
          onQuickEdit={onQuickEdit}
          onQuickBypass={onQuickBypass}
          onQuickRemove={onQuickRemove}
          onAdd={onAdd}
          onRoute={onRoute}
        />
      ))}
    </main>
  );
}
