"""Reject accidental aircraft source or controls in the public source fileset."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
repo=root.parents[1]
for path in (root/'overlay').rglob('*'):
    if path.is_file():
        assert not re.search(r'mavlink|sentinelFlight|SMF1|SMV1|flightEnabled|HardwareSerial',path.read_text(),re.I),path
for path in [repo/'README.md',root/'README.md',root/'OBSERVATIONS.md',*list((repo/'wiki').glob('*.md'))]:
    text=path.read_text(encoding='utf-8')
    assert not re.search(r'MAVLink|Wraith|XDRONE|Flight-Controller-Link|Presence-and-MAVLink',text,re.I),path
    for ref in re.findall(r'\[[^\]]*\]\(([^)]+)\)',text):
        if ':' not in ref and not ref.startswith('#'):
            assert (path.parent/ref.split('#')[0]).is_file(),(path,ref)
for name in ('page.html','body.html','wifi-section.html'):
    text=(repo/'site/sentinel'/name).read_text(encoding='utf-8')
    assert not re.search(r'MAVLink|Wraith|XDRONE|sn-flight|sn-telemetry',text,re.I),name
    for ref in re.findall(r'<img[^>]+src="([^"]+)"',text):
        assert ':' not in ref and (repo/'site/sentinel'/ref).is_file(),(name,ref)
assert not list((repo/'assets/presence').glob('wraith*'))
assert not (repo/'wiki/Flight-Controller-Link.md').exists()
print('PASS public source boundary and documentation links')
