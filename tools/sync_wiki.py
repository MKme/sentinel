"""Synchronize source wiki pages into an existing GitHub wiki checkout.

Run: python tools/sync_wiki.py --target ../sentinel-wiki [--check]
Inspect both Git working trees first. This writes documentation/assets only;
it never deletes files, commits, pushes, or changes branches.
"""
import argparse
import re
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--target', type=Path, required=True)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    source = repo / 'wiki'
    target = args.target.resolve()
    if not (target / '.git').exists() or target == source.resolve():
        raise SystemExit('Target must be a separate existing wiki Git checkout')
    pages = sorted(source.glob('*.md'))
    expected = {}
    for page in pages:
        content = page.read_text(encoding='utf-8')

        def link(match):
            prefix, ref = match.groups()
            path, sep, anchor = ref.partition('#')
            if ':' in path or not path:
                return match.group(0)
            resolved = (page.parent / path).resolve()
            if not resolved.is_file():
                raise ValueError(f'{page.name}: missing {ref}')
            if path.startswith('../assets/'):
                destination = path[3:]
                expected[destination] = resolved.read_bytes()
                destination = 'https://raw.githubusercontent.com/wiki/MKme/sentinel/' + destination
            elif path.startswith('../'):
                destination = 'https://github.com/MKme/sentinel/blob/main/' + path[3:]
            elif path.endswith('.md'):
                destination = path[:-3]
            else:
                destination = path
                expected[destination] = resolved.read_bytes()
            return prefix + destination + (sep + anchor if sep else '') + ')'

        transformed = re.sub(r'(!?\[[^\]]*\]\()([^\)]+)\)', link, content)
        expected[page.name] = transformed.encode('utf-8')
    changed = []
    for name, payload in sorted(expected.items()):
        dest = target / name
        same = dest.is_file() and dest.read_bytes() == payload
        if not same:
            changed.append(name)
            if not args.check:
                dest.parent.mkdir(parents=True, exist_ok=True)
                dest.write_bytes(payload)
    print(f'{len(pages)} pages; {len(expected)-len(pages)} assets; '
          f'{len(changed)} ' + ('differences' if args.check else 'updated'))
    if args.check and changed:
        print('\n'.join(changed))
        raise SystemExit(1)


if __name__ == '__main__':
    main()
