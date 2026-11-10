/* MIDI dispatch and render-latency smoke/benchmark for original WASM DSPs.
 * Timing is host-side WASM execution latency, not physical DAC/output latency.
 */
const fs=require('fs');
global.window=global;
global.fetch=async url=>({ok:true,arrayBuffer:async()=>fs.readFileSync('simulator/'+url.replace(/^\.\//,''))});
require('../simulator/native-engines.js');
const ms=ns=>Number(ns)/1e6;
const percentile=(a,p)=>a.slice().sort((x,y)=>x-y)[Math.min(a.length-1,Math.ceil(a.length*p)-1)];
(async()=>{
  const rack=new HYBRIDNativeEngines.NativeEngineRack();
  const loaded=await rack.loadAll();
  if(loaded.length!==3)throw Error('Expected 3 original DSP engines');
  for(const e of rack.engines.values()){
    const dispatch=[],render=[];
    for(let i=0;i<100;i++){
      const t=process.hrtime.bigint();
      rack.noteOn(53,127,0);
      const t1=process.hrtime.bigint();
      if(!e.notes.has(53)||rack._mask(e)!==1)
        throw Error(e.name+': MIDI Note On did not set the expected key bit');
      e.exports.web_render(128);
      const t2=process.hrtime.bigint();
      const l=new Float32Array(e.exports.memory.buffer,e.exports.web_out_l()>>>0,128);
      const r=new Float32Array(e.exports.memory.buffer,e.exports.web_out_r()>>>0,128);
      if(![...l,...r].every(Number.isFinite)||![...l,...r].some(v=>v!==0))
        throw Error(e.name+': MIDI note-on did not produce valid nonzero original PCM');
      rack.noteOff(53,0);
      const t3=process.hrtime.bigint();
      if(e.notes.has(53)||rack._mask(e)!==0)
        throw Error(e.name+': MIDI Note Off did not clear the expected key bit');
      e.exports.web_render(128);
      const t4=process.hrtime.bigint();
      dispatch.push(ms(t1-t),ms(t3-t2));
      render.push(ms(t2-t1),ms(t4-t3));
    }
    const avg=dispatch.reduce((a,b)=>a+b,0)/dispatch.length;
    const avgRender=render.reduce((a,b)=>a+b,0)/render.length;
    console.log(e.name+': MIDI note-on/off + key bitfield PASS (100 cycles); dispatch avg='+avg.toFixed(4)+' ms, p95='+percentile(dispatch,.95).toFixed(4)+' ms; 128-frame render avg='+avgRender.toFixed(4)+' ms, p95='+percentile(render,.95).toFixed(4)+' ms');
  }
  console.log('MIDI + WASM EXECUTION LATENCY TEST: PASS (hardware/DAC latency not measured)');
})().catch(e=>{console.error(e);process.exit(1);});
