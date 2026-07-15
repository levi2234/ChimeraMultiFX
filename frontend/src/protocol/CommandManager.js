const REQUEST_TIMEOUT_MS = 3500;
const HTTP_REQUEST_TIMEOUT_MS = 4000;
const HTTP_READ_ATTEMPTS = 3;
const HTTP_COMMANDS = new Set(['info', 'status']);

export class CommandManager {
  constructor(onConnection = () => {}) {
    this.onConnection = onConnection;
    this.socket = null;
    this.pending = null;
    this.queue = Promise.resolve();
  }

  connect() {
    if (this.socket && this.socket.readyState === 1) return Promise.resolve(true);
    if (!('WebSocket' in window)) return Promise.resolve(false);
    return new Promise((resolve) => {
      let settled = false;
      const scheme = location.protocol === 'https:' ? 'wss' : 'ws';
      const socket = new WebSocket(`${scheme}://${location.hostname}:81/ws`);
      this.socket = socket;
      const finish = (value) => {
        if (settled) return;
        settled = true;
        resolve(value);
      };
      const timer = setTimeout(() => finish(false), 1500);

      socket.onopen = () => {
        clearTimeout(timer);
        this.onConnection('websocket');
        finish(true);
      };
      socket.onmessage = (event) => {
        if (!this.pending) return;
        const pending = this.pending;
        this.pending = null;
        clearTimeout(pending.timer);
        pending.resolve(String(event.data).trim());
      };
      socket.onerror = () => finish(false);
      socket.onclose = () => {
        this.onConnection('http');
        if (this.pending) {
          clearTimeout(this.pending.timer);
          this.pending.reject(new Error('WebSocket disconnected'));
          this.pending = null;
        }
      };
    });
  }

  send(command) {
    this.queue = this.queue.catch(() => {}).then(() => this.request(command));
    return this.queue;
  }

  async request(command) {
    let response;
    const requiresHttp = HTTP_COMMANDS.has(command) || command.startsWith('effect ');
    if (!requiresHttp && this.socket && this.socket.readyState === 1) {
      response = await new Promise((resolve, reject) => {
        const timer = setTimeout(() => {
          this.pending = null;
          this.socket?.close();
          this.socket = null;
          reject(new Error(`Daisy timeout: ${command}`));
        }, REQUEST_TIMEOUT_MS);
        this.pending = { resolve, reject, timer };
        this.socket.send(command);
      });
    } else {
      response = await this.requestHttp(command, requiresHttp ? HTTP_READ_ATTEMPTS : 1);
    }
    if (response.startsWith('ERR')) throw new Error(response);
    return response;
  }

  async requestHttp(command, attempts) {
    let lastError;
    for (let attempt = 0; attempt < attempts; attempt += 1) {
      const controller = new AbortController();
      const timer = window.setTimeout(() => controller.abort(), HTTP_REQUEST_TIMEOUT_MS);
      try {
        const request = await fetch(`/api/daisy/command?cmd=${encodeURIComponent(command)}`, {
          signal: controller.signal,
        });
        const response = (await request.text()).trim();
        if (!request.ok) throw new Error(response || `HTTP ${request.status}`);
        return response;
      } catch (problem) {
        lastError = problem;
      } finally {
        window.clearTimeout(timer);
      }
    }
    throw lastError;
  }

  json(command) { return this.send(command).then(parseProtocolJson); }
  info() { return this.json('info'); }
  status() { return this.json('status'); }
  effect(name) { return this.json(`effect ${name}`); }
  add(lane, effect) { return this.send(`add ${lane} ${effect}`); }
  insert(lane, slot, effect) { return this.send(`insert ${lane} ${slot} ${effect}`); }
  remove(lane, slot) { return this.send(`remove ${lane} ${slot}`); }
  clear(lane) { return this.send(`clear ${lane}`); }
  move(fromLane, fromSlot, toLane, toSlot) { return this.send(`move ${fromLane} ${fromSlot} ${toLane} ${toSlot}`); }
  swap(lane, slotA, slotB) { return this.send(`swap ${lane} ${slotA} ${slotB}`); }
  route(lane, input, output) { return this.send(`route ${lane} ${input} ${output}`); }
  set(lane, slot, parameter, value) { return this.send(`set ${lane} ${slot} ${parameter} ${value}`); }
  get(lane, slot, parameter) { return this.send(`get ${lane} ${slot} ${parameter}`); }
  bypass(lane, slot, enabled) { return this.send(`bypass ${lane} ${slot} ${enabled ? 1 : 0}`); }
  level(lane, value) { return this.send(`level ${lane} ${value}`); }
}

export function parseProtocolJson(text) {
  try { return JSON.parse(text); }
  catch {
    try { return JSON.parse(`{${text}}`); }
    catch { throw new Error(`Invalid JSON from Daisy: ${text.slice(0, 80)}`); }
  }
}
