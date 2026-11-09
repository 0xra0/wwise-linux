#!/usr/bin/env python3
"""Evaluate JS in the launcher's renderer over CDP. Usage: cdp.py '<expr>'"""
import json,sys,urllib.request,websocket
tg=[t for t in json.load(urllib.request.urlopen("http://127.0.0.1:9222/json")) if t["type"]=="page"][0]
ws=websocket.create_connection(tg["webSocketDebuggerUrl"],suppress_origin=True,timeout=180)
expr=sys.argv[1] if len(sys.argv)>1 else sys.stdin.read()
ws.send(json.dumps({"id":1,"method":"Runtime.evaluate",
    "params":{"expression":expr,"returnByValue":True,"awaitPromise":True}}))
while True:
    m=json.loads(ws.recv())
    if m.get("id")==1: break
r=m.get("result",{})
if "exceptionDetails" in r: print("EXCEPTION:",json.dumps(r["exceptionDetails"])[:800])
v=r.get("result",{})
print(json.dumps(v.get("value",v.get("description")),indent=1,ensure_ascii=False)[:6000])
ws.close()
