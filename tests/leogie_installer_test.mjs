import assert from 'node:assert/strict';
import {prepareFiles,flashVerified,sha256,EXPECTED} from '../tools/leogie-installer/flash-plan.mjs';
const buffers=new Map([...EXPECTED.keys()].map(k=>[k,new Uint8Array([1,2,3,4])]));
const manifest={chip:'ESP32-C3',flash_size:0x800000,erase_all:false,segments:[]};
for(const [file,[address]] of EXPECTED)manifest.segments.push({file,address,size:4,sha256:await sha256(buffers.get(file))});
const read=async name=>buffers.get(name);
const files=await prepareFiles(manifest,read);assert.equal(files.length,3);
for(const mutate of [m=>m.erase_all=true,m=>m.segments[2].address=0x356000,m=>m.segments[2].size=0x400000,
 m=>m.segments[0].sha256='bad',m=>m.segments[0].file='../bootloader.bin',m=>m.segments.push(m.segments[0])]) {
 const bad=structuredClone(manifest);mutate(bad);await assert.rejects(()=>prepareFiles(bad,read));
}
let wrote=false,reads=0;
const loader={chip:{CHIP_NAME:'ESP32-C3'},detectFlashSize:async()=>'8MB',
 readFlash:async(a,n)=>{assert.equal(a,0x356000);assert.equal(n,0x4000);reads++;return new Uint8Array(n).fill(123);},
 writeFlash:async options=>{assert.equal(options.eraseAll,false);assert.equal(options.fileArray,files);wrote=true;}};
await flashVerified(loader,files,()=>'',()=>{});assert(wrote&&reads===2);
loader.detectFlashSize=async()=>'4MB';wrote=false;
await assert.rejects(()=>flashVerified(loader,files,()=>'',()=>{}));assert(!wrote);
loader.detectFlashSize=async()=>'8MB';reads=0;
loader.readFlash=async(a,n)=>new Uint8Array(n).fill(++reads);
await assert.rejects(()=>flashVerified(loader,files,()=>'',()=>{}));
console.log('Installer rejects unsafe manifests, wrong hardware, and altered identity; segmented write never erases all.');
