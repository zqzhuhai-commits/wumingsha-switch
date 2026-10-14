#!/usr/bin/env python3
"""Copy the complete tracked upstream package unchanged; exclude Git metadata only."""
import argparse,hashlib,json,shutil,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--upstream',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
source=a.upstream.resolve();output=a.output.resolve()
if output==source or source in output.parents:raise SystemExit('Output must be outside the upstream source')
if output.exists():raise SystemExit('Output already exists; choose a new staging directory')
names=subprocess.check_output(['git','-C',str(source),'ls-files','-z']).decode().split('\0')
commit=subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip()
if subprocess.check_output(['git','-C',str(source),'status','--porcelain'],text=True):raise SystemExit('Upstream must be an unmodified checkout')
web=output/'html-document/.htdocs';shutil.copytree(root/'full-port/web',web)
game=web/'game';records=[];total=0
for name in names:
    if not name:continue
    src=source/name
    if src.is_symlink() or not src.is_file():raise SystemExit('Unsupported source entry: '+name)
    dst=game/name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst)
    h=hashlib.sha256()
    with src.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
    actual=hashlib.sha256(dst.read_bytes()).hexdigest()
    if actual!=h.hexdigest():raise SystemExit('Copy mismatch: '+name)
    records.append({'path':name,'bytes':src.stat().st_size,'sha256':actual});total+=src.stat().st_size
manifest={'repository':'https://github.com/lieren2023/noname-for-dummies','commit':commit,'files':records,'file_count':len(records),'bytes':total,'excluded':['.git metadata only'],'runtime_verified':False}
(output/'payload-manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
(web/'payload-meta.js').write_text('window.WUMINGSHA_PAYLOAD='+json.dumps({'included':True,'commit':commit,'files':len(records),'bytes':total})+';\n')
print(json.dumps({'files':len(records),'bytes':total,'commit':commit,'output':str(output)}))
