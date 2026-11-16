import {test} from 'node:test';
import assert from 'node:assert/strict';
import {createGateway} from './server.mjs';
async function fixture(t){
 const sent=[],actions=[];let clock=100000;
 const app=createGateway({chat:'10',user:'20',key:'test-device-key',camera:'http://camera',access:'http://access',base:'http://gateway'}, {
  telegram:async(method,data)=>{sent.push({method,data});return true},
  device:async(board,route,options)=>{actions.push({board,route,options});return new Response(JSON.stringify({temperature:26,humidity:60}))},now:()=>clock
 });
 await new Promise(r=>app.server.listen(0,'127.0.0.1',r));t.after(()=>new Promise(r=>app.server.close(r)));
 const url=`http://127.0.0.1:${app.server.address().port}`;
 const challenge=async(id='0123456789abcdef')=>fetch(url+'/api/device/challenge',{method:'POST',headers:{'Content-Type':'application/json','X-Device-Key':'test-device-key'},body:JSON.stringify({id,pin:321})});
 const answer=(choice,id='0123456789abcdef',user='20')=>app.handleUpdate({callback_query:{id:'query',from:{id:user},message:{chat:{id:'10'}},data:`pin:${id}:${choice}`}});
 return{app,sent,actions,url,challenge,answer,advance:ms=>clock+=ms};
}
test('valid challenge has three distinct choices and correct answer unlocks once',async t=>{
 const f=await fixture(t);assert.equal((await f.challenge()).status,200);
 const buttons=f.sent.at(-1).data.reply_markup.inline_keyboard[0];assert.equal(new Set(buttons.map(b=>b.text)).size,3);assert(buttons.some(b=>b.text==='321'));
 await f.answer('321');await f.answer('321');assert.equal(f.actions.length,1);assert.match(f.actions[0].route,/grant=1/);
 const logs=await(await fetch(f.url+'/api/logs')).json();assert.equal(logs[0].event,'Access Granted');
});
test('incorrect selection denies access and logs it',async t=>{const f=await fixture(t);await f.challenge();await f.answer('123');assert.match(f.actions[0].route,/grant=0/);const logs=await(await fetch(f.url+'/api/logs')).json();assert.equal(logs[0].event,'Access Denied')});
test('wrong sender and expired challenges cannot operate lock',async t=>{const f=await fixture(t);await f.challenge();await f.answer('321',undefined,'99');assert.equal(f.actions.length,0);f.advance(56000);await f.answer('321');assert.equal(f.actions.length,0)});
test('duplicate and malformed challenge requests are rejected',async t=>{const f=await fixture(t);await f.challenge();assert.equal((await f.challenge()).status,409);assert.equal((await f.challenge('bad')).status,400)});
test('device key required; root dashboard and config served correctly',async t=>{const f=await fixture(t);assert.equal((await fetch(f.url+'/api/device/motion',{method:'POST'})).status,401);const html=await(await fetch(f.url)).text();assert(html.indexOf('Access history')<html.indexOf('Live camera'));assert.equal((await(await fetch(f.url+'/api/config')).json()).stream,'http://camera:81/stream')});
test('owner-only camera commands route to correct device',async t=>{const f=await fixture(t);const msg=user=>({message:{chat:{id:'10'},from:{id:user},text:'/lightOn'}});await f.app.handleUpdate(msg('99'));assert.equal(f.actions.length,0);await f.app.handleUpdate(msg('20'));assert.equal(f.actions[0].board,'camera');assert.equal(f.actions[0].route,'/relay?on=1')});
