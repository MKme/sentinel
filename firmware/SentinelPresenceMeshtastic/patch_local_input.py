"""Install the reviewed local ingress hook in disposable pinned build staging only."""
from pathlib import Path
root = Path(__file__).resolve().parent / '.build/meshtastic'
p = root / 'src/mesh/MeshService.cpp'
s = p.read_text()
marker = '// Sentinel companion input: consume before any RF routing.'
if marker not in s:
    anchor = 'void MeshService::handleToRadio(meshtastic_MeshPacket &p)\n{'
    assert s.count(anchor) == 1, 'Pinned local ingress changed'
    s = '#include "modules/optional/SentinelPresence/SentinelPresence.h"\n' + s
    s = s.replace(anchor, anchor + '\n    ' + marker + '\n    if (p.which_payload_variant == meshtastic_MeshPacket_decoded_tag &&\n        sentinelLocalObservation(p.to, p.decoded.portnum, p.decoded.payload.bytes, p.decoded.payload.size)) return;')
    p.write_text(s)
