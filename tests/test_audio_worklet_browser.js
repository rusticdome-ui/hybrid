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
    await page.waitForFunction(() => document.querySelector('#log').textContent.includes('AudioWorklet activo:'), null, { timeout:30000 });
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
    const metrics = await page.locator('#audioMetrics').textContent();
    const log = await page.locator('#log').textContent();
    if (!log.includes('GHOULBOX original firmware')) throw new Error('GHOULBOX original firmware was not reported ready: ' + log);\n    if (log.includes('AudioWorklet:')) throw new Error('AudioWorklet reported an error: ' + log);
    if (pageErrors.length) throw new Error('Browser errors: ' + pageErrors.join(' | '));
    console.log('CHROMIUM AUDIOWORKLET PASS');
    console.log('Non-zero audio signal observed at Web Audio analyser.');
    console.log(metrics);
  } finally {
    if (browser) await browser.close();
    await new Promise(resolve => server.close(resolve));
  }
})().catch(err => { console.error(err); process.exit(1); });
