/* Optional real-browser check. Uses installed Chrome and Node 22+, no npm packages.
 * Run: node animation-studio/test_browser.cjs
 * CHROME_PATH and PYTHON may override executable locations.
 * All imported animations live in a temporary library, outside the project.
 */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { spawn } = require('node:child_process');

const browserPath = process.env.CHROME_PATH || [
  'C:/Program Files/Google/Chrome/Application/chrome.exe',
  'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
  '/usr/bin/google-chrome', '/usr/bin/chromium',
  '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome'
].find(candidate => fs.existsSync(candidate));
if (!browserPath) throw new Error('Set CHROME_PATH to an installed Chrome or Edge executable.');

const temporary = fs.mkdtempSync(path.join(os.tmpdir(), 'face-lab-browser-'));
const fixture = spawn(process.env.PYTHON || 'python', ['-u', '-c', `
import sys, tempfile, threading
from pathlib import Path
sys.path.insert(0, ${JSON.stringify(__dirname)})
import studio
with tempfile.TemporaryDirectory(prefix='face-lab-library-') as directory:
    studio.LIBRARY = Path(directory) / 'animations.json'
    studio.HEADER = Path(directory) / 'custom_animations.h'
    studio.LIBRARY.write_text('[]', encoding='utf-8')
    server = studio.create_server(0)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    print('http://127.0.0.1:' + str(server.server_address[1]), flush=True)
    try:
        sys.stdin.readline()
    finally:
        server.shutdown()
        server.server_close()
`], { windowsHide: true });
let browser;
let socket;
let browserLog = '';
let nextID = 0;
const pending = new Map();
const errors = [];
const delay = ms => new Promise(resolve => setTimeout(resolve, ms));

function waitForOutput(child, stream, pattern) {
  return new Promise((resolve, reject) => {
    let output = '';
    const timer = setTimeout(() => reject(new Error(`Timed out starting process: ${output}`)), 20000);
    child.once('error', error => { clearTimeout(timer); reject(error); });
    child.once('exit', code => { clearTimeout(timer); reject(new Error(`Process exited (${code}): ${output}`)); });
    stream.on('data', chunk => {
      output += chunk;
      const match = output.match(pattern);
      if (match) { clearTimeout(timer); resolve(match[0]); }
    });
  });
}
function call(method, params = {}, sessionId) {
  return new Promise((resolve, reject) => {
    const id = ++nextID;
    const timer = setTimeout(() => { pending.delete(id); reject(new Error(`CDP timeout: ${method}`)); }, 15000);
    pending.set(id, { resolve: result => { clearTimeout(timer); resolve(result); }, reject });
    socket.send(JSON.stringify({ id, method, params, ...(sessionId ? { sessionId } : {}) }));
  });
}

(async () => {
  try {
    const url = await waitForOutput(fixture, fixture.stdout, /http:\/\/127\.0\.0\.1:\d+/);
    browser = spawn(browserPath, ['--headless', '--disable-gpu', '--no-first-run', '--no-default-browser-check',
      '--remote-debugging-port=0', `--user-data-dir=${temporary}`, 'about:blank'], { windowsHide: true });
    browser.stderr.on('data', chunk => { browserLog += chunk; });
    const debugURL = await waitForOutput(browser, browser.stderr, /ws:\/\/127\.0\.0\.1:\d+\/devtools\/browser\/[a-z0-9-]+/);
    socket = new WebSocket(debugURL);
    await new Promise((resolve, reject) => { socket.onopen = resolve; socket.onerror = reject; });
    socket.onmessage = event => {
      const message = JSON.parse(event.data);
      if (message.id && pending.has(message.id)) {
        const promise = pending.get(message.id);
        pending.delete(message.id);
        if (message.error) promise.reject(new Error(JSON.stringify(message.error)));
        else promise.resolve(message.result);
      } else if (message.method === 'Runtime.exceptionThrown') errors.push(message.params.exceptionDetails);
    };
    const { targetId } = await call('Target.createTarget', { url: 'about:blank' });
    const { sessionId } = await call('Target.attachToTarget', { targetId, flatten: true });
    const page = (method, params) => call(method, params, sessionId);
    await page('Page.enable');
    await page('Runtime.enable');
    await page('Emulation.setDeviceMetricsOverride', { width: 1280, height: 1100, deviceScaleFactor: 1, mobile: false });
    await page('Page.navigate', { url });
    async function evaluate(expression) {
      const response = await page('Runtime.evaluate', { expression, returnByValue: true, awaitPromise: true });
      if (response.exceptionDetails) throw new Error(JSON.stringify(response.exceptionDetails));
      return response.result.value;
    }
    async function until(expression) {
      for (let attempt = 0; attempt < 100; attempt++) {
        if (await evaluate(expression)) return;
        await delay(50);
      }
      throw new Error(`Browser condition failed: ${expression}`);
    }
    async function png(width, height, name) {
      await evaluate(`(async () => {
        const canvas = document.createElement('canvas'); canvas.width = ${width}; canvas.height = ${height};
        const ctx = canvas.getContext('2d');
        const palette = ['#ff0000','#00ff00','#0000ff','#ffffff'];
        for (let y=0; y<canvas.height; y++) for (let x=0; x<canvas.width; x++) {
          ctx.fillStyle = palette[(Math.floor(y/9)*2 + Math.floor(x/13)) % 4]; ctx.fillRect(x,y,1,1);
        }
        const blob = await new Promise(resolve => canvas.toBlob(resolve, 'image/png'));
        const file = new File([blob], ${JSON.stringify(name)}, { type:'image/png' });
        const transfer = new DataTransfer(); transfer.items.add(file);
        const input = document.querySelector('#sheet-file'); input.files = transfer.files;
        input.dispatchEvent(new Event('change', { bubbles:true }));
      })()`);
    }
    await until(`document.querySelectorAll('.animation-card').length === 4 && !document.querySelector('#sheet-file').disabled`);
    assert.equal(await evaluate(`document.querySelector('#preview-eyes').value`), 'blink');
    assert.equal(await evaluate(`document.querySelector('#preview-mouth').value`), 'talk');
    await png(14, 9, 'bad.png');
    await until(`document.querySelector('#import-status').dataset.state === 'error'`);
    assert.equal(await evaluate(`document.querySelector('#upload').disabled`), true);
    await evaluate(`document.querySelector('#animation-name').value = 'browser_eyes'`);
    await png(26, 18, 'eyes.png');
    await until(`document.querySelectorAll('.animation-card').length === 5 && !document.querySelector('#upload').disabled`);
    assert.deepEqual(await evaluate(`draft.frames.map(frame => frame[0])`), [224, 28, 3, 255]);
    await evaluate(`document.querySelector('#frame-order').value='columns'; document.querySelector('#frame-order').dispatchEvent(new Event('input'))`);
    assert.deepEqual(await evaluate(`draft.frames.map(frame => frame[0])`), [224, 3, 28, 255]);
    await evaluate(`document.querySelector('#frame-count').value='3'; document.querySelector('#frame-count').dispatchEvent(new Event('input'))`);
    assert.equal(await evaluate(`draft.frames.length`), 3);
    await evaluate(`document.querySelector('#import-form').requestSubmit()`);
    await until(`document.querySelector('#import-status').dataset.state === 'success'`);
    assert.equal(await evaluate(`document.querySelector('#preview-eyes').value`), 'browser_eyes');
    await page('Page.reload');
    await until(`document.querySelectorAll('.animation-card').length === 5 && !document.querySelector('#sheet-file').disabled`);
    await evaluate(`document.querySelector('input[value=mouth]').checked=true; document.querySelector('input[value=mouth]').dispatchEvent(new Event('input')); document.querySelector('#animation-name').value='browser_mouth'`);
    await png(26, 18, 'mouth.png');
    await until(`draft !== null && draft.part === 'mouth' && !document.querySelector('#upload').disabled`);
    assert.equal(await evaluate(`draft.frames[0].length`), 234);
    assert.deepEqual(await evaluate(`[draft.frames[0][12],draft.frames[0][13]]`), [224, 28]);
    await evaluate(`document.querySelector('#fps').value='12'; document.querySelector('#fps').dispatchEvent(new Event('input'))`);
    assert.deepEqual(await evaluate(`draft.durations`), [83, 83]);
    await evaluate(`document.querySelector('#import-form').requestSubmit()`);
    await until(`document.querySelector('#import-status').dataset.state === 'success'`);
    assert.equal(await evaluate(`document.querySelectorAll('.animation-card').length`), 6);
    await evaluate(`document.querySelector('#preview-fps').value='30'; document.querySelector('#preview-fps').dispatchEvent(new Event('change')); document.querySelector('#pause').click()`);
    assert.equal(await evaluate(`document.querySelector('#pause').textContent`), 'Play');
    const frozen = await evaluate(`elapsed`);
    await delay(100);
    assert.equal(await evaluate(`elapsed`), frozen);
    await evaluate(`document.querySelector('#restart').click()`);
    assert.equal(await evaluate(`elapsed`), 0);
    const screenshot = await page('Page.captureScreenshot', { format: 'png' });
    const screenshotPath = path.join(os.tmpdir(), 'face-lab-desktop.png');
    fs.writeFileSync(screenshotPath, Buffer.from(screenshot.data, 'base64'));
    await page('Emulation.setDeviceMetricsOverride', { width: 390, height: 844, deviceScaleFactor: 1, mobile: true });
    assert.equal(await evaluate(`document.documentElement.scrollWidth <= window.innerWidth`), true, 'Mobile layout should not overflow.');
    assert.deepEqual(errors, [], 'Browser must not report JavaScript exceptions.');
    console.log('Browser checks passed: PNG import, invalid dimensions, frame order/count, eye and mouth upload, persistence, FPS, pause/restart, and mobile layout.');
    console.log(`Desktop screenshot: ${screenshotPath}`);
  } catch (error) {
    if (browserLog) console.error(browserLog.slice(-6000));
    throw error;
  } finally {
    if (socket) { try { await call('Browser.close'); } catch {} socket.close(); }
    if (browser && browser.exitCode === null) browser.kill();
    fixture.stdin.end('\n');
    await delay(1200);
    if (fixture.exitCode === null) fixture.kill();
    // This directory is owned by this test and resolved under os.tmpdir().
    if (temporary.startsWith(path.join(os.tmpdir(), 'face-lab-browser-'))) {
      fs.rmSync(temporary, { recursive: true, force: true, maxRetries: 10, retryDelay: 200 });
    }
  }
})().catch(error => { console.error(error); process.exitCode = 1; });
