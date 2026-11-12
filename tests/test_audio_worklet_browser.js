/* Browser regression: single DUB-1 DSP, chord voicings, no retired engines or Sample Browser. */
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
(async()=>{
 let browser;
 try{
  await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
  browser=await chromium.launch({headless:true,args:['--no-sandbox','--disable-dev-shm-usage','--autoplay-policy=no-user-gesture-required']});
  const page=await browser.newPage(),errors=[];
  page.on('pageerror',e=>errors.push(e.message));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  const response=await page.goto('http://127.0.0.1:'+server.address().port+'/',{waitUntil:'networkidle'});
  if(!response||!response.ok())throw Error('DAW simulator failed to load');
  if(await page.locator('#dawWorkspace').count()!==1)throw Error('SMK-37 workstation missing');
  if(await page.locator('#dawSampleBrowser,#sampleFile0,#dawShowSamples').count()!==0)throw Error('Sample Browser was not removed');
  if(await page.locator('#dawArrangement .track-row').count()!==1)throw Error('Only the DUB-1 arrangement lane should remain');
  const visible=await page.locator('body').innerText();
  if(/AMB-1|ZEN-1|SAMPLE BROWSER/i.test(visible))throw Error('Retired engines or Sample Browser remain visible');
  if(await page.locator('#patchBank option').count()!==1||await page.locator('#patchBank').inputValue()!=='1')throw Error('Only DUB-1 soundbank should be selectable');
  if(await page.locator('#patchCount').textContent()!=='8 presets/vozes')throw Error('DUB-1 source patch count mismatch');
  if(await page.locator('#patchGrid [data-patch]').count()!==8)throw Error('DUB-1 quick patch grid missing');
  if(await page.locator('#keys [data-n]').count()!==37)throw Error('SMK-37 keybed must expose exactly 37 keys');
  await page.locator('#dawAudio').click();
  await page.waitForFunction(()=>{const t=document.querySelector('#log')?.textContent||'';return t.includes('AudioWorklet activo:')||t.includes('AudioWorklet:');},null,{timeout:30000});
  let log=await page.locator('#log').textContent();
  if(!log.includes('AudioWorklet activo:'))throw Error('AudioWorklet failed: '+log+' | '+errors.join(' | '));
  await page.waitForFunction(()=>window.__hybridNativeRack&&window.__hybridNativeRack.engines.size===1,null,{timeout:15000});
  const rack=await page.evaluate(()=>({ids:[...window.__hybridNativeRack.engines.keys()],names:[...window.__hybridNativeRack.engines.values()].map(e=>e.name)}));
  if(JSON.stringify(rack.ids)!=='[1]'||JSON.stringify(rack.names)!=='["DUB-1"]')throw Error('Only DUB-1 should load: '+JSON.stringify(rack));
  const key=page.locator('#keys [data-n="60"]');
  await page.locator('#chordMode').check();
  let notes;
  const chordCases={maj:[0,4,7],min:[0,3,7],'7':[0,4,7,10],maj7:[0,4,7,11],m7:[0,3,7,10],sus2:[0,2,7],sus4:[0,5,7],dim:[0,3,6],dim7:[0,3,6,9],aug:[0,4,8],'6':[0,4,7,9],m6:[0,3,7,9],m7b5:[0,3,6,10],'9':[0,4,7,10,14]};
  for(const [type,intervals] of Object.entries(chordCases)){
    await page.locator('#chordType').selectOption(type);
    await key.hover();await page.mouse.down();
    notes=await page.evaluate(()=>[...window.__hybridNativeRack.engines.get(1).notes].sort((a,b)=>a-b));
    const expected=intervals.map(n=>60+n);
    if(JSON.stringify(notes)!==JSON.stringify(expected))throw Error(type+' chord notes incorrect: '+JSON.stringify(notes)+' expected '+JSON.stringify(expected));
    await page.mouse.up();
    const held=await page.evaluate(()=>window.__hybridNativeRack.engines.get(1).notes.size);
    if(held!==0)throw Error(type+' chord did not release every note');
  }
  await page.locator('#chordType').selectOption('dim7');
  await page.locator('#chord').click();
  await page.waitForFunction(()=>document.querySelector('#log')?.textContent.includes('Acorde C · Diminuto 7 · notas 60, 63, 66, 69'),null,{timeout:5000});
  await page.waitForFunction(()=>window.__hybridNativeRack.engines.get(1).notes.size===0,null,{timeout:2000});
  await page.locator('#chordMode').uncheck();
  await key.hover();await page.mouse.down();
  notes=await page.evaluate(()=>[...window.__hybridNativeRack.engines.get(1).notes].sort((a,b)=>a-b));
  if(JSON.stringify(notes)!=='[60]')throw Error('Single-note mode did not restore: '+JSON.stringify(notes));
  await page.mouse.up();
  await page.locator('#patchSelect').selectOption('2');
  await page.waitForFunction(()=>document.querySelector('#log')?.textContent.includes('DUB-1 · PRESET: Steppers (3/8)'),null,{timeout:5000});
  if((await page.locator('#patchState').textContent()).trim()!=='DUB-1 · Steppers · 3/8')throw Error('DUB-1 patch selection failed');
  await page.waitForFunction(()=>window.__hybridAudioMetrics&&window.__hybridAudioMetrics.callbacks>=256,null,{timeout:15000});
  const metrics=await page.evaluate(()=>window.__hybridAudioMetrics);
  if(metrics.underruns!==0)throw Error('AudioWorklet underruns: '+JSON.stringify(metrics));
  if(errors.length)throw Error('Browser errors: '+errors.join(' | '));
  console.log('CHROMIUM DUB-1 PASS: one original DSP loaded, 37-key routing, source patches and no AMB-1/ZEN-1.');
  console.log('CHROMIUM CHORD PASS: all 14 chord types, complete note release, selected chord trigger and single-note restore.');
  console.log('CHROMIUM CLEANUP PASS: AMB-1, ZEN-1 and Sample Browser absent from visible UI; audio underruns=0.');
  console.log('CHROMIUM METRICS: '+JSON.stringify(metrics));
 }finally{if(browser)await browser.close();await new Promise(resolve=>server.close(resolve));}
})().catch(e=>{console.error(e);process.exit(1);});
