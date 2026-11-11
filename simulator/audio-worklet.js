/* AudioWorklet: original AMB/DUB/ZEN/GHOULBOX WASM renderers + HYBRID C mixer. */
const hasHighResClock=typeof globalThis.performance?.now==='function';
const timingSource=hasHighResClock?'performance.now':'Date.now (coarse, 1 ms granularity)';
const timingResolutionMs=hasHighResClock?0:1;
const VOLCA_NOTES=[
 [36,36,43,36,0,36,46,36,36,0,48,43,36,36,0,34],
 [60,0,64,67,0,64,67,71,60,0,64,67,0,65,69,72],
 [36,42,42,42,38,42,36,42,36,42,42,42,38,42,46,42],
 [48,55,60,64,67,64,60,55,50,57,62,65,69,65,62,57],
 [36,0,38,0,42,42,0,46,36,0,40,0,42,0,46,0]
];
const VOLCA_VELOCITY=[
 [112,72,96,72,0,96,80,72,104,0,92,88,104,72,0,100],
 [96,0,78,92,0,78,92,100,96,0,78,92,0,78,92,104],
 [118,0,76,0,110,0,84,0,118,0,76,0,110,0,96,0],
 [88,72,78,84,96,84,78,72,80,72,78,84,96,84,78,72],
 [108,0,76,0,92,88,0,100,108,0,76,0,92,0,100,0]
];
const nowMs=()=>hasHighResClock?globalThis.performance.now():Date.now();
class SMK37HybridProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();
    this.ready=false;this.failed=false;this.rack=null;this.core=null;this.ghoulbox=null;this.pendingMessages=[];
    this.count=0;this.underruns=0;this.lateCallbacks=0;this.renderOverrunCandidates=0;this.lastTime=0;this.maxJitter=0;this.jitter=[];
    this.maxRender=0;this.rackMax=0;this.ghoulMax=0;this.mixerMax=0;this.rackTotal=0;this.ghoulTotal=0;this.mixerTotal=0;this.seqMax=0;this.seqTotal=0;this.seqSteps=0;this.seqClockTicks=0;this.seqLaneEvents=[0,0,0,0];this.period=128/sampleRate*1000;this.ghoulViews=new Map();this.outputViews=new Map();this.volca={running:false,bpm:120,clockPhase:0,clockDivider:0,step:0,chain:[0],chainPosition:0,variation:0,fill:false,externalClock:false,muteMask:0,soloMask:0,active:[0,0,0,0]};
    this.port.onmessage=e=>this.message(e.data);
    this.initialize(options.processorOptions?.wasmBytes).catch(err=>{
      this.failed=true;this.port.postMessage({type:'error',message:err.message});
    });
  }
  async initialize(wasmBytes) {
    if(!wasmBytes?.core||!Array.isArray(wasmBytes?.engines)||wasmBytes.engines.length!==3)
      throw Error('Missing preloaded WASM buffers from main thread');
    const instance=await WebAssembly.instantiate(wasmBytes.core,{});
    this.core=instance.instance?.exports||instance.exports;
    if(this.core.hybrid_wasm_init()!==0)throw Error('HYBRID core init failed in AudioWorklet');
    this.core.hybrid_wasm_set_mix(3,0,0,0,0);
    this.ghoulbox=null;
    if(wasmBytes.ghoulbox){
      const ghoulInstance=await WebAssembly.instantiate(wasmBytes.ghoulbox,{});
      this.ghoulbox=ghoulInstance.instance?.exports||ghoulInstance.exports;
      for(const name of ['_initialize','web_nor_erase','web_boot','web_midi','web_render','web_out_l','web_out_r'])
        if(typeof this.ghoulbox[name]!=='function')throw Error('Missing original GHOULBOX WASM export: '+name);
      this.ghoulbox._initialize();this.ghoulbox.web_nor_erase();this.ghoulbox.web_boot();
      for(let i=0;i<180;i++)this.ghoulbox.web_render(128);
    }
    this.rack=new globalThis.HYBRIDNativeEngines.NativeEngineRack();
    const loaded=await this.rack.loadAll(wasmBytes.engines);
    if(loaded.length!==3)throw Error('Expected original AMB/DUB/ZEN DSPs');
    this.rack.setMaster(100);
    // Warm the exact three-DSP + GHOULBOX + Q15 mixer path before exposing the node.
    // This keeps first-call WASM/JIT work out of the live render deadline.
    for(let warm=0;warm<8;warm++){
      this.rack.renderToCore(this.core,128,false);
      if(this.ghoulbox)this.ghoulbox.web_render(128);
      if(this.ghoulbox){
      const gm=this.ghoulbox.memory.buffer,cm=this.core.memory.buffer;
      const glp=this.ghoulbox.web_out_l()>>>0,grp=this.ghoulbox.web_out_r()>>>0,gp=this.core.hybrid_wasm_pcm_ptr(3)>>>0;
      let gv=this.ghoulViews.get(128);
      if(!gv||gv.ghoulMemory!==gm||gv.coreMemory!==cm||gv.glp!==glp||gv.grp!==grp||gv.gp!==gp){
        gv={ghoulMemory:gm,coreMemory:cm,glp,grp,gp,left:new Float32Array(gm,glp,128),right:new Float32Array(gm,grp,128),pcm:new Int32Array(cm,gp,256)};
        this.ghoulViews.set(128,gv);
      }
      for(let i=0;i<128;i++){
        gv.pcm[2*i]=Math.max(-32768,Math.min(32767,Math.round(gv.left[i]*32767)));
        gv.pcm[2*i+1]=Math.max(-32768,Math.min(32767,Math.round(gv.right[i]*32767)));
      }
      if(this.core.hybrid_wasm_submit_pcm(3,128)!==0)throw Error('GHOULBOX warm-up PCM submit failed');
      }
      if(this.core.hybrid_wasm_process(128)!==0)throw Error('HYBRID warm-up mixer failed');
    }
    this.rack.allNotesOff();this.core.hybrid_wasm_panic();
    this.ready=true;
    for(const message of this.pendingMessages.splice(0))this.message(message);
    this.port.postMessage({type:'ready',engines:[...loaded,...(this.ghoulbox?['GHOULBOX original firmware']:[])]});
  }
  volcaConfigure(m) {
    const s=this.volca;
    if(Number.isFinite(m.bpm))s.bpm=Math.max(40,Math.min(220,m.bpm|0));
    if(Array.isArray(m.chain)&&m.chain.length)s.chain=m.chain.slice(0,16).map(x=>Math.max(0,Math.min(4,x|0)));
    if(Number.isFinite(m.variation))s.variation=m.variation?1:0;
    if(typeof m.fill==='boolean')s.fill=m.fill;
    if(typeof m.externalClock==='boolean')s.externalClock=m.externalClock;
    if(Number.isInteger(m.muteMask))s.muteMask=m.muteMask&15;
    if(Number.isInteger(m.soloMask))s.soloMask=m.soloMask&15;
  }
  volcaNote(track,note,velocity,on) {
    if(track<3&&this.rack){
      return on?this.rack.noteOnEngine(track,note,velocity):this.rack.noteOffEngine(track,note);
    } else if(track===3&&this.ghoulbox) {
      this.ghoulbox.web_midi((on?0x90:0x80)|3,note,on?velocity:0);return true;
    }
    return false;
  }
  volcaStep() {
    const s=this.volca,step=s.step,family=s.chain[s.chainPosition]??0,solo=s.soloMask;
    for(let track=0;track<4;track++){
      if(s.active[track]){this.volcaNote(track,s.active[track],0,false);s.active[track]=0;}
      const bit=1<<track;if((s.muteMask&bit)||(solo&&!(solo&bit)))continue;
      let note=VOLCA_NOTES[family][step],velocity=VOLCA_VELOCITY[family][step];
      if(s.variation){if(step%4===2)note=0;else if(note&&family!==2&&family!==4)note=Math.min(127,note+12);}
      if(s.fill&&step>=12&&((step+track)&1)===0){note=family===2?38:(note||48);velocity=120;}
      if(note){
        if(track<3&&family!==2&&family!==4)note=Math.min(127,note+track*12);
        if(this.volcaNote(track,note,velocity,true)){this.seqLaneEvents[track]++;s.active[track]=note;}
      }
    }
    s.step=(s.step+1)&15;this.seqSteps++;
    if(s.step===0)s.chainPosition=(s.chainPosition+1)%s.chain.length;
  }
  volcaStart() {
    const s=this.volca;
    for(let track=0;track<4;track++)if(s.active[track]){this.volcaNote(track,s.active[track],0,false);s.active[track]=0;}
    s.running=true;s.clockPhase=0;s.clockDivider=0;s.step=0;s.chainPosition=0;
    if(this.ghoulbox)this.ghoulbox.web_midi(0xFA,0,0);
    this.volcaStep();
  }
  volcaStop() {
    const s=this.volca;
    for(let track=0;track<4;track++)if(s.active[track]){this.volcaNote(track,s.active[track],0,false);s.active[track]=0;}
    s.running=false;s.clockPhase=0;s.clockDivider=0;
    if(this.ghoulbox)this.ghoulbox.web_midi(0xFC,0,0);
  }
  volcaClockPulse(external=false) {
    const s=this.volca;if(!s.running||(external&&!s.externalClock))return;
    this.seqClockTicks++;
    if(this.ghoulbox)this.ghoulbox.web_midi(0xF8,0,0);
    if(++s.clockDivider>=6){s.clockDivider=0;this.volcaStep();}
  }
  volcaAdvance(frames) {
    const s=this.volca;if(!s.running||s.externalClock)return;
    const samplesPerClock=sampleRate*60/(s.bpm*24);
    s.clockPhase+=frames;
    while(s.clockPhase>=samplesPerClock){s.clockPhase-=samplesPerClock;this.volcaClockPulse(false);}
  }
  message(m) {
    if(!m)return;
    if(m.type==='volcaConfigure'){this.volcaConfigure(m);return;}
    if(m.type==='volcaStart'){this.volcaConfigure(m);this.volcaStart();return;}
    if(m.type==='volcaStop'){this.volcaStop();return;}
    if(m.type==='volcaExternalStart'){this.volca.externalClock=true;this.volcaStart();return;}
    if(m.type==='volcaExternalStop'){this.volcaStop();return;}
    if(m.type==='volcaExternalClock'){this.volcaClockPulse(true);return;}
    if(!this.ready){if(['presetStep','programChange','volcaConfigure','volcaStart','volcaStop','volcaExternalStart','volcaExternalStop','volcaExternalClock'].includes(m.type))this.pendingMessages.push(m);return;}
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
    else if(m.type==='programChange'){const ch=Number.isInteger(m.channel)&&m.channel>=1&&m.channel<=16?m.channel:1;const program=Math.max(0,Math.min(127,m.program|0));if(this.ghoulbox)this.ghoulbox.web_midi(0xC0|(ch-1),program,0);}
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
    if(this.lastTime){const interval=now-this.lastTime,j=Math.abs(interval-this.period);this.jitter.push(j);if(this.jitter.length>512)this.jitter.shift();this.maxJitter=Math.max(this.maxJitter,j);if(interval>this.period*1.5+timingResolutionMs)this.lateCallbacks++;}
    this.lastTime=now;
    const start=nowMs(),frames=left.length;this.period=frames/sampleRate*1000;
    try{
      if(frames>128)throw Error('AudioWorklet quantum exceeds 128 frames');
      let segmentStart=nowMs();
      this.volcaAdvance(frames);
      const seqMs=nowMs()-segmentStart;this.seqMax=Math.max(this.seqMax,seqMs);this.seqTotal+=seqMs;
      segmentStart=nowMs();
      this.rack.renderToCore(this.core,frames,false);
      const rackMs=nowMs()-segmentStart;this.rackMax=Math.max(this.rackMax,rackMs);this.rackTotal+=rackMs;
      segmentStart=nowMs();
      if(this.ghoulbox){
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
      }
      if(this.core.hybrid_wasm_process(frames)!==0)throw Error('HYBRID mixer process failed');
      const outputMemory=this.core.memory.buffer,ptr=this.core.hybrid_wasm_output_ptr()>>>0;
      let ov=this.outputViews.get(frames);
      if(!ov||ov.memory!==outputMemory||ov.ptr!==ptr){ov={memory:outputMemory,ptr,pcm:new Int32Array(outputMemory,ptr,frames*2)};this.outputViews.set(frames,ov);}
      const pcm=ov.pcm;
      for(let i=0;i<frames;i++){left[i]=pcm[i*2]/32768;right[i]=pcm[i*2+1]/32768;}
      const mixerMs=nowMs()-segmentStart;this.mixerMax=Math.max(this.mixerMax,mixerMs);this.mixerTotal+=mixerMs;
    }catch(err){left.fill(0);if(right!==left)right.fill(0);missed=true;this.port.postMessage({type:'error',message:err.message});}
    const renderMs=nowMs()-start;this.maxRender=Math.max(this.maxRender,renderMs);
    if(renderMs>this.period+timingResolutionMs){if(hasHighResClock)missed=true;else this.renderOverrunCandidates++;}
    if(missed)this.underruns++;
    this.count++;
    if((this.count&255)===0){
      const s=this.jitter.slice().sort((a,b)=>a-b),p95=s.length?s[Math.min(s.length-1,Math.ceil(s.length*.95)-1)]:0;
      this.port.postMessage({type:'metrics',callbacks:this.count,underruns:this.underruns,lateCallbacks:this.lateCallbacks,renderOverrunCandidates:this.renderOverrunCandidates,jitterP95:p95,jitterMax:this.maxJitter,renderMax:this.maxRender,sequencerMax:this.seqMax,sequencerAvg:this.seqTotal/this.count,seqSteps:this.seqSteps,seqClockTicks:this.seqClockTicks,seqLaneEvents:this.seqLaneEvents.slice(),rackMax:this.rackMax,ghoulboxMax:this.ghoulMax,mixerMax:this.mixerMax,rackAvg:this.rackTotal/this.count,ghoulboxAvg:this.ghoulTotal/this.count,mixerAvg:this.mixerTotal/this.count,timingSource,timingResolutionMs});
    }
    return true;
  }
}
registerProcessor('smk37-hybrid-audio',SMK37HybridProcessor);
