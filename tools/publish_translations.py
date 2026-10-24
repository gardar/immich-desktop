"""Compile source catalogs into immutable downloads and a versioned manifest."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import xml.etree.ElementTree as ET


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--lrelease', default='lrelease')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    root = Path(__file__).resolve().parents[1]
    manifest = {'version': 1, 'sourceRevision': os.environ.get('GITHUB_SHA', ''), 'catalogs': {}}
    for source in sorted((root / 'translations').glob('immich_*.ts')):
        language = source.stem.removeprefix('immich_')
        if not re.fullmatch(r'[a-z]{2,3}(?:_[A-Za-z0-9]{2,8}){0,2}', language):
            raise ValueError(f'Invalid locale: {language}')
        if ET.parse(source).getroot().get('language') != language:
            raise ValueError(f'Catalog language differs from filename: {source}')
        output = args.output / f'immich_{language}.qm'
        subprocess.run([args.lrelease, '-nounfinished', str(source), '-qm', str(output)], check=True)
        data = output.read_bytes()
        if not 0 < len(data) <= 2 * 1024 * 1024:
            raise ValueError(f'Catalog exceeds download limits: {source}')
        digest = hashlib.sha256(data).hexdigest()
        output.replace(args.output / f'immich_{language}_{digest}.qm')
        manifest['catalogs'][language] = {'sha256': digest, 'size': len(data)}
    if not 0 < len(manifest['catalogs']) <= 64:
        raise ValueError('Expected 1–64 catalogs')
    if sum(entry['size'] for entry in manifest['catalogs'].values()) > 16 * 1024 * 1024:
        raise ValueError('Catalogs exceed the total download limit')
    (args.output / 'index.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
