"""Local native regressions using the installed compiler; no downloads."""
from pathlib import Path
import subprocess
root = Path(__file__).resolve().parents[1]
out = root / '.build/ground-tests'
out.mkdir(parents=True, exist_ok=True)
for name in ('core_test', 'observation_test'):
    exe = out / (name + '.exe')
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    str(root/'tests'/f'{name}.cpp'), '-o', str(exe)], check=True)
    result = subprocess.run([str(exe)], check=True, capture_output=True, text=True)
    (out/(name+'.txt')).write_text(result.stdout)
    print(result.stdout, end='')
for name in ('ground_integration.py', 'power_integration.py', 'settings_test.py', 'check_public.py', 'render_portal.py'):
    subprocess.run(['python', '-X', 'utf8', str(root/'tests'/name)], check=True)
