from pathlib import Path
import re
import markdown
root=Path(__file__).resolve().parents[1];repo=root.parents[1]
out=root/'.build/docs-preview';out.mkdir(parents=True,exist_ok=True)
docs=[repo/'README.md',root/'README.md',root/'OBSERVATIONS.md',*sorted((repo/'wiki').glob('*.md'))]
for i,doc in enumerate(docs):
    content=doc.read_text(encoding='utf-8')
    for alt,ref in re.findall(r'!\[([^\]]*)\]\(([^)]+)\)',content):
        assert alt.strip() and (doc.parent/ref).is_file(),(doc,ref)
    for ref in re.findall(r'(?<!!)\[[^\]]*\]\(([^)]+)\)',content):
        if ':' not in ref and not ref.startswith('#'):
            assert (doc.parent/ref.split('#')[0]).exists(),(doc,ref)
    html=markdown.markdown(content,extensions=['tables','fenced_code'])
    (out/f'{i:02d}-{doc.stem}.html').write_text('<!doctype html><html><meta charset="utf-8"><base href="'+doc.parent.as_uri()+'/"><style>body{font:16px system-ui;line-height:1.55;max-width:1000px;margin:40px auto;padding:20px}img{max-width:100%}td,th{border:1px solid #ccc;padding:10px}pre{overflow:auto}</style>'+html+'</html>',encoding='utf-8')
print(f'PASS {len(docs)} documents: all local links and images resolve')
