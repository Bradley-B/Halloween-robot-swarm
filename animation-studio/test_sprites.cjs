const assert = require('node:assert/strict');
const { rgb332, channels, sheetInfo, splitSheet, durationForFPS, frameIndex } = require('./sprites.js');

assert.equal(rgb332(255, 255, 255), 255);
assert.equal(rgb332(255, 0, 0), 224);
assert.equal(rgb332(0, 255, 0), 28);
assert.equal(rgb332(0, 0, 255), 3);
assert.deepEqual(channels(255), [255, 255, 255]);
assert.deepEqual(sheetInfo(26, 18, 'eyes'), { frameWidth: 13, columns: 2, rows: 2, count: 4 });
assert.deepEqual(sheetInfo(26, 18, 'mouth'), { frameWidth: 26, columns: 1, rows: 2, count: 2 });
assert.throws(() => sheetInfo(14, 9, 'eyes'));
assert.throws(() => sheetInfo(26, 10, 'mouth'));

// A four-cell grid makes horizontal and vertical traversal visibly different.
const rgba = new Uint8ClampedArray(26 * 18 * 4);
const cellColors = [[255, 0, 0, 255], [0, 255, 0, 255], [0, 0, 255, 255], [255, 255, 255, 255]];
for (let y = 0; y < 18; y++) {
  for (let x = 0; x < 26; x++) rgba.set(cellColors[Math.floor(y / 9) * 2 + Math.floor(x / 13)], (y * 26 + x) * 4);
}
assert.deepEqual(splitSheet(rgba, 26, 18, 'eyes').map(frame => frame[0]), [224, 28, 3, 255]);
assert.deepEqual(splitSheet(rgba, 26, 18, 'eyes', 'columns').map(frame => frame[0]), [224, 3, 28, 255]);
assert.equal(splitSheet(rgba, 26, 18, 'eyes', 'rows', 3).length, 3);
assert.throws(() => splitSheet(rgba, 26, 18, 'eyes', 'rows', 5));
assert.throws(() => splitSheet(new Uint8Array(1), 26, 18, 'eyes'));
const mouth = splitSheet(rgba, 26, 18, 'mouth');
assert.equal(mouth[0].length, 234);
assert.equal(mouth[0][12], 224);
assert.equal(mouth[0][13], 28);
assert.equal(mouth[1][12], 3);
assert.equal(mouth[1][13], 255);
rgba.fill(255); // Hidden RGB must not light transparent pixels.
for (let i = 3; i < rgba.length; i += 4) rgba[i] = 0;
assert.ok(splitSheet(rgba, 26, 18, 'eyes').every(frame => frame.every(pixel => pixel === 0)));
for (let i = 3; i < rgba.length; i += 4) rgba[i] = 128;
assert.equal(splitSheet(rgba, 26, 18, 'eyes')[0][0], rgb332(128, 128, 128));
assert.equal(durationForFPS(8), 125);
assert.equal(durationForFPS(60), 17);
assert.throws(() => durationForFPS(NaN));
assert.throws(() => durationForFPS(0));
assert.equal(frameIndex([2400, 100, 180, 100], 2399), 0);
assert.equal(frameIndex([2400, 100, 180, 100], 2400), 1);
assert.equal(frameIndex([2400, 100, 180, 100], 2780), 0);
assert.equal(frameIndex([2400, 100, 180, 100], 250, 8), 2);
console.log('PNG slicing, mouth seam, transparency, RGB332, and timing checks passed.');
