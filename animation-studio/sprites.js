/* Pure conversion and timing helpers, shared by the app and the Node checks. */
(function (root) {
  'use strict';
  function rgb332(red, green, blue) {
    return (red & 0xe0) | ((green & 0xe0) >> 3) | (blue >> 6);
  }
  function channels(pixel) {
    return [Math.floor((pixel >> 5) * 255 / 7),
      Math.floor(((pixel >> 2) & 7) * 255 / 7), (pixel & 3) * 85];
  }
  function sheetInfo(width, height, part) {
    const frameWidth = part === 'eyes' ? 13 : 26;
    if (!['eyes', 'mouth'].includes(part) || !Number.isInteger(width) || !Number.isInteger(height) ||
        width < frameWidth || height < 9 || width % frameWidth || height % 9) {
      throw new Error(`Use an unscaled PNG with ${frameWidth} × 9 frames and no padding or spacing.`);
    }
    return { frameWidth, columns: width / frameWidth, rows: height / 9, count: (width / frameWidth) * (height / 9) };
  }
  function splitSheet(rgba, width, height, part, order = 'rows', frameCount = null) {
    const info = sheetInfo(width, height, part);
    const count = frameCount === null ? info.count : frameCount;
    if (!Number.isInteger(count) || count < 1 || count > 255 || count > info.count) {
      throw new Error(`Choose 1–${Math.min(255, info.count)} frames. Firmware supports at most 255 per animation.`);
    }
    if (rgba.length !== width * height * 4) throw new Error('The PNG pixel data is incomplete.');
    if (!['rows', 'columns'].includes(order)) throw new Error('Choose a valid frame order.');
    const frames = [];
    for (let index = 0; index < count; index++) {
      const column = order === 'columns' ? Math.floor(index / info.rows) : index % info.columns;
      const row = order === 'columns' ? index % info.rows : Math.floor(index / info.columns);
      const frame = [];
      for (let y = 0; y < 9; y++) {
        for (let x = 0; x < info.frameWidth; x++) {
          const offset = ((row * 9 + y) * width + column * info.frameWidth + x) * 4;
          const alpha = rgba[offset + 3] / 255;
          frame.push(rgb332(Math.round(rgba[offset] * alpha), Math.round(rgba[offset + 1] * alpha),
            Math.round(rgba[offset + 2] * alpha)));
        }
      }
      frames.push(frame);
    }
    return frames;
  }
  function durationForFPS(fps) {
    if (!Number.isFinite(fps) || fps < 1 || fps > 60) throw new Error('Choose a frame rate from 1 to 60 FPS.');
    return Math.round(1000 / fps);
  }
  function frameIndex(durations, elapsed, overrideFPS = null) {
    if (overrideFPS !== null) return Math.floor(elapsed / durationForFPS(overrideFPS)) % durations.length;
    const total = durations.reduce((sum, ms) => sum + ms, 0);
    let position = elapsed % total;
    for (let index = 0; index < durations.length; index++) {
      if (position < durations[index]) return index;
      position -= durations[index];
    }
    return 0;
  }
  const tools = { rgb332, channels, sheetInfo, splitSheet, durationForFPS, frameIndex };
  if (typeof module !== 'undefined' && module.exports) module.exports = tools;
  else root.SpriteTools = tools;
})(typeof globalThis === 'undefined' ? this : globalThis);
