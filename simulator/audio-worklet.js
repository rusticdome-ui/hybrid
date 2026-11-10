/* AudioWorklet: original AMB/DUB/ZEN/GHOULBOX WASM renderers + HYBRID C mixer. */
const hasHighResClock=typeof globalThis.performance?.now==='function';
const timingSource=hasHighResClock?'performance.now':'Date.now (coarse, 1 ms granularity)';
const timingResolutionMs=hasHighResClock?0:1;
const nowMs=()=>hasHighResClock?globalThis.performance.now():Date.now();
class SMK37HybridProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();
    this.ready=false;this.failed=false;this.rack=null;this.core=null;this.ghoulbox=null;this.pendingMessages=[];
    this.count=0;this.underruns=0;this.lastTime=0;this.maxJitter=0;this.jitter=[];
    this.maxRender=0;this.rackMax=0;this.ghoulMax=0;this.mixerMax=0;this.rackTotal=0;this.ghoulTotal=0;this.mixerTotal=0;this.period=128/sampleRate*1000;this.ghoulViews=new Map();this.outputViews=new Map();
    this.port.onmessage=e=>this.message(e.data);
    this.initialize(options.processorOptions?.wasmBytes).catch(err=>{
      this.failed=true;this.port.postMessage({type:'error',message:err.message});
    });
  }
  async initialize(wasmBytes) {
    if(!wasmBytes?.core||!wasmBytes?.ghoulbox||!Array.isArray(wasmBytes?.engines)||wasmBytes.engines.length!==3)
      throw Error('Missing preloaded WASM buffers from main thread');
    const instance=await WebAssembly.instantiate(wasmBytes.core,{});
    this.core=instance.instance?.exports||instance.exports;
    if(this.core.hybrid_wasm_init()!==0)throw Error('HYBRID core init failed in AudioWorklet');
    this.core.hybrid_wasm_set_mix(3,0,0,0,0);
    const ghoulInstance=await WebAssembly.instantiate(wasmBytes.ghoulbox,{});
    this.ghoulbox=ghoulInstance.instance?.exports||ghoulInstance.exports;
    for(const name of ['_initialize','web_nor_erase','web_boot','web_midi','web_render','web_out_l','web_out_r'])
      if(typeof this.ghoulbox[name]!=='function')throw Error('Missing original GHOULBOX WASM export: '+name);
    this.ghoulbox._initialize();this.ghoulbox.web_nor_erase();this.ghoulbox.web_boot();
    for(let i=0;i<180;i++)this.ghoulbox.web_render(128); // run original firmware boot clock before audio starts
    this.rack=new globalThis.HYBRIDNativeEngines.NativeEngineRack();
    const loaded=await this.rack.loadAll(wasmBytes.engines);
    if(loaded.length!==3)throw Error('Expected original AMB/DUB/ZEN DSPs');
    this.rack.setMaster(100);this.ready=true;
    for(const message of this.pendingMessages.splice(0))this.message(message);
    this.port.postMessage({type:'ready',engines:[...loaded,'GHOULBOX original firmware']});
  }
  message(m) {
    if(!m)return;
    if(!this.ready){if(m.type==='presetStep'||m.type==='programChange')this.pendingMessages.push(m);return;}
    if(m.type==='noteOn'){
      this.rack.noteOn(m.note,m.velocity,m.channel);
      const ch=Number.isInteger(m.channel)&&m.channel>=1&&m.channel<=4?m.channel:1;
      this.ghoulbox.web_midi(0x90|(ch-1),m.note,m.velocity);
    }
    else if(m.type==='noteOff'){
      this.rack.noteOff(m.note,m.channel);
      const ch=Number.isInteger(m.channel)&&m.channel>=1&&m.channel<=4?m.channel:1;
      this.ghoulbox.web_midi(0x80|(ch-1),m.note,0);
    }
    else if(m.type==='panic'){
      this.rack.allNotesOff();
      for(let ch=0;ch<4;ch++)for(let note=0;note<128;note++)this.ghoulbox.web_midi(0x80|ch,note,0);
    }
    else if(m.type==='presetStep'){if(this.rack&&Number.isInteger(m.id)&&m.id>=0&&m.id<3)this.rack.encoder(m.id,2,m.delta|0);}
    else if(m.type==='programChange'){const ch=Number.isInteger(m.channel)&&m.channel>=1&&m.channel<=16?m.channel:1;const program=Math.max(0,Math.min(127,m.program|0));this.ghoulbox.web_midi(0xC0|(ch-1),program,0);}
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
    const now=nowMs();let missed=false;
    if(this.lastTime){const interval=now-this.lastTime,j=Math.abs(interval-this.period);this.jitter.push(j);if(this.jitter.length>512)this.jitter.shift();this.maxJitter=Math.max(this.maxJitter,j);if(interval>this.period*1.5+timingResolutionMs)missed=true;}
    this.lastTime=now;
    const start=nowMs(),frames=left.length;this.period=frames/sampleRate*1000;
    try{
      if(frames>128)throw Error('AudioWorklet quantum exceeds 128 frames');
      let segmentStart=nowMs();
      this.rack.renderToCore(this.core,frames,false);
      const rackMs=nowMs()-segmentStart;this.rackMax=Math.max(this.rackMax,rackMs);this.rackTotal+=rackMs;
      segmentStart=nowMs();
      this.ghoulbox.web_render(frames);
      const ghoulMs=nowMs()-segmentStart;this.ghoulMax=Math.max(this.ghoulMax,ghoulMs);this.ghoulTotal+=ghoulMs;
      segmentStart=nowMs();
      const ghoulMemory=this.ghoulbox.memory.buffer,coreMemory=this.core.memory.buffer;
      const glp=this.ghoulbox.web_out_l()>>>0,grp=this.ghoulbox.web_out_r()>>>0,gp=this.core.hybrid_wasm_pcm_ptr(3)>>>0;
      let gv=this.ghoulViews.get(frames);
      if(!gv||gv.ghoulMemory!==ghoulMemory||gv.coreMemory!==coreMemory||gv.glp!==glp||gv.grp!==grp||gv.gp!==gp){
        gv={ghoulMemory,coreMemory,glp,grp,gp,left:new Float32Array(ghoulMemory,glp,frames),right:new Float32Array(ghoulMemory,grp,frames),pcm:new Int32Array(coreMemory,gp,frames*2)};
        this.ghoulViews.set(frames,gv);
      }
      const gl=gv.left,gr=gv.right,gq=gv.pcm;
      for(let i=0;i<frames;i++){
        gq[2*i]=Math.max(-32768,Math.min(32767,Math.round(gl[i]*32767)));
        gq[2*i+1]=Math.max(-32768,Math.min(32767,Math.round(gr[i]*32767)));
      }
      if(this.core.hybrid_wasm_submit_pcm(3,frames)!==0)throw Error('GHOULBOX PCM submit failed');
      if(this.core.hybrid_wasm_process(frames)!==0)throw Error('HYBRID mixer process failed');
      const outputMemory=this.core.memory.buffer,ptr=this.core.hybrid_wasm_output_ptr()>>>0;
      let ov=this.outputViews.get(frames);
      if(!ov||ov.memory!==outputMemory||ov.ptr!==ptr){ov={memory:outputMemory,ptr,pcm:new Int32Array(outputMemory,ptr,frames*2)};this.outputViews.set(frames,ov);}
      const pcm=ov.pcm;
      for(let i=0;i<frames;i++){left[i]=pcm[i*2]/32768;right[i]=pcm[i*2+1]/32768;}
      const mixerMs=nowMs()-segmentStart;this.mixerMax=Math.max(this.mixerMax,mixerMs);this.mixerTotal+=mixerMs;
    }catch(err){left.fill(0);if(right!==left)right.fill(0);missed=true;this.port.postMessage({type:'error',message:err.message});}
    const renderMs=nowMs()-start;this.maxRender=Math.max(this.maxRender,renderMs);
    if(renderMs>this.period+timingResolutionMs)missed=true;
    if(missed)this.underruns++;
    this.count++;
    if((this.count&255)===0){
      const s=this.jitter.slice().sort((a,b)=>a-b),p95=s.length?s[Math.min(s.length-1,Math.ceil(s.length*.95)-1)]:0;
      this.port.postMessage({type:'metrics',callbacks:this.count,underruns:this.underruns,jitterP95:p95,jitterMax:this.maxJitter,renderMax:this.maxRender,rackMax:this.rackMax,ghoulboxMax:this.ghoulMax,mixerMax:this.mixerMax,rackAvg:this.rackTotal/this.count,ghoulboxAvg:this.ghoulTotal/this.count,mixerAvg:this.mixerTotal/this.count,timingSource,timingResolutionMs});
    }
    return true;
  }
}
registerProcessor('smk37-hybrid-audio',SMK37HybridProcessor);
