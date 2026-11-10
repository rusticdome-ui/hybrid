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
  const transferred = rack.renderToCore(core,128);
  if(transferred.length!==3)throw new Error('Expected exactly three PCM transfers');
  const expected=new Map();
  for(const t of transferred) {
    if(!t.sourcePcm||t.sourcePcm.length!==256)throw new Error(t.name+': missing original float PCM snapshot');
    const q=new Int32Array(256);let nonzero=0;
    for(let i=0;i<256;i++) {
      const sample=t.sourcePcm[i];
      if(!Number.isFinite(sample))throw new Error(t.name+': non-finite original PCM');
      q[i]=clamp(Math.round(Math.max(-1,Math.min(1,sample))*32767));
      if(q[i]!==0)nonzero++;
      if(t.samples[i]!==q[i])throw new Error(t.name+': produced PCM differs from Q15 transfer at sample '+i);
    }
    if(!nonzero)throw new Error(t.name+': original DSP rendered zero PCM');
    expected.set(t.id,{name:t.name,q,nonzero});
    const ptr=core.hybrid_wasm_pcm_ptr(t.id)>>>0;
    const received=new Int32Array(core.memory.buffer,ptr,256);
    for(let i=0;i<256;i++)if(received[i]!==q[i])
      throw new Error(t.name+': HYBRID input buffer differs at sample '+i);
    console.log(t.name+': nonzero PCM='+nonzero+'/256; original float PCM == Q15 transfer == HYBRID input');
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
  // Long-run block stability: exercise each supported block size, repeated
  // original DSP render -> PCM submit -> Q15 mixer -> output path.
  const blockSizes=[1,16,32,64,128];
  let stableBlocks=0,nonzeroBlocks=0,stableFrames=0;
  rack.noteOn(53,127,0);
  for(let cycle=0;cycle<30000;cycle++){
    const frames=128;
    rack.renderToCore(core,frames);
    if(core.hybrid_wasm_process(frames)!==0)throw new Error('Mixer failed at stability cycle '+cycle+' / '+frames+' frames');
    const p=core.hybrid_wasm_output_ptr()>>>0;
    const out=new Int32Array(core.memory.buffer,p,frames*2);
    for(let i=0;i<out.length;i++)if(!Number.isInteger(out[i])||out[i]<-32768||out[i]>32767)
      throw new Error('Invalid Q15 output at cycle '+cycle+', sample '+i);
    if(out.some(v=>v!==0))nonzeroBlocks++;
    stableBlocks++;stableFrames+=frames;
  }
  for(let cycle=0;cycle<200;cycle++){
    const frames=blockSizes[cycle%blockSizes.length];
    rack.renderToCore(core,frames);
    if(core.hybrid_wasm_process(frames)!==0)throw new Error('Variable PCM block failed at cycle '+cycle+' / '+frames+' frames');
    stableBlocks++;stableFrames+=frames;
  }
  if(stableBlocks!==30200||nonzeroBlocks===0)throw new Error('PCM block stability check failed');
  console.log('PCM long-run stability PASS: '+stableBlocks+' blocks / '+stableFrames+' frames (~'+(stableFrames/44100).toFixed(1)+' s at 44.1 kHz); fixed 128-frame blocks plus 200 variable blocks (1/16/32/64/128); nonzero='+nonzeroBlocks);
  console.log('END-TO-END ORIGINAL DSP WASM PCM TEST: PASS');
})().catch(err=>{console.error(err);process.exit(1);});
