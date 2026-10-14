"""Prepare an isolated build using only the installed local Meshtastic source/cache."""
import argparse
import pathlib
import shutil
import subprocess
import json

HERE = pathlib.Path(__file__).resolve().parent
REF = '47e129f4bdec33a5012851aa3546073aa87d9254'
p = argparse.ArgumentParser()
p.add_argument('--source', type=pathlib.Path, default=pathlib.Path('C:/GitHub/lilygo/meshtastic-firmware'))
p.add_argument('--platform', type=pathlib.Path, default=pathlib.Path.home()/'.platformio/platforms/espressif32@6.9.0')
a = p.parse_args()
source = a.source.resolve()
dest = HERE/'.build/meshtastic'
if subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip() != REF:
    raise SystemExit('Local Meshtastic revision differs from the supported pin')
libs = source/'.pio/libdeps/t-watch-s3'
if not libs.is_dir() or not a.platform.is_dir():
    raise SystemExit('Installed local platform/library cache missing; no downloads attempted')
if dest.exists():
    raise SystemExit('Build staging already exists. Preserve it; use build.ps1 for incremental builds.')
dest.mkdir(parents=True)
# Only tracked files; no Git metadata or unrelated uncommitted board edit is copied.
files = subprocess.check_output(['git','-C',str(source),'ls-files','-z']).decode().split('\0')
for name in filter(None, files):
    if name.startswith('.github/'): continue
    target = dest/name
    if not (source/name).is_file():
        continue
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_bytes(subprocess.check_output(['git','-C',str(source),'show',f'{REF}:{name}']))
modules = dest/'src/modules/Modules.cpp'
text = modules.read_text()
text = '#include "modules/optional/SentinelPresence/SentinelPresence.h"\n'+text
text = text.replace('void setupModules()\n{','void setupModules()\n{\n    setupSentinelPresence();',1)
modules.write_text(text)
power=dest/'src/PowerFSM.cpp'
text=power.read_text()
condition='if ((isRouter || config.power.is_power_saving) && !isWifiAvailable() && !isTrackerOrSensor) {'
assert text.count(condition)==1
text=text.replace(condition,'// Sentinel needs continuous reception. Preserve explicit shutdown and low-battery handling.\n    '+condition.replace('if ((', 'if (false && ('))
power.write_text(text)
shutil.copytree(HERE/'overlay',dest,dirs_exist_ok=True)
for library in libs.iterdir():
    if library.is_dir() and library.name!='Nanopb':
        shutil.copytree(library,dest/'lib'/library.name,
                        ignore=shutil.ignore_patterns('.git','examples','test','tests','__pycache__'))
# Meshtastic commits generated protobuf C files; only Nanopb runtime is needed.
# Use a local runtime copy without its unconditional pip-install generator hook.
runtime=dest/'lib/Nanopb'
runtime.mkdir(parents=True)
for file in (libs/'Nanopb').iterdir():
    if file.suffix in ('.c','.h') or file.name=='LICENSE.txt': shutil.copy2(file,runtime/file.name)
manifest=json.loads((libs/'Nanopb/library.json').read_text())
manifest['build'].pop('extraScript',None)
(runtime/'library.json').write_text(json.dumps(manifest,indent=2))
script=dest/'extra_scripts/esp32_extra.py'
text=script.read_text()
# Installed esptool 4.5.1 handles binary images without IntelHex. Do not run pip.
start=text.index('# IntelHex workaround');end=text.index('import esptool',start)
script.write_text(text[:start]+text[end:])
version=dest/'bin/readprops.py'
version.write_text(version.read_text().replace('suffix = sha','suffix = "sentinel1"'))
with (dest/'platformio.ini').open('a') as f:
    f.write('''
[env:sentinel-xiao-s3]
extends = env:seeed-xiao-s3
platform = PLATFORM
platform_packages =
upload_protocol =
lib_deps =
lib_extra_dirs =
lib_ldf_mode = chain
build_unflags =
  ${env:seeed-xiao-s3.build_unflags}
  -flto
build_flags =
  ${env:seeed-xiao-s3.build_flags}
  -DMESHTASTIC_EXCLUDE_PAXCOUNTER=1
  -DMESHTASTIC_EXCLUDE_WEBSERVER=1
  -DMESHTASTIC_EXCLUDE_DETECTIONSENSOR=1
  -DMESHTASTIC_EXCLUDE_AUDIO=1
  -DMESHTASTIC_EXCLUDE_GPS=1
  -DMESHTASTIC_EXCLUDE_SERIAL=1
build_src_filter =
  ${esp32s3_base.build_src_filter}
  -<mesh/http/>
  -<modules/esp32/PaxcounterModule.cpp>
'''.replace('PLATFORM',a.platform.resolve().as_posix()).replace('LIBS',libs.as_posix()))
print(dest)
