const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const {pathToFileURL}=require('node:url');
const xtoc=process.env.XTOC_WORKSPACE || path.resolve(__dirname,'../../../../XTOC');
const {tsImport}=require(path.join(xtoc,'xtoc-web/node_modules/tsx/dist/esm/api/index.cjs'));
(async()=>{
 const codec=await tsImport(pathToFileURL(path.join(xtoc,'xtoc-web/src/core/packet.ts')).href,__filename);
 const lines=fs.readFileSync(process.argv[2],'utf8').trim().split(/\r?\n/);
 assert.equal(lines.length,4);
 for(const [i,text] of lines.entries()){
   assert.ok(Buffer.byteLength(text)<180);
   const p=codec.parsePacket(text),s=codec.decodeSentinelClear(p.payload);
   assert.equal(s.nodeId,0x10203040);assert.equal(s.label,'OBS FIRE');assert.equal(s.alert,true);
   assert.equal(s.observation.id,0x12345678);assert.equal(s.observation.confidence,92);
   assert.equal(s.locationState,['none','fresh','stale','stale'][i]);
   if(!i){assert.ok(Number.isNaN(s.lat));assert.equal(s.t,0);assert.equal(s.timeUnknown,true);}
   else {assert.equal(s.lat,45);assert.equal(s.lon,-80);}
   assert.equal(codec.encodeSentinelClear(s),p.payload);
 }
 console.log('PASS four native observation packets through actual XTOC codec');
})().catch(e=>{console.error(e);process.exit(1)});
