/* ES5 bootstrap deliberately runs even when modern game syntax is unsupported. */
(function(){
  var report={schema:1,build:'0.2.0',userAgent:navigator.userAgent,results:{},payload:window.WUMINGSHA_PAYLOAD||{included:false}};
  var tests=[['dom','DOM 创建与事件'],['css','CSS 布局'],['promise','Promise'],['map','Map / Set'],['proxy','Proxy / Reflect'],['syntax','现代 JavaScript 语法'],['module','原生 ES 模块'],['dynamicImport','动态 import()'],['fetch','相对资源 fetch'],['localStorage','本地存储读写'],['indexedDB','IndexedDB 写入并重开读取'],['worker','Blob Worker'],['webAudio','WebAudio API'],['audioPlayback','手动音频播放请求']];
  function render(){
    var table=document.getElementById('results'),html='',passed=0,failed=0;
    for(var i=0;i<tests.length;i++){var r=report.results[tests[i][0]],state=r?(r.ok?'通过':'未通过'):'等待';if(r){if(r.ok)passed++;else failed++;}html+='<tr><td>'+tests[i][1]+'</td><td class="'+(r?(r.ok?'good':'bad'):'wait')+'">'+state+(r?' · '+r.note:'')+'</td></tr>';}
    table.innerHTML=html;document.getElementById('summary').textContent='通过 '+passed+' 项，未通过 '+failed+' 项。';document.getElementById('report').value=JSON.stringify(report,null,2);
  }
  function mark(key,ok,note){report.results[key]={ok:!!ok,note:String(note||'')};render();}
  window.WUMINGSHA_MARK=mark;
  function run(key,test){try{mark(key,test(),'实际调用成功');}catch(e){mark(key,false,e.name||'调用失败');}}
  run('dom',function(){var e=document.createElement('div');e.addEventListener('click',function(){});return !!e.style;});
  run('css',function(){var e=document.createElement('div');e.style.display='flex';return e.style.display==='flex';});
  run('promise',function(){return typeof Promise==='function'&&typeof Promise.resolve==='function';});
  run('map',function(){return new Map().set('a',1).get('a')===1&&new Set([1]).has(1);});
  run('proxy',function(){return new Proxy({x:2},{}).x===2&&Reflect.get({x:2},'x')===2;});
  run('syntax',function(){new Function('const f=async()=>{let a={x:1};return a?.x ?? 0;}; return f;')();return true;});
  run('localStorage',function(){var before=localStorage.getItem('wumingsha_probe');localStorage.setItem('wumingsha_probe','verified');report.storageSeenFromEarlierRun=before==='verified';return localStorage.getItem('wumingsha_probe')==='verified';});
  try{fetch('sample.json').then(function(r){return r.json();}).then(function(r){mark('fetch',r.wumingsha===true,'相对 JSON 资源读取');},function(){mark('fetch',false,'资源请求失败');});}catch(e){mark('fetch',false,e.name);}
  try{
    var dbName='wumingsha_probe_v1',open=indexedDB.open(dbName,1);
    open.onupgradeneeded=function(){open.result.createObjectStore('test');};
    open.onerror=function(){mark('indexedDB',false,'打开失败');};
    open.onsuccess=function(){var db=open.result,tx=db.transaction('test','readwrite');tx.objectStore('test').put('ok','value');tx.onerror=function(){mark('indexedDB',false,'写入失败');};tx.oncomplete=function(){db.close();var again=indexedDB.open(dbName,1);again.onerror=function(){mark('indexedDB',false,'重开失败');};again.onsuccess=function(){var db2=again.result,q=db2.transaction('test').objectStore('test').get('value');q.onsuccess=function(){mark('indexedDB',q.result==='ok','实际写入、关闭、重开及读取');db2.close();};q.onerror=function(){mark('indexedDB',false,'读取失败');db2.close();};};};};
  }catch(e){mark('indexedDB',false,e.name);}
  try{var url=URL.createObjectURL(new Blob(['postMessage("ok");'],{type:'application/javascript'})),worker=new Worker(url);worker.onmessage=function(e){mark('worker',e.data==='ok','Worker 消息往返');worker.terminate();URL.revokeObjectURL(url);};worker.onerror=function(){mark('worker',false,'Worker 执行失败');worker.terminate();URL.revokeObjectURL(url);};}catch(e){mark('worker',false,e.name);}
  run('webAudio',function(){return typeof (window.AudioContext||window.webkitAudioContext)==='function';});
  document.getElementById('sound').onclick=function(){try{var audio=new Audio('tone.wav'),p=audio.play();if(p&&p.then)p.then(function(){mark('audioPlayback',true,'播放请求获接受；请确认是否听到声音');},function(){mark('audioPlayback',false,'播放被拒绝');});else mark('audioPlayback',true,'已发起播放；请确认是否听到声音');}catch(e){mark('audioPlayback',false,e.name);}};
  var included=report.payload.included;document.getElementById('payload').textContent=included?'已附完整原包，可点击启动。版本：'+report.payload.commit:'本检测 NSP 未附约 GB 级原包；完整包导入与构建脚本已另行提供。';
  if(!included){document.getElementById('game').removeAttribute('href');document.getElementById('game').onclick=function(){return false;};}
  document.getElementById('export').onclick=function(){location.href='http://localhost/wumingsha-probe?report='+encodeURIComponent(JSON.stringify(report));};
  setTimeout(function(){for(var i=0;i<tests.length;i++){var k=tests[i][0];if(!report.results[k]&&k!=='audioPlayback')mark(k,false,'7秒内未执行完成');}},7000);
  render();
}());
