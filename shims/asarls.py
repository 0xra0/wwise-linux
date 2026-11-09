import json,struct,sys,os
p="/mnt/sata/wwise/prefix/drive_c/Program Files/Wwise Launcher/resources/app.asar"
f=open(p,'rb')
# pickle-ish header: uint32 4, uint32 headerpicklesize, uint32 4? standard: 16 bytes then json
d=f.read(16)
sz=struct.unpack('<I',d[12:16])[0]
hdr=json.loads(f.read(sz).decode('utf8'))
base=16+sz
base=(base+3)//4*4 if False else base
def walk(node,path=''):
    for k,v in node.get('files',{}).items():
        pp=path+'/'+k
        if 'files' in v: yield from walk(v,pp)
        else: yield pp,int(v.get('offset',-1)),v.get('size',0),v.get('unpacked',False)
ents=list(walk(hdr))
if len(sys.argv)>1 and sys.argv[1]=='get':
    tgt=sys.argv[2]
    for pp,off,size,unp in ents:
        if pp==tgt:
            f.seek(base+off); open(sys.argv[3],'wb').write(f.read(size)); print("wrote",size); break
else:
    for pp,off,size,unp in sorted(ents,key=lambda e:-e[2])[:30]:
        print(f"{size:>10}  {pp}")
