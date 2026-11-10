/* AudioWorklet: original AMB/DUB/ZEN WASM DSPs + HYBRID C mixer run in this thread. */
class SMK37HybridProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();
    this.ready=false;this.failed=false;this.rack=null;this.core=null;
    this.count=0;this.underruns=0;this.lastTime=0;this.maxJitter=0;this.jitter=[];
    this.maxRender=0;this.period=128/sampleRate*1000;
    this.port.onmessage=e=>this.message(e.data);
    this.initialize(options.processorOptions?.baseUrl||'./').catch(err=>{
      this.failed=true;this.port.postMessage({type:'error',message:err.message});
    });
  }
  async initialize(baseUrl) {
    const coreBytes=await (await fetch(new URL('hybrid_core.wasm',baseUrl))).arrayBuffer();
    const instance=await WebAssembly.instantiate(coreBytes,{});
    this.core=instance.instance?.exports||instance.exports;
    if(this.core.hybrid_wasm_init()!==0)throw Error('HYBRID core init failed in AudioWorklet');
    this.core.hybrid_wasm_set_mix(3,0,0,1,0); // GHOULBOX original DSP is not ported; keep TEST ONLY slot silent.
    this.rack=new globalThis.HYBRIDNativeEngines.NativeEngineRack();
    const loaded=await this.rack.loadAll();
    if(loaded.length!==3)throw Error('Expected original AMB/DUB/ZEN DSPs');
    this.rack.setMaster(100);this.ready=true;
    this.port.postMessage({type:'ready',engines:loaded});
  }
  message(m) {
    if(!m||!this.ready)return;
    if(m.type==='noteOn')this.rack.noteOn(m.note,m.velocity,m.channel);
    else if(m.type==='noteOff')this.rack.noteOff(m.note,m.channel);
    else if(m.type==='panic')this.rack.allNotesOff();
    else if(m.type==='channel')this.rack.channel=m.value==='Omni'?0:+m.value;
    else if(m.type==='mix'){
      this.core.hybrid_wasm_set_mix(m.id,m.volume,m.pan,+m.mute,+m.solo);
      this.core.hybrid_wasm_set_route(m.id,m.mode,m.low,m.high);
      this.core.hybrid_wasm_set_midi(m.id,m.channel,m.velocityLow||1,m.velocityHigh||127,m.transpose||0);
    } else if(m.type==='master')this.rack.setMaster(m.value);
  }
  process(_inputs,outputs) {
    const out=outputs[0],left=out[0],right=out[1]||out[0];
    if(!this.ready||this.failed){left.fill(0);if(right!==left)right.fill(0);return true;}
    const now=performance.now();
    if(this.lastTime){const interval=now-this.lastTime,j=Math.abs(interval-this.period);this.jitter.push(j);if(this.jitter.length>512)this.jitter.shift();this.maxJitter=Math.max(this.maxJitter,j);if(interval>this.period*1.5)this.underruns++;}
    this.lastTime=now;
    const start=performance.now(),frames=left.length;this.period=frames/sampleRate*1000;
    try{
      if(frames>128)throw Error('AudioWorklet quantum exceeds 128 frames');
      this.rack.renderToCore(this.core,frames);
      if(this.core.hybrid_wasm_process(frames)!==0)throw Error('HYBRID mixer process failed');
      const ptr=this.core.hybrid_wasm_output_ptr()>>>0;
      const pcm=new Int32Array(this.core.memory.buffer,ptr,frames*2);
      for(let i=0;i<frames;i++){left[i]=pcm[i*2]/32768;right[i]=pcm[i*2+1]/32768;}
    }catch(err){left.fill(0);if(right!==left)right.fill(0);this.underruns++;this.port.postMessage({type:'error',message:err.message});}
    const renderMs=performance.now()-start;this.maxRender=Math.max(this.maxRender,renderMs);
    if(renderMs>this.period)this.underruns++;
    this.count++;
    if((this.count&255)===0){
      const s=this.jitter.slice().sort((a,b)=>a-b),p95=s.length?s[Math.min(s.length-1,Math.ceil(s.length*.95)-1)]:0;
      this.port.postMessage({type:'metrics',callbacks:this.count,underruns:this.underruns,jitterP95:p95,jitterMax:this.maxJitter,renderMax:this.maxRender});
    }
    return true;
  }
}
registerProcessor('smk37-hybrid-audio',SMK37HybridProcessor);
