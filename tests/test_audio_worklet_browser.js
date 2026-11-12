/* Chromium regression: real MSFA FM6/DX7 audio, four banks, chords and no DUB-1. */
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
  if(await page.locator('#dawSampleBrowser,#sampleFile0,#dawShowSamples').count()!==0)throw Error('Sample Browser UI reappeared');
  if(await page.locator('#dawArrangement .track-row').count()!==1)throw Error('Expected one FM6/DX7 arrangement lane');
  const visible=await page.locator('body').innerText();
  if(/DUB-1|AMB-1|ZEN-1|SAMPLE BROWSER/i.test(visible))throw Error('Removed DSP or Sample Browser remains visible');
  if(!visible.includes('FM6/DX7'))throw Error('FM6/DX7 engine not visible');
  if(await page.locator('#dx7BankSelect option').count()!==4)throw Error('DX7 must expose four banks');
  for(let bank=0;bank<4;bank++){
    await page.locator('#dx7BankSelect').selectOption(String(bank));
    if(await page.locator('#dx7VoiceSelect option').count()!==32)throw Error('DX7 bank '+bank+' must contain 32 voices');
    const names=await page.locator('#dx7VoiceSelect option').allTextContents();
    if(names.some(x=>!x.trim()))throw Error('DX7 voice name missing');
  }
  if(!/FM-1_015/.test(await page.locator('#dx7SourceTag').textContent()))throw Error('DX7 bank provenance missing');
  await page.locator('#dx7VoiceSelect').selectOption('13');
  await page.locator('#dawAudio').click();
  await page.waitForFunction(()=>{const t=document.querySelector('#log')?.textContent||'';return t.includes('AudioWorklet activo:')||t.includes('AudioWorklet:');},null,{timeout:30000});
  let log=await page.locator('#log').textContent();
  if(!log.includes('AudioWorklet ativo: FM6/DX7'))throw Error('FM6/DX7 AudioWorklet failed: '+log+' | '+errors.join(' | '));
  for(let bank=0;bank<4;bank++){await page.locator('#dx7BankSelect').selectOption(String(bank));await page.waitForFunction(expected=>window.__hybridDx7Status?.bank===expected,bank,{timeout:5000});}
  await page.locator('#dx7VoiceSelect').selectOption('13');await page.waitForFunction(()=>window.__hybridDx7Status?.program===13,null,{timeout:5000});
  await page.waitForFunction(()=>window.__hybridAudioMetrics&&window.__hybridAudioMetrics.callbacks>=256,null,{timeout:15000});
  const key=page.locator('#keys [data-n="60"]');
  await key.hover();await page.mouse.down();
  await page.waitForFunction(()=>window.__hybridDx7Notes?.includes(60),null,{timeout:5000});
  await page.waitForFunction(()=>window.__hybridAudioMetrics?.dx7Peak>0,null,{timeout:10000});
  const audible=await page.evaluate(()=>({peak:window.__hybridAudioMetrics.dx7Peak,notes:window.__hybridDx7Notes.slice()}));
  if(audible.peak<=0||!audible.notes.includes(60))throw Error('DX7 note did not produce non-zero PCM: '+JSON.stringify(audible));
  await page.mouse.up();
  await page.waitForFunction(()=>window.__hybridDx7Notes?.length===0,null,{timeout:5000});
  await page.locator('#chordMode').check();
  const chordCases={maj:[0,4,7],min:[0,3,7],'7':[0,4,7,10],maj7:[0,4,7,11],m7:[0,3,7,10],sus2:[0,2,7],sus4:[0,5,7],dim:[0,3,6],dim7:[0,3,6,9],aug:[0,4,8],'6':[0,4,7,9],m6:[0,3,7,9],m7b5:[0,3,6,10],'9':[0,4,7,10,14]};
  for(const [type,intervals] of Object.entries(chordCases)){
    await page.locator('#chordType').selectOption(type);
    await key.hover();await page.mouse.down();
    const expected=intervals.map(n=>60+n).sort((a,b)=>a-b);
    await page.waitForFunction(want=>JSON.stringify([...(window.__hybridDx7Notes||[])].sort((a,b)=>a-b))===JSON.stringify(want),expected,{timeout:5000});
    await page.mouse.up();
    await page.waitForFunction(()=>window.__hybridDx7Notes?.length===0,null,{timeout:5000});
  }
  await page.locator('#chordType').selectOption('dim7');
  await page.locator('#chord').click();
  await page.waitForFunction(()=>document.querySelector('#log')?.textContent.includes('Acorde C · Diminuto 7'),null,{timeout:5000});
  await page.waitForFunction(()=>window.__hybridDx7Notes?.length===0,null,{timeout:5000});
  await page.locator('#chordMode').uncheck();
  await key.hover();await page.mouse.down();
  await page.waitForFunction(()=>JSON.stringify(window.__hybridDx7Notes||[])==='[60]',null,{timeout:5000});
  await page.mouse.up();
  await page.waitForFunction(()=>window.__hybridDx7Notes?.length===0,null,{timeout:5000});
  await page.locator('#dx7BankSelect').selectOption('3');
  await page.locator('#dx7VoiceSelect').selectOption('31');
  await page.waitForFunction(()=>window.__hybridDx7Status?.bank===3&&window.__hybridDx7Status?.program===31,null,{timeout:5000});
  await page.locator('#dx7BankSelect').selectOption('0');
  const [download]=await Promise.all([page.waitForEvent('download'),page.locator('#dx7DownloadBank').click()]);
  const bytes=fs.readFileSync(await download.path());
  if(bytes.length!==4104||bytes[0]!==0xF0||bytes[1]!==0x43||bytes[2]!==0||bytes[3]!==9||bytes[4]!==0x20||bytes[5]!==0||bytes[4103]!==0xF7)throw Error('DX7 SysEx bank framing/length invalid: '+bytes.length);
  let checksum=0;for(let i=6;i<4103;i++)checksum=(checksum+bytes[i])&127;
  if(checksum!==0)throw Error('DX7 SysEx checksum invalid: '+checksum);
  const metrics=await page.evaluate(()=>window.__hybridAudioMetrics);
  if(metrics.underruns!==0)throw Error('AudioWorklet underruns: '+JSON.stringify(metrics));
  if(errors.length)throw Error('Browser errors: '+errors.join(' | '));
  console.log('CHROMIUM FM6/DX7 PASS: MSFA WASM engine, real non-zero DX7 PCM, four banks and program selection.');
  console.log('CHROMIUM CHORD PASS: all 14 chord types, note-off, chord trigger and single-note restore.');
  console.log('CHROMIUM CLEANUP PASS: DUB-1, AMB-1, ZEN-1 and Sample Browser absent; AudioWorklet underruns=0.');
  console.log('CHROMIUM METRICS: '+JSON.stringify(metrics));
 }finally{if(browser)await browser.close();await new Promise(resolve=>server.close(resolve));}
})().catch(e=>{console.error(e);process.exit(1);});
