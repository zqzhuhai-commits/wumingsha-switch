#!/usr/bin/env python3
"""Build a standalone homebrew NSP using private local keys.
Requires devkitPro, hacBrewPack, and an existing development keyset.
Never puts the keyset in the repository, staging tree, or output NSP.
"""
import argparse
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--keyset', required=True, type=Path)
parser.add_argument('--hacbrewpack', default='hacbrewpack')
args = parser.parse_args()
keys = dict(re.findall(r'^\s*([a-zA-Z0-9_]+)\s*=\s*([0-9a-fA-F]+)\s*$', args.keyset.read_text(), re.M))
required = ['header_key', 'key_area_key_application_00']
if not all(k in keys for k in required):
    raise SystemExit('Keyset lacks required homebrew packaging keys')
subprocess.run(['make', '-j2'], cwd=root, check=True)
with tempfile.TemporaryDirectory(prefix='sanguosha-nsp-') as temp:
    temp = Path(temp)
    for folder in ['exefs','control','nsp','nca','temp']:
        (temp/folder).mkdir()
    keyset = temp/'private.keys'
    keyset.write_text(''.join(k+' = '+keys[k]+'\n' for k in required))
    keyset.chmod(0o600)
    subprocess.run(['elf2nso', str(root/'noname-switch.elf'), str(temp/'exefs/main')], check=True)
    subprocess.run(['npdmtool', str(root/'packaging/application.json'), str(temp/'exefs/main.npdm')], check=True)
    subprocess.run(['nacptool', '--create', '无名杀 Switch版', 'zqzhuhai / noname community', '0.1.1', str(temp/'control/control.nacp'), '--titleid=0100A53547530000'], check=True)
    # Older nacptool fills only 12 language slots; include Chinese and Korean.
    nacp_file = temp/'control/control.nacp'
    nacp = bytearray(nacp_file.read_bytes())
    for language in range(16):
        nacp[language*0x300:(language+1)*0x300] = nacp[:0x300]
    nacp[0x302C:0x3030] = (0xFFFF).to_bytes(4, 'little')
    nacp_file.write_bytes(nacp)
    languages = ['AmericanEnglish','BritishEnglish','Japanese','French','German',
                 'LatinAmericanSpanish','Spanish','Italian','Dutch','CanadianFrench',
                 'Portuguese','Russian','Korean','TraditionalChinese','SimplifiedChinese',
                 'BrazilianPortuguese']
    for language in languages:
        shutil.copy2(root/'icon.jpg', temp/'control'/('icon_'+language+'.dat'))
    subprocess.run([args.hacbrewpack, '-k', str(keyset), '--exefsdir', str(temp/'exefs'), '--romfsdir', str(root/'romfs'), '--controldir', str(temp/'control'), '--nspdir', str(temp/'nsp'), '--ncadir', str(temp/'nca'), '--tempdir', str(temp/'temp'), '--nologo'], check=True)
    output = root/'dist/sanguosha-switch-v0.1.1.nsp'
    output.parent.mkdir(exist_ok=True)
    shutil.copy2(temp/'nsp/0100a53547530000.nsp', output)
    print('Created:', output)
