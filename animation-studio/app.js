'use strict';

const $ = selector => document.querySelector(selector);
const { sheetInfo, splitSheet, durationForFPS, frameIndex, channels } = SpriteTools;
const form = $('#import-form');
const library = new Map();
let draft = null;
let source = null;
let sheetURL = null;
let imageRequest = 0;
let saving = false;
let playing = true;
let elapsed = 0;
let previousTime = performance.now();
let cards = [];
let lastHeadKey = '';
const colors = Array.from({ length: 256 }, (_, pixel) => `rgb(${channels(pixel).join(',')})`);

function status(element, message, state = '') {
  element.textContent = message;
  element.dataset.state = state;
}
function part() { return form.elements.part.value; }
function animations() {
  return [...library.entries(), ...(draft ? [['@draft', draft]] : [])];
}
function animationFor(id) { return id === '@draft' ? draft : library.get(id); }
function resetTime() { elapsed = 0; lastHeadKey = ''; cards.forEach(card => { card.lastKey = ''; }); }

function refreshOptions(select, target) {
  const selected = select.value;
  select.replaceChildren(new Option('Off', ''));
  for (const [id, animation] of animations()) {
    if (animation.part === target) select.add(new Option(`${animation.name}${id === '@draft' ? ' · unsaved' : ''}`, id));
  }
  select.value = [...select.options].some(option => option.value === selected) ? selected : '';
}
function refreshLibrary() {
  refreshOptions($('#preview-eyes'), 'eyes');
  refreshOptions($('#preview-mouth'), 'mouth');
  const gallery = $('#gallery');
  gallery.replaceChildren();
  cards = [];
  for (const [id, animation] of animations()) {
    const card = document.createElement('button');
    card.type = 'button';
    card.className = `animation-card${id === '@draft' ? ' draft' : ''}`;
    card.setAttribute('aria-label', `Preview ${animation.name} on ${animation.part}`);
    const canvas = document.createElement('canvas');
    canvas.width = 620;
    canvas.height = 420;
    canvas.setAttribute('aria-hidden', 'true');
    const info = document.createElement('div');
    info.className = 'card-info';
    const title = document.createElement('div');
    title.className = 'card-title';
    const name = document.createElement('strong');
    name.textContent = animation.name;
    const tag = document.createElement('span');
    tag.className = 'tag';
    tag.textContent = id === '@draft' ? 'UNSAVED' : animation.builtin ? 'BUILT-IN' : 'CUSTOM';
    title.append(name, tag);
    const meta = document.createElement('span');
    meta.className = 'card-meta';
    const uniform = animation.durations.every(ms => ms === animation.durations[0]);
    const rate = uniform ? `${(1000 / animation.durations[0]).toFixed(1).replace(/\.0$/, '')} FPS` : 'Variable timing';
    meta.textContent = `${animation.part === 'eyes' ? 'Eyes · 13×9' : 'Mouth · 26×9'} · ${animation.frames.length} frame${animation.frames.length === 1 ? '' : 's'} · ${rate}`;
    const frameLabel = document.createElement('span');
    frameLabel.className = 'card-frame';
    info.append(title, meta, frameLabel);
    card.append(canvas, info);
    card.addEventListener('click', () => {
      $(`#preview-${animation.part}`).value = id;
      resetTime();
    });
    gallery.append(card);
    cards.push({ id, canvas, frameLabel, lastKey: '' });
  }
  $('#library-count').textContent = `${library.size} saved${draft ? ' + 1 draft' : ''}`;
  lastHeadKey = '';
}

function rebuildDraft(selectDraft = false) {
  draft = null;
  $('#upload').disabled = true;
  try {
    const fps = $('#fps').valueAsNumber;
    const duration = durationForFPS(fps);
    $('#timing-hint').textContent = `${duration} ms per frame`;
    if (!source) {
      refreshLibrary();
      return;
    }
    const info = sheetInfo(source.width, source.height, part());
    const count = $('#frame-count').value === '' ? null : $('#frame-count').valueAsNumber;
    const frames = splitSheet(source.rgba, source.width, source.height, part(), $('#frame-order').value, count);
    const name = $('#animation-name').value.trim();
    draft = { name: name || 'untitled', part: part(), frames, durations: frames.map(() => duration) };
    $('#sheet-info').textContent = `${source.width}×${source.height} PNG · ${info.columns} columns × ${info.rows} rows · ${frames.length} frames`;
    status($('#import-status'), `Ready to preview · ${frames.length} frame${frames.length === 1 ? '' : 's'}.`);
    const validName = /^[a-z][a-z0-9_-]{0,31}$/.test(name);
    const duplicate = library.has(name) || name === 'off';
    $('#upload').disabled = saving || !validName || duplicate;
    if (duplicate) status($('#import-status'), 'That name already exists. Choose a new name.', 'error');
    else if (!validName) status($('#import-status'), 'Preview ready. Enter a valid name to upload.');
    refreshLibrary();
    if (selectDraft) $(`#preview-${part()}`).value = '@draft';
    resetTime();
  } catch (error) {
    status($('#import-status'), error.message, 'error');
    refreshLibrary();
  }
}

async function importFile(file) {
  if (!file || saving) return;
  const request = ++imageRequest;
  source = null;
  draft = null;
  refreshLibrary();
  $('#upload').disabled = true;
  $('#sheet-details').hidden = true;
  status($('#import-status'), 'Reading PNG…');
  let url;
  try {
    if (file.size > 8 * 1024 * 1024) throw new Error('Choose a PNG smaller than 8 MiB. Export from Piskel at 1× scale.');
    const signature = new Uint8Array(await file.slice(0, 8).arrayBuffer());
    if ([137, 80, 78, 71, 13, 10, 26, 10].some((byte, i) => signature[i] !== byte)) {
      throw new Error('Choose a PNG spritesheet exported from Piskel.');
    }
    url = URL.createObjectURL(file);
    const image = new Image();
    image.src = url;
    await image.decode();
    if (request !== imageRequest) return;
    if (image.naturalWidth * image.naturalHeight > 2_000_000) throw new Error('The PNG is too large. Export pixel art at 1× scale.');
    const canvas = document.createElement('canvas');
    canvas.width = image.naturalWidth;
    canvas.height = image.naturalHeight;
    const context = canvas.getContext('2d', { willReadFrequently: true });
    context.drawImage(image, 0, 0);
    source = { width: canvas.width, height: canvas.height, rgba: context.getImageData(0, 0, canvas.width, canvas.height).data };
    if (sheetURL) URL.revokeObjectURL(sheetURL);
    sheetURL = url;
    url = null;
    $('#sheet-image').src = sheetURL;
    $('#sheet-details').hidden = false;
    $('#file-label').textContent = file.name;
    $('#frame-count').value = '';
    if (!$('#animation-name').value) {
      const name = file.name.replace(/\.png$/i, '').toLowerCase().replace(/[^a-z0-9_-]+/g, '_').replace(/^[^a-z]+/, '');
      $('#animation-name').value = (name || 'animation').slice(0, 32);
    }
    rebuildDraft(true);
  } catch (error) {
    if (request !== imageRequest) return;
    status($('#import-status'), error.message || 'Could not decode that PNG.', 'error');
  } finally {
    if (url) URL.revokeObjectURL(url);
  }
}

$('#sheet-file').addEventListener('change', event => importFile(event.target.files[0]));
const drop = $('#drop-zone');
for (const type of ['dragover', 'dragenter']) drop.addEventListener(type, event => {
  event.preventDefault(); drop.classList.add('dragging');
});
for (const type of ['dragleave', 'drop']) drop.addEventListener(type, event => {
  event.preventDefault(); drop.classList.remove('dragging');
});
drop.addEventListener('drop', event => {
  if (event.dataTransfer.files.length !== 1) {
    status($('#import-status'), 'Choose one spritesheet at a time.', 'error');
    return;
  }
  importFile(event.dataTransfer.files[0]);
});
for (const input of form.querySelectorAll('input:not([type=file]), select')) {
  input.addEventListener('input', () => rebuildDraft(true));
}

form.addEventListener('submit', async event => {
  event.preventDefault();
  if (!draft || saving || $('#upload').disabled || !form.reportValidity()) return;
  saving = true;
  const snapshot = { ...draft, name: $('#animation-name').value.trim() };
  for (const control of form.querySelectorAll('input, select, button')) control.disabled = true;
  status($('#import-status'), 'Adding animation to the project…');
  try {
    const response = await fetch('/api/animations', {
      method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(snapshot)
    });
    const result = await response.json();
    if (!response.ok) throw new Error(result.error || 'Could not save the animation.');
    const animation = result.animation;
    library.set(animation.name, animation);
    draft = null;
    source = null;
    $('#sheet-file').value = '';
    $('#animation-name').value = '';
    $('#frame-count').value = '';
    $('#sheet-details').hidden = true;
    $('#file-label').textContent = 'Choose or drop a PNG';
    if (sheetURL) URL.revokeObjectURL(sheetURL);
    sheetURL = null;
    refreshLibrary();
    $(`#preview-${animation.part}`).value = animation.name;
    resetTime();
    status($('#import-status'), `“${animation.name}” added. ${result.message}`, 'success');
  } catch (error) {
    status($('#import-status'), `${error.message} Your preview is still available.`, 'error');
  } finally {
    saving = false;
    for (const control of form.querySelectorAll('input, select, button')) control.disabled = false;
    $('#upload').disabled = !draft;
  }
});

function roundedBox(context, x, y, width, height, radius, fill, stroke = null) {
  context.beginPath();
  context.roundRect(x, y, width, height, radius);
  context.fillStyle = fill;
  context.fill();
  if (stroke) { context.strokeStyle = stroke; context.lineWidth = 2; context.stroke(); }
}
function drawPanel(context, pixels, x, y, width, sourceWidth = width, offset = 0) {
  const pitch = 14;
  roundedBox(context, x - 7, y - 7, width * pitch + 10, 9 * pitch + 10, 8, '#080b10', '#303340');
  for (let row = 0; row < 9; row++) {
    for (let col = 0; col < width; col++) {
      const pixel = pixels ? pixels[row * sourceWidth + col + offset] : 0;
      context.fillStyle = pixel ? colors[pixel] : '#171c26';
      context.beginPath();
      context.arc(x + col * pitch + 5, y + row * pitch + 5, 4.6, 0, Math.PI * 2);
      context.fill();
    }
  }
}
function drawHead(canvas, eyePixels, mouthPixels) {
  const context = canvas.getContext('2d');
  context.clearRect(0, 0, canvas.width, canvas.height);
  // One face, four actual panels. Mouth samples the same continuous 26-pixel row.
  roundedBox(context, 301, 22, 18, 38, 5, '#474052');
  roundedBox(context, 300, 14, 20, 12, 6, '#ffa36c');
  roundedBox(context, 48, 137, 32, 100, 9, '#282a35', '#414350');
  roundedBox(context, 540, 137, 32, 100, 9, '#282a35', '#414350');
  roundedBox(context, 71, 54, 478, 320, 36, '#242630', '#454352');
  roundedBox(context, 82, 65, 456, 297, 27, '#1a1d26', '#303440');
  drawPanel(context, eyePixels, 100, 88, 13);
  drawPanel(context, eyePixels, 338, 88, 13);
  drawPanel(context, mouthPixels, 126, 224, 13, 26, 0);
  drawPanel(context, mouthPixels, 310, 224, 13, 26, 13);
  for (const [x, y] of [[92, 76], [528, 76], [92, 351], [528, 351]]) {
    context.beginPath(); context.arc(x, y, 3, 0, Math.PI * 2); context.fillStyle = '#686372'; context.fill();
  }
}
function sample(id, override) {
  const animation = animationFor(id);
  if (!animation) return { pixels: null, key: 'off', index: 0 };
  const index = frameIndex(animation.durations, elapsed, override);
  return { pixels: animation.frames[index], key: `${id}:${index}`, index };
}
function render(now) {
  // A suspended browser tab resumes on the same timeline rather than slowing down.
  if (playing) elapsed += now - previousTime;
  previousTime = now;
  const override = $('#preview-fps').value ? Number($('#preview-fps').value) : null;
  const eyeID = $('#preview-eyes').value;
  const mouthID = $('#preview-mouth').value;
  const eyes = sample(eyeID, override);
  const mouth = sample(mouthID, override);
  const headKey = `${eyes.key}|${mouth.key}`;
  if (headKey !== lastHeadKey) {
    drawHead($('#head-preview'), eyes.pixels, mouth.pixels);
    lastHeadKey = headKey;
  }
  for (const card of cards) {
    const animation = animationFor(card.id);
    const own = sample(card.id, override);
    const cardEyes = animation.part === 'eyes' ? own : eyes;
    const cardMouth = animation.part === 'mouth' ? own : mouth;
    const key = `${cardEyes.key}|${cardMouth.key}`;
    if (key !== card.lastKey) {
      drawHead(card.canvas, cardEyes.pixels, cardMouth.pixels);
      card.frameLabel.textContent = `Frame ${own.index + 1} / ${animation.frames.length}`;
      card.lastKey = key;
    }
  }
  requestAnimationFrame(render);
}
$('#pause').addEventListener('click', () => {
  playing = !playing;
  $('#pause').textContent = playing ? 'Pause' : 'Play';
  $('#pause').setAttribute('aria-pressed', String(!playing));
});
$('#restart').addEventListener('click', resetTime);
for (const selector of ['#preview-fps', '#preview-eyes', '#preview-mouth']) $(selector).addEventListener('change', resetTime);

async function loadLibrary() {
  try {
    const response = await fetch('/api/animations');
    const result = await response.json();
    if (!response.ok) throw new Error(result.error || 'Could not load animations.');
    for (const animation of result.animations) library.set(animation.name, animation);
    refreshLibrary();
    $('#preview-eyes').value = 'blink';
    $('#preview-mouth').value = 'talk';
    status($('#library-status'), '');
  } catch (error) {
    status($('#library-status'), `${error.message} Restart the launcher and refresh this page.`, 'error');
    $('#library-count').textContent = 'Library unavailable';
    // Avoid saving until duplicate names and the on-disk library are known.
    for (const control of form.querySelectorAll('input, select, button')) control.disabled = true;
  }
}
for (const control of form.querySelectorAll('input, select, button')) control.disabled = true;
loadLibrary().then(() => {
  if (library.size) {
    for (const control of form.querySelectorAll('input, select, button')) control.disabled = false;
    $('#upload').disabled = true;
  }
});
requestAnimationFrame(render);
