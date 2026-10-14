from pathlib import Path
import re
import markdown

root=Path(__file__).resolve().parents[1]
repo=root.parents[1]
for doc in (repo/'README.md',root/'README.md',repo/'wiki/Presence-and-MAVLink.md'):
    content=doc.read_text(encoding='utf-8')
    assert not re.search(r'\[Wraith[^\]]*\]\(https?://',content,re.I), 'Keep aircraft imagery self-contained'
    assert not re.search(r'CAD[^\n]*from[^\n]*repositor',content,re.I), 'Do not disclose CAD source attribution'
    images=re.findall(r'!\[([^\]]*)\]\(([^)]+)\)',content)
    assert len(images)>=3,doc
    for alt,ref in images:
        assert alt.strip() and (doc.parent/ref).is_file(),ref
    for ref in re.findall(r'(?<!!)\[[^\]]*\]\(([^)]+)\)',content):
        if ':' not in ref and not ref.startswith('#'):
            assert (doc.parent/ref.split('#')[0]).exists(),ref
    assert 'simulated' in content and '115200' in content
    html=markdown.markdown(content,extensions=['tables','fenced_code'])
    name='wiki-guide.html' if doc.parent.name=='wiki' else 'readme.html' if doc.parent==repo else 'firmware-readme.html'
    out=root/'.build/portal-preview'/name
    out.write_text('<!doctype html><html><head><meta charset="utf-8"><base href="'+doc.parent.as_uri()+'/">'
        '<style>body{font:16px system-ui;line-height:1.55;color:#24292f;max-width:1000px;margin:40px auto;padding:20px}'
        'img{max-width:100%}table{border-collapse:collapse}td,th{border:1px solid #d0d7de;padding:10px}'
        'pre{background:#f6f8fa;padding:16px;overflow:auto}a{color:#0969da}</style></head><body>'+html+'</body></html>',encoding='utf-8')
    print('PASS',doc,len(images),'images; all local links resolve')
