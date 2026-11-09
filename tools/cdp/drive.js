(async()=>{
 const sleep=ms=>new Promise(r=>setTimeout(r,ms));
 const fire=el=>{for(const t of ["pointerdown","mousedown","pointerup","mouseup","click"])el.dispatchEvent(new MouseEvent(t,{bubbles:true,cancelable:true,view:window,button:0}));};
 const q=id=>document.querySelector('[data-ak-test-id="'+id+'"]');
 const waitFor=async(id,ms=15000)=>{const t0=Date.now();while(Date.now()-t0<ms){const e=q(id);if(e)return e;await sleep(200)}return null};
 const sel=async id=>{const w=await waitFor(id);if(!w)return null;return w.querySelector("[role=combobox]")||w.querySelector(".MuiSelect-select")||w};
 const opts=()=>[...document.querySelectorAll("[role=option]")];
 const openAndPick=async(id,match)=>{
   for(let a=0;a<3;a++){
     const e=await sel(id); if(!e) return "no "+id;
     fire(e);
     for(let i=0;i<25;i++){await sleep(250); const o=opts(); if(o.length){
        const hit=o.find(x=>x.innerText.trim().startsWith(match));
        if(!hit) return "opts["+o.map(x=>x.innerText.trim()).join("|")+"]";
        hit.scrollIntoView(); fire(hit); await sleep(1200); return "ok";
     }}
   }
   return "never opened "+id;
 };
 const L=[];
 if(!q("bundle-selector-group")){const w=await waitFor("wwise-icon");fire(w);await sleep(2500);L.push("nav wwise");}
 L.push("group:"+await openAndPick("bundle-selector-group","All"));
 await sleep(1500);
 L.push("gtext:"+(q("bundle-selector-group")?.innerText||"").trim().replace(/\s+/g," "));
 L.push("bundle:"+await openAndPick("bundle-selector-bundle","2023.1.14"));
 await sleep(1500);
 L.push("btext:"+(q("bundle-selector-bundle")?.innerText||"").trim().replace(/\s+/g," "));
 const ib=await waitFor("install-bundle"); if(!ib){L.push("no install-bundle");return L.join(" | ");}
 fire(ib);
 const nb=await waitFor("button-next",20000);
 if(!nb){L.push("no button-next; body="+document.body.innerText.slice(100,260));return L.join(" | ");}
 await sleep(6000);
 const b=q("button-next");
 const ti=[...document.querySelectorAll("input")].filter(i=>i.type!=="checkbox");
 return JSON.stringify({trace:L,
   next:{disabled:b.disabled,cls:b.className.match(/Mui-disabled/)?"Mui-disabled":"enabled",title:b.getAttribute("title")||b.parentElement?.getAttribute("aria-label")},
   textInputs:ti.map(i=>({name:i.name,value:i.value,invalid:i.getAttribute("aria-invalid")})),
   checked:[...document.querySelectorAll("input[type=checkbox]")].filter(c=>c.checked).length,
   helper:[...document.querySelectorAll(".MuiFormHelperText-root")].map(e=>e.innerText).filter(Boolean),
   errs:[...document.querySelectorAll(".Mui-error")].map(e=>e.innerText.slice(0,120)),
   body:document.body.innerText.slice(110,900)});
})()
