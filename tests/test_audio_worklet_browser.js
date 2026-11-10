/* Browser-level regression: real Chromium AudioWorklet, original DSP WASM and non-zero output. */
'use strict';
const http = require('node:http');
const fs = require('node:fs');
const path = require('node:path');
const { chromium } = require('playwright');

const root = path.resolve(__dirname, '..', 'simulator');
const mime = { '.html':'text/html; charset=utf-8', '.js':'text/javascript; charset=utf-8', '.wasm':'application/wasm', '.json':'application/json' };
const server = http.createServer((req, res) => {
  const pathname = decodeURIComponent(new URL(req.url, 'http://127.0.0.1').pathname);
  const file = path.resolve(root, '.' + pathname);
  if (pathname !== '/' && !file.startsWith(root + path.sep)) {
    res.writeHead(403); res.end('forbidden'); return;
  }
  const target = pathname === '/' ? path.join(root, 'index.html') : file;
  fs.readFile(target, (err, data) => {
    if (err) { res.writeHead(404); res.end('not found'); return; }
    res.writeHead(200, { 'content-type': mime[path.extname(target)] || 'application/octet-stream', 'cache-control':'no-store' });
    res.end(data);
  });
});

(async () => {
  let browser;
  try {
    await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
    const address = server.address();
    browser = await chromium.launch({ headless:true, args:['--no-sandbox','--disable-dev-shm-usage','--autoplay-policy=no-user-gesture-required'] });
    const page = await browser.newPage();
    const pageErrors = [];
    page.on('pageerror', e => pageErrors.push(e.message));
    page.on('console', m => { if (m.type() === 'error') pageErrors.push(m.text()); });
    const response = await page.goto('http://127.0.0.1:' + address.port + '/', { waitUntil:'networkidle' });
    if (!response || !response.ok()) throw new Error('Simulator HTTP load failed');
    await page.locator('#audio').click();
    await page.waitForFunction(() => {
      const t = document.querySelector('#log')?.textContent || '';
      return t.includes('AudioWorklet activo:') || t.includes('ERRO WASM:') || t.includes('AudioWorklet:');
    }, null, { timeout:30000 });
    const startupLog = await page.locator('#log').textContent();
    if (!startupLog.includes('AudioWorklet activo:')) throw new Error('AudioWorklet did not become ready. Browser log: ' + startupLog + ' | page errors: ' + pageErrors.join(' | '));
    // Isolate slot D so the analyser proves the original GHOULBOX WASM renderer is audible.
    for (let i = 0; i < 4; i++) {
      await page.locator('#engines input[data-i="' + i + '"][data-k="gain"]').evaluate((el, value) => {
        el.value = String(value);
        el.dispatchEvent(new Event('input', { bubbles:true }));
      }, i === 3 ? 100 : 0);
    }
    await page.locator('#chord').click();
    await page.waitForFunction(() => {
      const a = window.__hybridAudioProbe;
      if (!a) return false;
      const data = new Float32Array(a.fftSize);
      a.getFloatTimeDomainData(data);
      let sum = 0;
      for (const v of data) sum += v * v;
      return Math.sqrt(sum / data.length) > 0.00001;
    }, null, { timeout:12000 });
    await page.waitForFunction(() => document.querySelector('#audioMetrics').textContent.includes('AudioWorklet 256'), null, { timeout:15000 });
    // Exercise all four patch controls in real Chromium, observing the messages
    // actually sent to the AudioWorklet rather than merely checking button presence.
    await page.waitForFunction(() => (document.querySelector('#log')?.textContent || '').includes('DSP originais WASM activos'), null, { timeout:30000 });
    await page.evaluate(() => {
      const port = window.__hybridAudioNode.port;
      const original = port.postMessage.bind(port);
      window.__hybridPatchMessages = [];
      port.postMessage = message => {
        if (message && (message.type === 'presetStep' || message.type === 'programChange')) window.__hybridPatchMessages.push(message);
        return original(message);
      };
    });
    for (const [select, button, id] of [['#presetAmb','#applyAmb',0],['#presetDub','#applyDub',1],['#presetZen','#applyZen',2]]) {
      await page.locator(select).selectOption('4');
      await page.locator(button).click();
    }
    await page.locator('#presetGhoul').fill('37');
    await page.locator('#applyGhoul').click();
    const patchMessages = await page.evaluate(() => window.__hybridPatchMessages);
    for (const id of [0,1,2]) {
      if (!patchMessages.some(m => m.type === 'presetStep' && m.id === id && m.delta === 4))
        throw new Error('Chromium patch selector did not dispatch PRESET +4 for engine ' + id + ': ' + JSON.stringify(patchMessages));
    }
    if (!patchMessages.some(m => m.type === 'programChange' && m.program === 37 && m.channel === 1))
      throw new Error('Chromium GHOULBOX selector did not dispatch Program Change 37: ' + JSON.stringify(patchMessages));
    // Validate the ZEN import path and its exact 128-byte contract.
    await page.locator('#zenPatch').setInputFiles({ name:'chromium-patch.bin', mimeType:'application/octet-stream', buffer:Buffer.alloc(128, 0x5a) });
    await page.locator('#zenPatchSet').click();
    await page.waitForFunction(() => (document.querySelector('#log')?.textContent || '').includes('Patch ZEN aplicado no adaptador nativo'), null, { timeout:5000 });
    console.log('CHROMIUM PATCH SELECTORS PASS: AMB-1, DUB-1, ZEN-1 PRESET; GHOULBOX Program Change; ZEN 128-byte import.');
    const metrics = await page.locator('#audioMetrics').textContent();
    const log = await page.locator('#log').textContent();
    if (!log.includes('GHOULBOX original firmware')) throw new Error('GHOULBOX original firmware was not reported ready: ' + log);
    if (log.includes('AudioWorklet:')) throw new Error('AudioWorklet reported an error: ' + log);
    if (pageErrors.length) throw new Error('Browser errors: ' + pageErrors.join(' | '));
    console.log('CHROMIUM AUDIOWORKLET PASS');
    console.log('Non-zero audio signal observed at Web Audio analyser.');
    console.log(metrics);
  } finally {
    if (browser) await browser.close();
    await new Promise(resolve => server.close(resolve));
  }
})().catch(err => { console.error(err); process.exit(1); });
