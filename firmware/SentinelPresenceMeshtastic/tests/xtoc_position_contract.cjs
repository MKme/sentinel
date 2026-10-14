// Use the existing local XTOC decoder; no duplicated JavaScript codec or new package.
const assert=require('node:assert/strict');
const fs=require('node:fs');
const {tsImport}=require('C:/GitHub/XTOC/xtoc-web/node_modules/tsx/dist/esm/api/index.cjs');
(async()=>{
 const codec=await tsImport('file:///C:/GitHub/XTOC/xtoc-web/src/core/packet.ts',__filename);
 const text=fs.readFileSync(process.argv[2],'utf8').split(/\r?\n/).find(s=>s.startsWith('X1.11.'));
 assert.ok(text,'native firmware encoder fixture required');
 const packet=codec.parsePacket(text);assert.ok(packet);assert.equal(packet.templateId,11);assert.equal(packet.mode,'C');
 const decoded=codec.decodeSentinelClear(packet.payload);
 assert.deepEqual(decoded,{nodeId:0x10203040,t:Math.floor(1791972001/60)*60000,lat:-33.86514,lon:151.2099,label:'WRAITH-01',sensors:[]});
 assert.equal(codec.encodeSentinelClear(decoded),packet.payload);
 console.log('PASS: native firmware T=11 wrapper parsed and round-tripped by actual XTOC codec');
})().catch(e=>{console.error(e);process.exit(1)});
