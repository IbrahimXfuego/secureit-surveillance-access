import http from 'node:http';
import {readFile,writeFile,mkdir} from 'node:fs/promises';
import {randomInt,timingSafeEqual} from 'node:crypto';
import {fileURLToPath} from 'node:url';
import path from 'node:path';
const root=path.dirname(fileURLToPath(import.meta.url));
export function createGateway(c,{telegram,device,now=Date.now}={}){
 let challenge=null,logs=[],saving=Promise.resolve(),motionBusy=false,lastMotion=-Infinity;
 const seen=new Set();
 telegram??=async(method,data)=>{
  const form=data instanceof FormData;
  const r=await fetch(`https://api.telegram.org/bot${c.token}/${method}`,{method:'POST',headers:form?{}:{'Content-Type':'application/json'},body:form?data:JSON.stringify(data),signal:AbortSignal.timeout(40000)});
  const result=await r.json();if(!r.ok||!result.ok)throw Error('Telegram request failed');return result.result;
 };
 device??=async(board,route,options={})=>{
  const r=await fetch(`${board==='camera'?c.camera:c.access}${route}`,{...options,headers:{...options.headers,'X-Device-Key':c.key},signal:AbortSignal.timeout(12000)});
  if(!r.ok)throw Error('Device request failed');return r;
 };
 const tell=text=>telegram('sendMessage',{chat_id:c.chat,text});
 const authorized=value=>{const a=Buffer.from(value??''),b=Buffer.from(c.key);return a.length===b.length&&timingSafeEqual(a,b)};
 async function record(event){
  logs.unshift({event,timestamp:new Date(now()).toISOString()});logs=logs.slice(0,100);
  if(c.dataDir){const snapshot=JSON.stringify(logs,null,2);saving=saving.catch(()=>{}).then(async()=>{await mkdir(c.dataDir,{recursive:true});await writeFile(path.join(c.dataDir,'logs.json'),snapshot)});await saving}
 }
 async function photo(flash=false){
  const r=await device('camera',`/photo?flash=${flash?1:0}`);const bytes=await r.arrayBuffer();
  const form=new FormData();form.set('chat_id',c.chat);form.set('photo',new Blob([bytes],{type:'image/jpeg'}),'secureit.jpg');await telegram('sendPhoto',form);
 }
 async function handleUpdate(update){
  const q=update.callback_query,m=q?.message??update.message,user=q?.from??m?.from;
  if(String(m?.chat?.id)!==c.chat||String(user?.id)!==c.user)return;
  if(q){
   await telegram('answerCallbackQuery',{callback_query_id:q.id});
   const match=/^pin:([a-f0-9]{16}):([0-9]{3})$/.exec(q.data??'');if(!match)return;
   const [,id,choice]=match;
   if(!challenge||id!==challenge.id||now()>=challenge.expires)return tell('Challenge expired or already answered. Press the button again.');
   const granted=Number(choice)===challenge.pin;challenge=null;
   await device('access',`/decision?id=${id}&grant=${granted?1:0}`,{method:'POST'});
   await record(granted?'Access Granted':'Access Denied');
   return tell(granted?'Access Granted — unlocking for 5 seconds.':'Access Denied');
  }
  const cmd=(m?.text??'').split(/\s/)[0].split('@')[0];
  switch(cmd){
   case '/start':case '/help':return telegram('sendMessage',{chat_id:c.chat,text:'SecureIT controls. Press the physical button for an LCD number challenge.',reply_markup:{keyboard:[['/photo','/photoWithFlash'],['/lightOn','/lightOff'],['/temperature','/stream'],['/motionOn','/motionOff']],resize_keyboard:true}});
   case '/photo':return photo();
   case '/photoWithFlash':return photo(true);
   case '/lightOn':case '/lightOff':await device('camera',`/relay?on=${cmd==='/lightOn'?1:0}`,{method:'POST'});return tell(cmd==='/lightOn'?'Bulb on.':'Bulb off.');
   case '/motionOn':case '/motionOff':await device('camera',`/motion?on=${cmd==='/motionOn'?1:0}`,{method:'POST'});return tell(cmd==='/motionOn'?'Motion alerts enabled.':'Motion alerts disabled.');
   case '/temperature':case '/temp':{const r=await device('camera','/temperature'),d=await r.json();return tell(`Temperature: ${d.temperature} °C\nHumidity: ${d.humidity}%`)}
   case '/stream':return tell(`Live stream (same LAN/VPN): ${c.camera}:81/stream\nDashboard: ${c.base}`);
   default:if(cmd)return tell('Use /help for controls.');
  }
 }
 const json=(res,code,data)=>{res.writeHead(code,{'Content-Type':'application/json','Cache-Control':'no-store'});res.end(JSON.stringify(data))};
 async function body(req){let s='';for await(const chunk of req){s+=chunk;if(s.length>4096)throw Error('Body too large')}return JSON.parse(s||'{}')}
 const server=http.createServer(async(req,res)=>{
  try{
   const u=new URL(req.url,'http://localhost');
   if(u.pathname.startsWith('/api/device/')){
    if(!authorized(req.headers['x-device-key']))return json(res,401,{error:'Unauthorized'});
    if(req.method!=='POST')return json(res,405,{error:'POST required'});
    const d=await body(req);
    if(u.pathname==='/api/device/challenge'){
     if(!/^[a-f0-9]{16}$/.test(d.id??'')||!Number.isInteger(d.pin)||d.pin<100||d.pin>999)return json(res,400,{error:'Invalid challenge'});
     if(seen.has(d.id))return json(res,409,{error:'Duplicate challenge'});
     seen.add(d.id);if(seen.size>1000)seen.delete(seen.values().next().value);
     const options=[d.pin];while(options.length<3){const n=randomInt(100,1000);if(!options.includes(n))options.push(n)}
     for(let i=2;i>0;i--){const j=randomInt(i+1);[options[i],options[j]]=[options[j],options[i]]}
     challenge={id:d.id,pin:d.pin,expires:now()+55000};
     try{await telegram('sendMessage',{chat_id:c.chat,text:'Select the number shown on the LCD. Expires in about 55 seconds.',reply_markup:{inline_keyboard:[options.map(n=>({text:String(n),callback_data:`pin:${d.id}:${n}`}))]}})}catch(e){challenge=null;throw e}
     return json(res,200,{ok:true});
    }
    if(u.pathname==='/api/device/motion'){
     json(res,202,{ok:true});
     // Acknowledge first, then fetch the photo so the camera can finish its HTTP POST.
     if(!motionBusy&&now()-lastMotion>=10000){motionBusy=true;lastMotion=now();setImmediate(async()=>{try{await tell('Motion detected.');await photo()}catch{console.error('Motion photo failed; check camera connection.')}finally{motionBusy=false}})}
     return;
    }
    return json(res,404,{error:'Not found'});
   }
   if(req.method!=='GET')return json(res,405,{error:'GET required'});
   if(u.pathname==='/api/logs')return json(res,200,logs);
   if(u.pathname==='/api/config')return json(res,200,{stream:`${c.camera}:81/stream`});
   if(u.pathname==='/'){const html=await readFile(path.join(root,'public/index.html'));res.writeHead(200,{'Content-Type':'text/html; charset=utf-8'});return res.end(html)}
   json(res,404,{error:'Not found'});
  }catch{if(!res.headersSent)json(res,502,{error:'Operation failed; check device/server configuration.'});else res.end()}
 });
 async function loadLogs(){if(!c.dataDir)return;try{const saved=JSON.parse(await readFile(path.join(c.dataDir,'logs.json'),'utf8'));if(!Array.isArray(saved))throw Error('Invalid logs');logs=saved.slice(0,100)}catch(e){if(e.code!=='ENOENT')throw Error('Cannot read log storage')}}
 async function poll(signal){let offset=0;while(!signal.aborted){try{const updates=await telegram('getUpdates',{offset,timeout:25,allowed_updates:['message','callback_query']});for(const update of updates){offset=update.update_id+1;try{await handleUpdate(update)}catch{await tell('Request failed. Check device connectivity and retry.').catch(()=>{})}}}catch{if(!signal.aborted){console.error('Telegram polling failed; retrying. Run one gateway only and remove any existing webhook.');await new Promise(r=>setTimeout(r,3000))}}}}
 return{server,handleUpdate,poll,loadLogs};
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
 const e=process.env;for(const name of ['BOT_TOKEN','OWNER_CHAT_ID','OWNER_USER_ID','DEVICE_KEY','CAMERA_URL','ACCESS_URL','PUBLIC_BASE_URL'])if(!e[name]||/REPLACE|YOUR_/.test(e[name]))throw Error(`Configure ${name} in .env`);
 const c={token:e.BOT_TOKEN,chat:e.OWNER_CHAT_ID,user:e.OWNER_USER_ID,key:e.DEVICE_KEY,camera:e.CAMERA_URL.replace(/\/$/,''),access:e.ACCESS_URL.replace(/\/$/,''),base:e.PUBLIC_BASE_URL,dataDir:path.join(root,'data')};
 const app=createGateway(c);await app.loadLogs();const stop=new AbortController();
 app.server.listen(Number(e.PORT||3000),'0.0.0.0',()=>console.log('SecureIT gateway ready. Open your configured dashboard URL.'));app.poll(stop.signal);
 for(const event of ['SIGINT','SIGTERM'])process.on(event,()=>{stop.abort();app.server.close()});
}
