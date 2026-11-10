/* SPDX-License-Identifier: GPL-3.0-only
 * Native AMB-1 / DUB-1 / ZEN-1 browser adapters.
 * Maps MIDI note events to the original host keyboard interfaces and forwards
 * native panel controls. Audio remains the original DSP's web_render PCM.
 */
(() => {
  'use strict';
  const SPECS = [
    {id:0,name:'AMB-1',url:'./native/amb1.wasm',kind:'fm1-host'},
    {id:1,name:'DUB-1',url:'./native/dub1.wasm',kind:'fm1-host'},
    {id:2,name:'ZEN-1',url:'./native/zen1.wasm',kind:'zen-host'}
  ];
  const KEY_MIN = 53; // FM-1 keybed F3; plat.h documents F3..G5 (27 keys).
  const KEY_COUNT = 27;
  const clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
  function wasiNamespace(memoryRef){
    const mem=()=>memoryRef.memory;
    const u32=(p,v)=>{if(mem())new DataView(mem().buffer).setUint32(p>>>0,v>>>0,true);};
    const u64=(p,v)=>{if(mem())new DataView(mem().buffer).setBigUint64(p>>>0,BigInt.asUintN(64,BigInt(v)),true);};
    const api={
      fd_write(_fd,_iov,_count,nwritten){u32(nwritten,0);return 0;},
      fd_read(_fd,_iov,_count,nread){u32(nread,0);return 0;},
      fd_close(){return 0;},fd_sync(){return 0;},fd_datasync(){return 0;},
      fd_seek(_fd,_offset,_whence,newOffset){u64(newOffset,0);return 0;},
      fd_tell(_fd,offset){u64(offset,0);return 0;},
      args_sizes_get(argc,buf){u32(argc,0);u32(buf,0);return 0;},
      args_get(){return 0;},
      environ_sizes_get(count,buf){u32(count,0);u32(buf,0);return 0;},
      environ_get(){return 0;},
      clock_time_get(_id,_precision,out){u64(out,BigInt(Date.now())*1000000n);return 0;},
      random_get(ptr,len){
        const m=mem();if(!m)return 0;
        const bytes=new Uint8Array(m.buffer,ptr>>>0,len>>>0);
        if(globalThis.crypto&&globalThis.crypto.getRandomValues){
          for(let i=0;i<bytes.length;i+=65536)globalThis.crypto.getRandomValues(bytes.subarray(i,Math.min(i+65536,bytes.length)));
        }else bytes.fill(0);
        return 0;
      },
      proc_exit(code){throw new Error('WASM chamou proc_exit('+code+')');},
      sched_yield(){return 0;}
    };
    return new Proxy(api,{get:(target,key)=>target[key]||(()=>0)});
  }
  async function instantiateNative(bytes){
    const module=await WebAssembly.compile(bytes),memoryRef={memory:null},imports={};
    for(const item of WebAssembly.Module.imports(module)){
      if(item.kind!=='function')throw new Error('import WASM não suportado: '+item.module+'.'+item.name+' ('+item.kind+')');
      if(!imports[item.module])imports[item.module]=item.module==='wasi_snapshot_preview1'?wasiNamespace(memoryRef):new Proxy({}, {get:()=>()=>0});
    }
    const instance=await WebAssembly.instantiate(module,imports);
    memoryRef.memory=instance.exports.memory||null;
    if(!memoryRef.memory)throw new Error('módulo standalone sem export de memória');
    return instance;
  }
  class NativeEngineRack {
    constructor(){this.engines=new Map();this.enabled=false;this.status={};this.held=new Map();this.channel=0;this.master=80;}
    async loadAll(){
      const loaded=[];
      for(const spec of SPECS){
        try{
          const response=await fetch(spec.url);
          if(!response.ok)throw new Error('HTTP '+response.status+' ao obter '+spec.url);
          const bytes=await response.arrayBuffer();
          const instance=await instantiateNative(bytes);
          const e=instance.exports;
          for(const fn of ['web_boot','web_render','web_out_l','web_out_r','web_keys','web_buttons','web_enc','web_master'])
            if(typeof e[fn]!=='function')throw new Error('export em falta: '+fn);
          if(!(e.memory instanceof WebAssembly.Memory))throw new Error('memória WASM não exportada');
          if(e._initialize)e._initialize();
          e.web_boot();
          if(spec.id===2 && typeof e.web_mode==='function')e.web_mode(0);
          e.web_master(Math.round(this.master/100*4096));
          const engine={...spec,instance,exports:e,notes:new Set(),buttons:0,encoders:new Map()};
          this.engines.set(spec.id,engine);this.status[spec.id]='ready';loaded.push(spec.name);
        }catch(err){this.status[spec.id]='error: '+err.message;throw new Error(spec.name+': '+err.message);}
      }
      this.enabled=true;return loaded;
    }
    _mask(engine){
      let mask=0;
      for(const n of engine.notes){
        const key=n-KEY_MIN;
        if(key>=0&&key<KEY_COUNT)mask|=(1<<key);
      }
      return mask>>>0;
    }
    noteOn(note,velocity,channel=0){
      if(channel&&this.channel&&channel!==this.channel)return;
      const n=clamp(note|0,0,127);
      for(const e of this.engines.values()){
        e.notes.add(n);
        if(e.id===2 && typeof e.exports.web_key==='function')e.exports.web_key(n-KEY_MIN,1);
        else e.exports.web_keys(this._mask(e));
      }
    }
    noteOff(note,channel=0){
      if(channel&&this.channel&&channel!==this.channel)return;
      const n=note|0;
      for(const e of this.engines.values()){
        e.notes.delete(n);
        if(e.id===2 && typeof e.exports.web_key==='function')e.exports.web_key(n-KEY_MIN,0);
        else e.exports.web_keys(this._mask(e));
      }
    }
    allNotesOff(){
      for(const e of this.engines.values()){e.notes.clear();if(e.id===2&&typeof e.exports.web_panic==='function')e.exports.web_panic();else e.exports.web_keys(0);}
      this.held.clear();
      const z=this.engines.get(2);if(z&&typeof z.exports.web_panic==='function')z.exports.web_panic();
    }
    setMaster(percent){
      this.master=clamp(+percent||0,0,100);
      for(const e of this.engines.values())e.exports.web_master(Math.round(this.master/100*4096));
    }
    setButton(id,bit,down){
      const e=this.engines.get(id);if(!e)return;
      e.buttons=down?(e.buttons|(1<<bit)):(e.buttons&~(1<<bit));
      e.exports.web_buttons(e.buttons>>>0);
    }
    encoder(id,role,delta){
      const e=this.engines.get(id);if(e)e.exports.web_enc(role|0,delta|0);
    }
    setZenParameter(param,value){
      const z=this.engines.get(2);
      if(!z||typeof z.exports.web_param!=='function')return false;
      z.exports.web_param(param|0,value|0);return true;
    }
    setZenPatch(bytes){
      const z=this.engines.get(2);
      if(!z||typeof z.exports.web_patch!=='function'||typeof z.exports.web_patch_set!=='function')return false;
      const ptr=z.exports.web_patch()>>>0;
      new Uint8Array(z.exports.memory.buffer,ptr,128).set(bytes.subarray(0,128));
      z.exports.web_patch_set();return true;
    }
    renderToCore(coreExports,frames=128){
      const transferred=[];
      for(const e of this.engines.values()){
        const x=e.exports;
        x.web_render(frames);
        const lp=x.web_out_l()>>>0,rp=x.web_out_r()>>>0;
        const l=new Float32Array(x.memory.buffer,lp,frames);
        const r=new Float32Array(x.memory.buffer,rp,frames);
        const id=e.id,ptr=coreExports.hybrid_wasm_pcm_ptr(id)>>>0;
        const dst=new Int32Array(coreExports.memory.buffer,ptr,frames*2);
        for(let i=0;i<frames;i++){
          const a=clamp(Number.isFinite(l[i])?l[i]:0,-1,1);
          const b=clamp(Number.isFinite(r[i])?r[i]:0,-1,1);
          dst[i*2]=Math.round(a*32767);
          dst[i*2+1]=Math.round(b*32767);
        }
        const sourcePcm=new Float32Array(frames*2);for(let i=0;i<frames;i++){sourcePcm[i*2]=l[i];sourcePcm[i*2+1]=r[i];}transferred.push({id,name:e.name,sourcePcm,samples:dst.slice()});
        const rc=coreExports.hybrid_wasm_submit_pcm(id,frames);
        if(rc!==0)throw new Error(e.name+' PCM submit falhou: '+rc);
      }
      return transferred;
    }
  }
  window.HYBRIDNativeEngines={NativeEngineRack,SPECS,KEY_MIN,KEY_COUNT};
})();