/* Browser regression: DAW workstation, three original DSP WASM engines and 8-slot PCM sampler. */
'use strict';
const http=require('node:http'),fs=require('node:fs'),path=require('node:path');
const {chromium}=require('playwright');
const root=path.resolve(__dirname,'..','simulator');
const mime={'.html':'text/html; charset=utf-8','.js':'text/javascript; charset=utf-8','.wasm':'application/wasm'};
const server=http.createServer((req,res)=>{
 const pathname=decodeURIComponent(new URL(req.url,'http://127.0.0.1').pathname),file=path.resolve(root,'.'+pathname);
 if(pathname!=='/'&&!file.startsWith(root+path.sep)){res.writeHead(403);res.end('forbidden');return;}
 const target=pathname==='/'?path.join(root,'index.html'):file;
 fs.readFile(target,(err,data)=>{if(err){res.writeHead(404);res.end('not found');return;}res.writeHead(200,{'content-type':mime[path.extname(target)]||'application/octet-stream','cache-control':'no-store'});res.end(data);});
});
function wavSine(freq=220,duration=.5,rate=44100){
 const frames=Math.floor(rate*duration),data=Buffer.alloc(44+frames*2);
 data.write('RIFF',0);data.writeUInt32LE(36+frames*2,4);data.write('WAVE',8);data.write('fmt ',12);data.writeUInt32LE(16,16);
 data.writeUInt16LE(1,20);data.writeUInt16LE(1,22);data.writeUInt32LE(rate,24);data.writeUInt32LE(rate*2,28);
 data.writeUInt16LE(2,32);data.writeUInt16LE(16,34);data.write('data',36);data.writeUInt32LE(frames*2,40);
 for(let i=0;i<frames;i++)data.writeInt16LE(Math.round(Math.sin(2*Math.PI*freq*i/rate)*12000),44+i*2);
 return data;
}
(async()=>{
 let browser;
 try{
  await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
  browser=await chromium.launch({headless:true,args:['--no-sandbox','--disable-dev-shm-usage','--autoplay-policy=no-user-gesture-required']});
  const page=await browser.newPage(),errors=[];
  page.on('pageerror',e=>errors.push(e.message));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  const response=await page.goto('http://127.0.0.1:'+server.address().port+'/',{waitUntil:'networkidle'});
  if(!response||!response.ok())throw Error('DAW simulator failed to load');
  if(await page.locator('#dawWorkspace').count()!==1||await page.locator('#sampleFile7').count()!==1)throw Error('DAW arrangement or 8-slot sample browser missing');
  await page.locator('#dawAudio').click();
  await page.waitForFunction(()=>{const t=document.querySelector('#log')?.textContent||'';return t.includes('AudioWorklet activo:')||t.includes('ERRO WASM:');},null,{timeout:30000});
  let log=await page.locator('#log').textContent();
  if(!log.includes('AudioWorklet activo:'))throw Error('AudioWorklet failed: '+log+' | '+errors.join(' | '));
  await page.waitForFunction(()=>document.querySelector('#log')?.textContent.includes('DSP originais WASM activos'),null,{timeout:30000});
  // Load four real WAV files into four independent sampler slots.
  for(let slot=0;slot<4;slot++){
   await page.locator('#sampleFile'+slot).setInputFiles({name:'sample-'+slot+'.wav',mimeType:'audio/wav',buffer:wavSine(150+slot*70)});
   await page.waitForFunction(i=>window.__hybridSampleLoaded&&window.__hybridSampleLoaded[i],slot,{timeout:5000});
  }
  await page.locator('[data-sample-hit="0"]').click();
  // Route the sampler to the output and verify real PCM reaches the browser analyser.
  for(let i=0;i<4;i++)await page.locator('#engines input[data-i="'+i+'"][data-k="gain"]').evaluate((el,v)=>{el.value=String(v);el.dispatchEvent(new Event('input',{bubbles:true}));},i===3?100:0);
  await page.waitForFunction(()=>{
   const a=window.__hybridAudioProbe;if(!a)return false;const d=new Float32Array(a.fftSize);a.getFloatTimeDomainData(d);
   let sum=0;for(const x of d)sum+=x*x;return Math.sqrt(sum/d.length)>0.00001;
  },null,{timeout:10000});
  await page.waitForFunction(()=>window.__hybridAudioMetrics&&window.__hybridAudioMetrics.callbacks>=256,null,{timeout:15000});
  let metrics=await page.evaluate(()=>window.__hybridAudioMetrics);
  if(metrics.underruns!==0)throw Error('AudioWorklet underruns: '+JSON.stringify(metrics));
  if(metrics.samplesLoaded<4)throw Error('Sampler did not report four loaded samples: '+JSON.stringify(metrics));
  if(!Number.isFinite(metrics.renderMax)||!Number.isFinite(metrics.sequencerMax))throw Error('Invalid CPU timing metrics');
  console.log('CHROMIUM PCM SAMPLER PASS: four WAV imports, preview trigger, non-zero output, zero underruns.');
  // The arrangement clips drive the bounded family chain; four bars exercise Bass, Keys, Drum and Sample.
  await page.locator('#dawArrangement .clip[data-bar="7"]').first().click();
  for(const bar of [3,5,6,7])await page.locator('#dawArrangement .clip[data-bar="'+bar+'"]').first().click();
  await page.locator('#dawVariation').selectOption('0');await page.locator('#dawFill').uncheck();await page.locator('#dawApply').click();
  const base=await page.evaluate(()=>({steps:window.__hybridAudioMetrics.seqSteps,lanes:window.__hybridAudioMetrics.seqLaneEvents,triggers:window.__hybridAudioMetrics.sampleTriggers}));
  await page.locator('#dawPlay').click();
  await page.waitForFunction(({steps,lanes,triggers})=>{
   const m=window.__hybridAudioMetrics;
   return m&&m.seqSteps>=steps+50&&m.seqLaneEvents[0]>lanes[0]&&m.seqLaneEvents[1]>lanes[1]&&m.seqLaneEvents[2]>lanes[2]&&m.seqLaneEvents[3]>lanes[3]&&m.sampleTriggers>triggers;
  },base,{timeout:15000});
  await page.locator('#dawStop').click();
  await page.locator('#dawVariation').selectOption('1');await page.locator('#dawFill').check();
  await page.locator('[data-mute="1"]').click();await page.locator('[data-solo="2"]').click();await page.locator('#dawApply').click();
  const seqBase=await page.evaluate(()=>({steps:window.__hybridAudioMetrics.seqSteps,clocks:window.__hybridAudioMetrics.seqClockTicks}));
  await page.locator('#dawPlay').click();
  await page.waitForFunction(({steps,clocks})=>window.__hybridAudioMetrics&&window.__hybridAudioMetrics.seqSteps>=steps+2&&window.__hybridAudioMetrics.seqClockTicks>=clocks+6,seqBase,{timeout:12000});
  await page.locator('#dawStop').click();
  await page.locator('#dawClock').selectOption('midi');await page.locator('#dawPlay').click();
  const clockBase=await page.evaluate(()=>window.__hybridAudioMetrics.seqClockTicks);
  await page.evaluate(()=>{for(let i=0;i<6;i++)window.__hybridAudioNode.port.postMessage({type:'volcaExternalClock'});});
  await page.waitForFunction(v=>window.__hybridAudioMetrics&&window.__hybridAudioMetrics.seqClockTicks>=v+6,clockBase,{timeout:10000});
  await page.locator('#dawStop').click();
  log=await page.locator('#log').textContent();
  if(log.includes('AudioWorklet:'))throw Error('AudioWorklet runtime error: '+log);
  if(errors.length)throw Error('Browser errors: '+errors.join(' | '));
  metrics=await page.evaluate(()=>window.__hybridAudioMetrics);
  console.log('CHROMIUM DAW PASS: arrangement, shared pattern chain, variation B, fill, mute/solo, MIDI 24-PPQN transport.');
  console.log('CHROMIUM DAW METRICS: '+JSON.stringify(metrics));
 }finally{if(browser)await browser.close();await new Promise(resolve=>server.close(resolve));}
})().catch(e=>{console.error(e);process.exit(1);});
