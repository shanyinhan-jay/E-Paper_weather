from pathlib import Path
import re

base = Path('d:/ink/E-Paper_weather/e_weather/icon')
small_dir = base / '3636'
medium_dir = base / '7272'

for p in small_dir.glob('*.c'):
    content = p.read_text(encoding='utf-8')
    content = re.sub(r'gImage_(\d+)', r'gImage_\1_S', content)
    p.write_text(content, encoding='utf-8')

for p in medium_dir.glob('*.c'):
    content = p.read_text(encoding='utf-8')
    content = re.sub(r'gImage_(\d+)', r'gImage_\1_M', content)
    p.write_text(content, encoding='utf-8')

print("Renamed array variables in 3636 and 7272 to avoid conflicts.")
