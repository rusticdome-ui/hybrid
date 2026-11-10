/* Original GHOULBOX firmware WebAssembly smoke test (Felucca's own web/emu renderer). */
'use strict';
const fs = require('node:fs');

(async () => {
  const bytes = fs.readFileSync('simulator/native/ghoulbox.wasm');
  const result = await WebAssembly.instantiate(bytes, {});
  const ex = result.instance.exports;
  for (const name of ['_initialize','web_nor_erase','web_boot','web_midi','web_render','web_out_l','web_out_r']) {
    if (typeof ex[name] !== 'function') throw new Error('GHOULBOX WASM export missing: ' + name);
  }
  ex._initialize();
  ex.web_nor_erase();
  ex.web_boot();
  for (let i = 0; i < 180; i++) ex.web_render(128); // firmware boot clock (~522 ms)
  const notes = [[0x90,60],[0x91,64],[0x92,67],[0x93,36]];
  for (const [status,note] of notes) {
    if (ex.web_midi(status,note,112) === 0) throw new Error('GHOULBOX MIDI queue rejected channel ' + (status & 15 + 1));
  }
  let peak = 0, energy = 0, samples = 0;
  for (let block = 0; block < 220; block++) {
    ex.web_render(128);
    const l = new Float32Array(ex.memory.buffer, ex.web_out_l() >>> 0, 128);
    const r = new Float32Array(ex.memory.buffer, ex.web_out_r() >>> 0, 128);
    for (let i = 0; i < 128; i++) {
      const a = Math.max(Math.abs(l[i]), Math.abs(r[i]));
      if (a > peak) peak = a;
      energy += l[i] * l[i] + r[i] * r[i];
      samples += 2;
    }
  }
  for (const [status,note] of notes) ex.web_midi(0x80 | (status & 15),note,0);
  const rms = Math.sqrt(energy / samples);
  if (!(peak > 1e-5) || !(rms > 1e-6)) throw new Error('Original GHOULBOX renderer produced silent PCM: peak=' + peak + ' rms=' + rms);
  console.log('GHOULBOX ORIGINAL FIRMWARE WASM PASS');
  console.log('4 MIDI channels; peak=' + peak.toFixed(6) + ' RMS=' + rms.toFixed(6));
})().catch(err => { console.error(err); process.exit(1); });
