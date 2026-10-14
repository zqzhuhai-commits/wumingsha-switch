#!/usr/bin/env python3
"""Build an offline web-app NSP with a proper HtmlDocument content record.
--payload-stage points to output from stage_full_package.py. Without it, build a small runtime probe.
"""
import argparse,json,os,re,shutil,subprocess,tempfile,math,struct,wave,hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--keyset',required=True,type=Path);parser.add_argument('--hacpack',required=True);parser.add_argument('--payload-stage',type=Path);parser.add_argument('--output',type=Path);args=parser.parse_args()
dkp=Path(os.environ['DEVKITPRO']);tools=dkp/'tools/bin';cc=dkp/'devkitA64/bin/aarch64-none-elf-gcc'
keydata=dict(re.findall(r'^\s*(\w+)\s*=\s*([0-9a-fA-F]+)\s*$',args.keyset.read_text(),re.M));needed=['header_key','key_area_key_application_00']
if not all(k in keydata for k in needed):raise SystemExit('Missing private packing keys')
output=(args.output or root/'dist/wumingsha-runtime-probe-v0.2.0.nsp').resolve()
if args.payload_stage and not args.output:raise SystemExit('Full payload requires an explicit output path; do not overwrite the small probe')
with tempfile.TemporaryDirectory(prefix='wumingsha-web-') as tmp:
    tmp=Path(tmp)
    for n in ['exefs','control','program','manual','meta','nsp','temp','backup']:(tmp/n).mkdir()
    keys=tmp/'private.keys';keys.write_text(''.join(k+' = '+keydata[k]+'\n' for k in needed));keys.chmod(0o600)
    title='0100A53547540000'
    config=json.loads((root/'packaging/application.json').read_text());config['name']='wuming-web'
    for n in ['program_id','program_id_range_min','program_id_range_max']:config[n]='0x'+title
    config['filesystem_access']['content_owner_ids']=['0x'+title]
    (tmp/'application.json').write_text(json.dumps(config))
    subprocess.run([str(cc),'-std=gnu11','-O2','-Wall','-Wextra','-Werror','-march=armv8-a+crc+crypto','-mtune=cortex-a57','-mtp=soft','-fPIE','-D__SWITCH__','-I'+str(dkp/'libnx/include'),'-specs='+str(dkp/'libnx/switch.specs'),str(root/'full-port/native/main.c'),'-L'+str(dkp/'libnx/lib'),'-lnx','-o',str(tmp/'runtime.elf')],check=True)
    subprocess.run([str(tools/'elf2nso'),str(tmp/'runtime.elf'),str(tmp/'exefs/main')],check=True)
    subprocess.run([str(tools/'npdmtool'),str(tmp/'application.json'),str(tmp/'exefs/main.npdm')],check=True)
    subprocess.run([str(tools/'nacptool'),'--create','无名杀 引擎验证','zqzhuhai / noname community','0.2.0',str(tmp/'control/control.nacp'),'--titleid='+title],check=True)
    n=tmp/'control/control.nacp';b=bytearray(n.read_bytes())
    for i in range(16):b[i*0x300:(i+1)*0x300]=b[:0x300]
    b[0x302c:0x3030]=(0xffff).to_bytes(4,'little');n.write_bytes(b)
    for lang in ['AmericanEnglish','BritishEnglish','Japanese','French','German','LatinAmericanSpanish','Spanish','Italian','Dutch','CanadianFrench','Portuguese','Russian','Korean','TraditionalChinese','SimplifiedChinese','BrazilianPortuguese']:
        shutil.copy2(root/'icon.jpg',tmp/'control'/('icon_'+lang+'.dat'))
    if args.payload_stage:
        manual=args.payload_stage.resolve()
        if not (manual/'payload-manifest.json').exists():raise SystemExit('Not a verified full-package stage')
        if not (manual/'html-document/.htdocs/game/index.html').exists():raise SystemExit('Missing original entry')
        manifest=json.loads((manual/'payload-manifest.json').read_text())
        for entry in manifest['files']:
            original=manual/'html-document/.htdocs/game'/entry['path']
            if original.stat().st_size!=entry['bytes']:raise SystemExit('Original file changed: '+entry['path'])
    else:
        manual=tmp/'manual-files';web=manual/'html-document/.htdocs';shutil.copytree(root/'full-port/web',web)
        with wave.open(str(web/'tone.wav'),'wb') as f:
            f.setnchannels(1);f.setsampwidth(2);f.setframerate(16000)
            f.writeframes(b''.join(struct.pack('<h',int(math.sin(2*math.pi*440*i/16000)*9000)) for i in range(4800)))
    def invoke(arguments):
        subprocess.run([args.hacpack,'-k',str(keys),'--titleid',title,'--tempdir',str(tmp/'temp'),'--backupdir',str(tmp/'backup')]+arguments,check=True)
    def nca(kind,out,extra):
        invoke(['--type','nca','--ncatype',kind,'-o',str(out)]+extra)
        files=list(out.glob('*.nca'))
        if len(files)!=1:raise SystemExit('Unexpected NCA output')
        return files[0]
    program=nca('program',tmp/'program',['--exefsdir',str(tmp/'exefs'),'--noselfsignncasig2'])
    control=nca('control',tmp/'control-nca',['--romfsdir',str(tmp/'control')])
    html=nca('manual',tmp/'manual',['--romfsdir',str(manual)])
    meta=nca('meta',tmp/'meta',['--titletype','application','--programnca',str(program),'--controlnca',str(control),'--htmldocnca',str(html)])
    all_nca=tmp/'all-nca';all_nca.mkdir()
    for p in [program,control,html,meta]:shutil.copy2(p,all_nca/p.name)
    invoke(['--type','nsp','--ncadir',str(all_nca),'-o',str(tmp/'nsp')])
    nsps=list((tmp/'nsp').glob('*.nsp'))
    if len(nsps)!=1:raise SystemExit('Unexpected NSP output')
    # Do not trust a successful external packer exit code without length checks.
    with nsps[0].open('rb') as f:
        h=f.read(16)
        if h[:4]!=b'PFS0':raise SystemExit('Output is not an installable PFS0 container')
        count,strings=struct.unpack_from('<II',h,4);entries=f.read(count*24)
        expected=16+count*24+strings+max(sum(struct.unpack_from('<QQ',entries,i*24)) for i in range(count))
        if expected!=nsps[0].stat().st_size:raise SystemExit('NSP output was truncated')
    output.parent.mkdir(parents=True,exist_ok=True)
    digest=hashlib.sha256();written=0
    # Use checked buffered writes instead of an opaque large-file fast copy.
    with nsps[0].open('rb') as src,output.open('wb') as dst:
        for chunk in iter(lambda:src.read(4*1024*1024),b''):
            if dst.write(chunk)!=len(chunk):raise SystemExit('Short NSP output write')
            digest.update(chunk);written+=len(chunk)
        dst.flush();os.fsync(dst.fileno())
    if written!=expected:raise SystemExit('Incomplete NSP output stream')
    if output.stat().st_size!=expected:raise SystemExit('NSP output copy was truncated')
    output.with_suffix(output.suffix+'.sha256').write_text(digest.hexdigest()+'  '+output.name+'\n')
    print('Created',output,'bytes',output.stat().st_size,'full original payload',bool(args.payload_stage))
