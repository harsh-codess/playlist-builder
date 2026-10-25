'use strict';

/* =========================================================================
   Helpers
   ========================================================================= */

const $ = (sel, root = document) => root.querySelector(sel);
const $$ = (sel, root = document) => Array.from(root.querySelectorAll(sel));

function esc(value) {
  return String(value ?? '').replace(/[&<>"']/g, ch => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
  }[ch]));
}

function debounce(fn, ms) {
  let t;
  return (...args) => { clearTimeout(t); t = setTimeout(() => fn(...args), ms); };
}

const pad = n => String(n).padStart(2, '0');

function fmtClock(sec) {
  const t = Math.max(0, Math.floor(sec || 0));
  const h = Math.floor(t / 3600), m = Math.floor((t % 3600) / 60), s = t % 60;
  return h ? `${h}:${pad(m)}:${pad(s)}` : `${m}:${pad(s)}`;
}

const fmtDur = sec => (sec > 0 ? fmtClock(sec) : '--:--');

function fmtTotal(sec) {
  const mins = Math.round((sec || 0) / 60);
  if (mins < 60) return `${mins} min`;
  const h = Math.floor(mins / 60), m = mins % 60;
  return m ? `${h} h ${m} min` : `${h} h`;
}

const plural = (n, word) => `${n} ${word}${n === 1 ? '' : 's'}`;

// Accepts "", "125" (seconds), "m:ss" or "h:mm:ss". Returns null when malformed.
function parseDuration(str) {
  const s = String(str || '').trim();
  if (!s) return 0;
  if (/^\d+$/.test(s)) return parseInt(s, 10);
  const parts = s.split(':');
  if (parts.length > 3 || parts.some(p => !/^\d+$/.test(p))) return null;
  const nums = parts.map(Number);
  if (nums.slice(1).some(n => n >= 60)) return null;
  return nums.reduce((acc, n) => acc * 60 + n, 0);
}

const isMobile = () => window.matchMedia('(max-width: 959px)').matches;

const TYPES = ['song', 'podcast', 'audiobook'];
const TYPE_META = {
  song: { label: 'Song', creator: 'Artist', icon: 'music' },
  podcast: { label: 'Podcast', creator: 'Host', icon: 'mic' },
  audiobook: { label: 'Audiobook', creator: 'Author', icon: 'book' },
};
const safeType = t => (TYPES.includes(t) ? t : 'song');

/* =========================================================================
   Icons (inline SVG)
   ========================================================================= */

const svg = body => `<svg class="i" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true" focusable="false">${body}</svg>`;
const dot = (cx, cy) => `<circle cx="${cx}" cy="${cy}" r="1.4" fill="currentColor" stroke="none"/>`;

const ICONS = {
  chevronDown: svg('<path d="M6 9l6 6 6-6"/>'),
  chevronUp: svg('<path d="M6 15l6-6 6 6"/>'),
  chevronLeft: svg('<path d="M15 6l-6 6 6 6"/>'),
  chevronRight: svg('<path d="M9 6l6 6-6 6"/>'),
  dots: svg(dot(12, 5) + dot(12, 12) + dot(12, 19)),
  more: svg(dot(5, 12) + dot(12, 12) + dot(19, 12)),
  heart: svg('<path d="M12 20s-7.2-4.4-8.9-9.1C2 7.8 4 4.6 7.3 4.6c2 0 3.5 1.1 4.7 2.8 1.2-1.7 2.7-2.8 4.7-2.8 3.3 0 5.3 3.2 4.2 6.3C19.2 15.6 12 20 12 20z"/>'),
  shuffle: svg('<path d="M3 7h3c2.2 0 3.4 1 4.5 2.8l3 5.4C14.6 16.9 15.8 18 18 18h3"/><path d="M3 17h3c1.3 0 2.3-.4 3.1-1.1"/><path d="M14.9 8.1C15.7 7.4 16.7 7 18 7h3"/><path d="M18.5 4.5L21 7l-2.5 2.5"/><path d="M18.5 15.5L21 18l-2.5 2.5"/>'),
  prev: svg('<path d="M6 5.5v13"/><path d="M18.5 6.2v11.6a.8.8 0 0 1-1.25.66L9.4 12.66a.8.8 0 0 1 0-1.32l7.85-5.8a.8.8 0 0 1 1.25.66z"/>'),
  next: svg('<path d="M18 5.5v13"/><path d="M5.5 6.2v11.6a.8.8 0 0 0 1.25.66l7.85-5.8a.8.8 0 0 0 0-1.32L6.75 5.54a.8.8 0 0 0-1.25.66z"/>'),
  play: svg('<path d="M7.5 5.6v12.8a.9.9 0 0 0 1.37.77l10.2-6.4a.9.9 0 0 0 0-1.54L8.87 4.83A.9.9 0 0 0 7.5 5.6z" fill="currentColor" stroke="none"/>'),
  pause: svg('<rect x="6.5" y="5" width="3.6" height="14" rx="1.1" fill="currentColor" stroke="none"/><rect x="13.9" y="5" width="3.6" height="14" rx="1.1" fill="currentColor" stroke="none"/>'),
  repeat: svg('<path d="M17 3l3 3-3 3"/><path d="M4 11V9.5A3.5 3.5 0 0 1 7.5 6H20"/><path d="M7 21l-3-3 3-3"/><path d="M20 13v1.5a3.5 3.5 0 0 1-3.5 3.5H4"/>'),
  home: svg('<path d="M4 10.2L12 4l8 6.2V19a1.5 1.5 0 0 1-1.5 1.5H15V15a1 1 0 0 0-1-1h-4a1 1 0 0 0-1 1v5.5H5.5A1.5 1.5 0 0 1 4 19z"/>'),
  queue: svg('<path d="M4 6h15M4 11h15M4 16h8"/><path d="M15.5 14v6l5-3z" fill="currentColor"/>'),
  wave: svg('<path d="M3 11v2M6 9v6M9 6v12M12 9.5v5M15 4v16M18 8v8M21 10.5v3"/>'),
  bell: svg('<path d="M6.5 16.5V11a5.5 5.5 0 0 1 11 0v5.5l1.5 2h-14z"/><path d="M10 20.5a2.1 2.1 0 0 0 4 0"/>'),
  search: svg('<circle cx="11" cy="11" r="6.5"/><path d="M20 20l-4.2-4.2"/>'),
  plus: svg('<path d="M12 5v14M5 12h14"/>'),
  close: svg('<path d="M6.5 6.5l11 11M17.5 6.5l-11 11"/>'),
  trash: svg('<path d="M4.5 7h15"/><path d="M9.5 7V5.2c0-.7.5-1.2 1.2-1.2h2.6c.7 0 1.2.5 1.2 1.2V7"/><path d="M6.5 7l.8 11.6c.1 1 .9 1.9 2 1.9h5.4c1.1 0 1.9-.9 2-1.9L17.5 7"/>'),
  grip: svg(dot(9, 6) + dot(15, 6) + dot(9, 12) + dot(15, 12) + dot(9, 18) + dot(15, 18)),
  download: svg('<path d="M12 4v11"/><path d="M7.5 10.5L12 15l4.5-4.5"/><path d="M5 19.5h14"/>'),
  copy: svg('<rect x="8" y="8" width="12" height="12" rx="2.5"/><path d="M16 8V6.5A2.5 2.5 0 0 0 13.5 4h-7A2.5 2.5 0 0 0 4 6.5v7A2.5 2.5 0 0 0 6.5 16H8"/>'),
  edit: svg('<path d="M4 20h4L19 9a2.8 2.8 0 0 0-4-4L4 16z"/><path d="M13.5 6.5l4 4"/>'),
  folder: svg('<path d="M3.5 7.5a2 2 0 0 1 2-2h4l2 2h7a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2h-13a2 2 0 0 1-2-2z"/>'),
  check: svg('<path d="M5 12.5l4.5 4.5L19 7.5"/>'),
  alert: svg('<circle cx="12" cy="12" r="8.5"/><path d="M12 7.5v5.5"/><circle cx="12" cy="16.4" r=".7" fill="currentColor"/>'),
  info: svg('<circle cx="12" cy="12" r="8.5"/><path d="M12 11v5.5"/><circle cx="12" cy="7.8" r=".7" fill="currentColor"/>'),
  music: svg('<path d="M9 18V6l11-2v12"/><circle cx="6.5" cy="18" r="2.5"/><circle cx="17.5" cy="16" r="2.5"/>'),
  mic: svg('<rect x="9" y="3.5" width="6" height="11" rx="3"/><path d="M5.5 11.5a6.5 6.5 0 0 0 13 0M12 18v2.5"/>'),
  book: svg('<path d="M12 6.5C10.5 5.2 8.3 4.5 4 4.5v13c4.3 0 6.5.7 8 2 1.5-1.3 3.7-2 8-2v-13c-4.3 0-6.5.7-8 2z"/><path d="M12 6.5v13"/>'),
  refresh: svg('<path d="M19.5 12a7.5 7.5 0 1 1-2.2-5.3"/><path d="M19.5 4.5v4h-4"/>'),
  listAdd: svg('<path d="M4 6h12M4 11h12M4 16h7"/><path d="M17.5 13v7M14 16.5h7"/>'),
  eraser: svg('<path d="M4 20h16"/><path d="M6.5 15.5l8.8-8.8a2 2 0 0 1 2.8 0l1.2 1.2a2 2 0 0 1 0 2.8L12.5 17.5H8.5z"/>'),
};

/* =========================================================================
   Deterministic album art (inline SVG data URIs, cached)
   ========================================================================= */

function hashStr(s) {
  let h = 2166136261 >>> 0;
  for (let i = 0; i < s.length; i++) { h ^= s.charCodeAt(i); h = Math.imul(h, 16777619); }
  return h >>> 0;
}

function rng(seed) {
  return () => {
    seed = (seed + 0x6D2B79F5) | 0;
    let t = Math.imul(seed ^ (seed >>> 15), 1 | seed);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

const ART_HUES = [262, 276, 290, 304, 318, 332, 346, 188, 176, 200, 214, 18, 30, 42];
const artCache = new Map();

function artUrl(item) {
  const key = item ? `${item.id}|${item.title || ''}` : '__none__';
  const cached = artCache.get(key);
  if (cached) return cached;

  const r = rng(hashStr(key));
  const R = (a, b) => a + r() * (b - a);
  const n = x => Math.round(x * 10) / 10;
  const hue = () => ART_HUES[Math.floor(r() * ART_HUES.length)] + R(-8, 8);
  const c = (h, s, l, a = 1) => `hsla(${n(((h % 360) + 360) % 360)},${n(s)}%,${n(l)}%,${a})`;
  const lin = (a, b, x2 = 1, y2 = 1) => `<linearGradient id="g" x1="0" y1="0" x2="${x2}" y2="${y2}"><stop offset="0" stop-color="${a}"/><stop offset="1" stop-color="${b}"/></linearGradient>`;
  const h1 = hue(), h2 = hue(), h3 = hue(), h4 = hue();
  const variant = item ? Math.floor(r() * 5) : -1;

  let defs = '<filter id="b" x="-60%" y="-60%" width="220%" height="220%"><feGaussianBlur stdDeviation="7"/></filter>'
    + '<filter id="s" x="-60%" y="-60%" width="220%" height="220%"><feGaussianBlur stdDeviation="1.6"/></filter>'
    + '<radialGradient id="v" cx=".5" cy=".5" r=".75"><stop offset=".55" stop-color="#000" stop-opacity="0"/><stop offset="1" stop-color="#000" stop-opacity=".3"/></radialGradient>';
  let body = '';

  if (variant === 0) {
    // Bloom: pastel backdrop with a cluster of petals
    defs += lin(c(h1, 70, R(76, 86)), c(h2, 65, R(58, 68)));
    body += `<g filter="url(#b)"><circle cx="${n(R(5, 40))}" cy="${n(R(60, 95))}" r="${n(R(20, 32))}" fill="${c(h3, 80, 72, 0.75)}"/><circle cx="${n(R(60, 95))}" cy="${n(R(5, 40))}" r="${n(R(18, 28))}" fill="${c(h4, 85, 88, 0.7)}"/></g>`;
    const sx = R(30, 46), sy = R(16, 26), ex = R(54, 70), ey = R(70, 84);
    const bend = R(-10, 12);
    body += `<path d="M${n(sx)} ${n(sy)} Q${n(sx - 6)} ${n(sy - 8)} ${n(sx + R(-2, 6))} 2" stroke="${c(h1, 40, 92, 0.8)}" stroke-width="0.9" fill="none"/>`;
    const count = 18 + Math.floor(r() * 10);
    for (let i = 0; i < count; i++) {
      const t = i / (count - 1);
      const rad = 2.5 + t * R(6, 9);
      const x = sx + (ex - sx) * t + Math.sin(t * Math.PI) * bend + R(-rad, rad) * 0.6;
      const y = sy + (ey - sy) * t + R(-2, 2);
      const col = c(h3 + t * R(-40, 40), R(55, 80), 84 - t * R(20, 34));
      body += `<circle cx="${n(x)}" cy="${n(y)}" r="${n(rad)}" fill="${col}"/>`;
      body += `<circle cx="${n(x - rad * 0.3)}" cy="${n(y - rad * 0.32)}" r="${n(rad * 0.45)}" fill="rgba(255,255,255,.32)"/>`;
    }
  } else if (variant === 1) {
    // Chrome sphere on a horizon
    defs += lin(c(h1, 18, R(82, 90)), c(h1, 22, R(46, 56)), 0, 1);
    defs += `<radialGradient id="sp" cx=".34" cy=".3" r=".78"><stop offset="0" stop-color="#fff"/><stop offset=".3" stop-color="${c(h1, 10, 84)}"/><stop offset=".78" stop-color="${c(h1, 14, 36)}"/><stop offset="1" stop-color="${c(h1, 20, 20)}"/></radialGradient>`;
    const hy = R(58, 70), sr = R(13, 19), cx = R(40, 60);
    body += `<path d="M0 ${n(hy)} L100 ${n(hy + R(-5, 5))} L100 100 L0 100Z" fill="${c(h2, 14, 38, 0.5)}"/>`;
    body += `<path d="M${n(cx - 34)} 100 L${n(cx - 3)} ${n(hy)} L${n(cx + 3)} ${n(hy)} L${n(cx + 34)} 100Z" fill="rgba(255,255,255,.14)"/>`;
    body += `<ellipse cx="${n(cx)}" cy="${n(hy + 2)}" rx="${n(sr * 1.1)}" ry="${n(sr * 0.22)}" fill="rgba(0,0,0,.35)" filter="url(#s)"/>`;
    body += `<circle cx="${n(cx)}" cy="${n(hy - sr * 0.9)}" r="${n(sr)}" fill="url(#sp)"/>`;
  } else if (variant === 2) {
    // Aurora night
    defs += lin(c(h1, 55, 8), c(h2, 60, 20), 0, 1);
    for (let i = 0; i < 16; i++) {
      body += `<circle cx="${n(R(0, 100))}" cy="${n(R(0, 55))}" r="${n(R(0.2, 0.6))}" fill="rgba(255,255,255,${n(R(0.3, 0.9))})"/>`;
    }
    body += '<g filter="url(#b)">';
    for (let i = 0; i < 3; i++) {
      const y0 = R(25, 60);
      body += `<path d="M-10 ${n(y0)} C30 ${n(y0 - R(10, 30))} 60 ${n(y0 + R(10, 30))} 110 ${n(y0 - R(0, 20))}" stroke="${c(i % 2 ? h4 : 176 + R(-12, 20), 85, R(55, 68), 0.85)}" stroke-width="${n(R(8, 16))}" fill="none"/>`;
    }
    body += '</g>';
    const gy = R(78, 88);
    body += `<path d="M0 ${n(gy)} Q30 ${n(gy - 7)} 55 ${n(gy - 2)} T100 ${n(gy - 5)} V100 H0Z" fill="${c(h1, 40, 5)}"/>`;
  } else if (variant === 3) {
    // Dunes under a soft sun
    defs += lin(c(h1, 55, R(80, 88)), c(h2, 50, R(58, 66)), 0, 1);
    body += `<circle cx="${n(R(25, 75))}" cy="${n(R(16, 32))}" r="${n(R(8, 13))}" fill="${c(h4, 90, 93, 0.9)}" filter="url(#s)"/>`;
    for (let k = 0; k < 3; k++) {
      const y = 52 + k * 14 + R(-3, 3);
      body += `<path d="M0 ${n(y)} C25 ${n(y - R(4, 12))} 55 ${n(y + R(2, 10))} 100 ${n(y - R(0, 10))} V100 H0Z" fill="${c(h3, 30 + k * 6, 60 - k * 15, 0.92)}"/>`;
    }
    body += `<rect x="${n(R(42, 58))}" y="${n(R(60, 66))}" width="1" height="3.2" rx=".5" fill="rgba(20,10,30,.7)"/>`;
  } else if (variant === 4) {
    // Colour splash
    defs += lin(c(h1, 70, R(12, 26)), c(h2, 75, R(30, 46)));
    body += '<g filter="url(#b)">';
    const hs = [h1, h2, h3, h4];
    for (let i = 0; i < 6; i++) {
      body += `<circle cx="${n(R(5, 95))}" cy="${n(R(5, 95))}" r="${n(R(10, 26))}" fill="${c(hs[i % 4] + R(-20, 20), 85, R(52, 72), 0.85)}"/>`;
    }
    body += '</g>';
    if (r() > 0.5) {
      body += `<circle cx="${n(R(35, 65))}" cy="${n(R(35, 65))}" r="${n(R(14, 24))}" fill="none" stroke="rgba(255,255,255,.55)" stroke-width=".7"/>`;
    } else {
      body += `<rect x="${n(R(30, 50))}" y="${n(R(30, 50))}" width="${n(R(16, 26))}" height="${n(R(16, 26))}" rx="4" transform="rotate(${n(R(-30, 30))} 50 50)" fill="${c(h3, 90, 72, 0.8)}"/>`;
    }
  } else {
    defs += lin('hsla(275,40%,22%,1)', 'hsla(250,45%,10%,1)');
    body += '<g filter="url(#b)"><circle cx="35" cy="35" r="24" fill="hsla(290,70%,55%,.5)"/><circle cx="70" cy="70" r="22" fill="hsla(220,70%,55%,.35)"/></g>';
  }

  const svgStr = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100" preserveAspectRatio="xMidYMid slice"><defs>${defs}</defs><rect width="100" height="100" fill="url(#g)"/>${body}<rect width="100" height="100" fill="url(#v)"/></svg>`;
  const url = `url('data:image/svg+xml;charset=utf-8,${encodeURIComponent(svgStr)}')`;
  artCache.set(key, url);
  return url;
}

const artStyle = item => `background-image:${artUrl(item)}`;

/* =========================================================================
   API
   ========================================================================= */

function makeError(type, message, status) {
  const err = new Error(message);
  err.type = type;
  err.status = status;
  return err;
}

async function errorFromResponse(res) {
  let data = null;
  try { data = await res.json(); } catch { /* non-JSON error body */ }
  const e = (data && data.error) || {};
  return makeError(e.type || 'HttpError', e.message || `Request failed (${res.status})`, e.status || res.status);
}

async function api(method, path, body) {
  const opts = { method, headers: { Accept: 'application/json' } };
  if (body !== undefined) {
    opts.headers['Content-Type'] = 'application/json';
    opts.body = JSON.stringify(body);
  }
  let res;
  try {
    res = await fetch(path, opts);
  } catch {
    throw makeError('NetworkError', 'Cannot reach the Playlist Builder server', 0);
  }
  if (!res.ok) throw await errorFromResponse(res);
  const text = await res.text();
  if (!text) return null;
  try { return JSON.parse(text); } catch { throw makeError('BadResponse', 'Server returned invalid JSON', res.status); }
}

const enc = encodeURIComponent;

/* =========================================================================
   State
   ========================================================================= */

const state = {
  view: 'library',
  mobileNow: false,
  lib: { q: '', type: 'all', sort: 'recent', items: [], loading: false, loaded: false, error: null, seq: 0 },
  stats: null,
  playlists: [],
  plLoaded: false,
  plError: null,
  detail: null,
  detailId: null,
  detailError: null,
  addType: 'song',
  activity: [],
  unread: 0,
};

// Canonical media objects by id, so every list shares the same instance.
const mediaById = new Map();

function intern(m) {
  if (!m || m.id == null) return m;
  const cur = mediaById.get(m.id);
  if (cur) { Object.assign(cur, m); return cur; }
  mediaById.set(m.id, m);
  return m;
}

/* =========================================================================
   Toasts, activity log, errors
   ========================================================================= */

function toast(message, kind = 'info', errType) {
  const host = $('#toasts');
  const el = document.createElement('div');
  el.className = `toast glass ${kind}`;
  const icon = kind === 'error' ? ICONS.alert : kind === 'success' ? ICONS.check : ICONS.info;
  el.innerHTML = `${icon}<div>${errType ? `<span class="tag">${esc(errType)}</span>` : ''}${esc(message)}</div>`;
  const dismiss = () => {
    if (el.classList.contains('out')) return;
    el.classList.add('out');
    setTimeout(() => el.remove(), 300);
  };
  el.addEventListener('click', dismiss);
  host.appendChild(el);
  while (host.children.length > 4) host.firstElementChild.remove();
  setTimeout(dismiss, 3500);
}

function logActivity(entry) {
  const visible = state.view === 'activity' && !(isMobile() && state.mobileNow);
  state.activity.unshift({ time: Date.now(), read: visible, ...entry });
  if (state.activity.length > 200) state.activity.length = 200;
  if (visible) VIEWS.activity.update();
  else state.unread++;
  updateNav();
}

function showError(err, context) {
  const type = err.type || 'Error';
  toast(err.message || String(err), 'error', type);
  logActivity({ kind: 'error', type, title: context || 'Something went wrong', message: err.message || String(err) });
}

function inlineErrorHtml(err) {
  return `${ICONS.alert}<div>${err.type && err.type !== 'ValidationError' ? `<span class="tag">${esc(err.type)}</span>` : ''}${esc(err.message)}</div>`;
}

// Shows the error inside a form, marks matching fields, and (for server errors) toasts + logs it.
function reportFormError(form, err, context) {
  const box = $('.form-error', form);
  if (box) { box.innerHTML = inlineErrorHtml(err); box.hidden = false; }
  $$('.input.invalid', form).forEach(i => i.classList.remove('invalid'));
  const msg = (err.message || '').toLowerCase();
  $$('.input[name]', form).forEach(input => {
    if (err.field === input.name || (!err.field && msg.includes(input.name.toLowerCase()))) input.classList.add('invalid');
  });
  if (err.type !== 'ValidationError') showError(err, context);
}

function clearFormError(form) {
  const box = $('.form-error', form);
  if (box) box.hidden = true;
  $$('.input.invalid', form).forEach(i => i.classList.remove('invalid'));
}

function setBusy(btn, busy, label) {
  if (!btn) return;
  if (busy) {
    btn.dataset.html = btn.innerHTML;
    btn.disabled = true;
    btn.innerHTML = `<span class="spinner"></span>${label ? esc(label) : ''}`;
  } else {
    btn.disabled = false;
    if (btn.dataset.html) btn.innerHTML = btn.dataset.html;
  }
}

/* =========================================================================
   Data loading
   ========================================================================= */

async function loadLibrary() {
  const lib = state.lib;
  const seq = ++lib.seq;
  lib.loading = true;
  if (state.view === 'library') VIEWS.library.update();
  try {
    const params = new URLSearchParams({ q: lib.q, type: lib.type, sort: lib.sort });
    const data = await api('GET', `/api/media?${params}`);
    if (seq !== lib.seq) return;
    lib.items = (data && data.items || []).map(intern);
    lib.error = null;
  } catch (err) {
    if (seq !== lib.seq) return;
    lib.error = err;
    showError(err, 'Could not load library');
  } finally {
    if (seq === lib.seq) {
      lib.loading = false;
      lib.loaded = true;
      if (state.view === 'library') VIEWS.library.update();
    }
  }
}

async function loadStats() {
  try {
    state.stats = await api('GET', '/api/stats');
  } catch {
    state.stats = null;
  }
  const el = $('#libStats');
  if (el) el.textContent = statsText();
}

function statsText() {
  const s = state.stats;
  if (!s) return '';
  return `${plural(s.total || 0, 'item')} · ${fmtTotal(s.totalDuration)} · ${plural(s.favorites || 0, 'favorite')}`;
}

async function loadPlaylists() {
  try {
    const data = await api('GET', '/api/playlists');
    state.playlists = (data && data.playlists) || [];
    state.plError = null;
  } catch (err) {
    state.plError = err;
    showError(err, 'Could not load playlists');
  }
  state.plLoaded = true;
  if (state.view === 'playlists') VIEWS.playlists.update();
}

function setDetail(d) {
  if (!d) return;
  d.tracks = (d.tracks || []).map(intern);
  if (state.detailId === d.id || state.view !== 'playlist') state.detail = d;
  const i = state.playlists.findIndex(p => p.id === d.id);
  const summary = { id: d.id, name: d.name, description: d.description, count: d.count, duration: d.duration, trackIds: d.trackIds || d.tracks.map(t => t.id) };
  if (i >= 0) state.playlists[i] = summary;
  if (state.view === 'playlist' && state.detailId === d.id) VIEWS.playlist.update();
}

async function loadDetail(id) {
  try {
    const d = await api('GET', `/api/playlists/${enc(id)}`);
    state.detailError = null;
    setDetail(d);
  } catch (err) {
    if (state.detailId === id) {
      state.detailError = err;
      if (state.view === 'playlist') VIEWS.playlist.update();
    }
    showError(err, 'Could not open playlist');
  }
}

function refreshAfterMediaChange() {
  loadLibrary();
  loadStats();
  loadPlaylists();
  if (state.view === 'playlist' && state.detailId) loadDetail(state.detailId);
}

/* =========================================================================
   Player
   ========================================================================= */

const audio = $('#audio');
const player = {
  queue: [], order: [], pos: -1, source: null,
  shuffle: false, repeat: 'off', playing: false,
  counted: false, errorShown: false,
  seeking: false, seekP: 0,
};

const current = () => (player.pos >= 0 ? player.queue[player.order[player.pos]] || null : null);
const hasSrc = () => !!audio.getAttribute('src');

function shuffled(arr) {
  for (let i = arr.length - 1; i > 0; i--) {
    const j = Math.floor(Math.random() * (i + 1));
    [arr[i], arr[j]] = [arr[j], arr[i]];
  }
  return arr;
}

function buildOrder(startIndex) {
  const n = player.queue.length;
  const idx = [...Array(n).keys()];
  if (!n) { player.order = []; player.pos = -1; return; }
  if (player.shuffle) {
    shuffled(idx);
    if (startIndex >= 0) {
      idx.splice(idx.indexOf(startIndex), 1);
      idx.unshift(startIndex);
    }
    player.order = idx;
    player.pos = 0;
  } else {
    player.order = idx;
    player.pos = startIndex >= 0 ? startIndex : 0;
  }
}

function setQueue(items, startIndex, source, shuffle) {
  if (!items.length) return;
  if (shuffle !== undefined) player.shuffle = shuffle;
  player.queue = items.slice();
  player.source = source;
  buildOrder(startIndex);
  loadCurrent(true);
}

function loadCurrent(autoplay) {
  const item = current();
  player.counted = false;
  player.errorShown = false;
  if (!item) {
    audio.pause();
    audio.removeAttribute('src');
    audio.load();
    player.playing = false;
    renderPlayer();
    return;
  }
  updateMediaSession(item);
  if (!item.playable || !item.streamUrl) {
    audio.pause();
    audio.removeAttribute('src');
    audio.load();
    player.playing = false;
    renderPlayer();
    if (autoplay) playbackError(item);
    return;
  }
  audio.src = item.streamUrl;
  renderPlayer();
  if (autoplay) audio.play().catch(onPlayReject);
}

function onPlayReject(err) {
  if (err && err.name === 'AbortError') return;
  player.playing = false;
  renderPlayState();
  if (err && err.name === 'NotSupportedError') playbackError(current());
}

function playbackError(item) {
  if (!item || player.errorShown) return;
  player.errorShown = true;
  const msg = `Can't play "${item.title}": file not found`;
  toast(msg, 'error');
  logActivity({ kind: 'error', type: 'MediaError', title: msg, message: item.source || item.streamUrl || '' });
}

function togglePlay() {
  const item = current();
  if (!item) {
    if (state.lib.items.length) setQueue(state.lib.items, 0, 'lib');
    else toast('Your library is empty — add something first');
    return;
  }
  if (!item.playable) { player.errorShown = false; playbackError(item); return; }
  if (audio.error || !hasSrc()) { loadCurrent(true); return; }
  if (audio.paused) audio.play().catch(onPlayReject);
  else audio.pause();
}

function next(auto = false) {
  if (!player.queue.length) return;
  if (player.pos + 1 < player.order.length) {
    player.pos++;
  } else if (player.repeat === 'all') {
    if (player.shuffle) player.order = shuffled(player.order.slice());
    player.pos = 0;
  } else {
    if (auto) { player.playing = false; renderPlayState(); }
    else toast('End of queue');
    return;
  }
  loadCurrent(true);
}

function prev() {
  if (!player.queue.length) return;
  if (hasSrc() && audio.currentTime > 3) { audio.currentTime = 0; return; }
  skipTo(-1);
}

function skipTo(delta) {
  const n = player.order.length;
  if (!n) return;
  let p = player.pos + delta;
  if (p < 0 || p >= n) {
    if (player.repeat !== 'all') { if (delta < 0 && hasSrc()) audio.currentTime = 0; return; }
    p = (p + n) % n;
  }
  player.pos = p;
  loadCurrent(true);
}

function neighbour(delta) {
  const n = player.order.length;
  if (n < 2 || player.pos < 0) return null;
  let p = player.pos + delta;
  if (p < 0 || p >= n) {
    if (player.repeat !== 'all') return null;
    p = (p + n) % n;
  }
  return player.queue[player.order[p]];
}

function toggleShuffle() {
  const qi = player.pos >= 0 ? player.order[player.pos] : -1;
  player.shuffle = !player.shuffle;
  if (player.queue.length) buildOrder(qi);
  renderPlayer();
  toast(player.shuffle ? 'Shuffle on' : 'Shuffle off');
}

function cycleRepeat() {
  player.repeat = { off: 'all', all: 'one', one: 'off' }[player.repeat];
  renderPlayer();
  toast({ off: 'Repeat off', all: 'Repeat all', one: 'Repeat one' }[player.repeat]);
}

function getDuration() {
  const d = audio.duration;
  if (hasSrc() && isFinite(d) && d > 0) return d;
  const item = current();
  return item ? item.duration || 0 : 0;
}

const canSeek = () => hasSrc() && !audio.error && isFinite(audio.duration) && audio.duration > 0;

function removeFromQueue(id) {
  const curIdx = player.pos >= 0 ? player.order[player.pos] : -1;
  const cur = current();
  const wasCurrent = cur && cur.id === id;
  const map = new Map();
  const keep = [];
  player.queue.forEach((m, i) => { if (m.id !== id) { map.set(i, keep.length); keep.push(m); } });
  const order = player.order.filter(i => map.has(i)).map(i => map.get(i));
  let pos;
  if (wasCurrent) {
    pos = Math.min(player.order.slice(0, player.pos).filter(i => map.has(i)).length, order.length - 1);
  } else {
    pos = curIdx >= 0 ? order.indexOf(map.get(curIdx)) : -1;
  }
  player.queue = keep;
  player.order = order;
  player.pos = order.length ? pos : -1;
  if (wasCurrent) loadCurrent(false);
  else renderPlayer();
}

audio.addEventListener('play', () => { player.playing = true; renderPlayState(); });
audio.addEventListener('pause', () => { player.playing = false; renderPlayState(); });
audio.addEventListener('timeupdate', updateProgress);
audio.addEventListener('durationchange', updateProgress);
audio.addEventListener('ended', () => {
  if (player.repeat === 'one') { audio.currentTime = 0; audio.play().catch(onPlayReject); }
  else next(true);
});
audio.addEventListener('playing', () => {
  const item = current();
  if (!item || player.counted) return;
  player.counted = true;
  api('POST', `/api/media/${enc(item.id)}/play`).then(intern).catch(() => {});
});
audio.addEventListener('loadedmetadata', () => {
  const item = current();
  updateProgress();
  if (!item || item.duration > 0 || !isFinite(audio.duration) || audio.duration <= 0) return;
  const duration = Math.round(audio.duration);
  item.duration = duration;
  renderNow();
  refreshLists();
  api('POST', `/api/media/${enc(item.id)}/duration`, { duration })
    .then(m => { intern(m); refreshLists(); loadStats(); })
    .catch(() => {});
});
audio.addEventListener('error', () => {
  if (!hasSrc()) return;
  player.playing = false;
  renderPlayState();
  playbackError(current());
});

function updateMediaSession(item) {
  if (!('mediaSession' in navigator) || !item) return;
  try {
    navigator.mediaSession.metadata = new MediaMetadata({
      title: item.title,
      artist: item.creator,
      album: item.album || item.show || item.typeLabel || '',
    });
  } catch { /* unsupported */ }
}

function initMediaSession() {
  if (!('mediaSession' in navigator)) return;
  const handlers = {
    play: () => togglePlay(),
    pause: () => audio.pause(),
    previoustrack: () => prev(),
    nexttrack: () => next(),
    seekto: d => { if (canSeek() && d.seekTime != null) audio.currentTime = d.seekTime; },
  };
  for (const [action, fn] of Object.entries(handlers)) {
    try { navigator.mediaSession.setActionHandler(action, fn); } catch { /* unsupported action */ }
  }
}

/* =========================================================================
   Now playing + mini player rendering
   ========================================================================= */

function renderPlayer() {
  renderNow();
  renderMini();
  renderPlayState();
  updateProgress();
  updateNav();
  refreshLists();
}

function renderNow() {
  const item = current();
  $('#nowTitle').textContent = item ? item.title : 'Not playing';
  $('#nowCreator').textContent = item ? item.creator : 'Pick something from your library';
  const meta = $('#nowMeta');
  if (item) {
    const t = safeType(item.type);
    meta.innerHTML = `<span class="type-chip ${t}">${esc(item.typeLabel || TYPE_META[t].label)}</span><span class="sub">${esc(item.subtitle || '')}</span>`;
  } else {
    meta.innerHTML = '';
  }
  const fav = $('#nowFav');
  fav.disabled = !item;
  fav.setAttribute('aria-pressed', String(!!(item && item.favorite)));
  fav.setAttribute('aria-label', item && item.favorite ? 'Remove from favorites' : 'Add to favorites');
  $('#discArt').style.backgroundImage = artUrl(item);

  const pv = neighbour(-1), nx = neighbour(1);
  const nbPrev = $('#nbPrev'), nbNext = $('#nbNext');
  nbPrev.hidden = !pv;
  nbNext.hidden = !nx;
  if (pv) { nbPrev.style.backgroundImage = artUrl(pv); nbPrev.setAttribute('aria-label', `Previous: ${pv.title}`); nbPrev.title = pv.title; }
  if (nx) { nbNext.style.backgroundImage = artUrl(nx); nbNext.setAttribute('aria-label', `Next: ${nx.title}`); nbNext.title = nx.title; }

  const shuffle = $('#btnShuffle');
  shuffle.setAttribute('aria-pressed', String(player.shuffle));
  const rep = $('#btnRepeat');
  rep.setAttribute('aria-pressed', String(player.repeat !== 'off'));
  rep.setAttribute('aria-label', `Repeat: ${player.repeat}`);
  rep.innerHTML = ICONS.repeat + (player.repeat === 'one' ? '<span class="repeat-one">1</span>' : '');

  document.title = item ? `${item.title} · ${item.creator} — Playlist Builder` : 'Playlist Builder';
}

function renderMini() {
  const item = current();
  const mini = $('#mini');
  mini.hidden = !item;
  $('#panelMain').classList.toggle('no-mini', !item);
  if (!item) return;
  $('#miniArt').style.backgroundImage = artUrl(item);
  $('#miniTitle').textContent = item.title;
  $('#miniCreator').textContent = item.creator;
}

function renderPlayState() {
  const playing = player.playing;
  const label = playing ? 'Pause' : 'Play';
  for (const btn of [$('#btnPlay'), $('#miniPlay')]) {
    btn.innerHTML = playing ? ICONS.pause : ICONS.play;
    btn.setAttribute('aria-label', label);
  }
  $('#ring').classList.toggle('is-playing', playing);
  $$('.nav-avatar').forEach(a => a.classList.toggle('spinning', playing));
  $$('.row.is-current .eq').forEach(e => e.classList.toggle('on', playing));
}

function drawArc(p) {
  const C = 50, R = 41;
  const bar = $('#arcBar'), knob = $('#arcKnob');
  const th = Math.min(Math.max(p, 0), 0.9999) * Math.PI * 2;
  const x = C - R * Math.sin(th), y = C + R * Math.cos(th);
  bar.setAttribute('d', p <= 0.001 ? `M${C} ${C + R}` : `M${C} ${C + R} A${R} ${R} 0 ${th > Math.PI ? 1 : 0} 1 ${x.toFixed(2)} ${y.toFixed(2)}`);
  knob.setAttribute('cx', x.toFixed(2));
  knob.setAttribute('cy', y.toFixed(2));
}

function updateProgress() {
  const dur = getDuration();
  const t = player.seeking ? player.seekP * dur : (hasSrc() ? audio.currentTime : 0);
  const p = dur > 0 ? Math.min(t / dur, 1) : 0;
  drawArc(p);
  $('#timeCur').textContent = fmtClock(t);
  $('#timeTotal').textContent = current() ? `/ ${fmtDur(dur)}` : '';
  $('#miniFill').style.width = `${(p * 100).toFixed(2)}%`;
  const ring = $('#ring');
  ring.setAttribute('aria-valuemax', String(Math.round(dur)));
  ring.setAttribute('aria-valuenow', String(Math.round(t)));
  ring.setAttribute('aria-valuetext', `${fmtClock(t)} of ${fmtDur(dur)}`);
}

function initRing() {
  const ring = $('#ring');
  let lastP = 0, justSeeked = false;

  const geometry = e => {
    const r = ring.getBoundingClientRect();
    const dx = e.clientX - (r.left + r.width / 2);
    const dy = e.clientY - (r.top + r.height / 2);
    let th = Math.atan2(-dx, dy);
    if (th < 0) th += Math.PI * 2;
    return { dist: Math.hypot(dx, dy) / (r.width / 2), p: th / (Math.PI * 2) };
  };

  ring.addEventListener('pointerdown', e => {
    const g = geometry(e);
    if (g.dist < 0.64 || !canSeek()) return;
    e.preventDefault();
    ring.setPointerCapture(e.pointerId);
    player.seeking = true;
    player.seekP = lastP = g.p;
    ring.classList.add('seeking');
    updateProgress();
  });
  ring.addEventListener('pointermove', e => {
    if (!player.seeking) return;
    let { p } = geometry(e);
    if (Math.abs(p - lastP) > 0.5) p = lastP > 0.5 ? 1 : 0;
    player.seekP = lastP = p;
    updateProgress();
  });
  const end = () => {
    if (!player.seeking) return;
    player.seeking = false;
    ring.classList.remove('seeking');
    if (canSeek()) audio.currentTime = Math.min(player.seekP, 0.999) * audio.duration;
    justSeeked = true;
    setTimeout(() => { justSeeked = false; }, 0);
    updateProgress();
  };
  ring.addEventListener('pointerup', end);
  ring.addEventListener('pointercancel', end);
  ring.addEventListener('click', e => {
    if (justSeeked) return;
    if (geometry(e).dist < 0.64) togglePlay();
  });
  ring.addEventListener('keydown', e => {
    if (!canSeek()) return;
    const step = { ArrowRight: 5, ArrowUp: 5, ArrowLeft: -5, ArrowDown: -5, PageUp: 30, PageDown: -30 }[e.key];
    if (step) { e.preventDefault(); audio.currentTime = Math.max(0, Math.min(audio.duration - 0.1, audio.currentTime + step)); }
    else if (e.key === 'Home') { e.preventDefault(); audio.currentTime = 0; }
  });
}

/* =========================================================================
   Navigation (both panels share one state)
   ========================================================================= */

const NAV_ITEMS = [
  { key: 'library', icon: 'home', label: 'Library' },
  { key: 'playlists', icon: 'queue', label: 'Playlists' },
  { key: 'now', label: 'Now playing' },
  { key: 'add', icon: 'wave', label: 'Add media' },
  { key: 'activity', icon: 'bell', label: 'Activity' },
];

function buildNav() {
  for (const nav of $$('.nav')) {
    nav.innerHTML = NAV_ITEMS.map(it => `<button class="nav-item" data-action="nav" data-key="${it.key}" aria-label="${it.label}" title="${it.label}">
      ${it.key === 'now' ? '<span class="nav-avatar art"></span>' : ICONS[it.icon]}
      ${it.key === 'activity' ? '<span class="badge-dot" hidden></span>' : ''}
    </button>`).join('');
  }
}

function updateNav() {
  const viewKey = state.view === 'playlist' ? 'playlists' : state.view;
  const active = isMobile() && state.mobileNow ? 'now' : viewKey;
  $$('.nav-item').forEach(btn => {
    const on = btn.dataset.key === active;
    btn.classList.toggle('active', on);
    if (on) btn.setAttribute('aria-current', 'page'); else btn.removeAttribute('aria-current');
  });
  const art = artUrl(current());
  $$('.nav-avatar').forEach(a => { a.style.backgroundImage = art; a.classList.toggle('spinning', player.playing); });
  $$('.badge-dot').forEach(b => { b.hidden = state.unread === 0; });
  const back = $('#mainBack');
  back.classList.toggle('main-back-left', state.view === 'playlist');
  back.setAttribute('aria-label', state.view === 'playlist' ? 'Back to playlists' : 'Scroll to top');
}

function applyMobile() {
  $('#stage').dataset.mobile = state.mobileNow ? 'now' : 'main';
  updateNav();
}

function openNow() {
  if (isMobile()) {
    state.mobileNow = true;
    applyMobile();
  } else {
    const panel = $('#panelNow');
    panel.classList.remove('pulse');
    void panel.offsetWidth;
    panel.classList.add('pulse');
    $('#ring').focus({ preventScroll: true });
  }
}

function showView(view) {
  state.view = view;
  state.mobileNow = false;
  closeMenu();
  mountView();
  applyMobile();
}

function mountView() {
  const host = $('#viewHost');
  host.innerHTML = `<div class="view" id="view-${state.view}">${VIEWS[state.view].shell()}</div>`;
  if (VIEWS[state.view].mounted) VIEWS[state.view].mounted($('.view', host));
  VIEWS[state.view].update();
}

function refreshLists() {
  if (state.view === 'library') VIEWS.library.update();
  else if (state.view === 'playlist') VIEWS.playlist.update();
}

/* =========================================================================
   Shared row / mosaic markup
   ========================================================================= */

function emptyHtml(icon, title, text, button) {
  return `<div class="empty"><div class="empty-icon">${ICONS[icon]}</div><h3>${esc(title)}</h3><p>${esc(text)}</p>${button || ''}</div>`;
}

function skeletonRows(count = 6) {
  return Array.from({ length: count }, () => '<div class="skel-row"><span class="skel a"></span><span class="lines"><span class="skel l1"></span><span class="skel l2"></span></span></div>').join('');
}

function mediaRowHtml(item, i, ctx) {
  const cur = current();
  const isCur = !!cur && cur.id === item.id;
  const t = safeType(item.type);
  const kind = t !== 'song' && state.lib.type === 'all' ? `<span class="kind"> · ${esc(item.typeLabel || TYPE_META[t].label)}</span>` : '';
  const warn = item.playable === false ? '<span class="warn"> · missing file</span>' : '';
  const fav = item.favorite ? `<span class="fav" aria-label="Favorite">${ICONS.heart}</span>` : '';
  const eq = isCur ? `<span class="eq${player.playing ? ' on' : ''}" aria-hidden="true"><i></i><i></i><i></i></span>` : '';
  const id = esc(item.id);
  const label = esc(`Play ${item.title} by ${item.creator}`);
  const art = `<button class="row-art-btn" data-action="play-row" data-ctx="${ctx}" data-index="${i}" tabindex="-1" aria-label="${label}"><span class="thumb art" style="display:block;${artStyle(item)}"></span><span class="play-ov">${ICONS.play}</span></button>`;
  const main = `<button class="row-main" data-action="play-row" data-ctx="${ctx}" data-index="${i}" aria-label="${label}">
      <span class="row-text"><span class="row-title"><span class="t">${esc(item.title)}</span>${fav}</span><span class="row-sub">${esc(item.subtitle || item.creator)}${kind}${warn}</span></span>
      <span class="row-dur">${eq}${fmtDur(item.duration)}</span>
    </button>`;

  if (ctx === 'pl') {
    const last = state.detail ? state.detail.tracks.length - 1 : 0;
    return `<div class="row track${isCur ? ' is-current' : ''}" draggable="true" data-id="${id}" data-index="${i}">
      <span class="grip" aria-hidden="true">${ICONS.grip}</span>${art}
      <div class="row-body">${main}
        <span class="reorder">
          <button data-action="move-track" data-from="${i}" data-to="${i - 1}" aria-label="Move up" ${i === 0 ? 'disabled' : ''}>${ICONS.chevronUp}</button>
          <button data-action="move-track" data-from="${i}" data-to="${i + 1}" aria-label="Move down" ${i === last ? 'disabled' : ''}>${ICONS.chevronDown}</button>
        </span>
        <button class="row-act danger" data-action="remove-track" data-index="${i}" aria-label="Remove from playlist">${ICONS.close}</button>
      </div>
    </div>`;
  }
  return `<div class="row${isCur ? ' is-current' : ''}" data-id="${id}" data-index="${i}">${art}
    <div class="row-body">${main}
      <button class="row-act" data-action="add-to-pl" data-id="${id}" aria-label="Add to playlist" title="Add to playlist">${ICONS.plus}</button>
      <button class="row-act" data-action="row-menu" data-id="${id}" aria-label="More options for ${esc(item.title)}" aria-haspopup="menu">${ICONS.more}</button>
    </div>
  </div>`;
}

function mosaicHtml(items) {
  const list = items.filter(Boolean);
  if (!list.length) return `<span class="mosaic empty">${ICONS.music}</span>`;
  if (list.length < 4) return `<span class="mosaic single"><span class="art" style="${artStyle(list[0])}"></span></span>`;
  return `<span class="mosaic">${list.slice(0, 4).map(m => `<span class="art" style="${artStyle(m)}"></span>`).join('')}</span>`;
}

const mosaicForIds = ids => mosaicHtml((ids || []).slice(0, 4).map(id => mediaById.get(id) || { id, title: '' }));
const plMeta = pl => `${plural(pl.count || 0, 'item')} · ${fmtTotal(pl.duration)}`;

/* =========================================================================
   Views
   ========================================================================= */

const FILTERS = [
  ['all', 'All'], ['song', 'Songs'], ['podcast', 'Podcasts'], ['audiobook', 'Audiobooks'], ['favorites', 'Favorites'],
];
const SORTS = [
  ['recent', 'Recently added'], ['title', 'Title'], ['creator', 'Creator'], ['duration', 'Duration'], ['plays', 'Most played'],
];

const VIEWS = {};

VIEWS.library = {
  shell() {
    const lib = state.lib;
    return `<div class="view-head">
      <div class="view-title-row"><h2 class="view-title">Library</h2></div>
      <p class="view-sub" id="libStats">${esc(statsText())}</p>
      <label class="search glass">${ICONS.search}
        <input id="libSearch" type="search" placeholder="Search titles, artists, hosts…" value="${esc(lib.q)}" aria-label="Search library" autocomplete="off" spellcheck="false">
        <button type="button" class="icon-btn" id="libClear" data-action="clear-search" aria-label="Clear search" ${lib.q ? '' : 'hidden'}>${ICONS.close}</button>
      </label>
      <div class="chips" role="group" aria-label="Filter by type">
        ${FILTERS.map(([k, l]) => `<button class="chip" data-action="filter" data-type="${k}" aria-pressed="${lib.type === k}">${l}</button>`).join('')}
      </div>
      <div class="toolbar">
        <span id="libCount"></span>
        <select class="select" id="libSort" aria-label="Sort by">
          ${SORTS.map(([k, l]) => `<option value="${k}" ${lib.sort === k ? 'selected' : ''}>${l}</option>`).join('')}
        </select>
      </div>
    </div>
    <div class="list" id="libList" role="list"></div>`;
  },
  mounted(root) {
    const input = $('#libSearch', root);
    const run = debounce(() => { state.lib.q = input.value.trim(); loadLibrary(); }, 250);
    input.addEventListener('input', () => { $('#libClear').hidden = !input.value; run(); });
    $('#libSort', root).addEventListener('change', e => { state.lib.sort = e.target.value; loadLibrary(); });
  },
  update() {
    const list = $('#libList');
    if (!list) return;
    const lib = state.lib;
    const count = $('#libCount');
    if (count) count.innerHTML = lib.loading && lib.loaded ? '<span class="spinner" style="display:inline-block;vertical-align:-3px"></span>' : (lib.loaded ? esc(plural(lib.items.length, 'result')) : '');
    $$('.chip', list.parentElement).forEach(c => c.setAttribute('aria-pressed', String(c.dataset.type === lib.type)));
    if (!lib.loaded) { list.innerHTML = skeletonRows(); return; }
    if (lib.error && !lib.items.length) {
      list.innerHTML = emptyHtml('alert', 'Library unavailable', lib.error.message, '<button class="btn" data-action="reload-library">Try again</button>');
      return;
    }
    if (!lib.items.length) {
      if (lib.q || lib.type !== 'all') {
        list.innerHTML = emptyHtml('search', 'Nothing found', lib.q ? `No results for “${lib.q}”.` : 'No items match this filter.', '<button class="btn" data-action="reset-filters">Reset filters</button>');
      } else {
        list.innerHTML = emptyHtml('music', 'Your library is empty', 'Add a song, podcast or audiobook, or scan a folder.', `<button class="btn btn-primary" data-action="nav" data-key="add">${ICONS.plus} Add media</button>`);
      }
      return;
    }
    list.innerHTML = lib.items.map((m, i) => mediaRowHtml(m, i, 'lib')).join('');
  },
};

VIEWS.playlists = {
  shell() {
    return `<div class="view-head">
      <div class="view-title-row"><h2 class="view-title">Playlists</h2>
        <button class="btn btn-sm" data-action="new-playlist">${ICONS.plus} New</button></div>
      <p class="view-sub" id="plSub"></p>
    </div>
    <div class="list" id="plList"></div>`;
  },
  update() {
    const list = $('#plList');
    if (!list) return;
    const sub = $('#plSub');
    if (!state.plLoaded) { list.innerHTML = skeletonRows(3); return; }
    const pls = state.playlists;
    sub.textContent = pls.length ? `${plural(pls.length, 'playlist')} · ${fmtTotal(pls.reduce((a, p) => a + (p.duration || 0), 0))}` : '';
    if (state.plError && !pls.length) {
      list.innerHTML = emptyHtml('alert', 'Playlists unavailable', state.plError.message, '<button class="btn" data-action="reload-playlists">Try again</button>');
      return;
    }
    if (!pls.length) {
      list.innerHTML = emptyHtml('queue', 'No playlists yet', 'Create one, then add tracks from your library with the + button.', `<button class="btn btn-primary" data-action="new-playlist">${ICONS.plus} New playlist</button>`);
      return;
    }
    list.innerHTML = pls.map(pl => {
      const id = esc(pl.id);
      return `<div class="row" data-pl="${id}">
        <button class="row-art-btn" data-action="open-pl" data-id="${id}" tabindex="-1" aria-label="Open ${esc(pl.name)}">${mosaicForIds(pl.trackIds)}</button>
        <div class="row-body">
          <button class="row-main" data-action="open-pl" data-id="${id}" aria-label="Open ${esc(pl.name)}">
            <span class="row-text"><span class="row-title"><span class="t">${esc(pl.name)}</span></span><span class="row-sub">${esc(plMeta(pl))}${pl.description ? ` · ${esc(pl.description)}` : ''}</span></span>
          </button>
          <button class="row-act" data-action="play-pl" data-id="${id}" aria-label="Play ${esc(pl.name)}" title="Play">${ICONS.play}</button>
          <span class="row-dur" style="color:var(--faint)">${ICONS.chevronRight.replace('class="i"', 'class="i" style="width:18px;height:18px"')}</span>
        </div>
      </div>`;
    }).join('');
  },
};

VIEWS.playlist = {
  shell() {
    return `<div class="view-head">
      <button class="back-link" data-action="nav" data-key="playlists">${ICONS.chevronLeft} Playlists</button>
      <div id="plHero"></div>
    </div>
    <div class="list" id="plTracks"></div>`;
  },
  mounted(root) { initTrackDnD(root); },
  update() {
    const hero = $('#plHero'), list = $('#plTracks');
    if (!hero || !list) return;
    const d = state.detail && state.detail.id === state.detailId ? state.detail : null;
    if (!d) {
      if (state.detailError) {
        hero.innerHTML = '';
        list.innerHTML = emptyHtml('alert', 'Playlist unavailable', state.detailError.message, '<button class="btn" data-action="nav" data-key="playlists">Back to playlists</button>');
      } else {
        hero.innerHTML = '<div class="pl-hero"><span class="mosaic skel" style="animation-duration:1.3s"></span><div class="pl-hero-text"><div class="skel" style="height:22px;width:70%"></div><div class="skel" style="height:12px;width:40%;margin-top:10px"></div></div></div>';
        list.innerHTML = skeletonRows(4);
      }
      return;
    }
    const tracks = d.tracks || [];
    hero.innerHTML = `<div class="pl-hero">${mosaicHtml(tracks.slice(0, 4))}
        <div class="pl-hero-text">
          <h2 class="pl-name">${esc(d.name)}</h2>
          ${d.description ? `<p class="pl-desc">${esc(d.description)}</p>` : ''}
          <p class="pl-meta">${esc(plMeta(d))}</p>
        </div>
      </div>
      <div class="actions">
        <button class="btn btn-primary" data-action="pl-play" ${tracks.length ? '' : 'disabled'}>${ICONS.play} Play</button>
        <button class="btn" data-action="pl-shuffle-play" ${tracks.length ? '' : 'disabled'}>${ICONS.shuffle} Shuffle</button>
        <button class="btn btn-icon" data-action="pl-export" aria-label="Export .m3u" title="Export .m3u">${ICONS.download}</button>
        <button class="btn btn-icon" data-action="pl-duplicate" aria-label="Duplicate playlist" title="Duplicate">${ICONS.copy}</button>
        <button class="btn btn-icon" data-action="pl-rename" aria-label="Rename playlist" title="Rename">${ICONS.edit}</button>
        <button class="btn btn-icon btn-danger" data-action="pl-delete" aria-label="Delete playlist" title="Delete">${ICONS.trash}</button>
      </div>`;
    list.innerHTML = tracks.length
      ? tracks.map((m, i) => mediaRowHtml(m, i, 'pl')).join('')
      : emptyHtml('listAdd', 'This playlist is empty', 'Add tracks from your library using the + button on any row.', '<button class="btn" data-action="nav" data-key="library">Browse library</button>');
  },
};

VIEWS.add = {
  shell() {
    const t = state.addType;
    const idx = TYPES.indexOf(t);
    return `<div class="view-head">
      <div class="view-title-row"><h2 class="view-title">Add media</h2></div>
      <p class="view-sub">Add a song, podcast episode or audiobook chapter.</p>
      <div class="seg glass" role="radiogroup" aria-label="Media type">
        <span class="seg-thumb" style="transform:translateX(${idx * 100}%)"></span>
        ${TYPES.map(k => `<button type="button" class="seg-btn" role="radio" data-action="add-type" data-type="${k}" aria-checked="${k === t}">${ICONS[TYPE_META[k].icon]}${TYPE_META[k].label}</button>`).join('')}
      </div>
    </div>
    <form class="card glass form" id="addForm" novalidate>
      <div class="form-error" hidden></div>
      <div class="form" id="addFields">${mediaFieldsHtml(t, {})}</div>
      <button class="btn btn-primary btn-block" type="submit">${ICONS.plus} Add to library</button>
    </form>
    <section class="card glass" aria-labelledby="scanTitle">
      <h3 class="card-title" id="scanTitle">Scan a folder</h3>
      <p class="card-sub">Imports every audio file in a folder on the machine running the server.</p>
      <form class="form" id="scanForm" novalidate>
        <div class="form-error" hidden></div>
        <div class="inline-row">
          <input class="input" name="folder" id="scanFolder" placeholder="/Users/you/Music" autocomplete="off" spellcheck="false" aria-label="Absolute folder path">
          <button class="btn" type="submit">${ICONS.folder} Scan</button>
        </div>
        <div class="form-ok" id="scanResult" hidden></div>
      </form>
    </section>`;
  },
  mounted(root) {
    const form = $('#addForm', root);
    form.addEventListener('input', e => { e.target.classList.remove('invalid'); });
    form.addEventListener('submit', async e => {
      e.preventDefault();
      clearFormError(form);
      const btn = $('[type=submit]', form);
      let body;
      try { body = collectMedia(form, state.addType); } catch (err) { reportFormError(form, err); return; }
      setBusy(btn, true, 'Adding…');
      try {
        const m = intern(await api('POST', '/api/media', body));
        toast(`Added “${m.title}”`, 'success');
        logActivity({ kind: 'added', title: `Added “${m.title}”`, message: m.subtitle || `${m.typeLabel} · ${m.creator}` });
        $('#addFields').innerHTML = mediaFieldsHtml(state.addType, {});
        refreshAfterMediaChange();
      } catch (err) {
        reportFormError(form, err, 'Could not add media');
      } finally {
        setBusy(btn, false);
      }
    });

    const scan = $('#scanForm', root);
    scan.addEventListener('submit', async e => {
      e.preventDefault();
      clearFormError(scan);
      const out = $('#scanResult');
      out.hidden = true;
      const folder = $('#scanFolder').value.trim();
      if (!folder) { reportFormError(scan, Object.assign(makeError('ValidationError', 'Enter an absolute folder path'), { field: 'folder' })); return; }
      const btn = $('[type=submit]', scan);
      setBusy(btn, true, 'Scanning…');
      try {
        const res = await api('POST', '/api/scan', { folder });
        (res.items || []).forEach(intern);
        const msg = `Added ${res.added || 0}, skipped ${res.skipped || 0}`;
        out.innerHTML = `${ICONS.check}<div>${esc(msg)}</div>`;
        out.hidden = false;
        toast(`Scan complete · ${msg}`, 'success');
        logActivity({ kind: 'added', title: `Scanned ${folder}`, message: msg });
        refreshAfterMediaChange();
      } catch (err) {
        reportFormError(scan, err, `Scan failed: ${folder}`);
      } finally {
        setBusy(btn, false);
      }
    });
  },
  update() {},
};

VIEWS.activity = {
  shell() {
    return `<div class="view-head">
      <div class="view-title-row"><h2 class="view-title">Activity</h2>
        <button class="btn btn-sm" data-action="clear-activity" id="actClear">${ICONS.eraser} Clear</button></div>
      <p class="view-sub">Events from this session</p>
    </div>
    <div class="act-list" id="actList"></div>`;
  },
  update() {
    const list = $('#actList');
    if (!list) return;
    $('#actClear').disabled = !state.activity.length;
    if (!state.activity.length) {
      list.innerHTML = emptyHtml('bell', 'No activity yet', 'Adds, deletes, exports and errors will show up here.');
    } else {
      const icon = { added: 'plus', deleted: 'trash', exported: 'download', error: 'alert', created: 'queue', info: 'info' };
      list.innerHTML = state.activity.map(a => `<div class="act-item ${esc(a.kind)}${a.read ? '' : ' unread'}">
        <span class="act-icon">${ICONS[icon[a.kind] || 'info']}</span>
        <div class="act-text">
          <div class="act-title">${a.kind === 'error' && a.type ? `<span class="tag">${esc(a.type)}</span>` : ''}${esc(a.title)}</div>
          ${a.message ? `<div class="act-msg">${esc(a.message)}</div>` : ''}
        </div>
        <time class="act-time">${new Date(a.time).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}</time>
      </div>`).join('');
    }
    state.activity.forEach(a => { a.read = true; });
    state.unread = 0;
    updateNav();
  },
};

/* =========================================================================
   Media form fields (Add view + Edit sheet)
   ========================================================================= */

function mediaFieldsHtml(type, v) {
  const field = (name, label, ph, attrs = '') =>
    `<label class="field"><span>${label}</span><input class="input" name="${name}" value="${esc(v[name] ?? '')}" placeholder="${esc(ph)}" autocomplete="off" ${attrs}></label>`;
  const num = 'inputmode="numeric"';
  let numField = '', extra = '';
  if (type === 'song') {
    numField = field('year', 'Year', '2024', num);
    extra = `<div class="grid-2">${field('album', 'Album', 'Horizons')}${field('genre', 'Genre', 'Electronic')}</div>`;
  } else if (type === 'podcast') {
    numField = field('episode', 'Episode #', '12', num);
    extra = field('show', 'Show', 'The Daily Build');
  } else {
    numField = field('chapter', 'Chapter #', '3', num);
    extra = field('narrator', 'Narrator', 'Stephen Fry');
  }
  return `${field('title', 'Title', type === 'song' ? 'Odyssey' : type === 'podcast' ? 'Episode title' : 'Chapter title')}
    ${field('creator', TYPE_META[type].creator, type === 'song' ? 'Roary' : type === 'podcast' ? 'Host name' : 'Author name')}
    <div class="grid-2">${field('duration', 'Duration', 'mm:ss', num)}${numField}</div>
    ${extra}
    ${field('source', 'Source', '/path/to/file.mp3 or https://…', 'spellcheck="false"')}`;
}

function collectMedia(form, type) {
  const fd = new FormData(form);
  const g = k => String(fd.get(k) ?? '').trim();
  const bad = (field, message) => Object.assign(makeError('ValidationError', message), { field });
  const duration = parseDuration(g('duration'));
  if (duration === null) throw bad('duration', 'Duration must look like mm:ss, e.g. 3:25');
  const int = (k, label) => {
    const s = g(k);
    if (!s) return 0;
    if (!/^\d+$/.test(s)) throw bad(k, `${label} must be a whole number`);
    return parseInt(s, 10);
  };
  const body = { type, title: g('title'), creator: g('creator'), duration, source: g('source') };
  if (type === 'song') Object.assign(body, { album: g('album'), genre: g('genre'), year: int('year', 'Year') });
  else if (type === 'podcast') Object.assign(body, { show: g('show'), episode: int('episode', 'Episode number') });
  else Object.assign(body, { narrator: g('narrator'), chapter: int('chapter', 'Chapter number') });
  return body;
}

function mediaValues(item) {
  const v = { ...item, duration: item.duration > 0 ? fmtClock(item.duration) : '' };
  for (const k of ['year', 'episode', 'chapter']) if (!v[k]) v[k] = '';
  return v;
}

/* =========================================================================
   Menu (glass dropdown)
   ========================================================================= */

let menuItems = [];
let menuAnchor = null;

function openMenu(anchor, items) {
  const menu = $('#menu');
  if (!menu.hidden && menuAnchor === anchor) { closeMenu(); return; }
  menuItems = items.filter(Boolean);
  menuAnchor = anchor;
  menu.innerHTML = menuItems.map((it, i) => it === 'sep'
    ? '<div class="menu-sep" role="separator"></div>'
    : `<button class="menu-item${it.danger ? ' danger' : ''}" role="menuitem" data-mi="${i}" ${it.disabled ? 'disabled' : ''}>${ICONS[it.icon] || ''}<span>${esc(it.label)}</span></button>`).join('');
  menu.hidden = false;
  const r = anchor.getBoundingClientRect();
  const w = menu.offsetWidth, h = menu.offsetHeight;
  let left = Math.min(r.right - w, window.innerWidth - w - 8);
  left = Math.max(8, left);
  let top = r.bottom + 6;
  if (top + h > window.innerHeight - 8) top = Math.max(8, r.top - h - 6);
  menu.style.left = `${left}px`;
  menu.style.top = `${top}px`;
  anchor.setAttribute('aria-expanded', 'true');
  const first = $('.menu-item:not(:disabled)', menu);
  if (first) first.focus({ preventScroll: true });
}

function closeMenu() {
  const menu = $('#menu');
  if (menu.hidden) return;
  menu.hidden = true;
  if (menuAnchor) {
    menuAnchor.setAttribute('aria-expanded', 'false');
    if (menu.contains(document.activeElement)) menuAnchor.focus({ preventScroll: true });
  }
  menuAnchor = null;
}

function initMenu() {
  const menu = $('#menu');
  menu.addEventListener('click', e => {
    const btn = e.target.closest('[data-mi]');
    if (!btn || btn.disabled) return;
    const it = menuItems[Number(btn.dataset.mi)];
    closeMenu();
    if (it && it.run) it.run();
  });
  menu.addEventListener('keydown', e => {
    if (e.key !== 'ArrowDown' && e.key !== 'ArrowUp') return;
    e.preventDefault();
    const items = $$('.menu-item:not(:disabled)', menu);
    const i = items.indexOf(document.activeElement);
    const nextI = (i + (e.key === 'ArrowDown' ? 1 : -1) + items.length) % items.length;
    items[nextI].focus();
  });
  document.addEventListener('pointerdown', e => {
    if (!menu.hidden && !menu.contains(e.target) && !(menuAnchor && menuAnchor.contains(e.target))) closeMenu();
  });
  window.addEventListener('resize', closeMenu);
  document.addEventListener('scroll', e => { if (!menu.contains(e.target)) closeMenu(); }, true);
}

function mediaMenuItems(item, { includePlay } = {}) {
  if (!item) return [];
  return [
    includePlay && { label: 'Play', icon: 'play', run: () => playMediaById(item.id) },
    { label: 'Add to playlist…', icon: 'listAdd', run: () => addToPlaylistSheet(item.id) },
    { label: item.favorite ? 'Remove from favorites' : 'Add to favorites', icon: 'heart', run: () => toggleFavorite(item.id) },
    { label: 'Edit details…', icon: 'edit', run: () => editMediaSheet(item) },
    'sep',
    { label: 'Delete from library', icon: 'trash', danger: true, run: () => deleteMedia(item.id) },
  ];
}

function viewMenuItems() {
  const d = state.detail;
  switch (state.view) {
    case 'library': return [
      { label: 'Refresh', icon: 'refresh', run: () => { loadLibrary(); loadStats(); } },
      { label: 'Scan a folder…', icon: 'folder', run: () => { showView('add'); setTimeout(() => $('#scanFolder') && $('#scanFolder').focus(), 50); } },
      { label: 'Reset search & filters', icon: 'eraser', run: resetFilters, disabled: !state.lib.q && state.lib.type === 'all' && state.lib.sort === 'recent' },
    ];
    case 'playlists': return [
      { label: 'New playlist…', icon: 'plus', run: newPlaylistSheet },
      { label: 'Refresh', icon: 'refresh', run: loadPlaylists },
    ];
    case 'playlist': return [
      { label: 'Rename', icon: 'edit', run: renamePlaylistSheet, disabled: !d },
      { label: 'Duplicate', icon: 'copy', run: duplicatePlaylistSheet, disabled: !d },
      { label: 'Shuffle', icon: 'shuffle', run: shufflePlaylistOrder, disabled: !d || d.tracks.length < 2 },
      { label: 'Export .m3u', icon: 'download', run: () => exportPlaylist(d), disabled: !d },
      'sep',
      { label: 'Delete playlist', icon: 'trash', danger: true, run: deletePlaylist, disabled: !d },
    ];
    case 'add': return [
      { label: 'Clear form', icon: 'eraser', run: () => { const f = $('#addForm'); if (f) { $('#addFields').innerHTML = mediaFieldsHtml(state.addType, {}); clearFormError(f); } } },
      { label: 'Go to library', icon: 'home', run: () => showView('library') },
    ];
    case 'activity': return [
      { label: 'Mark all as read', icon: 'check', run: () => VIEWS.activity.update(), disabled: !state.activity.length },
      { label: 'Clear activity', icon: 'eraser', run: clearActivity, disabled: !state.activity.length },
    ];
    default: return [];
  }
}

/* =========================================================================
   Sheets (modal dialogs)
   ========================================================================= */

let sheetReturnFocus = null;
let sheetOnClose = null;

function openSheet(html, onMount, onClose) {
  closeMenu();
  const backdrop = $('#sheetBackdrop'), sheet = $('#sheet');
  if (!backdrop.hidden && sheetOnClose) sheetOnClose();
  sheetReturnFocus = backdrop.hidden ? document.activeElement : sheetReturnFocus;
  sheetOnClose = onClose || null;
  sheet.innerHTML = html;
  backdrop.hidden = false;
  const heading = $('h3', sheet);
  if (heading) { heading.id = 'sheetTitle'; sheet.setAttribute('aria-labelledby', 'sheetTitle'); }
  $$('[data-close]', sheet).forEach(b => b.addEventListener('click', () => closeSheet()));
  if (onMount) onMount(sheet);
  const focusEl = $('input, textarea', sheet) || $('.btn-primary, .sheet-row, .btn', sheet);
  if (focusEl) setTimeout(() => focusEl.focus(), 30);
}

function closeSheet() {
  const backdrop = $('#sheetBackdrop');
  if (backdrop.hidden) return;
  backdrop.hidden = true;
  $('#sheet').innerHTML = '';
  const cb = sheetOnClose;
  sheetOnClose = null;
  if (cb) cb();
  if (sheetReturnFocus && document.contains(sheetReturnFocus)) sheetReturnFocus.focus({ preventScroll: true });
}

function bindSheetForm(sheet, onSubmit, context) {
  const form = $('form', sheet);
  form.addEventListener('input', e => e.target.classList.remove('invalid'));
  form.addEventListener('submit', async e => {
    e.preventDefault();
    clearFormError(form);
    const btn = $('[type=submit]', form);
    setBusy(btn, true);
    try {
      await onSubmit(form);
      closeSheet();
    } catch (err) {
      reportFormError(form, err, context);
      setBusy(btn, false);
    }
  });
}

function confirmSheet({ title, message, confirmLabel = 'Delete', danger = true }) {
  return new Promise(resolve => {
    let done = false;
    const finish = v => { if (!done) { done = true; resolve(v); } };
    openSheet(`<h3>${esc(title)}</h3><p class="sheet-sub">${esc(message)}</p>
      <div class="sheet-actions"><button class="btn" data-close>Cancel</button>
      <button class="btn ${danger ? 'btn-danger' : 'btn-primary'}" id="confirmOk">${esc(confirmLabel)}</button></div>`,
    sheet => { $('#confirmOk', sheet).addEventListener('click', () => { finish(true); closeSheet(); }); },
    () => finish(false));
  });
}

function playlistFormHtml({ title, sub, name = '', description = '', submit, nameOnly = false }) {
  return `<h3>${esc(title)}</h3>${sub ? `<p class="sheet-sub">${esc(sub)}</p>` : '<div style="height:14px"></div>'}
    <form class="form" novalidate>
      <div class="form-error" hidden></div>
      <label class="field"><span>Name</span><input class="input" name="name" value="${esc(name)}" maxlength="80" placeholder="Late Night" autocomplete="off"></label>
      ${nameOnly ? '' : `<label class="field"><span>Description</span><textarea class="input" name="description" maxlength="240" placeholder="Optional">${esc(description)}</textarea></label>`}
      <div class="sheet-actions"><button type="button" class="btn" data-close>Cancel</button><button class="btn btn-primary" type="submit">${esc(submit)}</button></div>
    </form>`;
}

const formValue = (form, name) => String(new FormData(form).get(name) ?? '').trim();

/* =========================================================================
   Media actions
   ========================================================================= */

function playMediaById(id) {
  const i = state.lib.items.findIndex(m => m.id === id);
  if (i >= 0) setQueue(state.lib.items, i, 'lib');
  else { const m = mediaById.get(id); if (m) setQueue([m], 0, 'single'); }
}

async function toggleFavorite(id) {
  try {
    const m = intern(await api('POST', `/api/media/${enc(id)}/favorite`));
    const heart = $('#nowFav');
    if (current() && current().id === id) {
      heart.classList.add('pop');
      setTimeout(() => heart.classList.remove('pop'), 250);
    }
    toast(m.favorite ? `Added “${m.title}” to favorites` : `Removed “${m.title}” from favorites`, 'success');
    renderNow();
    if (state.lib.type === 'favorites') loadLibrary(); else refreshLists();
    loadStats();
  } catch (err) {
    showError(err, 'Could not update favorite');
  }
}

async function deleteMedia(id) {
  const item = mediaById.get(id);
  if (!item) return;
  const ok = await confirmSheet({
    title: 'Delete from library?',
    message: `“${item.title}” by ${item.creator} will be removed from your library and from every playlist.`,
  });
  if (!ok) return;
  try {
    await api('DELETE', `/api/media/${enc(id)}`);
    removeFromQueue(id);
    mediaById.delete(id);
    state.lib.items = state.lib.items.filter(m => m.id !== id);
    toast(`Deleted “${item.title}”`, 'success');
    logActivity({ kind: 'deleted', title: `Deleted “${item.title}”`, message: `${item.typeLabel || ''} · ${item.creator}` });
    refreshLists();
    refreshAfterMediaChange();
  } catch (err) {
    showError(err, `Could not delete “${item.title}”`);
  }
}

function editMediaSheet(item) {
  const t = safeType(item.type);
  openSheet(`<h3>Edit details</h3>
    <p class="sheet-sub"><span class="type-chip ${t}">${esc(item.typeLabel || TYPE_META[t].label)}</span></p>
    <form class="form" novalidate>
      <div class="form-error" hidden></div>
      ${mediaFieldsHtml(t, mediaValues(item))}
      <div class="sheet-actions"><button type="button" class="btn" data-close>Cancel</button><button class="btn btn-primary" type="submit">Save</button></div>
    </form>`, sheet => bindSheetForm(sheet, async form => {
    const body = collectMedia(form, t);
    const m = intern(await api('PUT', `/api/media/${enc(item.id)}`, body));
    toast(`Saved “${m.title}”`, 'success');
    renderPlayer();
    refreshAfterMediaChange();
  }, 'Could not save changes'));
}

function resetFilters() {
  Object.assign(state.lib, { q: '', type: 'all', sort: 'recent' });
  if (state.view === 'library') mountView();
  loadLibrary();
}

/* =========================================================================
   Playlist actions
   ========================================================================= */

async function createPlaylist(name, description) {
  const pl = await api('POST', '/api/playlists', { name, description });
  logActivity({ kind: 'created', title: `Created playlist “${pl.name}”`, message: description || '' });
  await loadPlaylists();
  return pl;
}

function newPlaylistSheet() {
  openSheet(playlistFormHtml({ title: 'New playlist', sub: 'Give it a name — you can add tracks from the library afterwards.', submit: 'Create' }),
    sheet => bindSheetForm(sheet, async form => {
      const pl = await createPlaylist(formValue(form, 'name'), formValue(form, 'description'));
      toast(`Created “${pl.name}”`, 'success');
    }, 'Could not create playlist'));
}

async function addToPlaylistSheet(mediaId) {
  const item = mediaById.get(mediaId);
  if (!item) return;
  if (!state.plLoaded) await loadPlaylists();
  const rows = state.playlists.map(pl => `<button class="sheet-row" data-pl="${esc(pl.id)}">${mosaicForIds(pl.trackIds)}
      <span class="row-text"><span class="row-title"><span class="t">${esc(pl.name)}</span></span><span class="row-sub">${esc(plMeta(pl))}</span></span></button>`).join('');
  openSheet(`<h3>Add to playlist</h3><p class="sheet-sub">${esc(item.title)} · ${esc(item.creator)}</p>
    <div class="sheet-list">
      <button class="sheet-row" data-new><span class="new-ic">${ICONS.plus}</span><span class="row-text"><span class="row-title">New playlist…</span></span></button>
      ${rows}
    </div>
    <div class="sheet-actions"><button class="btn" data-close>Cancel</button></div>`, sheet => {
    sheet.addEventListener('click', async e => {
      const row = e.target.closest('[data-pl]');
      if (row) {
        row.disabled = true;
        const ok = await addTrack(row.dataset.pl, mediaId);
        if (ok) closeSheet(); else row.disabled = false;
      } else if (e.target.closest('[data-new]')) {
        openSheet(playlistFormHtml({ title: 'New playlist', sub: `“${item.title}” will be added to it.`, submit: 'Create & add' }),
          s => bindSheetForm(s, async form => {
            const pl = await createPlaylist(formValue(form, 'name'), formValue(form, 'description'));
            await addTrack(pl.id, mediaId);
          }, 'Could not create playlist'));
      }
    });
  });
}

async function addTrack(plId, mediaId) {
  const item = mediaById.get(mediaId);
  try {
    const d = await api('POST', `/api/playlists/${enc(plId)}/tracks`, { mediaId });
    setDetail(d);
    toast(`Added “${item ? item.title : 'item'}” to ${d.name}`, 'success');
    loadPlaylists();
    return true;
  } catch (err) {
    showError(err, 'Could not add to playlist');
    return false;
  }
}

function openPlaylist(id) {
  state.detailId = id;
  state.detailError = null;
  if (!state.detail || state.detail.id !== id) state.detail = null;
  showView('playlist');
  loadDetail(id);
}

async function playPlaylist(id, shuffle) {
  try {
    let d = state.detail && state.detail.id === id ? state.detail : null;
    if (!d) { d = await api('GET', `/api/playlists/${enc(id)}`); d.tracks = (d.tracks || []).map(intern); }
    if (!d.tracks.length) throw makeError('EmptyPlaylistException', `“${d.name}” has no tracks yet`, 422);
    setQueue(d.tracks, shuffle ? -1 : 0, `pl:${id}`, shuffle);
  } catch (err) {
    showError(err, 'Could not play playlist');
  }
}

async function moveTrack(from, to) {
  const d = state.detail;
  if (!d || from === to || to < 0 || to >= d.tracks.length) return;
  const [t] = d.tracks.splice(from, 1);
  d.tracks.splice(to, 0, t);
  VIEWS.playlist.update();
  try {
    setDetail(await api('POST', `/api/playlists/${enc(d.id)}/move`, { from, to }));
  } catch (err) {
    showError(err, 'Could not reorder playlist');
    loadDetail(d.id);
  }
}

async function removeTrack(index) {
  const d = state.detail;
  if (!d) return;
  const t = d.tracks[index];
  try {
    setDetail(await api('DELETE', `/api/playlists/${enc(d.id)}/tracks/${index}`));
    toast(`Removed “${t ? t.title : 'track'}” from ${d.name}`, 'success');
    loadPlaylists();
  } catch (err) {
    showError(err, 'Could not remove track');
  }
}

function renamePlaylistSheet() {
  const d = state.detail;
  if (!d) return;
  openSheet(playlistFormHtml({ title: 'Rename playlist', name: d.name, description: d.description || '', submit: 'Save' }),
    sheet => bindSheetForm(sheet, async form => {
      setDetail(await api('PUT', `/api/playlists/${enc(d.id)}`, { name: formValue(form, 'name'), description: formValue(form, 'description') }));
      toast('Playlist updated', 'success');
      loadPlaylists();
    }, 'Could not rename playlist'));
}

function duplicatePlaylistSheet() {
  const d = state.detail;
  if (!d) return;
  openSheet(playlistFormHtml({ title: 'Duplicate playlist', sub: `Makes a copy of “${d.name}” with the same tracks.`, name: `${d.name} (copy)`, submit: 'Duplicate', nameOnly: true }),
    sheet => bindSheetForm(sheet, async form => {
      const name = formValue(form, 'name');
      const copy = await api('POST', `/api/playlists/${enc(d.id)}/duplicate`, name ? { name } : {});
      toast(`Created “${copy.name}”`, 'success');
      logActivity({ kind: 'created', title: `Duplicated “${d.name}”`, message: `as “${copy.name}”` });
      state.detailId = copy.id;
      setDetail(copy);
      mountView();
      loadPlaylists();
    }, 'Could not duplicate playlist'));
}

async function shufflePlaylistOrder() {
  const d = state.detail;
  if (!d) return;
  try {
    setDetail(await api('POST', `/api/playlists/${enc(d.id)}/shuffle`));
    toast(`Shuffled ${d.name}`, 'success');
  } catch (err) {
    showError(err, 'Could not shuffle playlist');
  }
}

async function deletePlaylist() {
  const d = state.detail;
  if (!d) return;
  const ok = await confirmSheet({ title: 'Delete playlist?', message: `“${d.name}” will be deleted. The tracks stay in your library.` });
  if (!ok) return;
  try {
    await api('DELETE', `/api/playlists/${enc(d.id)}`);
    toast(`Deleted “${d.name}”`, 'success');
    logActivity({ kind: 'deleted', title: `Deleted playlist “${d.name}”`, message: plMeta(d) });
    state.detail = null;
    state.detailId = null;
    state.playlists = state.playlists.filter(p => p.id !== d.id);
    showView('playlists');
    loadPlaylists();
    loadStats();
  } catch (err) {
    showError(err, 'Could not delete playlist');
  }
}

async function exportPlaylist(pl) {
  if (!pl) return;
  try {
    let res;
    try { res = await fetch(`/api/playlists/${enc(pl.id)}/export.m3u`); } catch { throw makeError('NetworkError', 'Cannot reach the Playlist Builder server', 0); }
    if (!res.ok) throw await errorFromResponse(res);
    const blob = await res.blob();
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `${(pl.name || 'playlist').replace(/[\\/:*?"<>|]+/g, '_').trim() || 'playlist'}.m3u`;
    a.hidden = true;
    document.body.appendChild(a);
    a.click();
    a.remove();
    setTimeout(() => URL.revokeObjectURL(url), 4000);
    toast(`Exported ${a.download}`, 'success');
    logActivity({ kind: 'exported', title: `Exported “${pl.name}”`, message: `${a.download} · ${plural(pl.count || 0, 'item')}` });
  } catch (err) {
    showError(err, `Could not export “${pl.name}”`);
  }
}

function clearActivity() {
  state.activity = [];
  state.unread = 0;
  VIEWS.activity.update();
}

/* =========================================================================
   Playlist drag & drop reorder
   ========================================================================= */

function initTrackDnD(root) {
  let from = -1;
  const clear = () => $$('.drop-before, .drop-after', root).forEach(r => r.classList.remove('drop-before', 'drop-after'));
  const targetOf = e => {
    const row = e.target.closest && e.target.closest('.row.track');
    if (!row) return null;
    const rect = row.getBoundingClientRect();
    return { row, index: Number(row.dataset.index), after: e.clientY > rect.top + rect.height / 2 };
  };

  root.addEventListener('dragstart', e => {
    const row = e.target.closest && e.target.closest('.row.track');
    if (!row) return;
    from = Number(row.dataset.index);
    row.classList.add('dragging');
    e.dataTransfer.effectAllowed = 'move';
    e.dataTransfer.setData('text/plain', String(from));
  });
  root.addEventListener('dragover', e => {
    if (from < 0) return;
    const t = targetOf(e);
    if (!t) return;
    e.preventDefault();
    e.dataTransfer.dropEffect = 'move';
    clear();
    t.row.classList.add(t.after ? 'drop-after' : 'drop-before');
  });
  root.addEventListener('drop', e => {
    if (from < 0) return;
    const t = targetOf(e);
    e.preventDefault();
    clear();
    if (!t) return;
    const insertAt = t.index + (t.after ? 1 : 0);
    const to = insertAt > from ? insertAt - 1 : insertAt;
    const f = from;
    from = -1;
    moveTrack(f, to);
  });
  root.addEventListener('dragend', () => {
    from = -1;
    clear();
    $$('.dragging', root).forEach(r => r.classList.remove('dragging'));
  });
}

/* =========================================================================
   Click actions (event delegation)
   ========================================================================= */

const ACTIONS = {
  nav(el) {
    const key = el.dataset.key;
    if (key === 'now') { openNow(); return; }
    state.mobileNow = false;
    showView(key);
  },
  'open-now': () => openNow(),
  'now-collapse'() {
    if (isMobile()) { state.mobileNow = false; applyMobile(); }
  },
  'main-back'() {
    if (state.view === 'playlist') showView('playlists');
    else { const v = $('.view'); if (v) v.scrollTo({ top: 0, behavior: 'smooth' }); }
  },
  'open-menu'(el) {
    if (el.dataset.menu === 'now') {
      const item = current();
      openMenu(el, item ? mediaMenuItems(item) : [{ label: 'Nothing is playing', icon: 'info', disabled: true }]);
    } else {
      openMenu(el, viewMenuItems());
    }
  },
  'row-menu'(el) {
    openMenu(el, mediaMenuItems(mediaById.get(el.dataset.id), { includePlay: true }));
  },
  'play-row'(el) {
    const i = Number(el.dataset.index);
    if (el.dataset.ctx === 'pl') {
      const d = state.detail;
      if (d) setQueue(d.tracks, i, `pl:${d.id}`);
    } else {
      setQueue(state.lib.items, i, 'lib');
    }
  },
  toggle: () => togglePlay(),
  next: () => next(),
  prev: () => prev(),
  shuffle: () => toggleShuffle(),
  repeat: () => cycleRepeat(),
  'now-fav'() { const c = current(); if (c) toggleFavorite(c.id); },
  'add-to-pl'(el) { addToPlaylistSheet(el.dataset.id); },
  filter(el) { state.lib.type = el.dataset.type; loadLibrary(); },
  'clear-search'() {
    const input = $('#libSearch');
    if (input) { input.value = ''; input.focus(); }
    $('#libClear').hidden = true;
    state.lib.q = '';
    loadLibrary();
  },
  'reset-filters': () => resetFilters(),
  'reload-library'() { loadLibrary(); loadStats(); },
  'reload-playlists': () => loadPlaylists(),
  'new-playlist': () => newPlaylistSheet(),
  'open-pl'(el) { openPlaylist(el.dataset.id); },
  'play-pl'(el) { playPlaylist(el.dataset.id, false); },
  'pl-play'() { if (state.detail) playPlaylist(state.detail.id, false); },
  'pl-shuffle-play'() { if (state.detail) playPlaylist(state.detail.id, true); },
  'pl-export'() { exportPlaylist(state.detail); },
  'pl-duplicate': () => duplicatePlaylistSheet(),
  'pl-rename': () => renamePlaylistSheet(),
  'pl-delete': () => deletePlaylist(),
  'move-track'(el) { moveTrack(Number(el.dataset.from), Number(el.dataset.to)); },
  'remove-track'(el) { removeTrack(Number(el.dataset.index)); },
  'add-type'(el) {
    const type = el.dataset.type;
    if (type === state.addType) return;
    const form = $('#addForm');
    const values = Object.fromEntries(new FormData(form));
    state.addType = type;
    $$('.seg-btn').forEach(b => b.setAttribute('aria-checked', String(b.dataset.type === type)));
    $('.seg-thumb').style.transform = `translateX(${TYPES.indexOf(type) * 100}%)`;
    $('#addFields').innerHTML = mediaFieldsHtml(type, values);
    clearFormError(form);
  },
  'clear-activity': () => clearActivity(),
};

/* =========================================================================
   Init
   ========================================================================= */

function isTyping(el) {
  return !!el && (el.isContentEditable || /^(INPUT|TEXTAREA|SELECT)$/.test(el.tagName));
}

function init() {
  $$('[data-icon]').forEach(el => { el.outerHTML = ICONS[el.dataset.icon]; });
  buildNav();
  initMenu();
  initRing();
  initMediaSession();

  document.addEventListener('click', e => {
    const el = e.target.closest('[data-action]');
    if (!el || el.disabled || el.closest('#menu')) return;
    const fn = ACTIONS[el.dataset.action];
    if (fn) fn(el, e);
  });

  $('#sheetBackdrop').addEventListener('pointerdown', e => {
    if (e.target === e.currentTarget) closeSheet();
  });

  document.addEventListener('keydown', e => {
    if (e.key === 'Escape') {
      if (!$('#menu').hidden) { closeMenu(); return; }
      if (!$('#sheetBackdrop').hidden) { closeSheet(); return; }
      if (isMobile() && state.mobileNow) { state.mobileNow = false; applyMobile(); }
      return;
    }
    if ((e.code === 'Space' || e.key === ' ') && !isTyping(e.target) && $('#sheetBackdrop').hidden && $('#menu').hidden) {
      e.preventDefault();
      togglePlay();
    }
  });

  window.matchMedia('(max-width: 959px)').addEventListener('change', applyMobile);

  mountView();
  renderPlayer();
  applyMobile();
  loadLibrary();
  loadStats();
  loadPlaylists();
}

init();
