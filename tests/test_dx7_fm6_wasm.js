/* Native/WebAssembly parity smoke for the real MSFA six-operator DX7 core. */
'use strict';
const fs=require('node:fs');
const path=require('node:path');
global.window=global;
require('../simulator/dx7-factory-banks.js');
const banks=global.SMK37_DX7_FACTORY.banks;
const wasmBytes=fs.readFileSync(path.join(__dirname,'../simulator/dx7_fm6.wasm'));
(async()=>{
  const mod=new WebAssembly.Module(wasmBytes);
  const imports=WebAssembly.Module.imports(mod);
  if(imports.length)throw Error('DX7 standalone WASM must have no host imports: '+JSON.stringify(imports));
  const {instance}=await WebAssembly.instantiate(mod,{});
  const e=instance.exports;
  const ctors=e.__wasm_call_ctors||e._initialize;if(typeof ctors==='function')ctors();
  const fn=n=>{const f=e[n]||e['_'+n];if(typeof f!=='function')throw Error('Missing DX7 export '+n);return f};
  if(!(e.memory instanceof WebAssembly.Memory))throw Error('DX7 WASM memory export missing');
  if(fn('dx7_init')(44100)!==0)throw Error('DX7 init failed');
  const bankPtr=fn('dx7_bank_ptr')()>>>0,outputPtr=fn('dx7_output_ptr')()>>>0;
  let voicesChecked=0,bankChecks=[];
  for(let bankIndex=0;bankIndex<4;bankIndex++){
    const bank=banks[bankIndex],raw=Buffer.from(bank.data,'base64');
    if(raw.length!==4096||bank.voices.length!==32)throw Error('Malformed DX7 bank '+bank.name);
    new Uint8Array(e.memory.buffer,bankPtr,4096).set(raw);
    if(fn('dx7_load_bank')()!==0)throw Error('Bank load failed: '+bank.name);
    let audible=0;
    for(let program=0;program<32;program++){
      if(fn('dx7_select_program')(program)!==0)throw Error('Program select failed '+bank.name+':'+program);
      fn('dx7_panic')();
      fn('dx7_midi')(0x90,60,110);
      let peak=0;
      for(let block=0;block<176;block++){
        if(fn('dx7_render')(128)!==0)throw Error('DX7 render failed');
        const pcm=new Int16Array(e.memory.buffer,outputPtr,128);
        for(let i=0;i<pcm.length;i++)peak=Math.max(peak,Math.abs(pcm[i]));
      }
      fn('dx7_midi')(0x80,60,0);
      if(peak>0)audible++;
      else throw Error('Silent DX7 voice '+bank.name+' '+(program+1)+' '+bank.voices[program]);
      voicesChecked++;
    }
    bankChecks.push({bank:bank.name,voices:audible});
  }
  fn('dx7_panic')();
  console.log('MSFA DX7 FM6 WASM PASS:',JSON.stringify({voicesChecked,bankChecks,imports:imports.length}));
})().catch(err=>{console.error(err);process.exit(1);});
