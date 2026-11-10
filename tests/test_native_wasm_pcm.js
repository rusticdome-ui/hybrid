/* End-to-end original DSP WASM -> HYBRID C mixer transport test. */
const fs = require('fs');
const { WASI } = require('node:wasi');
global.window = global;
global.fetch = async url => ({
  ok: true,
  arrayBuffer: async () => fs.readFileSync('simulator/' + url.replace(/^\.\//, ''))
});
require('../simulator/native-engines.js');

const clamp = x => Math.max(-32768, Math.min(32767, x));
(async () => {
  const coreModule = await WebAssembly.compile(fs.readFileSync('simulator/hybrid_core.wasm'));
  const coreImports = {};
  for (const item of WebAssembly.Module.imports(coreModule)) {
    if (item.kind === 'function') {
      if (!coreImports[item.module]) coreImports[item.module] = new Proxy({}, {get:()=>()=>0});
    } else throw new Error('Unexpected HYBRID core import: '+item.module+'.'+item.name);
  }
  const core = (await WebAssembly.instantiate(coreModule, coreImports)).exports;
  if (core.hybrid_wasm_init() !== 0) throw new Error('HYBRID core init failed');
  for (let id=0; id<3; id++) {
    if (core.hybrid_wasm_set_mix(id,32767,0,0,0)!==0) throw new Error('mix config failed for '+id);
  }
  if (core.hybrid_wasm_set_mix(3,32767,0,1,0)!==0) throw new Error('mute slot D failed');

  const rack = new global.HYBRIDNativeEngines.NativeEngineRack();
  const loaded = await rack.loadAll();
  if (loaded.length !== 3) throw new Error('Expected three independent original DSP modules');
  rack.noteOn(53,127,0);
  const expected = new Map();
  for (const e of rack.engines.values()) {
    const original = e.exports.web_render;
    e.exports.web_render = function(frames) {
      original(frames);
      const l = new Float32Array(e.exports.memory.buffer,e.exports.web_out_l()>>>0,frames);
      const r = new Float32Array(e.exports.memory.buffer,e.exports.web_out_r()>>>0,frames);
      const q = new Int32Array(frames*2);
      let nonzero=0;
      for(let i=0;i<frames;i++) {
        if(!Number.isFinite(l[i])||!Number.isFinite(r[i])) throw new Error(e.name+': non-finite source PCM');
        const a=clamp(Math.round(Math.max(-1,Math.min(1,l[i]))*32767));
        const b=clamp(Math.round(Math.max(-1,Math.min(1,r[i]))*32767));
        q[i*2]=a;q[i*2+1]=b;
        if(a!==0||b!==0)nonzero++;
      }
      if(!nonzero)throw new Error(e.name+': original DSP rendered zero PCM');
      expected.set(e.id,{name:e.name,q,nonzero});
    };
  }

  const transferred = rack.renderToCore(core,128);
  if(transferred.length!==3)throw new Error('Expected exactly three PCM transfers');
  for(const t of transferred) {
    const source=expected.get(t.id);
    if(!source)throw new Error('Unknown PCM source '+t.id);
    for(let i=0;i<source.q.length;i++) {
      if(t.samples[i]!==source.q[i])throw new Error(source.name+': transfer snapshot differs at sample '+i);
    }
    const ptr=core.hybrid_wasm_pcm_ptr(t.id)>>>0;
    const received=new Int32Array(core.memory.buffer,ptr,256);
    for(let i=0;i<256;i++)if(received[i]!==source.q[i])
      throw new Error(source.name+': HYBRID input buffer differs at sample '+i);
    console.log(source.name+': nonzero PCM='+source.nonzero+'/128; source == transfer == HYBRID input');
  }

  if(core.hybrid_wasm_process(128)!==0)throw new Error('HYBRID mixer process failed');
  const outPtr=core.hybrid_wasm_output_ptr()>>>0;
  const actual=new Int32Array(core.memory.buffer,outPtr,256);
  const sources=[0,1,2].map(id=>expected.get(id).q);
  for(let i=0;i<256;i++) {
    const sum=clamp(sources[0][i]+sources[1][i]+sources[2][i]);
    if(actual[i]!==sum)throw new Error('Q15 mix mismatch at sample '+i+': got '+actual[i]+', expected '+sum);
  }
  console.log('HYBRID mixer: exact saturated Q15 sum PASS (256 interleaved samples)');
  console.log('END-TO-END ORIGINAL DSP WASM PCM TEST: PASS');
})().catch(err=>{console.error(err);process.exit(1);});
